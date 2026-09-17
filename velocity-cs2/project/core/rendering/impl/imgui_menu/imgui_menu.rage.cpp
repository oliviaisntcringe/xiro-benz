#include <pch/pch.hpp>
#include <core/features/features.hpp>
#include <core/settings.hpp>
#include <external/imgui/imgui.h>
#include <core/rendering/rendering.hpp>
#include "../../imgui_menu.hpp"

namespace rendering {

void imgui_menu::draw_rage_tab( )
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

			this->draw_function( "Enable ragebot", ragebot.enabled );
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
				this->draw_function( "Zeusbot##rage", zeusbot.enabled );
				if ( zeusbot.enabled.value )
				{
					ImGui::SliderFloat( "Zeus FOV##rage", &zeusbot.max_fov.value, 1.0f, 180.0f, "%.0f deg" );
					ImGui::Checkbox( "Drop after##rage", &zeusbot.drop_after.value );
				}
				this->draw_function( "Knifebot##rage", knifebot.enabled );
				if ( knifebot.enabled.value )
				{
					ImGui::SliderFloat( "Knife FOV##rage", &knifebot.max_fov.value, 1.0f, 180.0f, "%.0f deg" );
				}
				ImGui::EndGroup( );

				ImGui::Separator( );
				ImGui::Text( "Anti aim" );
				this->draw_function( "Enable anti aim##rage", anti_aim.enabled );
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
				this->draw_color( "Direction color##rage", anti_aim.direction_indicator_color );
				ImGui::Checkbox( "Direction glow##rage", &anti_aim.direction_indicator_glow.value );
				ImGui::SliderFloat( "Direction glow strength##rage", &anti_aim.direction_indicator_glow_strength.value, 0.1f, 1.0f, "%.2f" );

				ImGui::Separator( );
				ImGui::Text( "Peek assistance" );
				this->draw_function( "Quick peek##rage", quick_peek.enabled );
				this->draw_color( "Quick peek color##rage", quick_peek.color );
				this->draw_color( "Retracting color##rage", quick_peek.retrack_color );
				this->draw_function( "Duck peek##rage", duck_peek.enabled );
			}
}


} // namespace rendering
