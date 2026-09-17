#include <pch/pch.hpp>
#include <core/features/features.hpp>
#include <core/settings.hpp>
#include <external/imgui/imgui.h>
#include <core/rendering/rendering.hpp>
#include "../../imgui_menu.hpp"

namespace rendering {

void imgui_menu::draw_visuals_tab( )
{
	static constexpr const char* visual_sections[ 9 ]{ "Enemy", "Team", "Local", "Viewmodel", "Items", "Projectiles", "Other", "Scene", "Weather" };
		auto& esp = settings::g_esp;
			auto& player = esp.m_player;

		const auto world_tab = this->m_tab == 1;
		const auto section_count = world_tab ? 2 : 7;
		const auto section_width = std::max( 120.0f, ( ImGui::GetContentRegionAvail( ).x - 12.0f ) / 3.0f );
		for ( auto i = 0; i < section_count; ++i )
		{
			if ( i > 0 && i % 3 != 0 )
			{
				ImGui::SameLine( );
			}

			const auto section = world_tab ? i + 7 : i;
			if ( ImGui::Selectable( visual_sections[ section ], this->m_visual_section == section, 0, ImVec2{ section_width, 28.0f } ) )
			{
				this->m_visual_section = section;
			}
			}
			ImGui::Separator( );

		auto draw_layer = [ this ]( const char* label, settings::esp::chams_layer& layer )
			{
				this->draw_function( label, layer.enabled );
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
					this->draw_color( "Color", layer.color );
					ImGui::TreePop( );
				}
			};

