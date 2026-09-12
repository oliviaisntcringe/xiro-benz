#include <pch/pch.hpp>
#include <utilities/math/math.hpp>
#include <utilities/memory/memory.hpp>
#include <utilities/addresses/addresses.hpp>
#include <core/systems/systems.hpp>
#include <core/settings.hpp>
#include <core/features/features.hpp>

#include "../rendering.hpp"
#include "../retro_style.hpp"
#include <utilities/security/security.hpp>

namespace rendering {

	void widgets::draw( )
	{
		auto& dl = xdraw::get( );

		if ( settings::g_misc.m_watermark.enabled.value )
		{
			this->watermark( dl );
		}

		this->keybinds( dl );
	}

	void widgets::watermark( xdraw::draw_list& draw_list )
	{
		const auto screen_w = static_cast<float>( xdraw::viewport_size( ).first );
		const auto scale = retro::viewport_scale( );
		const auto& wm = settings::g_misc.m_watermark;
		const auto framerate = xdraw::framerate( );
		const auto local = systems::g_local.get( );

		const auto margin = 10.0f * scale;

		// ── time ────────────────────────────────────────────────────────────
		SYSTEMTIME st{};
		GetLocalTime( &st );
		char time_buf[ 8 ]{};
		std::snprintf( time_buf, sizeof( time_buf ), "%02d:%02d", st.wHour, st.wMinute );

		// ── fps ─────────────────────────────────────────────────────────────
		static auto smoothed_fps{ 0.0f };
		if ( smoothed_fps == 0.0f ) smoothed_fps = framerate;
		smoothed_fps += ( framerate - smoothed_fps ) * std::min( 2.0f * xdraw::delta_time( ), 1.0f );
		char fps_val[ 8 ]{};
		std::snprintf( fps_val, sizeof( fps_val ), "%.0f", smoothed_fps );

		// ── ping ────────────────────────────────────────────────────────────
		auto ping{ 0 };
		if ( local.is_alive && local.controller && systems::g_entities.exists( local.controller ) )
			ping = memory::read<std::uint32_t>( local.controller + SCHEMA( "CCSPlayerController", "m_iPing"_hash ) );
		char ping_val[ 8 ]{};
		std::snprintf( ping_val, sizeof( ping_val ), "%d", ping );

		// ── map name (stored reliably from level_initialization hook) ────────
		const bool has_map = wm.show_map.value && !s_map_name.empty( );

		// ── tick rate (measured from server_tick delta over ~2 s of real time) ─
		static auto last_server_tick{ 0 };
		static auto last_curtime{ 0.0f };
		static auto measured_tickrate{ 0 };

		if ( local.controller )
		{
			const auto net_for_tick = addresses::globals::network_client_service;
			const auto tick_state   = net_for_tick ? memory::call_vfunc<std::uintptr_t>( net_for_tick, 23 ) : 0;
			const auto server_tick  = tick_state   ? memory::read<int>( tick_state + 892 ) : 0;
			const auto gv           = memory::read<std::uintptr_t>( addresses::globals::global_vars );
			const auto curtime      = gv ? memory::read<float>( gv + 0x30 ) : 0.0f;

			if ( server_tick > 0 && last_server_tick > 0 && curtime - last_curtime >= 2.0f )
			{
				const auto tick_delta = server_tick - last_server_tick;
				const auto time_delta = curtime - last_curtime;
				if ( tick_delta > 0 && time_delta > 0.5f )
				{
					const auto rate = static_cast<int>( std::round( tick_delta / time_delta ) );
					if ( rate >= 16 && rate <= 256 ) measured_tickrate = rate;
				}
				last_server_tick = server_tick;
				last_curtime     = curtime;
			}
			else if ( last_server_tick == 0 && server_tick > 0 )
			{
				last_server_tick = server_tick;
				last_curtime     = curtime;
			}
		}
		else
		{
			last_server_tick = 0;
			last_curtime     = 0.0f;
			measured_tickrate = 0;
		}

		const bool has_tick = wm.show_tick.value && local.controller && measured_tickrate > 0;
		char tick_val[ 8 ]{};
		if ( has_tick ) std::snprintf( tick_val, sizeof( tick_val ), "%d", measured_tickrate );

		// ── velocity ──────────────────────────────────────────────────────
		const bool has_velocity = wm.show_velocity.value && local.is_alive && local.pawn;
		static auto smoothed_velocity{ 0.0f };
		char vel_val[ 8 ]{};
		if ( has_velocity )
		{
			const auto velocity = memory::read<math::vector3>( local.pawn + SCHEMA( "C_BaseEntity", "m_vecAbsVelocity"_hash ) );
			const auto speed = velocity.length_2d( );
			smoothed_velocity += ( speed - smoothed_velocity ) * std::min( 8.0f * xdraw::delta_time( ), 1.0f );
			std::snprintf( vel_val, sizeof( vel_val ), "%.0f", smoothed_velocity );
		}
		else
		{
			smoothed_velocity = 0.0f;
		}


		struct stat_entry
		{
			const char* label{};
			const char* value{};
			const char* unit{};
		};

		std::array<stat_entry, 8> stats{};
		auto stat_count = 0u;
		auto add_stat = [ & ]( const char* label, const char* value, const char* unit = "" )
			{
				if ( stat_count < stats.size( ) ) stats[ stat_count++ ] = { label, value, unit };
			};

		if ( wm.show_fps.value )  add_stat( "FPS", fps_val );
		if ( wm.show_ping.value ) add_stat( "PING", ping_val, "ms" );
		if ( has_map )            add_stat( "MAP", s_map_name.c_str( ) );
		if ( has_tick )           add_stat( "TICK", tick_val );
		if ( has_velocity )       add_stat( "VEL", vel_val, "u/s" );
		if ( wm.show_time.value ) add_stat( "TIME", time_buf );
		if ( wm.show_user.value ) add_stat( "USER", "developer" );

		const auto panel_w = 400.0f * scale;
		const auto panel_h = 44.0f * scale;
		const auto header_h = 14.0f * scale;
		const auto grid_pad = 9.0f * scale;
		const auto cell_gap = 5.0f * scale;
		const auto cell_h = 13.0f * scale;
		const auto x = screen_w - panel_w - margin;
		const auto y = margin;

		retro::draw_frame( draw_list, { x, y, panel_w, panel_h }, retro::palette::panel, retro::palette::border_strong );
		retro::push_font( scale );
		draw_list.rect_filled( x, y, 3.0f * scale, panel_h, retro::to_xdraw_color( retro::palette::accent_green ) );
		draw_list.text( x + grid_pad + 2.0f * scale, y + 1.0f * scale, "XI/RO.BENZ", retro::to_xdraw_color( retro::palette::accent_green ) );
		const auto live_w = xdraw::measure_text( "LIVE" ).first;
		draw_list.text( x + panel_w - grid_pad - live_w, y + 1.0f * scale, "LIVE", retro::to_xdraw_color( retro::palette::focus ) );
		retro::draw_rule( draw_list, x + grid_pad, y + header_h, x + panel_w - grid_pad, retro::palette::accent_green_dim );

		const auto columns = 4u;
		const auto cell_w = ( panel_w - grid_pad * 2.0f - cell_gap * static_cast<float>( columns - 1 ) ) / static_cast<float>( columns );
		for ( auto i = 0u; i < stat_count; ++i )
		{
			const auto& stat = stats[ i ];
			const auto col = i % columns;
			const auto row = i / columns;
			const auto cell_x = x + grid_pad + static_cast<float>( col ) * ( cell_w + cell_gap );
			const auto cell_y = y + header_h + 2.0f * scale + static_cast<float>( row ) * cell_h;

			draw_list.push_clip( cell_x, cell_y, cell_w, cell_h );
			draw_list.text( cell_x, cell_y, stat.label, retro::to_xdraw_color( retro::palette::text_muted ) );
			const auto [ value_w, value_h ] = xdraw::measure_text( stat.value );
			const auto [ unit_w, unit_h ] = xdraw::measure_text( stat.unit );
			const auto value_y = cell_y + cell_h - std::max( value_h, unit_h );
			draw_list.text( cell_x, value_y, stat.value, retro::to_xdraw_color( retro::palette::text ) );
			if ( stat.unit[ 0 ] != '\0' )
				draw_list.text( cell_x + value_w + 3.0f, value_y, stat.unit, retro::to_xdraw_color( retro::palette::accent_green_dim ) );
			draw_list.pop_clip( );
		}

		retro::pop_font( );
	}

