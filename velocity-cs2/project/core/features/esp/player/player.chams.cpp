#include <pch/pch.hpp>
#include <utilities/memory/memory.hpp>
#include <utilities/addresses/addresses.hpp>
#include <utilities/logging/logging.hpp>
#include <core/systems/systems.hpp>
#include <core/settings.hpp>
#include <core/features/features.hpp>
#include <protection/game_addresses.hpp>
#include "../primitive_buffer.hpp"

namespace features::esp::player {
	namespace {
		std::atomic_bool g_scene_object_path_disabled{};

		bool create_scene_object_safe(
			std::uintptr_t mesh_system,
			std::uintptr_t model_handle,
			__m128* transform,
			std::int64_t flags,
			std::uintptr_t world_group_handle,
			std::uintptr_t& scene_object ) noexcept
		{
			__try
			{
				scene_object = memory::call_vfunc<std::uintptr_t>(
					mesh_system,
					20,
					model_handle,
					transform,
					"AnimatableSceneObjectDesc",
					flags,
					0x4100000001ll,
					world_group_handle );
				return true;
			}
			__except ( EXCEPTION_EXECUTE_HANDLER )
			{
				scene_object = 0;
				return false;
			}
		}

		bool destroy_scene_object_safe(
			std::uintptr_t scene_system,
			std::uintptr_t scene_object ) noexcept
		{
			__try
			{
				memory::call_vfunc<void>( scene_system, 16, scene_object );
				return true;
			}
			__except ( EXCEPTION_EXECUTE_HANDLER )
			{
				return false;
			}
		}

		bool get_world_group_id_safe(
			std::uintptr_t function,
			std::uintptr_t game_scene_node,
			int& scratch,
			int*& world_group_id ) noexcept
		{
			world_group_id = nullptr;
			__try
			{
				world_group_id = memory::call<int*>( function, game_scene_node, &scratch );
				return world_group_id != nullptr;
			}
			__except ( EXCEPTION_EXECUTE_HANDLER )
			{
				world_group_id = nullptr;
				return false;
			}
		}

		bool get_world_group_handle_safe(
			std::uintptr_t function,
			std::uintptr_t render_game_system,
			int world_group_id,
			std::uintptr_t& world_group_handle ) noexcept
		{
			world_group_handle = 0;
			__try
			{
				world_group_handle = memory::call<std::uintptr_t>(
					function,
					render_game_system,
					world_group_id );
				return world_group_handle != 0;
			}
			__except ( EXCEPTION_EXECUTE_HANDLER )
			{
				world_group_handle = 0;
				return false;
			}
		}

		void report_scene_path_rejection( const char* reason ) noexcept
		{
			g_scene_object_path_disabled.store( true, std::memory_order_release );
			static std::atomic_bool reported{};
			if ( !reported.exchange( true, std::memory_order_relaxed ) )
			{
				logging::console::print( reason );
			}
		}
	}

