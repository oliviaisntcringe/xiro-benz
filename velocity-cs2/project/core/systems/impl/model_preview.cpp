#include <pch/pch.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <new>
#include <vector>

#include <Windows.h>
#include <d3d11.h>

#include <core/features/changer/impl/econ_item_attribute_manager.hpp>
#include <core/features/misc/misc.hpp>
#include <core/systems/systems.hpp>
#include <protection/game_addresses.hpp>
#include <utilities/addresses/addresses.hpp>
#include <utilities/hooking/hooking.hpp>
#include <utilities/logging/logging.hpp>
#include <utilities/memory/memory.hpp>

namespace systems
{
    namespace
    {
        constexpr std::ptrdiff_t BLOCK_CTOR_CALL = 0x158;
        constexpr std::ptrdiff_t PARSE_ARRAY_CALL = 0x2AD;
        constexpr std::ptrdiff_t BLOCK_DTOR_CALL = 0x2CC;
        constexpr std::uint32_t PREVIEW_WIDTH = 1024;
        constexpr std::uint32_t PREVIEW_HEIGHT = 768;
        constexpr std::uint64_t SETTLE_MS = 35;
        constexpr std::uint64_t RETRY_MS = 500;
        constexpr std::uint64_t TIMEOUT_MS = 5000;

        using block_ctor_fn = void* ( __fastcall* )( void*, std::int64_t, char );
        using parse_array_fn = bool ( __fastcall* )( void*, const void*, int );
        using block_dtor_fn = void ( __fastcall* )( void* );
        using create_item_fn = void* ( __fastcall* )( const void*, std::uint64_t* );
        using render_targets_fn = void ( __fastcall* )( void* );
        using acquire_layer_fn = void* ( __fastcall* )( void*, void* );

        struct state_t
        {
            std::mutex mutex{};
            model_preview_request pending{};
            std::uint64_t revision{};
            std::uint64_t processed_revision{};
            std::uint64_t last_attempt_revision{};
            ULONGLONG changed_at{};
            ULONGLONG last_attempt_at{};
            ULONGLONG last_request_at{};

            block_ctor_fn block_ctor{};
            parse_array_fn parse_array{};
            block_dtor_fn block_dtor{};
            create_item_fn create_item{};
            hooking::jmp render_targets_hook{};
            hooking::jmp acquire_layer_hook{};

            std::uintptr_t main_menu_pointer{};
            std::atomic<model_preview_status> status{ model_preview_status::uninitialized };
            std::atomic<ID3D11ShaderResourceView*> staged{};
            std::atomic<ID3D11ShaderResourceView*> current{};
            std::atomic<std::uint32_t> width{};
            std::atomic<std::uint32_t> height{};
            std::atomic_bool capture_reported{};
            std::atomic_bool promote_reported{};
            std::atomic_bool capture_attempt_reported{};

            ID3D11Device* device{};
            ID3D11DeviceContext* context{};
            features::misc::c_ui_panel* panel_context{};
            std::uint64_t item_id{};
            std::uint64_t panel_revision{};
            char panel_id[ 96 ]{};
            char texture_name[ 96 ]{};
            bool render_ready{};
            bool panel_active{};
        };

        state_t* g_state{};

        bool valid_pointer( std::uintptr_t address )
        {
            return address >= 0x10000 && address != static_cast< std::uintptr_t >( -1 );
        }

        bool executable( std::uintptr_t address )
        {
            if ( !valid_pointer( address ) )
                return false;

            MEMORY_BASIC_INFORMATION info{};
            if ( VirtualQuery( reinterpret_cast< const void* >( address ), &info, sizeof( info ) ) != sizeof( info ) )
                return false;

            const auto protection = info.Protect & 0xFFu;
            return info.State == MEM_COMMIT &&
                ( protection == PAGE_EXECUTE || protection == PAGE_EXECUTE_READ ||
                    protection == PAGE_EXECUTE_READWRITE || protection == PAGE_EXECUTE_WRITECOPY );
        }

