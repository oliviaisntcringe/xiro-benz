#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace features::changer::econ_attributes
{
	struct sticker
	{
		bool enabled{};
		int kit{};
		float wear{};
		float scale{ 1.0f };
		float rotation{};
		float offset_x{};
		float offset_y{};
	};

	struct keychain
	{
		bool enabled{};
		int id{};
		int seed{};
		float offset_x{};
		float offset_y{};
		float offset_z{};
	};

	using sticker_array = std::array<sticker, 5>;

	bool create( std::uintptr_t item_view, int paint_kit, float wear, int seed, int stattrak, const sticker_array& stickers, const keychain& charm );
	bool remove( std::uintptr_t item_view );
	bool sync_stickers( std::uintptr_t item_view, const sticker_array& stickers );
	bool set_keychain_id( std::uintptr_t item_view, int id );
}
