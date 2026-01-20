//------------------------------------------------------------------------------
// ui_camera_debug.h - Camera debug panel UI (header-only)
//------------------------------------------------------------------------------
#ifndef UI_CAMERA_DEBUG_H
#define UI_CAMERA_DEBUG_H

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"
#include "../orbit_camera.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

//------------------------------------------------------------------------------
// Types
//------------------------------------------------------------------------------
typedef struct {
    orbit_camera_t* camera;
} ui_camera_debug_state_t;

//------------------------------------------------------------------------------
// Functions
//------------------------------------------------------------------------------

// Initialize camera debug state
static inline void ui_camera_debug_init(ui_camera_debug_state_t* dbg, orbit_camera_t* camera) {
    dbg->camera = camera;
}

// Draw the Camera Debug window
static inline void ui_camera_debug_draw(ui_camera_debug_state_t* dbg) {
    igBegin("Camera Debug", NULL, ImGuiWindowFlags_None);

    orbit_camera_t* cam = dbg->camera;

    // Current action display
    const char* action_str = "None";
    if (cam->current_action == ORBIT_CAM_ACTION_ROTATE) action_str = "Rotate";
    else if (cam->current_action == ORBIT_CAM_ACTION_PAN) action_str = "Pan";
    igText("Current Action: %s", action_str);

    igSeparator();

    // Eye position (read-only)
    vec3_t eye = orbit_camera_get_eye_position(cam);
    igText("Eye Position:");
    igText("  X: %.3f  Y: %.3f  Z: %.3f", eye.x, eye.y, eye.z);

    igSeparator();

    // Spherical coordinates (editable)
    igText("Spherical Coordinates:");

    igSliderFloat("Distance", &cam->distance,
                  ORBIT_CAM_MIN_DISTANCE, ORBIT_CAM_MAX_DISTANCE, "%.2f", ImGuiSliderFlags_None);

    // Azimuth in degrees for easier editing
    float azimuth_deg = cam->azimuth * (180.0f / (float)M_PI);
    if (igSliderFloat("Azimuth", &azimuth_deg, -180.0f, 180.0f, "%.1f deg", ImGuiSliderFlags_None)) {
        cam->azimuth = azimuth_deg * ((float)M_PI / 180.0f);
    }

    // Elevation in degrees
    float elevation_deg = cam->elevation * (180.0f / (float)M_PI);
    float min_elev_deg = ORBIT_CAM_MIN_ELEVATION * (180.0f / (float)M_PI);
    float max_elev_deg = ORBIT_CAM_MAX_ELEVATION * (180.0f / (float)M_PI);
    if (igSliderFloat("Elevation", &elevation_deg, min_elev_deg, max_elev_deg, "%.1f deg", ImGuiSliderFlags_None)) {
        cam->elevation = elevation_deg * ((float)M_PI / 180.0f);
    }

    igSeparator();

    // Target position (editable)
    igText("Target Position:");
    igDragFloat3("Target", &cam->target.x, 0.1f, -100.0f, 100.0f, "%.2f", ImGuiSliderFlags_None);

    igSeparator();

    // Velocity display
    igText("Inertia Velocity:");
    igText("  Azimuth: %.5f  Elevation: %.5f", cam->velocity_azimuth, cam->velocity_elevation);

    igSeparator();

    // Action buttons
    if (igButton("Reset Camera", (ImVec2){0, 0})) {
        orbit_camera_reset(cam);
    }

    igSameLine(0, -1);

    if (igButton("Reset Target", (ImVec2){0, 0})) {
        cam->target = (vec3_t){0.0f, 0.0f, 0.0f};
    }

    igSameLine(0, -1);

    if (igButton("Stop Inertia", (ImVec2){0, 0})) {
        cam->velocity_azimuth = 0.0f;
        cam->velocity_elevation = 0.0f;
    }

    igEnd();
}

#endif // UI_CAMERA_DEBUG_H
