#include "../velocity-cs2/project/core/rendering/retro_style_tokens.hpp"
#include <cassert>

int main( )
{
	using namespace rendering::retro;
	assert( palette::accent_green.r == 0x77 );
	assert( palette::accent_green.g == 0xC8 );
	assert( palette::accent_green.b == 0x4A );
	const auto r = centered_rect( 1920.0f, 620.0f, 110.0f, 420.0f );
	assert( r.x == 650.0f && r.y == 110.0f && r.w == 620.0f && r.h == 420.0f );
	assert( palette::corner_radius == 0.0f );
}
