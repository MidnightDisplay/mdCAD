//------------------------------------------------------------------------------
// ui_theme.h - ImGui theme configuration (header-only)
//
// Available themes:
// - Visual Studio dark (blue-gray with rectangular styling)
// - iOS Light (clean white/gray with rounded corners and gray accents)
// - Catppuccin Frappé (muted pastel dark theme with lavender accent)
// - One Dark (Atom-inspired slate theme with blue accent)
// - Rose Moon (Rosé Pine Moon-inspired plum theme)
// - Nord (Arctic blue-gray theme)
// - Tokyo Storm (midnight indigo theme with neon blue accent)
// - Tokyo Night (deeper night-sky variant of Tokyo palette)
// - Gruvbox Material Dark (earthy warm dark theme)
// - Cyberpunk 2077 (black terminal with red body and yellow highlights)
// - shadcn/ui Dark (minimal black theme with monochrome accents)
//------------------------------------------------------------------------------
#ifndef UI_THEME_H
#define UI_THEME_H

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"

#include <stdbool.h>
#include <string.h>

//------------------------------------------------------------------------------
// Theme Types
//------------------------------------------------------------------------------
typedef enum {
    UI_THEME_VISUAL_STUDIO,
    UI_THEME_IOS_LIGHT,
    UI_THEME_CATPPUCCIN_FRAPPE,
    UI_THEME_ONE_DARK,
    UI_THEME_ROSE_MOON,
    UI_THEME_NORD,
    UI_THEME_TOKYO_STORM,
    UI_THEME_TOKYO_NIGHT,
    UI_THEME_GRUVBOX_MATERIAL_DARK,
    UI_THEME_CYBERPUNK_2077,
    UI_THEME_SHADCN_DARK,
    UI_THEME_COUNT
} ui_theme_t;

#define UI_THEME_DEFAULT UI_THEME_SHADCN_DARK

// Theme names for UI display
static const char* const ui_theme_names[UI_THEME_COUNT] = {
    "Visual Studio Dark",
    "iOS Light",
    "Catppuccin Frappe",
    "OneDark",
    "Rose Moon",
    "Nord",
    "Tokyo Storm",
    "Tokyo Night",
    "Gruvbox Material Dark",
    "Cyberpunk 2077",
    "shadcn/ui Dark"
};

static const char* const ui_theme_tokens[UI_THEME_COUNT] = {
    "visual-studio-dark",
    "ios-light",
    "catppuccin-frappe",
    "one-dark",
    "rose-moon",
    "nord",
    "tokyo-storm",
    "tokyo-night",
    "gruvbox-material-dark",
    "cyberpunk-2077",
    "shadcn-ui-dark"
};

static ui_theme_t g_ui_theme_current = UI_THEME_DEFAULT;

typedef struct {
    bool has_pending_theme;
    ui_theme_t pending_theme;
} ui_theme_settings_state_t;

static ui_theme_settings_state_t g_ui_theme_settings_state = {
    false,
    UI_THEME_DEFAULT
};

static inline ui_theme_t ui_theme_get_current(void) {
    return g_ui_theme_current;
}

static inline const char* ui_theme_get_token(ui_theme_t theme) {
    if ((theme < 0) || (theme >= UI_THEME_COUNT)) {
        theme = UI_THEME_DEFAULT;
    }
    return ui_theme_tokens[theme];
}

static inline bool ui_theme_try_parse_token(const char* token, ui_theme_t* out_theme) {
    if (!token || !out_theme) {
        return false;
    }

    for (int i = 0; i < (int)UI_THEME_COUNT; ++i) {
        if (strcmp(token, ui_theme_tokens[i]) == 0) {
            *out_theme = (ui_theme_t)i;
            return true;
        }
    }

    return false;
}

static inline void ui_theme_mark_settings_dirty(void) {
    igMarkIniSettingsDirty_Nil();
}

static inline void ui_theme_apply_zero_rounding(ImGuiStyle* style) {
    style->WindowRounding = 0.0f;
    style->FrameRounding = 0.0f;
    style->ScrollbarRounding = 0.0f;
    style->GrabRounding = 0.0f;
    style->TabRounding = 0.0f;
    style->ChildRounding = 0.0f;
    style->PopupRounding = 0.0f;
}

//------------------------------------------------------------------------------
// iOS Light Theme
//------------------------------------------------------------------------------

static inline void ui_theme_apply_ios_light(void) {
    ImGuiStyle* style = igGetStyle();

    g_ui_theme_current = UI_THEME_IOS_LIGHT;

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

    g_ui_theme_current = UI_THEME_CATPPUCCIN_FRAPPE;

    // Zero rounding (rectangular styling)
    ui_theme_apply_zero_rounding(style);

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
    colors[ImGuiCol_CheckMark]              = (ImVec4){0.792f, 0.619f, 0.902f, 1.00f};     // #babbf1 (Lavender)
    colors[ImGuiCol_SliderGrab]             = (ImVec4){0.792f, 0.619f, 0.902f, 1.00f};     // #babbf1 (Lavender)
    colors[ImGuiCol_SliderGrabActive]       = (ImVec4){0.549f, 0.667f, 0.933f, 1.00f};     // #8caaee (Blue)

    // Buttons
    colors[ImGuiCol_Button]                 = (ImVec4){0.318f, 0.341f, 0.427f, 1.00f};     // #51576d (Surface1)
    colors[ImGuiCol_ButtonHovered]          = (ImVec4){0.792f, 0.619f, 0.902f, 0.80f};     // #babbf1 (Lavender)
    colors[ImGuiCol_ButtonActive]           = (ImVec4){0.792f, 0.619f, 0.902f, 1.00f};     // #babbf1 (Lavender)

    // Headers
    colors[ImGuiCol_Header]                 = (ImVec4){0.318f, 0.341f, 0.427f, 1.00f};     // #51576d (Surface1)
    colors[ImGuiCol_HeaderHovered]          = (ImVec4){0.792f, 0.619f, 0.902f, 0.60f};     // #babbf1 (Lavender)
    colors[ImGuiCol_HeaderActive]           = (ImVec4){0.792f, 0.619f, 0.902f, 0.80f};     // #babbf1 (Lavender)

    // Separators
    colors[ImGuiCol_Separator]              = (ImVec4){0.384f, 0.408f, 0.502f, 1.00f};     // #626880 (Surface2)
    colors[ImGuiCol_SeparatorHovered]       = (ImVec4){0.792f, 0.619f, 0.902f, 0.60f};     // Lavender
    colors[ImGuiCol_SeparatorActive]        = (ImVec4){0.792f, 0.619f, 0.902f, 1.00f};     // Lavender

    // Resize grip
    colors[ImGuiCol_ResizeGrip]             = (ImVec4){0.384f, 0.408f, 0.502f, 0.50f};     // Surface2
    colors[ImGuiCol_ResizeGripHovered]      = (ImVec4){0.792f, 0.619f, 0.902f, 0.60f};     // Lavender
    colors[ImGuiCol_ResizeGripActive]       = (ImVec4){0.792f, 0.619f, 0.902f, 1.00f};     // Lavender

    // Tabs
    colors[ImGuiCol_Tab]                    = (ImVec4){0.161f, 0.173f, 0.235f, 1.00f};     // #292c3c (Mantle)
    colors[ImGuiCol_TabHovered]             = (ImVec4){0.792f, 0.619f, 0.902f, 0.80f};     // Lavender
    colors[ImGuiCol_TabSelected]            = (ImVec4){0.318f, 0.341f, 0.427f, 1.00f};     // #51576d (Surface1)
    colors[ImGuiCol_TabSelectedOverline]    = (ImVec4){0.792f, 0.619f, 0.902f, 1.00f};     // Lavender
    colors[ImGuiCol_TabDimmed]              = (ImVec4){0.137f, 0.149f, 0.204f, 1.00f};     // Crust
    colors[ImGuiCol_TabDimmedSelected]      = (ImVec4){0.255f, 0.271f, 0.349f, 1.00f};     // Surface0
    colors[ImGuiCol_TabDimmedSelectedOverline] = (ImVec4){0.514f, 0.545f, 0.655f, 1.00f}; // Overlay1

    // Docking
    colors[ImGuiCol_DockingPreview]         = (ImVec4){0.792f, 0.619f, 0.902f, 0.50f};     // Lavender
    colors[ImGuiCol_DockingEmptyBg]         = (ImVec4){0.137f, 0.149f, 0.204f, 1.00f};     // Crust

    // Plots - using theme accent colors
    colors[ImGuiCol_PlotLines]              = (ImVec4){0.549f, 0.667f, 0.933f, 1.00f};     // Blue
    colors[ImGuiCol_PlotLinesHovered]       = (ImVec4){0.906f, 0.510f, 0.518f, 1.00f};     // Red rgb(231, 130, 132)
    colors[ImGuiCol_PlotHistogram]          = (ImVec4){0.651f, 0.820f, 0.537f, 1.00f};     // Green #a6d189
    colors[ImGuiCol_PlotHistogramHovered]   = (ImVec4){0.898f, 0.784f, 0.565f, 1.00f};     // Yellow #e5c890

    // Tables
    colors[ImGuiCol_TableHeaderBg]          = (ImVec4){0.255f, 0.271f, 0.349f, 1.00f};     // Surface0
    colors[ImGuiCol_TableBorderStrong]      = (ImVec4){0.384f, 0.408f, 0.502f, 1.00f};     // Surface2
    colors[ImGuiCol_TableBorderLight]       = (ImVec4){0.318f, 0.341f, 0.427f, 1.00f};     // Surface1
    colors[ImGuiCol_TableRowBg]             = (ImVec4){0.00f, 0.00f, 0.00f, 0.00f};
    colors[ImGuiCol_TableRowBgAlt]          = (ImVec4){0.776f, 0.816f, 0.961f, 0.03f};     // Text @ 3%

    // Selection and highlights
    colors[ImGuiCol_TextSelectedBg]         = (ImVec4){0.792f, 0.619f, 0.902f, 0.30f};     // Lavender #BABAF1 #BABAF1 202, 158, 230 #ca9ee6 0.792f, 0.619f, 0.902f | 0.729f, 0.733f, 0.945f
    colors[ImGuiCol_DragDropTarget]         = (ImVec4){0.792f, 0.619f, 0.902f, 0.90f};     // Lavender
    colors[ImGuiCol_NavCursor]              = (ImVec4){0.792f, 0.619f, 0.902f, 1.00f};     // Lavender
    colors[ImGuiCol_NavWindowingHighlight]  = (ImVec4){0.776f, 0.816f, 0.961f, 0.70f};     // Text
    colors[ImGuiCol_NavWindowingDimBg]      = (ImVec4){0.137f, 0.149f, 0.204f, 0.50f};     // Crust
    colors[ImGuiCol_ModalWindowDimBg]       = (ImVec4){0.137f, 0.149f, 0.204f, 0.50f};     // Crust
}

