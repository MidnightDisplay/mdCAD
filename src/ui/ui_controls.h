//------------------------------------------------------------------------------
// ui_controls.h - Controls window UI (header-only)
//------------------------------------------------------------------------------
#ifndef UI_CONTROLS_H
#define UI_CONTROLS_H

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"
#include "sokol_gfx.h"
#include "../platform.h"
#include "../orbit_camera.h"
#include "ui_theme.h"

//------------------------------------------------------------------------------
// Types
//------------------------------------------------------------------------------
typedef struct {
    float clear_color[3];
    orbit_camera_t* camera;               // Pointer to orbital camera
    sg_pass_action* offscreen_pass_action; // Pointer to pass action to update
} ui_controls_state_t;

//------------------------------------------------------------------------------
// Functions
//------------------------------------------------------------------------------

// Sync clear color with current theme's FrameBg
static inline void ui_controls_sync_clear_color(ui_controls_state_t* ctrl) {
    ui_theme_get_frame_bg(&ctrl->clear_color[0], &ctrl->clear_color[1], &ctrl->clear_color[2]);
    ctrl->offscreen_pass_action->colors[0].clear_value.r = ctrl->clear_color[0];
    ctrl->offscreen_pass_action->colors[0].clear_value.g = ctrl->clear_color[1];
    ctrl->offscreen_pass_action->colors[0].clear_value.b = ctrl->clear_color[2];
}

// Initialize controls state
static inline void ui_controls_init(ui_controls_state_t* ctrl, orbit_camera_t* camera, sg_pass_action* pass_action) {
    ctrl->camera = camera;
    ctrl->offscreen_pass_action = pass_action;

    // Initialize pass action
    pass_action->colors[0].load_action = SG_LOADACTION_CLEAR;
#if MDCAD_VIEWPORT_MSAA_SAMPLES > 1
    // Offscreen MSAA color attachment is resolved to a single-sample texture each pass.
    pass_action->colors[0].store_action = SG_STOREACTION_DONTCARE;
#else
    pass_action->colors[0].store_action = SG_STOREACTION_STORE;
#endif
    pass_action->colors[0].clear_value.a = 1.0f;
    pass_action->depth.load_action = SG_LOADACTION_CLEAR;
    pass_action->depth.clear_value = 1.0f;

    // Sync clear color with theme's FrameBg
    ui_controls_sync_clear_color(ctrl);
}

// Draw the Controls window
static inline void ui_controls_draw(ui_controls_state_t* ctrl) {
    igBegin("Controls", NULL, ImGuiWindowFlags_None);

    // Theme selector
    int current_theme = (int)ui_theme_get_current();

    igText("UI Theme");
    if (igCombo_Str_arr("##Theme", &current_theme, ui_theme_names, UI_THEME_COUNT, -1)) {
        ui_theme_apply((ui_theme_t)current_theme);
        // Sync clear color with new theme's FrameBg
        ui_controls_sync_clear_color(ctrl);
    }

    igSeparator();
    igText("3D Viewport Settings");

    if (igColorEdit3("Clear Color", ctrl->clear_color, ImGuiColorEditFlags_None)) {
        // Update pass action when color changes
        ctrl->offscreen_pass_action->colors[0].clear_value.r = ctrl->clear_color[0];
        ctrl->offscreen_pass_action->colors[0].clear_value.g = ctrl->clear_color[1];
        ctrl->offscreen_pass_action->colors[0].clear_value.b = ctrl->clear_color[2];
    }

    igSeparator();
    igText("Camera Controls:");
    igBulletText("Left drag: Orbit");
    igBulletText("Middle drag / Shift+Left: Pan");
    igBulletText("Scroll: Zoom");

    if (igButton("Reset Camera", (ImVec2){0, 0})) {
        orbit_camera_reset(ctrl->camera);
    }

    igEnd();
}

#endif // UI_CONTROLS_H
