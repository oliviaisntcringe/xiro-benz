#include <pch/pch.hpp>
#include <core/systems/systems.hpp>
#include <core/features/features.hpp>
#include <utilities/diag.hpp>

#include "../rendering.hpp"

namespace rendering {

	bool context::try_bind_ui_assets( )
	{
		if ( this->m_ui_assets_ready )
		{
			return true;
		}

		g_fonts.initialize( );

		this->m_ui_assets_ready = true;
		return true;
	}

	bool context::initialize( IDXGISwapChain* swap_chain )
	{
		if ( this->m_initialized )
		{
			return true;
		}
		if ( !swap_chain )
		{
			diag::write( diag::level::error, "render context initialization rejected a null swap chain" );
			return false;
		}

		if ( FAILED( swap_chain->GetDevice( __uuidof( ID3D11Device ), reinterpret_cast< void** >( &this->m_device ) ) ) )
		{
			diag::write( diag::level::error, "render context failed to acquire the D3D11 device" );
			return false;
		}

		this->m_device->GetImmediateContext( &this->m_context );
		if ( !this->m_context )
		{
			diag::write( diag::level::error, "render context failed to acquire the immediate D3D11 context" );
			this->m_device->Release( );
			this->m_device = nullptr;
			return false;
		}

		DXGI_SWAP_CHAIN_DESC desc{};
		swap_chain->GetDesc( &desc );

		this->m_window = desc.OutputWindow;

		this->create_rtv( swap_chain );
		if ( !this->m_rtv )
		{
			diag::write( diag::level::error, "render context failed to create the swap-chain render target" );
			this->shutdown( );
			return false;
		}
		this->setup_zdraw( this->m_window );
		if ( !g_imgui_menu.initialize( swap_chain, this->m_window ) )
		{
			diag::write( diag::level::error, "render context failed to initialize the ImGui DX11 backend" );
			this->shutdown( );
			return false;
		}

		g_menu.initialize_graphics( );
		this->try_bind_ui_assets( );
		systems::g_model_preview.initialize_render( this->m_device, this->m_context );
		diag::write( diag::level::info, "render pipeline initialized; ImGui interactive path ready, XDraw gameplay pass ready" );

		this->m_initialized = true;
		return true;
	}

	void context::shutdown( )
	{
		g_imgui_menu.shutdown( );

		if ( this->m_rtv )
		{
			this->m_rtv->Release( );
			this->m_rtv = nullptr;
		}

		if ( this->m_context )
		{
			this->m_context->Release( );
			this->m_context = nullptr;
		}

		if ( this->m_device )
		{
			this->m_device->Release( );
			this->m_device = nullptr;
		}

		this->m_window = nullptr;
		this->m_initialized = false;
	}