	bool chams::on_generate_primitives( std::uintptr_t owner_entity, std::uint32_t owner_hash, std::uintptr_t scene_object, std::uintptr_t primitive_buffer, void( __fastcall* original_fn )( std::uintptr_t, std::uintptr_t, std::uintptr_t, std::uintptr_t ), std::uintptr_t a1, std::uintptr_t scene_view )
	{
		const auto is_player = owner_hash == "C_CSPlayerPawn"_hash;
		const auto is_arms = owner_hash == "C_CS2HudModelArms"_hash;
		const auto is_weapon = owner_hash == "C_CS2HudModelWeapon"_hash;
		const auto scene_object_path_enabled =
			!g_scene_object_path_disabled.load( std::memory_order_acquire );

		const auto is_local_attachment = [ & ]( std::uintptr_t view_pawn ) -> bool
			{
				const auto game_scene_node = memory::safe_read<std::uintptr_t>( owner_entity + SCHEMA( "C_BaseEntity", "m_pGameSceneNode"_hash ) ).value_or( 0 );
				if ( !game_scene_node )
				{
					return false;
				}

				const auto parent_node = memory::safe_read<std::uintptr_t>( game_scene_node + SCHEMA( "CGameSceneNode", "m_pParent"_hash ) ).value_or( 0 );
				if ( !parent_node )
				{
					return false;
				}

				const auto parent_owner = memory::safe_read<std::uintptr_t>( parent_node + SCHEMA( "CGameSceneNode", "m_pOwner"_hash ) ).value_or( 0 );
				return parent_owner == view_pawn;
			};

		const auto apply_config = [ & ]( const settings::esp::chams_config& cfg, std::uintptr_t target_scene_obj, bool force_original = false )
			{
				if ( cfg.secondary.enabled.value )
				{
					this->apply_layer( primitive_buffer, original_fn, a1, target_scene_obj, scene_view, cfg.secondary.color, cfg.secondary.material );
				}

				if ( cfg.primary.enabled.value )
				{
					this->apply_layer( primitive_buffer, original_fn, a1, target_scene_obj, scene_view, cfg.primary.color, cfg.primary.material );
				}

				if ( !cfg.primary.enabled.value && !cfg.secondary.enabled.value && ( cfg.overlay.enabled.value || force_original ) )
				{
					original_fn( a1, target_scene_obj, scene_view, primitive_buffer );
				}

				if ( cfg.overlay.enabled.value )
				{
					this->apply_overlay( primitive_buffer, original_fn, a1, target_scene_obj, scene_view, cfg.overlay.color, cfg.overlay.material );
				}
			};

		if ( !is_player && !is_arms && !is_weapon )
		{
			if ( !settings::g_esp.m_viewmodel.weapon.enabled.value )
			{
				return false;
			}

			if ( !settings::g_misc.m_camera.thirdperson.value || !is_local_attachment( systems::g_local.get( ).view_pawn( ) ) )
			{
				return false;
			}

			apply_config( settings::g_esp.m_viewmodel.weapon, scene_object );
			return true;
		}

		if ( is_arms || is_weapon )
		{
			const auto& cfg = is_arms ? settings::g_esp.m_viewmodel.arms : settings::g_esp.m_viewmodel.weapon;
			if ( !cfg.enabled.value )
			{
				return false;
			}

			apply_config( cfg, scene_object );
			return true;
		}

		const auto local = systems::g_local.get( );
		const auto& chams_cfg = settings::g_esp.m_player.m_chams;

		const auto team = memory::safe_read<int>( owner_entity + SCHEMA( "C_BaseEntity", "m_iTeamNum"_hash ) ).value_or( 0 );
		const auto health = memory::safe_read<int>( owner_entity + SCHEMA( "C_BaseEntity", "m_iHealth"_hash ) ).value_or( 0 );

		const auto is_other_team = local.is_this_other_team( team );
		const auto is_local = owner_entity == local.view_pawn( );
		const auto is_dead = health <= 0;

		if ( scene_object_path_enabled && is_player && is_other_team && !is_dead && chams_cfg.backtrack.enabled.value )
		{
			if ( this->m_backtrack.has_active( owner_entity ) )
			{
				const auto bt_scene_object = this->m_backtrack.get_scene_object( owner_entity );
				if ( bt_scene_object )
				{
					const auto before = detail::read_primitive_buffer( primitive_buffer );
					const auto prev_count = before ? before->count() : -1;

					apply_config( chams_cfg.backtrack, bt_scene_object, true );

					const auto after = detail::read_primitive_buffer( primitive_buffer );
					const auto new_count = after ? after->count() : -1;
					if ( after && prev_count >= 0 && new_count > prev_count )
					{
						for ( auto i = prev_count; i < new_count; ++i )
						{
							detail::mark_primitive_last( after->at( i ) );
						}
					}
				}
			}
		}
		if ( scene_object_path_enabled && is_player && is_other_team && chams_cfg.onshot.enabled.value ) {
			if (this->m_onshot.has_active (owner_entity)) {
				const auto os_obj = this->m_onshot.get_scene_object (owner_entity);
				if (os_obj) {
					const auto& ocfg = chams_cfg.onshot;

					const auto alpha = this->m_onshot.get_alpha (owner_entity);
					const auto fade = [alpha] (xdraw::color c) -> xdraw::color {
						c.a = static_cast<std::uint8_t>(c.a * alpha);
						return c;
					};

					auto faded_cfg = ocfg;

					if (faded_cfg.primary.enabled.value)
						faded_cfg.primary.color.value = fade (faded_cfg.primary.color.value);

					if (faded_cfg.secondary.enabled.value)
						faded_cfg.secondary.color.value = fade (faded_cfg.secondary.color.value);

					if (faded_cfg.overlay.enabled.value)
						faded_cfg.overlay.color.value = fade (faded_cfg.overlay.color.value);

					const auto before = detail::read_primitive_buffer( primitive_buffer );
					const auto prev_count = before ? before->count() : -1;

					apply_config (faded_cfg, os_obj, true);

					const auto after = detail::read_primitive_buffer( primitive_buffer );
					const auto new_count = after ? after->count() : -1;
					if ( after && prev_count >= 0 && new_count > prev_count ) {
						for (auto i = prev_count; i < new_count; ++i)
							detail::mark_primitive_last( after->at( i ) );
					}
				}
			}
		}

		const settings::esp::chams_config* target{ nullptr };

		if ( is_dead )
		{
			if ( is_local )
			{
				target = &chams_cfg.local_ragdoll;
			}
			else if ( is_other_team )
			{
				target = &chams_cfg.enemy_ragdoll;
			}
			else
			{
				target = &chams_cfg.team_ragdoll;
			}
		}
		else
		{
			if ( is_local )
			{
				target = &chams_cfg.local;
			}
			else if ( is_other_team )
			{
				target = &chams_cfg.enemy;
			}
			else
			{
				target = &chams_cfg.team;
			}
		}

		if ( !target || !target->enabled.value )
		{
			return false;
		}

		if ( !target->primary.enabled.value && !target->secondary.enabled.value && !target->overlay.enabled.value )
		{
			return false;
		}

		{
			const auto flags = memory::safe_read<std::uint8_t>( scene_object + 0x78 );
			if ( flags ) {
				(void) memory::safe_write<std::uint8_t>(
					scene_object + 0x78,
					static_cast<std::uint8_t>( *flags & ~( 1u << 3 ) ) );
			}
		}

		if ( is_local )
		{
			if ( misc::g_other.is_alpha_changed( ) )
			{
				this->apply_clone( primitive_buffer, original_fn, a1, scene_object, scene_view, systems::materials::clone_type::translucent );

				if ( target->overlay.enabled.value )
				{
					this->apply_overlay( primitive_buffer, original_fn, a1, scene_object, scene_view, target->overlay.color, target->overlay.material );
				}
			}
			else
			{
				apply_config( *target, scene_object );
			}

			return true;
		}

		apply_config( *target, scene_object );
		return true;
	}

