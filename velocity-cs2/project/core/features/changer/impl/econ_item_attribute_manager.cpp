#include <pch/pch.hpp>
#include <core/settings.hpp>
#include <core/systems/systems.hpp>
#include <utilities/addresses/addresses.hpp>
#include <utilities/diag.hpp>
#include <utilities/logging/logging.hpp>
#include <utilities/memory/memory.hpp>

#include <atomic>

#include "econ_item_attribute_manager.hpp"

namespace features::changer::econ_attributes
{
	namespace
	{
		struct attribute
		{
			std::byte pad[ 0x30 ];
			std::uint16_t definition{};
			std::byte pad2[ 2 ];
			float value{};
			float initial_value{};
			std::int32_t refundable_currency{};
			bool set_bonus{};
			std::byte pad3[ 7 ];
		};

		static_assert( sizeof( attribute ) == 0x48 );

		struct attribute_vector
		{
			std::int32_t size{};
			std::int32_t pad{};
			std::uintptr_t data{};
			std::int32_t allocation_count{};
			std::int32_t grow_flags{};
		};

		static_assert( sizeof( attribute_vector ) == 0x18 );

		constexpr std::uint16_t paint_attribute = 6;
		constexpr std::uint16_t seed_attribute = 7;
		constexpr std::uint16_t wear_attribute = 8;
		constexpr std::uint16_t stattrak_attribute = 80;
		constexpr std::uint16_t sticker_base = 113;
		constexpr std::uint16_t sticker_stride = 4;
		constexpr std::uint16_t sticker_offset_base = 278;
		constexpr std::uint16_t sticker_offset_stride = 2;
		constexpr std::uint16_t sticker_schema_base = 290;
		constexpr std::uint16_t keychain_id = 299;
		constexpr std::uint16_t keychain_x = 300;
		constexpr std::uint16_t keychain_y = 301;
		constexpr std::uint16_t keychain_z = 302;
		constexpr std::uint16_t keychain_seed = 303;
		constexpr std::uintptr_t item_view_attribute_vector_offset = 0x210;
		constexpr std::int32_t maximum_attribute_count = 16384;

		std::uintptr_t allocate( std::size_t bytes )
		{
			if ( !addresses::globals::mem_alloc )
			{
				return 0;
			}
			return reinterpret_cast<std::uintptr_t>( memory::call_vfunc<void*>( addresses::globals::mem_alloc, 1, bytes ) );
		}

		void release( std::uintptr_t pointer )
		{
			if ( pointer && addresses::globals::mem_alloc )
			{
				memory::call_vfunc<void>( addresses::globals::mem_alloc, 3, reinterpret_cast<void*>( pointer ) );
			}
		}

		bool read_vector( std::uintptr_t item_view, attribute_vector& output )
		{
			if ( item_view < 0x10000 || item_view == static_cast< std::uintptr_t >( -1 ) )
			{
				return false;
			}

			const auto value = memory::safe_read<attribute_vector>( item_view + item_view_attribute_vector_offset );
			if ( !value || value->size < 0 || value->size > maximum_attribute_count ||
				value->allocation_count < 0 || value->allocation_count > maximum_attribute_count * 2 ||
				( value->size > 0 && ( value->data < 0x10000 || value->allocation_count < value->size ) ) )
			{
				return false;
			}

			output = *value;
			return true;
		}

		bool write_vector( std::uintptr_t item_view, const attribute_vector& value )
		{
			return item_view >= 0x10000 && item_view != static_cast< std::uintptr_t >( -1 ) &&
				memory::safe_write<attribute_vector>( item_view + item_view_attribute_vector_offset, value );
		}

		bool enabled( const sticker& value )
		{
			return value.enabled && value.kit > 0;
		}

		void set_integer( attribute& value, std::uint16_t definition, std::int32_t data )
		{
			value.definition = definition;
			value.value = std::bit_cast<float>( data );
			value.initial_value = value.value;
		}

		void set_float( attribute& value, std::uint16_t definition, float data )
		{
			value.definition = definition;
			value.value = data;
			value.initial_value = data;
		}

		std::size_t count_stickers( const sticker_array& stickers )
		{
			std::size_t count{};
			for ( std::size_t slot{}; slot < stickers.size( ); ++slot )
			{
				const auto& value = stickers[ slot ];
				if ( !enabled( value ) )
				{
					continue;
				}
				count += 4;
				if ( value.offset_x != 0.0f || value.offset_y != 0.0f )
				{
					count += 3;
				}
			}
			return count;
		}

