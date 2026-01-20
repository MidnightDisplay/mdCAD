//------------------------------------------------------------------------------
// ui_theme.h - ImGui theme configuration (header-only)
//
// Available themes:
// - Visual Studio dark (blue-gray with rectangular styling)
// - iOS Light (clean white/gray with rounded corners and gray accents)
// - Catppuccin Frappé (muted pastel dark theme with lavender accent)
//------------------------------------------------------------------------------
#ifndef UI_THEME_H
#define UI_THEME_H

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"

//------------------------------------------------------------------------------
// Theme Types
//------------------------------------------------------------------------------
typedef enum {
    UI_THEME_VISUAL_STUDIO,
    UI_THEME_IOS_LIGHT,
    UI_THEME_CATPPUCCIN_FRAPPE,
    UI_THEME_COUNT
} ui_theme_t;

// Theme names for UI display
static const char* ui_theme_names[] = {
    "Visual Studio Dark",
    "iOS Light",
    "Catppuccin Frappe"
};

//------------------------------------------------------------------------------
// iOS Light Theme
//------------------------------------------------------------------------------

static inline void ui_theme_apply_ios_light(void) {
    ImGuiStyle* style = igGetStyle();

    // iOS-style rounded corners (subtle)
    style->WindowRounding = 5.0f;
    style->FrameRounding = 4.0f;
    style->ScrollbarRounding = 4.0f;
    style->GrabRounding = 4.0f;
    style->TabRounding = 0.0f;
    style->ChildRounding = 4.0f;
    style->PopupRounding = 5.0f;

    // iOS-style padding and spacing
    style->WindowPadding = (ImVec2){12.0f, 12.0f};
    style->FramePadding = (ImVec2){8.0f, 4.0f};
    style->ItemSpacing = (ImVec2){8.0f, 6.0f};
    style->ItemInnerSpacing = (ImVec2){6.0f, 6.0f};
    style->ScrollbarSize = 12.0f;
    style->GrabMinSize = 12.0f;

    // iOS Light color palette with gray accents
    // Background: #F2F2F7 (grouped), #FFFFFF (content)
    // Text: #000000 (primary), #3C3C43 @ 60% (secondary)
    // Separator: #C6C6C8
    // Accents: Various grays instead of system blue
    ImVec4* colors = style->Colors;

    // Text
    colors[ImGuiCol_Text]                   = (ImVec4){0.00f, 0.00f, 0.00f, 1.00f};        // Black
    colors[ImGuiCol_TextDisabled]           = (ImVec4){0.235f, 0.235f, 0.263f, 0.60f};     // #3C3C43 @ 60%

    // Backgrounds
    colors[ImGuiCol_WindowBg]               = (ImVec4){0.949f, 0.949f, 0.969f, 1.00f};     // #F2F2F7
    colors[ImGuiCol_ChildBg]                = (ImVec4){1.00f, 1.00f, 1.00f, 1.00f};        // White
    colors[ImGuiCol_PopupBg]                = (ImVec4){1.00f, 1.00f, 1.00f, 0.98f};        // White with slight transparency

    // Borders
    colors[ImGuiCol_Border]                 = (ImVec4){0.776f, 0.776f, 0.784f, 1.00f};     // #C6C6C8
    colors[ImGuiCol_BorderShadow]           = (ImVec4){0.00f, 0.00f, 0.00f, 0.00f};

    // Frame backgrounds (input fields, checkboxes, etc.)
    colors[ImGuiCol_FrameBg]                = (ImVec4){0.92f, 0.92f, 0.94f, 1.00f};        // Light gray
    colors[ImGuiCol_FrameBgHovered]         = (ImVec4){0.50f, 0.50f, 0.52f, 0.20f};        // Gray @ 20%
    colors[ImGuiCol_FrameBgActive]          = (ImVec4){0.50f, 0.50f, 0.52f, 0.30f};        // Gray @ 30%

    // Title bar
    colors[ImGuiCol_TitleBg]                = (ImVec4){0.949f, 0.949f, 0.969f, 1.00f};     // #F2F2F7
    colors[ImGuiCol_TitleBgActive]          = (ImVec4){1.00f, 1.00f, 1.00f, 1.00f};        // White
    colors[ImGuiCol_TitleBgCollapsed]       = (ImVec4){0.949f, 0.949f, 0.969f, 0.75f};

    // Menu bar
    colors[ImGuiCol_MenuBarBg]              = (ImVec4){0.949f, 0.949f, 0.969f, 1.00f};     // #F2F2F7

    // Scrollbar
    colors[ImGuiCol_ScrollbarBg]            = (ImVec4){0.949f, 0.949f, 0.969f, 0.50f};
    colors[ImGuiCol_ScrollbarGrab]          = (ImVec4){0.776f, 0.776f, 0.784f, 1.00f};     // #C6C6C8
    colors[ImGuiCol_ScrollbarGrabHovered]   = (ImVec4){0.60f, 0.60f, 0.60f, 1.00f};
    colors[ImGuiCol_ScrollbarGrabActive]    = (ImVec4){0.50f, 0.50f, 0.50f, 1.00f};

    // Checkmark and sliders - Gray accent
    colors[ImGuiCol_CheckMark]              = (ImVec4){0.40f, 0.40f, 0.42f, 1.00f};        // Dark gray
    colors[ImGuiCol_SliderGrab]             = (ImVec4){0.55f, 0.55f, 0.57f, 1.00f};        // Medium gray
    colors[ImGuiCol_SliderGrabActive]       = (ImVec4){0.40f, 0.40f, 0.42f, 1.00f};        // Dark gray

    // Buttons - Gray
    colors[ImGuiCol_Button]                 = (ImVec4){0.85f, 0.85f, 0.87f, 1.00f};        // Light gray button
    colors[ImGuiCol_ButtonHovered]          = (ImVec4){0.75f, 0.75f, 0.77f, 1.00f};        // Darker on hover
    colors[ImGuiCol_ButtonActive]           = (ImVec4){0.65f, 0.65f, 0.67f, 1.00f};        // Even darker when active

    // Headers (collapsing headers, tree nodes)
    colors[ImGuiCol_Header]                 = (ImVec4){0.50f, 0.50f, 0.52f, 0.15f};        // Gray @ 15%
    colors[ImGuiCol_HeaderHovered]          = (ImVec4){0.50f, 0.50f, 0.52f, 0.25f};        // Gray @ 25%
    colors[ImGuiCol_HeaderActive]           = (ImVec4){0.50f, 0.50f, 0.52f, 0.35f};        // Gray @ 35%

    // Separators
    colors[ImGuiCol_Separator]              = (ImVec4){0.776f, 0.776f, 0.784f, 1.00f};     // #C6C6C8
    colors[ImGuiCol_SeparatorHovered]       = (ImVec4){0.55f, 0.55f, 0.57f, 0.60f};
    colors[ImGuiCol_SeparatorActive]        = (ImVec4){0.40f, 0.40f, 0.42f, 1.00f};

    // Resize grip
    colors[ImGuiCol_ResizeGrip]             = (ImVec4){0.776f, 0.776f, 0.784f, 0.50f};
    colors[ImGuiCol_ResizeGripHovered]      = (ImVec4){0.55f, 0.55f, 0.57f, 0.60f};
    colors[ImGuiCol_ResizeGripActive]       = (ImVec4){0.40f, 0.40f, 0.42f, 1.00f};

    // Tabs
    colors[ImGuiCol_Tab]                    = (ImVec4){0.92f, 0.92f, 0.94f, 1.00f};
    colors[ImGuiCol_TabHovered]             = (ImVec4){0.80f, 0.80f, 0.82f, 1.00f};
    colors[ImGuiCol_TabSelected]            = (ImVec4){1.00f, 1.00f, 1.00f, 1.00f};        // White (selected tab)
    colors[ImGuiCol_TabSelectedOverline]    = (ImVec4){0.50f, 0.50f, 0.52f, 1.00f};        // Gray overline
    colors[ImGuiCol_TabDimmed]              = (ImVec4){0.92f, 0.92f, 0.94f, 0.80f};
    colors[ImGuiCol_TabDimmedSelected]      = (ImVec4){0.949f, 0.949f, 0.969f, 1.00f};
    colors[ImGuiCol_TabDimmedSelectedOverline] = (ImVec4){0.50f, 0.50f, 0.50f, 1.00f};

    // Docking
    colors[ImGuiCol_DockingPreview]         = (ImVec4){0.50f, 0.50f, 0.52f, 0.40f};
    colors[ImGuiCol_DockingEmptyBg]         = (ImVec4){0.949f, 0.949f, 0.969f, 1.00f};

    // Plots
    colors[ImGuiCol_PlotLines]              = (ImVec4){0.40f, 0.40f, 0.42f, 1.00f};
    colors[ImGuiCol_PlotLinesHovered]       = (ImVec4){0.30f, 0.30f, 0.32f, 1.00f};
    colors[ImGuiCol_PlotHistogram]          = (ImVec4){0.55f, 0.55f, 0.57f, 1.00f};
    colors[ImGuiCol_PlotHistogramHovered]   = (ImVec4){0.40f, 0.40f, 0.42f, 1.00f};

    // Tables
    colors[ImGuiCol_TableHeaderBg]          = (ImVec4){0.92f, 0.92f, 0.94f, 1.00f};
    colors[ImGuiCol_TableBorderStrong]      = (ImVec4){0.776f, 0.776f, 0.784f, 1.00f};
    colors[ImGuiCol_TableBorderLight]       = (ImVec4){0.85f, 0.85f, 0.87f, 1.00f};
    colors[ImGuiCol_TableRowBg]             = (ImVec4){0.00f, 0.00f, 0.00f, 0.00f};
    colors[ImGuiCol_TableRowBgAlt]          = (ImVec4){0.00f, 0.00f, 0.00f, 0.03f};

    // Selection and highlights
    colors[ImGuiCol_TextSelectedBg]         = (ImVec4){0.50f, 0.50f, 0.52f, 0.25f};
    colors[ImGuiCol_DragDropTarget]         = (ImVec4){0.50f, 0.50f, 0.52f, 0.90f};
    colors[ImGuiCol_NavCursor]              = (ImVec4){0.50f, 0.50f, 0.52f, 1.00f};
    colors[ImGuiCol_NavWindowingHighlight]  = (ImVec4){1.00f, 1.00f, 1.00f, 0.70f};
    colors[ImGuiCol_NavWindowingDimBg]      = (ImVec4){0.00f, 0.00f, 0.00f, 0.20f};
    colors[ImGuiCol_ModalWindowDimBg]       = (ImVec4){0.00f, 0.00f, 0.00f, 0.35f};
}

