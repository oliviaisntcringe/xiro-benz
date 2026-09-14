#include <pch/pch.hpp>
#include <core/settings.hpp>
#include <core/systems/systems.hpp>
#include <utilities/addresses/addresses.hpp>
#include <utilities/memory/memory.hpp>
#include <external/imgui/imgui.h>

#include "../rendering.hpp"

namespace rendering {

	namespace {

		constexpr auto panel = IM_COL32( 23, 23, 23, 238 );
		constexpr auto raised = IM_COL32( 41, 41, 41, 245 );
		constexpr auto border = IM_COL32( 96, 96, 96, 235 );
		constexpr auto border_dim = IM_COL32( 63, 63, 63, 220 );
		constexpr auto text = IM_COL32( 228, 228, 228, 255 );
		constexpr auto green = IM_COL32( 119, 200, 74, 255 );
		constexpr auto green_dim = IM_COL32( 63, 111, 53, 255 );
		constexpr auto purple = IM_COL32( 141, 74, 176, 255 );

		void draw_frame( ImDrawList* draw, ImVec2 min, ImVec2 max, ImU32 fill = panel )
		{
			draw->AddRectFilled( min, max, fill, 0.0f );
			draw->AddRect( min, max, border, 0.0f, 0, 1.0f );
			draw->AddRectFilled( ImVec2{ min.x, max.y - 1.0f }, max, green_dim );
		}

	}

	void widgets::draw( )
	{
		if ( !ImGui::GetCurrentContext( ) || g_menu.is_open( ) )
		{
			return;
		}

		const auto* viewport = ImGui::GetMainViewport( );
		if ( !viewport )
		{
			return;
		}

		auto* draw = ImGui::GetForegroundDrawList( );
		const auto& wm = settings::g_misc.m_watermark;
		if ( wm.enabled.value )
		{
			this->draw_watermark( draw, viewport );
		}
		this->draw_keybinds( draw, viewport );
	}

	void widgets::draw_watermark( ImDrawList* draw, const ImGuiViewport* viewport )
	{
		const auto& wm = settings::g_misc.m_watermark;
		const auto local = systems::g_local.get( );
		const auto fps = static_cast< int >( std::round( ImGui::GetIO( ).Framerate ) );
		const auto ping = local.is_valid( ) && local.controller && systems::g_entities.exists( local.controller )
			? memory::safe_read<std::uint32_t>( local.controller + SCHEMA( "CCSPlayerController", "m_iPing"_hash ) ).value_or( 0u )
			: 0u;

		SYSTEMTIME time{};
		GetLocalTime( &time );
		char time_text[ 8 ]{};
		std::snprintf( time_text, sizeof( time_text ), "%02u:%02u", time.wHour, time.wMinute );

		std::vector<std::string> values{};
		values.emplace_back( "TRIADA" );
		if ( wm.show_user.value ) values.emplace_back( "developer" );
		if ( wm.show_map.value && !s_map_name.empty( ) ) values.emplace_back( s_map_name );
		if ( wm.show_ping.value ) values.emplace_back( std::format( "{} ms", ping ) );
		if ( wm.show_fps.value ) values.emplace_back( std::format( "{} fps", fps ) );
		static auto last_server_tick{ 0 };
		static auto last_curtime{ 0.0f };
		static auto measured_tickrate{ 0 };
		if ( local.is_valid( ) && local.controller )
		{
			const auto tick_state = addresses::globals::network_client_service
				? memory::call_vfunc<std::uintptr_t>( addresses::globals::network_client_service, 23 )
				: 0;
			const auto server_tick = tick_state ? memory::safe_read<int>( tick_state + 892 ).value_or( 0 ) : 0;
			const auto global_vars = memory::safe_read<std::uintptr_t>( addresses::globals::global_vars ).value_or( 0 );
			const auto curtime = global_vars ? memory::safe_read<float>( global_vars + 0x30 ).value_or( 0.0f ) : 0.0f;
			if ( server_tick > 0 && last_server_tick > 0 && curtime - last_curtime >= 2.0f )
			{
				const auto tick_delta = server_tick - last_server_tick;
				const auto time_delta = curtime - last_curtime;
				if ( tick_delta > 0 && time_delta > 0.5f )
				{
					const auto rate = static_cast< int >( std::round( tick_delta / time_delta ) );
					if ( rate >= 16 && rate <= 256 ) measured_tickrate = rate;
				}
				last_server_tick = server_tick;
				last_curtime = curtime;
			}
			else if ( last_server_tick == 0 && server_tick > 0 )
			{
				last_server_tick = server_tick;
				last_curtime = curtime;
			}
		}
		else
		{
			last_server_tick = 0;
			last_curtime = 0.0f;
			measured_tickrate = 0;
		}
		if ( wm.show_tick.value && measured_tickrate > 0 ) values.emplace_back( std::format( "{} tick", measured_tickrate ) );
		if ( wm.show_velocity.value && local.is_alive && local.pawn )
		{
			const auto velocity = memory::safe_read<math::vector3>( local.pawn + SCHEMA( "C_BaseEntity", "m_vecAbsVelocity"_hash ) ).value_or( math::vector3{} );
			values.emplace_back( std::format( "{:.0f} u/s", velocity.length_2d( ) ) );
		}
		if ( wm.show_time.value ) values.emplace_back( time_text );

		float width = 20.0f;
		for ( const auto& value : values )
		{
			width += ImGui::CalcTextSize( value.c_str( ) ).x + 22.0f;
		}
		width = std::max( 172.0f, width );
		const ImVec2 min{ viewport->WorkPos.x + viewport->WorkSize.x - width - 14.0f, viewport->WorkPos.y + 14.0f };
		const ImVec2 max{ min.x + width, min.y + 30.0f };
		draw_frame( draw, min, max );
		draw->AddRectFilled( ImVec2{ min.x + 1.0f, min.y + 1.0f }, ImVec2{ min.x + 5.0f, max.y - 1.0f }, green );
		draw->AddText( ImVec2{ min.x + 14.0f, min.y + 8.0f }, green, values.front( ).c_str( ) );

		float x = min.x + 94.0f;
		for ( std::size_t i = 1; i < values.size( ); ++i )
		{
			const auto item_width = ImGui::CalcTextSize( values[ i ].c_str( ) ).x + 22.0f;
			draw->AddLine( ImVec2{ x - 8.0f, min.y + 7.0f }, ImVec2{ x - 8.0f, max.y - 7.0f }, border_dim, 1.0f );
			draw->AddText( ImVec2{ x, min.y + 8.0f }, i == values.size( ) - 1 ? purple : text, values[ i ].c_str( ) );
			x += item_width;
		}
	}