	void chams::on_sort_primitives( std::uintptr_t entries, std::uint32_t count )
	{
		if ( !count || !entries || count > ( 1u << 20 ) )
		{
			return;
		}

		const auto overlay_mat_count = this->m_overlay_material_count.load( std::memory_order_acquire );
		if ( overlay_mat_count <= 0 )
		{
			return;
		}

		const auto total = static_cast< int >( count );
		if ( total <= 1 )
		{
			return;
		}

		std::vector<detail::mesh_primitive> sorted;
		sorted.reserve( total );

		for ( auto i = 0; i < total; ++i )
		{
			const auto primitive = memory::safe_read<detail::mesh_primitive>(
				entries + static_cast<std::size_t>( i ) * detail::primitive_size );
			if ( !primitive ) {
				return;
			}

			sorted.push_back( *primitive );
		}

		const auto overlay_begin = std::stable_partition(
			sorted.begin(), sorted.end(), [ this ]( const auto& primitive ) {
				return !this->is_overlay_material( primitive.material );
			} );
		const auto overlay_count = static_cast<int>(
			std::distance( overlay_begin, sorted.end() ) );

		if ( overlay_count <= 0 || overlay_count >= total )
		{
			return;
		}

		for ( auto i = 0; i < total; ++i )
		{
			if ( !memory::safe_write<detail::mesh_primitive>(
					entries + static_cast<std::size_t>( i ) * detail::primitive_size,
					sorted[ i ] ) ) {
				return;
			}
		}
	}