		std::size_t count_keychain( const keychain& value )
		{
			return value.enabled && value.id > 0 ? 5 : 0;
		}

		void write_stickers( attribute* output, std::size_t& index, const sticker_array& stickers )
		{
			for ( std::size_t slot{}; slot < stickers.size( ); ++slot )
			{
				const auto& value = stickers[ slot ];
				if ( !enabled( value ) )
				{
					continue;
				}

				const auto base = static_cast<std::uint16_t>( sticker_base + slot * sticker_stride );
				set_integer( output[ index++ ], base, value.kit );
				set_float( output[ index++ ], base + 1, std::clamp( value.wear, 0.0f, 1.0f ) );
				set_float( output[ index++ ], base + 2, std::clamp( value.scale, 0.1f, 5.0f ) );
				set_float( output[ index++ ], base + 3, std::clamp( value.rotation, -180.0f, 180.0f ) );

				if ( value.offset_x != 0.0f || value.offset_y != 0.0f )
				{
					const auto offset_base = static_cast<std::uint16_t>( sticker_offset_base + slot * sticker_offset_stride );
					set_float( output[ index++ ], offset_base, std::clamp( value.offset_x, -0.5f, 0.5f ) );
					set_float( output[ index++ ], offset_base + 1, std::clamp( value.offset_y, -0.5f, 0.5f ) );
					set_integer( output[ index++ ], static_cast<std::uint16_t>( sticker_schema_base + slot ), 0 );
				}
			}
		}

		void write_keychain( attribute* output, std::size_t& index, const keychain& value )
		{
			if ( !value.enabled || value.id <= 0 )
			{
				return;
			}
			set_integer( output[ index++ ], keychain_id, value.id );
			set_float( output[ index++ ], keychain_x, value.offset_x );
			set_float( output[ index++ ], keychain_y, value.offset_y );
			set_float( output[ index++ ], keychain_z, value.offset_z );
			set_integer( output[ index++ ], keychain_seed, std::clamp( value.seed, 0, 100000 ) );
		}
	}

	bool create( std::uintptr_t item_view, int paint_kit, float wear, int seed, int stattrak, const sticker_array& stickers, const keychain& charm )
	{
		diag::exception_scope exception_scope{ "econ_attributes::create" };
		diag::writef( diag::level::debug,
			"[econ] create begin item=0x%p paint=%d wear=%.4f seed=%d stattrak=%d stickers=%u keychain=%u",
			reinterpret_cast< void* >( item_view ), paint_kit, wear, seed, stattrak,
			static_cast< unsigned >( std::count_if( stickers.begin( ), stickers.end( ), []( const auto& value ) { return enabled( value ); } ) ),
			( charm.enabled && charm.id > 0 ) ? 1u : 0u );
		attribute_vector vector{};
		if ( !read_vector( item_view, vector ) )
		{
			static std::atomic_bool reported_missing_vector{};
			if ( !reported_missing_vector.exchange( true, std::memory_order_relaxed ) )
			{
				logging::console::print( xs( "[econ] item attribute vector is unavailable; stickers/keychain skipped" ) );
			}
			diag::write( diag::level::warning, "[econ] create aborted: attribute vector read failed" );
			return false;
		}
		if ( vector.size != 0 || vector.data != 0 )
		{
			diag::writef( diag::level::debug, "[econ] create skipped: existing vector size=%d data=0x%p capacity=%d",
				vector.size, reinterpret_cast< void* >( vector.data ), vector.allocation_count );
			static std::atomic_bool reported_existing_vector{};
			if ( !reported_existing_vector.exchange( true, std::memory_order_relaxed ) )
			{
				logging::console::print( xs( "[econ] item attribute vector already owns data; using in-place cosmetic sync" ) );
			}
			return false;
		}

		const auto count = ( paint_kit > 0 ? 3u : 0u ) + ( stattrak >= 0 ? 1u : 0u ) + count_stickers( stickers ) + count_keychain( charm );
		if ( count == 0 )
		{
			diag::write( diag::level::debug, "[econ] create skipped: no active attributes" );
			return true;
		}

		constexpr auto extra_capacity = 8u;
		const auto capacity = count + extra_capacity;
		const auto data = allocate( capacity * sizeof( attribute ) );
		if ( !data )
		{
			diag::writef( diag::level::error, "[econ] create allocation failed count=%zu capacity=%zu", count, capacity );
			logging::console::print( xs( "[econ] attribute allocation failed (count={})" ), count );
			return false;
		}

		std::memset( reinterpret_cast<void*>( data ), 0, capacity * sizeof( attribute ) );
		auto* output = reinterpret_cast<attribute*>( data );
		std::size_t index{};
		if ( paint_kit > 0 )
		{
			set_float( output[ index++ ], paint_attribute, static_cast<float>( paint_kit ) );
			set_float( output[ index++ ], seed_attribute, static_cast<float>( std::clamp( seed, 0, 1000 ) ) );
			set_float( output[ index++ ], wear_attribute, std::clamp( wear, 0.0001f, 1.0f ) );
		}
		if ( stattrak >= 0 )
		{
			set_integer( output[ index++ ], stattrak_attribute, stattrak );
		}
		write_stickers( output, index, stickers );
		write_keychain( output, index, charm );

		vector.size = static_cast<std::int32_t>( index );
		vector.data = data;
		vector.allocation_count = static_cast<std::int32_t>( capacity );
		vector.grow_flags = 0x40000000;
		if ( !write_vector( item_view, vector ) )
		{
			release( data );
			logging::console::print( xs( "[econ] attribute vector commit failed; allocation released" ) );
			return false;
		}
		diag::writef( diag::level::debug, "[econ] create committed size=%d data=0x%p capacity=%d",
			vector.size, reinterpret_cast< void* >( vector.data ), vector.allocation_count );
		return true;
	}