	void widgets::draw_keybinds( ImDrawList* draw, const ImGuiViewport* viewport )
	{
		struct row { const char* name; const char* key; };
		std::array<row, 24> rows{};
		std::size_t count{};
		for ( auto* setting : xui::binds::all( ) )
		{
			if ( !setting || !setting->bind.key || !setting->bind.active || count >= rows.size( ) )
			{
				continue;
			}
			rows[ count++ ] = { setting->name.c_str( ), xui::vk_name( setting->bind.key ) };
		}
		if ( !count )
		{
			return;
		}

		constexpr auto row_height = 23.0f;
		constexpr auto panel_width = 238.0f;
		const auto panel_height = 36.0f + row_height * static_cast< float >( count );
		const ImVec2 min{ viewport->WorkPos.x + 14.0f, viewport->WorkPos.y + 64.0f };
		const ImVec2 max{ min.x + panel_width, min.y + panel_height };
		draw_frame( draw, min, max );
		draw->AddText( ImVec2{ min.x + 12.0f, min.y + 9.0f }, green, "[ KEYBINDS ]" );
		draw->AddLine( ImVec2{ min.x + 12.0f, min.y + 29.0f }, ImVec2{ max.x - 12.0f, min.y + 29.0f }, green_dim, 1.0f );

		for ( std::size_t i = 0; i < count; ++i )
		{
			const auto y = min.y + 36.0f + row_height * static_cast< float >( i );
			if ( i )
			{
				draw->AddLine( ImVec2{ min.x + 12.0f, y }, ImVec2{ max.x - 12.0f, y }, border_dim, 1.0f );
			}
			draw->AddText( ImVec2{ min.x + 12.0f, y + 5.0f }, text, rows[ i ].name );
			const auto key_width = ImGui::CalcTextSize( rows[ i ].key ).x + 12.0f;
			draw->AddRectFilled( ImVec2{ max.x - key_width - 12.0f, y + 4.0f }, ImVec2{ max.x - 12.0f, y + 20.0f }, raised );
			draw->AddRect( ImVec2{ max.x - key_width - 12.0f, y + 4.0f }, ImVec2{ max.x - 12.0f, y + 20.0f }, border_dim );
			draw->AddText( ImVec2{ max.x - key_width - 6.0f, y + 5.0f }, green, rows[ i ].key );
		}
	}

} // namespace rendering