//------------------------------------------------------------------------------
// One Dark Theme
//------------------------------------------------------------------------------

static inline void ui_theme_apply_one_dark(void) {
    ImGuiStyle* style = igGetStyle();
    ImVec4* colors = style->Colors;

    g_ui_theme_current = UI_THEME_ONE_DARK;
    ui_theme_apply_zero_rounding(style);

    colors[ImGuiCol_Text]                   = (ImVec4){0.671f, 0.698f, 0.749f, 1.00f};
    colors[ImGuiCol_TextDisabled]           = (ImVec4){0.361f, 0.388f, 0.439f, 1.00f};
    colors[ImGuiCol_WindowBg]               = (ImVec4){0.157f, 0.173f, 0.204f, 1.00f};
    colors[ImGuiCol_ChildBg]                = (ImVec4){0.157f, 0.173f, 0.204f, 1.00f};
    colors[ImGuiCol_PopupBg]                = (ImVec4){0.129f, 0.145f, 0.169f, 0.98f};
    colors[ImGuiCol_Border]                 = (ImVec4){0.243f, 0.267f, 0.318f, 1.00f};
    colors[ImGuiCol_BorderShadow]           = (ImVec4){0.00f, 0.00f, 0.00f, 0.00f};
    colors[ImGuiCol_FrameBg]                = (ImVec4){0.173f, 0.192f, 0.227f, 1.00f};
    colors[ImGuiCol_FrameBgHovered]         = (ImVec4){0.227f, 0.251f, 0.314f, 1.00f};
    colors[ImGuiCol_FrameBgActive]          = (ImVec4){0.380f, 0.686f, 0.937f, 0.35f};
    colors[ImGuiCol_TitleBg]                = (ImVec4){0.129f, 0.145f, 0.169f, 1.00f};
    colors[ImGuiCol_TitleBgActive]          = (ImVec4){0.173f, 0.192f, 0.227f, 1.00f};
    colors[ImGuiCol_TitleBgCollapsed]       = (ImVec4){0.129f, 0.145f, 0.169f, 0.75f};
    colors[ImGuiCol_MenuBarBg]              = (ImVec4){0.129f, 0.145f, 0.169f, 1.00f};
    colors[ImGuiCol_ScrollbarBg]            = (ImVec4){0.129f, 0.145f, 0.169f, 0.50f};
    colors[ImGuiCol_ScrollbarGrab]          = (ImVec4){0.243f, 0.267f, 0.318f, 1.00f};
    colors[ImGuiCol_ScrollbarGrabHovered]   = (ImVec4){0.294f, 0.322f, 0.388f, 1.00f};
    colors[ImGuiCol_ScrollbarGrabActive]    = (ImVec4){0.361f, 0.388f, 0.439f, 1.00f};
    colors[ImGuiCol_CheckMark]              = (ImVec4){0.380f, 0.686f, 0.937f, 1.00f};
    colors[ImGuiCol_SliderGrab]             = (ImVec4){0.380f, 0.686f, 0.937f, 1.00f};
    colors[ImGuiCol_SliderGrabActive]       = (ImVec4){0.596f, 0.765f, 0.475f, 1.00f};
    colors[ImGuiCol_Button]                 = (ImVec4){0.243f, 0.267f, 0.318f, 1.00f};
    colors[ImGuiCol_ButtonHovered]          = (ImVec4){0.380f, 0.686f, 0.937f, 0.55f};
    colors[ImGuiCol_ButtonActive]           = (ImVec4){0.380f, 0.686f, 0.937f, 0.80f};
    colors[ImGuiCol_Header]                 = (ImVec4){0.173f, 0.192f, 0.227f, 1.00f};
    colors[ImGuiCol_HeaderHovered]          = (ImVec4){0.380f, 0.686f, 0.937f, 0.50f};
    colors[ImGuiCol_HeaderActive]           = (ImVec4){0.380f, 0.686f, 0.937f, 0.80f};
    colors[ImGuiCol_Separator]              = (ImVec4){0.243f, 0.267f, 0.318f, 1.00f};
    colors[ImGuiCol_SeparatorHovered]       = (ImVec4){0.380f, 0.686f, 0.937f, 0.55f};
    colors[ImGuiCol_SeparatorActive]        = (ImVec4){0.380f, 0.686f, 0.937f, 0.90f};
    colors[ImGuiCol_ResizeGrip]             = (ImVec4){0.243f, 0.267f, 0.318f, 0.60f};
    colors[ImGuiCol_ResizeGripHovered]      = (ImVec4){0.380f, 0.686f, 0.937f, 0.55f};
    colors[ImGuiCol_ResizeGripActive]       = (ImVec4){0.380f, 0.686f, 0.937f, 0.90f};
    colors[ImGuiCol_Tab]                    = (ImVec4){0.129f, 0.145f, 0.169f, 1.00f};
    colors[ImGuiCol_TabHovered]             = (ImVec4){0.380f, 0.686f, 0.937f, 0.55f};
    colors[ImGuiCol_TabSelected]            = (ImVec4){0.173f, 0.192f, 0.227f, 1.00f};
    colors[ImGuiCol_TabSelectedOverline]    = (ImVec4){0.380f, 0.686f, 0.937f, 1.00f};
    colors[ImGuiCol_TabDimmed]              = (ImVec4){0.106f, 0.114f, 0.137f, 1.00f};
    colors[ImGuiCol_TabDimmedSelected]      = (ImVec4){0.157f, 0.173f, 0.204f, 1.00f};
    colors[ImGuiCol_TabDimmedSelectedOverline] = (ImVec4){0.361f, 0.388f, 0.439f, 1.00f};
    colors[ImGuiCol_DockingPreview]         = (ImVec4){0.380f, 0.686f, 0.937f, 0.45f};
    colors[ImGuiCol_DockingEmptyBg]         = (ImVec4){0.118f, 0.129f, 0.153f, 1.00f};
    colors[ImGuiCol_PlotLines]              = (ImVec4){0.380f, 0.686f, 0.937f, 1.00f};
    colors[ImGuiCol_PlotLinesHovered]       = (ImVec4){0.820f, 0.604f, 0.400f, 1.00f};
    colors[ImGuiCol_PlotHistogram]          = (ImVec4){0.596f, 0.765f, 0.475f, 1.00f};
    colors[ImGuiCol_PlotHistogramHovered]   = (ImVec4){0.898f, 0.753f, 0.482f, 1.00f};
    colors[ImGuiCol_TableHeaderBg]          = (ImVec4){0.173f, 0.192f, 0.227f, 1.00f};
    colors[ImGuiCol_TableBorderStrong]      = (ImVec4){0.243f, 0.267f, 0.318f, 1.00f};
    colors[ImGuiCol_TableBorderLight]       = (ImVec4){0.173f, 0.192f, 0.227f, 1.00f};
    colors[ImGuiCol_TableRowBg]             = (ImVec4){0.00f, 0.00f, 0.00f, 0.00f};
    colors[ImGuiCol_TableRowBgAlt]          = (ImVec4){0.671f, 0.698f, 0.749f, 0.03f};
    colors[ImGuiCol_TextSelectedBg]         = (ImVec4){0.380f, 0.686f, 0.937f, 0.35f};
    colors[ImGuiCol_DragDropTarget]         = (ImVec4){0.380f, 0.686f, 0.937f, 0.90f};
    colors[ImGuiCol_NavCursor]              = (ImVec4){0.380f, 0.686f, 0.937f, 1.00f};
    colors[ImGuiCol_NavWindowingHighlight]  = (ImVec4){0.671f, 0.698f, 0.749f, 0.70f};
    colors[ImGuiCol_NavWindowingDimBg]      = (ImVec4){0.118f, 0.129f, 0.153f, 0.20f};
    colors[ImGuiCol_ModalWindowDimBg]       = (ImVec4){0.118f, 0.129f, 0.153f, 0.40f};
}

//------------------------------------------------------------------------------
// Rose Moon Theme
//------------------------------------------------------------------------------