        template <typename T>
        T resolve_call( std::uintptr_t base, std::ptrdiff_t offset )
        {
            const auto call = base + offset;
            if ( !executable( call ) || memory::safe_read<std::uint8_t>( call ).value_or( 0 ) != 0xE8 )
                return nullptr;

            const auto displacement = memory::safe_read<std::int32_t>( call + 1 );
            if ( !displacement )
                return nullptr;

            const auto target = call + 5 + static_cast< std::intptr_t >( *displacement );
            return executable( target ) ? reinterpret_cast< T >( target ) : nullptr;
        }

        std::uintptr_t rip_target( std::uintptr_t instruction, std::size_t displacement_offset, std::size_t instruction_size )
        {
            const auto displacement = memory::safe_read<std::int32_t>( instruction + displacement_offset );
            if ( !displacement )
                return 0;

            return instruction + instruction_size + static_cast< std::intptr_t >( *displacement );
        }

        class proto_writer
        {
        public:
            explicit proto_writer( std::size_t reserve = 128 ) { m_data.reserve( reserve ); }

            void uint32( std::uint32_t field, std::uint64_t value )
            {
                varint( ( static_cast< std::uint64_t >( field ) << 3 ) | 0u );
                varint( value );
            }

            void float32( std::uint32_t field, float value )
            {
                std::uint32_t bits{};
                std::memcpy( &bits, &value, sizeof( bits ) );
                varint( ( static_cast< std::uint64_t >( field ) << 3 ) | 5u );
                for ( auto i = 0; i < 4; ++i )
                    m_data.push_back( static_cast< std::uint8_t >( bits >> ( i * 8 ) ) );
            }

            void message( std::uint32_t field, const proto_writer& nested )
            {
                if ( nested.m_data.empty( ) )
                    return;
                varint( ( static_cast< std::uint64_t >( field ) << 3 ) | 2u );
                varint( nested.m_data.size( ) );
                m_data.insert( m_data.end( ), nested.m_data.begin( ), nested.m_data.end( ) );
            }

            [[nodiscard]] std::vector<std::uint8_t> take( ) { return std::move( m_data ); }

        private:
            void varint( std::uint64_t value )
            {
                while ( value >= 0x80 )
                {
                    m_data.push_back( static_cast< std::uint8_t >( value ) | 0x80u );
                    value >>= 7;
                }
                m_data.push_back( static_cast< std::uint8_t >( value ) );
            }

            std::vector<std::uint8_t> m_data{};
        };

        std::vector<std::uint8_t> serialize_request( const model_preview_request& request )
        {
            proto_writer outer( 256 );
            outer.uint32( 3, request.definition_index );
            if ( request.paint_kit > 0 )
            {
                outer.uint32( 4, request.paint_kit );
                if ( request.rarity > 0 )
                    outer.uint32( 5, request.rarity );

                std::uint32_t wear_bits{};
                const auto wear = std::clamp( request.wear, 0.0001f, 1.0f );
                std::memcpy( &wear_bits, &wear, sizeof( wear_bits ) );
                outer.uint32( 7, wear_bits );
                outer.uint32( 8, std::clamp( request.seed, 0, 1000 ) );
            }

            if ( request.stattrak )
            {
                outer.uint32( 9, 0 );
                outer.uint32( 10, 0 );
            }

            for ( std::size_t slot{}; slot < request.stickers.size( ); ++slot )
            {
                const auto& sticker = request.stickers[ slot ];
                if ( !sticker.enabled || sticker.kit <= 0 )
                    continue;

                proto_writer nested( 64 );
                nested.uint32( 1, slot );
                nested.uint32( 2, sticker.kit );
                nested.float32( 3, std::clamp( sticker.wear, 0.0f, 1.0f ) );
                nested.float32( 4, std::clamp( sticker.scale, 0.1f, 5.0f ) );
                nested.float32( 5, std::clamp( sticker.rotation, -180.0f, 180.0f ) );
                if ( sticker.offset_x != 0.0f )
                    nested.float32( 7, std::clamp( sticker.offset_x, -0.5f, 0.5f ) );
                if ( sticker.offset_y != 0.0f )
                    nested.float32( 8, std::clamp( sticker.offset_y, -0.5f, 0.5f ) );
                outer.message( 12, nested );
            }

            if ( request.keychain.enabled && request.keychain.id > 0 )
            {
                proto_writer nested( 64 );
                nested.uint32( 1, 0 );
                nested.uint32( 2, request.keychain.id );
                if ( request.keychain.offset_x != 0.0f ) nested.float32( 7, request.keychain.offset_x );
                if ( request.keychain.offset_y != 0.0f ) nested.float32( 8, request.keychain.offset_y );
                if ( request.keychain.offset_z != 0.0f ) nested.float32( 9, request.keychain.offset_z );
                nested.uint32( 10, std::clamp( request.keychain.seed, 0, 100000 ) );
                outer.message( 20, nested );
            }

            return outer.take( );
        }

