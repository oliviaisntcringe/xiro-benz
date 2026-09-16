#pragma once

#include <shellapi.h>

#if defined( _MSC_VER )
#pragma comment( lib, "shell32" )
#endif

// Lightweight diagnostics used before the rest of the project is initialized.
// The logger deliberately uses Win32 file I/O so it remains usable from SEH
// handlers and does not depend on the state of iostreams.
namespace diag {

	enum class level
	{
		debug,
		info,
		warning,
		error,
		fatal
	};

	inline wchar_t g_log_path[ MAX_PATH ]{};
	inline wchar_t g_previous_log_path[ MAX_PATH ]{};
	inline wchar_t g_dump_path[ MAX_PATH ]{};
	inline wchar_t g_previous_dump_path[ MAX_PATH ]{};
	inline wchar_t g_log_directory[ MAX_PATH ]{};
	inline HANDLE g_log_file{};
	inline HMODULE g_module{};
	inline std::uintptr_t g_module_end{};
	inline volatile LONG64 g_log_sequence{};
	inline volatile LONG64 g_hook_event_sequence{};
	inline volatile LONG g_log_lock{};
	inline volatile LONG g_log_dropped{};
	inline bool g_verbose_logging{};
	inline thread_local bool g_writing_log{};
	inline thread_local std::uint32_t g_exception_scope_depth{};
	inline thread_local std::uint32_t g_probe_scope_depth{};
	inline thread_local const char* g_exception_phase{ "none" };

	struct hook_snapshot
	{
		const char* name{ "none" };
		std::uintptr_t args[ 4 ]{};
	};

	inline thread_local hook_snapshot g_hook_snapshot{};

	// DbgHelp declares this structure under 4-byte packing, including on x64.
#pragma pack( push, 4 )
	struct minidump_exception_information
	{
		DWORD thread_id;
		EXCEPTION_POINTERS* exception_pointers;
		BOOL client_pointers;
	};
#pragma pack( pop )
	static_assert( sizeof( minidump_exception_information ) == 16 );

	inline volatile LONG g_crash_report_enabled{};
	inline volatile LONG g_crash_claimed{};
	inline volatile LONG g_crash_artifacts_opened{};

	struct crash_report_request
	{
		EXCEPTION_RECORD record{};
		CONTEXT context{};
		EXCEPTION_POINTERS pointers{};
		hook_snapshot hook{};
		char stage[ 128 ]{};
		DWORD fault_thread_id{};
	};

	inline crash_report_request g_crash_request{};
	inline constexpr DWORD diagnostic_snapshot_code = 0xE0560001;

	#if defined( DEV )
	using minidump_write_fn = BOOL( WINAPI* )(
		HANDLE,
		DWORD,
		HANDLE,
		unsigned long,
		minidump_exception_information*,
		void*,
		void* );

	inline minidump_write_fn g_minidump_write{};
	inline thread_local bool g_writing_minidump{};

	// MINIDUMP_TYPE flags from dbghelp.h. Keeping the ABI-compatible values
	// local avoids adding a static dbghelp dependency to the injected DLL.
	inline constexpr unsigned long minidump_with_unloaded_modules = 0x20;
	inline constexpr unsigned long minidump_with_indirectly_referenced_memory = 0x40;
	inline constexpr unsigned long minidump_with_thread_info = 0x1000;
	#endif

	inline const char* level_name( level value )
	{
		switch ( value )
		{
		case level::debug:
			return "DEBUG";
		case level::info:
			return "INFO ";
		case level::warning:
			return "WARN ";
		case level::error:
			return "ERROR";
		case level::fatal:
			return "FATAL";
		}

		return "?????";
	}

	inline void append_path( wchar_t* path, std::size_t capacity, const wchar_t* file_name )
	{
		std::size_t length{};
		while ( length < capacity && path[ length ] )
		{
			++length;
		}

		for ( std::size_t i{}; length + i + 1 < capacity; ++i )
		{
			path[ length + i ] = file_name[ i ];
			if ( !file_name[ i ] )
			{
				return;
			}
		}

		path[ capacity - 1 ] = L'\0';
	}

	inline void ensure_directory_separator( wchar_t* path, std::size_t capacity )
	{
		if ( !path || capacity < 2 )
		{
			return;
		}

		std::size_t length{};
		while ( length + 1 < capacity && path[ length ] )
		{
			++length;
		}

		if ( !length || path[ length - 1 ] == L'\\' || path[ length - 1 ] == L'/' )
		{
			return;
		}

		if ( length + 1 < capacity )
		{
			path[ length ] = L'\\';
			path[ length + 1 ] = L'\0';
		}
	}

