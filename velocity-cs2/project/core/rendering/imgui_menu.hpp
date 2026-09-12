#pragma once

struct IDXGISwapChain;

namespace rendering {

	class imgui_menu
	{
	public:
		bool initialize( IDXGISwapChain* swap_chain, HWND window );
		void shutdown( );
		void draw( );
		bool wndproc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam );

		void toggle( ) { this->m_open = !this->m_open; }
		[[nodiscard]] bool is_open( ) const { return this->m_open; }

	private:
		void draw_sidebar( );
		void draw_panel( );

		bool m_initialized{};
		bool m_open{};
		int m_tab{};
	};

	inline imgui_menu g_imgui_menu{};

} // namespace rendering
