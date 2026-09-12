#pragma once

#include <algorithm>

#include <external/xdraw/xdraw.hpp>
#include "retro_style_tokens.hpp"
#include "rendering.hpp"

namespace rendering::retro
{
	[[nodiscard]] inline float viewport_scale( )
	{
		const auto [width, height] = xdraw::viewport_size( );
		return std::clamp( std::min( static_cast<float>( width ) / 1920.0f, static_cast<float>( height ) / 1080.0f ), 0.75f, 1.25f );
	}

	[[nodiscard]] constexpr xdraw::color to_xdraw_color( color value )
	{
		return { value.r, value.g, value.b, value.a };
	}

	inline void draw_frame( xdraw::draw_list& draw_list, const layout_rect& rect, color fill = palette::panel, color outline = palette::border )
	{
		draw_list.rect_filled( rect.x, rect.y, rect.w, rect.h, to_xdraw_color( fill ), xdraw::corner_radius{ palette::corner_radius } );
		draw_list.rect( rect.x, rect.y, rect.w, rect.h, to_xdraw_color( outline ), xdraw::corner_radius{ palette::corner_radius }, 1.0f );
	}

	inline void draw_rule( xdraw::draw_list& draw_list, float x0, float y, float x1, color token_color )
	{
		draw_list.line( x0, y, x1, y, to_xdraw_color( token_color ), 1.0f );
	}

	inline void push_font( float scale = 1.0f )
	{
		const auto size = scale < 0.9f ? rendering::fonts::size::petite : rendering::fonts::size::normal;
		xdraw::push_font( rendering::g_fonts.smallest_pixel7[ size ] );
	}

	inline void pop_font( )
	{
		xdraw::pop_font( );
	}
}
