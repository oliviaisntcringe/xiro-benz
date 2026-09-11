#include <pch/pch.hpp>
#include <core/settings.hpp>
#include <core/rendering/retro_style.hpp>

#include "../../rendering.hpp"

namespace rendering
{
    void menu::draw_local( float group_w ) const
    {
        ( void )group_w;
        auto& hat = settings::g_misc.m_hud.m_hat;
        const auto wx = this->m_x;
        const auto wy = this->m_y;
        const auto content_x = wx + tokens::gap + tokens::sidebar_w + tokens::gap;
        const auto body_y = wy + tokens::gap + tokens::subtab_bar_h + tokens::gap;
        const auto content_w = this->m_w - tokens::gap * 2.0f - tokens::sidebar_w - tokens::gap;
        const auto col_w = ( content_w - tokens::gap ) * 0.5f;
        const auto right_x = content_x + col_w + tokens::gap;

        xui::layout::set_cursor( content_x - wx, body_y - wy );

        if ( this->m_subtab == 0 )
        {
            if ( xui::begin_child( "##local_preview", col_w, 310.0f ) )
            {
                const auto bounds = xui::layout::current_window_const( )->bounds;
                auto& dl = xui::draw::current( );
                const auto cx = bounds.x + bounds.w * 0.5f;
                const auto cy = bounds.y + 138.0f;
                const auto primary = retro::to_xdraw_color( retro::palette::accent_green );
                const auto secondary = retro::to_xdraw_color( retro::palette::accent_purple );
                const auto muted = retro::to_xdraw_color( retro::palette::text_muted );

                retro::push_font( );
                dl.text( bounds.x + 12.0f, bounds.y + 10.0f, "> local // preview", primary );
                retro::draw_rule( dl, bounds.x + 12.0f, bounds.y + 28.0f, bounds.right( ) - 12.0f, retro::palette::accent_green_dim );

                dl.circle( cx, cy - 54.0f, 25.0f, muted, 1.0f, 24 );
                dl.rect( cx - 21.0f, cy - 28.0f, 42.0f, 88.0f, muted, 1.0f );
                dl.line( cx - 21.0f, cy - 10.0f, cx - 51.0f, cy + 32.0f, muted, 1.0f );
                dl.line( cx + 21.0f, cy - 10.0f, cx + 51.0f, cy + 32.0f, muted, 1.0f );
                dl.line( cx - 13.0f, cy + 60.0f, cx - 22.0f, cy + 103.0f, muted, 1.0f );
                dl.line( cx + 13.0f, cy + 60.0f, cx + 22.0f, cy + 103.0f, muted, 1.0f );

                if ( hat.enabled.value )
                {
                    dl.line( cx - 39.0f, cy - 55.0f, cx, cy - 76.0f, primary, 1.5f );
                    dl.line( cx + 39.0f, cy - 55.0f, cx, cy - 76.0f, primary, 1.5f );
                    dl.line( cx - 39.0f, cy - 55.0f, cx + 39.0f, cy - 55.0f, primary, 1.5f );
                    dl.circle( cx, cy - 76.0f, 3.0f, secondary, 1.0f, 8 );
                }

                dl.circle( cx, cy + 112.0f, 48.0f, secondary, 1.0f, 32 );
                dl.line( cx - 64.0f, cy + 112.0f, cx + 64.0f, cy + 112.0f, primary, 1.0f );
                dl.text( bounds.x + 12.0f, bounds.bottom( ) - 24.0f, hat.enabled.value ? "SAMURAI // ACTIVE" : "NO LOADOUT", hat.enabled.value ? primary : muted );
                xui::end_child( );
            }

            xui::layout::set_cursor( right_x - wx, body_y - wy );
            if ( xui::begin_child( "##local_controls", col_w ) )
            {
                xui::checkbox( "samurai head", hat.enabled );
                if ( xui::begin_popup( "##local_head_popup", 220.0f ) )
                {
                    constexpr const char* types[ ]{ "kasa", "bucket" };
                    xui::combo( "helmet", hat.type.value, types, 2 );
                    xui::color_picker( "primary color", hat.color );
                    xui::color_picker( "secondary color", hat.secondary_color );
                    xui::checkbox( "glow", hat.glow );
                    xui::slider_float( "glow strength", hat.glow_strength, 0.1f, 1.0f, "%.2f" );
                    xui::end_popup( );
                }

                xui::text( "ground ring // preview layer", tokens::col_text_dim );
                xui::text( "wireframe body // preview layer", tokens::col_text_dim );
                xui::layout::separator( );
                xui::text( "preview controls", tokens::col_text );
                xui::text( "orbit camera: mouse drag", tokens::col_text_dim );
                xui::text( "preset: SAMURAI", tokens::col_accent );
                xui::end_child( );
            }
        }
        else if ( this->m_subtab == 1 )
        {
            if ( xui::begin_child( "##local_presets", content_w ) )
            {
                xui::text( "> presets // local character", tokens::col_accent );
                xui::layout::separator( );
                if ( xui::button( "SAMURAI", 150.0f, 28.0f ) )
                {
                    hat.enabled.value = true;
                    hat.type.value = settings::misc::hud::hat::hat_type::kasa;
                }
                xui::same_line( );
                if ( xui::button( "BUCKET", 150.0f, 28.0f ) )
                {
                    hat.enabled.value = true;
                    hat.type.value = settings::misc::hud::hat::hat_type::bucket;
                }
                xui::end_child( );
            }
        }
        else
        {
            if ( xui::begin_child( "##local_layers", content_w ) )
            {
                xui::text( "> layers // world-space accessories", tokens::col_accent );
                xui::layout::separator( );
                xui::text( "head accessory // linked", tokens::col_text );
                xui::text( "shoulder panels // planned", tokens::col_text_dim );
                xui::text( "ground ring // preview", tokens::col_text_dim );
                xui::text( "status marker // preview", tokens::col_text_dim );
                xui::end_child( );
            }
        }
    }
}
