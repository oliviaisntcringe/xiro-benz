#include <pch/pch.hpp>

#include <cstdio>

#include <utilities/logging/logging.hpp>
#include <utilities/addresses/addresses.hpp>
#include <utilities/security/security.hpp>
#include <utilities/memory/memory.hpp>
#include <utilities/threadpool/threadpool.hpp>
#include <utilities/steam/steam.hpp>

#include <core/hooks/hooks.hpp>
#include <core/systems/systems.hpp>
#include <core/settings.hpp>
#include <core/features/features.hpp>
#include <core/rendering/rendering.hpp>

#include <utilities/diag.hpp>

namespace {

	std::atomic<LPTOP_LEVEL_EXCEPTION_FILTER> g_previous_exception_filter{};
	PVOID g_vectored_exception_handler{};

	LONG WINAPI diag_unhandled_exception_filter( EXCEPTION_POINTERS* info );
	LONG CALLBACK diag_vectored_exception_filter( EXCEPTION_POINTERS* info );

	bool install_exception_handlers( )
	{
		if ( !g_vectored_exception_handler )
		{
			g_vectored_exception_handler =
				AddVectoredExceptionHandler( 1, diag_vectored_exception_filter );
		}

		const auto previous_filter =
			SetUnhandledExceptionFilter( diag_unhandled_exception_filter );
		if ( previous_filter && previous_filter != diag_unhandled_exception_filter )
		{
			g_previous_exception_filter.store(
				previous_filter,
				std::memory_order_release );
		}

		if ( !g_vectored_exception_handler )
		{
			diag::writef(
				diag::level::error,
				"failed to install vectored exception handler; win32_error=%lu",
				GetLastError( ) );
			return false;
		}

		diag::write( diag::level::info, "crash handlers installed" );
		return true;
	}

#if defined( DEV )
	hooking::jmp g_minidump_hook{};
	hooking::jmp g_terminate_process_hook{};

	BOOL WINAPI diag_minidump_write_detour(
		HANDLE process,
		DWORD process_id,
		HANDLE file,
		unsigned long dump_type,
		diag::minidump_exception_information* exception,
		void* user_stream,
		void* callback )
	{
		const auto original =
			g_minidump_hook.original<diag::minidump_write_fn>( );
		if ( !diag::g_writing_minidump &&
			exception &&
			!exception->client_pointers &&
			exception->exception_pointers )
		{
			diag::record_crash(
				exception->exception_pointers,
				"game fatal handler (Source 2 caught the exception)" );
		}

		return original
			? original(
				process,
				process_id,
				file,
				dump_type,
				exception,
				user_stream,
				callback )
			: FALSE;
	}

	bool install_game_crash_capture( )
	{
		if ( !diag::g_minidump_write )
		{
			return false;
		}

		if ( !hooking::manager::create( {
				{
					&g_minidump_hook,
					reinterpret_cast<void*>( diag_minidump_write_detour ),
					"dbghelp!MiniDumpWriteDump",
					reinterpret_cast<std::uintptr_t>( diag::g_minidump_write )
				}
			} ) )
		{
			diag::write(
				diag::level::warning,
				"failed to hook the Source 2 minidump path; "
				"only self/unhandled crashes will be captured" );
			return false;
		}

		diag::write(
			diag::level::info,
			"Source 2 fatal minidump path hooked" );
		return true;
	}

	BOOL WINAPI diag_terminate_process_detour(
		HANDLE process,
		UINT exit_code )
	{
		const auto original =
			g_terminate_process_hook.original<decltype( &TerminateProcess )>( );

		if ( GetProcessId( process ) == GetCurrentProcessId( ) )
		{
			char stage[ 96 ]{};
			_snprintf_s(
				stage,
				sizeof( stage ),
				_TRUNCATE,
				"TerminateProcess requested for CS2; exit_code=0x%08X",
				exit_code );
			diag::capture_snapshot( stage );
		}

		return original ? original( process, exit_code ) : FALSE;
	}