	inline void copy_path( wchar_t* destination, std::size_t capacity, const wchar_t* source )
	{
		if ( !destination || !capacity )
		{
			return;
		}

		std::size_t i{};
		for ( ; i + 1 < capacity && source && source[ i ]; ++i )
		{
			destination[ i ] = source[ i ];
		}
		destination[ i ] = L'\0';
	}

	inline bool environment_flag( const wchar_t* name, bool fallback )
	{
		wchar_t value[ 16 ]{};
		const auto length = GetEnvironmentVariableW( name, value, static_cast< DWORD >( std::size( value ) ) );
		if ( !length || length >= std::size( value ) )
		{
			return fallback;
		}

		return value[ 0 ] != L'0' && value[ 0 ] != L'n' && value[ 0 ] != L'N' && value[ 0 ] != L'f' && value[ 0 ] != L'F';
	}

	inline bool acquire_log_lock( )
	{
		if ( g_writing_log )
		{
			return false;
		}

		for ( auto attempt = 0; attempt < 64; ++attempt )
		{
			if ( InterlockedCompareExchange( &g_log_lock, 1, 0 ) == 0 )
			{
				g_writing_log = true;
				return true;
			}

			YieldProcessor( );
		}

		InterlockedIncrement( &g_log_dropped );
		return false;
	}

	inline void release_log_lock( )
	{
		g_writing_log = false;
		InterlockedExchange( &g_log_lock, 0 );
	}

	inline void make_artifact_path(
		wchar_t* destination,
		std::size_t capacity,
		const wchar_t* directory,
		const wchar_t* file_name )
	{
		std::size_t i{};
		for ( ; i + 1 < capacity && directory[ i ]; ++i )
		{
			destination[ i ] = directory[ i ];
		}
		destination[ i ] = L'\0';
		append_path( destination, capacity, file_name );
	}

	inline void write( level severity, const char* message )
	{
		if ( !message )
		{
			return;
		}

		std::size_t message_length{};
		while ( message[ message_length ] )
		{
			++message_length;
		}
		while ( message_length &&
			( message[ message_length - 1 ] == '\r' ||
				message[ message_length - 1 ] == '\n' ) )
		{
			--message_length;
		}

		SYSTEMTIME time{};
		GetLocalTime( &time );

		const auto sequence = InterlockedIncrement64( &g_log_sequence );
		char line[ 4096 ]{};
		const int length = _snprintf_s(
			line,
			sizeof( line ),
			_TRUNCATE,
			"#%llu [%04u-%02u-%02u %02u:%02u:%02u.%03u] [%s] [P%lu:T%lu] [phase=%s] [hook=%s] %.*s\r\n",
			static_cast< unsigned long long >( sequence ),
			time.wYear,
			time.wMonth,
			time.wDay,
			time.wHour,
			time.wMinute,
			time.wSecond,
			time.wMilliseconds,
			level_name( severity ),
			GetCurrentProcessId( ),
			GetCurrentThreadId( ),
			g_exception_phase ? g_exception_phase : "none",
			g_hook_snapshot.name ? g_hook_snapshot.name : "none",
			static_cast<int>( message_length ),
			message );

		DWORD bytes{};
		while ( bytes + 1 < sizeof( line ) && line[ bytes ] )
		{
			++bytes;
		}

		const auto locked = acquire_log_lock( );
		if ( locked && g_log_file )
		{
			DWORD written{};
			WriteFile( g_log_file, line, bytes, &written, nullptr );
			FlushFileBuffers( g_log_file );
		}

		if ( locked )
		{
			const auto output = GetStdHandle( STD_OUTPUT_HANDLE );
			DWORD console_mode{};
			if ( output && output != INVALID_HANDLE_VALUE &&
				GetConsoleMode( output, &console_mode ) )
			{
				DWORD written{};
				WriteFile( output, line, bytes, &written, nullptr );
			}

			release_log_lock( );
		}

		OutputDebugStringA( line );
	}

	template <typename... args_t>
	inline void writef( level severity, const char* format, args_t... args )
	{
		char message[ 2048 ]{};
		_snprintf_s(
			message,
			sizeof( message ),
			_TRUNCATE,
			format,
			args... );
		write( severity, message );
	}