static inline void ui_theme_apply_rose_moon(void) {
    ImGuiStyle* style = igGetStyle();
    ImVec4* colors = style->Colors;

    g_ui_theme_current = UI_THEME_ROSE_MOON;
    ui_theme_apply_zero_rounding(style);

    colors[ImGuiCol_Text]                   = (ImVec4){0.878f, 0.871f, 0.957f, 1.00f};
    colors[ImGuiCol_TextDisabled]           = (ImVec4){0.565f, 0.549f, 0.667f, 1.00f};
    colors[ImGuiCol_WindowBg]               = (ImVec4){0.137f, 0.129f, 0.212f, 1.00f};
    colors[ImGuiCol_ChildBg]                = (ImVec4){0.137f, 0.129f, 0.212f, 1.00f};
    colors[ImGuiCol_PopupBg]                = (ImVec4){0.165f, 0.153f, 0.247f, 0.98f};
    colors[ImGuiCol_Border]                 = (ImVec4){0.267f, 0.255f, 0.353f, 1.00f};
    colors[ImGuiCol_BorderShadow]           = (ImVec4){0.00f, 0.00f, 0.00f, 0.00f};
    colors[ImGuiCol_FrameBg]                = (ImVec4){0.165f, 0.153f, 0.247f, 1.00f};
    colors[ImGuiCol_FrameBgHovered]         = (ImVec4){0.224f, 0.208f, 0.322f, 1.00f};
    colors[ImGuiCol_FrameBgActive]          = (ImVec4){0.769f, 0.655f, 0.906f, 0.32f};
    colors[ImGuiCol_TitleBg]                = (ImVec4){0.122f, 0.114f, 0.180f, 1.00f};
    colors[ImGuiCol_TitleBgActive]          = (ImVec4){0.165f, 0.153f, 0.247f, 1.00f};
    colors[ImGuiCol_TitleBgCollapsed]       = (ImVec4){0.122f, 0.114f, 0.180f, 0.75f};
    colors[ImGuiCol_MenuBarBg]              = (ImVec4){0.122f, 0.114f, 0.180f, 1.00f};
    colors[ImGuiCol_ScrollbarBg]            = (ImVec4){0.122f, 0.114f, 0.180f, 0.50f};
    colors[ImGuiCol_ScrollbarGrab]          = (ImVec4){0.267f, 0.255f, 0.353f, 1.00f};
    colors[ImGuiCol_ScrollbarGrabHovered]   = (ImVec4){0.431f, 0.416f, 0.525f, 1.00f};
    colors[ImGuiCol_ScrollbarGrabActive]    = (ImVec4){0.565f, 0.549f, 0.667f, 1.00f};
    colors[ImGuiCol_CheckMark]              = (ImVec4){0.769f, 0.655f, 0.906f, 1.00f};
    colors[ImGuiCol_SliderGrab]             = (ImVec4){0.769f, 0.655f, 0.906f, 1.00f};
    colors[ImGuiCol_SliderGrabActive]       = (ImVec4){0.965f, 0.757f, 0.467f, 1.00f};
    colors[ImGuiCol_Button]                 = (ImVec4){0.224f, 0.208f, 0.322f, 1.00f};
    colors[ImGuiCol_ButtonHovered]          = (ImVec4){0.769f, 0.655f, 0.906f, 0.55f};
    colors[ImGuiCol_ButtonActive]           = (ImVec4){0.769f, 0.655f, 0.906f, 0.80f};
    colors[ImGuiCol_Header]                 = (ImVec4){0.224f, 0.208f, 0.322f, 1.00f};
    colors[ImGuiCol_HeaderHovered]          = (ImVec4){0.769f, 0.655f, 0.906f, 0.45f};
    colors[ImGuiCol_HeaderActive]           = (ImVec4){0.769f, 0.655f, 0.906f, 0.75f};
    colors[ImGuiCol_Separator]              = (ImVec4){0.267f, 0.255f, 0.353f, 1.00f};
    colors[ImGuiCol_SeparatorHovered]       = (ImVec4){0.769f, 0.655f, 0.906f, 0.55f};
    colors[ImGuiCol_SeparatorActive]        = (ImVec4){0.965f, 0.757f, 0.467f, 0.90f};
    colors[ImGuiCol_ResizeGrip]             = (ImVec4){0.267f, 0.255f, 0.353f, 0.60f};
    colors[ImGuiCol_ResizeGripHovered]      = (ImVec4){0.769f, 0.655f, 0.906f, 0.55f};
    colors[ImGuiCol_ResizeGripActive]       = (ImVec4){0.965f, 0.757f, 0.467f, 0.90f};
    colors[ImGuiCol_Tab]                    = (ImVec4){0.122f, 0.114f, 0.180f, 1.00f};
    colors[ImGuiCol_TabHovered]             = (ImVec4){0.769f, 0.655f, 0.906f, 0.55f};
    colors[ImGuiCol_TabSelected]            = (ImVec4){0.224f, 0.208f, 0.322f, 1.00f};
    colors[ImGuiCol_TabSelectedOverline]    = (ImVec4){0.965f, 0.757f, 0.467f, 1.00f};
    colors[ImGuiCol_TabDimmed]              = (ImVec4){0.102f, 0.094f, 0.145f, 1.00f};
    colors[ImGuiCol_TabDimmedSelected]      = (ImVec4){0.165f, 0.153f, 0.247f, 1.00f};
    colors[ImGuiCol_TabDimmedSelectedOverline] = (ImVec4){0.431f, 0.416f, 0.525f, 1.00f};
    colors[ImGuiCol_DockingPreview]         = (ImVec4){0.769f, 0.655f, 0.906f, 0.45f};
    colors[ImGuiCol_DockingEmptyBg]         = (ImVec4){0.122f, 0.114f, 0.180f, 1.00f};
    colors[ImGuiCol_PlotLines]              = (ImVec4){0.243f, 0.561f, 0.690f, 1.00f};
    colors[ImGuiCol_PlotLinesHovered]       = (ImVec4){0.922f, 0.435f, 0.573f, 1.00f};
    colors[ImGuiCol_PlotHistogram]          = (ImVec4){0.965f, 0.757f, 0.467f, 1.00f};
    colors[ImGuiCol_PlotHistogramHovered]   = (ImVec4){0.612f, 0.812f, 0.847f, 1.00f};
    colors[ImGuiCol_TableHeaderBg]          = (ImVec4){0.165f, 0.153f, 0.247f, 1.00f};
    colors[ImGuiCol_TableBorderStrong]      = (ImVec4){0.267f, 0.255f, 0.353f, 1.00f};
    colors[ImGuiCol_TableBorderLight]       = (ImVec4){0.224f, 0.208f, 0.322f, 1.00f};
    colors[ImGuiCol_TableRowBg]             = (ImVec4){0.00f, 0.00f, 0.00f, 0.00f};
    colors[ImGuiCol_TableRowBgAlt]          = (ImVec4){0.878f, 0.871f, 0.957f, 0.03f};
    colors[ImGuiCol_TextSelectedBg]         = (ImVec4){0.769f, 0.655f, 0.906f, 0.35f};
    colors[ImGuiCol_DragDropTarget]         = (ImVec4){0.965f, 0.757f, 0.467f, 0.90f};
    colors[ImGuiCol_NavCursor]              = (ImVec4){0.769f, 0.655f, 0.906f, 1.00f};
    colors[ImGuiCol_NavWindowingHighlight]  = (ImVec4){0.878f, 0.871f, 0.957f, 0.70f};
    colors[ImGuiCol_NavWindowingDimBg]      = (ImVec4){0.122f, 0.114f, 0.180f, 0.45f};
    colors[ImGuiCol_ModalWindowDimBg]       = (ImVec4){0.122f, 0.114f, 0.180f, 0.55f};
}

//------------------------------------------------------------------------------
// Nord Theme
//------------------------------------------------------------------------------