	bool install_termination_capture( )
	{
		const auto kernel32 = GetModuleHandleW( L"kernel32.dll" );
		const auto terminate_process = kernel32
			? GetProcAddress( kernel32, "TerminateProcess" )
			: nullptr;
		if ( !terminate_process ||
			!hooking::manager::create( {
				{
					&g_terminate_process_hook,
					reinterpret_cast<void*>( diag_terminate_process_detour ),
					"kernel32!TerminateProcess",
					reinterpret_cast<std::uintptr_t>( terminate_process )
				}
			} ) )
		{
			diag::write(
				diag::level::warning,
				"failed to hook forced process termination" );
			return false;
		}

		diag::write(
			diag::level::info,
			"forced process termination capture hooked" );
		return true;
	}
#endif

	LONG CALLBACK diag_vectored_exception_filter( EXCEPTION_POINTERS* info )
	{
		if ( !info || !info->ExceptionRecord ||
			!diag::is_serious_exception( info->ExceptionRecord->ExceptionCode ) )
		{
			return EXCEPTION_CONTINUE_SEARCH;
		}

		if ( diag::probe_active( ) )
		{
			return EXCEPTION_CONTINUE_SEARCH;
		}

		// The host or Steam may replace the single process-wide last-chance
		// filter after injection. Re-arm it at first chance and preserve the
		// displaced handler so the host still receives the crash after us.
		const auto displaced_filter =
			SetUnhandledExceptionFilter( diag_unhandled_exception_filter );
		if ( displaced_filter != diag_unhandled_exception_filter )
		{
			g_previous_exception_filter.store(
				displaced_filter,
				std::memory_order_release );
		}

		if ( diag::is_module_address( info->ExceptionRecord->ExceptionAddress ) )
		{
			diag::write_exception_details( diag::level::error, "VEH EXCEPTION", info );
			diag::record_crash(
				info,
				diag::g_exception_scope_depth
					? diag::g_exception_phase
					: "first-chance fault in velocity DLL" );
			return EXCEPTION_CONTINUE_SEARCH;
		}

		if ( diag::g_exception_scope_depth == 0 )
		{
			return EXCEPTION_CONTINUE_SEARCH;
		}

		diag::write_exception_details( diag::level::error, "FEATURE EXCEPTION", info );

		return EXCEPTION_CONTINUE_SEARCH;
	}

	LONG WINAPI diag_unhandled_exception_filter( EXCEPTION_POINTERS* info )
	{
		if ( !info || !info->ExceptionRecord )
		{
			return EXCEPTION_CONTINUE_SEARCH;
		}

		diag::write_exception_details( diag::level::fatal, "UNHANDLED EXCEPTION", info );
		diag::record_crash( info, "unhandled exception" );

		const auto previous_filter =
			g_previous_exception_filter.load( std::memory_order_acquire );
		if ( previous_filter &&
			previous_filter != diag_unhandled_exception_filter )
		{
			return previous_filter( info );
		}

		return EXCEPTION_CONTINUE_SEARCH;
	}

#if defined( DEV )
	#define INIT_FAIL( msg ) \
		do { \
			rendering::g_imgui_menu.loading_failed( msg ); \
			diag::write( diag::level::error, msg ); \
			return 0; \
		} while ( 0 )

	#define INIT_WARN( msg ) \
		do { \
			rendering::g_imgui_menu.loading_check_result( 0 ); \
			diag::write( diag::level::warning, msg ); \
		} while ( 0 )
#else
	#define INIT_FAIL( msg ) \
		do { \
			rendering::g_imgui_menu.loading_failed( msg ); \
			diag::write( diag::level::error, msg ); \
			MessageBoxA( nullptr, xs( msg ), xs( "..." ), MB_ICONERROR ); \
			return 0; \
		} while ( 0 )

	#define INIT_WARN( msg ) INIT_FAIL( msg )
#endif

	#define INIT_TEST( label, expression, failure ) \
		do { \
			rendering::g_imgui_menu.loading_begin_check( label ); \
		if ( !( expression ) ) \
			INIT_FAIL( failure ); \
		rendering::g_imgui_menu.loading_check_result( 1 ); \
		} while ( 0 )