	inline void set_module( HMODULE module_handle )
	{
		g_module = module_handle;

		wchar_t module_directory[ MAX_PATH ]{};
		const DWORD path_length =
			GetModuleFileNameW( module_handle, module_directory, MAX_PATH );
		if ( !path_length || path_length >= MAX_PATH )
		{
			return;
		}

		for ( DWORD i = path_length; i > 0; --i )
		{
			if ( module_directory[ i - 1 ] == L'\\' || module_directory[ i - 1 ] == L'/' )
			{
				module_directory[ i ] = L'\0';
				break;
			}
		}

		wchar_t override_directory[ MAX_PATH ]{};
		const auto override_length = GetEnvironmentVariableW(
			L"XI_BENZ_LOG_DIR",
			override_directory,
			static_cast< DWORD >( std::size( override_directory ) ) );
		if ( override_length && override_length < std::size( override_directory ) )
		{
			ensure_directory_separator( override_directory, std::size( override_directory ) );
			copy_path( g_log_directory, std::size( g_log_directory ), override_directory );
		}
		else
		{
			copy_path( g_log_directory, std::size( g_log_directory ), module_directory );
		}

#if defined( DEV )
		g_verbose_logging = environment_flag( L"XI_BENZ_VERBOSE_LOG", true );
#else
		g_verbose_logging = environment_flag( L"XI_BENZ_VERBOSE_LOG", false );
#endif
		(void) CreateDirectoryW( g_log_directory, nullptr );

		make_artifact_path(
			g_log_path,
			MAX_PATH,
			g_log_directory,
			L"velocity_init.log" );
		make_artifact_path(
			g_previous_log_path,
			MAX_PATH,
			g_log_directory,
			L"velocity_init.previous.log" );

		MoveFileExW(
			g_log_path,
			g_previous_log_path,
			MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH );

		g_log_file = CreateFileW(
			g_log_path,
			FILE_APPEND_DATA,
			FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			nullptr,
			CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			nullptr );
		if ( g_log_file == INVALID_HANDLE_VALUE )
		{
			g_log_file = nullptr;
		}

		if ( !g_log_file )
		{
			const auto primary_error = GetLastError( );
			wchar_t temp_directory[ MAX_PATH ]{};
			const auto temp_length = GetTempPathW(
				static_cast< DWORD >( std::size( temp_directory ) ),
				temp_directory );
			if ( temp_length && temp_length < std::size( temp_directory ) )
			{
				ensure_directory_separator( temp_directory, std::size( temp_directory ) );
				copy_path( g_log_directory, std::size( g_log_directory ), temp_directory );
				make_artifact_path( g_log_path, MAX_PATH, g_log_directory, L"xiro-benz.velocity_init.log" );
				make_artifact_path( g_previous_log_path, MAX_PATH, g_log_directory, L"xiro-benz.velocity_init.previous.log" );
				MoveFileExW(
					g_log_path,
					g_previous_log_path,
					MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH );
				g_log_file = CreateFileW(
					g_log_path,
					FILE_APPEND_DATA,
					FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
					nullptr,
					CREATE_ALWAYS,
					FILE_ATTRIBUTE_NORMAL,
					nullptr );
				if ( g_log_file == INVALID_HANDLE_VALUE )
				{
					g_log_file = nullptr;
				}
			}

			if ( !g_log_file )
			{
				char fallback_message[ 256 ]{};
				_snprintf_s(
					fallback_message,
					sizeof( fallback_message ),
					_TRUNCATE,
					"[xiro-benz] diagnostics log open failed; primary_win32_error=%lu fallback_win32_error=%lu\n",
					primary_error,
					GetLastError( ) );
				OutputDebugStringA( fallback_message );
			}
		}
		InterlockedExchange( &g_crash_report_enabled, 1 );

		const auto* dos_header =
			reinterpret_cast<const IMAGE_DOS_HEADER*>( module_handle );
		if ( dos_header->e_magic == IMAGE_DOS_SIGNATURE )
		{
			const auto* nt_headers =
				reinterpret_cast<const IMAGE_NT_HEADERS*>(
					reinterpret_cast<std::uintptr_t>( module_handle ) +
					dos_header->e_lfanew );
			if ( nt_headers->Signature == IMAGE_NT_SIGNATURE )
			{
				g_module_end =
					reinterpret_cast<std::uintptr_t>( module_handle ) +
					nt_headers->OptionalHeader.SizeOfImage;
			}
		}

#if defined( DEV )
		make_artifact_path(
			g_dump_path,
			MAX_PATH,
			g_log_directory,
			L"velocity_crash.dmp" );
		make_artifact_path(
			g_previous_dump_path,
			MAX_PATH,
			g_log_directory,
			L"velocity_crash.previous.dmp" );
		MoveFileExW(
			g_dump_path,
			g_previous_dump_path,
			MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH );
#endif

		writef(
			level::info,
			"diagnostics initialized; module=0x%p size=0x%llX log=%ls previous=%ls dump=%ls verbose=%s",
			module_handle,
			g_module_end
				? static_cast<unsigned long long>(
					g_module_end -
					reinterpret_cast<std::uintptr_t>( module_handle ) )
				: 0ull,
			g_log_path,
			g_previous_log_path,
			g_dump_path,
			g_verbose_logging ? "true" : "false" );
	}