static inline void ui_theme_apply_nord(void) {
    ImGuiStyle* style = igGetStyle();
    ImVec4* colors = style->Colors;

    g_ui_theme_current = UI_THEME_NORD;
    ui_theme_apply_zero_rounding(style);

    colors[ImGuiCol_Text]                   = (ImVec4){0.925f, 0.937f, 0.957f, 1.00f};
    colors[ImGuiCol_TextDisabled]           = (ImVec4){0.655f, 0.694f, 0.761f, 1.00f};
    colors[ImGuiCol_WindowBg]               = (ImVec4){0.180f, 0.204f, 0.251f, 1.00f};
    colors[ImGuiCol_ChildBg]                = (ImVec4){0.180f, 0.204f, 0.251f, 1.00f};
    colors[ImGuiCol_PopupBg]                = (ImVec4){0.231f, 0.259f, 0.322f, 0.98f};
    colors[ImGuiCol_Border]                 = (ImVec4){0.298f, 0.337f, 0.416f, 1.00f};
    colors[ImGuiCol_BorderShadow]           = (ImVec4){0.00f, 0.00f, 0.00f, 0.00f};
    colors[ImGuiCol_FrameBg]                = (ImVec4){0.231f, 0.259f, 0.322f, 1.00f};
    colors[ImGuiCol_FrameBgHovered]         = (ImVec4){0.263f, 0.298f, 0.369f, 1.00f};
    colors[ImGuiCol_FrameBgActive]          = (ImVec4){0.506f, 0.631f, 0.757f, 0.35f};
    colors[ImGuiCol_TitleBg]                = (ImVec4){0.169f, 0.188f, 0.231f, 1.00f};
    colors[ImGuiCol_TitleBgActive]          = (ImVec4){0.231f, 0.259f, 0.322f, 1.00f};
    colors[ImGuiCol_TitleBgCollapsed]       = (ImVec4){0.169f, 0.188f, 0.231f, 0.75f};
    colors[ImGuiCol_MenuBarBg]              = (ImVec4){0.169f, 0.188f, 0.231f, 1.00f};
    colors[ImGuiCol_ScrollbarBg]            = (ImVec4){0.169f, 0.188f, 0.231f, 0.50f};
    colors[ImGuiCol_ScrollbarGrab]          = (ImVec4){0.298f, 0.337f, 0.416f, 1.00f};
    colors[ImGuiCol_ScrollbarGrabHovered]   = (ImVec4){0.369f, 0.506f, 0.675f, 1.00f};
    colors[ImGuiCol_ScrollbarGrabActive]    = (ImVec4){0.533f, 0.753f, 0.816f, 1.00f};
    colors[ImGuiCol_CheckMark]              = (ImVec4){0.533f, 0.753f, 0.816f, 1.00f};
    colors[ImGuiCol_SliderGrab]             = (ImVec4){0.506f, 0.631f, 0.757f, 1.00f};
    colors[ImGuiCol_SliderGrabActive]       = (ImVec4){0.533f, 0.753f, 0.816f, 1.00f};
    colors[ImGuiCol_Button]                 = (ImVec4){0.263f, 0.298f, 0.369f, 1.00f};
    colors[ImGuiCol_ButtonHovered]          = (ImVec4){0.369f, 0.506f, 0.675f, 0.65f};
    colors[ImGuiCol_ButtonActive]           = (ImVec4){0.533f, 0.753f, 0.816f, 0.80f};
    colors[ImGuiCol_Header]                 = (ImVec4){0.263f, 0.298f, 0.369f, 1.00f};
    colors[ImGuiCol_HeaderHovered]          = (ImVec4){0.369f, 0.506f, 0.675f, 0.60f};
    colors[ImGuiCol_HeaderActive]           = (ImVec4){0.506f, 0.631f, 0.757f, 0.80f};
    colors[ImGuiCol_Separator]              = (ImVec4){0.298f, 0.337f, 0.416f, 1.00f};
    colors[ImGuiCol_SeparatorHovered]       = (ImVec4){0.533f, 0.753f, 0.816f, 0.55f};
    colors[ImGuiCol_SeparatorActive]        = (ImVec4){0.533f, 0.753f, 0.816f, 0.90f};
    colors[ImGuiCol_ResizeGrip]             = (ImVec4){0.298f, 0.337f, 0.416f, 0.60f};
    colors[ImGuiCol_ResizeGripHovered]      = (ImVec4){0.533f, 0.753f, 0.816f, 0.55f};
    colors[ImGuiCol_ResizeGripActive]       = (ImVec4){0.533f, 0.753f, 0.816f, 0.90f};
    colors[ImGuiCol_Tab]                    = (ImVec4){0.169f, 0.188f, 0.231f, 1.00f};
    colors[ImGuiCol_TabHovered]             = (ImVec4){0.369f, 0.506f, 0.675f, 0.70f};
    colors[ImGuiCol_TabSelected]            = (ImVec4){0.263f, 0.298f, 0.369f, 1.00f};
    colors[ImGuiCol_TabSelectedOverline]    = (ImVec4){0.533f, 0.753f, 0.816f, 1.00f};
    colors[ImGuiCol_TabDimmed]              = (ImVec4){0.141f, 0.161f, 0.200f, 1.00f};
    colors[ImGuiCol_TabDimmedSelected]      = (ImVec4){0.231f, 0.259f, 0.322f, 1.00f};
    colors[ImGuiCol_TabDimmedSelectedOverline] = (ImVec4){0.298f, 0.337f, 0.416f, 1.00f};
    colors[ImGuiCol_DockingPreview]         = (ImVec4){0.533f, 0.753f, 0.816f, 0.40f};
    colors[ImGuiCol_DockingEmptyBg]         = (ImVec4){0.141f, 0.161f, 0.200f, 1.00f};
    colors[ImGuiCol_PlotLines]              = (ImVec4){0.533f, 0.753f, 0.816f, 1.00f};
    colors[ImGuiCol_PlotLinesHovered]       = (ImVec4){0.749f, 0.380f, 0.416f, 1.00f};
    colors[ImGuiCol_PlotHistogram]          = (ImVec4){0.639f, 0.745f, 0.549f, 1.00f};
    colors[ImGuiCol_PlotHistogramHovered]   = (ImVec4){0.922f, 0.796f, 0.545f, 1.00f};
    colors[ImGuiCol_TableHeaderBg]          = (ImVec4){0.231f, 0.259f, 0.322f, 1.00f};
    colors[ImGuiCol_TableBorderStrong]      = (ImVec4){0.298f, 0.337f, 0.416f, 1.00f};
    colors[ImGuiCol_TableBorderLight]       = (ImVec4){0.263f, 0.298f, 0.369f, 1.00f};
    colors[ImGuiCol_TableRowBg]             = (ImVec4){0.00f, 0.00f, 0.00f, 0.00f};
    colors[ImGuiCol_TableRowBgAlt]          = (ImVec4){0.925f, 0.937f, 0.957f, 0.03f};
    colors[ImGuiCol_TextSelectedBg]         = (ImVec4){0.533f, 0.753f, 0.816f, 0.35f};
    colors[ImGuiCol_DragDropTarget]         = (ImVec4){0.533f, 0.753f, 0.816f, 0.90f};
    colors[ImGuiCol_NavCursor]              = (ImVec4){0.533f, 0.753f, 0.816f, 1.00f};
    colors[ImGuiCol_NavWindowingHighlight]  = (ImVec4){0.925f, 0.937f, 0.957f, 0.70f};
    colors[ImGuiCol_NavWindowingDimBg]      = (ImVec4){0.141f, 0.161f, 0.200f, 0.45f};
    colors[ImGuiCol_ModalWindowDimBg]       = (ImVec4){0.141f, 0.161f, 0.200f, 0.55f};
}

//------------------------------------------------------------------------------
// Tokyo Storm Theme
//------------------------------------------------------------------------------

static inline void ui_theme_apply_tokyo_storm(void) {
    ImGuiStyle* style = igGetStyle();
    ImVec4* colors = style->Colors;

    g_ui_theme_current = UI_THEME_TOKYO_STORM;
    ui_theme_apply_zero_rounding(style);

    colors[ImGuiCol_Text]                   = (ImVec4){0.753f, 0.792f, 0.961f, 1.00f};
    colors[ImGuiCol_TextDisabled]           = (ImVec4){0.337f, 0.373f, 0.537f, 1.00f};
    colors[ImGuiCol_WindowBg]               = (ImVec4){0.141f, 0.157f, 0.231f, 1.00f};
    colors[ImGuiCol_ChildBg]                = (ImVec4){0.141f, 0.157f, 0.231f, 1.00f};
    colors[ImGuiCol_PopupBg]                = (ImVec4){0.122f, 0.137f, 0.208f, 0.98f};
    colors[ImGuiCol_Border]                 = (ImVec4){0.255f, 0.282f, 0.408f, 1.00f};
    colors[ImGuiCol_BorderShadow]           = (ImVec4){0.00f, 0.00f, 0.00f, 0.00f};
    colors[ImGuiCol_FrameBg]                = (ImVec4){0.161f, 0.180f, 0.259f, 1.00f};
    colors[ImGuiCol_FrameBgHovered]         = (ImVec4){0.184f, 0.208f, 0.310f, 1.00f};
    colors[ImGuiCol_FrameBgActive]          = (ImVec4){0.478f, 0.635f, 0.969f, 0.35f};
    colors[ImGuiCol_TitleBg]                = (ImVec4){0.122f, 0.137f, 0.208f, 1.00f};
    colors[ImGuiCol_TitleBgActive]          = (ImVec4){0.161f, 0.180f, 0.259f, 1.00f};
    colors[ImGuiCol_TitleBgCollapsed]       = (ImVec4){0.122f, 0.137f, 0.208f, 0.75f};
    colors[ImGuiCol_MenuBarBg]              = (ImVec4){0.122f, 0.137f, 0.208f, 1.00f};
    colors[ImGuiCol_ScrollbarBg]            = (ImVec4){0.122f, 0.137f, 0.208f, 0.50f};
    colors[ImGuiCol_ScrollbarGrab]          = (ImVec4){0.255f, 0.282f, 0.408f, 1.00f};
    colors[ImGuiCol_ScrollbarGrabHovered]   = (ImVec4){0.478f, 0.635f, 0.969f, 0.80f};
    colors[ImGuiCol_ScrollbarGrabActive]    = (ImVec4){0.451f, 0.855f, 0.792f, 1.00f};
    colors[ImGuiCol_CheckMark]              = (ImVec4){0.478f, 0.635f, 0.969f, 1.00f};
    colors[ImGuiCol_SliderGrab]             = (ImVec4){0.478f, 0.635f, 0.969f, 1.00f};
    colors[ImGuiCol_SliderGrabActive]       = (ImVec4){0.451f, 0.855f, 0.792f, 1.00f};
    colors[ImGuiCol_Button]                 = (ImVec4){0.184f, 0.208f, 0.310f, 1.00f};
    colors[ImGuiCol_ButtonHovered]          = (ImVec4){0.478f, 0.635f, 0.969f, 0.60f};
    colors[ImGuiCol_ButtonActive]           = (ImVec4){0.478f, 0.635f, 0.969f, 0.80f};
    colors[ImGuiCol_Header]                 = (ImVec4){0.184f, 0.208f, 0.310f, 1.00f};
    colors[ImGuiCol_HeaderHovered]          = (ImVec4){0.733f, 0.604f, 0.969f, 0.45f};
    colors[ImGuiCol_HeaderActive]           = (ImVec4){0.478f, 0.635f, 0.969f, 0.80f};
    colors[ImGuiCol_Separator]              = (ImVec4){0.255f, 0.282f, 0.408f, 1.00f};
    colors[ImGuiCol_SeparatorHovered]       = (ImVec4){0.478f, 0.635f, 0.969f, 0.55f};
    colors[ImGuiCol_SeparatorActive]        = (ImVec4){0.451f, 0.855f, 0.792f, 0.90f};
    colors[ImGuiCol_ResizeGrip]             = (ImVec4){0.255f, 0.282f, 0.408f, 0.60f};
    colors[ImGuiCol_ResizeGripHovered]      = (ImVec4){0.478f, 0.635f, 0.969f, 0.55f};
    colors[ImGuiCol_ResizeGripActive]       = (ImVec4){0.451f, 0.855f, 0.792f, 0.90f};
    colors[ImGuiCol_Tab]                    = (ImVec4){0.122f, 0.137f, 0.208f, 1.00f};
    colors[ImGuiCol_TabHovered]             = (ImVec4){0.478f, 0.635f, 0.969f, 0.55f};
    colors[ImGuiCol_TabSelected]            = (ImVec4){0.161f, 0.180f, 0.259f, 1.00f};
    colors[ImGuiCol_TabSelectedOverline]    = (ImVec4){0.451f, 0.855f, 0.792f, 1.00f};
    colors[ImGuiCol_TabDimmed]              = (ImVec4){0.094f, 0.106f, 0.161f, 1.00f};
    colors[ImGuiCol_TabDimmedSelected]      = (ImVec4){0.141f, 0.157f, 0.231f, 1.00f};
    colors[ImGuiCol_TabDimmedSelectedOverline] = (ImVec4){0.255f, 0.282f, 0.408f, 1.00f};
    colors[ImGuiCol_DockingPreview]         = (ImVec4){0.478f, 0.635f, 0.969f, 0.45f};
    colors[ImGuiCol_DockingEmptyBg]         = (ImVec4){0.094f, 0.106f, 0.161f, 1.00f};
    colors[ImGuiCol_PlotLines]              = (ImVec4){0.451f, 0.855f, 0.792f, 1.00f};
    colors[ImGuiCol_PlotLinesHovered]       = (ImVec4){1.000f, 0.620f, 0.392f, 1.00f};
    colors[ImGuiCol_PlotHistogram]          = (ImVec4){0.620f, 0.808f, 0.416f, 1.00f};
    colors[ImGuiCol_PlotHistogramHovered]   = (ImVec4){0.878f, 0.686f, 0.408f, 1.00f};
    colors[ImGuiCol_TableHeaderBg]          = (ImVec4){0.161f, 0.180f, 0.259f, 1.00f};
    colors[ImGuiCol_TableBorderStrong]      = (ImVec4){0.255f, 0.282f, 0.408f, 1.00f};
    colors[ImGuiCol_TableBorderLight]       = (ImVec4){0.184f, 0.208f, 0.310f, 1.00f};
    colors[ImGuiCol_TableRowBg]             = (ImVec4){0.00f, 0.00f, 0.00f, 0.00f};
    colors[ImGuiCol_TableRowBgAlt]          = (ImVec4){0.753f, 0.792f, 0.961f, 0.03f};
    colors[ImGuiCol_TextSelectedBg]         = (ImVec4){0.478f, 0.635f, 0.969f, 0.35f};
    colors[ImGuiCol_DragDropTarget]         = (ImVec4){0.451f, 0.855f, 0.792f, 0.90f};
    colors[ImGuiCol_NavCursor]              = (ImVec4){0.478f, 0.635f, 0.969f, 1.00f};
    colors[ImGuiCol_NavWindowingHighlight]  = (ImVec4){0.753f, 0.792f, 0.961f, 0.70f};
    colors[ImGuiCol_NavWindowingDimBg]      = (ImVec4){0.094f, 0.106f, 0.161f, 0.45f};
    colors[ImGuiCol_ModalWindowDimBg]       = (ImVec4){0.094f, 0.106f, 0.161f, 0.55f};
}