	void chams::backtrack::update( )
	{
		scene_object_mutation_scope mutation_scope;
		std::unique_lock lock( this->m_mutex );
		const auto& cfg = settings::g_esp.m_player.m_chams;
		const auto local = systems::g_local.get( );

		if ( !local.is_alive || !cfg.backtrack.enabled.value )
		{
			for ( auto it = this->m_objects.begin( ); it != this->m_objects.end( ); )
			{
				it->second.destroy( );
				it = this->m_objects.erase( it );
			}

			return;
		}

		const auto players = systems::g_entities.get_by_type( systems::entities::type::player );

		std::unordered_set<std::uintptr_t> valid_pawns;

		for ( const auto& p : players )
		{
			if ( !p.ptr || p.ptr == local.controller )
			{
				continue;
			}

			const auto pawn_handle = memory::read<std::uint32_t>( p.ptr + SCHEMA( "CBasePlayerController", "m_hPawn"_hash ) );
			const auto pawn = systems::g_entities.lookup( pawn_handle );

			if ( pawn && pawn != local.pawn )
			{
				valid_pawns.insert( pawn );
			}
		}

		for ( auto it = this->m_objects.begin( ); it != this->m_objects.end( ); )
		{
			if ( !valid_pawns.contains( it->first ) )
			{
				it->second.destroy( );
				it = this->m_objects.erase( it );
			}
			else
			{
				++it;
			}
		}

		std::unordered_set<std::uintptr_t> active;

		for ( const auto& p : players )
		{
			if ( !p.ptr || p.ptr == local.controller )
			{
				continue;
			}

			if ( !memory::safe_read<bool>( p.ptr + SCHEMA( "CCSPlayerController", "m_bPawnIsAlive"_hash ) ).value_or( false ) )
			{
				const auto dead_pawn_handle = memory::safe_read<std::uint32_t>( p.ptr + SCHEMA( "CBasePlayerController", "m_hPawn"_hash ) ).value_or( 0u );
				const auto dead_pawn = systems::g_entities.lookup( dead_pawn_handle );
				auto it = this->m_objects.find( dead_pawn );
				if ( dead_pawn && it != this->m_objects.end( ) )
				{
					it->second.destroy( );
					this->m_objects.erase( it );
				}

				continue;
			}

			const auto pawn_handle = memory::read<std::uint32_t>( p.ptr + SCHEMA( "CBasePlayerController", "m_hPawn"_hash ) );
			const auto pawn = systems::g_entities.lookup( pawn_handle );

			if ( !pawn || pawn == local.pawn )
			{
				continue;
			}

			const auto team = memory::read<int>( pawn + SCHEMA( "C_BaseEntity", "m_iTeamNum"_hash ) );
			if ( !local.is_this_other_team( team ) )
			{
				continue;
			}

			const auto health = memory::read<int>( pawn + SCHEMA( "C_BaseEntity", "m_iHealth"_hash ) );
			if ( health <= 0 )
			{
				auto it = this->m_objects.find( pawn );
				if ( it != this->m_objects.end( ) )
				{
					it->second.destroy( );
					this->m_objects.erase( it );
				}

				continue;
			}

			const auto oldest = combat::g_shared.lc( ).get_oldest_was_valid( pawn );
			if ( !oldest )
			{
				auto it = this->m_objects.find( pawn );
				if ( it != this->m_objects.end( ) )
				{
					it->second.destroy( );
					this->m_objects.erase( it );
				}

				continue;
			}

			const auto game_scene_node = memory::read<std::uintptr_t>( pawn + SCHEMA( "C_BaseEntity", "m_pGameSceneNode"_hash ) );
			if ( game_scene_node )
			{
				if ( oldest->origin.distance( memory::read<math::vector3>( game_scene_node + SCHEMA( "CGameSceneNode", "m_vecAbsOrigin"_hash ) ) ) < 0.25f )
				{
					auto it = this->m_objects.find( pawn );
					if ( it != this->m_objects.end( ) )
					{
						it->second.destroy( );
						this->m_objects.erase( it );
					}

					continue;
				}
			}

			auto& obj = this->m_objects[ pawn ];
			if ( !obj.scene_object )
			{
				obj.create( pawn );
			}

			if ( !obj.scene_object )
			{
				continue;
			}

			active.insert( pawn );
			obj.active = true;
			obj.setup_bones( oldest->bones, oldest->bone_count );
		}

		for ( auto it = this->m_objects.begin( ); it != this->m_objects.end( ); )
		{
			if ( !active.contains( it->first ) )
			{
				it->second.destroy( );
				it = this->m_objects.erase( it );
			}
			else
			{
				++it;
			}
		}
	}

