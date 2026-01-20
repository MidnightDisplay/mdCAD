//------------------------------------------------------------------------------
// ui_theme.h - ImGui theme configuration (header-only)
//
// Visual Studio dark theme based on ImThemes repository.
//------------------------------------------------------------------------------
#ifndef UI_THEME_H
#define UI_THEME_H

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"

//------------------------------------------------------------------------------
// Theme Application
//------------------------------------------------------------------------------

static inline void ui_theme_apply_visual_studio(void) {
    ImGuiStyle* style = igGetStyle();

    // Style settings (rectangular, no rounding)
    style->WindowRounding = 0.0f;
    style->FrameRounding = 0.0f;
    style->ScrollbarRounding = 0.0f;
    style->GrabRounding = 0.0f;
    style->TabRounding = 0.0f;
    style->ChildRounding = 0.0f;
    style->PopupRounding = 0.0f;

    // Visual Studio dark color scheme
    ImVec4* colors = style->Colors;

    colors[ImGuiCol_Text]                   = (ImVec4){1.00f, 1.00f, 1.00f, 1.00f};
    colors[ImGuiCol_TextDisabled]           = (ImVec4){0.50f, 0.50f, 0.50f, 1.00f};
    colors[ImGuiCol_WindowBg]               = (ImVec4){0.145f, 0.145f, 0.149f, 1.00f};  // #252526
    colors[ImGuiCol_ChildBg]                = (ImVec4){0.145f, 0.145f, 0.149f, 1.00f};
    colors[ImGuiCol_PopupBg]                = (ImVec4){0.145f, 0.145f, 0.149f, 1.00f};
    colors[ImGuiCol_Border]                 = (ImVec4){0.30f, 0.30f, 0.30f, 1.00f};
    colors[ImGuiCol_BorderShadow]           = (ImVec4){0.00f, 0.00f, 0.00f, 0.00f};
    colors[ImGuiCol_FrameBg]                = (ImVec4){0.20f, 0.20f, 0.216f, 1.00f};   // #333337
    colors[ImGuiCol_FrameBgHovered]         = (ImVec4){0.114f, 0.592f, 0.925f, 0.40f}; // #1D97EC with alpha
    colors[ImGuiCol_FrameBgActive]          = (ImVec4){0.00f, 0.467f, 0.784f, 0.67f};  // #0077C8 with alpha
    colors[ImGuiCol_TitleBg]                = (ImVec4){0.145f, 0.145f, 0.149f, 1.00f};
    colors[ImGuiCol_TitleBgActive]          = (ImVec4){0.00f, 0.467f, 0.784f, 1.00f};  // #0077C8
    colors[ImGuiCol_TitleBgCollapsed]       = (ImVec4){0.145f, 0.145f, 0.149f, 1.00f};
    colors[ImGuiCol_MenuBarBg]              = (ImVec4){0.20f, 0.20f, 0.216f, 1.00f};
    colors[ImGuiCol_ScrollbarBg]            = (ImVec4){0.145f, 0.145f, 0.149f, 1.00f};
    colors[ImGuiCol_ScrollbarGrab]          = (ImVec4){0.40f, 0.40f, 0.40f, 1.00f};
    colors[ImGuiCol_ScrollbarGrabHovered]   = (ImVec4){0.50f, 0.50f, 0.50f, 1.00f};
    colors[ImGuiCol_ScrollbarGrabActive]    = (ImVec4){0.60f, 0.60f, 0.60f, 1.00f};
    colors[ImGuiCol_CheckMark]              = (ImVec4){0.00f, 0.467f, 0.784f, 1.00f};  // #0077C8
    colors[ImGuiCol_SliderGrab]             = (ImVec4){0.114f, 0.592f, 0.925f, 1.00f}; // #1D97EC
    colors[ImGuiCol_SliderGrabActive]       = (ImVec4){0.00f, 0.467f, 0.784f, 1.00f};  // #0077C8
    colors[ImGuiCol_Button]                 = (ImVec4){0.20f, 0.20f, 0.216f, 1.00f};   // #333337
    colors[ImGuiCol_ButtonHovered]          = (ImVec4){0.114f, 0.592f, 0.925f, 1.00f}; // #1D97EC
    colors[ImGuiCol_ButtonActive]           = (ImVec4){0.00f, 0.467f, 0.784f, 1.00f};  // #0077C8
    colors[ImGuiCol_Header]                 = (ImVec4){0.20f, 0.20f, 0.216f, 1.00f};   // #333337
    colors[ImGuiCol_HeaderHovered]          = (ImVec4){0.114f, 0.592f, 0.925f, 1.00f}; // #1D97EC
    colors[ImGuiCol_HeaderActive]           = (ImVec4){0.00f, 0.467f, 0.784f, 1.00f};  // #0077C8
    colors[ImGuiCol_Separator]              = (ImVec4){0.30f, 0.30f, 0.30f, 1.00f};
    colors[ImGuiCol_SeparatorHovered]       = (ImVec4){0.114f, 0.592f, 0.925f, 1.00f};
    colors[ImGuiCol_SeparatorActive]        = (ImVec4){0.00f, 0.467f, 0.784f, 1.00f};
    colors[ImGuiCol_ResizeGrip]             = (ImVec4){0.20f, 0.20f, 0.216f, 1.00f};
    colors[ImGuiCol_ResizeGripHovered]      = (ImVec4){0.114f, 0.592f, 0.925f, 1.00f};
    colors[ImGuiCol_ResizeGripActive]       = (ImVec4){0.00f, 0.467f, 0.784f, 1.00f};
    colors[ImGuiCol_Tab]                    = (ImVec4){0.145f, 0.145f, 0.149f, 1.00f};
    colors[ImGuiCol_TabHovered]             = (ImVec4){0.114f, 0.592f, 0.925f, 1.00f};
    colors[ImGuiCol_TabSelected]            = (ImVec4){0.00f, 0.467f, 0.784f, 1.00f};
    colors[ImGuiCol_TabSelectedOverline]    = (ImVec4){0.114f, 0.592f, 0.925f, 1.00f};
    colors[ImGuiCol_TabDimmed]              = (ImVec4){0.145f, 0.145f, 0.149f, 1.00f};
    colors[ImGuiCol_TabDimmedSelected]      = (ImVec4){0.20f, 0.20f, 0.216f, 1.00f};
    colors[ImGuiCol_TabDimmedSelectedOverline] = (ImVec4){0.50f, 0.50f, 0.50f, 1.00f};
    colors[ImGuiCol_DockingPreview]         = (ImVec4){0.114f, 0.592f, 0.925f, 0.70f};
    colors[ImGuiCol_DockingEmptyBg]         = (ImVec4){0.10f, 0.10f, 0.10f, 1.00f};
    colors[ImGuiCol_PlotLines]              = (ImVec4){0.114f, 0.592f, 0.925f, 1.00f};
    colors[ImGuiCol_PlotLinesHovered]       = (ImVec4){0.00f, 0.467f, 0.784f, 1.00f};
    colors[ImGuiCol_PlotHistogram]          = (ImVec4){0.114f, 0.592f, 0.925f, 1.00f};
    colors[ImGuiCol_PlotHistogramHovered]   = (ImVec4){0.00f, 0.467f, 0.784f, 1.00f};
    colors[ImGuiCol_TableHeaderBg]          = (ImVec4){0.20f, 0.20f, 0.216f, 1.00f};
    colors[ImGuiCol_TableBorderStrong]      = (ImVec4){0.30f, 0.30f, 0.30f, 1.00f};
    colors[ImGuiCol_TableBorderLight]       = (ImVec4){0.25f, 0.25f, 0.25f, 1.00f};
    colors[ImGuiCol_TableRowBg]             = (ImVec4){0.00f, 0.00f, 0.00f, 0.00f};
    colors[ImGuiCol_TableRowBgAlt]          = (ImVec4){1.00f, 1.00f, 1.00f, 0.03f};
    colors[ImGuiCol_TextSelectedBg]         = (ImVec4){0.00f, 0.467f, 0.784f, 0.35f};
    colors[ImGuiCol_DragDropTarget]         = (ImVec4){0.114f, 0.592f, 0.925f, 1.00f};
    colors[ImGuiCol_NavCursor]              = (ImVec4){0.114f, 0.592f, 0.925f, 1.00f};
    colors[ImGuiCol_NavWindowingHighlight]  = (ImVec4){1.00f, 1.00f, 1.00f, 0.70f};
    colors[ImGuiCol_NavWindowingDimBg]      = (ImVec4){0.80f, 0.80f, 0.80f, 0.20f};
    colors[ImGuiCol_ModalWindowDimBg]       = (ImVec4){0.20f, 0.20f, 0.20f, 0.35f};
}

#endif // UI_THEME_H