        void release_srv( std::atomic<ID3D11ShaderResourceView*>& slot )
        {
            auto* value = slot.exchange( nullptr, std::memory_order_acq_rel );
            if ( value )
                value->Release( );
        }

        void __fastcall render_targets_detour( void* render_target )
        {
            if ( !g_state )
                return;
            const auto original = g_state->render_targets_hook.original<void ( __fastcall* )( void* )>( );
            if ( original )
                original( render_target );
            g_model_preview.capture_render_target( render_target );
        }

        void* __fastcall acquire_layer_detour( void* cache, void* layer_desc )
        {
            if ( !g_state )
                return nullptr;
            const auto original = g_state->acquire_layer_hook.original<void* ( __fastcall* )( void*, void* )>( );
            const auto target = original ? original( cache, layer_desc ) : nullptr;
            g_model_preview.capture_render_target( target );
            return target;
        }

        features::misc::c_ui_panel* read_root( std::uintptr_t pointer )
        {
            if ( !valid_pointer( pointer ) )
                return nullptr;
            const auto root = memory::safe_read<std::uintptr_t>( pointer ).value_or( 0 );
            return valid_pointer( root ) ? memory::safe_read<features::misc::c_ui_panel*>( root + 0x8 ).value_or( nullptr ) : nullptr;
        }

        bool build_native_item( state_t& state, const std::vector<std::uint8_t>& protobuf, void*& item_view, std::uint64_t& item_id )
        {
            alignas( 16 ) std::array<std::uint8_t, 0x100> block{};
            bool parsed = false;
            item_view = nullptr;
            item_id = 0;
            __try
            {
                state.block_ctor( block.data( ), 0, 0 );
                parsed = state.parse_array( block.data( ), protobuf.data( ), static_cast< int >( protobuf.size( ) ) );
                if ( parsed )
                    item_view = state.create_item( block.data( ), &item_id );
                state.block_dtor( block.data( ) );
            }
            __except ( EXCEPTION_EXECUTE_HANDLER )
            {
                parsed = false;
                item_view = nullptr;
                item_id = 0;
            }
            return parsed && item_view && item_id;
        }

        bool run_script_safe( features::misc::c_ui_engine* engine, features::misc::c_ui_panel* panel, const char* script )
        {
            if ( !engine || !panel || !script )
                return false;

            const auto engine_address = reinterpret_cast< std::uintptr_t >( engine );
            const auto vtable = memory::safe_read<std::uintptr_t>( engine_address ).value_or( 0 );
            const auto function = vtable
                ? memory::safe_read<std::uintptr_t>( vtable + 77 * sizeof( std::uintptr_t ) ).value_or( 0 )
                : 0;
            const auto panorama_begin = addresses::modules::panorama;
            const auto panorama_end = panorama_begin + memory::get_module_size( panorama_begin );
            if ( !executable( function ) || !panorama_begin || function < panorama_begin || function >= panorama_end )
                return false;

            __try
            {
                engine->run_script( panel, script );
                return true;
            }
            __except ( EXCEPTION_EXECUTE_HANDLER )
            {
                return false;
            }
        }
    }