//------------------------------------------------------------------------------
// Catppuccin Frappé Theme
// https://github.com/catppuccin/catppuccin
//------------------------------------------------------------------------------

static inline void ui_theme_apply_catppuccin_frappe(void) {
    ImGuiStyle* style = igGetStyle();

    // Zero rounding (rectangular styling)
    style->WindowRounding = 0.0f;
    style->FrameRounding = 0.0f;
    style->ScrollbarRounding = 0.0f;
    style->GrabRounding = 0.0f;
    style->TabRounding = 0.0f;
    style->ChildRounding = 0.0f;
    style->PopupRounding = 0.0f;

    // Catppuccin Frappé color palette
    // Base: #303446, Mantle: #292c3c, Crust: #232634
    // Surface0: #414559, Surface1: #51576d, Surface2: #626880
    // Overlay0: #737994, Overlay1: #838ba7, Overlay2: #949cbb
    // Text: #c6d0f5, Subtext0: #a5adce, Subtext1: #b5bfe2
    // Lavender: #babbf1 (accent), Blue: #8caaee
    ImVec4* colors = style->Colors;

    // Text
    colors[ImGuiCol_Text]                   = (ImVec4){0.776f, 0.816f, 0.961f, 1.00f};     // #c6d0f5
    colors[ImGuiCol_TextDisabled]           = (ImVec4){0.647f, 0.678f, 0.808f, 1.00f};     // #a5adce (Subtext0)

    // Backgrounds
    colors[ImGuiCol_WindowBg]               = (ImVec4){0.188f, 0.204f, 0.275f, 1.00f};     // #303446 (Base)
    colors[ImGuiCol_ChildBg]                = (ImVec4){0.188f, 0.204f, 0.275f, 1.00f};     // #303446
    colors[ImGuiCol_PopupBg]                = (ImVec4){0.161f, 0.173f, 0.235f, 0.98f};     // #292c3c (Mantle)

    // Borders
    colors[ImGuiCol_Border]                 = (ImVec4){0.384f, 0.408f, 0.502f, 1.00f};     // #626880 (Surface2)
    colors[ImGuiCol_BorderShadow]           = (ImVec4){0.00f, 0.00f, 0.00f, 0.00f};

    // Frame backgrounds
    colors[ImGuiCol_FrameBg]                = (ImVec4){0.255f, 0.271f, 0.349f, 1.00f};     // #414559 (Surface0)
    colors[ImGuiCol_FrameBgHovered]         = (ImVec4){0.318f, 0.341f, 0.427f, 1.00f};     // #51576d (Surface1)
    colors[ImGuiCol_FrameBgActive]          = (ImVec4){0.384f, 0.408f, 0.502f, 1.00f};     // #626880 (Surface2)

    // Title bar
    colors[ImGuiCol_TitleBg]                = (ImVec4){0.137f, 0.149f, 0.204f, 1.00f};     // #232634 (Crust)
    colors[ImGuiCol_TitleBgActive]          = (ImVec4){0.161f, 0.173f, 0.235f, 1.00f};     // #292c3c (Mantle)
    colors[ImGuiCol_TitleBgCollapsed]       = (ImVec4){0.137f, 0.149f, 0.204f, 0.75f};     // #232634

    // Menu bar
    colors[ImGuiCol_MenuBarBg]              = (ImVec4){0.161f, 0.173f, 0.235f, 1.00f};     // #292c3c (Mantle)

    // Scrollbar
    colors[ImGuiCol_ScrollbarBg]            = (ImVec4){0.161f, 0.173f, 0.235f, 0.50f};     // #292c3c
    colors[ImGuiCol_ScrollbarGrab]          = (ImVec4){0.384f, 0.408f, 0.502f, 1.00f};     // #626880 (Surface2)
    colors[ImGuiCol_ScrollbarGrabHovered]   = (ImVec4){0.451f, 0.475f, 0.580f, 1.00f};     // #737994 (Overlay0)
    colors[ImGuiCol_ScrollbarGrabActive]    = (ImVec4){0.514f, 0.545f, 0.655f, 1.00f};     // #838ba7 (Overlay1)

    // Checkmark and sliders - Lavender accent
    colors[ImGuiCol_CheckMark]              = (ImVec4){0.729f, 0.733f, 0.945f, 1.00f};     // #babbf1 (Lavender)
    colors[ImGuiCol_SliderGrab]             = (ImVec4){0.729f, 0.733f, 0.945f, 1.00f};     // #babbf1 (Lavender)
    colors[ImGuiCol_SliderGrabActive]       = (ImVec4){0.549f, 0.667f, 0.933f, 1.00f};     // #8caaee (Blue)

    // Buttons
    colors[ImGuiCol_Button]                 = (ImVec4){0.318f, 0.341f, 0.427f, 1.00f};     // #51576d (Surface1)
    colors[ImGuiCol_ButtonHovered]          = (ImVec4){0.729f, 0.733f, 0.945f, 0.80f};     // #babbf1 (Lavender)
    colors[ImGuiCol_ButtonActive]           = (ImVec4){0.729f, 0.733f, 0.945f, 1.00f};     // #babbf1 (Lavender)

    // Headers
    colors[ImGuiCol_Header]                 = (ImVec4){0.318f, 0.341f, 0.427f, 1.00f};     // #51576d (Surface1)
    colors[ImGuiCol_HeaderHovered]          = (ImVec4){0.729f, 0.733f, 0.945f, 0.60f};     // #babbf1 (Lavender)
    colors[ImGuiCol_HeaderActive]           = (ImVec4){0.729f, 0.733f, 0.945f, 0.80f};     // #babbf1 (Lavender)

    // Separators
    colors[ImGuiCol_Separator]              = (ImVec4){0.384f, 0.408f, 0.502f, 1.00f};     // #626880 (Surface2)
    colors[ImGuiCol_SeparatorHovered]       = (ImVec4){0.729f, 0.733f, 0.945f, 0.60f};     // Lavender
    colors[ImGuiCol_SeparatorActive]        = (ImVec4){0.729f, 0.733f, 0.945f, 1.00f};     // Lavender

    // Resize grip
    colors[ImGuiCol_ResizeGrip]             = (ImVec4){0.384f, 0.408f, 0.502f, 0.50f};     // Surface2
    colors[ImGuiCol_ResizeGripHovered]      = (ImVec4){0.729f, 0.733f, 0.945f, 0.60f};     // Lavender
    colors[ImGuiCol_ResizeGripActive]       = (ImVec4){0.729f, 0.733f, 0.945f, 1.00f};     // Lavender

    // Tabs
    colors[ImGuiCol_Tab]                    = (ImVec4){0.161f, 0.173f, 0.235f, 1.00f};     // #292c3c (Mantle)
    colors[ImGuiCol_TabHovered]             = (ImVec4){0.729f, 0.733f, 0.945f, 0.80f};     // Lavender
    colors[ImGuiCol_TabSelected]            = (ImVec4){0.318f, 0.341f, 0.427f, 1.00f};     // #51576d (Surface1)
    colors[ImGuiCol_TabSelectedOverline]    = (ImVec4){0.729f, 0.733f, 0.945f, 1.00f};     // Lavender
    colors[ImGuiCol_TabDimmed]              = (ImVec4){0.137f, 0.149f, 0.204f, 1.00f};     // Crust
    colors[ImGuiCol_TabDimmedSelected]      = (ImVec4){0.255f, 0.271f, 0.349f, 1.00f};     // Surface0
    colors[ImGuiCol_TabDimmedSelectedOverline] = (ImVec4){0.514f, 0.545f, 0.655f, 1.00f}; // Overlay1

    // Docking
    colors[ImGuiCol_DockingPreview]         = (ImVec4){0.729f, 0.733f, 0.945f, 0.50f};     // Lavender
    colors[ImGuiCol_DockingEmptyBg]         = (ImVec4){0.137f, 0.149f, 0.204f, 1.00f};     // Crust

    // Plots - using theme accent colors
    colors[ImGuiCol_PlotLines]              = (ImVec4){0.549f, 0.667f, 0.933f, 1.00f};     // Blue
    colors[ImGuiCol_PlotLinesHovered]       = (ImVec4){0.906f, 0.510f, 0.518f, 1.00f};     // Red #e78284
    colors[ImGuiCol_PlotHistogram]          = (ImVec4){0.651f, 0.820f, 0.537f, 1.00f};     // Green #a6d189
    colors[ImGuiCol_PlotHistogramHovered]   = (ImVec4){0.898f, 0.784f, 0.565f, 1.00f};     // Yellow #e5c890

    // Tables
    colors[ImGuiCol_TableHeaderBg]          = (ImVec4){0.255f, 0.271f, 0.349f, 1.00f};     // Surface0
    colors[ImGuiCol_TableBorderStrong]      = (ImVec4){0.384f, 0.408f, 0.502f, 1.00f};     // Surface2
    colors[ImGuiCol_TableBorderLight]       = (ImVec4){0.318f, 0.341f, 0.427f, 1.00f};     // Surface1
    colors[ImGuiCol_TableRowBg]             = (ImVec4){0.00f, 0.00f, 0.00f, 0.00f};
    colors[ImGuiCol_TableRowBgAlt]          = (ImVec4){0.776f, 0.816f, 0.961f, 0.03f};     // Text @ 3%

    // Selection and highlights
    colors[ImGuiCol_TextSelectedBg]         = (ImVec4){0.729f, 0.733f, 0.945f, 0.30f};     // Lavender
    colors[ImGuiCol_DragDropTarget]         = (ImVec4){0.729f, 0.733f, 0.945f, 0.90f};     // Lavender
    colors[ImGuiCol_NavCursor]              = (ImVec4){0.729f, 0.733f, 0.945f, 1.00f};     // Lavender
    colors[ImGuiCol_NavWindowingHighlight]  = (ImVec4){0.776f, 0.816f, 0.961f, 0.70f};     // Text
    colors[ImGuiCol_NavWindowingDimBg]      = (ImVec4){0.137f, 0.149f, 0.204f, 0.50f};     // Crust
    colors[ImGuiCol_ModalWindowDimBg]       = (ImVec4){0.137f, 0.149f, 0.204f, 0.50f};     // Crust
}

//------------------------------------------------------------------------------
// Visual Studio Dark Theme
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

//------------------------------------------------------------------------------
// Theme Application Helper
//------------------------------------------------------------------------------

static inline void ui_theme_apply(ui_theme_t theme) {
    switch (theme) {
        case UI_THEME_VISUAL_STUDIO:
            ui_theme_apply_visual_studio();
            break;
        case UI_THEME_IOS_LIGHT:
            ui_theme_apply_ios_light();
            break;
        case UI_THEME_CATPPUCCIN_FRAPPE:
            ui_theme_apply_catppuccin_frappe();
            break;
        default:
            ui_theme_apply_catppuccin_frappe();
            break;
    }
}

// Get the current theme's FrameBg color (useful for viewport clear color)
static inline void ui_theme_get_frame_bg(float* r, float* g, float* b) {
    ImGuiStyle* style = igGetStyle();
    ImVec4 col = style->Colors[ImGuiCol_FrameBg];
    *r = col.x;
    *g = col.y;
    *b = col.z;
}

#endif // UI_THEME_H
