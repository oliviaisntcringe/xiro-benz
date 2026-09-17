#include <pch/pch.hpp>
#include <core/features/features.hpp>
#include <core/settings.hpp>
#include <utilities/memory/memory.hpp>
#include <utilities/steam/steam.hpp>
#include <utilities/diag.hpp>
#include <external/xdraw/xdraw.hpp>
#include <core/resources/logos/logo.hpp>

#include <external/imgui/imgui.h>
#include <external/imgui/backends/imgui_impl_dx11.h>
#include <external/imgui/backends/imgui_impl_win32.h>
#include <core/rendering/rendering.hpp>

#include "../imgui_menu.hpp"
#include "retro_style.hpp"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam );

namespace rendering {
	namespace {
	struct backdrop_particle
		{
			ImVec2 origin{};
			float speed{};
			float phase{};
			float drift{};
			bool skull{};
		};

		struct nav_item
		{
			const char* label;
			const char* glyph;
			int tab;
			int visual_section;
		};

		static constexpr nav_item k_nav_items[ 8 ]{
			{ "Rage", "R", 3, -1 },
			{ "Legit", "L", 2, -1 },
			{ "Player", "P", 0, 0 },
			{ "Visuals", "V", 1, 7 },
			{ "Misc", "M", 4, -1 },
			{ "Skins", "S", 5, -1 },
			{ "Person", "N", 6, -1 },
			{ "Config", "C", 7, -1 }
		};

		int active_nav_index( int tab )
		{
			for ( auto i = 0; i < IM_ARRAYSIZE( k_nav_items ); ++i )
			{
				if ( k_nav_items[ i ].tab == tab )
				{
					return i;
				}
			}
			return 0;
		}

		void draw_ascii_wordmark( ImDrawList* draw, const ImVec2& pos, ImFont* font )
		{
			if ( !draw )
			{
				return;
			}

			static constexpr auto title = "trida.benz";
			const auto* render_font = font ? font : ImGui::GetFont( );
			constexpr auto font_size = 25.0f;
			const auto shadow = IM_COL32( 141, 74, 176, 110 );
			const auto green_dim = IM_COL32( 119, 200, 74, 95 );
			draw->AddText( render_font, font_size, ImVec2{ pos.x + 2.0f, pos.y + 1.0f }, shadow, title );
			draw->AddText( render_font, font_size, ImVec2{ pos.x - 1.0f, pos.y }, green_dim, title );
			draw->AddText( render_font, font_size, pos, IM_COL32( 160, 238, 102, 255 ), title );

			const auto text_width = ImGui::CalcTextSize( title ).x;
			const auto phase = ImGui::GetTime( ) * 4.0f;
			for ( auto i = 0; i < 3; ++i )
			{
				const auto y = pos.y + 6.0f + static_cast< float >( i ) * 7.0f;
				const auto wave = std::sin( phase + static_cast< float >( i ) * 1.7f ) * 3.0f;
				draw->AddLine(
					ImVec2{ pos.x + wave, y },
					ImVec2{ pos.x + text_width - wave, y },
					IM_COL32( 119, 200, 74, 42 ),
					1.0f
				);
			}
		}

		void draw_texture_contain(
			ImDrawList* draw,
			ID3D11ShaderResourceView* texture,
			int texture_width,
			int texture_height,
			const ImVec2& center,
			float max_size,
			ImU32 tint )
		{
			if ( !draw || !texture || texture_width <= 0 || texture_height <= 0 || max_size <= 0.0f )
			{
				return;
			}

			const auto aspect = static_cast< float >( texture_width ) / static_cast< float >( texture_height );
			auto width = max_size;
			auto height = max_size;
			if ( aspect > 1.0f )
			{
				height = max_size / aspect;
			}
			else
			{
				width = max_size * aspect;
			}

			const auto half_size = ImVec2{ width * 0.5f, height * 0.5f };
			draw->AddImage(
				reinterpret_cast< ImTextureID >( texture ),
				ImVec2{ center.x - half_size.x, center.y - half_size.y },
				ImVec2{ center.x + half_size.x, center.y + half_size.y },
				ImVec2{ 0.0f, 0.0f },
				ImVec2{ 1.0f, 1.0f },
				tint
			);
		}

		void draw_imgui_backdrop(
			ImGuiViewport* viewport,
			ID3D11ShaderResourceView* logo_texture,
			int logo_width,
			int logo_height )
		{
			if ( !viewport )
			{
				return;
			}

			static std::array<backdrop_particle, 48> particles{};
			static bool initialized{};
			const auto width = viewport->WorkSize.x;
			const auto height = viewport->WorkSize.y;
			if ( !initialized )
			{
				for ( auto i = 0u; i < particles.size( ); ++i )
				{
					auto& particle = particles[ i ];
					particle.origin.x = std::fmod( 83.0f + i * 137.0f, std::max( 1.0f, width - 40.0f ) ) + 20.0f;
					particle.origin.y = std::fmod( 41.0f + i * 89.0f, std::max( 1.0f, height ) );
					particle.speed = 15.0f + static_cast< float >( i % 7 ) * 5.0f;
					particle.phase = static_cast< float >( i ) * 0.7f;
					particle.drift = 7.0f + static_cast< float >( i % 4 ) * 3.0f;
					particle.skull = ( i % 5 ) == 0;
				}
				initialized = true;
			}

			const auto dt = ImGui::GetIO( ).DeltaTime;
			auto* draw = ImGui::GetBackgroundDrawList( );
			const auto viewport_max = ImVec2{
				viewport->WorkPos.x + viewport->WorkSize.x,
				viewport->WorkPos.y + viewport->WorkSize.y
			};
			draw->AddRectFilled( viewport->WorkPos, viewport_max, IM_COL32( 0, 0, 0, 122 ) );

			static constexpr const char* flower_symbols[ 6 ]{ " .-. ", "( * )", "<.*.>", "{ o }", "\\|/", "(_|_)" };
			static constexpr const char* skull_symbols[ 6 ]{ " .-. ", "(o o)", "[o_o]", "/xxx\\", "| ^ |", "\\___/" };
			for ( auto& particle : particles )
			{
				particle.origin.y += particle.speed * dt;
				particle.phase += dt;
				if ( particle.origin.y > height + 24.0f )
				{
					particle.origin.y = -24.0f;
				}

				const auto x = particle.origin.x + std::sin( particle.phase ) * particle.drift;
				const auto glyph_index = static_cast< int >( particle.phase * 2.0f ) % 6;
				const auto glyph = particle.skull ? skull_symbols[ glyph_index ] : flower_symbols[ glyph_index ];
				const auto color = particle.skull ? IM_COL32( 119, 200, 74, 105 ) : IM_COL32( 119, 200, 74, 82 );
				draw->AddText( ImVec2{ std::floor( viewport->WorkPos.x + x ), std::floor( viewport->WorkPos.y + particle.origin.y ) }, color, glyph );
			}

			const auto center = ImVec2{
				viewport->WorkPos.x + viewport->WorkSize.x * 0.5f,
				viewport->WorkPos.y + viewport->WorkSize.y * 0.5f
			};
			if ( logo_texture )
			{
				draw_texture_contain(
					draw,
					logo_texture,
					logo_width,
					logo_height,
					ImVec2{ center.x, center.y - 10.0f },
					180.0f,
					IM_COL32( 119, 200, 74, 30 )
				);
			}
			else
			{
				static constexpr auto fallback_logo = "trida.benz";
				const auto fallback_size = ImGui::CalcTextSize( fallback_logo );
				draw->AddText(
					ImVec2{ center.x - fallback_size.x * 0.5f, center.y - fallback_size.y * 0.5f },
					IM_COL32( 119, 200, 74, 48 ),
					fallback_logo
				);
			}
			draw->AddLine(
				ImVec2{ viewport->WorkPos.x + width * 0.22f, center.y + 24.0f },
				ImVec2{ viewport->WorkPos.x + width * 0.78f, center.y + 24.0f },
				IM_COL32( 119, 200, 74, 34 ),
				1.0f
			);
		}