		auto draw_chams = [ &draw_layer, this ]( const char* label, settings::esp::chams_config& chams, bool overlay )
			{
				this->draw_function( label, chams.enabled );
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
				this->draw_function( "Enable ESP", overlay.enabled );
				this->draw_function( "Box", overlay.m_box.enabled, true );
				this->draw_function( "Skeleton", overlay.m_skeleton.enabled );
				this->draw_function( "Name", overlay.m_name.enabled );
				this->draw_function( "Weapon", overlay.m_weapon.enabled );
				this->draw_function( "Info flags", overlay.m_info_flags.enabled );
				this->draw_function( "Out-of-view arrows", overlay.m_oof_arrow.enabled );

				if ( overlay.m_box.enabled.value && ImGui::TreeNode( "Box settings" ) )
				{
					static constexpr const char* box_styles[ ]{ "Full", "Cornered" };
					auto style = static_cast< int >( overlay.m_box.style.value );
					ImGui::Combo( "Style", &style, box_styles, IM_ARRAYSIZE( box_styles ) );
					overlay.m_box.style.value = static_cast< decltype( overlay.m_box.style.value ) >( style );
					ImGui::Checkbox( "Fill", &overlay.m_box.fill.value );
					ImGui::Checkbox( "Outline", &overlay.m_box.outline.value );
					ImGui::SliderFloat( "Corner length", &overlay.m_box.corner_length.value, 2.0f, 20.0f, "%.0f" );
					this->draw_color( "Visible##box", overlay.m_box.visible_color );
					this->draw_color( "Occluded##box", overlay.m_box.occluded_color );
					ImGui::TreePop( );
				}

				if ( overlay.m_skeleton.enabled.value && ImGui::TreeNode( "Skeleton settings" ) )
				{
					static constexpr const char* skeleton_modes[ ]{ "Normal", "Backtrack" };
					auto mode = static_cast< int >( overlay.m_skeleton.type.value );
					ImGui::Combo( "Mode", &mode, skeleton_modes, IM_ARRAYSIZE( skeleton_modes ) );
					overlay.m_skeleton.type.value = static_cast< decltype( overlay.m_skeleton.type.value ) >( mode );
					ImGui::SliderFloat( "Thickness", &overlay.m_skeleton.thickness.value, 0.5f, 4.0f, "%.1f" );
					this->draw_color( "Visible##skeleton", overlay.m_skeleton.visible_color );
					this->draw_color( "Occluded##skeleton", overlay.m_skeleton.occluded_color );
					ImGui::TreePop( );
				}

				if ( overlay.m_weapon.enabled.value && ImGui::TreeNode( "Weapon settings" ) )
				{
					static constexpr const char* weapon_display[ ]{ "Text", "Icon", "Text + icon" };
					auto display = static_cast< int >( overlay.m_weapon.display.value );
					ImGui::Combo( "Display", &display, weapon_display, IM_ARRAYSIZE( weapon_display ) );
					overlay.m_weapon.display.value = static_cast< decltype( overlay.m_weapon.display.value ) >( display );
					this->draw_color( "Text color##weapon", overlay.m_weapon.text_color );
					this->draw_color( "Icon color##weapon", overlay.m_weapon.icon_color );
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
					this->draw_color( "Visible##oof", overlay.m_oof_arrow.visible_color );
					this->draw_color( "Occluded##oof", overlay.m_oof_arrow.occluded_color );
					ImGui::TreePop( );
				}
				ImGui::EndGroup( );

				ImGui::SameLine( );
				ImGui::BeginGroup( );
				ImGui::Text( "Chams and glow" );
				draw_chams( "Chams", chams, true );
				draw_chams( "Ragdoll chams", chams_ragdoll, false );
				this->draw_function( "Glow", glow.enabled );
				if ( glow.enabled.value )
				{
					this->draw_color( "Glow color", glow.color );
				}
				this->draw_function( "Ragdoll glow", glow_ragdoll.enabled );
				if ( glow_ragdoll.enabled.value )
				{
					this->draw_color( "Ragdoll glow color", glow_ragdoll.color );
				}
				ImGui::EndGroup( );
			}
			else if ( this->m_visual_section == 2 )
			{
				ImGui::Text( "Local player" );
				draw_chams( "Chams", player.m_chams.local, true );
				this->draw_function( "Lower opacity", esp.m_local_alpha.enabled );
				if ( esp.m_local_alpha.enabled.value )
				{
					ImGui::SliderFloat( "Opacity", &esp.m_local_alpha.opacity.value, 0.0f, 1.0f, "%.2f" );
					ImGui::Checkbox( "Only when scoped", &esp.m_local_alpha.only_scoped.value );
				}
				draw_chams( "Ragdoll chams", player.m_chams.local_ragdoll, false );
				this->draw_function( "Glow", player.m_glow.local.enabled );
				if ( player.m_glow.local.enabled.value )
				{
					this->draw_color( "Glow color", player.m_glow.local.color );
				}
				this->draw_function( "Ragdoll glow", player.m_glow.local_ragdoll.enabled );
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
					this->draw_color( "Text color##item", group.text_color );
					this->draw_color( "Icon color##item", group.icon_color );
					ImGui::TreePop( );
				}
				ImGui::Checkbox( "Item chams", &item.m_chams.group_toggle( item_group ).value );
				draw_chams( "Item chams layers", item.m_chams.get_group( item_group ), false );
				ImGui::Checkbox( "Item glow", &item.m_glow.group_toggle( item_group ).value );
				if ( item.m_glow.group_toggle( item_group ).value )
				{
					this->draw_color( "Item glow color", item.m_glow.groups[ item_group ].color );
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
					this->draw_color( "Tracer color", impacts.bullet_tracer_color );
					ImGui::SliderFloat( "Tracer duration", &impacts.bullet_tracer_duration.value, 0.1f, 5.0f, "%.1f s" );
				}
				this->draw_function( "Projectile ESP", projectile.m_overlay.enabled );
				ImGui::Combo( "Group##projectile", &projectile_group, projectile_groups, IM_ARRAYSIZE( projectile_groups ) );
				ImGui::Checkbox( "Enabled##projectile", &projectile.m_overlay.group_toggle( projectile_group ).value );
				if ( projectile_group == 5 )
				{
					auto& inferno = projectile.m_overlay.m_infernos;
					this->draw_color( "Fill color##inferno", inferno.fill_color );
					this->draw_color( "Outline color##inferno", inferno.outline_color );
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
					this->draw_color( "Text color##projectile", group.text_color );
					this->draw_color( "Icon color##projectile", group.icon_color );
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
				this->draw_color( "Sky color", scene.skybox.skybox_color );
				this->draw_color( "Cloud color", scene.skybox.cloud_color );
				this->draw_color( "Sun color", scene.skybox.sun_color );
				ImGui::Checkbox( "World color", &scene.world_setting.value );
				this->draw_color( "World color value", scene.world_color );
				ImGui::Checkbox( "Lighting", &scene.lighting.value );
				ImGui::SliderFloat( "Lighting intensity", &scene.lighting_intensity.value, 0.0f, 2.0f, "%.2f" );
				this->draw_color( "Lighting color", scene.lighting_color );
				ImGui::Checkbox( "Ambient", &scene.ambient.value );
				ImGui::SliderFloat( "Ambient intensity", &scene.ambient_intensity.value, 0.0f, 3.0f, "%.2f" );
				this->draw_color( "Ambient color", scene.ambient_color );
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
				this->draw_function( "Weather", weather.enabled );
				static constexpr const char* weather_types[ 3 ]{ "Snow", "Rain", "Stars" };
				auto weather_type = static_cast< int >( weather.type.value );
				if ( ImGui::Combo( "Type##weather", &weather_type, weather_types, IM_ARRAYSIZE( weather_types ) ) )
				{
					weather.type.value = static_cast< settings::world::weather::weather_type >( weather_type );
				}
				this->draw_color( "Weather color", weather.color );
				ImGui::Checkbox( "Fog", &weather.fog_enabled.value );
				ImGui::SliderFloat( "Fog density", &weather.fog_density.value, 0.0f, 1.0f, "%.2f" );
				ImGui::SliderFloat( "Fog anisotropy", &weather.fog_anisotropy.value, 0.0f, 1.0f, "%.2f" );
				ImGui::SliderFloat( "Fog draw distance", &weather.fog_draw_distance.value, 500.0f, 20000.0f, "%.0f" );
				this->draw_color( "Fog color", weather.fog_color );
				ImGui::Checkbox( "Wetness", &weather.wetness.value );
				ImGui::SliderFloat( "Wetness density", &weather.wetness_density.value, 0.0f, 5.0f, "%.1f" );
				ImGui::SliderFloat( "Wetness speed", &weather.wetness_speed.value, 0.0f, 3.0f, "%.1f" );
				ImGui::Checkbox( "Wind", &weather.wind.value );
				ImGui::SliderFloat( "Wind strength", &weather.wind_strength.value, 0.0f, 5.0f, "%.1f" );
				ImGui::SliderFloat( "Wind direction", &weather.wind_direction.value, 0.0f, 360.0f, "%.0f" );
				ImGui::SliderFloat( "Wind turbulence", &weather.wind_turbulence.value, 0.0f, 5.0f, "%.1f" );
			}
}

} // namespace rendering
