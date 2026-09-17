#include <pch/pch.hpp>
#include <core/rendering/rendering.hpp>
#include <external/config.hpp>
#include <external/imgui/imgui.h>

#include "../../imgui_menu.hpp"

namespace rendering {

	namespace {

		const char* menu_key_name( int key )
		{
			switch ( key )
			{
			case VK_INSERT: return "Insert";
			case VK_DELETE: return "Delete";
			case VK_HOME: return "Home";
			case VK_END: return "End";
			case VK_F1: return "F1";
			case VK_F2: return "F2";
			case VK_F3: return "F3";
			case VK_F4: return "F4";
			case VK_F5: return "F5";
			case VK_F6: return "F6";
			case VK_F7: return "F7";
			case VK_F8: return "F8";
			case VK_F9: return "F9";
			case VK_F10: return "F10";
			case VK_F11: return "F11";
			case VK_F12: return "F12";
			default: return "Custom";
			}
		}

	} // namespace

	void imgui_menu::draw_color( const char* label, config::col& color )
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
	}

	bool imgui_menu::draw_function( const char* label, xui::setting& setting, bool expandable )
	{
		ImGui::PushID( &setting );
		ImGui::BeginGroup( );
		const auto changed = ImGui::Checkbox( label, &setting.value );
		ImGui::SameLine( );
		const auto binding = this->m_rebinding_setting == &setting;
		const auto bind_label = binding ? "..." : menu_key_name( setting.bind.key );
		if ( ImGui::SmallButton( bind_label ) )
		{
			this->m_rebinding_setting = &setting;
		}
		ImGui::SameLine( );
		static constexpr const char* modes[ 3 ]{ "Toggle", "Hold", "Release" };
		auto mode = static_cast< int >( setting.bind.mode );
		ImGui::SetNextItemWidth( 82.0f );
		if ( ImGui::Combo( "##bind_mode", &mode, modes, IM_ARRAYSIZE( modes ) ) )
		{
			setting.bind.mode = static_cast< xui::bind_mode >( mode );
		}
		ImGui::EndGroup( );
		if ( expandable && setting.value )
		{
			ImGui::Indent( 12.0f );
		}
		if ( expandable && setting.value && changed )
		{
			ImGui::SetItemDefaultFocus( );
		}
		if ( expandable && setting.value )
		{
			ImGui::Unindent( 12.0f );
		}
		ImGui::PopID( );
		return changed;
	}

} // namespace rendering