	void widgets::keybinds( xdraw::draw_list& draw_list )
	{
		struct row_anim_t
		{
			animation::fade alpha;
			animation::spring offset_y;
			bool active_this_frame{ false };
		};

		static std::map<std::string, row_anim_t> row_states;
		static animation::fade container_alpha;
		static animation::spring smoothed_base_y;

		const auto screen_h = static_cast<float>( xdraw::viewport_size( ).second );
		const auto scale = retro::viewport_scale( );

		const auto margin = 10.0f * scale;
		const auto row_spacing = 2.0f * scale;
		const auto row_h = 18.0f * scale;
		const auto header_h = 21.0f * scale;
		const auto panel_w = 300.0f * scale;
		const auto text_pad_x = 10.0f * scale;

		struct bind_entry
		{
			const char* name;
			char value[ 32 ];
			char unit[ 8 ];
			bool has_value;
			xui::bind_mode mode;
		};

		bind_entry entries[ 32 ]{};
		auto count{ 0 };

		const auto& ctx = features::combat::g_shared.ctx( );
		const auto has_weapon = ctx.valid && ctx.weapon_type >= cstypes::weapon_type::pistol && ctx.weapon_type <= cstypes::weapon_type::lmg;

		for ( const auto setting : xui::binds::all( ) )
		{
			if ( !setting || setting->bind.key == 0 || !setting->bind.active || count >= 32 )
			{
				continue;
			}

			auto is_rage_group{ false };
			for ( auto i = 0u; i < settings::combat::ragebot::k_group_count; ++i )
			{
				const auto& g = settings::g_combat.m_ragebot.groups[ i ];
				if ( setting == &g.min_damage_override || setting == &g.hitchance_override || setting == &g.force_shot || setting == &g.force_shot_air || setting == &g.body_aim || setting == &g.silent || setting == &g.no_spread )
				{
					is_rage_group = true;
					break;
				}
			}

			if ( is_rage_group )
			{
				if ( !settings::g_combat.m_ragebot.enabled || !has_weapon )
				{
					continue;
				}

				const auto active_group = &settings::g_combat.m_ragebot.get_group( ctx.weapon_type );
				auto is_active{ false };

				for ( auto i = 0u; i < settings::combat::ragebot::k_group_count; ++i )
				{
					const auto& g = settings::g_combat.m_ragebot.groups[ i ];
					if ( &g == active_group )
					{
						if ( setting == &g.min_damage_override || setting == &g.hitchance_override || setting == &g.force_shot || setting == &g.force_shot_air || setting == &g.body_aim )
						{
							is_active = true;
						}
						break;
					}
				}

				if ( !is_active )
				{
					continue;
				}

				auto& e = entries[ count++ ];
				e.name = setting->name.c_str( );
				e.mode = setting->bind.mode;

				if ( setting == &active_group->min_damage_override )
				{
					std::snprintf( e.value, sizeof( e.value ), "%d", active_group->min_damage_override_value.value );
					e.has_value = true;
				}
				else if ( setting == &active_group->hitchance_override )
				{
					std::snprintf( e.value, sizeof( e.value ), "%d", active_group->hitchance_override_value.value );
					e.unit[ 0 ] = '%';
					e.unit[ 1 ] = '\0';
					e.has_value = true;
				}
				else
				{
					e.value[ 0 ] = '\0';
					e.has_value = false;
				}
				continue;
			}

			auto is_legit_group{ false };
			for ( auto i = 0u; i < settings::combat::legitbot::k_group_count; ++i )
			{
				const auto& g = settings::g_combat.m_legitbot.groups[ i ];
				if ( setting == &g.aimbot || setting == &g.rcs || setting == &g.standalone_rcs || setting == &g.triggerbot || setting == &g.autowall || setting == &g.visualize_fov || setting == &g.trigger_head_only || setting == &g.give_me_your_seed )
				{
					is_legit_group = true;
					break;
				}
			}

			if ( is_legit_group )
			{
				if ( !settings::g_combat.m_legitbot.enabled.value || !has_weapon )
				{
					continue;
				}

				const auto* active_group = &settings::g_combat.m_legitbot.get_group( ctx.weapon_type );
				auto is_active{ false };

				for ( auto i = 0u; i < settings::combat::legitbot::k_group_count; ++i )
				{
					if ( &settings::g_combat.m_legitbot.groups[ i ] == active_group )
					{
						const auto& g = settings::g_combat.m_legitbot.groups[ i ];
						if ( setting == &g.aimbot || setting == &g.rcs || setting == &g.standalone_rcs || setting == &g.triggerbot || setting == &g.autowall || setting == &g.visualize_fov || setting == &g.trigger_head_only || setting == &g.give_me_your_seed )
						{
							is_active = true;
						}

						if ( is_active && setting == &active_group->give_me_your_seed && !active_group->triggerbot.value )
						{
							is_active = false;
						}
						break;
					}
				}

				if ( !is_active )
				{
					continue;
				}

				auto& e = entries[ count++ ];
				e.name = setting->name.c_str( );
				e.mode = setting->bind.mode;
				e.value[ 0 ] = '\0';
				e.has_value = false;
				continue;
			}

			if ( setting == &settings::g_combat.m_antiaim.enabled || setting == &settings::g_combat.m_antiaim.manual_left || setting == &settings::g_combat.m_antiaim.manual_right || setting == &settings::g_combat.m_antiaim.hide_shots || setting == &settings::g_combat.m_antiaim.avoid_backstab || setting == &settings::g_combat.m_antiaim.direction_indicator )
			{
				if ( !settings::g_combat.m_antiaim.enabled.value )
				{
					continue;
				}
			}

			auto& e = entries[ count++ ];
			e.name = setting->name.c_str( );
			e.mode = setting->bind.mode;
			e.value[ 0 ] = '\0';
			e.has_value = false;
		}

		if ( count > 0 )
			container_alpha.fade_in( 0.2f );
		else
			container_alpha.fade_out( 0.2f );

		container_alpha.update( );
		if ( !container_alpha.visible( ) )
			return;

		const auto master_alpha = container_alpha.alpha( );
		const auto total_h = header_h + row_spacing + ( static_cast< float >( count ) * ( row_h + row_spacing ) );
		const auto target_base_y = ( screen_h * 0.5f ) - ( total_h * 0.5f );

		smoothed_base_y.set_target( target_base_y );
		smoothed_base_y.update( );

		const auto base_ry = smoothed_base_y.value( );
		const auto x = margin;
		auto with_alpha = [ ]( retro::color color, float alpha )
			{
				color.a = static_cast<std::uint8_t>( std::clamp( alpha, 0.0f, 1.0f ) * 255.0f );
				return color;
			};

		retro::draw_frame(
			draw_list,
			{ x, base_ry, panel_w, total_h },
			with_alpha( retro::palette::panel, master_alpha ),
			with_alpha( retro::palette::border, master_alpha )
		);

		retro::push_font( scale );
		draw_list.text( x + text_pad_x, base_ry + 5.0f * scale, "> keybinds", retro::to_xdraw_color( with_alpha( retro::palette::accent_green, master_alpha ) ) );
		retro::draw_rule( draw_list, x + text_pad_x, base_ry + header_h, x + panel_w - text_pad_x, with_alpha( retro::palette::accent_green_dim, master_alpha ) );

		for ( auto& [ name, state ] : row_states )
			state.active_this_frame = false;

		float current_offset_y = header_h + row_spacing;
		for ( auto i = 0; i < count; ++i )
		{
			const auto& e = entries[ i ];
			auto& anim = row_states[ e.name ];

			if ( !anim.active_this_frame && anim.alpha.alpha( ) <= 0.01f )
				anim.offset_y.snap( current_offset_y );

			anim.active_this_frame = true;
			anim.alpha.fade_in( 0.2f );
			anim.offset_y.set_target( current_offset_y );
			anim.alpha.update( );
			anim.offset_y.update( );

			const auto row_alpha = anim.alpha.alpha( ) * master_alpha;
			const auto draw_y = base_ry + anim.offset_y.value( );
			const auto name_h = xdraw::measure_text( e.name ).second;
			const auto text_col = ( e.mode == xui::bind_mode::toggle ) ? retro::palette::text_muted : retro::palette::accent_green;
			const auto name_clip_w = e.has_value ? panel_w - text_pad_x * 2.0f - 72.0f : panel_w - text_pad_x * 2.0f;

			draw_list.push_clip( x + text_pad_x, draw_y, name_clip_w, row_h );
			draw_list.text(
				x + text_pad_x,
				draw_y + ( row_h - name_h ) * 0.5f,
				e.name,
				retro::to_xdraw_color( with_alpha( text_col, row_alpha ) )
			);
			draw_list.pop_clip( );

			if ( e.has_value )
			{
				const auto [ value_w, value_h ] = xdraw::measure_text( e.value );
				const auto [ unit_w, unit_h ] = xdraw::measure_text( e.unit );
				const auto value_x = x + panel_w - text_pad_x - value_w - ( e.unit[ 0 ] ? unit_w + 3.0f : 0.0f );
				const auto value_y = draw_y + ( row_h - std::max( value_h, unit_h ) ) * 0.5f;
				draw_list.text( value_x, value_y, e.value, retro::to_xdraw_color( with_alpha( retro::palette::accent_green, row_alpha ) ) );
				if ( e.unit[ 0 ] )
					draw_list.text( value_x + value_w + 3.0f, value_y, e.unit, retro::to_xdraw_color( with_alpha( retro::palette::text_muted, row_alpha ) ) );
			}

			retro::draw_rule( draw_list, x + text_pad_x, draw_y + row_h, x + panel_w - text_pad_x, with_alpha( retro::palette::accent_green_dim, row_alpha * 0.65f ) );
			current_offset_y += row_h + row_spacing;
		}

		for ( auto it = row_states.begin( ); it != row_states.end( ); )
		{
			if ( !it->second.active_this_frame )
			{
				it->second.alpha.fade_out( 0.15f );
				it->second.alpha.update( );
				it->second.offset_y.update( );

				if ( it->second.alpha.alpha( ) <= 0.001f )
				{
					it = row_states.erase( it );
					continue;
				}

				const auto row_alpha = it->second.alpha.alpha( ) * master_alpha;
				const auto draw_y = base_ry + it->second.offset_y.value( );
				const auto name_h = xdraw::measure_text( it->first.c_str( ) ).second;

				draw_list.push_clip( x + text_pad_x, draw_y, panel_w - text_pad_x * 2.0f, row_h );
				draw_list.text(
					x + text_pad_x,
					draw_y + ( row_h - name_h ) * 0.5f,
					it->first.c_str( ),
					retro::to_xdraw_color( with_alpha( retro::palette::text_muted, row_alpha ) )
				);
				draw_list.pop_clip( );
			}
			++it;
		}

		retro::pop_font( );
	}

} // namespace rendering
