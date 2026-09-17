#pragma once

#include <external/imgui/imgui.h>

namespace rendering {
    namespace retro {
        inline constexpr ImVec4 accent_green{ 0.47f, 0.78f, 0.29f, 1.0f };
        inline constexpr ImVec4 accent_green_dim{ 0.247f, 0.435f, 0.208f, 1.0f };
        inline constexpr ImVec4 accent_purple{ 0.553f, 0.290f, 0.690f, 1.0f };
        inline constexpr ImVec4 text_primary{ 0.894f, 0.894f, 0.894f, 1.0f };
        inline constexpr ImVec4 text_muted{ 0.565f, 0.565f, 0.565f, 1.0f };

        // Apply the shared palette and geometry used by the in-game menu and previewer.
        inline void apply_rifk7_palette( ImGuiStyle& style )
        {
            auto& colors = style.Colors;
            colors[ ImGuiCol_Text ] = text_primary;
            colors[ ImGuiCol_TextDisabled ] = text_muted;
            colors[ ImGuiCol_WindowBg ] = ImVec4{ 0.035f, 0.045f, 0.052f, 0.985f };
            colors[ ImGuiCol_ChildBg ] = ImVec4{ 0.045f, 0.055f, 0.062f, 0.985f };
            colors[ ImGuiCol_PopupBg ] = ImVec4{ 0.040f, 0.050f, 0.058f, 1.0f };
            colors[ ImGuiCol_FrameBg ] = ImVec4{ 0.070f, 0.080f, 0.088f, 1.0f };
            colors[ ImGuiCol_FrameBgHovered ] = ImVec4{ 0.105f, 0.135f, 0.108f, 1.0f };
            colors[ ImGuiCol_FrameBgActive ] = ImVec4{ 0.125f, 0.170f, 0.125f, 1.0f };
            colors[ ImGuiCol_Button ] = ImVec4{ 0.060f, 0.070f, 0.078f, 1.0f };
            colors[ ImGuiCol_ButtonHovered ] = ImVec4{ 0.105f, 0.135f, 0.108f, 1.0f };
            colors[ ImGuiCol_ButtonActive ] = ImVec4{ 0.125f, 0.170f, 0.125f, 1.0f };
            colors[ ImGuiCol_Border ] = ImVec4{ 0.180f, 0.215f, 0.230f, 0.9f };
            colors[ ImGuiCol_BorderShadow ] = ImVec4{ 0.0f, 0.0f, 0.0f, 0.0f };
            colors[ ImGuiCol_Separator ] = ImVec4{ 0.180f, 0.230f, 0.200f, 0.8f };
            colors[ ImGuiCol_SeparatorHovered ] = accent_green;
            colors[ ImGuiCol_SeparatorActive ] = accent_green;
            colors[ ImGuiCol_Header ] = ImVec4{ 0.090f, 0.145f, 0.095f, 1.0f };
            colors[ ImGuiCol_HeaderHovered ] = ImVec4{ 0.115f, 0.180f, 0.115f, 1.0f };
            colors[ ImGuiCol_HeaderActive ] = ImVec4{ 0.145f, 0.220f, 0.135f, 1.0f };
            colors[ ImGuiCol_CheckMark ] = ImVec4{ 0.71f, 0.97f, 0.42f, 1.0f };
            colors[ ImGuiCol_SliderGrab ] = accent_green;
            colors[ ImGuiCol_SliderGrabActive ] = ImVec4{ 0.71f, 0.97f, 0.42f, 1.0f };

            style.WindowPadding = ImVec2{ 12.0f, 12.0f };
            style.FramePadding = ImVec2{ 9.0f, 5.0f };
            style.ItemInnerSpacing = ImVec2{ 8.0f, 6.0f };
            style.IndentSpacing = 18.0f;
            style.WindowRounding = 0.0f;
            style.ChildRounding = 0.0f;
            style.FrameRounding = 0.0f;
            style.PopupRounding = 0.0f;
            style.ScrollbarRounding = 0.0f;
            style.GrabRounding = 0.0f;
            style.TabRounding = 0.0f;
            style.WindowBorderSize = 1.0f;
            style.FrameBorderSize = 0.0f;
            style.ItemSpacing = ImVec2{ 10.0f, 8.0f };
        }

        // Simple helper to draw a retro header bar (wordmark area) using the ImDrawList
        inline void draw_retro_header( ImDrawList* draw_list, const ImVec2& min, const ImVec2& max )
        {
            if ( !draw_list )
                return;

            draw_list->AddRectFilled( min, max, IM_COL32( 13, 18, 22, 255 ) );
            draw_list->AddRect( min, max, IM_COL32( 64, 78, 84, 255 ), 0.0f, 0, 1.0f );
            draw_list->AddLine( ImVec2{ min.x, min.y + 1.0f }, ImVec2{ max.x, min.y + 1.0f }, IM_COL32( 78, 92, 98, 255 ), 1.0f );
            draw_list->AddRectFilled(
                ImVec2{ min.x, max.y - 2.0f },
                ImVec2{ max.x, max.y },
                IM_COL32( 119, 200, 74, 255 )
            );
            draw_list->AddRectFilled(
                ImVec2{ max.x - 28.0f, max.y - 2.0f },
                max,
                IM_COL32( 141, 74, 176, 255 )
            );
        }
    }
}