	inline void initialize_crash_dumps( )
	{
		InterlockedExchange( &g_crash_report_enabled, 1 );
#if defined( DEV )
		// Resolve outside the loader lock; LoadLibrary is unsafe from DLL attach.
		if ( const auto dbghelp = LoadLibraryW( L"dbghelp.dll" ) )
		{
			g_minidump_write = reinterpret_cast<minidump_write_fn>(
				GetProcAddress( dbghelp, "MiniDumpWriteDump" ) );
		}

		writef(
			g_minidump_write ? level::info : level::warning,
			"crash dumps %s; path=%ls",
			g_minidump_write ? "enabled" : "unavailable",
			g_dump_path );
#endif
	}

	inline bool is_module_address( const void* address )
	{
		const auto value = reinterpret_cast<std::uintptr_t>( address );
		return g_module && value >= reinterpret_cast<std::uintptr_t>( g_module ) &&
			value < g_module_end;
	}

	class hook_scope
	{
	public:
		hook_scope(
			const char* name,
			std::uintptr_t arg0 = 0,
			std::uintptr_t arg1 = 0,
			std::uintptr_t arg2 = 0,
			std::uintptr_t arg3 = 0 )
			: m_previous( g_hook_snapshot )
		{
			g_hook_snapshot.name = name ? name : "none";
			g_hook_snapshot.args[ 0 ] = arg0;
			g_hook_snapshot.args[ 1 ] = arg1;
			g_hook_snapshot.args[ 2 ] = arg2;
			g_hook_snapshot.args[ 3 ] = arg3;
			const auto hook_event = InterlockedIncrement64( &g_hook_event_sequence );
			m_trace = g_verbose_logging && ( hook_event <= 64 || hook_event % 256 == 0 );
			m_started = GetTickCount64( );
			if ( m_trace )
			{
				writef(
					level::debug,
					"hook enter name=%s arg0=0x%p arg1=0x%p arg2=0x%p arg3=0x%p",
					g_hook_snapshot.name,
					reinterpret_cast<void*>( arg0 ),
					reinterpret_cast<void*>( arg1 ),
					reinterpret_cast<void*>( arg2 ),
					reinterpret_cast<void*>( arg3 ) );
			}
		}

		~hook_scope( )
		{
			if ( m_trace )
			{
				writef(
					level::debug,
					"hook leave name=%s duration_ms=%llu",
					g_hook_snapshot.name,
					GetTickCount64( ) - m_started );
			}
			g_hook_snapshot = m_previous;
		}

		hook_scope( const hook_scope& ) = delete;
		hook_scope& operator=( const hook_scope& ) = delete;

	private:
		hook_snapshot m_previous{};
		bool m_trace{};
		ULONGLONG m_started{};
	};

	// Keep this probe independent from memory::safe_read so it can run from
	// the process-wide exception handler without introducing an include cycle.
	inline bool try_read_u64( std::uintptr_t address, std::uint64_t& value )
	{
		__try
		{
			value = *reinterpret_cast<const std::uint64_t*>( address );
			return true;
		}
		__except ( EXCEPTION_EXECUTE_HANDLER )
		{
			return false;
		}
	}

	inline const char* exception_access_kind( const EXCEPTION_RECORD* record )
	{
		if ( !record || record->ExceptionCode != EXCEPTION_ACCESS_VIOLATION ||
			record->NumberParameters == 0 )
		{
			return "unknown";
		}

		switch ( record->ExceptionInformation[ 0 ] )
		{
		case 0:
			return "read";
		case 1:
			return "write";
		case 8:
			return "execute";
		default:
			return "unknown";
		}
	}

