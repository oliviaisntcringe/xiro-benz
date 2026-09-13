#pragma once

#include <external/imgui/imgui.h>

namespace rendering {
    namespace retro {
        inline constexpr ImVec4 accent_green{ 0.47f, 0.78f, 0.29f, 1.0f };
        inline constexpr ImVec4 accent_green_dim{ 0.247f, 0.435f, 0.208f, 1.0f };
        inline constexpr ImVec4 accent_purple{ 0.553f, 0.290f, 0.690f, 1.0f };
        inline constexpr ImVec4 text_primary{ 0.894f, 0.894f, 0.894f, 1.0f };
        inline constexpr ImVec4 text_muted{ 0.565f, 0.565f, 0.565f, 1.0f };

        // Apply RIFK7 retro terminal inspired palette and base style tweaks
        inline void apply_rifk7_palette( ImGuiStyle& style )
        {
            // Colors (derived from the design spec: dark slate background with green/yellow accents)
            auto& colors = style.Colors;
            colors[ ImGuiCol_WindowBg ] = ImVec4{ 0.09f, 0.09f, 0.09f, 0.96f };
            colors[ ImGuiCol_ChildBg ] = ImVec4{ 0.125f, 0.125f, 0.125f, 0.96f };
            colors[ ImGuiCol_FrameBg ] = ImVec4{ 0.16f, 0.16f, 0.16f, 1.0f };
            colors[ ImGuiCol_FrameBgHovered ] = ImVec4{ 0.25f, 0.42f, 0.18f, 1.0f };
            colors[ ImGuiCol_FrameBgActive ] = ImVec4{ 0.34f, 0.58f, 0.22f, 1.0f };
            colors[ ImGuiCol_Button ] = ImVec4{ 0.12f, 0.12f, 0.12f, 1.0f };
            colors[ ImGuiCol_ButtonHovered ] = ImVec4{ 0.25f, 0.42f, 0.18f, 1.0f };
            colors[ ImGuiCol_ButtonActive ] = ImVec4{ 0.34f, 0.58f, 0.22f, 1.0f };
            colors[ ImGuiCol_Border ] = ImVec4{ 0.38f, 0.38f, 0.38f, 1.0f };
            colors[ ImGuiCol_Header ] = ImVec4{ 0.25f, 0.42f, 0.18f, 1.0f };
            colors[ ImGuiCol_HeaderHovered ] = ImVec4{ 0.34f, 0.58f, 0.22f, 1.0f };
            colors[ ImGuiCol_HeaderActive ] = ImVec4{ 0.34f, 0.58f, 0.22f, 1.0f };
            colors[ ImGuiCol_CheckMark ] = ImVec4{ 0.71f, 0.97f, 0.42f, 1.0f };
            colors[ ImGuiCol_SliderGrab ] = ImVec4{ 0.47f, 0.78f, 0.29f, 1.0f };
            colors[ ImGuiCol_SliderGrabActive ] = ImVec4{ 0.71f, 0.97f, 0.42f, 1.0f };

            // Subtle tweaks for a more terminal/retro feel
            style.WindowRounding = 0.0f;
            style.ChildRounding = 0.0f;
            style.FrameRounding = 0.0f;
            style.PopupRounding = 0.0f;
            style.WindowBorderSize = 1.0f;
            style.FrameBorderSize = 1.0f;
            style.ItemSpacing = ImVec2{ 8.0f, 6.0f };

            // Typography and spacing tokens can be adjusted by the caller via IO.Fonts additions.
        }

        // Simple helper to draw a retro header bar (wordmark area) using the ImDrawList
        inline void draw_retro_header( ImDrawList* draw_list, const ImRect& rect )
        {
            if ( !draw_list )
                return;

            draw_list->AddRectFilled( rect.Min, rect.Max, IM_COL32(20, 20, 20, 220), 0.0f );
            draw_list->AddRect( rect.Min, rect.Max, IM_COL32(96, 96, 96, 230), 0.0f, 0, 1.0f );
            draw_list->AddRectFilled(
                ImVec2{ rect.Min.x, rect.Max.y - 2.0f },
                ImVec2{ rect.Max.x, rect.Max.y },
                IM_COL32(119, 200, 74, 230 )
            );
            draw_list->AddRectFilled(
                ImVec2{ rect.Max.x - 28.0f, rect.Max.y - 2.0f },
                rect.Max,
                IM_COL32(141, 74, 176, 230 )
            );
        }
    }
}