		void draw_animated_window_separator( float height = 2.0f )
		{
			auto* draw = ImGui::GetWindowDrawList( );
			const auto start = ImGui::GetCursorScreenPos( );
			const auto width = ImGui::GetContentRegionAvail( ).x;
			const auto time = ImGui::GetTime( );
			constexpr auto segment_width = 10.0f;
			const auto segment_count = std::max( 1, static_cast< int >( std::ceil( width / segment_width ) ) );

			for ( auto i = 0; i < segment_count; ++i )
			{
				const auto x0 = start.x + width * static_cast< float >( i ) / segment_count;
				const auto x1 = start.x + width * static_cast< float >( i + 1 ) / segment_count;
				const auto wave = std::sin( time * 3.0f - static_cast< float >( i ) * 0.22f ) * 0.5f + 0.5f;
				const auto red = static_cast< int >( 42.0f + wave * 25.0f );
				const auto green = static_cast< int >( 112.0f + wave * 88.0f );
				const auto blue = static_cast< int >( 38.0f + wave * 42.0f );
				const auto alpha = static_cast< int >( 155.0f + wave * 75.0f );
				draw->AddRectFilled(
					ImVec2{ x0, start.y },
					ImVec2{ x1 + 0.5f, start.y + height },
					IM_COL32( red, green, blue, alpha )
				);
			}

			ImGui::Dummy( ImVec2{ 0.0f, height + 7.0f } );
		}

		const char* menu_key_name( int key )
		{
			switch ( key )
			{
			case VK_INSERT: return "Insert";
			case VK_DELETE: return "Delete";
			case VK_HOME: return "Home";
			case VK_END: return "End";
			case VK_F1: return "F1";
			case VK_F2: return "F2";
			case VK_F3: return "F3";
			case VK_F4: return "F4";
			case VK_F5: return "F5";
			case VK_F6: return "F6";
			case VK_F7: return "F7";
			case VK_F8: return "F8";
			case VK_F9: return "F9";
			case VK_F10: return "F10";
			case VK_F11: return "F11";
			case VK_F12: return "F12";
			default: return "Custom";
			}
		}

		void draw_overlay_frame( ImDrawList* draw, const ImVec2& min, const ImVec2& max )
		{
			draw->AddRectFilled( min, max, IM_COL32( 23, 23, 23, 242 ), 0.0f );
			draw->AddRect( min, max, IM_COL32( 96, 96, 96, 240 ), 0.0f, 0, 1.0f );
			draw->AddRectFilled( ImVec2{ min.x, max.y - 1.0f }, max, IM_COL32( 63, 111, 53, 255 ) );
		}

		void draw_overlay_header( ImDrawList* draw, const ImVec2& min, float width, const char* title, int count = -1 )
		{
			const auto green = ImGui::ColorConvertFloat4ToU32( rendering::retro::accent_green );
			const auto purple = ImGui::ColorConvertFloat4ToU32( rendering::retro::accent_purple );
			const auto green_dim = ImGui::ColorConvertFloat4ToU32( rendering::retro::accent_green_dim );
			draw->AddText( ImVec2{ min.x + 12.0f, min.y + 9.0f }, green, std::format( "[ {} ]", title ).c_str( ) );
			if ( count >= 0 )
			{
				draw->AddText( ImVec2{ min.x + width - 34.0f, min.y + 9.0f }, purple, std::format( "{:02}", count ).c_str( ) );
			}
			draw->AddLine( ImVec2{ min.x + 12.0f, min.y + 29.0f }, ImVec2{ min.x + width - 12.0f, min.y + 29.0f }, green_dim, 1.0f );
		}
	}

	bool imgui_menu::initialize( IDXGISwapChain* swap_chain, HWND window )
	{
		if ( this->m_initialized )
		{
			return true;
		}

		ID3D11Device* device{};
		if ( !swap_chain || FAILED( swap_chain->GetDevice( __uuidof( ID3D11Device ), reinterpret_cast< void** >( &device ) ) ) )
		{
			return false;
		}

		ID3D11DeviceContext* context{};
		device->GetImmediateContext( &context );
		IMGUI_CHECKVERSION( );
		ImGui::CreateContext( );
		auto& io = ImGui::GetIO( );
		const auto font_path = std::filesystem::path{ "C:\\Windows\\Fonts\\segoeui.ttf" };
		if ( std::filesystem::exists( font_path ) )
		{
			io.Fonts->AddFontFromFileTTF( font_path.string( ).c_str( ), 16.0f, nullptr, io.Fonts->GetGlyphRangesCyrillic( ) );
		}
		const std::array<std::filesystem::path, 3> mono_font_paths{
			std::filesystem::path{ "C:\\Windows\\Fonts\\CascadiaMono.ttf" },
			std::filesystem::path{ "C:\\Windows\\Fonts\\consola.ttf" },
			std::filesystem::path{ "C:\\Windows\\Fonts\\lucon.ttf" }
		};
		for ( const auto& mono_font_path : mono_font_paths )
		{
			if ( std::filesystem::exists( mono_font_path ) )
			{
				this->m_mono_font = io.Fonts->AddFontFromFileTTF(
					mono_font_path.string( ).c_str( ), 14.0f, nullptr, io.Fonts->GetGlyphRangesCyrillic( )
				);
				break;
			}
		}
		auto& style = ImGui::GetStyle( );
		rendering::retro::apply_rifk7_palette( style );
		io.IniFilename = nullptr;

		if ( !ImGui_ImplWin32_Init( window ) || !ImGui_ImplDX11_Init( device, context ) )
		{
			ImGui::DestroyContext( );
			if ( context )
			{
				context->Release( );
			}
			if ( device )
			{
				device->Release( );
			}
			return false;
		}

		this->m_logo_width = 0;
		this->m_logo_height = 0;
		this->m_logo = xdraw::load_texture(
			std::span<const std::byte>{ reinterpret_cast< const std::byte* >( logo ), logo_size },
			&this->m_logo_width,
			&this->m_logo_height
		);
		if ( !this->m_logo )
		{
			this->m_logo_width = 0;
			this->m_logo_height = 0;
		}

		this->m_initialized = true;
		return true;
	}

