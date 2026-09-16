#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>

#include <algorithm>
#include <cmath>
#include <external/imgui/imgui.h>
#include <external/imgui/backends/imgui_impl_dx11.h>
#include <external/imgui/backends/imgui_impl_win32.h>

#include "../velocity-cs2/project/core/rendering/impl/retro_style.hpp"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam );

namespace {
	ID3D11Device* g_device{};
	ID3D11DeviceContext* g_context{};
	IDXGISwapChain* g_swap_chain{};
	ID3D11RenderTargetView* g_render_target{};
	UINT g_resize_width{};
	UINT g_resize_height{};

	struct preview_state
	{
		int top_tab{};
		int visual_section{};
		bool enabled{ true };
		bool occluded{ true };
		bool glow{ true };
		bool teammates{};
		bool recoil_crosshair{ true };
		bool bunnyhop{};
		bool hit_sound{ true };
		float max_distance{ 2500.0f };
		float smoothness{ 0.35f };
		int accent{};
		char profile_name[ 32 ]{ "preview" };
		float loading_elapsed{};
		bool loading_complete{};
	};

	void create_render_target( )
	{
		ID3D11Texture2D* back_buffer{};
		if ( SUCCEEDED( g_swap_chain->GetBuffer( 0, IID_PPV_ARGS( &back_buffer ) ) ) )
		{
			g_device->CreateRenderTargetView( back_buffer, nullptr, &g_render_target );
			back_buffer->Release( );
		}
	}

	void cleanup_render_target( )
	{
		if ( g_render_target )
		{
			g_render_target->Release( );
			g_render_target = nullptr;
		}
	}

	bool create_device( HWND window )
	{
		DXGI_SWAP_CHAIN_DESC swap_chain_desc{};
		swap_chain_desc.BufferCount = 2;
		swap_chain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swap_chain_desc.OutputWindow = window;
		swap_chain_desc.SampleDesc.Count = 1;
		swap_chain_desc.Windowed = TRUE;
		swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		swap_chain_desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

		constexpr D3D_FEATURE_LEVEL feature_levels[ 2 ]{
			D3D_FEATURE_LEVEL_11_0,
			D3D_FEATURE_LEVEL_10_0
		};
		D3D_FEATURE_LEVEL feature_level{};
		const auto result = D3D11CreateDeviceAndSwapChain(
			nullptr,
			D3D_DRIVER_TYPE_HARDWARE,
			nullptr,
			0,
			feature_levels,
			2,
			D3D11_SDK_VERSION,
			&swap_chain_desc,
			&g_swap_chain,
			&g_device,
			&feature_level,
			&g_context
		);
		if ( FAILED( result ) )
		{
			return false;
		}

		create_render_target( );
		return g_render_target != nullptr;
	}

	void cleanup_device( )
	{
		cleanup_render_target( );
		if ( g_swap_chain )
		{
			g_swap_chain->Release( );
			g_swap_chain = nullptr;
		}
		if ( g_context )
		{
			g_context->Release( );
			g_context = nullptr;
		}
		if ( g_device )
		{
			g_device->Release( );
			g_device = nullptr;
		}
	}