    bool model_preview::initialize( )
    {
        if ( m_initialized )
            return true;

        m_initialized = true;
        m_current_texture.store( 0, std::memory_order_release );
        g_state = new ( std::nothrow ) state_t{};
        if ( !g_state )
            return true;

        const auto creator = memory::resolve_pattern( patterns::preview_create_item.data.data );
        const auto decoder = memory::resolve_pattern( patterns::preview_decode_string.data.data );
        const auto render_targets = memory::resolve_pattern( patterns::preview_render_targets.data.data );
        const auto acquire_layer = memory::resolve_pattern( patterns::preview_acquire_layer_rt.data.data );
        const auto main_menu = memory::resolve_pattern( patterns::preview_main_menu_panel.data.data );

        g_state->create_item = executable( creator ) ? reinterpret_cast<create_item_fn>( creator ) : nullptr;
        g_state->block_ctor = resolve_call<block_ctor_fn>( decoder, BLOCK_CTOR_CALL );
        g_state->parse_array = resolve_call<parse_array_fn>( decoder, PARSE_ARRAY_CALL );
        g_state->block_dtor = resolve_call<block_dtor_fn>( decoder, BLOCK_DTOR_CALL );
        g_state->main_menu_pointer = main_menu ? rip_target( main_menu, 5, 9 ) : 0;

        if ( !g_state->create_item || !g_state->block_ctor || !g_state->parse_array ||
            !g_state->block_dtor || !executable( render_targets ) )
        {
            g_state->status.store( model_preview_status::missing_patterns, std::memory_order_release );
            logging::console::print( xs( "[model_preview] unavailable: current-build signatures not complete" ) );
            return true;
        }

        if ( !g_state->render_targets_hook.create( reinterpret_cast< void* >( render_targets ), reinterpret_cast< void* >( &render_targets_detour ) ) ||
            !g_state->render_targets_hook.enable( ) )
        {
            g_state->status.store( model_preview_status::missing_patterns, std::memory_order_release );
            logging::console::print( xs( "[model_preview] unavailable: render target hook failed" ) );
            return true;
        }

        if ( executable( acquire_layer ) &&
            g_state->acquire_layer_hook.create( reinterpret_cast< void* >( acquire_layer ), reinterpret_cast< void* >( &acquire_layer_detour ) ) )
            g_state->acquire_layer_hook.enable( );

        g_state->status.store( model_preview_status::waiting_for_request, std::memory_order_release );
        logging::console::print( xs( "[model_preview] native Panorama preview initialized" ) );
        return true;
    }

    bool model_preview::initialize_render( ID3D11Device* device, ID3D11DeviceContext* context )
    {
        if ( !g_state || !device || !context )
            return false;
        if ( g_state->render_ready )
            return true;

        g_state->device = device;
        g_state->context = context;
        device->AddRef( );
        context->AddRef( );
        g_state->render_ready = true;
        logging::console::print( xs( "[model_preview] render context ready" ) );
        return true;
    }

    void model_preview::shutdown( )
    {
        if ( !g_state )
        {
            m_initialized = false;
            return;
        }

        g_state->render_targets_hook.reset( );
        g_state->acquire_layer_hook.reset( );
        release_srv( g_state->staged );
        release_srv( g_state->current );
        if ( g_state->context ) g_state->context->Release( );
        if ( g_state->device ) g_state->device->Release( );
        delete g_state;
        g_state = nullptr;
        m_current_texture.store( 0, std::memory_order_release );
        m_initialized = false;
    }

    void model_preview::submit( const model_preview_request& request )
    {
        if ( !g_state || !request.definition_index )
            return;

        std::lock_guard lock( g_state->mutex );
        if ( g_state->revision && g_state->pending == request )
        {
            g_state->last_request_at = GetTickCount64( );
            return;
        }
        g_state->pending = request;
        ++g_state->revision;
        g_state->changed_at = GetTickCount64( );
        g_state->last_request_at = g_state->changed_at;
        g_state->status.store( model_preview_status::waiting_for_item, std::memory_order_release );
    }

    void model_preview::set_rotation( float, float )
    {
        // Rotation is intentionally left to the native panel's camera for now. The
        // preview remains functional without writing into model scene memory.
    }

    model_preview_status model_preview::status( ) const
    {
        return g_state ? g_state->status.load( std::memory_order_acquire ) : model_preview_status::uninitialized;
    }