	void imgui_menu::loading_begin_check( const char* label ) noexcept
	{
		const auto index = this->m_loading_check_count.load( std::memory_order_relaxed );
		if ( index < 0 || index >= k_loading_check_capacity )
		{
			return;
		}

		this->m_loading_labels[ index ] = label ? label : "unnamed check";
		this->m_loading_check_states[ index ].store( 0, std::memory_order_release );
		this->m_loading_current_check.store( index, std::memory_order_release );
		this->m_loading_check_count.store( index + 1, std::memory_order_release );
	}

	void imgui_menu::loading_check_result( int result ) noexcept
	{
		const auto index = this->m_loading_current_check.load( std::memory_order_acquire );
		if ( index < 0 || index >= k_loading_check_capacity )
		{
			return;
		}

		this->m_loading_check_states[ index ].store( result > 0 ? 1 : result < 0 ? -1 : 2, std::memory_order_release );
	}

	void imgui_menu::loading_failed( const char* reason ) noexcept
	{
		this->loading_check_result( -1 );
		this->m_loading_error.store( reason ? reason : "unknown initialization failure", std::memory_order_release );
		this->m_loading_state.store( 2, std::memory_order_release );
	}

	void imgui_menu::loading_complete( ) noexcept
	{
		this->loading_check_result( 1 );
		this->m_loading_state.store( 1, std::memory_order_release );
	}

	void imgui_menu::draw_loading_screen( const ImGuiViewport* viewport )
	{
		if ( !viewport )
		{
			return;
		}

		const auto dt = std::clamp( ImGui::GetIO( ).DeltaTime, 0.0f, 0.1f );
		this->m_loading_elapsed += dt;
		const auto elapsed = this->m_loading_elapsed;
		const auto state = this->m_loading_state.load( std::memory_order_acquire );
		const auto failed = state == 2;
		const auto fade = state == 1 && elapsed > 3.6f
			? 1.0f - std::clamp( ( elapsed - 3.6f ) / 0.8f, 0.0f, 1.0f )
			: 1.0f;
		const auto alpha = static_cast< int >( 236.0f * fade );
		const auto center = ImVec2{
			viewport->WorkPos.x + viewport->WorkSize.x * 0.5f,
			viewport->WorkPos.y + viewport->WorkSize.y * 0.40f
		};
		auto* draw = ImGui::GetForegroundDrawList( );
		draw->AddRectFilled(
			viewport->WorkPos,
			ImVec2{ viewport->WorkPos.x + viewport->WorkSize.x, viewport->WorkPos.y + viewport->WorkSize.y },
			IM_COL32( 3, 6, 8, alpha )
		);

		static constexpr const char* glyphs[ 10 ]{ ".", "+", "x", "*", "o", "#", "@", "%", ":", "=" };
		const auto spin = elapsed * 2.7f;
		const auto wormhole_progress = std::clamp( elapsed / 2.35f, 0.0f, 1.0f );
		for ( auto i = 0; i < 96; ++i )
		{
			const auto t = static_cast< float >( i ) / 95.0f;
			const auto angle = t * 6.2831853f * 4.0f + spin - t * 1.4f;
			const auto radius = 9.0f + t * t * 190.0f;
			const auto wobble = std::sin( elapsed * 4.0f + t * 18.0f ) * 4.0f;
			const auto x = center.x + std::cos( angle ) * ( radius + wobble );
			const auto y = center.y + std::sin( angle ) * ( radius + wobble ) * 0.62f;
			const auto glyph = glyphs[ ( i + static_cast< int >( elapsed * 12.0f ) ) % std::size( glyphs ) ];
			const auto base_alpha = static_cast< int >( 38.0f + 172.0f * ( 1.0f - t ) * wormhole_progress );
			const auto color = failed
				? IM_COL32( 224, 88, 92, base_alpha )
				: IM_COL32( 119, 200, 74, base_alpha );
			draw->AddText( ImVec2{ x, y }, color, glyph );
		}

		const auto ring_phase = std::sin( elapsed * 3.4f ) * 0.5f + 0.5f;
		draw->AddCircle( center, 18.0f + ring_phase * 8.0f, failed ? IM_COL32( 224, 88, 92, 190 ) : IM_COL32( 119, 200, 74, 190 ), 48, 2.0f );
		draw->AddCircle( center, 8.0f + ring_phase * 3.0f, IM_COL32( 185, 224, 128, 210 ), 32, 1.0f );

		if ( !failed && elapsed > 1.85f && elapsed < 3.15f )
		{
			const auto blast = std::clamp( ( elapsed - 1.85f ) / 0.85f, 0.0f, 1.0f );
			const auto flash_alpha = static_cast< int >( 100.0f * ( 1.0f - blast ) );
			draw->AddCircleFilled( center, 26.0f * ( 1.0f - blast * 0.65f ), IM_COL32( 198, 238, 138, flash_alpha ) );
			for ( auto i = 0; i < 28; ++i )
			{
				const auto angle = static_cast< float >( i ) / 28.0f * 6.2831853f + elapsed * 1.2f;
				const auto inner = 25.0f + blast * 18.0f;
				const auto outer = inner + blast * ( 80.0f + static_cast< float >( i % 5 ) * 18.0f );
				draw->AddLine(
					ImVec2{ center.x + std::cos( angle ) * inner, center.y + std::sin( angle ) * inner },
					ImVec2{ center.x + std::cos( angle ) * outer, center.y + std::sin( angle ) * outer },
					IM_COL32( 119, 200, 74, static_cast< int >( 190.0f * ( 1.0f - blast ) ) ),
					1.5f
				);
			}
		}

		const auto logo_alpha = static_cast< int >( 255.0f * std::clamp( ( elapsed - 1.15f ) / 0.55f, 0.0f, 1.0f ) * fade );
		if ( logo_alpha > 0 )
		{
			const auto logo_tint = failed ? IM_COL32( 242, 112, 116, logo_alpha ) : IM_COL32( 185, 224, 128, logo_alpha );
			const auto has_logo = this->m_logo && this->m_logo_width > 0 && this->m_logo_height > 0;
			if ( has_logo )
			{
				draw_texture_contain(
					draw,
					this->m_logo.Get( ),
					this->m_logo_width,
					this->m_logo_height,
					ImVec2{ center.x, center.y + 2.0f },
					96.0f + ring_phase * 8.0f,
					logo_tint
				);
			}

			static constexpr auto logo_label = "trida.benz";
			const auto logo_text_size = 34.0f + ring_phase * 2.0f;
			const auto logo_dimensions = ImGui::CalcTextSize( logo_label );
			const auto logo_y = center.y + ( has_logo ? 64.0f : 48.0f );
			draw->AddText(
				ImGui::GetFont(),
				logo_text_size,
				ImVec2{ center.x - logo_dimensions.x * 0.5f, logo_y },
				logo_tint,
				logo_label
			);
			const auto signature = failed ? ">signature velocity_init: FAIL" : state == 1 ? ">signature velocity_init: OK" : ">signature velocity_init: ...";
			const auto signature_dimensions = ImGui::CalcTextSize( signature );
			draw->AddText(
				ImVec2{ center.x - signature_dimensions.x * 0.5f, logo_y + 38.0f },
				failed ? IM_COL32( 242, 112, 116, logo_alpha ) : IM_COL32( 156, 178, 164, logo_alpha ),
				signature
			);
		}

		const auto check_count = std::clamp( this->m_loading_check_count.load( std::memory_order_acquire ), 0, k_loading_check_capacity );
		const auto panel_width = std::min( 650.0f, std::max( 300.0f, viewport->WorkSize.x - 48.0f ) );
		const auto panel_height = std::min( 330.0f, std::max( 170.0f, viewport->WorkSize.y * 0.34f ) );
		const auto panel_x = viewport->WorkPos.x + ( viewport->WorkSize.x - panel_width ) * 0.5f;
		const auto panel_y = std::min( center.y + 132.0f, viewport->WorkPos.y + viewport->WorkSize.y - panel_height - 24.0f );
		draw->AddRectFilled( ImVec2{ panel_x, panel_y }, ImVec2{ panel_x + panel_width, panel_y + panel_height }, IM_COL32( 12, 19, 23, static_cast< int >( 238.0f * fade ) ), 8.0f );
		draw->AddRect( ImVec2{ panel_x, panel_y }, ImVec2{ panel_x + panel_width, panel_y + panel_height }, IM_COL32( 72, 92, 98, static_cast< int >( 230.0f * fade ) ), 8.0f, 0, 1.0f );
		draw->AddText( ImVec2{ panel_x + 18.0f, panel_y + 14.0f }, IM_COL32( 119, 200, 74, static_cast< int >( 255.0f * fade ) ), failed ? "INITIALIZATION ABORTED" : "INITIALIZING trida.benz" );

		const auto row_height = 17.0f;
		const auto visible_rows = std::max( 1, static_cast< int >( ( panel_height - 62.0f ) / row_height ) );
		const auto first_row = std::max( 0, check_count - visible_rows );
		for ( auto i = first_row; i < check_count; ++i )
		{
			const auto check_state = this->m_loading_check_states[ i ].load( std::memory_order_acquire );
			const auto marker = check_state < 0 ? "!!" : check_state > 0 ? "OK" : check_state == 2 ? "--" : "..";
			const auto color = check_state < 0 ? IM_COL32( 242, 112, 116, 255 ) : check_state == 2 ? IM_COL32( 235, 193, 93, 255 ) : check_state > 0 ? IM_COL32( 185, 224, 128, 255 ) : IM_COL32( 156, 178, 164, 255 );
			const auto y = panel_y + 42.0f + static_cast< float >( i - first_row ) * row_height;
			draw->AddText( ImVec2{ panel_x + 18.0f, y }, color, marker );
			draw->AddText( ImVec2{ panel_x + 52.0f, y }, IM_COL32( 204, 214, 213, 230 ), this->m_loading_labels[ i ] ? this->m_loading_labels[ i ] : "unnamed check" );
		}

		if ( failed )
		{
			const auto* reason = this->m_loading_error.load( std::memory_order_acquire );
			draw->AddText( ImVec2{ panel_x + 18.0f, panel_y + panel_height - 26.0f }, IM_COL32( 242, 112, 116, 255 ), reason ? reason : "unknown initialization failure" );
		}
	}

