#define NOMINMAX
#include <windows.h>
#include <tlhelp32.h>
#include <shellapi.h>

#include <algorithm>
#include <chrono>
#include <cwctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace fs = std::filesystem;

namespace
{

enum class level
{
	info,
	warning,
	error,
};

struct logger
{
	std::wofstream file;

	void open( const fs::path& path )
	{
		std::error_code error;
		if ( !path.empty( ) )
		{
			fs::create_directories( path.parent_path( ), error );
			file.open( path, std::ios::out | std::ios::trunc );
		}
	}

	void write( level severity, std::wstring_view message )
	{
		SYSTEMTIME now{};
		GetLocalTime( &now );
		const wchar_t* label = severity == level::error
			? L"ERROR"
			: severity == level::warning ? L"WARN " : L"INFO ";
		std::wstringstream line;
		line << L"[injector] [" << now.wHour << L":";
		if ( now.wMinute < 10 ) line << L'0';
		line << now.wMinute << L":";
		if ( now.wSecond < 10 ) line << L'0';
		line << now.wSecond << L'.';
		if ( now.wMilliseconds < 100 ) line << L'0';
		if ( now.wMilliseconds < 10 ) line << L'0';
		line << now.wMilliseconds << L"] [" << label << L"] " << message;
		const auto text = line.str( );
		std::wcout << text << L'\n';
		if ( file.is_open( ) )
		{
			file << text << L'\n';
			file.flush( );
		}
	}
};

struct options
{
	std::wstring game;
	std::wstring dll;
	std::wstring arguments{ L"-insecure -novid" };
	std::wstring log;
	DWORD pid{};
	DWORD wait_ms{ 30000 };
	bool launch{};
	bool help{};
};

logger g_log;

std::wstring lower( std::wstring value )
{
	std::transform( value.begin( ), value.end( ), value.begin( ),
		[]( wchar_t character ) { return static_cast< wchar_t >( std::towlower( character ) ); } );
	return value;
}

std::wstring win32_error( DWORD error = GetLastError( ) )
{
	wchar_t* buffer{};
	const auto flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS;
	FormatMessageW( flags, nullptr, error, 0, reinterpret_cast< LPWSTR >( &buffer ), 0, nullptr );
	std::wstring result = buffer ? buffer : L"unknown error";
	if ( buffer ) LocalFree( buffer );
	while ( !result.empty( ) && ( result.back( ) == L'\r' || result.back( ) == L'\n' ) ) result.pop_back( );
	return result + L" (" + std::to_wstring( error ) + L")";
}

fs::path module_directory( )
{
	wchar_t buffer[ MAX_PATH ]{};
	const auto length = GetModuleFileNameW( nullptr, buffer, static_cast< DWORD >( std::size( buffer ) ) );
	if ( !length || length >= std::size( buffer ) ) return fs::current_path( );
	return fs::path( buffer ).parent_path( );
}

std::wstring quote_argument( std::wstring_view value )
{
	if ( value.find_first_of( L" \t\"" ) == std::wstring_view::npos ) return std::wstring( value );
	std::wstring result{ L"\"" };
	for ( const auto character : value )
	{
		if ( character == L'\"' ) result += L"\\\"";
		else result += character;
	}
	result += L'\"';
	return result;
}

bool file_exists( const fs::path& path )
{
	std::error_code error;
	return !path.empty( ) && fs::is_regular_file( path, error );
}

std::optional< fs::path > resolve_path( std::wstring value )
{
	if ( value.empty( ) ) return std::nullopt;
	fs::path path( value );
	std::error_code error;
	if ( path.is_relative( ) ) path = fs::absolute( path, error );
	if ( error || !file_exists( path ) ) return std::nullopt;
	return fs::weakly_canonical( path, error );
}

DWORD find_process( std::wstring_view image_name )
{
	const auto snapshot = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
	if ( snapshot == INVALID_HANDLE_VALUE ) return 0;
	PROCESSENTRY32W entry{ sizeof( entry ) };
	DWORD result{};
	if ( Process32FirstW( snapshot, &entry ) )
	{
		do
		{
			if ( lower( entry.szExeFile ) == lower( std::wstring( image_name ) ) )
			{
				result = entry.th32ProcessID;
				break;
			}
		} while ( Process32NextW( snapshot, &entry ) );
	}
	CloseHandle( snapshot );
	return result;
}

std::optional< fs::path > process_path( DWORD pid )
{
	const auto process = OpenProcess( PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid );
	if ( !process ) return std::nullopt;
	wchar_t buffer[ 32768 ]{};
	DWORD length = static_cast< DWORD >( std::size( buffer ) );
	const auto success = QueryFullProcessImageNameW( process, 0, buffer, &length ) != FALSE;
	CloseHandle( process );
	return success ? std::optional< fs::path >{ fs::path( std::wstring( buffer, length ) ) } : std::nullopt;
}

bool module_loaded( DWORD pid, std::wstring_view wanted )
{
	const auto snapshot = CreateToolhelp32Snapshot( TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid );
	if ( snapshot == INVALID_HANDLE_VALUE ) return false;
	MODULEENTRY32W entry{ sizeof( entry ) };
	bool found{};
	if ( Module32FirstW( snapshot, &entry ) )
	{
		do
		{
			if ( lower( entry.szModule ) == lower( std::wstring( wanted ) ) )
			{
				found = true;
				break;
			}
		} while ( Module32NextW( snapshot, &entry ) );
	}
	CloseHandle( snapshot );
	return found;
}

bool wait_for_game_modules( DWORD pid, DWORD timeout_ms )
{
	g_log.write( level::info, L"step=modules wait=client.dll,engine2.dll" );
	const auto deadline = std::chrono::steady_clock::now( ) + std::chrono::milliseconds( timeout_ms );
	for ( ;; )
	{
		const auto process = OpenProcess( SYNCHRONIZE, FALSE, pid );
		if ( !process )
		{
			g_log.write( level::error, L"step=modules OpenProcess failed: " + win32_error( ) );
			return false;
		}
		const auto exited = WaitForSingleObject( process, 0 ) == WAIT_OBJECT_0;
		CloseHandle( process );
		if ( exited )
		{
			g_log.write( level::error, L"step=modules CS2 exited before its modules loaded" );
			return false;
		}
		if ( module_loaded( pid, L"client.dll" ) && module_loaded( pid, L"engine2.dll" ) )
		{
			g_log.write( level::info, L"step=modules ready" );
			return true;
		}
		if ( std::chrono::steady_clock::now( ) >= deadline )
		{
			g_log.write( level::error, L"step=modules timed out after " + std::to_wstring( timeout_ms ) + L"ms" );
			return false;
		}
		Sleep( 100 );
	}
}

bool inject_library( DWORD pid, const fs::path& dll )
{
	g_log.write( level::info, L"step=open-process pid=" + std::to_wstring( pid ) );
	const auto process = OpenProcess(
		PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION |
			PROCESS_VM_WRITE | PROCESS_VM_READ,
		FALSE,
		pid );
	if ( !process )
	{
		g_log.write( level::error, L"step=open-process failed: " + win32_error( ) );
		return false;
	}

	const auto path_text = dll.wstring( );
	const auto bytes = ( path_text.size( ) + 1 ) * sizeof( wchar_t );
	g_log.write( level::info, L"step=allocate-remote-path bytes=" + std::to_wstring( bytes ) );
	const auto remote_path = VirtualAllocEx( process, nullptr, bytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE );
	if ( !remote_path )
	{
		g_log.write( level::error, L"step=allocate-remote-path failed: " + win32_error( ) );
		CloseHandle( process );
		return false;
	}

	SIZE_T written{};
	const auto write_ok = WriteProcessMemory(
		process, remote_path, path_text.c_str( ), bytes, &written ) != FALSE;
	if ( !write_ok || written != bytes )
	{
		g_log.write( level::error, L"step=write-remote-path failed: " + win32_error( ) );
		VirtualFreeEx( process, remote_path, 0, MEM_RELEASE );
		CloseHandle( process );
		return false;
	}

	const auto kernel32 = GetModuleHandleW( L"kernel32.dll" );
	const auto load_library = kernel32 ? GetProcAddress( kernel32, "LoadLibraryW" ) : nullptr;
	if ( !load_library )
	{
		g_log.write( level::error, L"step=resolve-loadlibrary failed: " + win32_error( ) );
		VirtualFreeEx( process, remote_path, 0, MEM_RELEASE );
		CloseHandle( process );
		return false;
	}

	g_log.write( level::info, L"step=create-remote-thread LoadLibraryW" );
	const auto thread = CreateRemoteThread(
		process,
		nullptr,
		0,
		reinterpret_cast< LPTHREAD_START_ROUTINE >( load_library ),
		remote_path,
		0,
		nullptr );
	if ( !thread )
	{
		g_log.write( level::error, L"step=create-remote-thread failed: " + win32_error( ) );
		VirtualFreeEx( process, remote_path, 0, MEM_RELEASE );
		CloseHandle( process );
		return false;
	}

	const auto wait_result = WaitForSingleObject( thread, 30000 );
	DWORD remote_module{};
	const auto exit_ok = GetExitCodeThread( thread, &remote_module ) != FALSE;
	CloseHandle( thread );
	VirtualFreeEx( process, remote_path, 0, MEM_RELEASE );
	CloseHandle( process );
	if ( wait_result != WAIT_OBJECT_0 )
	{
		g_log.write( level::error, L"step=remote-thread wait failed result=0x" +
			std::to_wstring( wait_result ) );
		return false;
	}
	if ( !exit_ok || remote_module == 0 )
	{
		g_log.write( level::error, L"step=remote-thread LoadLibraryW returned null" );
		return false;
	}

	std::wstringstream address;
	address << L"step=inject complete remote_module=0x" << std::hex << remote_module;
	g_log.write( level::info, address.str( ) );
	return true;
}

void print_help( )
{
	std::wcout << L"triada.benz CS2 injector\n\n"
		L"Usage: injector.exe [options]\n\n"
		L"  --game <path>       CS2 executable (auto-detected when omitted)\n"
		L"  --dll <path>        DLL to load (defaults to velocity-debug.dll)\n"
		L"  --args <text>       CS2 arguments (default: -insecure -novid)\n"
		L"  --pid <number>      Attach to an existing cs2.exe\n"
		L"  --launch             Launch a new CS2; fails if one is already running\n"
		L"  --wait-ms <number>   Module wait timeout (default: 30000)\n"
		L"  --log <path>         Injector log path (default: injector.log)\n"
		L"  --help               Show this help\n";
}

bool parse_options( options& result, int count, wchar_t** arguments )
{
	const auto value = [&]( int& index, std::wstring& target )
	{
		if ( index + 1 >= count ) return false;
		target = arguments[ ++index ];
		return true;
	};
	for ( int index = 1; index < count; ++index )
	{
		const std::wstring name = arguments[ index ];
		if ( name == L"--help" || name == L"-h" ) result.help = true;
		else if ( name == L"--launch" ) result.launch = true;
		else if ( name == L"--game" )
		{
			if ( !value( index, result.game ) ) return false;
		}
		else if ( name == L"--dll" )
		{
			if ( !value( index, result.dll ) ) return false;
		}
		else if ( name == L"--args" )
		{
			if ( !value( index, result.arguments ) ) return false;
		}
		else if ( name == L"--log" )
		{
			if ( !value( index, result.log ) ) return false;
		}
		else if ( name == L"--pid" )
		{
			std::wstring text;
			if ( !value( index, text ) ) return false;
			try { result.pid = static_cast< DWORD >( std::stoul( text ) ); }
			catch ( ... ) { return false; }
		}
		else if ( name == L"--wait-ms" )
		{
			std::wstring text;
			if ( !value( index, text ) ) return false;
			try { result.wait_ms = static_cast< DWORD >( std::stoul( text ) ); }
			catch ( ... ) { return false; }
		}
		else return false;
	}
	return true;
}

std::optional< fs::path > default_game( )
{
	wchar_t* environment{};
	if ( _wdupenv_s( &environment, nullptr, L"CS2_EXE" ) == 0 && environment && *environment )
	{
		const auto path = resolve_path( environment );
		free( environment );
		if ( path ) return path;
	}
	else if ( environment ) free( environment );
	const std::vector< fs::path > candidates{
		L"D:/SteamLibrary/steamapps/common/Counter-Strike Global Offensive/game/bin/win64/cs2.exe",
		L"C:/Program Files (x86)/Steam/steamapps/common/Counter-Strike Global Offensive/game/bin/win64/cs2.exe",
	};
	for ( const auto& candidate : candidates ) if ( const auto path = resolve_path( candidate.wstring( ) ) ) return path;
	return std::nullopt;
}

std::optional< fs::path > default_dll( )
{
	const auto directory = module_directory( );
	const std::vector< fs::path > candidates{
		directory / L"velocity-debug.dll",
		directory / L"velocity.dll",
		directory / L"bin" / L"velocity-debug.dll",
		directory / L"bin" / L"velocity.dll",
	};
	for ( const auto& candidate : candidates ) if ( const auto path = resolve_path( candidate.wstring( ) ) ) return path;
	return std::nullopt;
}

int run( int argc, wchar_t** argv )
{
	options config;
	if ( !parse_options( config, argc, argv ) )
	{
		print_help( );
		return 2;
	}
	if ( config.help )
	{
		print_help( );
		return 0;
	}

	const auto game = config.game.empty( ) ? default_game( ) : resolve_path( config.game );
	const auto dll = config.dll.empty( ) ? default_dll( ) : resolve_path( config.dll );
	if ( !game )
	{
		std::wcerr << L"CS2 executable not found; pass --game <path>\n";
		return 1;
	}
	if ( !dll )
	{
		std::wcerr << L"DLL not found; pass --dll <path>\n";
		return 1;
	}
	if ( lower( game->filename( ).wstring( ) ) != L"cs2.exe" )
	{
		std::wcerr << L"--game must point to cs2.exe\n";
		return 1;
	}

	const auto log_path = config.log.empty( )
		? module_directory( ) / L"injector.log"
		: fs::path( config.log );
	g_log.open( log_path );
	g_log.write( level::info, L"step=start game=" + game->wstring( ) + L" dll=" + dll->wstring( ) );

	DWORD pid = config.pid;
	PROCESS_INFORMATION launched{};
	if ( pid )
	{
		g_log.write( level::info, L"step=attach pid=" + std::to_wstring( pid ) );
	}
	else
	{
		const auto existing = find_process( L"cs2.exe" );
		if ( existing && config.launch )
		{
			g_log.write( level::error, L"step=launch refused: cs2.exe is already running pid=" + std::to_wstring( existing ) );
			return 1;
		}
		if ( existing )
		{
			pid = existing;
			g_log.write( level::info, L"step=attach existing cs2 pid=" + std::to_wstring( pid ) );
		}
		else
		{
			std::wstring command = quote_argument( game->wstring( ) );
			if ( !config.arguments.empty( ) ) command += L" " + config.arguments;
			std::vector< wchar_t > mutable_command( command.begin( ), command.end( ) );
			mutable_command.push_back( L'\0' );
			STARTUPINFOW startup{ sizeof( startup ) };
			const auto working_directory = game->parent_path( ).wstring( );
			g_log.write( level::info, L"step=launch command=" + command );
			if ( !CreateProcessW(
				nullptr,
				mutable_command.data( ),
				nullptr,
				nullptr,
				FALSE,
				0,
				nullptr,
				working_directory.c_str( ),
				&startup,
				&launched ) )
			{
				g_log.write( level::error, L"step=launch failed: " + win32_error( ) );
				return 1;
			}
			pid = launched.dwProcessId;
			CloseHandle( launched.hThread );
			g_log.write( level::info, L"step=launch complete pid=" + std::to_wstring( pid ) );
		}
	}

	const auto owner = process_path( pid );
	if ( !owner || lower( owner->filename( ).wstring( ) ) != L"cs2.exe" )
	{
		g_log.write( level::error, L"step=validate-process rejected pid=" + std::to_wstring( pid ) );
		if ( launched.hProcess ) CloseHandle( launched.hProcess );
		return 1;
	}
	if ( !wait_for_game_modules( pid, config.wait_ms ) )
	{
		if ( launched.hProcess ) CloseHandle( launched.hProcess );
		return 1;
	}
	const auto success = inject_library( pid, *dll );
	if ( launched.hProcess ) CloseHandle( launched.hProcess );
	g_log.write( success ? level::info : level::error, success ? L"step=done injection successful" : L"step=done injection failed" );
	return success ? 0 : 1;
}

} // namespace

int wmain( int argc, wchar_t** argv )
{
	return run( argc, argv );
}
