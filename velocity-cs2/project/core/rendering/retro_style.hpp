#pragma once

#include <external/xdraw/xdraw.hpp>
#include "retro_style_tokens.hpp"
#include "rendering.hpp"

namespace rendering::retro
{
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

	inline void push_font( )
	{
		xdraw::push_font( rendering::g_fonts.smallest_pixel7[ rendering::fonts::size::normal ] );
	}
}