	void imgui_menu::shutdown( )
	{
		if ( !this->m_initialized )
		{
			return;
		}

		ImGui_ImplDX11_Shutdown( );
		ImGui_ImplWin32_Shutdown( );
		ImGui::DestroyContext( );
		this->m_logo.Reset( );
		this->m_logo_width = 0;
		this->m_logo_height = 0;
		this->m_avatar.Reset( );
		this->m_initialized = false;
	}

	void imgui_menu::try_load_avatar( )
	{
		if ( this->m_avatar )
		{
			return;
		}

		this->m_avatar_retry_delay -= ImGui::GetIO( ).DeltaTime;
		if ( this->m_avatar_retry_delay > 0.0f )
		{
			return;
		}
		this->m_avatar_retry_delay = 1.0f;

		const auto steam_id = steam::user::get_steam_id( );
		if ( !steam_id )
		{
			return;
		}

		const auto image = steam::friends::get_medium_friend_avatar( steam_id );
		if ( image <= 0 )
		{
			return;
		}

		std::uint32_t width{}, height{};
		if ( !steam::utils::get_image_size( image, &width, &height ) || !width || !height )
		{
			return;
		}

		if ( width > 512 || height > 512 || width > std::numeric_limits<std::uint32_t>::max( ) / height / 4 )
		{
			return;
		}

		std::vector<std::uint8_t> rgba( width * height * 4 );
		if ( !steam::utils::get_image_rgba( image, rgba.data( ), static_cast< int >( rgba.size( ) ) ) )
		{
			return;
		}

		this->m_avatar = xdraw::create_srv_from_rgba( rgba.data( ), static_cast< int >( width ), static_cast< int >( height ) );
	}

	void imgui_menu::begin_frame( )
	{
		if ( !this->m_initialized )
		{
			return;
		}

		ImGui_ImplDX11_NewFrame( );
		ImGui_ImplWin32_NewFrame( );
		ImGui::NewFrame( );
	}