	void context::on_present( IDXGISwapChain* swap_chain )
	{
		diag::exception_scope render_scope{ "render: present" };
		static volatile LONG64 frame_sequence{};
		const auto frame = InterlockedIncrement64( &frame_sequence );
		const auto trace_frame = diag::g_verbose_logging && ( frame <= 8 || frame % 120 == 0 );
		if ( trace_frame )
		{
			diag::writef( diag::level::debug, "render frame begin index=%llu swap_chain=0x%p initialized=%s", frame, swap_chain, this->m_initialized ? "true" : "false" );
		}
		diag::set_exception_phase( "render: context initialization" );
		if ( !this->m_initialized ) [[unlikely]]
		{
			if ( !this->initialize( swap_chain ) ) [[unlikely]]
			{
				diag::writef( diag::level::error, "render frame skipped index=%llu reason=context initialization failed", frame );
				return;
			}
		}

		diag::set_exception_phase( "render: bind UI assets" );
		this->try_bind_ui_assets( );
		diag::set_exception_phase( "render: dynamic lights" );
		features::misc::g_dlight.on_present( );

		if ( !this->m_context || !this->m_rtv || !g_imgui_menu.is_initialized( ) )
		{
			diag::writef( diag::level::error, "render frame skipped index=%llu reason=D3D11 or ImGui state incomplete", frame );
			return;
		}

		systems::g_model_preview.tick_render_thread( );

		diag::set_exception_phase( "render: ImGui begin frame" );
		m_context->OMSetRenderTargets( 1, &this->m_rtv, nullptr );
		g_imgui_menu.begin_frame( );

		diag::set_exception_phase( "render: XDraw feature pass" );
		xdraw::begin_frame( true );
		{
			auto& dl = xdraw::get( xdraw::layer::bottom );

			if ( this->m_ui_assets_ready && g_imgui_menu.gameplay_ready( ) && systems::g_local.get( ).is_valid( ) && systems::g_view.has_camera( ) )
			{
				diag::set_exception_phase( "render: impacts early" );
				features::misc::g_impacts.on_render_early( dl );
				diag::set_exception_phase( "render: antiaim" );
				features::combat::g_misc.antiaim( ).on_render( dl );
				diag::set_exception_phase( "render: edgebug" );
				features::movement::g_edgebug.on_render( dl );
				diag::set_exception_phase( "render: item overlay" );
				features::esp::item::g_overlay.on_render( dl );
				diag::set_exception_phase( "render: projectile overlay" );
				features::esp::projectile::g_overlay.on_render( dl, xdraw::get( xdraw::layer::middle ) );
				diag::set_exception_phase( "render: player overlay" );
				features::esp::player::g_overlay.on_render( dl );
				diag::set_exception_phase( "render: projectile trajectory" );
				features::misc::g_projectile_trajectory.on_render( dl );
				diag::set_exception_phase( "render: rage" );
				features::combat::g_rage.on_render( dl );
				diag::set_exception_phase( "render: legit" );
				features::combat::g_legit.on_render( dl );
				diag::set_exception_phase( "render: impacts" );
				features::misc::g_impacts.on_render( dl );
				{
					diag::exception_scope hud_scope{ "render: custom hud" };
					features::misc::g_hud.on_render( dl );
				}
				diag::set_exception_phase( "render: other overlay" );
				features::esp::other::g_overlay.on_render( dl );
			}

		}
		xdraw::end_frame( );
		diag::set_exception_phase( "render: ImGui overlays" );
		g_imgui_menu.draw_overlays( );
		diag::set_exception_phase( "render: ImGui menu" );
		g_imgui_menu.draw( );
		diag::set_exception_phase( "render: ImGui submit" );
		g_imgui_menu.render( );
		if ( trace_frame )
		{
			diag::writef( diag::level::debug, "render frame end index=%llu gameplay_ready=%s menu_open=%s", frame, g_imgui_menu.gameplay_ready( ) ? "true" : "false", g_imgui_menu.is_open( ) ? "true" : "false" );
		}
	}

	void context::on_resize_buffers( )
	{
		if ( this->m_rtv )
		{
			this->m_rtv->Release( );
			this->m_rtv = nullptr;
		}
	}

	void context::on_resize_buffers_post( IDXGISwapChain* swap_chain )
	{
		this->create_rtv( swap_chain );
	}

	void context::create_rtv( IDXGISwapChain* swap_chain )
	{
		ID3D11Texture2D* back_buffer{ nullptr };
		if ( SUCCEEDED( swap_chain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), reinterpret_cast< void** >( &back_buffer ) ) ) )
		{
			this->m_device->CreateRenderTargetView( back_buffer, nullptr, &this->m_rtv );

			D3D11_TEXTURE2D_DESC back_buffer_desc{};
			back_buffer->GetDesc( &back_buffer_desc );

			D3D11_VIEWPORT viewport{};
			viewport.TopLeftX = 0.0f;
			viewport.TopLeftY = 0.0f;
			viewport.Width = static_cast< float >( back_buffer_desc.Width );
			viewport.Height = static_cast< float >( back_buffer_desc.Height );
			viewport.MinDepth = 0.0f;
			viewport.MaxDepth = 1.0f;
			this->m_context->RSSetViewports( 1, &viewport );

			back_buffer->Release( );
		}
	}

	void context::setup_zdraw( HWND window )
	{
		if ( !xdraw::initialize( this->m_device, this->m_context ) )
		{
			return;
		}

		xui::initialize( window );
	}

} // namespace rendering
