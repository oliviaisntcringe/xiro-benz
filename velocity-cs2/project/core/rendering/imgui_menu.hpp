#pragma once

struct IDXGISwapChain;
struct ImFont;

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

		void toggle( ) { this->m_open = !this->m_open; }
		[[nodiscard]] bool is_open( ) const { return this->m_open; }

	private:
		void draw_sidebar( );
		void draw_panel( );
		void try_load_avatar( );

		bool m_initialized{};
		bool m_open{};
		bool m_rebinding_menu_key{};
		xui::setting* m_rebinding_setting{};
		int m_tab{};
		int m_visual_section{};
		int m_aim_weapon_group{};
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_avatar{};
		ImFont* m_mono_font{};
		float m_avatar_retry_delay{};
	};

	inline imgui_menu g_imgui_menu{};

} // namespace rendering
