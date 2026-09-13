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
		ImGui::Begin( "TRIADA.BENZ", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings );
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
		static constexpr const char* visual_sections[ 9 ]{ "Enemy", "Team", "Local", "Viewmodel", "Items", "Projectiles", "Other", "Scene", "Weather" };
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

			for ( auto i = 0; i < 9; ++i )
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
			else if ( this->m_visual_section == 3 )
			{
				ImGui::Text( "Viewmodel" );
				draw_chams( "Weapon chams", esp.m_viewmodel.weapon, true );
				draw_chams( "Arms chams", esp.m_viewmodel.arms, true );
			}
			else if ( this->m_visual_section == 4 )
			{
				auto& item = esp.m_item;
				static int item_group{};
				static constexpr const char* item_groups[ 6 ]{ "Pistol", "SMG", "Rifle", "Shotgun", "Sniper", "Utility" };
				ImGui::Text( "World items" );
				ImGui::Combo( "Group##item", &item_group, item_groups, IM_ARRAYSIZE( item_groups ) );
				ImGui::Checkbox( "Item ESP", &item.m_overlay.group_toggle( item_group ).value );
				if ( ImGui::TreeNode( "Item ESP settings" ) )
				{
					auto& group = item.m_overlay.get_group( item_group );
					static constexpr const char* display_types[ 3 ]{ "Text", "Icon", "Text + icon" };
					auto display = static_cast< int >( group.display.value );
					ImGui::Combo( "Display##item", &display, display_types, IM_ARRAYSIZE( display_types ) );
					group.display.value = static_cast< decltype( group.display.value ) >( display );
					ImGui::SliderFloat( "Max distance##item", &group.max_distance.value, 1.0f, 200.0f, "%.0f m" );
					draw_color( "Text color##item", group.text_color );
					draw_color( "Icon color##item", group.icon_color );
					ImGui::TreePop( );
				}
				ImGui::Checkbox( "Item chams", &item.m_chams.group_toggle( item_group ).value );
				draw_chams( "Item chams layers", item.m_chams.get_group( item_group ), false );
				ImGui::Checkbox( "Item glow", &item.m_glow.group_toggle( item_group ).value );
				if ( item.m_glow.group_toggle( item_group ).value )
				{
					draw_color( "Item glow color", item.m_glow.groups[ item_group ].color );
				}
			}
			else if ( this->m_visual_section == 5 )
			{
				auto& projectile = esp.m_projectile;
				auto& impacts = settings::g_misc.m_impacts;
				static int projectile_group{};
				static constexpr const char* projectile_groups[ 6 ]{ "HE grenade", "Flashbang", "Smoke", "Molotov", "Decoy", "Inferno" };
				ImGui::Text( "Projectiles" );
				ImGui::Checkbox( "Bullet tracers", &impacts.bullet_tracers.value );
				if ( impacts.bullet_tracers.value )
				{
					draw_color( "Tracer color", impacts.bullet_tracer_color );
					ImGui::SliderFloat( "Tracer duration", &impacts.bullet_tracer_duration.value, 0.1f, 5.0f, "%.1f s" );
				}
				ImGui::Checkbox( "Projectile ESP", &projectile.m_overlay.enabled.value );
				ImGui::Combo( "Group##projectile", &projectile_group, projectile_groups, IM_ARRAYSIZE( projectile_groups ) );
				ImGui::Checkbox( "Enabled##projectile", &projectile.m_overlay.group_toggle( projectile_group ).value );
				if ( projectile_group == 5 )
				{
					auto& inferno = projectile.m_overlay.m_infernos;
					draw_color( "Fill color##inferno", inferno.fill_color );
					draw_color( "Outline color##inferno", inferno.outline_color );
					ImGui::SliderFloat( "Outline thickness##inferno", &inferno.outline_thickness.value, 0.5f, 5.0f, "%.1f" );
					ImGui::Checkbox( "Glow##inferno", &inferno.glow.value );
					ImGui::SliderFloat( "Glow strength##inferno", &inferno.glow_strength.value, 0.1f, 1.0f, "%.2f" );
				}
				else
				{
					auto& group = projectile.m_overlay.get_group( projectile_group );
					static constexpr const char* display_types[ 3 ]{ "Text", "Icon", "Text + icon" };
					auto display = static_cast< int >( group.display.value );
					ImGui::Combo( "Display##projectile", &display, display_types, IM_ARRAYSIZE( display_types ) );
					group.display.value = static_cast< decltype( group.display.value ) >( display );
					ImGui::SliderFloat( "Max distance##projectile", &group.max_distance.value, 1.0f, 200.0f, "%.0f m" );
					draw_color( "Text color##projectile", group.text_color );
					draw_color( "Icon color##projectile", group.icon_color );
				}
			}
			else if ( this->m_visual_section == 6 )
			{
				auto& other = esp.m_other;
				ImGui::Text( "Other ESP" );
				ImGui::Checkbox( "Bomb timer", &other.bomb_timer.value );
				ImGui::Checkbox( "Spectator list", &other.spectator_list.value );
			}
			if ( this->m_visual_section == 7 )
			{
				auto& scene = settings::g_world.m_scene;
				ImGui::Text( "Scene" );
				ImGui::Checkbox( "Skybox material", &scene.skybox.custom_skybox.value );
				const auto& skyboxes = features::world::g_scene.get_skyboxes( );
				if ( !skyboxes.empty( ) )
				{
					std::vector< const char* > skybox_names;
					skybox_names.reserve( skyboxes.size( ) );
					for ( const auto& skybox : skyboxes )
					{
						skybox_names.push_back( skybox.display_name.c_str( ) );
					}
					scene.skybox.selected_skybox.value = std::clamp( scene.skybox.selected_skybox.value, 0, static_cast< int >( skybox_names.size( ) ) - 1 );
					ImGui::Combo( "Skybox", &scene.skybox.selected_skybox.value, skybox_names.data( ), static_cast< int >( skybox_names.size( ) ) );
				}
				ImGui::Checkbox( "Skybox color", &scene.skybox.custom_color.value );
				draw_color( "Sky color", scene.skybox.skybox_color );
				draw_color( "Cloud color", scene.skybox.cloud_color );
				draw_color( "Sun color", scene.skybox.sun_color );
				ImGui::Checkbox( "World color", &scene.world_setting.value );
				draw_color( "World color value", scene.world_color );
				ImGui::Checkbox( "Lighting", &scene.lighting.value );
				ImGui::SliderFloat( "Lighting intensity", &scene.lighting_intensity.value, 0.0f, 2.0f, "%.2f" );
				draw_color( "Lighting color", scene.lighting_color );
				ImGui::Checkbox( "Ambient", &scene.ambient.value );
				ImGui::SliderFloat( "Ambient intensity", &scene.ambient_intensity.value, 0.0f, 3.0f, "%.2f" );
				draw_color( "Ambient color", scene.ambient_color );
				ImGui::Checkbox( "Bloom", &scene.bloom.value );
				ImGui::SliderFloat( "Bloom value", &scene.bloom_value.value, 0.0f, 2.0f, "%.2f" );
				ImGui::Checkbox( "Gamma", &scene.gamma.value );
				ImGui::SliderFloat( "Gamma value", &scene.gamma_value.value, 0.5f, 5.0f, "%.1f" );
				ImGui::Checkbox( "Depth of field", &scene.dof.value );
				ImGui::SliderFloat( "Near blurry", &scene.dof_near_blurry.value, 0.0f, 50.0f, "%.0f" );
				ImGui::SliderFloat( "Near crisp", &scene.dof_near_crisp.value, 0.0f, 100.0f, "%.0f" );
				ImGui::SliderFloat( "Far crisp", &scene.dof_far_crisp.value, 100.0f, 2000.0f, "%.0f" );
				ImGui::SliderFloat( "Far blurry", &scene.dof_far_blurry.value, 200.0f, 5000.0f, "%.0f" );
			}
			else if ( this->m_visual_section == 8 )
			{
				auto& weather = settings::g_world.m_weather;
				ImGui::Text( "Weather" );
				ImGui::Checkbox( "Weather", &weather.enabled.value );
				static constexpr const char* weather_types[ 3 ]{ "Snow", "Rain", "Stars" };
				auto weather_type = static_cast< int >( weather.type.value );
				if ( ImGui::Combo( "Type##weather", &weather_type, weather_types, IM_ARRAYSIZE( weather_types ) ) )
				{
					weather.type.value = static_cast< settings::world::weather::weather_type >( weather_type );
				}
				draw_color( "Weather color", weather.color );
				ImGui::Checkbox( "Fog", &weather.fog_enabled.value );
				ImGui::SliderFloat( "Fog density", &weather.fog_density.value, 0.0f, 1.0f, "%.2f" );
				ImGui::SliderFloat( "Fog anisotropy", &weather.fog_anisotropy.value, 0.0f, 1.0f, "%.2f" );
				ImGui::SliderFloat( "Fog draw distance", &weather.fog_draw_distance.value, 500.0f, 20000.0f, "%.0f" );
				draw_color( "Fog color", weather.fog_color );
				ImGui::Checkbox( "Wetness", &weather.wetness.value );
				ImGui::SliderFloat( "Wetness density", &weather.wetness_density.value, 0.0f, 5.0f, "%.1f" );
				ImGui::SliderFloat( "Wetness speed", &weather.wetness_speed.value, 0.0f, 3.0f, "%.1f" );
				ImGui::Checkbox( "Wind", &weather.wind.value );
				ImGui::SliderFloat( "Wind strength", &weather.wind_strength.value, 0.0f, 5.0f, "%.1f" );
				ImGui::SliderFloat( "Wind direction", &weather.wind_direction.value, 0.0f, 360.0f, "%.0f" );
				ImGui::SliderFloat( "Wind turbulence", &weather.wind_turbulence.value, 0.0f, 5.0f, "%.1f" );
			}
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
					if ( impacts.hit_sound_type.value == settings::misc::impacts::sound_type::custom )
					{
						static char hit_sound_file[ 128 ]{};
						static std::string hit_sound_value{};
						if ( hit_sound_value != impacts.custom_hit_sound.value )
						{
							std::snprintf( hit_sound_file, sizeof( hit_sound_file ), "%s", impacts.custom_hit_sound.value.c_str( ) );
							hit_sound_value = impacts.custom_hit_sound.value;
						}
						if ( ImGui::InputText( "Custom hit sound", hit_sound_file, sizeof( hit_sound_file ) ) )
						{
							impacts.custom_hit_sound.value = hit_sound_file;
						}
						if ( ImGui::Button( "Preview hit sound" ) )
						{
							features::misc::g_impacts.play_custom_sound( impacts.custom_hit_sound.value, impacts.hit_sound_volume.value );
						}
					}
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
					if ( impacts.death_sound_type.value == settings::misc::impacts::sound_type::custom )
					{
						static char death_sound_file[ 128 ]{};
						static std::string death_sound_value{};
						if ( death_sound_value != impacts.custom_death_sound.value )
						{
							std::snprintf( death_sound_file, sizeof( death_sound_file ), "%s", impacts.custom_death_sound.value.c_str( ) );
							death_sound_value = impacts.custom_death_sound.value;
						}
						if ( ImGui::InputText( "Custom death sound", death_sound_file, sizeof( death_sound_file ) ) )
						{
							impacts.custom_death_sound.value = death_sound_file;
						}
						if ( ImGui::Button( "Preview death sound" ) )
						{
							features::misc::g_impacts.play_custom_sound( impacts.custom_death_sound.value, impacts.death_sound_volume.value );
						}
					}
					ImGui::Checkbox( "Death effect", &impacts.death_effect.value );
					draw_config_color( "Death effect color", impacts.death_effect_color );
					ImGui::Checkbox( "Bullet impacts", &impacts.bullet_impact_effect.value );
					auto impact_type = static_cast< int >( impacts.bullet_impact_effect_type.value );
					static constexpr const char* impact_types[ 3 ]{ "Overlay", "Sparks", "Both" };
					if ( ImGui::Combo( "Impact type", &impact_type, impact_types, IM_ARRAYSIZE( impact_types ) ) )
					{
						impacts.bullet_impact_effect_type.value = static_cast< settings::misc::impacts::bullet_impact_type >( impact_type );
					}
					draw_config_color( "Impact fill", impacts.bullet_impact_effect_fill_color );
					draw_config_color( "Impact edge", impacts.bullet_impact_effect_edge_color );
					draw_config_color( "Spark color", impacts.bullet_impact_effect_color_spark );
					ImGui::SliderFloat( "Impact duration", &impacts.bullet_impact_effect_duration.value, 0.1f, 5.0f, "%.1f s" );
					ImGui::Checkbox( "Impact glow", &impacts.bullet_impact_effect_glow.value );
					ImGui::SliderFloat( "Impact glow strength", &impacts.bullet_impact_effect_glow_strength.value, 0.1f, 1.0f, "%.2f" );
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
					auto& autobuy = misc.m_autobuy;
					static constexpr const char* primary_weapons[ 6 ]{ "None", "Rifle", "Scoped rifle", "Scout", "AWP", "Auto sniper" };
					static constexpr const char* secondary_weapons[ 5 ]{ "None", "Dual elites", "Five-seven / Tec-9", "Deagle", "Revolver" };
					static constexpr const char* grenades[ 5 ]{ "Molotov", "HE grenade", "Smoke", "Flashbang", "Decoy" };
					ImGui::Checkbox( "Auto buy", &autobuy.enabled.value );
					ImGui::Combo( "Primary weapon", &autobuy.primary_weapon.value, primary_weapons, IM_ARRAYSIZE( primary_weapons ) );
					ImGui::Combo( "Secondary weapon", &autobuy.secondary_weapon.value, secondary_weapons, IM_ARRAYSIZE( secondary_weapons ) );
					ImGui::Checkbox( "Armor", &autobuy.armor.value );
					ImGui::Checkbox( "Defuser", &autobuy.defuser.value );
					ImGui::Checkbox( "Taser", &autobuy.taser.value );
					for ( auto i = 0; i < 5; ++i )
					{
						ImGui::Checkbox( grenades[ i ], &autobuy.grenades.values[ i ] );
					}
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
		else if ( this->m_tab == 4 )
		{
			auto& changer = settings::g_changer;
			auto& econ = features::changer::g_econ_item_system;
			static constexpr const char* categories[ 4 ]{ "Weapons", "Knives", "Gloves", "Agents" };
			static int category{};
			static int selected_item{};
			static int selected_paint_id{};
			static int last_category{ -1 };
			static bool weapon_selection_dirty{};
			static char skin_search[ 128 ]{};

			auto display_name = [ ]( const std::string& localized, const std::string& fallback )
			{
				if ( !localized.empty( ) && localized != "???" )
				{
					return localized;
				}

				return fallback;
			};

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
					selected_paint_id = 0;
					weapon_selection_dirty = false;
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
				if ( category != last_category )
				{
					last_category = category;
					weapon_selection_dirty = false;
				}

				if ( !weapon_selection_dirty && ( category == 1 || category == 2 ) )
				{
					for ( auto i = 0; i < static_cast< int >( items.size( ) ); ++i )
					{
						if ( changer.skins.data.contains( items[ i ]->def_index ) )
						{
							selected_item = i;
							break;
						}
					}
				}

				selected_item = std::clamp( selected_item, 0, static_cast< int >( items.size( ) ) - 1 );
				const auto* item = items[ selected_item ];
				const auto weapon_name = display_name( item->localized_name, item->name );

				ImGui::BeginChild( "##skin_preview", ImVec2{ 280.0f, 0.0f }, true );
				const auto applied_it = changer.skins.data.find( item->def_index );
				const auto applied_paint = applied_it != changer.skins.data.end( ) ? applied_it->second.paint_kit_id : 0;
				if ( !weapon_selection_dirty && applied_paint != 0 )
				{
					selected_paint_id = applied_paint;
				}
				const auto selected_paint = selected_paint_id != 0 ? econ.find_paint_kit( selected_paint_id ) : econ.find_paint_kit( applied_paint );
				const auto skin_name = selected_paint ? display_name( selected_paint->localized_name, selected_paint->name ) : std::string{ "Default" };
				ImGui::TextWrapped( "%s | %s", weapon_name.c_str( ), skin_name.c_str( ) );
				ImGui::Separator( );

				const auto preview = econ.get_skin_image( item->def_index, selected_paint ? selected_paint->id : 0 );
				if ( preview && preview->width > 0 && preview->height > 0 )
				{
					const auto max_size = ImVec2{ 240.0f, 190.0f };
					const auto aspect = static_cast< float >( preview->width ) / static_cast< float >( preview->height );
					auto preview_size = ImVec2{ max_size.x, max_size.x / aspect };
					if ( preview_size.y > max_size.y )
					{
						preview_size = ImVec2{ max_size.y * aspect, max_size.y };
					}

					ImGui::SetCursorPosX( ( ImGui::GetWindowWidth( ) - preview_size.x ) * 0.5f );
					ImGui::Image( reinterpret_cast< ImTextureID >( preview->srv.Get( ) ), preview_size );
				}
				else
				{
					ImGui::TextDisabled( "Loading preview..." );
				}

				ImGui::Separator( );
				ImGui::Text( "Weapon" );
				ImGui::SetNextItemWidth( -1.0f );
				std::vector< const char* > item_names;
				item_names.reserve( items.size( ) );
				std::vector< std::string > item_name_storage;
				item_name_storage.reserve( items.size( ) );
				for ( const auto* entry : items )
				{
					item_name_storage.push_back( display_name( entry->localized_name, entry->name ) );
					item_names.push_back( item_name_storage.back( ).c_str( ) );
				}
				if ( ImGui::Combo( "##selected_weapon", &selected_item, item_names.data( ), static_cast< int >( item_names.size( ) ) ) )
				{
					selected_paint_id = 0;
					weapon_selection_dirty = true;
				}
				if ( category == 3 )
				{
					ImGui::Text( "Team: %s", item->team( ) == 3 ? "Counter-Terrorist" : "Terrorist" );
					if ( item->team( ) == 3 )
					{
						changer.agents.ct_def = item->def_index;
					}
					else if ( item->team( ) == 2 )
					{
						changer.agents.t_def = item->def_index;
					}
				}
				ImGui::EndChild( );

				ImGui::SameLine( );
				ImGui::BeginChild( "##skin_list", ImVec2{ 0.0f, 0.0f }, true );
				ImGui::Text( "Choose skin" );
				ImGui::InputText( "Search##skin", skin_search, sizeof( skin_search ) );
				ImGui::Separator( );

				std::vector< const features::changer::econ_item_system::paint_kit* > paint_kits;
				std::string skin_search_lower{ skin_search };
				for ( auto& character : skin_search_lower )
				{
					character = static_cast< char >( std::tolower( static_cast< unsigned char >( character ) ) );
				}
				for ( const auto& skin : econ.skins( ) )
				{
					if ( skin.def_index != item->def_index )
					{
						continue;
					}

					if ( const auto* paint = econ.find_paint_kit( skin.paint_kit_id ) )
					{
						auto paint_name = paint->localized_name.empty( ) ? paint->name : paint->localized_name;
						std::string paint_name_lower{ paint_name };
						for ( auto& character : paint_name_lower )
						{
							character = static_cast< char >( std::tolower( static_cast< unsigned char >( character ) ) );
						}
						if ( !skin_search_lower.empty( ) && paint_name_lower.find( skin_search_lower ) == std::string::npos )
						{
							continue;
						}
						paint_kits.push_back( paint );
					}
				}

				if ( paint_kits.empty( ) )
				{
					ImGui::TextDisabled( "No skins are available for this weapon." );
				}
				else
				{
					bool selected_is_valid = selected_paint_id == 0;
					for ( const auto* paint : paint_kits )
					{
						selected_is_valid = selected_is_valid || paint->id == selected_paint_id;
					}
					if ( !selected_is_valid )
					{
						selected_paint_id = 0;
					}

					for ( const auto* paint : paint_kits )
					{
						const auto paint_name = display_name( paint->localized_name, paint->name );
						if ( ImGui::Selectable( paint_name.c_str( ), selected_paint_id == paint->id ) )
						{
							selected_paint_id = paint->id;
							if ( category == 1 || category == 2 )
							{
								for ( auto it = changer.skins.data.begin( ); it != changer.skins.data.end( ); )
								{
									const auto* existing = econ.find_def( it->first );
									if ( existing && existing->category == ( category == 1 ? features::changer::econ_item_system::item_category::knife : features::changer::econ_item_system::item_category::glove ) )
									{
										it = changer.skins.data.erase( it );
									}
									else
									{
										++it;
									}
								}
							}
							changer.skins.data[ item->def_index ].paint_kit_id = paint->id;
							weapon_selection_dirty = false;
						}
					}

					if ( selected_paint_id != 0 )
					{
						auto& applied = changer.skins.data[ item->def_index ];
						ImGui::Separator( );
						ImGui::SliderFloat( "Wear", &applied.wear, 0.0f, 1.0f, "%.4f" );
						ImGui::SliderInt( "Seed", &applied.seed, 0, 1000 );
						ImGui::Checkbox( "StatTrak", &applied.stattrak );
						if ( ImGui::Button( "Clear selected skin" ) )
						{
							changer.skins.data.erase( item->def_index );
							selected_paint_id = 0;
						}
					}
				}
				ImGui::EndChild( );
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
		else if ( this->m_tab == 5 )
		{
			static std::vector<std::wstring> config_list{};
			static std::string search{};
			static int selected{ -1 };
			static bool refresh{ true };
			static char search_buffer[ 128 ]{};

			auto to_utf8 = [ ]( const std::wstring& value )
			{
				char buffer[ 256 ]{};
				WideCharToMultiByte( CP_UTF8, 0, value.c_str( ), -1, buffer, sizeof( buffer ), nullptr, nullptr );
				return std::string{ buffer };
			};
			auto to_wide = [ ]( const std::string& value )
			{
				wchar_t buffer[ 256 ]{};
				MultiByteToWideChar( CP_UTF8, 0, value.c_str( ), -1, buffer, IM_ARRAYSIZE( buffer ) );
				return std::wstring{ buffer };
			};
			auto copy_clipboard = [ ]( const std::string& value )
			{
				if ( !OpenClipboard( nullptr ) )
				{
					return;
				}
				EmptyClipboard( );
				const auto bytes = ( value.size( ) + 1 ) * sizeof( char );
				if ( const auto memory = GlobalAlloc( GMEM_MOVEABLE, bytes ) )
				{
					if ( const auto destination = GlobalLock( memory ) )
					{
						std::memcpy( destination, value.c_str( ), bytes );
						GlobalUnlock( memory );
						SetClipboardData( CF_TEXT, memory );
					}
					else
					{
						GlobalFree( memory );
					}
				}
				CloseClipboard( );
			};
			auto paste_clipboard = [ ]( )
			{
				std::string result{};
				if ( !OpenClipboard( nullptr ) )
				{
					return result;
				}
				if ( const auto memory = GetClipboardData( CF_TEXT ) )
				{
					if ( const auto source = static_cast< const char* >( GlobalLock( memory ) ) )
					{
						result = source;
						GlobalUnlock( memory );
					}
				}
				CloseClipboard( );
				return result;
			};

			if ( refresh )
			{
				config_list = config::registry::list( );
				selected = std::clamp( selected, -1, static_cast< int >( config_list.size( ) ) - 1 );
				refresh = false;
			}

			ImGui::Text( "Configuration" );
			if ( ImGui::InputText( "Search", search_buffer, sizeof( search_buffer ) ) )
			{
				search = search_buffer;
			}
			ImGui::BeginChild( "##config_list", ImVec2{ 0.0f, -42.0f }, true );
			for ( auto i = 0; i < static_cast< int >( config_list.size( ) ); ++i )
			{
				const auto name = to_utf8( config_list[ i ] );
				if ( !search.empty( ) && name.find( search ) == std::string::npos )
				{
					continue;
				}
				if ( ImGui::Selectable( name.c_str( ), selected == i ) )
				{
					selected = i;
					if ( config::registry::load( config_list[ i ] ) )
					{
						settings::finalize_binds( );
					}
				}
			}
			ImGui::EndChild( );

			const auto has_selection = selected >= 0 && selected < static_cast< int >( config_list.size( ) );
			ImGui::SameLine( );
			if ( ImGui::Button( "Save" ) )
			{
				const auto name = has_selection ? to_utf8( config_list[ selected ] ) : std::string{ search_buffer };
				if ( !name.empty( ) && config::registry::save( to_wide( name ) ) )
				{
					refresh = true;
				}
			}
			ImGui::SameLine( );
			if ( ImGui::Button( "Reset" ) )
			{
				auto& registry = config::detail::get_registry( );
				for ( auto& field : registry.fields )
				{
					char key[ 12 ]{};
					std::snprintf( key, sizeof( key ), "%08x", field.key );
					if ( const auto it = registry.defaults.find( key ); it != registry.defaults.end( ) )
					{
						config::serial::json_to_field( *it, field );
					}
				}
				settings::finalize_binds( );
			}
			ImGui::SameLine( );
			if ( ImGui::Button( "Delete" ) && has_selection )
			{
				if ( config::registry::remove( config_list[ selected ] ) )
				{
					selected = -1;
					refresh = true;
				}
			}
			ImGui::SameLine( );
			if ( ImGui::Button( "Import" ) )
			{
				const auto result = config::import_auto( paste_clipboard( ) );
				if ( result.success )
				{
					settings::finalize_binds( );
					refresh = true;
				}
			}
			ImGui::SameLine( );
			if ( ImGui::Button( "Export" ) && has_selection )
			{
				copy_clipboard( config::export_share_words( to_utf8( config_list[ selected ] ) ) );
			}
		}
		else
		{
			ImGui::Text( "Info" );
			ImGui::TextDisabled( "Select a tab from the navigation rail." );
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