	void imgui_menu::draw( )
	{
		if ( !this->m_initialized )
		{
			return;
		}

		const auto loading_state = this->m_loading_state.load( std::memory_order_acquire );
		const auto loading_visible = loading_state != 1 || this->m_loading_elapsed < 4.4f;
		if ( loading_visible )
		{
			this->draw_loading_screen( ImGui::GetMainViewport( ) );
			return;
		}

		if ( !this->m_open )
		{
			return;
		}

		this->try_load_avatar( );
		const auto viewport = ImGui::GetMainViewport( );
		draw_imgui_backdrop( viewport, this->m_logo.Get( ), this->m_logo_width, this->m_logo_height );

		const auto workspace_width = std::clamp( viewport->WorkSize.x - 80.0f, 720.0f, 1260.0f );
		const auto workspace_height = std::clamp( viewport->WorkSize.y - 96.0f, 520.0f, 860.0f );
		const auto max_workspace_width = std::max( 720.0f, viewport->WorkSize.x - 24.0f );
		const auto max_workspace_height = std::max( 520.0f, viewport->WorkSize.y - 90.0f );
		const auto workspace_pos = ImVec2{
			viewport->WorkPos.x + ( viewport->WorkSize.x - workspace_width ) * 0.5f,
			viewport->WorkPos.y + 34.0f
		};
		ImGui::SetNextWindowPos( workspace_pos, ImGuiCond_FirstUseEver );
		ImGui::SetNextWindowSize( ImVec2{ workspace_width, workspace_height }, ImGuiCond_FirstUseEver );
		ImGui::SetNextWindowSizeConstraints(
			ImVec2{ 720.0f, 520.0f },
			ImVec2{ max_workspace_width, max_workspace_height } );
		ImGui::Begin( "trida.benz // WORKSPACE", nullptr,
			ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoScrollbar );
		draw_animated_window_separator( );
		const auto header_pos = ImGui::GetCursorScreenPos( );
		const auto header_width = ImGui::GetContentRegionAvail( ).x;
		const auto header_max = ImVec2{ header_pos.x + header_width, header_pos.y + 58.0f };
		rendering::retro::draw_retro_header( ImGui::GetWindowDrawList( ), header_pos, header_max );
		draw_ascii_wordmark(
			ImGui::GetWindowDrawList( ),
			ImVec2{ header_pos.x + 18.0f, header_pos.y + 9.0f },
			this->m_mono_font
		);
		ImGui::SetCursorScreenPos( ImVec2{ header_pos.x + 18.0f, header_pos.y + 35.0f } );
		ImGui::TextColored( rendering::retro::text_muted, "operator workspace // %s", k_nav_items[ active_nav_index( this->m_tab ) ].label );
		ImGui::SetCursorScreenPos( ImVec2{ header_pos.x, header_max.y + 10.0f } );

		const auto body_height = std::max( 120.0f, ImGui::GetContentRegionAvail( ).y );
		constexpr auto rail_width = 86.0f;
		ImGui::BeginChild( "##imgui_left_rail", ImVec2{ rail_width, body_height }, true, ImGuiWindowFlags_NoScrollbar );
		this->draw_left_rail( rail_width, body_height );
		ImGui::EndChild( );
		ImGui::SameLine( 0.0f, 10.0f );
		ImGui::BeginChild( "##imgui_main_panel", ImVec2{ 0.0f, body_height }, false );
		this->draw_panel( );
		ImGui::EndChild( );
		ImGui::End( );

		static bool profile_actions_open{};
		const auto persona = steam::friends::get_persona_name( );
		const auto profile_name = persona && persona[ 0 ] ? persona : "steam user";
		const auto profile_width = std::clamp( ImGui::CalcTextSize( profile_name ).x + 116.0f, 240.0f, 420.0f );
		ImGui::SetNextWindowPos(
			ImVec2{ viewport->WorkPos.x + ( viewport->WorkSize.x - profile_width ) * 0.5f,
				viewport->WorkPos.y + viewport->WorkSize.y - 66.0f },
			ImGuiCond_Always
		);
		ImGui::SetNextWindowSize( ImVec2{ profile_width, 48.0f }, ImGuiCond_Always );
		ImGui::Begin( "##imgui_profile_hud", nullptr,
			ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize
			| ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing );
		draw_animated_window_separator( 1.0f );
		if ( ImGui::IsKeyPressed( ImGuiKey_F9, false ) )
		{
			profile_actions_open = !profile_actions_open;
		}
		if ( this->m_avatar )
		{
			ImGui::Image( reinterpret_cast< ImTextureID >( this->m_avatar.Get( ) ), ImVec2{ 28.0f, 28.0f } );
			ImGui::SameLine( );
		}
		ImGui::Text( "%s", profile_name );
		ImGui::SameLine( );
		ImGui::SetCursorPosX( profile_width - 58.0f );
		if ( ImGui::Button( "...", ImVec2{ 42.0f, 28.0f } ) )
		{
			profile_actions_open = !profile_actions_open;
		}
		if ( profile_actions_open )
		{
			ImGui::SetNextWindowPos(
				ImVec2{ viewport->WorkPos.x + ( viewport->WorkSize.x + profile_width ) * 0.5f - 190.0f,
					viewport->WorkPos.y + viewport->WorkSize.y - 146.0f },
				ImGuiCond_Always
			);
			ImGui::Begin( "##imgui_profile_actions", &profile_actions_open,
				ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize
				| ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize );
			draw_animated_window_separator( );
			ImGui::TextColored( rendering::retro::accent_green, "PROFILE / INPUT" );
			ImGui::Separator( );
			ImGui::Text( "Open / close: %s", menu_key_name( settings::g_misc.menu_key.value ) );
			if ( this->m_rebinding_menu_key )
			{
				ImGui::TextColored( rendering::retro::accent_purple, "Press any key to bind..." );
				if ( ImGui::Button( "Cancel rebind", ImVec2{ -1.0f, 0.0f } ) )
				{
					this->m_rebinding_menu_key = false;
				}
			}
			else if ( ImGui::Button( "Rebind menu key", ImVec2{ -1.0f, 0.0f } ) )
			{
				this->m_rebinding_menu_key = true;
			}
			ImGui::TextDisabled( "F9 toggles this panel" );
			ImGui::End( );
		}
		ImGui::End( );

	}