	DWORD WINAPI init_thread_impl( LPVOID param )
	{
		const auto module_handle = static_cast<HMODULE>( param );

		diag::step( "stage: thread start" );
		rendering::g_imgui_menu.loading_begin_check( "diagnostics / crash capture" );
		diag::initialize_crash_dumps( );
		logging::console::initialize( );
		rendering::g_imgui_menu.loading_check_result( 1 );

		diag::step( "stage: coinit" );
		rendering::g_imgui_menu.loading_begin_check( "COM / multithreaded" );
		const auto coinit_result =
			CoInitializeEx( nullptr, COINIT_MULTITHREADED );
		if ( FAILED( coinit_result ) )
		{
			rendering::g_imgui_menu.loading_check_result( 0 );
			diag::writef(
				diag::level::warning,
				"CoInitializeEx failed; hresult=0x%08lX",
				coinit_result );
		}
		else
		{
			rendering::g_imgui_menu.loading_check_result( 1 );
		}

		diag::step( "stage: config" );
		rendering::g_imgui_menu.loading_begin_check( "configuration / binds" );
		config::initialize( );
		settings::finalize_binds( );
		rendering::g_imgui_menu.loading_check_result( 1 );

		diag::step( "stage: regions" );
		rendering::g_imgui_menu.loading_begin_check( "module memory regions" );
		security::regions::add_module( module_handle );
		rendering::g_imgui_menu.loading_check_result( 1 );

		diag::step( "stage: logging" );
		{
			rendering::g_imgui_menu.loading_begin_check( "console logger" );
			if ( !logging::console::initialize( ) )
			{
#if defined( DEV )
				INIT_WARN( "failed to initialize console logging." );
#else
				INIT_FAIL( "failed to initialize console logging." );
#endif
			}
			else
			{
				rendering::g_imgui_menu.loading_check_result( 1 );
			}

			rendering::g_imgui_menu.loading_begin_check( "popup logger" );
			if ( !logging::popup::initialize( ) )
			{
#if defined( DEV )
				INIT_WARN( "failed to initialize popup logging." );
#else
				INIT_FAIL( "failed to initialize popup logging." );
#endif
			}
			else
			{
				rendering::g_imgui_menu.loading_check_result( 1 );
			}
		}

		diag::step( "stage: integrity" );
		{
			INIT_TEST( "integrity checks", security::integrity::initialize( ), "failed to initialize integrity checks." );

#if defined( DEV )
			rendering::g_imgui_menu.loading_begin_check( "Source 2 crash capture" );
			rendering::g_imgui_menu.loading_check_result( install_game_crash_capture( ) ? 1 : 0 );
			rendering::g_imgui_menu.loading_begin_check( "forced termination capture" );
			rendering::g_imgui_menu.loading_check_result( install_termination_capture( ) ? 1 : 0 );
#endif

			INIT_TEST( "thread pool", threadpool::initialize( ), "failed to initialize thread pool." );
			INIT_TEST( "steam http", steam::http::initialize( ), "failed to initialize steam http." );
			INIT_TEST( "steam friends", steam::friends::initialize( ), "failed to initialize steam friends." );
			INIT_TEST( "steam user", steam::user::initialize( ), "failed to initialize steam user." );
			INIT_TEST( "steam utils", steam::utils::initialize( ), "failed to initialize steam utils." );
		}

		diag::step( "stage: addresses" );
		{
			INIT_TEST( "module addresses", addresses::modules::initialize( ), "failed to initialize module addresses." );
			INIT_TEST( "global addresses", addresses::globals::initialize( ), "failed to initialize global addresses." );
			INIT_TEST( "function addresses", addresses::functions::initialize( ), "failed to initialize function addresses." );
		}

		diag::step( "stage: systems" );
		{
			INIT_TEST( "materials system", systems::materials::initialize( ), "failed to initialize materials system." );
			INIT_TEST( "event system", systems::events::initialize( ), "failed to initialize event system." );
			INIT_TEST( "vpk parse system", systems::g_icons.initialize( ), "failed to initialize vpk parse system." );
			INIT_TEST( "model preview system", systems::g_model_preview.initialize( ), "failed to initialize model preview system." );
		}

		diag::step( "stage: econ" );
		{
			INIT_TEST( "econ item system", features::changer::g_econ_item_system.initialize( ), "failed to initialize econ item system." );
		}

		diag::step( "stage: hooks" );
		{
			INIT_TEST( "utility hooks", hooks::utility::initialize( ), "failed to initialize utility hooks." );
			INIT_TEST( "feature hooks", hooks::cheat::initialize( ), "failed to initialize cheat hooks." );
		}

		diag::step( "stage: cvars" );
		{
			INIT_TEST( "hidden cvars", addresses::globals::cvar->unlock_all( ), "failed to unlock hidden cvars." );
		}

		diag::step( "stage: skyboxes" );
		rendering::g_imgui_menu.loading_begin_check( "skybox discovery" );
		features::world::g_scene.discover_skyboxes( );
		rendering::g_imgui_menu.loading_check_result( 1 );

		diag::step( "stage: done" );
		hooks::cheat::set_ready( true );
		return 1;
	}