//------------------------------------------------------------------------------
// Tokyo Night Theme
//------------------------------------------------------------------------------

static inline void ui_theme_apply_tokyo_night(void) {
    ImGuiStyle* style = igGetStyle();
    ImVec4* colors = style->Colors;

    g_ui_theme_current = UI_THEME_TOKYO_NIGHT;
    ui_theme_apply_zero_rounding(style);

    colors[ImGuiCol_Text]                   = (ImVec4){0.753f, 0.792f, 0.961f, 1.00f};
    colors[ImGuiCol_TextDisabled]           = (ImVec4){0.337f, 0.373f, 0.537f, 1.00f};
    colors[ImGuiCol_WindowBg]               = (ImVec4){0.102f, 0.106f, 0.149f, 1.00f};
    colors[ImGuiCol_ChildBg]                = (ImVec4){0.102f, 0.106f, 0.149f, 1.00f};
    colors[ImGuiCol_PopupBg]                = (ImVec4){0.086f, 0.086f, 0.118f, 0.98f};
    colors[ImGuiCol_Border]                 = (ImVec4){0.255f, 0.282f, 0.408f, 1.00f};
    colors[ImGuiCol_BorderShadow]           = (ImVec4){0.00f, 0.00f, 0.00f, 0.00f};
    colors[ImGuiCol_FrameBg]                = (ImVec4){0.122f, 0.137f, 0.208f, 1.00f};
    colors[ImGuiCol_FrameBgHovered]         = (ImVec4){0.161f, 0.180f, 0.259f, 1.00f};
    colors[ImGuiCol_FrameBgActive]          = (ImVec4){0.478f, 0.635f, 0.969f, 0.35f};
    colors[ImGuiCol_TitleBg]                = (ImVec4){0.067f, 0.071f, 0.106f, 1.00f};
    colors[ImGuiCol_TitleBgActive]          = (ImVec4){0.122f, 0.137f, 0.208f, 1.00f};
    colors[ImGuiCol_TitleBgCollapsed]       = (ImVec4){0.067f, 0.071f, 0.106f, 0.75f};
    colors[ImGuiCol_MenuBarBg]              = (ImVec4){0.067f, 0.071f, 0.106f, 1.00f};
    colors[ImGuiCol_ScrollbarBg]            = (ImVec4){0.067f, 0.071f, 0.106f, 0.50f};
    colors[ImGuiCol_ScrollbarGrab]          = (ImVec4){0.255f, 0.282f, 0.408f, 1.00f};
    colors[ImGuiCol_ScrollbarGrabHovered]   = (ImVec4){0.478f, 0.635f, 0.969f, 0.80f};
    colors[ImGuiCol_ScrollbarGrabActive]    = (ImVec4){0.733f, 0.604f, 0.969f, 1.00f};
    colors[ImGuiCol_CheckMark]              = (ImVec4){0.490f, 0.812f, 1.000f, 1.00f};
    colors[ImGuiCol_SliderGrab]             = (ImVec4){0.478f, 0.635f, 0.969f, 1.00f};
    colors[ImGuiCol_SliderGrabActive]       = (ImVec4){0.733f, 0.604f, 0.969f, 1.00f};
    colors[ImGuiCol_Button]                 = (ImVec4){0.161f, 0.180f, 0.259f, 1.00f};
    colors[ImGuiCol_ButtonHovered]          = (ImVec4){0.478f, 0.635f, 0.969f, 0.60f};
    colors[ImGuiCol_ButtonActive]           = (ImVec4){0.478f, 0.635f, 0.969f, 0.80f};
    colors[ImGuiCol_Header]                 = (ImVec4){0.161f, 0.180f, 0.259f, 1.00f};
    colors[ImGuiCol_HeaderHovered]          = (ImVec4){0.478f, 0.635f, 0.969f, 0.45f};
    colors[ImGuiCol_HeaderActive]           = (ImVec4){0.733f, 0.604f, 0.969f, 0.70f};
    colors[ImGuiCol_Separator]              = (ImVec4){0.255f, 0.282f, 0.408f, 1.00f};
    colors[ImGuiCol_SeparatorHovered]       = (ImVec4){0.478f, 0.635f, 0.969f, 0.55f};
    colors[ImGuiCol_SeparatorActive]        = (ImVec4){0.490f, 0.812f, 1.000f, 0.90f};
    colors[ImGuiCol_ResizeGrip]             = (ImVec4){0.255f, 0.282f, 0.408f, 0.60f};
    colors[ImGuiCol_ResizeGripHovered]      = (ImVec4){0.478f, 0.635f, 0.969f, 0.55f};
    colors[ImGuiCol_ResizeGripActive]       = (ImVec4){0.490f, 0.812f, 1.000f, 0.90f};
    colors[ImGuiCol_Tab]                    = (ImVec4){0.067f, 0.071f, 0.106f, 1.00f};
    colors[ImGuiCol_TabHovered]             = (ImVec4){0.478f, 0.635f, 0.969f, 0.55f};
    colors[ImGuiCol_TabSelected]            = (ImVec4){0.122f, 0.137f, 0.208f, 1.00f};
    colors[ImGuiCol_TabSelectedOverline]    = (ImVec4){0.733f, 0.604f, 0.969f, 1.00f};
    colors[ImGuiCol_TabDimmed]              = (ImVec4){0.059f, 0.063f, 0.090f, 1.00f};
    colors[ImGuiCol_TabDimmedSelected]      = (ImVec4){0.102f, 0.106f, 0.149f, 1.00f};
    colors[ImGuiCol_TabDimmedSelectedOverline] = (ImVec4){0.255f, 0.282f, 0.408f, 1.00f};
    colors[ImGuiCol_DockingPreview]         = (ImVec4){0.733f, 0.604f, 0.969f, 0.45f};
    colors[ImGuiCol_DockingEmptyBg]         = (ImVec4){0.059f, 0.063f, 0.090f, 1.00f};
    colors[ImGuiCol_PlotLines]              = (ImVec4){0.490f, 0.812f, 1.000f, 1.00f};
    colors[ImGuiCol_PlotLinesHovered]       = (ImVec4){1.000f, 0.620f, 0.392f, 1.00f};
    colors[ImGuiCol_PlotHistogram]          = (ImVec4){0.620f, 0.808f, 0.416f, 1.00f};
    colors[ImGuiCol_PlotHistogramHovered]   = (ImVec4){0.878f, 0.686f, 0.408f, 1.00f};
    colors[ImGuiCol_TableHeaderBg]          = (ImVec4){0.122f, 0.137f, 0.208f, 1.00f};
    colors[ImGuiCol_TableBorderStrong]      = (ImVec4){0.255f, 0.282f, 0.408f, 1.00f};
    colors[ImGuiCol_TableBorderLight]       = (ImVec4){0.161f, 0.180f, 0.259f, 1.00f};
    colors[ImGuiCol_TableRowBg]             = (ImVec4){0.00f, 0.00f, 0.00f, 0.00f};
    colors[ImGuiCol_TableRowBgAlt]          = (ImVec4){0.753f, 0.792f, 0.961f, 0.03f};
    colors[ImGuiCol_TextSelectedBg]         = (ImVec4){0.733f, 0.604f, 0.969f, 0.35f};
    colors[ImGuiCol_DragDropTarget]         = (ImVec4){0.490f, 0.812f, 1.000f, 0.90f};
    colors[ImGuiCol_NavCursor]              = (ImVec4){0.478f, 0.635f, 0.969f, 1.00f};
    colors[ImGuiCol_NavWindowingHighlight]  = (ImVec4){0.753f, 0.792f, 0.961f, 0.70f};
    colors[ImGuiCol_NavWindowingDimBg]      = (ImVec4){0.059f, 0.063f, 0.090f, 0.45f};
    colors[ImGuiCol_ModalWindowDimBg]       = (ImVec4){0.059f, 0.063f, 0.090f, 0.55f};
}