	void chams::backtrack::shutdown( )
	{
		scene_object_mutation_scope mutation_scope;
		std::unique_lock lock( this->m_mutex );
		for ( auto& [pawn, obj] : this->m_objects )
		{
			obj.destroy( );
		}

		this->m_objects.clear( );
	}

	bool chams::backtrack::is_active( std::uintptr_t scene_object ) const
	{
		for ( const auto& [pawn, obj] : this->m_objects )
		{
			if ( obj.scene_object == scene_object && obj.active )
			{
				return true;
			}
		}

		return false;
	}

	bool chams::backtrack::has_active( std::uintptr_t pawn ) const
	{
		auto it = this->m_objects.find( pawn );
		return it != this->m_objects.end( ) && it->second.scene_object && it->second.active;
	}

	std::uintptr_t chams::backtrack::get_scene_object( std::uintptr_t pawn ) const
	{
		auto it = this->m_objects.find( pawn );
		if ( it != this->m_objects.end( ) )
		{
			return it->second.scene_object;
		}

		return 0;
	}

	void chams::backtrack::object::create( std::uintptr_t target_pawn )
	{
		this->pawn = target_pawn;
		this->scene_object = 0;
		if ( !target_pawn || g_scene_object_path_disabled.load( std::memory_order_acquire ) )
		{
			return;
		}

		const auto game_scene_node = memory::safe_read<std::uintptr_t>( target_pawn + SCHEMA( "C_BaseEntity", "m_pGameSceneNode"_hash ) ).value_or( 0 );
		if ( !game_scene_node )
		{
			return;
		}

		const auto world_group_pattern = PATTERN( patterns::get_world_group_id );
		const auto world_group_handle_pattern = PATTERN( patterns::get_world_group_handle );
		if ( !world_group_pattern || !world_group_handle_pattern )
		{
			report_scene_path_rejection( xs( "[chams] scene-object path disabled: world-group pattern unavailable" ) );
			return;
		}

		auto temp{ 0 };
		int* world_group_id{};
		if ( !get_world_group_id_safe( world_group_pattern, game_scene_node, temp, world_group_id ) )
		{
			report_scene_path_rejection( xs( "[chams] scene-object path disabled: world-group id call faulted" ) );
			return;
		}

		const auto world_group_value = memory::safe_read<int>( reinterpret_cast<std::uintptr_t>( world_group_id ) );
		if ( !world_group_value )
		{
			return;
		}

		const auto render_game_system = memory::safe_read<std::uintptr_t>( addresses::globals::render_game_system_storage ).value_or( 0 );
		if ( !render_game_system )
		{
			return;
		}

		std::uintptr_t world_group_handle{};
		if ( !get_world_group_handle_safe( world_group_handle_pattern, render_game_system, *world_group_value, world_group_handle ) )
		{
			report_scene_path_rejection( xs( "[chams] scene-object path disabled: world-group handle call faulted" ) );
			return;
		}

		const auto flags = ( *world_group_value != 0 ) ? 0x2000000000ll : 0x2000000008ll;
		const auto model_handle = memory::safe_read<std::uintptr_t>( game_scene_node + SCHEMA( "CSkeletonInstance", "m_modelState"_hash ) + SCHEMA( "CModelState", "m_hModel"_hash ) ).value_or( 0 );
		if ( !model_handle || !addresses::globals::mesh_system )
		{
			return;
		}

		const auto node_to_world = game_scene_node + SCHEMA( "CGameSceneNode", "m_nodeToWorld"_hash );

		__m128 copy[ 2 ]{};
		const auto node_to_world_first = memory::safe_read<__m128>( node_to_world );
		const auto node_to_world_second = memory::safe_read<__m128>( node_to_world + 16 );
		if ( !node_to_world_first || !node_to_world_second )
		{
			return;
		}
		copy[ 0 ] = *node_to_world_first;
		copy[ 1 ] = *node_to_world_second;

		if ( !create_scene_object_safe( addresses::globals::mesh_system, model_handle, copy, flags, world_group_handle, this->scene_object ) )
		{
			report_scene_path_rejection( xs( "[chams] scene-object path disabled: scene object creation faulted" ) );
			return;
		}
		if ( !this->scene_object )
		{
			return;
		}

		if ( !memory::safe_write<std::uintptr_t>( this->scene_object + 0x110, game_scene_node ) ||
			!memory::safe_write<int>( this->scene_object + 0xc0, -1 ) )
		{
			const auto created_scene_object = this->scene_object;
			this->scene_object = 0;
			if ( addresses::globals::scene_system )
			{
				(void) destroy_scene_object_safe( addresses::globals::scene_system, created_scene_object );
			}
			report_scene_path_rejection( xs( "[chams] scene-object path disabled: scene object write faulted" ) );
			return;
		}

		const auto model_data = memory::safe_read<std::uintptr_t>( model_handle ).value_or( 0 );
		if ( model_data )
		{
			const auto model_flags_0 = memory::safe_read<std::uint32_t>( model_data + 16 ).value_or( 0 );
			const auto model_flags_1 = memory::safe_read<std::uint32_t>( model_data + 20 ).value_or( 0 );
			const auto has_force_lod = ( model_flags_0 & 0x400 ) != 0 || ( model_flags_1 & 0x400 ) != 0;
			const auto lod_value = memory::safe_read<std::uint8_t>( this->scene_object + 0x9a );
			if ( lod_value )
			{
				auto lod = *lod_value;
				lod = has_force_lod ? ( lod | 0x10 ) : ( lod & 0xef );
				(void) memory::safe_write( this->scene_object + 0x9a, lod );
			}
		}
	}