	DWORD diag_exception_filter( EXCEPTION_POINTERS* info )
	{
		char buf[ 128 ]{};
		_snprintf_s( buf, sizeof( buf ), _TRUNCATE, "EXCEPTION 0x%08lX at 0x%p", info->ExceptionRecord->ExceptionCode, info->ExceptionRecord->ExceptionAddress );
		diag::write( diag::level::fatal, buf );
		diag::record_crash( info, "initialization thread" );
		return EXCEPTION_EXECUTE_HANDLER;
	}

	DWORD WINAPI init_thread( LPVOID param )
	{
		DWORD result{};
		__try
		{
			result = init_thread_impl( param );
		}
		__except ( diag_exception_filter( GetExceptionInformation( ) ) )
		{
			rendering::g_imgui_menu.loading_failed( "initialization thread exception" );
			result = 0;
		}

		if ( result != 0 )
		{
			rendering::g_imgui_menu.loading_complete( );
			return result;
		}

		if ( rendering::g_imgui_menu.is_open( ) )
		{
			rendering::g_imgui_menu.toggle( );
		}

		Sleep( 2500 );
		FreeLibraryAndExitThread( static_cast<HMODULE>( param ), 0 );
		return 0;
	}

} // namespace

extern "C" int __stdcall entry( HMODULE module_handle, DWORD reason, LPVOID reserved )
{
	if ( reason == DLL_PROCESS_ATTACH )
	{
		_CRT_INIT( module_handle, reason, reserved );
		DisableThreadLibraryCalls( module_handle );

		diag::set_module( module_handle );
		install_exception_handlers( );
		diag::step( "stage: dll attach" );
		diag::step( "build: development diagnostics" );

		diag::step( "stage: crt done, spawning thread" );

		const auto thread = CreateThread( nullptr, 0, init_thread, module_handle, 0, nullptr );
		if ( !thread )
		{
			rendering::g_imgui_menu.loading_failed( "failed to create initialization thread" );
			diag::writef(
				diag::level::error,
				"failed to create initialization thread; win32_error=%lu",
				GetLastError( ) );
			return 0;
		}

		CloseHandle( thread );
		return 1;
	}
	else if ( reason == DLL_PROCESS_DETACH )
	{
#if defined( DEV )
		if ( g_vectored_exception_handler )
		{
			RemoveVectoredExceptionHandler( g_vectored_exception_handler );
			g_vectored_exception_handler = nullptr;
		}

		const auto previous_filter =
			g_previous_exception_filter.exchange(
				nullptr,
				std::memory_order_acq_rel );
		const auto current_filter =
			SetUnhandledExceptionFilter( previous_filter );
		if ( current_filter != diag_unhandled_exception_filter )
		{
			SetUnhandledExceptionFilter( current_filter );
		}

		g_terminate_process_hook.reset( );
		g_minidump_hook.reset( );

		features::esp::player::g_chams.bt( ).shutdown( );
		features::esp::player::g_chams.os( ).shutdown( );

		features::world::g_weather.release( );
		rendering::g_imgui_menu.shutdown( );
		rendering::g_menu.shutdown( );

		systems::events::shutdown( );
		hooks::utility::shutdown( );
		hooks::cheat::shutdown( );
		CoUninitialize( );
#endif

		logging::console::shutdown( );
		diag::shutdown( );

#if defined( DEV )
		_CRT_INIT( module_handle, reason, reserved );
#endif
	}

	return 1;
}
