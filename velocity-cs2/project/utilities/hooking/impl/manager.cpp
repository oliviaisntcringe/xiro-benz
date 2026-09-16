#include <pch/pch.hpp>
#include <utilities/security/security.hpp>
#include <utilities/diag.hpp>
#include <utilities/logging/logging.hpp>

#include "../hooking.hpp"

namespace hooking::manager {

	bool create( const std::initializer_list<entry>& entries )
	{
		for ( const auto& entry : entries )
		{
			diag::writef(
				diag::level::debug,
				"[hook] prepare name=%s target=0x%p detour=0x%p",
				entry.name ? entry.name : "unnamed",
				reinterpret_cast<void*>( entry.address ),
				entry.detour );

			if ( !entry.hook || !entry.detour )
			{
				diag::writef( diag::level::error, "[hook] invalid entry name=%s", entry.name ? entry.name : "unnamed" );
				logging::console::print( xs( "invalid hook entry: {}" ), entry.name );
				return false;
			}

			if ( !entry.address )
			{
				diag::writef( diag::level::error, "[hook] missing target name=%s", entry.name ? entry.name : "unnamed" );
				logging::console::print( xs( "failed to hook: {}" ), entry.name );
				return false;
			}
		}

		const auto rollback = [ &entries ]( )
		{
			for ( const auto& entry : entries )
			{
				if ( entry.hook )
				{
					entry.hook->reset( );
				}
			}
		};

		for ( const auto& entry : entries )
		{
			diag::writef(
				diag::level::debug,
				"[hook] create begin name=%s target=0x%p",
				entry.name ? entry.name : "unnamed",
				reinterpret_cast<void*>( entry.address ) );

			if ( !entry.hook->create( reinterpret_cast< void* >( entry.address ), entry.detour ) )
			{
				diag::writef( diag::level::error, "[hook] create failed name=%s target=0x%p", entry.name ? entry.name : "unnamed", reinterpret_cast<void*>( entry.address ) );
				logging::console::print( xs( "failed to hook: {}" ), entry.name );
				rollback( );
				return false;
			}

			diag::writef(
				diag::level::debug,
				"[hook] enable begin name=%s target=0x%p",
				entry.name ? entry.name : "unnamed",
				reinterpret_cast<void*>( entry.address ) );

			if ( !entry.hook->enable( ) )
			{
				diag::writef( diag::level::error, "[hook] enable failed name=%s target=0x%p", entry.name ? entry.name : "unnamed", reinterpret_cast<void*>( entry.address ) );
				logging::console::print( xs( "failed to enable hook: {}" ), entry.name );
				rollback( );
				return false;
			}

			security::prologues::add( entry.address, entry.hook->get_original_bytes( ), entry.hook->get_original_length( ) );
			diag::writef(
				diag::level::info,
				"[hook] enabled name=%s target=0x%p trampoline=0x%p original_length=%zu",
				entry.name ? entry.name : "unnamed",
				reinterpret_cast<void*>( entry.address ),
				entry.hook->get_trampoline( ),
				entry.hook->get_original_length( ) );
		}

		return true;
	}

} // namespace hooking::manager