    const char* model_preview::status_text( ) const
    {
        switch ( status( ) )
        {
        case model_preview_status::missing_patterns: return "3D preview signatures unavailable for this build.";
        case model_preview_status::waiting_for_request: return "3D preview waiting for a selection.";
        case model_preview_status::waiting_for_item: return "3D preview building native item.";
        case model_preview_status::waiting_for_panel: return "3D preview waiting for Panorama.";
        case model_preview_status::waiting_for_texture: return "3D preview rendering.";
        case model_preview_status::ready: return "3D preview ready.";
        default: return "3D preview is not initialized.";
        }
    }

    ID3D11ShaderResourceView* model_preview::get_native_texture_srv( ) const
    {
        if ( !g_state )
            return nullptr;
        auto* value = g_state->current.load( std::memory_order_acquire );
        return valid_pointer( reinterpret_cast< std::uintptr_t >( value ) ) ? value : nullptr;
    }

    bool model_preview::native_has_texture( ) const
    {
        return get_native_texture_srv( ) != nullptr;
    }

    void model_preview::capture_render_target( void* render_target )
    {
        if ( !g_state || !g_state->render_ready || !render_target || !g_state->panel_active )
            return;

        if ( !g_state->capture_attempt_reported.exchange( true, std::memory_order_acq_rel ) )
            logging::console::print( xs( "[model_preview] render-target callback observed target=0x{:X} expected={}" ),
                reinterpret_cast< std::uintptr_t >( render_target ), g_state->texture_name );

        const auto base = reinterpret_cast< std::uintptr_t >( render_target );
        const auto string_flags = memory::safe_read<std::uint32_t>( base + 0x0C ).value_or( 0 );
        const auto name_address = ( string_flags & 0x40000000u )
            ? base + 0x10
            : memory::safe_read<std::uintptr_t>( base + 0x10 ).value_or( 0 );
        if ( !valid_pointer( name_address ) || !g_state->texture_name[ 0 ] )
            return;

        char name[ 96 ]{};
        __try { strncpy_s( name, reinterpret_cast< const char* >( name_address ), _TRUNCATE ); }
        __except ( EXCEPTION_EXECUTE_HANDLER ) { return; }
        if ( std::strcmp( name, g_state->texture_name ) != 0 )
            return;

        const auto flags = memory::safe_read<std::uint32_t>( base + 0x18 ).value_or( 0 );
        const auto post_process = memory::safe_read<std::uintptr_t>( base + 0xE0 ).value_or( 0 );
        const auto binding = ( flags & 0x2u ) && post_process
            ? post_process
            : memory::safe_read<std::uintptr_t>( base + 0xD8 ).value_or( 0 );
        if ( !valid_pointer( binding ) || memory::safe_read<std::int32_t>( binding + 0x20 ).value_or( 0 ) <= 0 )
            return;

        const auto texture = memory::safe_read<std::uintptr_t>( binding ).value_or( 0 );
        if ( !valid_pointer( texture ) )
            return;
        auto* view = reinterpret_cast< ID3D11ShaderResourceView* >( memory::safe_read<std::uintptr_t>( texture + 0x10 ).value_or( 0 ) );
        if ( !valid_pointer( reinterpret_cast< std::uintptr_t >( view ) ) )
            view = reinterpret_cast< ID3D11ShaderResourceView* >( memory::safe_read<std::uintptr_t>( texture + 0x18 ).value_or( 0 ) );
        if ( !valid_pointer( reinterpret_cast< std::uintptr_t >( view ) ) )
            return;

        __try { view->AddRef( ); }
        __except ( EXCEPTION_EXECUTE_HANDLER ) { return; }
        auto* previous = g_state->staged.exchange( view, std::memory_order_acq_rel );
        if ( previous ) previous->Release( );
        if ( !g_state->capture_reported.exchange( true, std::memory_order_acq_rel ) )
            logging::console::print( xs( "[model_preview] composition target captured texture=0x{:X}" ), reinterpret_cast< std::uintptr_t >( view ) );
    }