	bool remove( std::uintptr_t item_view )
	{
		diag::exception_scope exception_scope{ "econ_attributes::remove" };
		diag::writef( diag::level::debug, "[econ] remove begin item=0x%p", reinterpret_cast< void* >( item_view ) );
		attribute_vector vector{};
		if ( !read_vector( item_view, vector ) )
		{
			return false;
		}
		const auto data = vector.data;
		if ( !write_vector( item_view, {} ) )
		{
			return false;
		}
		release( data );
		diag::writef( diag::level::debug, "[econ] remove complete item=0x%p data=0x%p", reinterpret_cast< void* >( item_view ), reinterpret_cast< void* >( data ) );
		return true;
	}

	bool sync_stickers( std::uintptr_t item_view, const sticker_array& stickers )
	{
		diag::exception_scope exception_scope{ "econ_attributes::sync_stickers" };
		diag::writef( diag::level::debug, "[econ] sync stickers begin item=0x%p", reinterpret_cast< void* >( item_view ) );
		attribute_vector vector{};
		if ( !read_vector( item_view, vector ) || !vector.data )
		{
			return false;
		}

		for ( std::size_t slot{}; slot < stickers.size( ); ++slot )
		{
			const auto base = static_cast<std::uint16_t>( sticker_base + slot * sticker_stride );
			const auto& value = stickers[ slot ];
			for ( auto i = 0; i < vector.size; ++i )
			{
				const auto address = vector.data + static_cast< std::uintptr_t >( i ) * sizeof( attribute );
				const auto current = memory::safe_read<attribute>( address );
				if ( !current || current->definition != base )
				{
					continue;
				}

				auto updated = *current;
				set_integer( updated, base, enabled( value ) ? value.kit : 0 );
				(void) memory::safe_write<attribute>( address, updated );
				break;
			}
		}
		diag::writef( diag::level::debug, "[econ] sync stickers complete item=0x%p size=%d", reinterpret_cast< void* >( item_view ), vector.size );
		return true;
	}

	bool set_keychain_id( std::uintptr_t item_view, int id )
	{
		diag::exception_scope exception_scope{ "econ_attributes::set_keychain_id" };
		diag::writef( diag::level::debug, "[econ] set keychain id begin item=0x%p id=%d", reinterpret_cast< void* >( item_view ), id );
		attribute_vector vector{};
		if ( !read_vector( item_view, vector ) || !vector.data )
		{
			return false;
		}
		for ( auto i = 0; i < vector.size; ++i )
		{
			const auto address = vector.data + static_cast< std::uintptr_t >( i ) * sizeof( attribute );
			const auto current = memory::safe_read<attribute>( address );
			if ( !current || current->definition != keychain_id )
			{
				continue;
			}
			auto updated = *current;
			set_integer( updated, keychain_id, id );
			const auto written = memory::safe_write<attribute>( address, updated );
			diag::writef( written ? diag::level::debug : diag::level::warning, "[econ] set keychain id end written=%u", written ? 1u : 0u );
			return written;
		}
		return false;
	}

}
