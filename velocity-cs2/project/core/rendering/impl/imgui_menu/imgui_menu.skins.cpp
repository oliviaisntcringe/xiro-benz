#include <pch/pch.hpp>
#include <core/features/features.hpp>
#include <core/settings.hpp>
#include <external/imgui/imgui.h>
#include <core/rendering/rendering.hpp>
#include "../../imgui_menu.hpp"

namespace rendering {

void imgui_menu::draw_skins_tab( )
{

			auto& changer = settings::g_changer;
			auto& econ = features::changer::g_econ_item_system;
			static constexpr const char* categories[ 4 ]{ "Weapons", "Knives", "Gloves", "Agents" };
			static int category{};
			static int selected_item{};
			static int selected_paint_id{};
			static int last_category{ -1 };
			static bool weapon_selection_dirty{};
			static char skin_search[ 128 ]{};
			static int agent_team{ 2 };

			auto display_name = [ ]( const std::string& localized, const std::string& fallback )
			{
				if ( !localized.empty( ) && localized != "???" && localized.find( "???" ) == std::string::npos )
				{
					return localized;
				}

				if ( !fallback.empty( ) && fallback != "???" && fallback.find( "???" ) == std::string::npos )
				{
					return fallback;
				}

				return std::string{ "Unknown item" };
			};
			auto rarity_color = [ ]( int rarity )
			{
				static constexpr ImVec4 colors[ 8 ]{
					{ 0.92f, 0.92f, 0.92f, 1.0f }, { 0.54f, 0.68f, 0.91f, 1.0f },
					{ 0.30f, 0.45f, 0.77f, 1.0f }, { 0.54f, 0.34f, 0.81f, 1.0f },
					{ 0.83f, 0.17f, 0.90f, 1.0f }, { 0.92f, 0.29f, 0.29f, 1.0f },
					{ 0.89f, 0.68f, 0.22f, 1.0f }, { 1.0f, 0.84f, 0.0f, 1.0f }
				};
				return colors[ std::clamp( rarity, 0, 7 ) ];
			};

			ImGui::Text( "Skin changer" );
			for ( auto i = 0; i < 4; ++i )
			{
				if ( i > 0 )
				{
					ImGui::SameLine( );
				}
				if ( ImGui::Selectable( categories[ i ], category == i, 0, ImVec2{ 92.0f, 28.0f } ) )
				{
					category = i;
					selected_item = 0;
					selected_paint_id = 0;
					weapon_selection_dirty = false;
				}
			}
			ImGui::Separator( );

			if ( category == 3 )
			{
				if ( ImGui::Selectable( "Terrorist", agent_team == 2, 0, ImVec2{ 110.0f, 26.0f } ) )
				{
					agent_team = 2;
					selected_item = 0;
					weapon_selection_dirty = false;
				}
				ImGui::SameLine( );
				if ( ImGui::Selectable( "Counter-Terrorist", agent_team == 3, 0, ImVec2{ 150.0f, 26.0f } ) )
				{
					agent_team = 3;
					selected_item = 0;
					weapon_selection_dirty = false;
				}
				ImGui::Separator( );
			}

			std::vector< const features::changer::econ_item_system::item_def* > items;
			if ( category == 0 )
			{
				items = econ.guns( );
			}
			else if ( category == 1 )
			{
				items = econ.knives( );
			}
			else if ( category == 2 )
			{
				items = econ.gloves( );
			}
			else
			{
				for ( const auto* agent : econ.agents( ) )
				{
					if ( agent->team( ) == agent_team )
					{
						items.push_back( agent );
					}
				}
			}
			if ( items.empty( ) )
			{
				ImGui::TextDisabled( "No item definitions are available yet." );
			}
			else
			{
				if ( category != last_category )
				{
					last_category = category;
					weapon_selection_dirty = false;
				}

				if ( !weapon_selection_dirty && ( category == 1 || category == 2 ) )
				{
					for ( auto i = 0; i < static_cast< int >( items.size( ) ); ++i )
					{
						if ( changer.skins.data.contains( items[ i ]->def_index ) )
						{
							selected_item = i;
							break;
						}
					}
				}

				selected_item = std::clamp( selected_item, 0, static_cast< int >( items.size( ) ) - 1 );
				const auto* item = items[ selected_item ];
				const auto weapon_name = display_name( item->localized_name, item->name );

				ImGui::BeginChild( "##skin_preview", ImVec2{ 280.0f, 0.0f }, true );
				const auto applied_it = changer.skins.data.find( item->def_index );
				const auto applied_paint = applied_it != changer.skins.data.end( ) ? applied_it->second.paint_kit_id : 0;
				if ( !weapon_selection_dirty && applied_paint != 0 )
				{
					selected_paint_id = applied_paint;
				}
				const auto selected_paint = selected_paint_id != 0 ? econ.find_paint_kit( selected_paint_id ) : econ.find_paint_kit( applied_paint );
				const auto skin_name = selected_paint ? display_name( selected_paint->localized_name, selected_paint->name ) : std::string{ "Default" };
				ImGui::TextWrapped( "%s | %s", weapon_name.c_str( ), skin_name.c_str( ) );
				ImGui::Separator( );

				const auto preview = econ.get_skin_image( item->def_index, selected_paint ? selected_paint->id : 0 );
				if ( preview && preview->width > 0 && preview->height > 0 )
				{
					const auto max_size = ImVec2{ 240.0f, 190.0f };
					const auto aspect = static_cast< float >( preview->width ) / static_cast< float >( preview->height );
					auto preview_size = ImVec2{ max_size.x, max_size.x / aspect };
					if ( preview_size.y > max_size.y )
					{
						preview_size = ImVec2{ max_size.y * aspect, max_size.y };
					}

					ImGui::SetCursorPosX( ( ImGui::GetWindowWidth( ) - preview_size.x ) * 0.5f );
					ImGui::Image( reinterpret_cast< ImTextureID >( preview->srv.Get( ) ), preview_size );
				}
				else
				{
					ImGui::TextDisabled( "Loading preview..." );
				}

				ImGui::Separator( );
				ImGui::Text( "Weapon" );
				ImGui::SetNextItemWidth( -1.0f );
				std::vector< const char* > item_names;
				item_names.reserve( items.size( ) );
				std::vector< std::string > item_name_storage;
				item_name_storage.reserve( items.size( ) );
				for ( const auto* entry : items )
				{
					item_name_storage.push_back( display_name( entry->localized_name, entry->name ) );
					item_names.push_back( item_name_storage.back( ).c_str( ) );
				}
				if ( ImGui::Combo( "##selected_weapon", &selected_item, item_names.data( ), static_cast< int >( item_names.size( ) ) ) )
				{
					selected_paint_id = 0;
					weapon_selection_dirty = true;
				}
				if ( category == 3 )
				{
					ImGui::Text( "Team: %s", item->team( ) == 3 ? "Counter-Terrorist" : "Terrorist" );
					if ( item->team( ) == 3 )
					{
						changer.agents.ct_def = item->def_index;
					}
					else if ( item->team( ) == 2 )
					{
						changer.agents.t_def = item->def_index;
					}
				}
				ImGui::EndChild( );

				ImGui::SameLine( );
				ImGui::BeginChild( "##skin_list", ImVec2{ 0.0f, 0.0f }, true );
				ImGui::Text( "Choose skin" );
				ImGui::InputText( "Search##skin", skin_search, sizeof( skin_search ) );
				ImGui::Separator( );

				std::vector< const features::changer::econ_item_system::paint_kit* > paint_kits;
				std::string skin_search_lower{ skin_search };
				for ( auto& character : skin_search_lower )
				{
					character = static_cast< char >( std::tolower( static_cast< unsigned char >( character ) ) );
				}
				for ( const auto& skin : econ.skins( ) )
				{
					if ( skin.def_index != item->def_index )
					{
						continue;
					}

					if ( const auto* paint = econ.find_paint_kit( skin.paint_kit_id ) )
					{
						auto paint_name = display_name( paint->localized_name, paint->name );
						std::string paint_name_lower{ paint_name };
						for ( auto& character : paint_name_lower )
						{
							character = static_cast< char >( std::tolower( static_cast< unsigned char >( character ) ) );
						}
						if ( !skin_search_lower.empty( ) && paint_name_lower.find( skin_search_lower ) == std::string::npos )
						{
							continue;
						}
						paint_kits.push_back( paint );
					}
				}

				if ( paint_kits.empty( ) )
				{
					ImGui::TextDisabled( "No skins are available for this weapon." );
				}
				else
				{
					bool selected_is_valid = selected_paint_id == 0;
					for ( const auto* paint : paint_kits )
					{
						selected_is_valid = selected_is_valid || paint->id == selected_paint_id;
					}
					if ( !selected_is_valid )
					{
						selected_paint_id = 0;
					}

					for ( const auto* paint : paint_kits )
					{
						const auto paint_name = display_name( paint->localized_name, paint->name );
						const auto rarity = econ.combined_rarity( item->def_index, paint->id );
						ImGui::PushID( paint->id );
						const auto* thumbnail = econ.get_skin_image( item->def_index, paint->id );
						if ( thumbnail && thumbnail->srv )
						{
							ImGui::Image( reinterpret_cast< ImTextureID >( thumbnail->srv.Get( ) ), ImVec2{ 64.0f, 30.0f } );
						}
						else
						{
							ImGui::Dummy( ImVec2{ 64.0f, 30.0f } );
						}
						ImGui::SameLine( 80.0f );
						ImGui::PushStyleColor( ImGuiCol_Text, rarity_color( rarity ) );
						if ( ImGui::Selectable( paint_name.c_str( ), selected_paint_id == paint->id, 0, ImVec2{ 0.0f, 30.0f } ) )
						{
							selected_paint_id = paint->id;
							if ( category == 1 || category == 2 )
							{
								for ( auto it = changer.skins.data.begin( ); it != changer.skins.data.end( ); )
								{
									const auto* existing = econ.find_def( it->first );
									if ( existing && existing->category == ( category == 1 ? features::changer::econ_item_system::item_category::knife : features::changer::econ_item_system::item_category::glove ) )
									{
										it = changer.skins.data.erase( it );
									}
									else
									{
										++it;
									}
								}
							}
							changer.skins.data[ item->def_index ].paint_kit_id = paint->id;
							weapon_selection_dirty = false;
						}
						ImGui::PopStyleColor( );
						ImGui::SameLine( );
						ImGui::TextDisabled( "[R%d]", rarity );
						ImGui::PopID( );
					}

					if ( selected_paint_id != 0 )
					{
						auto& applied = changer.skins.data[ item->def_index ];
						ImGui::Separator( );
						ImGui::SliderFloat( "Wear", &applied.wear, 0.0f, 1.0f, "%.4f" );
						ImGui::SliderInt( "Seed", &applied.seed, 0, 1000 );
						ImGui::Checkbox( "StatTrak", &applied.stattrak );
						if ( ImGui::Button( "Clear selected skin" ) )
						{
							changer.skins.data.erase( item->def_index );
							selected_paint_id = 0;
						}
					}
				}
				ImGui::EndChild( );
			}
}

} // namespace rendering