	void draw_preview_content( preview_state& state )
	{
		static constexpr const char* visual_sections[ 7 ]{
			"Enemy", "Team", "Local", "Viewmodel", "Items", "Projectiles", "Other"
		};
		static constexpr const char* top_names[ 5 ]{
			"Visuals", "Aiming", "Misc", "Skins", "Config"
		};

		ImGui::TextDisabled( "preview mode // no CS2 runtime attached" );
		ImGui::Spacing( );

		if ( state.top_tab == 0 )
		{
			const auto section_width = std::max( 120.0f, ( ImGui::GetContentRegionAvail( ).x - 12.0f ) / 3.0f );
			for ( auto i = 0; i < 7; ++i )
			{
				if ( i > 0 && i % 3 != 0 )
				{
					ImGui::SameLine( );
				}
				if ( ImGui::Selectable( visual_sections[ i ], state.visual_section == i, 0, ImVec2{ section_width, 28.0f } ) )
				{
					state.visual_section = i;
				}
			}
			ImGui::Separator( );
			ImGui::TextColored( rendering::retro::accent_green, "PLAYER OVERLAY" );
			ImGui::Checkbox( "Enable overlay", &state.enabled );
			ImGui::SameLine( );
			ImGui::Checkbox( "Glow", &state.glow );
			ImGui::Checkbox( "Show occluded", &state.occluded );
			ImGui::SameLine( );
			ImGui::Checkbox( "Teammates", &state.teammates );
			ImGui::SliderFloat( "Max distance", &state.max_distance, 500.0f, 5000.0f, "%.0f units" );
			ImGui::Spacing( );
			ImGui::TextColored( rendering::retro::accent_green, "COLOR PRESETS" );
			static constexpr const char* accents[ 3 ]{ "Green", "Purple", "Monochrome" };
			ImGui::Combo( "Accent", &state.accent, accents, IM_ARRAYSIZE( accents ) );
		}
		else if ( state.top_tab == 1 )
		{
			ImGui::TextColored( rendering::retro::accent_green, "AIM ASSIST" );
			ImGui::Checkbox( "Enable aim assist", &state.enabled );
			ImGui::Checkbox( "Recoil crosshair", &state.recoil_crosshair );
			ImGui::SliderFloat( "Smoothness", &state.smoothness, 0.05f, 1.0f, "%.2f" );
			ImGui::Separator( );
			ImGui::TextDisabled( "Weapon profile // rifle" );
			ImGui::BulletText( "Target selection: distance" );
			ImGui::BulletText( "Hitboxes: head, chest, stomach" );
		}
		else if ( state.top_tab == 2 )
		{
			ImGui::TextColored( rendering::retro::accent_green, "MOVEMENT & UTILITY" );
			ImGui::Checkbox( "Bunnyhop", &state.bunnyhop );
			ImGui::Checkbox( "Hit sound", &state.hit_sound );
			ImGui::Checkbox( "Performance overlay", &state.occluded );
			ImGui::Separator( );
			ImGui::TextDisabled( "Preview controls respond normally so spacing and states can be checked." );
		}
		else if ( state.top_tab == 3 )
		{
			ImGui::TextColored( rendering::retro::accent_green, "SKIN PREVIEW" );
			ImGui::BeginChild( "##skin_preview", ImVec2{ 0.0f, 190.0f }, true );
			ImGui::Text( "AK-47 | Slate" );
			ImGui::TextDisabled( "Factory New // pattern 412" );
			ImGui::Spacing( );
			ImGui::ProgressBar( 0.72f, ImVec2{ -1.0f, 12.0f }, "float 0.72" );
			ImGui::EndChild( );
		}
		else
		{
			ImGui::TextColored( rendering::retro::accent_green, "CONFIGURATION" );
			ImGui::InputText( "Profile name", state.profile_name, sizeof( state.profile_name ) );
			ImGui::TextDisabled( "Changes in this window are local to the preview process." );
		}

		ImGui::SetCursorPosY( ImGui::GetWindowHeight( ) - 38.0f );
		ImGui::Separator( );
		ImGui::TextColored( rendering::retro::text_muted, "XI.BENZ // %s // design preview", top_names[ state.top_tab ] );
	}

