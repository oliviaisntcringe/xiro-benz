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
        m_current_texture.store( 0, std::memory_order_release );
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
        if ( !m_initialized || owner_entity < 0x10000 || scene_object < 0x10000 ||
            owner_entity == static_cast< std::uintptr_t >( -1 ) ||
            scene_object == static_cast< std::uintptr_t >( -1 ) )
            return false;

        if ( owner_hash != "C_CSGO_PreviewPlayer"_hash )
            return false;

        // Keep the branch's stable behavior: preview rendering is UI-only and
        // does not inspect scene-layer texture internals from this worker hook.
        // The ImGui menu remains available and displays its waiting state.
        return false;
    }
}
