#pragma once

#include <cstdint>

namespace rendering::retro
{
	struct color
	{
		std::uint8_t r{};
		std::uint8_t g{};
		std::uint8_t b{};
		std::uint8_t a{ 255 };

		constexpr color( ) = default;
		constexpr color( std::uint8_t red, std::uint8_t green, std::uint8_t blue, std::uint8_t alpha = 255 )
			: r{ red }, g{ green }, b{ blue }, a{ alpha }
		{
		}
	};

	struct layout_rect
	{
		float x{};
		float y{};
		float w{};
		float h{};
	};

	struct palette
	{
		inline static constexpr color canvas{ 0x17, 0x17, 0x17 };
		inline static constexpr color panel{ 0x20, 0x20, 0x20 };
		inline static constexpr color panel_raised{ 0x29, 0x29, 0x29 };
		inline static constexpr color border{ 0x60, 0x60, 0x60 };
		inline static constexpr color border_strong{ 0x85, 0x85, 0x85 };
		inline static constexpr color text{ 0xE4, 0xE4, 0xE4 };
		inline static constexpr color text_muted{ 0x90, 0x90, 0x90 };
		inline static constexpr color accent_green{ 0x77, 0xC8, 0x4A };
		inline static constexpr color accent_green_dim{ 0x3F, 0x6F, 0x35 };
		inline static constexpr color accent_purple{ 0x8D, 0x4A, 0xB0 };
		inline static constexpr color focus{ 0xB4, 0xF7, 0x6A };

		inline static constexpr float corner_radius{ 0.0f };
	};

	[[nodiscard]] constexpr layout_rect centered_rect( float viewport_w, float panel_w, float y, float panel_h )
	{
		return { ( viewport_w - panel_w ) * 0.5f, y, panel_w, panel_h };
	}
}