	void chams::backtrack::object::destroy( )
	{
		if ( !this->scene_object )
		{
			return;
		}

		const auto flags = memory::safe_read<std::uint64_t>( this->scene_object + 128 );
		if ( !flags || ( *flags & 0x4000000000000000ull ) )
		{
			this->scene_object = 0;
			return;
		}

		if ( !addresses::globals::scene_system )
		{
			this->scene_object = 0;
			return;
		}

		if ( !destroy_scene_object_safe( addresses::globals::scene_system, this->scene_object ) )
		{
			report_scene_path_rejection( xs( "[chams] scene-object path disabled: scene object destruction faulted" ) );
		}
		this->scene_object = 0;
	}

	void chams::backtrack::object::setup_bones( systems::bones::data* bones, int count ) const
	{
		if ( !this->scene_object )
		{
			return;
		}

		const auto obj_bone_count = memory::safe_read<int>( this->scene_object + 0xd0 ).value_or( 0 );
		const auto render_bones = memory::safe_read<std::uintptr_t>( this->scene_object + 0xd8 ).value_or( 0 );

		if ( !bones || count <= 0 || !render_bones || obj_bone_count <= 0 )
		{
			return;
		}

		const auto write_count = std::min( count, obj_bone_count );

		for ( auto i = 0; i < write_count; i++ )
		{
			const auto& b = bones[ i ];
			const auto dst = render_bones + ( static_cast< std::size_t >( i ) * 48 );

			const auto bxx = b.rotation.x * b.rotation.x;
			const auto byy = b.rotation.y * b.rotation.y;
			const auto bzz = b.rotation.z * b.rotation.z;
			const auto bxy = b.rotation.x * b.rotation.y;
			const auto bxz = b.rotation.x * b.rotation.z;
			const auto byz = b.rotation.y * b.rotation.z;
			const auto bwx = b.rotation.w * b.rotation.x;
			const auto bwy = b.rotation.w * b.rotation.y;
			const auto bwz = b.rotation.w * b.rotation.z;

			const float values[ 12 ]{
				1.0f - 2.0f * ( byy + bzz ), 2.0f * ( bxy - bwz ), 2.0f * ( bxz + bwy ), b.position.x,
				2.0f * ( bxy + bwz ), 1.0f - 2.0f * ( bxx + bzz ), 2.0f * ( byz - bwx ), b.position.y,
				2.0f * ( bxz - bwy ), 2.0f * ( byz + bwx ), 1.0f - 2.0f * ( bxx + byy ), b.position.z
			};
			for ( auto component = 0u; component < std::size( values ); ++component )
			{
				if ( !memory::safe_write<float>( dst + component * sizeof( float ), values[ component ] ) )
				{
					return;
				}
			}
		}
	}