	inline void write_exception_details(
		level severity,
		const char* label,
		const EXCEPTION_POINTERS* info )
	{
		if ( !info || !info->ExceptionRecord )
		{
			return;
		}

		const auto* record = info->ExceptionRecord;
		const auto instruction = reinterpret_cast<std::uintptr_t>( record->ExceptionAddress );
		const auto accessed = record->NumberParameters > 1
			? record->ExceptionInformation[ 1 ]
			: 0ull;

		HMODULE fault_module{};
		char module_path[ MAX_PATH ]{ "unknown" };
		std::uintptr_t module_base{};
		if ( instruction && GetModuleHandleExA(
			GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
			GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
			reinterpret_cast<LPCSTR>( instruction ), &fault_module ) )
		{
			module_base = reinterpret_cast<std::uintptr_t>( fault_module );
			GetModuleFileNameA( fault_module, module_path, MAX_PATH );
		}

		const auto* module_name = strrchr( module_path, '\\' );
		module_name = module_name ? module_name + 1 : module_path;
		const auto relative = module_base && instruction >= module_base
			? static_cast<unsigned long long>( instruction - module_base )
			: 0ull;

		writef(
			severity,
			"%s [%s] code=0x%08lX kind=%s rip=0x%p module=%s+0x%llX access=0x%p",
			label ? label : "EXCEPTION",
			g_exception_phase ? g_exception_phase : "none",
			record->ExceptionCode,
			exception_access_kind( record ),
			record->ExceptionAddress,
			module_name,
			relative,
			reinterpret_cast<void*>( accessed ) );

		writef(
			severity,
			"exception hook=%s arg0=0x%p arg1=0x%p arg2=0x%p arg3=0x%p",
			g_hook_snapshot.name ? g_hook_snapshot.name : "none",
			reinterpret_cast<void*>( g_hook_snapshot.args[ 0 ] ),
			reinterpret_cast<void*>( g_hook_snapshot.args[ 1 ] ),
			reinterpret_cast<void*>( g_hook_snapshot.args[ 2 ] ),
			reinterpret_cast<void*>( g_hook_snapshot.args[ 3 ] ) );

		if ( info->ContextRecord )
		{
			const auto& context = *info->ContextRecord;
#if defined( _M_X64 ) || defined( __x86_64__ )
			writef(
				severity,
				"registers rcx=0x%llX rdx=0x%llX r8=0x%llX r9=0x%llX rsp=0x%llX rbp=0x%llX",
				static_cast<unsigned long long>( context.Rcx ),
				static_cast<unsigned long long>( context.Rdx ),
				static_cast<unsigned long long>( context.R8 ),
				static_cast<unsigned long long>( context.R9 ),
				static_cast<unsigned long long>( context.Rsp ),
				static_cast<unsigned long long>( context.Rbp ) );

				for ( auto i = 0u; i < 6u; ++i )
				{
					std::uint64_t value{};
					if ( !try_read_u64( context.Rsp + i * sizeof( std::uint64_t ), value ) )
					{
						break;
					}

					writef(
						severity,
						"stack[%u] rsp+0x%X=0x%llX",
						i,
						i * static_cast<unsigned>( sizeof( std::uint64_t ) ),
						static_cast<unsigned long long>( value ) );
				}
#endif
		}
	}

	class exception_scope
	{
	public:
		explicit exception_scope( const char* phase = "feature pipeline" )
			: m_previous_phase( g_exception_phase )
		{
			++g_exception_scope_depth;
			g_exception_phase = phase;
		}

		~exception_scope( )
		{
			g_exception_phase = m_previous_phase;
			--g_exception_scope_depth;
		}

		exception_scope( const exception_scope& ) = delete;
		exception_scope& operator=( const exception_scope& ) = delete;

	private:
		const char* m_previous_phase;
	};

	// Suppresses expected first-chance exceptions from an explicit SEH probe.
	// Construct this in a caller of the function containing __try; MSVC does not
	// permit unwindable C++ locals in the same function as SEH.
	class probe_scope
	{
	public:
		probe_scope( )
		{
			++g_probe_scope_depth;
		}

		~probe_scope( )
		{
			--g_probe_scope_depth;
		}