	void draw_preview_loading( float elapsed )
	{
		const auto* viewport = ImGui::GetMainViewport( );
		if ( !viewport )
		{
			return;
		}

		const auto center = ImVec2{
			viewport->WorkPos.x + viewport->WorkSize.x * 0.5f,
			viewport->WorkPos.y + viewport->WorkSize.y * 0.40f
		};
		auto* draw = ImGui::GetForegroundDrawList( );
		draw->AddRectFilled(
			viewport->WorkPos,
			ImVec2{ viewport->WorkPos.x + viewport->WorkSize.x, viewport->WorkPos.y + viewport->WorkSize.y },
			IM_COL32( 3, 6, 8, 236 )
		);

		static constexpr const char* glyphs[ 10 ]{ ".", "+", "x", "*", "o", "#", "@", "%", ":", "=" };
		for ( auto i = 0; i < 96; ++i )
		{
			const auto t = static_cast< float >( i ) / 95.0f;
			const auto angle = t * 6.2831853f * 4.0f + elapsed * 2.7f - t * 1.4f;
			const auto radius = 9.0f + t * t * 190.0f;
			const auto wobble = std::sin( elapsed * 4.0f + t * 18.0f ) * 4.0f;
			const auto x = center.x + std::cos( angle ) * ( radius + wobble );
			const auto y = center.y + std::sin( angle ) * ( radius + wobble ) * 0.62f;
			const auto glyph = glyphs[ ( i + static_cast< int >( elapsed * 12.0f ) ) % 10 ];
			const auto alpha = static_cast< int >( 38.0f + 172.0f * ( 1.0f - t ) * std::clamp( elapsed / 2.35f, 0.0f, 1.0f ) );
			draw->AddText( ImVec2{ x, y }, IM_COL32( 119, 200, 74, alpha ), glyph );
		}

		const auto ring_phase = std::sin( elapsed * 3.4f ) * 0.5f + 0.5f;
		draw->AddCircle( center, 18.0f + ring_phase * 8.0f, IM_COL32( 119, 200, 74, 190 ), 48, 2.0f );
		draw->AddCircle( center, 8.0f + ring_phase * 3.0f, IM_COL32( 185, 224, 128, 210 ), 32, 1.0f );

		if ( elapsed > 1.85f && elapsed < 3.15f )
		{
			const auto blast = std::clamp( ( elapsed - 1.85f ) / 0.85f, 0.0f, 1.0f );
			draw->AddCircleFilled( center, 26.0f * ( 1.0f - blast * 0.65f ), IM_COL32( 198, 238, 138, static_cast< int >( 100.0f * ( 1.0f - blast ) ) ) );
			for ( auto i = 0; i < 28; ++i )
			{
				const auto angle = static_cast< float >( i ) / 28.0f * 6.2831853f + elapsed * 1.2f;
				const auto inner = 25.0f + blast * 18.0f;
				const auto outer = inner + blast * ( 80.0f + static_cast< float >( i % 5 ) * 18.0f );
				draw->AddLine(
					ImVec2{ center.x + std::cos( angle ) * inner, center.y + std::sin( angle ) * inner },
					ImVec2{ center.x + std::cos( angle ) * outer, center.y + std::sin( angle ) * outer },
					IM_COL32( 119, 200, 74, static_cast< int >( 190.0f * ( 1.0f - blast ) ) ), 1.5f
				);
			}
		}

		const auto logo_alpha = static_cast< int >( 255.0f * std::clamp( ( elapsed - 1.15f ) / 0.55f, 0.0f, 1.0f ) );
		if ( logo_alpha > 0 )
		{
			const auto logo = "XI.BENZ";
			const auto logo_size = 34.0f + ring_phase * 2.0f;
			const auto logo_dimensions = ImGui::CalcTextSize( logo );
			draw->AddText( ImGui::GetFont( ), logo_size, ImVec2{ center.x - logo_dimensions.x * 0.5f, center.y + 48.0f }, IM_COL32( 185, 224, 128, logo_alpha ), logo );
			const auto signature = ">signature velocity_init: OK";
			const auto signature_dimensions = ImGui::CalcTextSize( signature );
			draw->AddText( ImVec2{ center.x - signature_dimensions.x * 0.5f, center.y + 86.0f }, IM_COL32( 156, 178, 164, logo_alpha ), signature );
		}

		static constexpr const char* checks[ 8 ]{
			"diagnostics / crash capture", "COM / multithreaded", "configuration / binds", "module memory regions",
			"integrity checks", "steam services", "function addresses", "render hooks"
		};
		const auto check_count = std::clamp( static_cast< int >( elapsed / 0.42f ) + 1, 1, 8 );
		const auto panel_width = std::min( 650.0f, std::max( 300.0f, viewport->WorkSize.x - 48.0f ) );
		const auto panel_x = viewport->WorkPos.x + ( viewport->WorkSize.x - panel_width ) * 0.5f;
		const auto panel_y = viewport->WorkPos.y + viewport->WorkSize.y - 178.0f;
		draw->AddRectFilled( ImVec2{ panel_x, panel_y }, ImVec2{ panel_x + panel_width, panel_y + 140.0f }, IM_COL32( 12, 19, 23, 238 ), 8.0f );
		draw->AddRect( ImVec2{ panel_x, panel_y }, ImVec2{ panel_x + panel_width, panel_y + 140.0f }, IM_COL32( 72, 92, 98, 230 ), 8.0f, 0, 1.0f );
		draw->AddText( ImVec2{ panel_x + 18.0f, panel_y + 14.0f }, IM_COL32( 119, 200, 74, 255 ), "INITIALIZING XI.BENZ" );
		for ( auto i = 0; i < check_count; ++i )
		{
			const auto column = i / 4;
			const auto row = i % 4;
			const auto x = panel_x + 18.0f + static_cast< float >( column ) * panel_width * 0.5f;
			const auto y = panel_y + 42.0f + static_cast< float >( row ) * 17.0f;
			draw->AddText( ImVec2{ x, y }, IM_COL32( 185, 224, 128, 255 ), "OK" );
			draw->AddText( ImVec2{ x + 34.0f, y }, IM_COL32( 204, 214, 213, 230 ), checks[ i ] );
		}
	}