	void imgui_menu::draw_overlays( )
	{
		if ( !this->gameplay_ready( ) )
		{
			return;
		}

		g_widgets.draw( );
		auto* draw = ImGui::GetForegroundDrawList( );
		const auto viewport = ImGui::GetMainViewport( );
		if ( !draw || !viewport )
		{
			return;
		}
		this->draw_spectators( draw, viewport );
		const auto& velocity = settings::g_misc.m_hud.m_velocity;
		const auto& local_status = settings::g_misc.m_hud.m_local_status;
		const auto local = systems::g_local.get( );
		if ( ( velocity.counter.value || velocity.chart.value ) && local.is_valid( ) )
		{
			const auto speed = memory::safe_read<math::vector3>( local.pawn + SCHEMA( "C_BaseEntity", "m_vecAbsVelocity"_hash ) ).value_or( math::vector3{} ).length_2d( );
			const auto width = velocity.chart.value ? std::clamp( velocity.chart_width.value, 148.0f, 300.0f ) : 150.0f;
			const auto height = velocity.chart.value ? std::clamp( velocity.chart_height.value + 48.0f, 72.0f, 132.0f ) : 62.0f;
			const auto min = ImVec2{ viewport->WorkPos.x + viewport->WorkSize.x * 0.5f - width * 0.5f, viewport->WorkPos.y + viewport->WorkSize.y - velocity.bottom_offset.value - height };
			const auto max = ImVec2{ min.x + width, min.y + height };
			draw_overlay_frame( draw, min, max );
			draw_overlay_header( draw, min, width, "VELOCITY" );
			draw->AddText( ImVec2{ min.x + 12.0f, min.y + 39.0f }, IM_COL32( 228, 228, 228, 255 ), std::format( "{:03.0f} u/s", speed ).c_str( ) );
			if ( velocity.chart.value )
			{
				const auto chart_min = ImVec2{ min.x + 12.0f, min.y + 58.0f };
				const auto chart_max = ImVec2{ max.x - 12.0f, max.y - 10.0f };
				draw->AddRect( chart_min, chart_max, IM_COL32( 96, 96, 96, 235 ), 0.0f, 0, 1.0f );
				draw->AddRectFilled( chart_min, ImVec2{ chart_min.x + ( chart_max.x - chart_min.x ) * std::clamp( speed / 320.0f, 0.0f, 1.0f ), chart_max.y }, IM_COL32( 119, 200, 74, 180 ) );
			}
		}

		if ( local.is_valid( ) && local_status.enabled.value && ( local_status.health.value || local_status.ammo.value ) )
		{
			const auto health = std::clamp( memory::safe_read<int>( local.pawn + SCHEMA( "C_BaseEntity", "m_iHealth"_hash ) ).value_or( 0 ), 0, 100 );
			const auto weapon_services = memory::safe_read<std::uintptr_t>( local.pawn + SCHEMA( "C_BasePlayerPawn", "m_pWeaponServices"_hash ) ).value_or( 0 );
			const auto weapon_handle = weapon_services
				? memory::safe_read<std::uint32_t>( weapon_services + SCHEMA( "CPlayer_WeaponServices", "m_hActiveWeapon"_hash ) ).value_or( 0u )
				: 0u;
			const auto weapon = weapon_handle ? systems::g_entities.lookup( weapon_handle ) : 0ull;
			const auto weapon_vdata = weapon
				? memory::safe_read<std::uintptr_t>( weapon + SCHEMA( "C_BaseEntity", "m_nSubclassID"_hash ) + 0x8 ).value_or( 0 )
				: 0ull;
			const auto ammo = weapon ? memory::safe_read<int>( weapon + SCHEMA( "C_BasePlayerWeapon", "m_iClip1"_hash ) ).value_or( 0 ) : 0;
			const auto max_ammo = weapon_vdata ? memory::safe_read<int>( weapon_vdata + SCHEMA( "CBasePlayerWeaponVData", "m_iMaxClip1"_hash ) ).value_or( 0 ) : 0;
			constexpr auto card_width = 260.0f;
			const auto row_count = static_cast< int >( local_status.health.value ) + static_cast< int >( local_status.ammo.value && max_ammo > 0 );
			const auto card_height = 37.0f + static_cast< float >( row_count ) * 36.0f;
			const auto x = viewport->WorkPos.x + 24.0f;
			const auto y = viewport->WorkPos.y + viewport->WorkSize.y - local_status.bottom_offset.value - card_height;
			constexpr auto padding = 14.0f;
			constexpr auto bar_width = card_width - padding * 2.0f;
			constexpr auto bar_height = 7.0f;
			const auto health_color = ImColor( local_status.health_color.value.r, local_status.health_color.value.g, local_status.health_color.value.b, local_status.health_color.value.a );
			const auto ammo_color = ImColor( local_status.ammo_color.value.r, local_status.ammo_color.value.g, local_status.ammo_color.value.b, local_status.ammo_color.value.a );

			draw_overlay_frame( draw, ImVec2{ x, y }, ImVec2{ x + card_width, y + card_height } );
			draw_overlay_header( draw, ImVec2{ x, y }, card_width, "PLAYER STATUS" );

			if ( local_status.health.value )
			{
				const auto row_y = y + 36.0f;
				const auto health_text = std::format( "HEALTH  {:03}", health );
				draw->AddText( ImVec2{ x + padding, row_y + 3.0f }, IM_COL32( 228, 228, 228, 255 ), health_text.c_str( ) );
				draw->AddRectFilled( ImVec2{ x + padding, row_y + 20.0f }, ImVec2{ x + padding + bar_width, row_y + 20.0f + bar_height }, IM_COL32( 41, 41, 41, 255 ) );
				draw->AddRect( ImVec2{ x + padding, row_y + 20.0f }, ImVec2{ x + padding + bar_width, row_y + 20.0f + bar_height }, IM_COL32( 96, 96, 96, 235 ) );
				draw->AddRectFilled( ImVec2{ x + padding, row_y + 20.0f }, ImVec2{ x + padding + bar_width * health / 100.0f, row_y + 20.0f + bar_height }, health_color );
			}

			if ( local_status.ammo.value && max_ammo > 0 )
			{
				const auto row_index = local_status.health.value ? 1.0f : 0.0f;
				const auto row_y = y + 36.0f + row_index * 36.0f;
				const auto clamped_ammo = std::clamp( ammo, 0, max_ammo );
				const auto ammo_text = std::format( "AMMO    {:03}/{:03}", clamped_ammo, max_ammo );
				draw->AddText( ImVec2{ x + padding, row_y + 3.0f }, IM_COL32( 228, 228, 228, 255 ), ammo_text.c_str( ) );
				draw->AddRectFilled( ImVec2{ x + padding, row_y + 20.0f }, ImVec2{ x + padding + bar_width, row_y + 20.0f + bar_height }, IM_COL32( 41, 41, 41, 255 ) );
				draw->AddRect( ImVec2{ x + padding, row_y + 20.0f }, ImVec2{ x + padding + bar_width, row_y + 20.0f + bar_height }, IM_COL32( 96, 96, 96, 235 ) );
				draw->AddRectFilled( ImVec2{ x + padding, row_y + 20.0f }, ImVec2{ x + padding + bar_width * clamped_ammo / static_cast< float >( max_ammo ), row_y + 20.0f + bar_height }, ammo_color );
			}
		}

		const auto& feed = features::misc::g_other.killfeed( );
		float y = viewport->WorkPos.y + 48.0f;
		const auto now = std::chrono::duration<float>( std::chrono::steady_clock::now( ).time_since_epoch( ) ).count( );
		std::array<std::string, 6> feed_lines{};
		std::size_t feed_count{};
		for ( auto it = feed.rbegin( ); it != feed.rend( ) && feed_count < feed_lines.size( ); ++it )
		{
			if ( now - it->time > 6.0f )
			{
				continue;
			}
			feed_lines[ feed_count++ ] = std::format( "{}  {}{}  {}", it->attacker, it->weapon, it->headshot ? " [HS]" : "", it->victim );
		}
		if ( feed_count )
		{
			constexpr auto feed_width = 330.0f;
			constexpr auto feed_row_height = 22.0f;
			const auto feed_height = 37.0f + feed_row_height * static_cast< float >( feed_count );
			const auto feed_max_x = viewport->WorkPos.x + viewport->WorkSize.x - 272.0f;
			const ImVec2 feed_min{ feed_max_x - feed_width, y };
			const ImVec2 feed_max{ feed_max_x, y + feed_height };
			draw_overlay_frame( draw, feed_min, feed_max );
			draw_overlay_header( draw, feed_min, feed_width, "KILLFEED" );
			for ( std::size_t i = 0; i < feed_count; ++i )
			{
				const auto row_y = feed_min.y + 36.0f + feed_row_height * static_cast< float >( i );
				if ( i )
				{
					draw->AddLine( ImVec2{ feed_min.x + 12.0f, row_y }, ImVec2{ feed_max.x - 12.0f, row_y }, IM_COL32( 63, 63, 63, 220 ), 1.0f );
				}
				draw->AddText( ImVec2{ feed_min.x + 12.0f, row_y + 4.0f }, IM_COL32( 228, 228, 228, 255 ), feed_lines[ i ].c_str( ) );
			}
		}

	}