		probe_scope( const probe_scope& ) = delete;
		probe_scope& operator=( const probe_scope& ) = delete;
	};

	inline bool probe_active( )
	{
		return g_probe_scope_depth != 0;
	}

	inline void set_exception_phase( const char* phase )
	{
		g_exception_phase = phase;
	}

	inline const char* exception_name( DWORD code )
	{
		switch ( code )
		{
		case EXCEPTION_ACCESS_VIOLATION:
			return "ACCESS_VIOLATION";
		case EXCEPTION_IN_PAGE_ERROR:
			return "IN_PAGE_ERROR";
		case EXCEPTION_ILLEGAL_INSTRUCTION:
			return "ILLEGAL_INSTRUCTION";
		case EXCEPTION_STACK_OVERFLOW:
			return "STACK_OVERFLOW";
		case EXCEPTION_INT_DIVIDE_BY_ZERO:
			return "INT_DIVIDE_BY_ZERO";
		case EXCEPTION_PRIV_INSTRUCTION:
			return "PRIV_INSTRUCTION";
		case 0xC0000409:
			return "STACK_BUFFER_OVERRUN";
		case diagnostic_snapshot_code:
			return "DIAGNOSTIC_SNAPSHOT";
		default:
			return "UNKNOWN";
		}
	}

	inline bool is_serious_exception( DWORD code )
	{
		switch ( code )
		{
		case EXCEPTION_ACCESS_VIOLATION:
		case EXCEPTION_IN_PAGE_ERROR:
		case EXCEPTION_ILLEGAL_INSTRUCTION:
		case EXCEPTION_STACK_OVERFLOW:
		case EXCEPTION_INT_DIVIDE_BY_ZERO:
		case EXCEPTION_PRIV_INSTRUCTION:
		case 0xC0000409:
			return true;
		default:
			return false;
		}
	}

	inline void format_module_address(
		char* destination,
		std::size_t capacity,
		const void* address )
	{
		HMODULE owner{};
		if ( address &&
			GetModuleHandleExA(
				GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
					GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
				reinterpret_cast<LPCSTR>( address ),
				&owner ) )
		{
			char path[ MAX_PATH ]{};
			GetModuleFileNameA( owner, path, MAX_PATH );
			const char* name = path;
			for ( const char* current = path; *current; ++current )
			{
				if ( *current == '\\' || *current == '/' )
				{
					name = current + 1;
				}
			}

			_snprintf_s(
				destination,
				capacity,
				_TRUNCATE,
				"%s+0x%llX",
				name,
				static_cast<unsigned long long>(
					reinterpret_cast<std::uintptr_t>( address ) -
					reinterpret_cast<std::uintptr_t>( owner ) ) );
			return;
		}

		_snprintf_s(
			destination,
			capacity,
			_TRUNCATE,
			"0x%p",
			address );
	}