	void draw_preview( preview_state& state )
	{
		const auto* viewport = ImGui::GetMainViewport( );
		if ( !viewport )
		{
			return;
		}

		const auto viewport_max = ImVec2{
			viewport->WorkPos.x + viewport->WorkSize.x,
			viewport->WorkPos.y + viewport->WorkSize.y
		};
		ImGui::GetBackgroundDrawList( )->AddRectFilled( viewport->WorkPos, viewport_max, IM_COL32( 8, 8, 8, 255 ) );
		if ( ImGui::IsKeyPressed( ImGuiKey_F8, false ) )
		{
			state.loading_elapsed = 0.0f;
			state.loading_complete = false;
		}
		if ( !state.loading_complete )
		{
			state.loading_elapsed += std::clamp( ImGui::GetIO( ).DeltaTime, 0.0f, 0.1f );
			draw_preview_loading( state.loading_elapsed );
			if ( state.loading_elapsed >= 4.4f )
			{
				state.loading_complete = true;
			}
			return;
		}

		static constexpr const char* top_names[ 5 ]{
			"Visuals", "Aiming", "Misc", "Skins", "Config"
		};
		const auto tab_width = std::clamp( viewport->WorkSize.x - 32.0f, 640.0f, 980.0f );
		const auto tab_pos = ImVec2{
			viewport->WorkPos.x + ( viewport->WorkSize.x - tab_width ) * 0.5f,
			viewport->WorkPos.y + 18.0f
		};
		ImGui::SetNextWindowPos( tab_pos, ImGuiCond_Always );
		ImGui::SetNextWindowSize( ImVec2{ tab_width, 48.0f }, ImGuiCond_Always );
		ImGui::Begin( "##preview_top_tabs", nullptr,
			ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoFocusOnAppearing );
		auto* tab_draw = ImGui::GetWindowDrawList( );
		const auto tab_min = ImGui::GetWindowPos( );
		rendering::retro::draw_retro_header( tab_draw, tab_min, ImVec2{ tab_min.x + tab_width, tab_min.y + 48.0f } );
		ImGui::SetCursorPos( ImVec2{ 12.0f, 8.0f } );
		ImGui::TextColored( rendering::retro::accent_green, "XI" );
		ImGui::SameLine( 0.0f, 2.0f );
		ImGui::TextColored( rendering::retro::accent_purple, "7" );
		ImGui::SameLine( 62.0f );
		ImGui::PushStyleVar( ImGuiStyleVar_FrameRounding, 5.0f );
		for ( auto i = 0; i < 5; ++i )
		{
			const auto active = state.top_tab == i;
			ImGui::PushID( i );
			ImGui::PushStyleColor( ImGuiCol_Header, active ? ImVec4{ 0.135f, 0.205f, 0.165f, 1.0f } : ImVec4{ 0.0f, 0.0f, 0.0f, 0.0f } );
			ImGui::PushStyleColor( ImGuiCol_HeaderHovered, ImVec4{ 0.185f, 0.285f, 0.205f, 0.95f } );
			ImGui::PushStyleColor( ImGuiCol_HeaderActive, ImVec4{ 0.235f, 0.380f, 0.245f, 1.0f } );
			const auto width = std::max( 88.0f, ( tab_width - 82.0f ) / 5.0f );
			if ( ImGui::Selectable( top_names[ i ], active, 0, ImVec2{ width, 31.0f } ) )
			{
				state.top_tab = i;
			}
			if ( active )
			{
				const auto item_min = ImGui::GetItemRectMin( );
				const auto item_max = ImGui::GetItemRectMax( );
				ImGui::GetWindowDrawList( )->AddRectFilled(
					ImVec2{ item_min.x + 12.0f, item_max.y - 2.0f },
					ImVec2{ item_max.x - 12.0f, item_max.y },
					ImGui::ColorConvertFloat4ToU32( rendering::retro::accent_green ), 2.0f
				);
			}
			ImGui::PopStyleColor( 3 );
			ImGui::PopID( );
			if ( i != 4 )
			{
				ImGui::SameLine( 0.0f, 4.0f );
			}
		}
		ImGui::PopStyleVar( );
		ImGui::End( );

		const auto workspace_width = std::clamp( viewport->WorkSize.x - 80.0f, 720.0f, 1260.0f );
		const auto workspace_height = std::clamp( viewport->WorkSize.y - 130.0f, 520.0f, 860.0f );
		const auto workspace_pos = ImVec2{
			viewport->WorkPos.x + ( viewport->WorkSize.x - workspace_width ) * 0.5f,
			viewport->WorkPos.y + 82.0f
		};
		ImGui::SetNextWindowPos( workspace_pos, ImGuiCond_FirstUseEver );
		ImGui::SetNextWindowSize( ImVec2{ workspace_width, workspace_height }, ImGuiCond_FirstUseEver );
		ImGui::SetNextWindowSizeConstraints( ImVec2{ 720.0f, 520.0f }, ImVec2{ viewport->WorkSize.x - 24.0f, viewport->WorkSize.y - 90.0f } );
		ImGui::Begin( "XI.BENZ // PREVIEW", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings );
		const auto header_pos = ImGui::GetCursorScreenPos( );
		const auto header_width = ImGui::GetContentRegionAvail( ).x;
		rendering::retro::draw_retro_header( ImGui::GetWindowDrawList( ), header_pos, ImVec2{ header_pos.x + header_width, header_pos.y + 58.0f } );
		ImGui::SetCursorScreenPos( ImVec2{ header_pos.x + 18.0f, header_pos.y + 12.0f } );
		ImGui::TextColored( rendering::retro::accent_green, "XI.BENZ" );
		ImGui::SameLine( );
		ImGui::TextColored( rendering::retro::accent_purple, "RIFK7" );
		ImGui::SetCursorScreenPos( ImVec2{ header_pos.x + 18.0f, header_pos.y + 35.0f } );
		ImGui::TextColored( rendering::retro::text_muted, "operator workspace // %s", top_names[ state.top_tab ] );
		ImGui::SetCursorScreenPos( ImVec2{ header_pos.x + 18.0f, header_pos.y + 72.0f } );
		draw_preview_content( state );
		ImGui::End( );
	}

