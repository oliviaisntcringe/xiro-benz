#pragma once

#include <atomic>
#include <array>

struct IDXGISwapChain;
struct ImFont;
struct ImDrawList;
struct ImGuiViewport;

namespace config {
	struct col;
}

namespace rendering {

	class imgui_menu
	{
	public:
		bool initialize( IDXGISwapChain* swap_chain, HWND window );
		void shutdown( );
		void begin_frame( );
		void draw( );
		void draw_overlays( );
		void render( );
		bool wndproc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam );
		[[nodiscard]] bool is_initialized( ) const { return this->m_initialized; }
		[[nodiscard]] bool gameplay_ready( ) const
		{
			return this->m_initialized && this->m_loading_state.load( std::memory_order_acquire ) == 1;
		}
		void loading_begin_check( const char* label ) noexcept;
		void loading_check_result( int result ) noexcept;
		void loading_failed( const char* reason ) noexcept;
		void loading_complete( ) noexcept;

		void toggle( ) { this->m_open = !this->m_open; }
		[[nodiscard]] bool is_open( ) const { return this->m_open; }

	private:
		void draw_loading_screen( const ImGuiViewport* viewport );
		void draw_left_rail( float width, float height );
		void draw_panel( );
		void draw_visuals_tab( );
		void draw_misc_tab( );
		void draw_skins_tab( );
		void draw_legit_tab( );
		void draw_rage_tab( );
		void draw_personal_tab( );
		void draw_config_tab( );
		void draw_color( const char* label, config::col& color );
		bool draw_function( const char* label, xui::setting& setting, bool expandable = false );
		void try_load_avatar( );
		void draw_spectators( ImDrawList* draw, const ImGuiViewport* viewport );

		bool m_initialized{};
		bool m_open{};
		bool m_rebinding_menu_key{};
		xui::setting* m_rebinding_setting{};
		int m_tab{};
		int m_visual_section{};
		int m_aim_weapon_group{};
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_logo{};
		int m_logo_width{};
		int m_logo_height{};
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_avatar{};
		ImFont* m_mono_font{};
		float m_avatar_retry_delay{};

		static constexpr int k_loading_check_capacity{ 32 };
		std::array<const char*, k_loading_check_capacity> m_loading_labels{};
		std::array<std::atomic<int>, k_loading_check_capacity> m_loading_check_states{};
		std::atomic<int> m_loading_check_count{};
		std::atomic<int> m_loading_current_check{ -1 };
		std::atomic<int> m_loading_state{};
		std::atomic<const char*> m_loading_error{};
		float m_loading_elapsed{};
	};

	inline imgui_menu g_imgui_menu{};

} // namespace rendering