	inline bool write_minidump(
		EXCEPTION_POINTERS* info,
		DWORD fault_thread_id )
	{
#if !defined( DEV )
		(void)info;
		(void)fault_thread_id;
		return false;
#else
		if ( !g_minidump_write )
		{
			write( level::error, "minidump unavailable: dbghelp export not resolved" );
			return false;
		}

		if ( !g_dump_path[ 0 ] )
		{
			write( level::error, "minidump unavailable: dump path not initialized" );
			return false;
		}

		const HANDLE file = CreateFileW(
			g_dump_path,
			GENERIC_WRITE,
			FILE_SHARE_READ,
			nullptr,
			CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH,
			nullptr );
		if ( file == INVALID_HANDLE_VALUE )
		{
			writef(
				level::error,
				"failed to create minidump; win32_error=%lu",
				GetLastError( ) );
			return false;
		}

		minidump_exception_information exception{};
		exception.thread_id = fault_thread_id;
		exception.exception_pointers = info;
		exception.client_pointers = FALSE;

		constexpr unsigned long rich_dump_type =
			minidump_with_indirectly_referenced_memory |
			minidump_with_thread_info |
			minidump_with_unloaded_modules;
		constexpr unsigned long safe_dump_type =
			minidump_with_thread_info |
			minidump_with_unloaded_modules;

		auto write_attempt = [&]( unsigned long dump_type, DWORD& error )
		{
			BOOL result{};
			g_writing_minidump = true;
			__try
			{
				result = g_minidump_write(
					GetCurrentProcess( ),
					GetCurrentProcessId( ),
					file,
					dump_type,
					&exception,
					nullptr,
					nullptr );
				if ( !result )
				{
					error = GetLastError( );
				}
			}
			__except ( EXCEPTION_EXECUTE_HANDLER )
			{
				error = GetExceptionCode( );
			}
			g_writing_minidump = false;
			return result != FALSE;
		};

		DWORD error{};
		bool result = write_attempt( rich_dump_type, error );
		if ( !result )
		{
			writef(
				level::warning,
				"rich minidump failed; error=0x%08lX, retrying safe minidump",
				error );

			LARGE_INTEGER beginning{};
			SetFilePointerEx( file, beginning, nullptr, FILE_BEGIN );
			SetEndOfFile( file );
			error = 0;
			result = write_attempt( safe_dump_type, error );
		}

		if ( !result )
		{
			writef(
				level::warning,
				"safe minidump failed; error=0x%08lX, retrying minimal minidump",
				error );

			LARGE_INTEGER beginning{};
			SetFilePointerEx( file, beginning, nullptr, FILE_BEGIN );
			SetEndOfFile( file );
			error = 0;
			result = write_attempt( 0, error );
		}

		FlushFileBuffers( file );
		CloseHandle( file );

		if ( !result )
		{
			writef(
				level::error,
				"MiniDumpWriteDump failed; error=0x%08lX",
				error );
			DeleteFileW( g_dump_path );
			return false;
		}

		char path[ MAX_PATH ]{};
		WideCharToMultiByte(
			CP_UTF8,
			0,
			g_dump_path,
			-1,
			path,
			MAX_PATH,
			nullptr,
			nullptr );
		writef( level::fatal, "minidump written: %s", path );
		return true;
#endif
	}

	inline void open_crash_artifacts( )
	{
		if ( InterlockedCompareExchange( &g_crash_artifacts_opened, 1, 0 ) != 0 )
		{
			return;
		}

		if ( g_log_path[ 0 ] )
		{
			const auto result = ShellExecuteW(
				nullptr,
				L"open",
				g_log_path,
				nullptr,
				nullptr,
				SW_SHOWNORMAL );
			if ( reinterpret_cast<INT_PTR>( result ) <= 32 )
			{
				writef(
					level::warning,
					"failed to open crash log; shell_error=%lld",
					static_cast<long long>( reinterpret_cast<INT_PTR>( result ) ) );
			}
		}

#if defined( DEV )
		if ( g_dump_path[ 0 ] )
		{
			ShellExecuteW(
				nullptr,
				L"open",
				g_dump_path,
				nullptr,
				nullptr,
				SW_SHOWNORMAL );
		}
#endif
	}

	inline void record_crash_impl(
		EXCEPTION_POINTERS* info,
		const char* stage,
		DWORD fault_thread_id,
		const hook_snapshot* hook )
	{
		const auto* record = info->ExceptionRecord;
		char location[ MAX_PATH + 32 ]{};
		format_module_address(
			location,
			sizeof( location ),
			record->ExceptionAddress );

		if ( ( record->ExceptionCode == EXCEPTION_ACCESS_VIOLATION ||
				record->ExceptionCode == EXCEPTION_IN_PAGE_ERROR ) &&
			record->NumberParameters > 1 )
		{
			const auto operation = record->ExceptionInformation[ 0 ];
			const char* operation_name =
				operation == 1 ? "write" : operation == 8 ? "execute" : "read";
			writef(
				level::fatal,
				"crash captured; stage=\"%s\" exception=%s (0x%08lX) "
				"fault_thread=%lu location=%s access=%s:0x%p",
				stage ? stage : "unknown",
				exception_name( record->ExceptionCode ),
				record->ExceptionCode,
				fault_thread_id,
				location,
				operation_name,
				reinterpret_cast<void*>( record->ExceptionInformation[ 1 ] ) );
		}
		else
		{
			writef(
				level::fatal,
				"crash captured; stage=\"%s\" exception=%s (0x%08lX) "
				"fault_thread=%lu location=%s",
				stage ? stage : "unknown",
				exception_name( record->ExceptionCode ),
				record->ExceptionCode,
				fault_thread_id,
				location );
		}

		if ( hook )
		{
			writef(
				level::fatal,
				"crash hook=%s arg0=0x%p arg1=0x%p arg2=0x%p arg3=0x%p",
				hook->name ? hook->name : "none",
				reinterpret_cast<void*>( hook->args[ 0 ] ),
				reinterpret_cast<void*>( hook->args[ 1 ] ),
				reinterpret_cast<void*>( hook->args[ 2 ] ),
				reinterpret_cast<void*>( hook->args[ 3 ] ) );
		}

#if defined( _M_X64 )
		if ( info->ContextRecord )
		{
			const auto* context = info->ContextRecord;
			writef(
				level::fatal,
				"registers; rip=0x%p rsp=0x%p rbp=0x%p "
				"rax=0x%p rbx=0x%p rcx=0x%p rdx=0x%p "
				"rsi=0x%p rdi=0x%p",
				reinterpret_cast<void*>( context->Rip ),
				reinterpret_cast<void*>( context->Rsp ),
				reinterpret_cast<void*>( context->Rbp ),
				reinterpret_cast<void*>( context->Rax ),
				reinterpret_cast<void*>( context->Rbx ),
				reinterpret_cast<void*>( context->Rcx ),
				reinterpret_cast<void*>( context->Rdx ),
				reinterpret_cast<void*>( context->Rsi ),
				reinterpret_cast<void*>( context->Rdi ) );
		}
#endif

		write_minidump( info, fault_thread_id );
		open_crash_artifacts( );
	}

