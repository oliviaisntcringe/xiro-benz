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
            colors[ ImGuiCol_WindowBg ] = ImVec4{ 0.055f, 0.065f, 0.075f, 0.985f };
            colors[ ImGuiCol_ChildBg ] = ImVec4{ 0.075f, 0.085f, 0.100f, 0.98f };
            colors[ ImGuiCol_PopupBg ] = ImVec4{ 0.070f, 0.080f, 0.095f, 0.995f };
            colors[ ImGuiCol_FrameBg ] = ImVec4{ 0.115f, 0.130f, 0.150f, 1.0f };
            colors[ ImGuiCol_FrameBgHovered ] = ImVec4{ 0.165f, 0.230f, 0.185f, 1.0f };
            colors[ ImGuiCol_FrameBgActive ] = ImVec4{ 0.205f, 0.315f, 0.220f, 1.0f };
            colors[ ImGuiCol_Button ] = ImVec4{ 0.100f, 0.115f, 0.135f, 1.0f };
            colors[ ImGuiCol_ButtonHovered ] = ImVec4{ 0.165f, 0.230f, 0.185f, 1.0f };
            colors[ ImGuiCol_ButtonActive ] = ImVec4{ 0.205f, 0.315f, 0.220f, 1.0f };
            colors[ ImGuiCol_Border ] = ImVec4{ 0.220f, 0.260f, 0.300f, 0.85f };
            colors[ ImGuiCol_BorderShadow ] = ImVec4{ 0.0f, 0.0f, 0.0f, 0.25f };
            colors[ ImGuiCol_Separator ] = ImVec4{ 0.220f, 0.300f, 0.255f, 0.65f };
            colors[ ImGuiCol_SeparatorHovered ] = accent_green;
            colors[ ImGuiCol_SeparatorActive ] = accent_green;
            colors[ ImGuiCol_Header ] = ImVec4{ 0.135f, 0.205f, 0.165f, 1.0f };
            colors[ ImGuiCol_HeaderHovered ] = ImVec4{ 0.185f, 0.285f, 0.205f, 1.0f };
            colors[ ImGuiCol_HeaderActive ] = ImVec4{ 0.235f, 0.380f, 0.245f, 1.0f };
            colors[ ImGuiCol_CheckMark ] = ImVec4{ 0.71f, 0.97f, 0.42f, 1.0f };
            colors[ ImGuiCol_SliderGrab ] = accent_green;
            colors[ ImGuiCol_SliderGrabActive ] = ImVec4{ 0.71f, 0.97f, 0.42f, 1.0f };

            style.WindowPadding = ImVec2{ 18.0f, 16.0f };
            style.FramePadding = ImVec2{ 11.0f, 7.0f };
            style.ItemInnerSpacing = ImVec2{ 8.0f, 6.0f };
            style.IndentSpacing = 18.0f;
            style.WindowRounding = 8.0f;
            style.ChildRounding = 6.0f;
            style.FrameRounding = 5.0f;
            style.PopupRounding = 6.0f;
            style.ScrollbarRounding = 5.0f;
            style.GrabRounding = 5.0f;
            style.TabRounding = 5.0f;
            style.WindowBorderSize = 1.0f;
            style.FrameBorderSize = 0.0f;
            style.ItemSpacing = ImVec2{ 10.0f, 8.0f };
        }

        // Simple helper to draw a retro header bar (wordmark area) using the ImDrawList
        inline void draw_retro_header( ImDrawList* draw_list, const ImVec2& min, const ImVec2& max )
        {
            if ( !draw_list )
                return;

            constexpr auto rounding = 8.0f;
            draw_list->AddRectFilled( min, max, IM_COL32( 15, 21, 25, 248 ), rounding );
            draw_list->AddRect( min, max, IM_COL32( 62, 78, 84, 235 ), rounding, 0, 1.0f );
            draw_list->AddLine( ImVec2{ min.x + 14.0f, min.y + 1.0f }, ImVec2{ max.x - 14.0f, min.y + 1.0f }, IM_COL32( 115, 146, 157, 100 ), 1.0f );
            draw_list->AddRectFilled(
                ImVec2{ min.x, max.y - 2.0f },
                ImVec2{ max.x, max.y },
                IM_COL32( 119, 200, 74, 230 ),
                rounding
            );
            draw_list->AddRectFilled(
                ImVec2{ max.x - 28.0f, max.y - 2.0f },
                max,
                IM_COL32( 141, 74, 176, 230 ),
                rounding
            );
        }
    }
}