	void imgui_menu::draw_spectators( ImDrawList* draw, const ImGuiViewport* viewport )
	{
		const auto& setting = settings::g_esp.m_other.spectator_list;
		if ( !setting.value || !viewport )
		{
			return;
		}

		const auto spectators = features::esp::other::g_overlay.get_spectators( );
		if ( spectators.empty( ) )
		{
			return;
		}

		constexpr auto width = 244.0f;
		constexpr auto row_height = 23.0f;
		const auto height = 37.0f + row_height * static_cast< float >( spectators.size( ) );
		const ImVec2 min{ viewport->WorkPos.x + viewport->WorkSize.x - width - 14.0f, viewport->WorkPos.y + 48.0f };
		const ImVec2 max{ min.x + width, min.y + height };
		draw_overlay_frame( draw, min, max );
		draw_overlay_header( draw, min, width, "SPECTATORS", static_cast< int >( spectators.size( ) ) );

		for ( std::size_t i = 0; i < spectators.size( ); ++i )
		{
			const auto y = min.y + 36.0f + row_height * static_cast< float >( i );
			if ( i )
			{
				draw->AddLine( ImVec2{ min.x + 12.0f, y }, ImVec2{ max.x - 12.0f, y }, IM_COL32( 63, 63, 63, 220 ), 1.0f );
			}
			draw->AddRectFilled( ImVec2{ min.x + 12.0f, y + 8.0f }, ImVec2{ min.x + 15.0f, y + 16.0f }, IM_COL32( 119, 200, 74, 255 ) );
			draw->AddText( ImVec2{ min.x + 24.0f, y + 4.0f }, IM_COL32( 228, 228, 228, 255 ), spectators[ i ].name.c_str( ) );
		}
	}

	void imgui_menu::render( )
	{
		if ( !this->m_initialized )
		{
			return;
		}
		if ( !ImGui::GetCurrentContext( ) )
		{
			static std::atomic_bool reported{};
			if ( !reported.exchange( true, std::memory_order_relaxed ) )
			{
				diag::write( diag::level::error, "render: ImGui submit skipped because the context is missing" );
			}
			return;
		}

		ImGui::Render( );
		if ( auto* draw_data = ImGui::GetDrawData( ) )
		{
			ImGui_ImplDX11_RenderDrawData( draw_data );
		}
		else
		{
			static std::atomic_bool reported{};
			if ( !reported.exchange( true, std::memory_order_relaxed ) )
			{
				diag::write( diag::level::error, "render: ImGui produced no draw data" );
			}
		}
	}