//------------------------------------------------------------------------------
// Gruvbox Material Dark Theme
//------------------------------------------------------------------------------

static inline void ui_theme_apply_gruvbox_material_dark(void) {
    ImGuiStyle* style = igGetStyle();
    ImVec4* colors = style->Colors;

    g_ui_theme_current = UI_THEME_GRUVBOX_MATERIAL_DARK;
    ui_theme_apply_zero_rounding(style);

    colors[ImGuiCol_Text]                   = (ImVec4){0.831f, 0.745f, 0.596f, 1.00f};
    colors[ImGuiCol_TextDisabled]           = (ImVec4){0.573f, 0.514f, 0.455f, 1.00f};
    colors[ImGuiCol_WindowBg]               = (ImVec4){0.157f, 0.157f, 0.157f, 1.00f};
    colors[ImGuiCol_ChildBg]                = (ImVec4){0.157f, 0.157f, 0.157f, 1.00f};
    colors[ImGuiCol_PopupBg]                = (ImVec4){0.114f, 0.125f, 0.129f, 0.98f};
    colors[ImGuiCol_Border]                 = (ImVec4){0.314f, 0.286f, 0.271f, 1.00f};
    colors[ImGuiCol_BorderShadow]           = (ImVec4){0.00f, 0.00f, 0.00f, 0.00f};
    colors[ImGuiCol_FrameBg]                = (ImVec4){0.196f, 0.188f, 0.184f, 1.00f};
    colors[ImGuiCol_FrameBgHovered]         = (ImVec4){0.235f, 0.219f, 0.212f, 1.00f};
    colors[ImGuiCol_FrameBgActive]          = (ImVec4){0.847f, 0.651f, 0.341f, 0.35f};
    colors[ImGuiCol_TitleBg]                = (ImVec4){0.114f, 0.125f, 0.129f, 1.00f};
    colors[ImGuiCol_TitleBgActive]          = (ImVec4){0.196f, 0.188f, 0.184f, 1.00f};
    colors[ImGuiCol_TitleBgCollapsed]       = (ImVec4){0.114f, 0.125f, 0.129f, 0.75f};
    colors[ImGuiCol_MenuBarBg]              = (ImVec4){0.114f, 0.125f, 0.129f, 1.00f};
    colors[ImGuiCol_ScrollbarBg]            = (ImVec4){0.114f, 0.125f, 0.129f, 0.50f};
    colors[ImGuiCol_ScrollbarGrab]          = (ImVec4){0.314f, 0.286f, 0.271f, 1.00f};
    colors[ImGuiCol_ScrollbarGrabHovered]   = (ImVec4){0.490f, 0.682f, 0.639f, 0.80f};
    colors[ImGuiCol_ScrollbarGrabActive]    = (ImVec4){0.663f, 0.714f, 0.396f, 1.00f};
    colors[ImGuiCol_CheckMark]              = (ImVec4){0.847f, 0.651f, 0.341f, 1.00f};
    colors[ImGuiCol_SliderGrab]             = (ImVec4){0.490f, 0.682f, 0.639f, 1.00f};
    colors[ImGuiCol_SliderGrabActive]       = (ImVec4){0.663f, 0.714f, 0.396f, 1.00f};
    colors[ImGuiCol_Button]                 = (ImVec4){0.235f, 0.219f, 0.212f, 1.00f};
    colors[ImGuiCol_ButtonHovered]          = (ImVec4){0.906f, 0.541f, 0.306f, 0.60f};
    colors[ImGuiCol_ButtonActive]           = (ImVec4){0.847f, 0.651f, 0.341f, 0.80f};
    colors[ImGuiCol_Header]                 = (ImVec4){0.235f, 0.219f, 0.212f, 1.00f};
    colors[ImGuiCol_HeaderHovered]          = (ImVec4){0.490f, 0.682f, 0.639f, 0.45f};
    colors[ImGuiCol_HeaderActive]           = (ImVec4){0.847f, 0.651f, 0.341f, 0.70f};
    colors[ImGuiCol_Separator]              = (ImVec4){0.314f, 0.286f, 0.271f, 1.00f};
    colors[ImGuiCol_SeparatorHovered]       = (ImVec4){0.906f, 0.541f, 0.306f, 0.55f};
    colors[ImGuiCol_SeparatorActive]        = (ImVec4){0.847f, 0.651f, 0.341f, 0.90f};
    colors[ImGuiCol_ResizeGrip]             = (ImVec4){0.314f, 0.286f, 0.271f, 0.60f};
    colors[ImGuiCol_ResizeGripHovered]      = (ImVec4){0.906f, 0.541f, 0.306f, 0.55f};
    colors[ImGuiCol_ResizeGripActive]       = (ImVec4){0.847f, 0.651f, 0.341f, 0.90f};
    colors[ImGuiCol_Tab]                    = (ImVec4){0.114f, 0.125f, 0.129f, 1.00f};
    colors[ImGuiCol_TabHovered]             = (ImVec4){0.906f, 0.541f, 0.306f, 0.55f};
    colors[ImGuiCol_TabSelected]            = (ImVec4){0.196f, 0.188f, 0.184f, 1.00f};
    colors[ImGuiCol_TabSelectedOverline]    = (ImVec4){0.847f, 0.651f, 0.341f, 1.00f};
    colors[ImGuiCol_TabDimmed]              = (ImVec4){0.078f, 0.086f, 0.090f, 1.00f};
    colors[ImGuiCol_TabDimmedSelected]      = (ImVec4){0.157f, 0.157f, 0.157f, 1.00f};
    colors[ImGuiCol_TabDimmedSelectedOverline] = (ImVec4){0.314f, 0.286f, 0.271f, 1.00f};
    colors[ImGuiCol_DockingPreview]         = (ImVec4){0.847f, 0.651f, 0.341f, 0.45f};
    colors[ImGuiCol_DockingEmptyBg]         = (ImVec4){0.078f, 0.086f, 0.090f, 1.00f};
    colors[ImGuiCol_PlotLines]              = (ImVec4){0.490f, 0.682f, 0.639f, 1.00f};
    colors[ImGuiCol_PlotLinesHovered]       = (ImVec4){0.918f, 0.412f, 0.384f, 1.00f};
    colors[ImGuiCol_PlotHistogram]          = (ImVec4){0.663f, 0.714f, 0.396f, 1.00f};
    colors[ImGuiCol_PlotHistogramHovered]   = (ImVec4){0.847f, 0.651f, 0.341f, 1.00f};
    colors[ImGuiCol_TableHeaderBg]          = (ImVec4){0.196f, 0.188f, 0.184f, 1.00f};
    colors[ImGuiCol_TableBorderStrong]      = (ImVec4){0.314f, 0.286f, 0.271f, 1.00f};
    colors[ImGuiCol_TableBorderLight]       = (ImVec4){0.235f, 0.219f, 0.212f, 1.00f};
    colors[ImGuiCol_TableRowBg]             = (ImVec4){0.00f, 0.00f, 0.00f, 0.00f};
    colors[ImGuiCol_TableRowBgAlt]          = (ImVec4){0.831f, 0.745f, 0.596f, 0.03f};
    colors[ImGuiCol_TextSelectedBg]         = (ImVec4){0.847f, 0.651f, 0.341f, 0.30f};
    colors[ImGuiCol_DragDropTarget]         = (ImVec4){0.847f, 0.651f, 0.341f, 0.90f};
    colors[ImGuiCol_NavCursor]              = (ImVec4){0.490f, 0.682f, 0.639f, 1.00f};
    colors[ImGuiCol_NavWindowingHighlight]  = (ImVec4){0.831f, 0.745f, 0.596f, 0.70f};
    colors[ImGuiCol_NavWindowingDimBg]      = (ImVec4){0.078f, 0.086f, 0.090f, 0.45f};
    colors[ImGuiCol_ModalWindowDimBg]       = (ImVec4){0.078f, 0.086f, 0.090f, 0.55f};
}

//------------------------------------------------------------------------------
// Cyberpunk 2077 Theme
//------------------------------------------------------------------------------

