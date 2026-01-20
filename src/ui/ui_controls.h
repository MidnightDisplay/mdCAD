//------------------------------------------------------------------------------
// ui_controls.h - Controls window UI (header-only)
//------------------------------------------------------------------------------
#ifndef UI_CONTROLS_H
#define UI_CONTROLS_H

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"
#include "sokol_gfx.h"
#include "../orbit_camera.h"

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

// Initialize controls state
static inline void ui_controls_init(ui_controls_state_t* ctrl, orbit_camera_t* camera, sg_pass_action* pass_action) {
    ctrl->clear_color[0] = 0.20f;
    ctrl->clear_color[1] = 0.20f;
    ctrl->clear_color[2] = 0.216f;
    ctrl->camera = camera;
    ctrl->offscreen_pass_action = pass_action;

    // Initialize pass action with default clear color
    pass_action->colors[0].load_action = SG_LOADACTION_CLEAR;
    pass_action->colors[0].clear_value.r = ctrl->clear_color[0];
    pass_action->colors[0].clear_value.g = ctrl->clear_color[1];
    pass_action->colors[0].clear_value.b = ctrl->clear_color[2];
    pass_action->colors[0].clear_value.a = 1.0f;
    pass_action->depth.load_action = SG_LOADACTION_CLEAR;
    pass_action->depth.clear_value = 1.0f;
}

// Draw the Controls window
static inline void ui_controls_draw(ui_controls_state_t* ctrl) {
    igBegin("Controls", NULL, ImGuiWindowFlags_None);

    igText("3D Viewport Settings");
    igSeparator();

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
