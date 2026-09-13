#include <pch/pch.hpp>
#include <utilities/memory/memory.hpp>
#include <utilities/addresses/addresses.hpp>
#include <utilities/logging/logging.hpp>
#include <core/systems/systems.hpp>

namespace systems
{
    bool model_preview::initialize( )
    {
        m_initialized = true;
        m_current_texture = nullptr;
        logging::console::print( "[model_preview] initialized" );
        return true;
    }

    bool model_preview::on_generate_primitives(
        std::uintptr_t owner_entity,
        std::uint32_t owner_hash,
        std::uintptr_t scene_object,
        std::uintptr_t primitive_buffer,
        void( __fastcall* original_fn )( std::uintptr_t, std::uintptr_t, std::uintptr_t, std::uintptr_t ),
        std::uintptr_t a1,
        std::uintptr_t scene_view )
    {
        if ( !m_initialized || !owner_entity || !scene_object )
            return false;

        if ( owner_hash != "C_CSGO_PreviewPlayer"_hash )
            return false;

        if ( auto* data = reinterpret_cast< c_generate_primitives_data* >( scene_object ) )
        {
            const auto capture_layer = [ this ]( c_scene_layer* scene_layer )
            {
                if ( !scene_layer || !scene_layer->m_texture_handle || !scene_layer->m_texture_handle->m_texture )
                {
                    return;
                }

                m_current_texture = scene_layer->m_texture_handle->m_texture;
            };

            capture_layer( data->m_scene_layer );
            if ( !m_current_texture )
            {
                capture_layer( data->m_scene_layer_2 );
            }
        }

        return false;
    }
}