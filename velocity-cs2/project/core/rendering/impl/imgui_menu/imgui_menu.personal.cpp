#include <pch/pch.hpp>
#include <core/features/features.hpp>
#include <core/settings.hpp>
#include <external/imgui/imgui.h>
#include <core/rendering/rendering.hpp>
#include "../../imgui_menu.hpp"

namespace rendering {

void imgui_menu::draw_personal_tab( )
{
			static constexpr const char* hat_types[ 5 ]{ "Kasa", "Bucket", "Halo", "Crown", "Horns" };

			ImGui::Text( "Personal" );
			ImGui::Separator( );
			ImGui::BeginGroup( );
			ImGui::Text( "Hat" );
			this->draw_function( "Enable hat", hat.enabled );
			auto hat_type = static_cast< int >( hat.type.value );
			if ( ImGui::Combo( "Hat type", &hat_type, hat_types, IM_ARRAYSIZE( hat_types ) ) )
			{
				hat.type.value = static_cast< settings::misc::hud::hat::hat_type >( hat_type );
			}
			this->draw_color( "Hat color", hat.color );
			this->draw_color( "Hat secondary color", hat.secondary_color );
			ImGui::Checkbox( "Hat glow", &hat.glow.value );
			ImGui::SliderFloat( "Hat glow strength", &hat.glow_strength.value, 0.1f, 1.0f, "%.2f" );
			ImGui::EndGroup( );

			ImGui::SameLine( );
			ImGui::BeginGroup( );
			ImGui::Text( "Agent preview" );
			ImGui::BeginChild( "##personal_model_preview", ImVec2{ 320.0f, 390.0f }, true );
			const auto preview_srv = systems::g_model_preview.get_current_texture_srv( );
			const auto preview_origin = ImGui::GetCursorScreenPos( );
			const auto preview_size = ImVec2{ 300.0f, 360.0f };
			if ( preview_srv )
			{
				ImGui::Image( reinterpret_cast< ImTextureID >( preview_srv ), preview_size );
				if ( hat.enabled )
				{
					auto* draw_list = ImGui::GetWindowDrawList( );
					const auto center_x = preview_origin.x + preview_size.x * 0.5f;
					const auto hat_y = preview_origin.y + preview_size.y * 0.22f;
					const auto primary = ImGui::ColorConvertU32ToFloat4( IM_COL32( hat.color.value.r, hat.color.value.g, hat.color.value.b, hat.color.value.a ) );
					const auto secondary = ImGui::ColorConvertU32ToFloat4( IM_COL32( hat.secondary_color.value.r, hat.secondary_color.value.g, hat.secondary_color.value.b, hat.secondary_color.value.a ) );
					const auto primary_u32 = ImGui::ColorConvertFloat4ToU32( primary );
					const auto secondary_u32 = ImGui::ColorConvertFloat4ToU32( secondary );
					draw_list->AddEllipse( ImVec2{ center_x, hat_y + 30.0f }, ImVec2{ 48.0f, 12.0f }, primary_u32, 0, 2.0f );
					if ( hat.type.value == settings::misc::hud::hat::hat_type::halo )
					{
						draw_list->AddEllipse( ImVec2{ center_x, hat_y + 10.0f }, ImVec2{ 52.0f, 14.0f }, primary_u32, 0, 32, 2.0f );
					}
					else if ( hat.type.value == settings::misc::hud::hat::hat_type::crown )
					{
						draw_list->AddRect( ImVec2{ center_x - 38.0f, hat_y + 6.0f }, ImVec2{ center_x + 38.0f, hat_y + 34.0f }, primary_u32, 0.0f, 0, 2.0f );
						draw_list->AddLine( ImVec2{ center_x - 38.0f, hat_y + 6.0f }, ImVec2{ center_x - 25.0f, hat_y - 22.0f }, secondary_u32, 2.0f );
						draw_list->AddLine( ImVec2{ center_x - 25.0f, hat_y - 22.0f }, ImVec2{ center_x, hat_y + 6.0f }, secondary_u32, 2.0f );
						draw_list->AddLine( ImVec2{ center_x, hat_y + 6.0f }, ImVec2{ center_x + 25.0f, hat_y - 22.0f }, secondary_u32, 2.0f );
						draw_list->AddLine( ImVec2{ center_x + 25.0f, hat_y - 22.0f }, ImVec2{ center_x + 38.0f, hat_y + 6.0f }, secondary_u32, 2.0f );
					}
					else if ( hat.type.value == settings::misc::hud::hat::hat_type::horns )
					{
						draw_list->AddLine( ImVec2{ center_x - 30.0f, hat_y + 24.0f }, ImVec2{ center_x - 46.0f, hat_y - 18.0f }, secondary_u32, 2.0f );
						draw_list->AddLine( ImVec2{ center_x - 46.0f, hat_y - 18.0f }, ImVec2{ center_x - 12.0f, hat_y + 10.0f }, secondary_u32, 2.0f );
						draw_list->AddLine( ImVec2{ center_x + 30.0f, hat_y + 24.0f }, ImVec2{ center_x + 46.0f, hat_y - 18.0f }, secondary_u32, 2.0f );
						draw_list->AddLine( ImVec2{ center_x + 46.0f, hat_y - 18.0f }, ImVec2{ center_x + 12.0f, hat_y + 10.0f }, secondary_u32, 2.0f );
					}
					else if ( hat.type.value == settings::misc::hud::hat::hat_type::kasa )
					{
						draw_list->AddLine( ImVec2{ center_x - 34.0f, hat_y + 30.0f }, ImVec2{ center_x, hat_y - 18.0f }, secondary_u32, 2.0f );
						draw_list->AddLine( ImVec2{ center_x, hat_y - 18.0f }, ImVec2{ center_x + 34.0f, hat_y + 30.0f }, secondary_u32, 2.0f );
					}
					else
					{
						draw_list->AddRect( ImVec2{ center_x - 24.0f, hat_y - 8.0f }, ImVec2{ center_x + 24.0f, hat_y + 30.0f }, secondary_u32, 0.0f, 0, 2.0f );
					}
				}
			}
			else
			{
				ImGui::TextDisabled( "Waiting for agent preview..." );
			}
			ImGui::EndChild( );
			ImGui::EndGroup( );
}

} // namespace rendering