static inline void ui_theme_apply_cyberpunk_2077(void) {
    ImGuiStyle* style = igGetStyle();
    ImVec4* colors = style->Colors;

    g_ui_theme_current = UI_THEME_CYBERPUNK_2077;
    ui_theme_apply_zero_rounding(style);

    colors[ImGuiCol_Text]                   = (ImVec4){0.957f, 0.357f, 0.412f, 1.00f};
    colors[ImGuiCol_TextDisabled]           = (ImVec4){0.549f, 0.176f, 0.239f, 1.00f};
    colors[ImGuiCol_WindowBg]               = (ImVec4){0.031f, 0.031f, 0.031f, 1.00f};
    colors[ImGuiCol_ChildBg]                = (ImVec4){0.047f, 0.047f, 0.047f, 1.00f};
    colors[ImGuiCol_PopupBg]                = (ImVec4){0.059f, 0.059f, 0.059f, 0.98f};
    colors[ImGuiCol_Border]                 = (ImVec4){0.549f, 0.063f, 0.125f, 1.00f};
    colors[ImGuiCol_BorderShadow]           = (ImVec4){0.00f, 0.00f, 0.00f, 0.00f};
    colors[ImGuiCol_FrameBg]                = (ImVec4){0.075f, 0.075f, 0.075f, 1.00f};
    colors[ImGuiCol_FrameBgHovered]         = (ImVec4){0.106f, 0.063f, 0.071f, 1.00f};
    colors[ImGuiCol_FrameBgActive]          = (ImVec4){0.957f, 0.357f, 0.412f, 0.30f};
    colors[ImGuiCol_TitleBg]                = (ImVec4){0.043f, 0.043f, 0.043f, 1.00f};
    colors[ImGuiCol_TitleBgActive]          = (ImVec4){0.102f, 0.051f, 0.063f, 1.00f};
    colors[ImGuiCol_TitleBgCollapsed]       = (ImVec4){0.043f, 0.043f, 0.043f, 0.75f};
    colors[ImGuiCol_MenuBarBg]              = (ImVec4){0.043f, 0.043f, 0.043f, 1.00f};
    colors[ImGuiCol_ScrollbarBg]            = (ImVec4){0.043f, 0.043f, 0.043f, 0.50f};
    colors[ImGuiCol_ScrollbarGrab]          = (ImVec4){0.549f, 0.063f, 0.125f, 1.00f};
    colors[ImGuiCol_ScrollbarGrabHovered]   = (ImVec4){1.000f, 0.831f, 0.000f, 0.80f};
    colors[ImGuiCol_ScrollbarGrabActive]    = (ImVec4){1.000f, 0.239f, 0.353f, 1.00f};
    colors[ImGuiCol_CheckMark]              = (ImVec4){1.000f, 0.831f, 0.000f, 1.00f};
    colors[ImGuiCol_SliderGrab]             = (ImVec4){1.000f, 0.831f, 0.000f, 1.00f};
    colors[ImGuiCol_SliderGrabActive]       = (ImVec4){1.000f, 0.239f, 0.353f, 1.00f};
    colors[ImGuiCol_Button]                 = (ImVec4){0.086f, 0.043f, 0.051f, 1.00f};
    colors[ImGuiCol_ButtonHovered]          = (ImVec4){1.000f, 0.831f, 0.000f, 0.55f};
    colors[ImGuiCol_ButtonActive]           = (ImVec4){1.000f, 0.239f, 0.353f, 0.80f};
    colors[ImGuiCol_Header]                 = (ImVec4){0.102f, 0.051f, 0.063f, 1.00f};
    colors[ImGuiCol_HeaderHovered]          = (ImVec4){1.000f, 0.831f, 0.000f, 0.35f};
    colors[ImGuiCol_HeaderActive]           = (ImVec4){1.000f, 0.239f, 0.353f, 0.75f};
    colors[ImGuiCol_Separator]              = (ImVec4){0.549f, 0.063f, 0.125f, 1.00f};
    colors[ImGuiCol_SeparatorHovered]       = (ImVec4){1.000f, 0.831f, 0.000f, 0.60f};
    colors[ImGuiCol_SeparatorActive]        = (ImVec4){1.000f, 0.239f, 0.353f, 0.90f};
    colors[ImGuiCol_ResizeGrip]             = (ImVec4){0.549f, 0.063f, 0.125f, 0.60f};
    colors[ImGuiCol_ResizeGripHovered]      = (ImVec4){1.000f, 0.831f, 0.000f, 0.60f};
    colors[ImGuiCol_ResizeGripActive]       = (ImVec4){1.000f, 0.239f, 0.353f, 0.90f};
    colors[ImGuiCol_Tab]                    = (ImVec4){0.043f, 0.043f, 0.043f, 1.00f};
    colors[ImGuiCol_TabHovered]             = (ImVec4){1.000f, 0.831f, 0.000f, 0.45f};
    colors[ImGuiCol_TabSelected]            = (ImVec4){0.086f, 0.043f, 0.051f, 1.00f};
    colors[ImGuiCol_TabSelectedOverline]    = (ImVec4){1.000f, 0.831f, 0.000f, 1.00f};
    colors[ImGuiCol_TabDimmed]              = (ImVec4){0.027f, 0.027f, 0.027f, 1.00f};
    colors[ImGuiCol_TabDimmedSelected]      = (ImVec4){0.075f, 0.075f, 0.075f, 1.00f};
    colors[ImGuiCol_TabDimmedSelectedOverline] = (ImVec4){0.549f, 0.063f, 0.125f, 1.00f};
    colors[ImGuiCol_DockingPreview]         = (ImVec4){1.000f, 0.831f, 0.000f, 0.45f};
    colors[ImGuiCol_DockingEmptyBg]         = (ImVec4){0.020f, 0.020f, 0.020f, 1.00f};
    colors[ImGuiCol_PlotLines]              = (ImVec4){0.957f, 0.357f, 0.412f, 1.00f};
    colors[ImGuiCol_PlotLinesHovered]       = (ImVec4){1.000f, 0.831f, 0.000f, 1.00f};
    colors[ImGuiCol_PlotHistogram]          = (ImVec4){1.000f, 0.831f, 0.000f, 1.00f};
    colors[ImGuiCol_PlotHistogramHovered]   = (ImVec4){1.000f, 0.925f, 0.200f, 1.00f};
    colors[ImGuiCol_TableHeaderBg]          = (ImVec4){0.075f, 0.075f, 0.075f, 1.00f};
    colors[ImGuiCol_TableBorderStrong]      = (ImVec4){0.549f, 0.063f, 0.125f, 1.00f};
    colors[ImGuiCol_TableBorderLight]       = (ImVec4){0.102f, 0.051f, 0.063f, 1.00f};
    colors[ImGuiCol_TableRowBg]             = (ImVec4){0.00f, 0.00f, 0.00f, 0.00f};
    colors[ImGuiCol_TableRowBgAlt]          = (ImVec4){0.957f, 0.357f, 0.412f, 0.02f};
    colors[ImGuiCol_TextSelectedBg]         = (ImVec4){1.000f, 0.831f, 0.000f, 0.25f};
    colors[ImGuiCol_DragDropTarget]         = (ImVec4){1.000f, 0.831f, 0.000f, 0.90f};
    colors[ImGuiCol_NavCursor]              = (ImVec4){1.000f, 0.831f, 0.000f, 1.00f};
    colors[ImGuiCol_NavWindowingHighlight]  = (ImVec4){0.957f, 0.357f, 0.412f, 0.70f};
    colors[ImGuiCol_NavWindowingDimBg]      = (ImVec4){0.020f, 0.020f, 0.020f, 0.55f};
    colors[ImGuiCol_ModalWindowDimBg]       = (ImVec4){0.020f, 0.020f, 0.020f, 0.65f};
}

//------------------------------------------------------------------------------
// shadcn/ui Dark Theme
//------------------------------------------------------------------------------

static inline void ui_theme_apply_shadcn_dark(void) {
    ImGuiStyle* style = igGetStyle();
    ImVec4* colors = style->Colors;

    g_ui_theme_current = UI_THEME_SHADCN_DARK;
    ui_theme_apply_zero_rounding(style);

    colors[ImGuiCol_Text]                   = (ImVec4){0.980f, 0.980f, 0.980f, 1.00f};
    colors[ImGuiCol_TextDisabled]           = (ImVec4){0.639f, 0.639f, 0.639f, 1.00f};
    colors[ImGuiCol_WindowBg]               = (ImVec4){0.039f, 0.039f, 0.039f, 1.00f};
    colors[ImGuiCol_ChildBg]                = (ImVec4){0.039f, 0.039f, 0.039f, 1.00f};
    colors[ImGuiCol_PopupBg]                = (ImVec4){0.067f, 0.067f, 0.067f, 0.98f};
    colors[ImGuiCol_Border]                 = (ImVec4){0.149f, 0.149f, 0.149f, 1.00f};
    colors[ImGuiCol_BorderShadow]           = (ImVec4){0.00f, 0.00f, 0.00f, 0.00f};
    colors[ImGuiCol_FrameBg]                = (ImVec4){0.069f, 0.069f, 0.069f, 1.00f};
    colors[ImGuiCol_FrameBgHovered]         = (ImVec4){0.231f, 0.231f, 0.231f, 1.00f};
    colors[ImGuiCol_FrameBgActive]          = (ImVec4){0.831f, 0.831f, 0.831f, 0.18f};
    colors[ImGuiCol_TitleBg]                = (ImVec4){0.039f, 0.039f, 0.039f, 1.00f};
    colors[ImGuiCol_TitleBgActive]          = (ImVec4){0.067f, 0.067f, 0.067f, 1.00f};
    colors[ImGuiCol_TitleBgCollapsed]       = (ImVec4){0.039f, 0.039f, 0.039f, 0.75f};
    colors[ImGuiCol_MenuBarBg]              = (ImVec4){0.067f, 0.067f, 0.067f, 1.00f};
    colors[ImGuiCol_ScrollbarBg]            = (ImVec4){0.039f, 0.039f, 0.039f, 0.50f};
    colors[ImGuiCol_ScrollbarGrab]          = (ImVec4){0.231f, 0.231f, 0.231f, 1.00f};
    colors[ImGuiCol_ScrollbarGrabHovered]   = (ImVec4){0.451f, 0.451f, 0.451f, 0.85f};
    colors[ImGuiCol_ScrollbarGrabActive]    = (ImVec4){0.831f, 0.831f, 0.831f, 1.00f};
    colors[ImGuiCol_CheckMark]              = (ImVec4){0.980f, 0.980f, 0.980f, 1.00f};
    colors[ImGuiCol_SliderGrab]             = (ImVec4){0.831f, 0.831f, 0.831f, 1.00f};
    colors[ImGuiCol_SliderGrabActive]       = (ImVec4){0.980f, 0.980f, 0.980f, 1.00f};
    colors[ImGuiCol_Button]                 = (ImVec4){0.149f, 0.149f, 0.149f, 1.00f};
    colors[ImGuiCol_ButtonHovered]          = (ImVec4){0.231f, 0.231f, 0.231f, 1.00f};
    colors[ImGuiCol_ButtonActive]           = (ImVec4){0.322f, 0.322f, 0.322f, 1.00f};
    colors[ImGuiCol_Header]                 = (ImVec4){0.149f, 0.149f, 0.149f, 1.00f};
    colors[ImGuiCol_HeaderHovered]          = (ImVec4){0.831f, 0.831f, 0.831f, 0.22f};
    colors[ImGuiCol_HeaderActive]           = (ImVec4){0.980f, 0.980f, 0.980f, 0.32f};
    colors[ImGuiCol_Separator]              = (ImVec4){0.149f, 0.149f, 0.149f, 1.00f};
    colors[ImGuiCol_SeparatorHovered]       = (ImVec4){0.831f, 0.831f, 0.831f, 0.55f};
    colors[ImGuiCol_SeparatorActive]        = (ImVec4){0.980f, 0.980f, 0.980f, 0.85f};
    colors[ImGuiCol_ResizeGrip]             = (ImVec4){0.149f, 0.149f, 0.149f, 0.60f};
    colors[ImGuiCol_ResizeGripHovered]      = (ImVec4){0.831f, 0.831f, 0.831f, 0.55f};
    colors[ImGuiCol_ResizeGripActive]       = (ImVec4){0.980f, 0.980f, 0.980f, 0.90f};
    colors[ImGuiCol_Tab]                    = (ImVec4){0.039f, 0.039f, 0.039f, 1.00f};
    colors[ImGuiCol_TabHovered]             = (ImVec4){0.831f, 0.831f, 0.831f, 0.35f};
    colors[ImGuiCol_TabSelected]            = (ImVec4){0.149f, 0.149f, 0.149f, 1.00f};
    colors[ImGuiCol_TabSelectedOverline]    = (ImVec4){0.980f, 0.980f, 0.980f, 1.00f};
    colors[ImGuiCol_TabDimmed]              = (ImVec4){0.027f, 0.027f, 0.027f, 1.00f};
    colors[ImGuiCol_TabDimmedSelected]      = (ImVec4){0.067f, 0.067f, 0.067f, 1.00f};
    colors[ImGuiCol_TabDimmedSelectedOverline] = (ImVec4){0.322f, 0.322f, 0.322f, 1.00f};
    colors[ImGuiCol_DockingPreview]         = (ImVec4){0.831f, 0.831f, 0.831f, 0.35f};
    colors[ImGuiCol_DockingEmptyBg]         = (ImVec4){0.039f, 0.039f, 0.039f, 1.00f};
    colors[ImGuiCol_PlotLines]              = (ImVec4){0.831f, 0.831f, 0.831f, 1.00f};
    colors[ImGuiCol_PlotLinesHovered]       = (ImVec4){0.980f, 0.980f, 0.980f, 1.00f};
    colors[ImGuiCol_PlotHistogram]          = (ImVec4){0.831f, 0.831f, 0.831f, 1.00f};
    colors[ImGuiCol_PlotHistogramHovered]   = (ImVec4){0.980f, 0.980f, 0.980f, 1.00f};
    colors[ImGuiCol_TableHeaderBg]          = (ImVec4){0.067f, 0.067f, 0.067f, 1.00f};
    colors[ImGuiCol_TableBorderStrong]      = (ImVec4){0.149f, 0.149f, 0.149f, 1.00f};
    colors[ImGuiCol_TableBorderLight]       = (ImVec4){0.231f, 0.231f, 0.231f, 1.00f};
    colors[ImGuiCol_TableRowBg]             = (ImVec4){0.00f, 0.00f, 0.00f, 0.00f};
    colors[ImGuiCol_TableRowBgAlt]          = (ImVec4){0.980f, 0.980f, 0.980f, 0.02f};
    colors[ImGuiCol_TextSelectedBg]         = (ImVec4){0.831f, 0.831f, 0.831f, 0.25f};
    colors[ImGuiCol_DragDropTarget]         = (ImVec4){0.980f, 0.980f, 0.980f, 0.90f};
    colors[ImGuiCol_NavCursor]              = (ImVec4){0.980f, 0.980f, 0.980f, 1.00f};
    colors[ImGuiCol_NavWindowingHighlight]  = (ImVec4){0.980f, 0.980f, 0.980f, 0.70f};
    colors[ImGuiCol_NavWindowingDimBg]      = (ImVec4){0.020f, 0.020f, 0.020f, 0.55f};
    colors[ImGuiCol_ModalWindowDimBg]       = (ImVec4){0.020f, 0.020f, 0.020f, 0.70f};
}