    void model_preview::tick_render_thread( )
    {
        if ( !g_state )
            return;
        auto* staged = g_state->staged.exchange( nullptr, std::memory_order_acq_rel );
        if ( !staged )
            return;

        ID3D11Resource* resource{};
        std::uint32_t width{};
        std::uint32_t height{};
        staged->GetResource( &resource );
        if ( resource )
        {
            ID3D11Texture2D* texture{};
            if ( SUCCEEDED( resource->QueryInterface( __uuidof( ID3D11Texture2D ), reinterpret_cast< void** >( &texture ) ) ) && texture )
            {
                D3D11_TEXTURE2D_DESC desc{};
                texture->GetDesc( &desc );
                width = desc.Width;
                height = desc.Height;
                texture->Release( );
            }
            resource->Release( );
        }
        if ( !width || !height )
        {
            staged->Release( );
            return;
        }
        auto* previous = g_state->current.exchange( staged, std::memory_order_acq_rel );
        g_state->width.store( width, std::memory_order_release );
        g_state->height.store( height, std::memory_order_release );
        g_state->status.store( model_preview_status::ready, std::memory_order_release );
        if ( !g_state->promote_reported.exchange( true, std::memory_order_acq_rel ) )
            logging::console::print( xs( "[model_preview] composition texture promoted size={}x{}" ), width, height );
        if ( previous ) previous->Release( );
    }