	void imgui_menu::draw_left_rail( float width, float height )
	{
		auto* draw = ImGui::GetWindowDrawList( );
		const auto active = active_nav_index( this->m_tab );
		const auto content_width = std::max( 1.0f, width - 8.0f );
		const auto* glyph_font = this->m_mono_font ? this->m_mono_font : ImGui::GetFont( );
		const auto row_height = std::clamp( ( height - 24.0f ) / 8.0f, 48.0f, 60.0f );

		ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2{ 0.0f, 3.0f } );
		for ( auto i = 0; i < IM_ARRAYSIZE( k_nav_items ); ++i )
		{
			const auto& item = k_nav_items[ i ];
			const auto selected = i == active;
			ImGui::PushID( i + 700 );
			ImGui::PushStyleColor( ImGuiCol_Header, selected ? ImVec4{ 0.135f, 0.205f, 0.165f, 1.0f } : ImVec4{ 0.0f, 0.0f, 0.0f, 0.0f } );
			ImGui::PushStyleColor( ImGuiCol_HeaderHovered, ImVec4{ 0.185f, 0.285f, 0.205f, 0.95f } );
			ImGui::PushStyleColor( ImGuiCol_HeaderActive, ImVec4{ 0.235f, 0.380f, 0.245f, 1.0f } );

			if ( ImGui::Selectable( "##nav_item", selected, 0, ImVec2{ content_width, 34.0f } ) )
			{
				this->m_tab = item.tab;
				if ( item.visual_section >= 0 )
				{
					this->m_visual_section = item.visual_section;
				}
			}
			const auto item_min = ImGui::GetItemRectMin( );
			const auto item_max = ImGui::GetItemRectMax( );
			const auto glyph_width = ImGui::CalcTextSize( item.glyph ).x;
			draw->AddText(
				glyph_font,
				22.0f,
				ImVec2{ item_min.x + ( content_width - glyph_width ) * 0.5f, item_min.y + 5.0f },
				selected ? IM_COL32( 160, 238, 102, 255 ) : IM_COL32( 156, 178, 164, 220 ),
				item.glyph
			);
			if ( selected )
			{
				draw->AddRectFilled(
					ImVec2{ item_max.x - 3.0f, item_min.y + 4.0f },
					ImVec2{ item_max.x, item_max.y - 4.0f },
					IM_COL32( 119, 200, 74, 235 )
				);
			}
			ImGui::PopStyleColor( 3 );

			const auto label_width = ImGui::CalcTextSize( item.label ).x;
			ImGui::SetCursorPosX( std::max( 0.0f, ( width - label_width ) * 0.5f ) );
			ImGui::TextColored( selected ? rendering::retro::accent_green : rendering::retro::text_muted, "%s", item.label );
			ImGui::Dummy( ImVec2{ 0.0f, std::max( 0.0f, row_height - 34.0f - ImGui::GetTextLineHeight() ) } );
			ImGui::PopID( );
		}
		ImGui::PopStyleVar( );
	}

	void imgui_menu::draw_panel( )
	{
		static constexpr const char* tab_names[ 9 ]{ "Visuals", "World", "Legit", "Rage", "Misc", "Skins", "Personal", "Config", "Info" };
		const auto accent = rendering::retro::accent_green;
		const auto muted = rendering::retro::text_muted;
		const auto panel_width = ImGui::GetContentRegionAvail( ).x;

		auto draw_rule = [ ]( const ImVec4& color, float thickness = 1.0f )
		{
			auto* draw = ImGui::GetWindowDrawList( );
			const auto min = ImGui::GetCursorScreenPos( );
			const auto max = ImVec2{ min.x + ImGui::GetContentRegionAvail( ).x, min.y + thickness };
			draw->AddRectFilled( min, max, ImGui::ColorConvertFloat4ToU32( color ) );
			ImGui::Dummy( ImVec2{ 0.0f, thickness + 7.0f } );
		};

		auto draw_section_title = [ & ]( const char* title, const char* description )
		{
			if ( this->m_mono_font )
			{
				ImGui::PushFont( this->m_mono_font );
			}
			ImGui::TextColored( accent, "[ %s ]", title );
			if ( this->m_mono_font )
			{
				ImGui::PopFont( );
			}
			ImGui::SameLine( );
			ImGui::TextColored( muted, "%s", description );
			draw_rule( rendering::retro::accent_green_dim );
		};

		draw_section_title( tab_names[ this->m_tab ], "operator workspace / select a module" );

		const char* subtab_names[ 2 ]{};
		int subtab_count{};
		int selected_subtab{};
		if ( this->m_tab == 0 || this->m_tab == 1 )
		{
			subtab_names[ 0 ] = "Players";
			subtab_names[ 1 ] = "World";
			subtab_count = 2;
			selected_subtab = this->m_tab == 1;
		}
		else if ( this->m_tab == 2 || this->m_tab == 3 )
		{
			subtab_names[ 0 ] = "Legit";
			subtab_names[ 1 ] = "Rage";
			subtab_count = 2;
			selected_subtab = this->m_tab == 3;
		}
		else if ( this->m_tab == 4 || this->m_tab == 6 )
		{
			subtab_names[ 0 ] = "Misc";
			subtab_names[ 1 ] = "Personal";
			subtab_count = 2;
			selected_subtab = this->m_tab == 6;
		}

		if ( subtab_count )
		{
			ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2{ 6.0f, 6.0f } );
			const auto subtab_width = std::max( 110.0f, ( panel_width - 6.0f ) * 0.5f );
			for ( auto i = 0; i < subtab_count; ++i )
			{
				ImGui::PushID( i + 32 );
				const auto active = selected_subtab == i;
				ImGui::PushStyleColor( ImGuiCol_Header, active ? ImVec4{ 0.135f, 0.205f, 0.165f, 1.0f } : ImVec4{ 0.0f, 0.0f, 0.0f, 0.0f } );
				ImGui::PushStyleColor( ImGuiCol_HeaderHovered, ImVec4{ 0.185f, 0.285f, 0.205f, 0.95f } );
				ImGui::PushStyleColor( ImGuiCol_HeaderActive, ImVec4{ 0.235f, 0.380f, 0.245f, 1.0f } );
				if ( ImGui::Selectable( subtab_names[ i ], selected_subtab == i, 0, ImVec2{ subtab_width, 30.0f } ) )
				{
					if ( this->m_tab == 0 || this->m_tab == 1 )
					{
						this->m_tab = i == 0 ? 0 : 1;
						this->m_visual_section = i == 0 ? 0 : 7;
					}
					else if ( this->m_tab == 2 || this->m_tab == 3 )
					{
						this->m_tab = i == 0 ? 2 : 3;
					}
					else
					{
						this->m_tab = i == 0 ? 4 : 6;
					}
					selected_subtab = i;
				}
				if ( active )
				{
					const auto item_min = ImGui::GetItemRectMin( );
					const auto item_max = ImGui::GetItemRectMax( );
					ImGui::GetWindowDrawList( )->AddRectFilled(
						ImVec2{ item_min.x + 12.0f, item_max.y - 2.0f },
						ImVec2{ item_max.x - 12.0f, item_max.y },
						ImGui::ColorConvertFloat4ToU32( rendering::retro::accent_green ), 2.0f
					);
				}
				ImGui::PopStyleColor( 3 );
				ImGui::PopID( );
				if ( i + 1 < subtab_count )
				{
					ImGui::SameLine( );
				}
			}
			ImGui::PopStyleVar( );
			ImGui::Spacing( );
		}

		ImGui::PushStyleColor( ImGuiCol_ChildBg, ImVec4{ 0.075f, 0.075f, 0.075f, 0.98f } );
		ImGui::BeginChild( "##imgui_content", ImVec2{ 0.0f, -30.0f }, false, ImGuiWindowFlags_AlwaysVerticalScrollbar );
		if ( this->m_mono_font )
		{
			ImGui::PushFont( this->m_mono_font );
		}
        ImGui::TextDisabled( "module loaded // all controls are contained in this scroll surface" );
        switch ( this->m_tab )
        {
        case 0:
        case 1:
            this->draw_visuals_tab( );
            break;
        case 2:
            this->draw_legit_tab( );
            break;
        case 3:
            this->draw_rage_tab( );
            break;
        case 4:
            this->draw_misc_tab( );
            break;
        case 5:
            this->draw_skins_tab( );
            break;
        case 6:
            this->draw_personal_tab( );
            break;
        case 7:
            this->draw_config_tab( );
            break;
        default:
            ImGui::Text( "Info" );
            ImGui::TextDisabled( "Select a tab from the navigation rail." );
            break;
        }
        if ( this->m_mono_font )
        {
            ImGui::PopFont( );
        }
        ImGui::EndChild( );
        ImGui::PopStyleColor( );
    }

	bool imgui_menu::wndproc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
	{
		if ( !this->m_initialized || !this->m_open )
		{
			return false;
		}

		if ( this->m_rebinding_menu_key && msg == WM_KEYDOWN && !( lparam & ( 1 << 30 ) ) )
		{
			if ( wparam == VK_ESCAPE )
			{
				this->m_rebinding_menu_key = false;
				return true;
			}

			settings::g_misc.menu_key.value = static_cast< int >( wparam );
			this->m_rebinding_menu_key = false;
			return true;
		}

		if ( this->m_rebinding_setting && msg == WM_KEYDOWN && !( lparam & ( 1 << 30 ) ) )
		{
			if ( wparam == VK_ESCAPE )
			{
				this->m_rebinding_setting = nullptr;
				return true;
			}

			this->m_rebinding_setting->bind.key = static_cast< int >( wparam );
			this->m_rebinding_setting->bind.active = false;
			this->m_rebinding_setting = nullptr;
			return true;
		}

		return ImGui_ImplWin32_WndProcHandler( hwnd, msg, wparam, lparam ) != 0;
	}

} // namespace rendering