//------------------------------------------------------------------------------
// Visual Studio Dark Theme
//------------------------------------------------------------------------------

static inline void ui_theme_apply_visual_studio(void) {
    ImGuiStyle* style = igGetStyle();

    g_ui_theme_current = UI_THEME_VISUAL_STUDIO;

    // Style settings (rectangular, no rounding)
    ui_theme_apply_zero_rounding(style);

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
    colors[ImGuiCol_TitleBgActive]          = (ImVec4){0.00f, 0.467f, 0.784f, 0.05f};  // #0077C8
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
    if ((theme < 0) || (theme >= UI_THEME_COUNT)) {
        theme = UI_THEME_DEFAULT;
    }

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
        case UI_THEME_ONE_DARK:
            ui_theme_apply_one_dark();
            break;
        case UI_THEME_ROSE_MOON:
            ui_theme_apply_rose_moon();
            break;
        case UI_THEME_NORD:
            ui_theme_apply_nord();
            break;
        case UI_THEME_TOKYO_STORM:
            ui_theme_apply_tokyo_storm();
            break;
        case UI_THEME_TOKYO_NIGHT:
            ui_theme_apply_tokyo_night();
            break;
        case UI_THEME_GRUVBOX_MATERIAL_DARK:
            ui_theme_apply_gruvbox_material_dark();
            break;
        case UI_THEME_CYBERPUNK_2077:
            ui_theme_apply_cyberpunk_2077();
            break;
        case UI_THEME_SHADCN_DARK:
            ui_theme_apply_shadcn_dark();
            break;
        default:
            ui_theme_apply_shadcn_dark();
            break;
    }
}

static inline void ui_theme_settings_reset_pending(void) {
    g_ui_theme_settings_state.has_pending_theme = false;
    g_ui_theme_settings_state.pending_theme = UI_THEME_DEFAULT;
}

static inline void ui_theme_settings_clear_all(ImGuiContext* ctx, ImGuiSettingsHandler* handler) {
    (void)ctx;
    (void)handler;
    ui_theme_settings_reset_pending();
}

static inline void ui_theme_settings_read_init(ImGuiContext* ctx, ImGuiSettingsHandler* handler) {
    (void)ctx;
    (void)handler;
    ui_theme_settings_reset_pending();
}

static inline void* ui_theme_settings_read_open(ImGuiContext* ctx, ImGuiSettingsHandler* handler, const char* name) {
    (void)ctx;
    (void)handler;
    if (!name || strcmp(name, "Theme") != 0) {
        return NULL;
    }

    ui_theme_settings_reset_pending();
    return &g_ui_theme_settings_state;
}

static inline void ui_theme_settings_read_line(ImGuiContext* ctx, ImGuiSettingsHandler* handler, void* entry, const char* line) {
    (void)ctx;
    (void)handler;
    if (!entry || !line || strncmp(line, "Selected=", 9) != 0) {
        return;
    }

    const char* value = line + 9;
    while (*value == ' ' || *value == '\t') {
        ++value;
    }

    char token[64];
    size_t len = strlen(value);
    while (len > 0u && (value[len - 1u] == ' ' || value[len - 1u] == '\t' || value[len - 1u] == '\r')) {
        --len;
    }
    if (len == 0u || len >= sizeof(token)) {
        return;
    }

    memcpy(token, value, len);
    token[len] = '\0';

    ui_theme_t parsed_theme = UI_THEME_DEFAULT;
    if (ui_theme_try_parse_token(token, &parsed_theme)) {
        ui_theme_settings_state_t* settings = (ui_theme_settings_state_t*)entry;
        settings->has_pending_theme = true;
        settings->pending_theme = parsed_theme;
    }
}

static inline void ui_theme_settings_apply_all(ImGuiContext* ctx, ImGuiSettingsHandler* handler) {
    (void)ctx;
    (void)handler;
    if (g_ui_theme_settings_state.has_pending_theme) {
        ui_theme_apply(g_ui_theme_settings_state.pending_theme);
        ui_theme_settings_reset_pending();
    }
}

static inline void ui_theme_settings_write_all(ImGuiContext* ctx, ImGuiSettingsHandler* handler, ImGuiTextBuffer* out_buf) {
    (void)ctx;
    (void)handler;
    ImGuiTextBuffer_appendf(
        out_buf,
        "[mdCAD][Theme]\nSelected=%s\n\n",
        ui_theme_get_token(ui_theme_get_current()));
}

static inline void ui_theme_register_settings_handler(void) {
    if (!igGetCurrentContext() || igFindSettingsHandler("mdCAD") != NULL) {
        return;
    }

    ImGuiSettingsHandler handler = {0};
    handler.TypeName = "mdCAD";
    handler.TypeHash = igImHashStr(handler.TypeName, 0, 0);
    handler.ClearAllFn = ui_theme_settings_clear_all;
    handler.ReadInitFn = ui_theme_settings_read_init;
    handler.ReadOpenFn = ui_theme_settings_read_open;
    handler.ReadLineFn = ui_theme_settings_read_line;
    handler.ApplyAllFn = ui_theme_settings_apply_all;
    handler.WriteAllFn = ui_theme_settings_write_all;
    handler.UserData = &g_ui_theme_settings_state;
    igAddSettingsHandler(&handler);
}

// Get the current theme's FrameBg color (useful for viewport clear color)
static inline void ui_theme_get_frame_bg(float* r, float* g, float* b) {
    ImGuiStyle* style = igGetStyle();
    ImVec4 col = style->Colors[ImGuiCol_FrameBg];
    *r = col.x;
    *g = col.y;
    *b = col.z;
}

// Get the current theme's hover color (for ECS entity highlighting)
static inline void ui_theme_get_hover_color(float* r, float* g, float* b, float* a) {
    ImGuiStyle* style = igGetStyle();
    ImVec4 col = style->Colors[ImGuiCol_HeaderHovered];
    *r = col.x;
    *g = col.y;
    *b = col.z;
    *a = col.w > 0.0f ? 1.0f : col.w;  // Use full alpha for 3D entities
}

// Get the current theme's selection color (for selected ECS entities)
static inline void ui_theme_get_selection_color(float* r, float* g, float* b, float* a) {
    ImGuiStyle* style = igGetStyle();
    ImVec4 col = style->Colors[ImGuiCol_HeaderActive];
    *r = col.x;
    *g = col.y;
    *b = col.z;
    *a = col.w > 0.0f ? 1.0f : col.w;  // Use full alpha for 3D entities
}

#endif // UI_THEME_H
