#include <pch/pch.hpp>
#include <core/settings.hpp>

#include <external/imgui/imgui.h>
#include <external/imgui/backends/imgui_impl_dx11.h>
#include <external/imgui/backends/imgui_impl_win32.h>

#include "../imgui_menu.hpp"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam );

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
		const auto max_panel_width = viewport->WorkSize.x > sidebar_width + 420.0f
			? viewport->WorkSize.x - sidebar_width - 16.0f
			: 420.0f;
		const auto max_panel_height = viewport->WorkSize.y > 320.0f
			? viewport->WorkSize.y - 16.0f
			: 320.0f;

		ImGui::SetNextWindowPos( ImVec2{ viewport->WorkPos.x + viewport->WorkSize.x - sidebar_width, viewport->WorkPos.y } );
		ImGui::SetNextWindowSize( ImVec2{ sidebar_width, viewport->WorkSize.y } );
		ImGui::Begin( "##xiro_imgui_sidebar", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings );
		this->draw_sidebar( );
		ImGui::End( );

		ImGui::SetNextWindowPos( panel_pos, ImGuiCond_FirstUseEver );
		ImGui::SetNextWindowSize( ImVec2{ panel_width, panel_height }, ImGuiCond_FirstUseEver );
		ImGui::SetNextWindowSizeConstraints(
			ImVec2{ 420.0f, 320.0f },
			ImVec2{ max_panel_width, max_panel_height }
		);
		ImGui::Begin( "XI.BENZ // RIFK7", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings );
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
		static constexpr const char* visual_sections[ 4 ]{ "Enemy", "Team", "Local", "Viewmodel" };
		ImGui::TextColored( ImVec4{ 0.47f, 0.78f, 0.29f, 1.0f }, "%s // operator console", tab_names[ this->m_tab ] );
		ImGui::Spacing( );
		ImGui::BeginChild( "##imgui_content", ImVec2{ 0.0f, 0.0f }, true );
		if ( this->m_tab == 0 )
		{
			auto& esp = settings::g_esp;
			auto& player = esp.m_player;

			for ( auto i = 0; i < 4; ++i )
			{
				if ( i > 0 )
				{
					ImGui::SameLine( );
				}

				if ( ImGui::Selectable( visual_sections[ i ], this->m_visual_section == i, 0, ImVec2{ 92.0f, 28.0f } ) )
				{
					this->m_visual_section = i;
				}
			}
			ImGui::Separator( );

			auto draw_color = [ ]( const char* label, config::col& color )
			{
				float rgba[ 4 ]{
					static_cast< float >( color.value.r ) / 255.0f,
					static_cast< float >( color.value.g ) / 255.0f,
					static_cast< float >( color.value.b ) / 255.0f,
					static_cast< float >( color.value.a ) / 255.0f
				};
				if ( ImGui::ColorEdit4( label, rgba, ImGuiColorEditFlags_AlphaBar ) )
				{
					color.value.r = static_cast< std::uint8_t >( rgba[ 0 ] * 255.0f );
					color.value.g = static_cast< std::uint8_t >( rgba[ 1 ] * 255.0f );
					color.value.b = static_cast< std::uint8_t >( rgba[ 2 ] * 255.0f );
					color.value.a = static_cast< std::uint8_t >( rgba[ 3 ] * 255.0f );
				}
			};

			auto draw_layer = [ &draw_color ]( const char* label, settings::esp::chams_layer& layer )
			{
				ImGui::Checkbox( label, &layer.enabled.value );
				if ( layer.enabled.value && ImGui::TreeNode( label ) )
				{
					static constexpr const char* materials[ ]{
						"liquid", "metallic", "matte", "flat", "bloom", "outlines", "glow", "electric", "distortion", "hologram", "pearl",
						"liquid (iz)", "matte (iz)", "flat (iz)", "bloom (iz)", "outlines (iz)", "glow (iz)", "distortion (iz)", "hologram (iz)"
					};
					auto material = static_cast< int >( layer.material.value );
					if ( ImGui::Combo( "Material", &material, materials, IM_ARRAYSIZE( materials ) ) )
					{
						layer.material.value = static_cast< settings::esp::cham_ids >( material );
					}
					draw_color( "Color", layer.color );
					ImGui::TreePop( );
				}
			};

			auto draw_chams = [ &draw_layer ]( const char* label, settings::esp::chams_config& chams, bool overlay )
			{
				ImGui::Checkbox( label, &chams.enabled.value );
				if ( !chams.enabled.value || !ImGui::TreeNode( label ) )
				{
					return;
				}
				draw_layer( "Primary layer", chams.primary );
				draw_layer( "Secondary layer", chams.secondary );
				if ( overlay )
				{
					draw_layer( "Overlay layer", chams.overlay );
				}
				ImGui::TreePop( );
			};

			if ( this->m_visual_section < 2 )
			{
				auto& overlay = player.m_overlay[ this->m_visual_section ];
				auto& glow = this->m_visual_section == 0 ? player.m_glow.enemy : player.m_glow.team;
				auto& glow_ragdoll = this->m_visual_section == 0 ? player.m_glow.enemy_ragdoll : player.m_glow.team_ragdoll;
				auto& chams = this->m_visual_section == 0 ? player.m_chams.enemy : player.m_chams.team;
				auto& chams_ragdoll = this->m_visual_section == 0 ? player.m_chams.enemy_ragdoll : player.m_chams.team_ragdoll;

				ImGui::BeginGroup( );
				ImGui::Text( "Player ESP" );
				ImGui::Checkbox( "Enable ESP", &overlay.enabled.value );
				ImGui::Checkbox( "Box", &overlay.m_box.enabled.value );
				ImGui::Checkbox( "Skeleton", &overlay.m_skeleton.enabled.value );
				ImGui::Checkbox( "Health bar", &overlay.m_health_bar.enabled.value );
				ImGui::Checkbox( "Ammo bar", &overlay.m_ammo_bar.enabled.value );
				ImGui::Checkbox( "Name", &overlay.m_name.enabled.value );
				ImGui::Checkbox( "Weapon", &overlay.m_weapon.enabled.value );
				ImGui::Checkbox( "Info flags", &overlay.m_info_flags.enabled.value );
				ImGui::Checkbox( "Out-of-view arrows", &overlay.m_oof_arrow.enabled.value );
				ImGui::EndGroup( );

				ImGui::SameLine( );
				ImGui::BeginGroup( );
				ImGui::Text( "Chams and glow" );
				draw_chams( "Chams", chams, true );
				draw_chams( "Ragdoll chams", chams_ragdoll, false );
				ImGui::Checkbox( "Glow", &glow.enabled.value );
				if ( glow.enabled.value )
				{
					draw_color( "Glow color", glow.color );
				}
				ImGui::Checkbox( "Ragdoll glow", &glow_ragdoll.enabled.value );
				if ( glow_ragdoll.enabled.value )
				{
					draw_color( "Ragdoll glow color", glow_ragdoll.color );
				}
				ImGui::EndGroup( );
			}
			else if ( this->m_visual_section == 2 )
			{
				ImGui::Text( "Local player" );
				draw_chams( "Chams", player.m_chams.local, true );
				ImGui::Checkbox( "Lower opacity", &esp.m_local_alpha.enabled.value );
				if ( esp.m_local_alpha.enabled.value )
				{
					ImGui::SliderFloat( "Opacity", &esp.m_local_alpha.opacity.value, 0.0f, 1.0f, "%.2f" );
					ImGui::Checkbox( "Only when scoped", &esp.m_local_alpha.only_scoped.value );
				}
				draw_chams( "Ragdoll chams", player.m_chams.local_ragdoll, false );
				ImGui::Checkbox( "Glow", &player.m_glow.local.enabled.value );
				if ( player.m_glow.local.enabled.value )
				{
					draw_color( "Glow color", player.m_glow.local.color );
				}
				ImGui::Checkbox( "Ragdoll glow", &player.m_glow.local_ragdoll.enabled.value );
			}
			else
			{
				ImGui::Text( "Viewmodel" );
				draw_chams( "Weapon chams", esp.m_viewmodel.weapon, true );
				draw_chams( "Arms chams", esp.m_viewmodel.arms, true );
			}
		}
		else if ( this->m_tab == 1 )
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
