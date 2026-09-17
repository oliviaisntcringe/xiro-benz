#include <pch/pch.hpp>
#include <core/features/features.hpp>
#include <core/settings.hpp>
#include <external/imgui/imgui.h>
#include <core/rendering/rendering.hpp>
#include "../../imgui_menu.hpp"

namespace rendering {

void imgui_menu::draw_legit_tab( )
{

			auto& legitbot = settings::g_combat.m_legitbot;
			static constexpr const char* weapon_groups[ 6 ]{ "Pistols", "SMG", "Rifles", "Shotguns", "Snipers", "LMG" };
			static constexpr const char* hitboxes[ 5 ]{ "Head", "Chest", "Stomach", "Arms", "Legs" };

			this->draw_function( "Enable legitbot", legitbot.enabled );
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
				this->draw_function( "Aimbot##aim", group.aimbot );
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
				this->draw_function( "Triggerbot##aim", group.triggerbot );
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

} // namespace rendering