	void chams::onshot::push (std::uintptr_t pawn) {
		std::unique_lock lock( this->m_mutex );
		const auto& cfg = settings::g_esp.m_player.m_chams;
		if (!cfg.onshot.enabled.value)
			return;

		const auto records = combat::g_shared.lc ().get_valid_records (pawn);
		if (records.empty ())
			return;

		auto* record = records.front (); /* just the newest for now, kiro make this customizable or smth */
		const auto bone_count = std::clamp (record->bone_count, 0, 27);
		auto& pending = this->m_pending [pawn];
		pending.bone_count = bone_count;
		std::copy_n (record->bones, bone_count, pending.bones.begin ());
	}

	void chams::onshot::update () {
		scene_object_mutation_scope mutation_scope;
		std::unique_lock lock( this->m_mutex );
		const auto& cfg = settings::g_esp.m_player.m_chams;

		if (!cfg.onshot.enabled.value) {
			for (auto& [pawn, e] : this->m_entries)
				e.destroy ();
			this->m_entries.clear ();
			this->m_pending.clear ();
			return;
		}

		const auto global_vars = memory::read<std::uintptr_t> (addresses::globals::global_vars);
		const auto current_time = memory::read<float> (global_vars + 0x30);

		// Creating mesh scene objects from CreateMove can race the scene graph and
		// fault inside client.dll. Materialize queued shots at frame stage instead.
		for (auto& [pawn, pending] : this->m_pending) {
			auto& e = this->m_entries [pawn];
			if (e.scene_object)
				e.destroy ();

			e.create (pawn);
			if (!e.scene_object) {
				this->m_entries.erase (pawn);
				continue;
			}

			e.pawn = pawn;
			e.spawn_time = current_time;
			e.active = true;
			e.setup_bones (pending.bones.data (), pending.bone_count);
		}
		this->m_pending.clear ();

		const auto fade_time = cfg.onshot_fade_time.value;

		for (auto it = this->m_entries.begin (); it != this->m_entries.end (); ) {
			if (current_time - it->second.spawn_time >= fade_time) {
				it->second.destroy ();
				it = this->m_entries.erase (it);
			} else {
				++it;
			}
		}
	}

	void chams::onshot::shutdown () {
		scene_object_mutation_scope mutation_scope;
		std::unique_lock lock( this->m_mutex );
		for (auto& [pawn, e] : this->m_entries)
			e.destroy ();
		this->m_entries.clear ();
		this->m_pending.clear ();
	}

	bool chams::onshot::has_active (std::uintptr_t pawn) const {
		auto it = this->m_entries.find (pawn);
		return it != this->m_entries.end () && it->second.scene_object;
	}

	bool chams::onshot::is_active (std::uintptr_t scene_object) const {
		for (const auto& [pawn, e] : this->m_entries)
			if (e.scene_object == scene_object)
				return true;
		return false;
	}

	std::uintptr_t chams::onshot::get_scene_object (std::uintptr_t pawn) const {
		auto it = this->m_entries.find (pawn);
		if (it != this->m_entries.end ())
			return it->second.scene_object;
		return 0;
	}

	float chams::onshot::get_alpha (std::uintptr_t pawn) const {
		auto it = this->m_entries.find (pawn);
		if (it == this->m_entries.end () || !it->second.scene_object)
			return 0.0f;

		const auto global_vars = memory::read<std::uintptr_t> (addresses::globals::global_vars);
		const auto current_time = memory::read<float> (global_vars + 0x30);
		const auto fade_time = settings::g_esp.m_player.m_chams.onshot_fade_time.value;

		if (fade_time <= 0.0f)
			return 0.0f;

		const auto elapsed = current_time - it->second.spawn_time;
		return std::clamp (1.0f - (elapsed / fade_time), 0.0f, 1.0f);
	}

