#include <pch/pch.hpp>
#include <core/features/features.hpp>
#include <core/settings.hpp>
#include <external/imgui/imgui.h>
#include <core/rendering/rendering.hpp>
#include "../../imgui_menu.hpp"

namespace rendering {

void imgui_menu::draw_misc_tab( )
{

				auto& misc = settings::g_misc;
				auto& movement = settings::g_movement;
				static constexpr const char* sections[ 4 ]{ "General", "Removals", "Camera", "HUD" };
				static constexpr const char* sound_types[ 10 ]{ "Shop click", "Home click", "Bell", "Killcard", "Bullet casing", "Coin pickup", "Item drop", "Popcan", "Key press", "Custom" };
				static constexpr const char* marker_types[ 3 ]{ "Classic", "Damage", "Both" };
				static constexpr const char* impact_types[ 3 ]{ "Overlay", "Sparks", "Both" };

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
					this->draw_color( "Marker color", impacts.hit_marker_color );
					ImGui::Checkbox( "Hit effect", &impacts.hit_effect.value );
					this->draw_color( "Hit effect color", impacts.hit_effect_color );
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
					static constexpr const char* death_effect_types[ 2 ]{ "Particle (fade)", "ASCII burst" };
					auto death_effect_type = static_cast< int >( impacts.death_effect_type.value );
					if ( ImGui::Combo( "Death effect type", &death_effect_type, death_effect_types, IM_ARRAYSIZE( death_effect_types ) ) )
					{
						impacts.death_effect_type.value = static_cast< settings::misc::impacts::death_effect_mode >( death_effect_type );
					}
					this->draw_color( "Death effect color", impacts.death_effect_color );
					ImGui::Checkbox( "Bullet impacts", &impacts.bullet_impact_effect.value );
					auto impact_type = static_cast< int >( impacts.bullet_impact_effect_type.value );
					static constexpr const char* impact_types[ 3 ]{ "Overlay", "Sparks", "Both" };
					if ( ImGui::Combo( "Impact type", &impact_type, impact_types, IM_ARRAYSIZE( impact_types ) ) )
					{
						impacts.bullet_impact_effect_type.value = static_cast< settings::misc::impacts::bullet_impact_type >( impact_type );
					}
					this->draw_color( "Impact fill", impacts.bullet_impact_effect_fill_color );
					this->draw_color( "Impact edge", impacts.bullet_impact_effect_edge_color );
					this->draw_color( "Spark color", impacts.bullet_impact_effect_color_spark );
					ImGui::SliderFloat( "Impact duration", &impacts.bullet_impact_effect_duration.value, 0.1f, 5.0f, "%.1f s" );
					ImGui::Checkbox( "Impact glow", &impacts.bullet_impact_effect_glow.value );
					ImGui::SliderFloat( "Impact glow strength", &impacts.bullet_impact_effect_glow_strength.value, 0.1f, 1.0f, "%.2f" );
					ImGui::EndGroup( );

					ImGui::SameLine( );
					ImGui::BeginGroup( );
					ImGui::Text( "World and movement" );
					this->draw_function( "Projectile trajectory", trajectory.enabled );
					ImGui::Checkbox( "Straight throw", &trajectory.straight_throw.value );
					ImGui::Checkbox( "Trajectory glow", &trajectory.glow.value );
					ImGui::SliderFloat( "Trajectory glow strength", &trajectory.glow_strength.value, 0.1f, 1.0f, "%.2f" );
					this->draw_color( "Held color", trajectory.held_color );
					this->draw_color( "Thrown color", trajectory.thrown_color );
					this->draw_color( "Damage held color", trajectory.will_deal_damage_held_color );
					this->draw_color( "Damage thrown color", trajectory.will_deal_damage_thrown_color );
					this->draw_function( "Dynamic light", dlight.enabled );
					this->draw_color( "Dynamic light color", dlight.color );
					ImGui::SliderFloat( "Light radius", &dlight.radius.value, 50.0f, 15000.0f, "%.0f" );
					ImGui::SliderFloat( "Light Z offset", &dlight.z_offset.value, 0.0f, 100.0f, "%.0f" );
					this->draw_function( "Penetration crosshair", penetration.enabled );
					ImGui::Checkbox( "Penetration glow", &penetration.glow.value );
					ImGui::SliderFloat( "Penetration glow strength", &penetration.glow_strength.value, 0.1f, 1.0f, "%.2f" );
					this->draw_color( "Can penetrate", penetration.can_penetrate_fill );
					this->draw_color( "Blocked", penetration.blocked_fill );
					ImGui::Checkbox( "Bunnyhop", &movement.bhop.value );
					this->draw_function( "Autostrafe", movement.m_test_strafer.enabled );
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
					this->draw_function( "Auto buy", autobuy.enabled );
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
					this->draw_function( "Viewmodel adjust", viewmodel.enabled );
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
					this->draw_function( "Crosshair overlay", hud.m_crosshair.enabled );
					ImGui::SliderFloat( "Crosshair size", &hud.m_crosshair.size.value, 0.5f, 10.0f, "%.1f" );
					ImGui::SliderFloat( "Crosshair outline", &hud.m_crosshair.outline.value, 0.0f, 4.0f, "%.1f" );
					this->draw_color( "Crosshair color", hud.m_crosshair.color );
					this->draw_color( "Crosshair outline color", hud.m_crosshair.outline_color );
					this->draw_function( "Scope overlay", hud.m_scope.enabled );
					ImGui::SliderFloat( "Scope line length", &hud.m_scope.line_length.value, 10.0f, 500.0f, "%.0f" );
					ImGui::SliderFloat( "Scope gap", &hud.m_scope.gap.value, 0.0f, 50.0f, "%.0f" );
					ImGui::SliderFloat( "Scope thickness", &hud.m_scope.thickness.value, 0.5f, 5.0f, "%.2f" );
					ImGui::SliderFloat( "Scope animation speed", &hud.m_scope.anim_speed.value, 1.0f, 30.0f, "%.0f" );
					this->draw_color( "Scope color", hud.m_scope.color );
					ImGui::Checkbox( "Scope fade in", &hud.m_scope.fade_in.value );
					ImGui::Checkbox( "Scope glow", &hud.m_scope.glow.value );
					ImGui::SliderFloat( "Scope glow strength", &hud.m_scope.glow_strength.value, 0.1f, 1.0f, "%.2f" );
					ImGui::EndGroup( );
					ImGui::SameLine( );
					ImGui::BeginGroup( );
					ImGui::Text( "Velocity HUD" );
					ImGui::Checkbox( "Velocity counter", &hud.m_velocity.counter.value );
					ImGui::Checkbox( "Velocity chart", &hud.m_velocity.chart.value );
					this->draw_color( "Velocity color", hud.m_velocity.color );
					ImGui::SliderFloat( "Velocity bottom offset", &hud.m_velocity.bottom_offset.value, 0.0f, 300.0f, "%.0f" );
					ImGui::SliderFloat( "Chart width", &hud.m_velocity.chart_width.value, 50.0f, 500.0f, "%.0f" );
					ImGui::SliderFloat( "Chart height", &hud.m_velocity.chart_height.value, 20.0f, 150.0f, "%.0f" );
					ImGui::Separator( );
					ImGui::Text( "Custom HUD" );
					this->draw_function( "Custom HUD enabled", hud.m_local_status.enabled );
					if ( hud.m_local_status.enabled.value )
					{
						ImGui::Checkbox( "Local health", &hud.m_local_status.health.value );
						ImGui::Checkbox( "Local ammo", &hud.m_local_status.ammo.value );
						this->draw_color( "Health color##local", hud.m_local_status.health_color );
						this->draw_color( "Ammo color##local", hud.m_local_status.ammo_color );
						ImGui::SliderFloat( "Custom HUD bottom offset", &hud.m_local_status.bottom_offset.value, 20.0f, 220.0f, "%.0f" );
					}
					ImGui::Separator( );
					ImGui::Text( "General" );
					ImGui::Checkbox( "Reveal radar", &misc.reveal_radar.value );
					ImGui::Checkbox( "Preserve killfeed", &misc.preserve_killfeed.value );
					ImGui::Checkbox( "Disable game logs", &misc.disable_game_logs.value );
					this->draw_function( "Scoreboard weapons", misc.m_scoreboard_weapons.enabled );
					this->draw_color( "Scoreboard color", misc.m_scoreboard_weapons.color );
					this->draw_function( "Watermark", misc.m_watermark.enabled );
					if ( misc.m_watermark.enabled.value )
					{
						ImGui::Text( "Watermark elements" );
						ImGui::Checkbox( "FPS##watermark", &misc.m_watermark.show_fps.value );
						ImGui::Checkbox( "Ping##watermark", &misc.m_watermark.show_ping.value );
						ImGui::Checkbox( "Time##watermark", &misc.m_watermark.show_time.value );
						ImGui::Checkbox( "User##watermark", &misc.m_watermark.show_user.value );
						ImGui::Checkbox( "Map##watermark", &misc.m_watermark.show_map.value );
						ImGui::Checkbox( "Tick rate##watermark", &misc.m_watermark.show_tick.value );
						ImGui::Checkbox( "Velocity##watermark", &misc.m_watermark.show_velocity.value );
					}
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

} // namespace rendering