    void model_preview::process_main_thread( )
    {
        if ( !g_state || status( ) == model_preview_status::missing_patterns )
            return;

        const auto now = GetTickCount64( );
        model_preview_request request{};
        std::uint64_t revision{};
        {
            std::lock_guard lock( g_state->mutex );
            revision = g_state->revision;
            if ( !revision || revision == g_state->processed_revision )
                return;
            if ( now - g_state->changed_at < SETTLE_MS ||
                ( g_state->last_attempt_revision == revision && now - g_state->last_attempt_at < RETRY_MS ) )
                return;
            request = g_state->pending;
            g_state->last_attempt_revision = revision;
            g_state->last_attempt_at = now;
        }

        auto* root = read_root( addresses::globals::hud );
        if ( !root )
            root = read_root( g_state->main_menu_pointer );
        auto* panorama = reinterpret_cast< features::misc::c_panorama_ui_engine* >( addresses::globals::panorama );
        auto* engine = panorama ? panorama->get_ui_engine( ) : nullptr;
        if ( !root || !engine )
        {
            g_state->status.store( model_preview_status::waiting_for_panel, std::memory_order_release );
            return;
        }

        const auto protobuf = serialize_request( request );
        void* item_view{};
        std::uint64_t item_id{};
        const bool parsed = build_native_item( *g_state, protobuf, item_view, item_id );

        if ( !parsed || !item_view || !item_id )
        {
            g_state->status.store( model_preview_status::waiting_for_item, std::memory_order_release );
            logging::console::print( xs( "[model_preview] item build failed revision={} parsed={} id={} " ), revision, parsed ? 1u : 0u, item_id );
            return;
        }

        if ( std::any_of( request.stickers.begin( ), request.stickers.end( ), []( const auto& value ) { return value.enabled && value.kit > 0; } ) )
        {
            features::changer::econ_attributes::sticker_array stickers{};
            for ( std::size_t i{}; i < stickers.size( ); ++i )
            {
                stickers[ i ].enabled = request.stickers[ i ].enabled;
                stickers[ i ].kit = request.stickers[ i ].kit;
                stickers[ i ].wear = request.stickers[ i ].wear;
                stickers[ i ].scale = request.stickers[ i ].scale;
                stickers[ i ].rotation = request.stickers[ i ].rotation;
                stickers[ i ].offset_x = request.stickers[ i ].offset_x;
                stickers[ i ].offset_y = request.stickers[ i ].offset_y;
            }
            (void) features::changer::econ_attributes::sync_stickers( reinterpret_cast< std::uintptr_t >( item_view ), stickers );
        }

        if ( g_state->panel_active && g_state->panel_context && g_state->panel_id[ 0 ] )
        {
            char close_script[ 256 ]{};
            const auto close_length = std::snprintf( close_script, sizeof( close_script ),
                "(function(){var p=$.GetContextPanel().FindChildTraverse('%s');if(p&&p.IsValid())p.DeleteAsync(0.0);})();", g_state->panel_id );
            if ( close_length > 0 && static_cast< std::size_t >( close_length ) < sizeof( close_script ) )
                run_script_safe( engine, g_state->panel_context, close_script );
            g_state->panel_active = false;
        }

        g_state->panel_context = root;
        g_state->item_id = item_id;
        g_state->panel_revision = revision;
        g_state->capture_reported.store( false, std::memory_order_release );
        g_state->promote_reported.store( false, std::memory_order_release );
        g_state->capture_attempt_reported.store( false, std::memory_order_release );
        std::snprintf( g_state->panel_id, sizeof( g_state->panel_id ), "triada_preview_%llu", static_cast< unsigned long long >( revision ) );
        std::snprintf( g_state->texture_name, sizeof( g_state->texture_name ), "triada_preview_rt_%llu", static_cast< unsigned long long >( revision ) );

        char script[ 8192 ]{};
        const auto length = std::snprintf( script, sizeof( script ), R"JS((function(){
const root=$.GetContextPanel();
const id='%s',texture='%s',itemId=BigInt('%llu');
if(!root||typeof InventoryAPI==='undefined'||!InventoryAPI.IsValidItemID(itemId))return;
const container=$.CreatePanel('Panel',root,id+'_container',{hittest:false,style:'width:%upx;height:%upx;opacity:0.01;brightness:0.0;x:0px;y:0px;z-index:-99999;'});
if(!container)return;
const panel=$.CreatePanel('MapItemPreviewPanel',container,id,{'require-composition-layer':'true','composition-layer-texture-name':texture,'transparent-background':'false','disable-depth-of-field':'false',hide_while_waiting_for_composite_materials:'true','pin-fov':'vertical',camera:'cam_default',player:'true',map:'de_dust2_vanity',initial_entity:'item',mouse_rotate:'false',rotation_limit_x:'360',rotation_limit_y:'90',auto_rotate_x:'0',auto_rotate_y:'0',auto_recenter:false,panzoom_enabled:'true',style:'width:100%%;height:100%%;opacity:1.0;'});
if(!panel){container.DeleteAsync(0.0);return;}
panel.hittest=false;panel.hittestchildren=false;panel.Data().itemId=itemId;panel.Data().active_item_idx=0;
try{panel.SetActiveItem(0);panel.SetItemItemId(itemId,'');panel.SetReadyForDisplay(true);}catch(e){}
try{panel.SetRenderInterval(1);panel.SetHideStaticGeometry(true);panel.SetHideParticles(true);panel.SetTransparentBackground(true);}catch(e){}
})(); )JS", g_state->panel_id, g_state->texture_name, static_cast< unsigned long long >( item_id ), PREVIEW_WIDTH, PREVIEW_HEIGHT );

        if ( length <= 0 || static_cast< std::size_t >( length ) >= sizeof( script ) )
        {
            g_state->status.store( model_preview_status::waiting_for_panel, std::memory_order_release );
            return;
        }

        if ( !run_script_safe( engine, root, script ) )
        {
            g_state->status.store( model_preview_status::waiting_for_panel, std::memory_order_release );
            return;
        }

        g_state->panel_active = true;
        g_state->processed_revision = revision;
        g_state->status.store( model_preview_status::waiting_for_texture, std::memory_order_release );
        logging::console::print( xs( "[model_preview] panel submitted revision={} item={} def={}" ), revision, item_id, request.definition_index );
    }

    void model_preview::reset( )
    {
        m_current_texture.store( 0, std::memory_order_release );
        if ( !g_state ) return;
        g_state->panel_active = false;
        g_state->panel_context = nullptr;
        g_state->panel_id[ 0 ] = '\0';
        g_state->texture_name[ 0 ] = '\0';
        release_srv( g_state->staged );
        release_srv( g_state->current );
        g_state->width.store( 0, std::memory_order_release );
        g_state->height.store( 0, std::memory_order_release );
        g_state->status.store( model_preview_status::waiting_for_request, std::memory_order_release );
    }

    bool model_preview::on_generate_primitives(
        std::uintptr_t, std::uint32_t, std::uintptr_t, std::uintptr_t,
        void( __fastcall* )( std::uintptr_t, std::uintptr_t, std::uintptr_t, std::uintptr_t ),
        std::uintptr_t, std::uintptr_t )
    {
        return false;
    }
}
