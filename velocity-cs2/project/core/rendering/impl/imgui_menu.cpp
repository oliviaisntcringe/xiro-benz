#include <pch/pch.hpp>
#include <core/features/features.hpp>
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
		static constexpr const char* tabs[ 7 ]{ "VISUALS", "LEGIT", "RAGE", "MISC", "SKINS", "CONFIG", "INFO" };
		for ( auto i = 0; i < 7; ++i )
		{
			if ( ImGui::Selectable( tabs[ i ], this->m_tab == i, 0, ImVec2{ 66.0f, 42.0f } ) )
			{
				this->m_tab = i;
			}
		}
	}

	void imgui_menu::draw_panel( )
	{
		static constexpr const char* tab_names[ 7 ]{ "Visuals", "Legit", "Rage", "Misc", "Skins", "Config", "Info" };
		static constexpr const char* visual_sections[ 4 ]{ "Enemy", "Team", "Local", "Viewmodel" };
		auto draw_config_color = [ ]( const char* label, config::col& color )
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

				if ( overlay.m_box.enabled.value && ImGui::TreeNode( "Box settings" ) )
				{
					static constexpr const char* box_styles[ ]{ "Full", "Cornered" };
					auto style = static_cast< int >( overlay.m_box.style.value );
					ImGui::Combo( "Style", &style, box_styles, IM_ARRAYSIZE( box_styles ) );
					overlay.m_box.style.value = static_cast< decltype( overlay.m_box.style.value ) >( style );
					ImGui::Checkbox( "Fill", &overlay.m_box.fill.value );
					ImGui::Checkbox( "Outline", &overlay.m_box.outline.value );
					ImGui::SliderFloat( "Corner length", &overlay.m_box.corner_length.value, 2.0f, 20.0f, "%.0f" );
					draw_color( "Visible##box", overlay.m_box.visible_color );
					draw_color( "Occluded##box", overlay.m_box.occluded_color );
					ImGui::TreePop( );
				}

				if ( overlay.m_skeleton.enabled.value && ImGui::TreeNode( "Skeleton settings" ) )
				{
					static constexpr const char* skeleton_modes[ ]{ "Normal", "Backtrack" };
					auto mode = static_cast< int >( overlay.m_skeleton.type.value );
					ImGui::Combo( "Mode", &mode, skeleton_modes, IM_ARRAYSIZE( skeleton_modes ) );
					overlay.m_skeleton.type.value = static_cast< decltype( overlay.m_skeleton.type.value ) >( mode );
					ImGui::SliderFloat( "Thickness", &overlay.m_skeleton.thickness.value, 0.5f, 4.0f, "%.1f" );
					draw_color( "Visible##skeleton", overlay.m_skeleton.visible_color );
					draw_color( "Occluded##skeleton", overlay.m_skeleton.occluded_color );
					ImGui::TreePop( );
				}

				if ( overlay.m_health_bar.enabled.value && ImGui::TreeNode( "Health bar settings" ) )
				{
					static constexpr const char* bar_positions[ ]{ "Left", "Top", "Bottom" };
					auto position = static_cast< int >( overlay.m_health_bar.position.value );
					ImGui::Combo( "Position##health", &position, bar_positions, IM_ARRAYSIZE( bar_positions ) );
					overlay.m_health_bar.position.value = static_cast< decltype( overlay.m_health_bar.position.value ) >( position );
					ImGui::Checkbox( "Outline##health", &overlay.m_health_bar.outline_setting.value );
					ImGui::Checkbox( "Gradient##health", &overlay.m_health_bar.gradient.value );
					ImGui::Checkbox( "Show value##health", &overlay.m_health_bar.show_value.value );
					ImGui::Checkbox( "Glow##health", &overlay.m_health_bar.glow.value );
					draw_color( "Full##health", overlay.m_health_bar.full_color );
					draw_color( "Low##health", overlay.m_health_bar.low_color );
					draw_color( "Background##health", overlay.m_health_bar.background_color );
					ImGui::SliderFloat( "Glow strength##health", &overlay.m_health_bar.glow_strength.value, 0.1f, 1.0f, "%.2f" );
					ImGui::TreePop( );
				}

				if ( overlay.m_ammo_bar.enabled.value && ImGui::TreeNode( "Ammo bar settings" ) )
				{
					static constexpr const char* bar_positions[ ]{ "Left", "Top", "Bottom" };
					auto position = static_cast< int >( overlay.m_ammo_bar.position.value );
					ImGui::Combo( "Position##ammo", &position, bar_positions, IM_ARRAYSIZE( bar_positions ) );
					overlay.m_ammo_bar.position.value = static_cast< decltype( overlay.m_ammo_bar.position.value ) >( position );
					ImGui::Checkbox( "Outline##ammo", &overlay.m_ammo_bar.outline_setting.value );
					ImGui::Checkbox( "Gradient##ammo", &overlay.m_ammo_bar.gradient.value );
					ImGui::Checkbox( "Show value##ammo", &overlay.m_ammo_bar.show_value.value );
					ImGui::Checkbox( "Glow##ammo", &overlay.m_ammo_bar.glow.value );
					draw_color( "Full##ammo", overlay.m_ammo_bar.full_color );
					draw_color( "Low##ammo", overlay.m_ammo_bar.low_color );
					draw_color( "Background##ammo", overlay.m_ammo_bar.background_color );
					ImGui::SliderFloat( "Glow strength##ammo", &overlay.m_ammo_bar.glow_strength.value, 0.1f, 1.0f, "%.2f" );
					ImGui::TreePop( );
				}

				if ( overlay.m_weapon.enabled.value && ImGui::TreeNode( "Weapon settings" ) )
				{
					static constexpr const char* weapon_display[ ]{ "Text", "Icon", "Text + icon" };
					auto display = static_cast< int >( overlay.m_weapon.display.value );
					ImGui::Combo( "Display", &display, weapon_display, IM_ARRAYSIZE( weapon_display ) );
					overlay.m_weapon.display.value = static_cast< decltype( overlay.m_weapon.display.value ) >( display );
					draw_color( "Text color##weapon", overlay.m_weapon.text_color );
					draw_color( "Icon color##weapon", overlay.m_weapon.icon_color );
					ImGui::TreePop( );
				}

				if ( overlay.m_info_flags.enabled.value && ImGui::TreeNode( "Info flag settings" ) )
				{
					static constexpr const char* flags[ ]{ "Money", "Armor", "Kit", "Scoped", "Defusing", "Flashed", "Ping", "Distance" };
					for ( auto flag = 0; flag < IM_ARRAYSIZE( flags ); ++flag )
					{
						ImGui::Checkbox( flags[ flag ], &overlay.m_info_flags.flags.values[ flag ] );
					}
					ImGui::TreePop( );
				}

				if ( overlay.m_oof_arrow.enabled.value && ImGui::TreeNode( "Out-of-view settings" ) )
				{
					ImGui::Checkbox( "Glow##oof", &overlay.m_oof_arrow.glow.value );
					ImGui::SliderFloat( "Width##oof", &overlay.m_oof_arrow.width.value, 4.0f, 40.0f, "%.0f" );
					ImGui::SliderFloat( "Height##oof", &overlay.m_oof_arrow.height.value, 4.0f, 40.0f, "%.0f" );
					ImGui::SliderFloat( "Radius X##oof", &overlay.m_oof_arrow.radius_x.value, 50.0f, 600.0f, "%.0f" );
					ImGui::SliderFloat( "Radius Y##oof", &overlay.m_oof_arrow.radius_y.value, 50.0f, 600.0f, "%.0f" );
					draw_color( "Visible##oof", overlay.m_oof_arrow.visible_color );
					draw_color( "Occluded##oof", overlay.m_oof_arrow.occluded_color );
					ImGui::TreePop( );
				}
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
			else if ( this->m_tab == 3 )
			{
				auto& misc = settings::g_misc;
				auto& movement = settings::g_movement;
				static constexpr const char* sections[ 4 ]{ "General", "Removals", "Camera", "HUD" };
				static constexpr const char* sound_types[ 10 ]{ "Shop click", "Home click", "Bell", "Killcard", "Bullet casing", "Coin pickup", "Item drop", "Popcan", "Key press", "Custom" };
				static constexpr const char* marker_types[ 3 ]{ "Classic", "Damage", "Both" };
				static constexpr const char* impact_types[ 3 ]{ "Overlay", "Sparks", "Both" };
				static constexpr const char* hat_types[ 2 ]{ "Kasa", "Bucket" };

				static int section{};
				for ( auto i = 0; i < 4; ++i )
				{
					if ( i > 0 )
					{
						ImGui::SameLine( );
					}
					if ( ImGui::Selectable( sections[ i ], section == i, 0, ImVec2{ 92.0f, 28.0f } ) )
					{
						section = i;
					}
				}
				ImGui::Separator( );

				if ( section == 0 )
				{
					auto& impacts = misc.m_impacts;
					auto& trajectory = misc.m_projectile_trajectory;
					auto& dlight = misc.m_dlight;
					auto& penetration = settings::g_combat.m_penetration_crosshair;

					ImGui::BeginGroup( );
					ImGui::Text( "Impacts and logs" );
					ImGui::Checkbox( "Hit logs", &impacts.hit_log.value );
					ImGui::SliderFloat( "Hit log duration", &impacts.hit_log_duration.value, 0.5f, 10.0f, "%.1f s" );
					ImGui::Checkbox( "Console logs", &impacts.console_log.value );
					ImGui::Checkbox( "Chat logs", &impacts.chat_log.value );
					ImGui::Checkbox( "Miss logs", &impacts.miss_log.value );
					ImGui::SliderFloat( "Miss log duration", &impacts.miss_log_duration.value, 0.5f, 10.0f, "%.1f s" );
					ImGui::Checkbox( "Hit sound", &impacts.hit_sound.value );
					auto hit_sound = static_cast< int >( impacts.hit_sound_type.value );
					if ( ImGui::Combo( "Hit sound type", &hit_sound, sound_types, IM_ARRAYSIZE( sound_types ) ) )
					{
						impacts.hit_sound_type.value = static_cast< settings::misc::impacts::sound_type >( hit_sound );
					}
					ImGui::SliderFloat( "Hit volume", &impacts.hit_sound_volume.value, 1.0f, 100.0f, "%.0f%%" );
					ImGui::Checkbox( "Hit marker", &impacts.hit_marker.value );
					auto marker = static_cast< int >( impacts.hit_marker_type.value );
					if ( ImGui::Combo( "Marker type", &marker, marker_types, IM_ARRAYSIZE( marker_types ) ) )
					{
						impacts.hit_marker_type.value = static_cast< settings::misc::impacts::marker_type >( marker );
					}
					ImGui::SliderFloat( "Marker duration", &impacts.hit_marker_duration.value, 0.1f, 5.0f, "%.1f s" );
					draw_config_color( "Marker color", impacts.hit_marker_color );
					ImGui::Checkbox( "Hit effect", &impacts.hit_effect.value );
					draw_config_color( "Hit effect color", impacts.hit_effect_color );
					ImGui::SliderFloat( "Hit effect duration", &impacts.hit_effect_duration.value, 0.1f, 5.0f, "%.1f s" );
					ImGui::SliderFloat( "Hit effect strength", &impacts.hit_effect_strength.value, 1.0f, 100.0f, "%.0f%%" );
					ImGui::Checkbox( "Death sound", &impacts.death_sound.value );
					auto death_sound = static_cast< int >( impacts.death_sound_type.value );
					if ( ImGui::Combo( "Death sound type", &death_sound, sound_types, IM_ARRAYSIZE( sound_types ) ) )
					{
						impacts.death_sound_type.value = static_cast< settings::misc::impacts::sound_type >( death_sound );
					}
					ImGui::SliderFloat( "Death volume", &impacts.death_sound_volume.value, 1.0f, 100.0f, "%.0f%%" );
					ImGui::Checkbox( "Death effect", &impacts.death_effect.value );
					draw_config_color( "Death effect color", impacts.death_effect_color );
					ImGui::EndGroup( );

					ImGui::SameLine( );
					ImGui::BeginGroup( );
					ImGui::Text( "World and movement" );
					ImGui::Checkbox( "Projectile trajectory", &trajectory.enabled.value );
					ImGui::Checkbox( "Straight throw", &trajectory.straight_throw.value );
					ImGui::Checkbox( "Trajectory glow", &trajectory.glow.value );
					ImGui::SliderFloat( "Trajectory glow strength", &trajectory.glow_strength.value, 0.1f, 1.0f, "%.2f" );
					draw_config_color( "Held color", trajectory.held_color );
					draw_config_color( "Thrown color", trajectory.thrown_color );
					draw_config_color( "Damage held color", trajectory.will_deal_damage_held_color );
					draw_config_color( "Damage thrown color", trajectory.will_deal_damage_thrown_color );
					ImGui::Checkbox( "Dynamic light", &dlight.enabled.value );
					draw_config_color( "Dynamic light color", dlight.color );
					ImGui::SliderFloat( "Light radius", &dlight.radius.value, 50.0f, 15000.0f, "%.0f" );
					ImGui::SliderFloat( "Light Z offset", &dlight.z_offset.value, 0.0f, 100.0f, "%.0f" );
					ImGui::Checkbox( "Penetration crosshair", &penetration.enabled.value );
					ImGui::Checkbox( "Penetration glow", &penetration.glow.value );
					ImGui::SliderFloat( "Penetration glow strength", &penetration.glow_strength.value, 0.1f, 1.0f, "%.2f" );
					draw_config_color( "Can penetrate", penetration.can_penetrate_fill );
					draw_config_color( "Blocked", penetration.blocked_fill );
					ImGui::Checkbox( "Bunnyhop", &movement.bhop.value );
					ImGui::Checkbox( "Autostrafe", &movement.m_test_strafer.enabled.value );
					ImGui::Checkbox( "Jumpbug", &movement.jumpbug.value );
					ImGui::Checkbox( "Fast ladder", &movement.fastladder.value );
					ImGui::Checkbox( "Edgejump", &movement.edgejump.value );
					ImGui::Checkbox( "Edgestop", &movement.edgestop.value );
					ImGui::Checkbox( "Edgebug", &movement.edgebug.value );
					ImGui::SliderInt( "Edgebug mode", &movement.edgebug_mode.value, 0, 4 );
					ImGui::SliderInt( "Edgebug passes", &movement.edgebug_passes.value, 1, 5 );
					ImGui::Checkbox( "Edgebug jump steps", &movement.edgebug_include_jump_steps.value );
					ImGui::Checkbox( "Slowwalk", &movement.slowwalk.value );
					ImGui::SliderFloat( "Slowwalk speed", &movement.slowwalk_speed.value, 1.0f, 100.0f, "%.0f" );
					ImGui::EndGroup( );
				}
				else if ( section == 1 )
				{
					auto& removals = misc.m_removals;
					ImGui::Text( "Removals" );
					ImGui::Checkbox( "Remove crosshair", &removals.crosshair.value );
					ImGui::Checkbox( "Remove scope", &removals.scope.value );
					ImGui::Checkbox( "Remove overhead", &removals.overhead.value );
					ImGui::Checkbox( "Remove legs", &removals.legs.value );
					ImGui::Checkbox( "Remove recoil", &removals.recoil.value );
					ImGui::Checkbox( "Remove skybox fog", &removals.skybox_fog.value );
					ImGui::Checkbox( "Remove 3D skybox", &removals.skybox_3d.value );
					ImGui::Checkbox( "Remove decals", &removals.decals.value );
					ImGui::Checkbox( "Remove smoke", &removals.smoke.value );
					ImGui::SliderFloat( "Flash alpha", &removals.flash_alpha.value, 0.0f, 100.0f, "%.0f%%" );
				}
				else if ( section == 2 )
				{
					auto& camera = misc.m_camera;
					auto& viewmodel = misc.m_viewmodel_adjust;
					ImGui::BeginGroup( );
					ImGui::Text( "Camera" );
					ImGui::Checkbox( "Custom FOV", &camera.change_fov.value );
					ImGui::SliderFloat( "FOV", &camera.fov.value, 60.0f, 150.0f, "%.0f" );
					ImGui::Checkbox( "Scoped FOV override", &camera.scoped_fov_override.value );
					ImGui::SliderFloat( "Scoped FOV", &camera.scoped_fov.value, 10.0f, 90.0f, "%.0f" );
					ImGui::Checkbox( "Thirdperson", &camera.thirdperson.value );
					ImGui::SliderFloat( "Thirdperson distance", &camera.thirdperson_distance.value, 35.0f, 200.0f, "%.0f" );
					ImGui::SliderFloat( "Thirdperson hull size", &camera.thirdperson_hull_size.value, 0.0f, 20.0f, "%.0f" );
					ImGui::Checkbox( "Custom aspect ratio", &camera.change_aspect_ratio.value );
					ImGui::SliderFloat( "Aspect ratio", &camera.aspect_ratio.value, 1.0f, 1.78f, "%.3f" );
					ImGui::EndGroup( );
					ImGui::SameLine( );
					ImGui::BeginGroup( );
					ImGui::Text( "Viewmodel" );
					ImGui::Checkbox( "Viewmodel adjust", &viewmodel.enabled.value );
					ImGui::SliderFloat( "Offset X", &viewmodel.offset_x.value, -10.0f, 10.0f, "%.1f" );
					ImGui::SliderFloat( "Offset Y", &viewmodel.offset_y.value, -10.0f, 10.0f, "%.1f" );
					ImGui::SliderFloat( "Offset Z", &viewmodel.offset_z.value, -10.0f, 10.0f, "%.1f" );
					ImGui::SliderFloat( "Viewmodel FOV", &viewmodel.fov.value, 54.0f, 90.0f, "%.0f" );
					ImGui::EndGroup( );
				}
				else if ( this->m_tab == 4 )
				{
					auto& changer = settings::g_changer;
					auto& econ = features::changer::g_econ_item_system;
					static constexpr const char* categories[ 4 ]{ "Weapons", "Knives", "Gloves", "Agents" };
					static int category{};
					static int selected_item{};
					static int selected_paint{};

					ImGui::Text( "Skin changer" );
					for ( auto i = 0; i < 4; ++i )
					{
						if ( i > 0 )
						{
							ImGui::SameLine( );
						}
						if ( ImGui::Selectable( categories[ i ], category == i, 0, ImVec2{ 92.0f, 28.0f } ) )
						{
							category = i;
							selected_item = 0;
						}
					}
					ImGui::Separator( );

					const auto& items = category == 0 ? econ.guns( ) : category == 1 ? econ.knives( ) : category == 2 ? econ.gloves( ) : econ.agents( );
					if ( items.empty( ) )
					{
						ImGui::TextDisabled( "No item definitions are available yet." );
					}
					else
					{
						selected_item = std::clamp( selected_item, 0, static_cast< int >( items.size( ) ) - 1 );
						std::vector< const char* > item_names;
						item_names.reserve( items.size( ) );
						for ( const auto* item : items )
						{
							item_names.push_back( item->localized_name.empty( ) ? item->name.c_str( ) : item->localized_name.c_str( ) );
						}

						if ( ImGui::Combo( "Item", &selected_item, item_names.data( ), static_cast< int >( item_names.size( ) ) ) )
						{
							selected_paint = 0;
						}

						const auto* item = items[ selected_item ];
						if ( category == 3 )
						{
							if ( item->team( ) == 3 )
							{
								changer.agents.ct_def = item->def_index;
							}
							else if ( item->team( ) == 2 )
							{
								changer.agents.t_def = item->def_index;
							}
							ImGui::Text( "Selected agent: %s", item_names[ selected_item ] );
						}
						else
						{
							std::vector< const char* > paint_names;
							std::vector< int > paint_ids;
							for ( const auto& paint : econ.paint_kits( ) )
							{
								if ( category == 0 && paint.id == 0 )
								{
									continue;
								}
								paint_ids.push_back( paint.id );
								paint_names.push_back( paint.localized_name.empty( ) ? paint.name.c_str( ) : paint.localized_name.c_str( ) );
							}

							if ( paint_names.empty( ) )
							{
								ImGui::TextDisabled( "No paint kits are available yet." );
							}
							else
							{
								selected_paint = std::clamp( selected_paint, 0, static_cast< int >( paint_names.size( ) ) - 1 );
								if ( ImGui::Combo( "Paint kit", &selected_paint, paint_names.data( ), static_cast< int >( paint_names.size( ) ) ) )
								{
									auto& applied = changer.skins.data[ item->def_index ];
									applied.paint_kit_id = paint_ids[ selected_paint ];
								}

								auto& applied = changer.skins.data[ item->def_index ];
								if ( applied.paint_kit_id == 0 )
								{
									applied.paint_kit_id = paint_ids[ selected_paint ];
								}
								ImGui::SliderFloat( "Wear", &applied.wear, 0.0f, 1.0f, "%.4f" );
								ImGui::SliderInt( "Seed", &applied.seed, 0, 1000 );
								ImGui::Checkbox( "StatTrak", &applied.stattrak );
								if ( ImGui::Button( "Clear selected skin" ) )
								{
									changer.skins.data.erase( item->def_index );
								}
							}
						}
					}
				}
				else
				{
					auto& hud = misc.m_hud;
					ImGui::BeginGroup( );
					ImGui::Text( "Crosshair and scope" );
					ImGui::Checkbox( "Crosshair overlay", &hud.m_crosshair.enabled.value );
					ImGui::SliderFloat( "Crosshair size", &hud.m_crosshair.size.value, 0.5f, 10.0f, "%.1f" );
					ImGui::SliderFloat( "Crosshair outline", &hud.m_crosshair.outline.value, 0.0f, 4.0f, "%.1f" );
					draw_config_color( "Crosshair color", hud.m_crosshair.color );
					draw_config_color( "Crosshair outline color", hud.m_crosshair.outline_color );
					ImGui::Checkbox( "Scope overlay", &hud.m_scope.enabled.value );
					ImGui::SliderFloat( "Scope line length", &hud.m_scope.line_length.value, 10.0f, 500.0f, "%.0f" );
					ImGui::SliderFloat( "Scope gap", &hud.m_scope.gap.value, 0.0f, 50.0f, "%.0f" );
					ImGui::SliderFloat( "Scope thickness", &hud.m_scope.thickness.value, 0.5f, 5.0f, "%.2f" );
					ImGui::SliderFloat( "Scope animation speed", &hud.m_scope.anim_speed.value, 1.0f, 30.0f, "%.0f" );
					draw_config_color( "Scope color", hud.m_scope.color );
					ImGui::Checkbox( "Scope fade in", &hud.m_scope.fade_in.value );
					ImGui::Checkbox( "Scope glow", &hud.m_scope.glow.value );
					ImGui::SliderFloat( "Scope glow strength", &hud.m_scope.glow_strength.value, 0.1f, 1.0f, "%.2f" );
					ImGui::EndGroup( );
					ImGui::SameLine( );
					ImGui::BeginGroup( );
					ImGui::Text( "Hat and velocity" );
					ImGui::Checkbox( "Hat", &hud.m_hat.enabled.value );
					auto hat = static_cast< int >( hud.m_hat.type.value );
					if ( ImGui::Combo( "Hat type", &hat, hat_types, IM_ARRAYSIZE( hat_types ) ) )
					{
						hud.m_hat.type.value = static_cast< settings::misc::hud::hat::hat_type >( hat );
					}
					draw_config_color( "Hat color", hud.m_hat.color );
					draw_config_color( "Hat secondary color", hud.m_hat.secondary_color );
					ImGui::Checkbox( "Hat glow", &hud.m_hat.glow.value );
					ImGui::SliderFloat( "Hat glow strength", &hud.m_hat.glow_strength.value, 0.1f, 1.0f, "%.2f" );
					ImGui::Checkbox( "Velocity counter", &hud.m_velocity.counter.value );
					ImGui::Checkbox( "Velocity chart", &hud.m_velocity.chart.value );
					draw_config_color( "Velocity color", hud.m_velocity.color );
					ImGui::SliderFloat( "Velocity bottom offset", &hud.m_velocity.bottom_offset.value, 0.0f, 300.0f, "%.0f" );
					ImGui::SliderFloat( "Chart width", &hud.m_velocity.chart_width.value, 50.0f, 500.0f, "%.0f" );
					ImGui::SliderFloat( "Chart height", &hud.m_velocity.chart_height.value, 20.0f, 150.0f, "%.0f" );
					ImGui::Separator( );
					ImGui::Text( "General" );
					ImGui::Checkbox( "Reveal radar", &misc.reveal_radar.value );
					ImGui::Checkbox( "Preserve killfeed", &misc.preserve_killfeed.value );
					ImGui::Checkbox( "Disable game logs", &misc.disable_game_logs.value );
					ImGui::Checkbox( "Scoreboard weapons", &misc.m_scoreboard_weapons.enabled.value );
					draw_config_color( "Scoreboard color", misc.m_scoreboard_weapons.color );
					ImGui::Checkbox( "Clantag", &misc.m_name_changer.clantag.value );
					ImGui::Checkbox( "Override name", &misc.m_name_changer.override_name.value );
					static char name_buffer[ 64 ]{};
					static std::string name_buffer_value{};
					if ( name_buffer_value != misc.m_name_changer.name.value )
					{
						std::snprintf( name_buffer, sizeof( name_buffer ), "%s", misc.m_name_changer.name.value.c_str( ) );
						name_buffer_value = misc.m_name_changer.name.value;
					}
					if ( ImGui::InputText( "Name", name_buffer, sizeof( name_buffer ) ) )
					{
						misc.m_name_changer.name.value = name_buffer;
						name_buffer_value = misc.m_name_changer.name.value;
					}
					ImGui::EndGroup( );
				}
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
			auto& legitbot = settings::g_combat.m_legitbot;
			static constexpr const char* weapon_groups[ 6 ]{ "Pistols", "SMG", "Rifles", "Shotguns", "Snipers", "LMG" };
			static constexpr const char* hitboxes[ 5 ]{ "Head", "Chest", "Stomach", "Arms", "Legs" };

			ImGui::Checkbox( "Enable legitbot", &legitbot.enabled.value );
			ImGui::Separator( );
			for ( auto i = 0; i < 6; ++i )
			{
				if ( i > 0 )
				{
					ImGui::SameLine( );
				}
				if ( ImGui::Selectable( weapon_groups[ i ], this->m_aim_weapon_group == i, 0, ImVec2{ 82.0f, 28.0f } ) )
				{
					this->m_aim_weapon_group = i;
				}
			}
			ImGui::Separator( );

			auto& group = legitbot.groups[ this->m_aim_weapon_group ];
			if ( !legitbot.enabled.value )
			{
				ImGui::TextDisabled( "Enable legitbot to edit weapon-group settings." );
			}
			else
			{
				ImGui::BeginGroup( );
				ImGui::Text( "Aimbot" );
				ImGui::Checkbox( "Aimbot##aim", &group.aimbot.value );
				ImGui::Checkbox( "Silent##aim", &group.silent.value );
				ImGui::Checkbox( "No spread##aim", &group.no_spread.value );
				ImGui::SliderFloat( "FOV##aim", &group.fov.value, 0.5f, 30.0f, "%.1f deg" );
				ImGui::SliderInt( "Smooth##aim", &group.smooth.value, 0, 100 );
				ImGui::Text( "Hitboxes" );
				for ( auto i = 0; i < 5; ++i )
				{
					ImGui::Checkbox( hitboxes[ i ], &group.hitboxes.values[ i ] );
				}
				ImGui::Checkbox( "Draw FOV##aim", &group.visualize_fov.value );
				float fov_color[ 4 ]{
					static_cast< float >( group.fov_color.value.r ) / 255.0f,
					static_cast< float >( group.fov_color.value.g ) / 255.0f,
					static_cast< float >( group.fov_color.value.b ) / 255.0f,
					static_cast< float >( group.fov_color.value.a ) / 255.0f
				};
				if ( ImGui::ColorEdit4( "FOV color", fov_color, ImGuiColorEditFlags_AlphaBar ) )
				{
					group.fov_color.value.r = static_cast< std::uint8_t >( fov_color[ 0 ] * 255.0f );
					group.fov_color.value.g = static_cast< std::uint8_t >( fov_color[ 1 ] * 255.0f );
					group.fov_color.value.b = static_cast< std::uint8_t >( fov_color[ 2 ] * 255.0f );
					group.fov_color.value.a = static_cast< std::uint8_t >( fov_color[ 3 ] * 255.0f );
				}
				ImGui::EndGroup( );

				ImGui::SameLine( );
				ImGui::BeginGroup( );
				ImGui::Text( "Recoil control" );
				ImGui::Checkbox( "RCS##aim", &group.rcs.value );
				ImGui::SliderInt( "RCS min##aim", &group.rcs_min.value, 50, 150, "%d%%" );
				ImGui::SliderInt( "RCS max##aim", &group.rcs_max.value, 50, 150, "%d%%" );
				ImGui::Checkbox( "Standalone RCS##aim", &group.standalone_rcs.value );
				ImGui::SliderInt( "Strength##srcs", &group.standalone_rcs_strength.value, 0, 100, "%d%%" );
				ImGui::SliderInt( "Min##srcs", &group.standalone_rcs_min.value, 50, 150, "%d%%" );
				ImGui::SliderInt( "Max##srcs", &group.standalone_rcs_max.value, 50, 150, "%d%%" );

				ImGui::Spacing( );
				ImGui::Text( "Triggerbot" );
				ImGui::Checkbox( "Triggerbot##aim", &group.triggerbot.value );
				ImGui::SliderInt( "Delay##trigger", &group.trigger_delay.value, 0, 250, "%d ms" );
				ImGui::SliderInt( "Hitchance##trigger", &group.trigger_hitchance.value, 0, 100, "%d%%" );
				ImGui::Checkbox( "Head only##trigger", &group.trigger_head_only.value );
				ImGui::Checkbox( "Seed prediction##trigger", &group.give_me_your_seed.value );

				ImGui::Spacing( );
				ImGui::Text( "Other" );
				ImGui::Checkbox( "Autowall##aim", &group.autowall.value );
				ImGui::SliderInt( "Minimum damage##aim", &group.min_damage.value, 1, 125 );
				ImGui::EndGroup( );
			}
		}
		else if ( this->m_tab == 2 )
		{
			auto& combat = settings::g_combat;
			auto& ragebot = combat.m_ragebot;
			auto& group = ragebot.groups[ this->m_aim_weapon_group ];
			auto& anti_aim = combat.m_antiaim;
			auto& quick_peek = combat.m_quickpeek;
			auto& duck_peek = combat.m_duckpeek;
			auto& zeusbot = combat.m_zeusbot;
			auto& knifebot = combat.m_knifebot;
			auto& autos = combat.m_autos;

			static constexpr const char* weapon_groups[ 6 ]{ "Pistols", "SMG", "Rifles", "Shotguns", "Snipers", "LMG" };
			static constexpr const char* hitboxes[ 6 ]{ "Head", "Chest", "Stomach", "Arms", "Legs", "Feet" };
			static constexpr const char* pitch_modes[ 3 ]{ "None", "Down", "Up" };

			ImGui::Checkbox( "Enable ragebot", &ragebot.enabled.value );
			ImGui::Separator( );
			for ( auto i = 0; i < 6; ++i )
			{
				if ( i > 0 )
				{
					ImGui::SameLine( );
				}
				if ( ImGui::Selectable( weapon_groups[ i ], this->m_aim_weapon_group == i, 0, ImVec2{ 82.0f, 28.0f } ) )
				{
					this->m_aim_weapon_group = i;
				}
			}
			ImGui::Separator( );

			if ( !ragebot.enabled.value )
			{
				ImGui::TextDisabled( "Enable ragebot to edit weapon-group settings." );
			}
			else
			{
				ImGui::BeginGroup( );
				ImGui::Text( "Rage aimbot" );
				ImGui::Checkbox( "Silent##rage", &group.silent.value );
				ImGui::Checkbox( "No spread##rage", &group.no_spread.value );
				ImGui::Checkbox( "Autostop##rage", &group.autostop.value );
				ImGui::Checkbox( "Force shot in air##rage", &group.force_shot_air.value );
				ImGui::Checkbox( "Force shot on ground##rage", &group.force_shot.value );
				ImGui::Checkbox( "Extrapolation##rage", &combat.m_lagcomp.extrapolation.value );
				ImGui::SliderFloat( "FOV##rage", &group.max_fov.value, 1.0f, 180.0f, "%.0f deg" );
				ImGui::SliderInt( "Hitchance##rage", &group.hitchance.value, 0, 100, "%d%%" );
				ImGui::SliderInt( "Minimum damage##rage", &group.min_damage.value, 1, 125 );
				ImGui::Checkbox( "Hitchance override##rage", &group.hitchance_override.value );
				if ( group.hitchance_override.value )
				{
					ImGui::SliderInt( "Override hitchance##rage", &group.hitchance_override_value.value, 0, 100, "%d%%" );
				}
				ImGui::Checkbox( "Minimum damage override##rage", &group.min_damage_override.value );
				if ( group.min_damage_override.value )
				{
					ImGui::SliderInt( "Override damage##rage", &group.min_damage_override_value.value, 0, 130 );
				}
				ImGui::EndGroup( );

				ImGui::SameLine( );
				ImGui::BeginGroup( );
				ImGui::Text( "Multipoint" );
				ImGui::Checkbox( "Force body aim##rage", &group.body_aim.value );
				ImGui::Checkbox( "Dynamic point scale##rage", &group.dynamic_pointscale.value );
				ImGui::Checkbox( "Debug multipoints##rage", &group.debug_multipoints.value );
				ImGui::SliderFloat( "Point scale##rage", &group.pointscale.value, 0.0f, 100.0f, "%.0f%%" );
				for ( auto i = 0; i < 6; ++i )
				{
					ImGui::Checkbox( hitboxes[ i ], &group.hitboxes.values[ i ] );
				}

				ImGui::Spacing( );
				ImGui::Text( "Other bots" );
				ImGui::Checkbox( "Auto revolver##rage", &autos.revolver.value );
				ImGui::Checkbox( "Auto scope##rage", &autos.scope.value );
				ImGui::Checkbox( "Zeusbot##rage", &zeusbot.enabled.value );
				if ( zeusbot.enabled.value )
				{
					ImGui::SliderFloat( "Zeus FOV##rage", &zeusbot.max_fov.value, 1.0f, 180.0f, "%.0f deg" );
					ImGui::Checkbox( "Drop after##rage", &zeusbot.drop_after.value );
				}
				ImGui::Checkbox( "Knifebot##rage", &knifebot.enabled.value );
				if ( knifebot.enabled.value )
				{
					ImGui::SliderFloat( "Knife FOV##rage", &knifebot.max_fov.value, 1.0f, 180.0f, "%.0f deg" );
				}
				ImGui::EndGroup( );

				ImGui::Separator( );
				ImGui::Text( "Anti aim" );
				ImGui::Checkbox( "Enable anti aim##rage", &anti_aim.enabled.value );
				auto pitch = static_cast< int >( anti_aim.pitch.value );
				if ( ImGui::Combo( "Pitch##rage", &pitch, pitch_modes, IM_ARRAYSIZE( pitch_modes ) ) )
				{
					anti_aim.pitch.value = static_cast< settings::combat::antiaim::pitch_mode >( pitch );
				}
				ImGui::Checkbox( "Compensate roll##rage", &anti_aim.auto_yaw_adjust.value );
				ImGui::Checkbox( "Force left##rage", &anti_aim.manual_left.value );
				ImGui::Checkbox( "Force right##rage", &anti_aim.manual_right.value );
				ImGui::Checkbox( "Hide onshot##rage", &anti_aim.hide_shots.value );
				ImGui::Checkbox( "Avoid backstab##rage", &anti_aim.avoid_backstab.value );
				ImGui::Checkbox( "Direction indicator##rage", &anti_aim.direction_indicator.value );
				draw_config_color( "Direction color##rage", anti_aim.direction_indicator_color );
				ImGui::Checkbox( "Direction glow##rage", &anti_aim.direction_indicator_glow.value );
				ImGui::SliderFloat( "Direction glow strength##rage", &anti_aim.direction_indicator_glow_strength.value, 0.1f, 1.0f, "%.2f" );

				ImGui::Separator( );
				ImGui::Text( "Peek assistance" );
				ImGui::Checkbox( "Quick peek##rage", &quick_peek.enabled.value );
				draw_config_color( "Quick peek color##rage", quick_peek.color );
				draw_config_color( "Retracting color##rage", quick_peek.retrack_color );
				ImGui::Checkbox( "Duck peek##rage", &duck_peek.enabled.value );
			}
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
