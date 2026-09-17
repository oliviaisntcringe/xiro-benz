#include <pch/pch.hpp>
#include <core/features/features.hpp>
#include <core/settings.hpp>
#include <external/imgui/imgui.h>
#include <core/rendering/rendering.hpp>
#include "../../imgui_menu.hpp"

namespace rendering {

void imgui_menu::draw_config_tab( )
{

			static std::vector<std::wstring> config_list{};
			static std::string search{};
			static int selected{ -1 };
			static bool refresh{ true };
			static char search_buffer[ 128 ]{};
			static bool confirm_delete{ false };

			auto to_utf8 = [ ]( const std::wstring& value )
			{
				char buffer[ 256 ]{};
				WideCharToMultiByte( CP_UTF8, 0, value.c_str( ), -1, buffer, sizeof( buffer ), nullptr, nullptr );
				return std::string{ buffer };
			};
			auto to_wide = [ ]( const std::string& value )
			{
				wchar_t buffer[ 256 ]{};
				MultiByteToWideChar( CP_UTF8, 0, value.c_str( ), -1, buffer, IM_ARRAYSIZE( buffer ) );
				return std::wstring{ buffer };
			};
			auto copy_clipboard = [ ]( const std::string& value )
			{
				if ( !OpenClipboard( nullptr ) )
				{
					return;
				}
				EmptyClipboard( );
				const auto bytes = ( value.size( ) + 1 ) * sizeof( char );
				if ( const auto memory = GlobalAlloc( GMEM_MOVEABLE, bytes ) )
				{
					if ( const auto destination = GlobalLock( memory ) )
					{
						std::memcpy( destination, value.c_str( ), bytes );
						GlobalUnlock( memory );
						SetClipboardData( CF_TEXT, memory );
					}
					else
					{
						GlobalFree( memory );
					}
				}
				CloseClipboard( );
			};
			auto paste_clipboard = [ ]( )
			{
				std::string result{};
				if ( !OpenClipboard( nullptr ) )
				{
					return result;
				}
				if ( const auto memory = GetClipboardData( CF_TEXT ) )
				{
					if ( const auto source = static_cast< const char* >( GlobalLock( memory ) ) )
					{
						result = source;
						GlobalUnlock( memory );
					}
				}
				CloseClipboard( );
				return result;
			};

			if ( refresh )
			{
				config_list = config::registry::list( );
				selected = std::clamp( selected, -1, static_cast< int >( config_list.size( ) ) - 1 );
				refresh = false;
			}

			ImGui::Text( "Configuration" );
			if ( ImGui::InputText( "Search", search_buffer, sizeof( search_buffer ) ) )
			{
				search = search_buffer;
			}
			ImGui::BeginChild( "##config_list", ImVec2{ 0.0f, -42.0f }, true );
			for ( auto i = 0; i < static_cast< int >( config_list.size( ) ); ++i )
			{
				const auto name = to_utf8( config_list[ i ] );
				if ( !search.empty( ) && name.find( search ) == std::string::npos )
				{
					continue;
				}
				if ( ImGui::Selectable( name.c_str( ), selected == i ) )
				{
					selected = i;
					confirm_delete = false;
				}
			}
			ImGui::EndChild( );

			const auto has_selection = selected >= 0 && selected < static_cast< int >( config_list.size( ) );
			const auto button_width = ( ImGui::GetContentRegionAvail( ).x - ImGui::GetStyle( ).ItemSpacing.x * 5.0f ) / 6.0f;
			const auto save_name = has_selection ? to_utf8( config_list[ selected ] ) : std::string{ search_buffer };

			if ( ImGui::Button( "Load", ImVec2{ button_width, 0.0f } ) && has_selection )
			{
				if ( config::registry::load( config_list[ selected ] ) )
				{
					settings::finalize_binds( );
				}
			}
			ImGui::SameLine( );
			if ( ImGui::Button( "Save", ImVec2{ button_width, 0.0f } ) )
			{
				if ( !save_name.empty( ) && config::registry::save( to_wide( save_name ) ) )
				{
					refresh = true;
				}
			}
			ImGui::SameLine( );
			if ( ImGui::Button( "Reset", ImVec2{ button_width, 0.0f } ) )
			{
				auto& registry = config::detail::get_registry( );
				for ( auto& field : registry.fields )
				{
					char key[ 12 ]{};
					std::snprintf( key, sizeof( key ), "%08x", field.key );
					if ( const auto it = registry.defaults.find( key ); it != registry.defaults.end( ) )
					{
						config::serial::json_to_field( *it, field );
					}
				}
				settings::finalize_binds( );
			}
			ImGui::SameLine( );
			const auto delete_label = confirm_delete ? "Confirm" : "Delete";
			if ( ImGui::Button( delete_label, ImVec2{ button_width, 0.0f } ) && has_selection )
			{
				if ( confirm_delete )
				{
					if ( config::registry::remove( config_list[ selected ] ) )
					{
						selected = -1;
						refresh = true;
					}
					confirm_delete = false;
				}
				else
				{
					confirm_delete = true;
				}
			}
			ImGui::SameLine( );
			if ( ImGui::Button( "Import", ImVec2{ button_width, 0.0f } ) )
			{
				const auto result = config::import_auto( paste_clipboard( ) );
				if ( result.success )
				{
					settings::finalize_binds( );
					std::wstring name;
					if ( !result.name.empty( ) )
					{
						name = to_wide( result.name );
					}
					else
					{
						SYSTEMTIME time{};
						GetLocalTime( &time );
						wchar_t generated_name[ 128 ]{};
						std::swprintf( generated_name, IM_ARRAYSIZE( generated_name ), L"import_%04d%02d%02d_%02d%02d%02d",
							time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond );
						name = generated_name;
					}

					if ( !name.empty( ) )
					{
						config::registry::save( name );
					}
					refresh = true;
				}
			}
			ImGui::SameLine( );
			if ( ImGui::Button( "Export", ImVec2{ button_width, 0.0f } ) )
			{
				const auto code = config::export_share_words( save_name );
				if ( !code.empty( ) )
				{
					copy_clipboard( code );
				}
			}
}

} // namespace rendering
