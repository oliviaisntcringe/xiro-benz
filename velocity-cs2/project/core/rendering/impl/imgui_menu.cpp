#include <pch/pch.hpp>
#include <core/settings.hpp>

#include <external/imgui/imgui.h>
#include <external/imgui/backends/imgui_impl_dx11.h>
#include <external/imgui/backends/imgui_impl_win32.h>

#include "../imgui_menu.hpp"

namespace rendering {

	bool imgui_menu::initialize( IDXGISwapChain* swap_chain, HWND window )
	{
		if ( this->m_initialized )
		{
			return true;
		}

		ID3D11Device* device{};
		if ( !swap_chain || FAILED( swap_chain->GetDevice( __uuidof( ID3D11Device ), reinterpret_cast< void** >( &device ) ) ) )
		{
			return false;
		}

		ID3D11DeviceContext* context{};
		device->GetImmediateContext( &context );
		IMGUI_CHECKVERSION( );
		ImGui::CreateContext( );
		auto& style = ImGui::GetStyle( );
		style.WindowRounding = 0.0f;
		style.ChildRounding = 0.0f;
		style.FrameRounding = 0.0f;
		style.PopupRounding = 0.0f;
		style.WindowBorderSize = 1.0f;
		style.FrameBorderSize = 1.0f;
		style.ItemSpacing = ImVec2{ 8.0f, 6.0f };

		auto& colors = style.Colors;
		colors[ ImGuiCol_WindowBg ] = ImVec4{ 0.09f, 0.09f, 0.09f, 0.96f };
		colors[ ImGuiCol_ChildBg ] = ImVec4{ 0.125f, 0.125f, 0.125f, 0.96f };
		colors[ ImGuiCol_FrameBg ] = ImVec4{ 0.16f, 0.16f, 0.16f, 1.0f };
		colors[ ImGuiCol_FrameBgHovered ] = ImVec4{ 0.25f, 0.42f, 0.18f, 1.0f };
		colors[ ImGuiCol_FrameBgActive ] = ImVec4{ 0.34f, 0.58f, 0.22f, 1.0f };
		colors[ ImGuiCol_Button ] = ImVec4{ 0.12f, 0.12f, 0.12f, 1.0f };
		colors[ ImGuiCol_ButtonHovered ] = ImVec4{ 0.25f, 0.42f, 0.18f, 1.0f };
		colors[ ImGuiCol_ButtonActive ] = ImVec4{ 0.34f, 0.58f, 0.22f, 1.0f };
		colors[ ImGuiCol_Border ] = ImVec4{ 0.38f, 0.38f, 0.38f, 1.0f };
		colors[ ImGuiCol_Header ] = ImVec4{ 0.25f, 0.42f, 0.18f, 1.0f };
		colors[ ImGuiCol_HeaderHovered ] = ImVec4{ 0.34f, 0.58f, 0.22f, 1.0f };
		colors[ ImGuiCol_HeaderActive ] = ImVec4{ 0.34f, 0.58f, 0.22f, 1.0f };
		colors[ ImGuiCol_CheckMark ] = ImVec4{ 0.71f, 0.97f, 0.42f, 1.0f };
		colors[ ImGuiCol_SliderGrab ] = ImVec4{ 0.47f, 0.78f, 0.29f, 1.0f };
		colors[ ImGuiCol_SliderGrabActive ] = ImVec4{ 0.71f, 0.97f, 0.42f, 1.0f };
		ImGui::GetIO( ).IniFilename = nullptr;

		if ( !ImGui_ImplWin32_Init( window ) || !ImGui_ImplDX11_Init( device, context ) )
		{
			ImGui::DestroyContext( );
			context->Release( );
			device->Release( );
			return false;
		}

		context->Release( );
		device->Release( );
		this->m_initialized = true;
		return true;
	}

	void imgui_menu::shutdown( )
	{
		if ( !this->m_initialized )
		{
			return;
		}

		ImGui_ImplDX11_Shutdown( );
		ImGui_ImplWin32_Shutdown( );
		ImGui::DestroyContext( );
		this->m_initialized = false;
	}

	void imgui_menu::draw( )
	{
		if ( !this->m_initialized || !this->m_open )
		{
			return;
		}

		ImGui_ImplDX11_NewFrame( );
		ImGui_ImplWin32_NewFrame( );
		ImGui::NewFrame( );

		const auto viewport = ImGui::GetMainViewport( );
		const auto sidebar_width = 82.0f;
		const auto panel_width = 650.0f;
		const auto panel_height = 510.0f;
		const auto panel_pos = ImVec2{
			viewport->WorkPos.x + ( viewport->WorkSize.x - sidebar_width - panel_width ) * 0.5f,
			viewport->WorkPos.y + ( viewport->WorkSize.y - panel_height ) * 0.5f
		};

		ImGui::SetNextWindowPos( ImVec2{ viewport->WorkPos.x + viewport->WorkSize.x - sidebar_width, viewport->WorkPos.y } );
		ImGui::SetNextWindowSize( ImVec2{ sidebar_width, viewport->WorkSize.y } );
		ImGui::Begin( "##xiro_imgui_sidebar", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings );
		this->draw_sidebar( );
		ImGui::End( );

		ImGui::SetNextWindowPos( panel_pos );
		ImGui::SetNextWindowSize( ImVec2{ panel_width, panel_height } );
		ImGui::Begin( "XI.BENZ // RIFK7", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings );
		ImGui::TextColored( ImVec4{ 0.47f, 0.78f, 0.29f, 1.0f }, "XI.BENZ" );
		ImGui::SameLine( );
		ImGui::TextColored( ImVec4{ 0.55f, 0.29f, 0.69f, 1.0f }, "RIFK7" );
		ImGui::Separator( );
		this->draw_panel( );
		ImGui::End( );

		ImGui::Render( );
		ImGui_ImplDX11_RenderDrawData( ImGui::GetDrawData( ) );
	}

	void imgui_menu::draw_sidebar( )
	{
		static constexpr const char* tabs[ 6 ]{ "VISUALS", "AIMING", "MISC", "SKINS", "CONFIG", "INFO" };
		for ( auto i = 0; i < 6; ++i )
		{
			if ( ImGui::Selectable( tabs[ i ], this->m_tab == i, 0, ImVec2{ 66.0f, 42.0f } ) )
			{
				this->m_tab = i;
			}
		}
	}

	void imgui_menu::draw_panel( )
	{
		static constexpr const char* tab_names[ 6 ]{ "Visuals", "Aiming", "Misc", "Skins", "Config", "Info" };
		ImGui::TextColored( ImVec4{ 0.47f, 0.78f, 0.29f, 1.0f }, "%s // operator console", tab_names[ this->m_tab ] );
		ImGui::Spacing( );
		ImGui::BeginChild( "##imgui_content", ImVec2{ 0.0f, 0.0f }, true );
		if ( this->m_tab == 1 )
		{
			ImGui::Text( "Legitbot prototype" );
			ImGui::Checkbox( "Enable legitbot", &settings::g_combat.m_legitbot.enabled.value );
			ImGui::TextDisabled( "Existing controls will be migrated after the shell is verified." );
		}
		else
		{
			ImGui::Text( "Prototype tab" );
			ImGui::TextDisabled( "This panel is intentionally isolated from the current menu." );
		}
		ImGui::EndChild( );
	}

	bool imgui_menu::wndproc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
	{
		if ( !this->m_initialized || !this->m_open )
		{
			return false;
		}

		return ImGui_ImplWin32_WndProcHandler( hwnd, msg, wparam, lparam ) != 0;
	}

} // namespace rendering