	inline DWORD WINAPI crash_report_thread( void* )
	{
		record_crash_impl(
			&g_crash_request.pointers,
			g_crash_request.stage,
			g_crash_request.fault_thread_id,
			&g_crash_request.hook );
		return 0;
	}

	inline void record_crash( EXCEPTION_POINTERS* info, const char* stage )
	{
		if ( !info || !info->ExceptionRecord ||
			InterlockedCompareExchange( &g_crash_claimed, 1, 0 ) != 0 )
		{
			return;
		}

		const DWORD fault_thread_id = GetCurrentThreadId( );
		g_crash_request.record = *info->ExceptionRecord;
		g_crash_request.record.ExceptionRecord = nullptr;
		if ( info->ContextRecord )
		{
			g_crash_request.context = *info->ContextRecord;
		}
		g_crash_request.pointers = {
			&g_crash_request.record,
			info->ContextRecord ? &g_crash_request.context : nullptr
		};
		g_crash_request.hook = g_hook_snapshot;
		_snprintf_s(
			g_crash_request.stage,
			sizeof( g_crash_request.stage ),
			_TRUNCATE,
			"%s",
			stage ? stage : "unknown" );
		g_crash_request.fault_thread_id = fault_thread_id;

		// DbgHelp is not safe to invoke from a faulting thread. Use a clean
		// stack for every crash, not only stack-overflow exceptions.
		if ( const auto thread = CreateThread(
				nullptr,
				0,
				crash_report_thread,
				nullptr,
				0,
				nullptr ) )
		{
			const auto wait_result = WaitForSingleObject( thread, 30000 );
			CloseHandle( thread );
			if ( wait_result == WAIT_OBJECT_0 )
			{
				return;
			}

			writef(
				level::error,
				"minidump worker did not complete; wait_result=0x%08lX",
				wait_result );
			return;
		}

		writef(
			level::error,
			"failed to create minidump worker; win32_error=%lu",
			GetLastError( ) );
		record_crash_impl(
			&g_crash_request.pointers,
			g_crash_request.stage,
			fault_thread_id,
			&g_crash_request.hook );
	}

	inline void capture_snapshot( const char* stage )
	{
		CONTEXT context{};
		context.ContextFlags = CONTEXT_FULL;
		RtlCaptureContext( &context );

		EXCEPTION_RECORD record{};
		record.ExceptionCode = diagnostic_snapshot_code;
#if defined( _M_X64 )
		record.ExceptionAddress = reinterpret_cast<void*>( context.Rip );
#else
		record.ExceptionAddress = nullptr;
#endif

		EXCEPTION_POINTERS info{ &record, &context };
		record_crash( &info, stage );
	}
	inline void step( const char* message )
	{
		write( level::debug, message );
	}

	inline void shutdown( )
	{
		if ( !g_log_file )
		{
			return;
		}

		write( level::info, "diagnostics shutting down" );
		writef( level::info, "diagnostics log writes dropped=%ld", g_log_dropped );
		FlushFileBuffers( g_log_file );
		CloseHandle( g_log_file );
		g_log_file = nullptr;
	}

} // namespace diag