	LRESULT WINAPI wnd_proc( HWND window, UINT message, WPARAM wparam, LPARAM lparam )
	{
		if ( ImGui_ImplWin32_WndProcHandler( window, message, wparam, lparam ) )
		{
			return TRUE;
		}
		if ( message == WM_SIZE && g_device != nullptr && wparam != SIZE_MINIMIZED )
		{
			g_resize_width = LOWORD( lparam );
			g_resize_height = HIWORD( lparam );
			return 0;
		}
		if ( message == WM_SYSCOMMAND && ( wparam & 0xfff0 ) == SC_KEYMENU )
		{
			return 0;
		}
		if ( message == WM_DESTROY )
		{
			PostQuitMessage( 0 );
			return 0;
		}
		return DefWindowProcW( window, message, wparam, lparam );
	}
}

int WINAPI wWinMain( HINSTANCE instance, HINSTANCE, PWSTR, int show_command )
{
	ImGui_ImplWin32_EnableDpiAwareness( );
	WNDCLASSEXW window_class{};
	window_class.cbSize = sizeof( WNDCLASSEXW );
	window_class.style = CS_CLASSDC;
	window_class.lpfnWndProc = wnd_proc;
	window_class.hInstance = instance;
	window_class.hCursor = LoadCursorW( nullptr, IDC_ARROW );
	window_class.lpszClassName = L"XiroBenzImGuiPreview";
	RegisterClassExW( &window_class );
	const auto window = CreateWindowW(
		window_class.lpszClassName,
		L"XI.BENZ ImGui Preview",
		WS_OVERLAPPEDWINDOW,
		100,
		100,
		1400,
		980,
		nullptr,
		nullptr,
		window_class.hInstance,
		nullptr
	);
	if ( !window || !create_device( window ) )
	{
		cleanup_device( );
		UnregisterClassW( window_class.lpszClassName, window_class.hInstance );
		return 1;
	}

	ShowWindow( window, show_command );
	UpdateWindow( window );

	IMGUI_CHECKVERSION( );
	ImGui::CreateContext( );
	auto& io = ImGui::GetIO( );
	io.IniFilename = nullptr;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	ImGui::StyleColorsDark( );
	rendering::retro::apply_rifk7_palette( ImGui::GetStyle( ) );
	ImGui_ImplWin32_Init( window );
	ImGui_ImplDX11_Init( g_device, g_context );

	preview_state state{};
	bool done{};
	while ( !done )
	{
		MSG message{};
		while ( PeekMessageW( &message, nullptr, 0, 0, PM_REMOVE ) )
		{
			TranslateMessage( &message );
			DispatchMessageW( &message );
			if ( message.message == WM_QUIT )
			{
				done = true;
			}
		}
		if ( done )
		{
			break;
		}

		if ( g_resize_width != 0 && g_resize_height != 0 )
		{
			cleanup_render_target( );
			g_swap_chain->ResizeBuffers( 0, g_resize_width, g_resize_height, DXGI_FORMAT_UNKNOWN, 0 );
			g_resize_width = 0;
			g_resize_height = 0;
			create_render_target( );
		}

		ImGui_ImplDX11_NewFrame( );
		ImGui_ImplWin32_NewFrame( );
		ImGui::NewFrame( );
		draw_preview( state );
		ImGui::Render( );

		constexpr float clear_color[ 4 ]{ 0.008f, 0.008f, 0.008f, 1.0f };
		g_context->OMSetRenderTargets( 1, &g_render_target, nullptr );
		g_context->ClearRenderTargetView( g_render_target, clear_color );
		ImGui_ImplDX11_RenderDrawData( ImGui::GetDrawData( ) );
		g_swap_chain->Present( 1, 0 );
	}

	ImGui_ImplDX11_Shutdown( );
	ImGui_ImplWin32_Shutdown( );
	ImGui::DestroyContext( );
	cleanup_device( );
	DestroyWindow( window );
	UnregisterClassW( window_class.lpszClassName, window_class.hInstance );
	return 0;
}
