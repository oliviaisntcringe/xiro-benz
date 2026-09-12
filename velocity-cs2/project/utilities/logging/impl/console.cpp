#include <pch/pch.hpp>
#include <utilities/diag.hpp>
#include "../logging.hpp"

namespace logging::console {

	namespace {

		bool g_owned_console{};

	}

	bool initialize( )
	{
		if ( GetConsoleWindow( ) != nullptr )
		{
			return true;
		}

		if ( !AllocConsole( ) )
		{
			return false;
		}

		g_owned_console = true;
		SetConsoleTitleA( "XI.BENZ diagnostics" );

		FILE* stream{};
		freopen_s( &stream, "CONIN$", "r", stdin );
		freopen_s( &stream, "CONOUT$", "w", stdout );
		freopen_s( &stream, "CONOUT$", "w", stderr );
		std::setvbuf( stdout, nullptr, _IONBF, 0 );
		std::setvbuf( stderr, nullptr, _IONBF, 0 );

		std::printf( "[xiro-benz] diagnostics console initialized\n" );
		return true;
	}

	void shutdown( )
	{
		if ( !g_owned_console )
		{
			return;
		}

		std::fflush( stdout );
		std::fflush( stderr );
		FreeConsole( );
		g_owned_console = false;
	}

	void print_raw( const char* text )
	{
		if ( !text ) {
			return;
		}

		const bool was_emitting = emitting;
		emitting = true;
		std::fputs( text, stdout );
		std::fputc( '\n', stdout );
		diag::write( diag::level::info, text );
		emitting = was_emitting;
	}

} // namespace logging::console