	void chams::apply_layer( std::uintptr_t primitive_buffer, void( __fastcall* original_fn )( std::uintptr_t, std::uintptr_t, std::uintptr_t, std::uintptr_t ), std::uintptr_t a1, std::uintptr_t scene_object, std::uintptr_t scene_view, const xdraw::color& color, settings::esp::cham_ids material_id )
	{
		const auto before = detail::read_primitive_buffer( primitive_buffer );
		const auto prev_count = before ? before->count() : -1;

		original_fn( a1, scene_object, scene_view, primitive_buffer );

		const auto after = detail::read_primitive_buffer( primitive_buffer );
		const auto new_count = after ? after->count() : -1;
		if ( !after || prev_count < 0 || prev_count >= new_count )
		{
			return;
		}

		const auto material = systems::materials::find( material_id );
		if ( !material )
		{
			return;
		}

		for ( auto i = prev_count; i < new_count; ++i )
		{
			detail::replace_primitive( after->at( i ), material, color );
		}
	}

	void chams::apply_overlay( std::uintptr_t primitive_buffer, void( __fastcall* original_fn )( std::uintptr_t, std::uintptr_t, std::uintptr_t, std::uintptr_t ), std::uintptr_t a1, std::uintptr_t scene_object, std::uintptr_t scene_view, const xdraw::color& color, settings::esp::cham_ids material_id )
	{
		const auto material = systems::materials::find( material_id );
		if ( !material )
		{
			return;
		}

		const auto before = detail::read_primitive_buffer( primitive_buffer );
		const auto prev_count = before ? before->count() : -1;

		original_fn( a1, scene_object, scene_view, primitive_buffer );

		const auto after = detail::read_primitive_buffer( primitive_buffer );
		const auto new_count = after ? after->count() : -1;
		if ( !after || prev_count < 0 || prev_count >= new_count )
		{
			return;
		}

		for ( auto i = prev_count; i < new_count; ++i )
		{
			detail::replace_primitive( after->at( i ), material, color );
		}

		this->add_overlay_material( material );
	}

	void chams::apply_clone( std::uintptr_t primitive_buffer, void( __fastcall* original_fn )( std::uintptr_t, std::uintptr_t, std::uintptr_t, std::uintptr_t ), std::uintptr_t a1, std::uintptr_t scene_object, std::uintptr_t scene_view, systems::materials::clone_type type )
	{
		const auto before = detail::read_primitive_buffer( primitive_buffer );
		const auto prev_count = before ? before->count() : -1;

		original_fn( a1, scene_object, scene_view, primitive_buffer );

		const auto after = detail::read_primitive_buffer( primitive_buffer );
		const auto new_count = after ? after->count() : -1;
		if ( !after || prev_count < 0 || prev_count >= new_count )
		{
			return;
		}

		for ( auto i = prev_count; i < new_count; ++i )
		{
			const auto primitive = after->at( i );
			const auto orig_mat = memory::safe_read<std::uintptr_t>(
				primitive + detail::primitive_material_offset );

			if ( !orig_mat || !*orig_mat )
			{
				continue;
			}

			const auto clone = systems::materials::get_or_create_clone( *orig_mat, type );
			if ( !clone )
			{
				continue;
			}

			(void) memory::safe_write<std::uintptr_t>(
				primitive + detail::primitive_material_offset, clone );
			(void) memory::safe_write<std::uintptr_t>(
				primitive + detail::primitive_material_copy_offset, clone );
		}
	}

	bool chams::is_overlay_material( std::uintptr_t mat ) const
	{
		const auto count = this->m_overlay_material_count.load( std::memory_order_acquire );

		for ( auto i = 0; i < count; ++i )
		{
			if ( this->m_overlay_materials[ i ].load( std::memory_order_relaxed ) == mat )
			{
				return true;
			}
		}

		return false;
	}

	void chams::add_overlay_material( std::uintptr_t mat )
	{
		const auto count = this->m_overlay_material_count.load( std::memory_order_acquire );

		for ( auto i = 0; i < count; ++i )
		{
			if ( this->m_overlay_materials[ i ].load( std::memory_order_relaxed ) == mat )
			{
				return;
			}
		}

		const auto idx = this->m_overlay_material_count.fetch_add( 1, std::memory_order_acq_rel );

		if ( idx < k_max_overlay_materials )
		{
			this->m_overlay_materials[ idx ].store( mat, std::memory_order_release );
		}
		else
		{
			this->m_overlay_material_count.fetch_sub( 1, std::memory_order_release );
		}
	}

} // namespace features::esp::player
