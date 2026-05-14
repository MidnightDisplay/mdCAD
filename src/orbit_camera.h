//------------------------------------------------------------------------------
// orbit_camera.h - Orbital camera system (header-only)
//------------------------------------------------------------------------------
#ifndef ORBIT_CAMERA_H
#define ORBIT_CAMERA_H

#include "math3d.h"
#include "math/cglm_entry.h"
#include <math.h>
#include <string.h>

//------------------------------------------------------------------------------
// Constants
//------------------------------------------------------------------------------
#define ORBIT_CAM_MIN_DISTANCE   0.5f
#define ORBIT_CAM_MAX_DISTANCE   50.0f
#define ORBIT_CAM_MIN_ELEVATION  -1.4f   // ~-80 degrees
#define ORBIT_CAM_MAX_ELEVATION  1.4f    // ~+80 degrees
#define ORBIT_CAM_INERTIA_DAMPING 0.92f
#define ORBIT_CAM_ROTATE_SPEED   0.005f  // radians per pixel
#define ORBIT_CAM_PAN_SPEED      0.005f  // units per pixel
#define ORBIT_CAM_ZOOM_SPEED     0.1f    // distance per wheel tick

// Default values
#define ORBIT_CAM_DEFAULT_DISTANCE  3.0f
#define ORBIT_CAM_DEFAULT_AZIMUTH   0.785398f  // 45 degrees
#define ORBIT_CAM_DEFAULT_ELEVATION 0.5f

//------------------------------------------------------------------------------
// Types
//------------------------------------------------------------------------------
typedef enum {
    ORBIT_CAM_ACTION_NONE = 0,
    ORBIT_CAM_ACTION_ROTATE,
    ORBIT_CAM_ACTION_PAN
} orbit_camera_action_t;

typedef struct {
    // Spherical coordinates
    float distance;           // Distance from target (zoom)
    float azimuth;            // Horizontal angle (yaw) in radians
    float elevation;          // Vertical angle (pitch) in radians

    // Target point (what the camera looks at)
    vec3_t target;

    // Inertia state
    float velocity_azimuth;
    float velocity_elevation;

    // Interaction state
    orbit_camera_action_t current_action;
    bool drag_started_in_viewport;
    bool was_dragging;
} orbit_camera_t;

//------------------------------------------------------------------------------
// Functions
//------------------------------------------------------------------------------

// Initialize camera to default state
static inline void orbit_camera_init(orbit_camera_t* cam) {
    cam->distance = ORBIT_CAM_DEFAULT_DISTANCE;
    cam->azimuth = ORBIT_CAM_DEFAULT_AZIMUTH;
    cam->elevation = ORBIT_CAM_DEFAULT_ELEVATION;
    cam->target = (vec3_t){0.0f, 0.0f, 0.0f};
    cam->velocity_azimuth = 0.0f;
    cam->velocity_elevation = 0.0f;
    cam->current_action = ORBIT_CAM_ACTION_NONE;
    cam->drag_started_in_viewport = false;
    cam->was_dragging = false;
}

// Reset camera to default view
static inline void orbit_camera_reset(orbit_camera_t* cam) {
    cam->distance = ORBIT_CAM_DEFAULT_DISTANCE;
    cam->azimuth = ORBIT_CAM_DEFAULT_AZIMUTH;
    cam->elevation = ORBIT_CAM_DEFAULT_ELEVATION;
    cam->target = (vec3_t){0.0f, 0.0f, 0.0f};
    cam->velocity_azimuth = 0.0f;
    cam->velocity_elevation = 0.0f;
    cam->current_action = ORBIT_CAM_ACTION_NONE;
    cam->drag_started_in_viewport = false;
}

static inline void orbit_camera_cancel_interaction(orbit_camera_t* cam) {
    if (!cam) return;
    cam->current_action = ORBIT_CAM_ACTION_NONE;
    cam->drag_started_in_viewport = false;
    cam->was_dragging = false;
    cam->velocity_azimuth = 0.0f;
    cam->velocity_elevation = 0.0f;
}

// Calculate eye position from spherical coordinates
static inline vec3s orbit_camera_get_eye_position_cglm(const orbit_camera_t* cam) {
    return (vec3s){ {
        cam->target.x + cam->distance * cosf(cam->elevation) * sinf(cam->azimuth),
        cam->target.y + cam->distance * sinf(cam->elevation),
        cam->target.z + cam->distance * cosf(cam->elevation) * cosf(cam->azimuth)
    } };
}

static inline vec3_t orbit_camera_get_eye_position(const orbit_camera_t* cam) {
    vec3s eye = orbit_camera_get_eye_position_cglm(cam);
    return (vec3_t){ eye.raw[0], eye.raw[1], eye.raw[2] };
}

// Get the view matrix for rendering
static inline mat4s orbit_camera_get_view_matrix_cglm(const orbit_camera_t* cam) {
    return glms_lookat_rh_zo(orbit_camera_get_eye_position_cglm(cam),
                             (vec3s){ { cam->target.x, cam->target.y, cam->target.z } },
                             GLMS_YUP);
}

static inline mat4_t orbit_camera_get_view_matrix(const orbit_camera_t* cam) {
    mat4_t view;
    mat4s view_cglm = orbit_camera_get_view_matrix_cglm(cam);
    memcpy(view.m, view_cglm.raw, sizeof(view.m));
    return view;
}

// Call at the beginning of frame to reset drag state when buttons released
static inline void orbit_camera_begin_frame(orbit_camera_t* cam, bool any_mouse_down) {
    if (!any_mouse_down) {
        // Transfer velocity from active drag to inertia when releasing
        if (cam->was_dragging && cam->current_action == ORBIT_CAM_ACTION_ROTATE) {
            // Keep velocities for inertia
        }
        cam->current_action = ORBIT_CAM_ACTION_NONE;
        cam->drag_started_in_viewport = false;
        cam->was_dragging = false;
    }
}

// Handle mouse input for camera control
// hovered: is mouse over viewport image
// dx, dy: mouse delta this frame
// wheel: mouse wheel delta
// left, middle, right: mouse button states
// shift: shift key state
static inline void orbit_camera_handle_input(orbit_camera_t* cam,
                                             bool hovered,
                                             float dx, float dy, float wheel,
                                             bool left, bool middle, bool shift) {
    // Handle zoom (works when hovered)
    if (hovered && wheel != 0.0f) {
        cam->distance -= wheel * ORBIT_CAM_ZOOM_SPEED;
        if (cam->distance < ORBIT_CAM_MIN_DISTANCE) cam->distance = ORBIT_CAM_MIN_DISTANCE;
        if (cam->distance > ORBIT_CAM_MAX_DISTANCE) cam->distance = ORBIT_CAM_MAX_DISTANCE;
    }

    // Start drag only if mouse is over viewport
    bool wants_rotate = left && !shift;
    bool wants_pan = middle || (left && shift);

    if ((wants_rotate || wants_pan) && !cam->drag_started_in_viewport) {
        if (hovered) {
            cam->drag_started_in_viewport = true;
            cam->current_action = wants_pan ? ORBIT_CAM_ACTION_PAN : ORBIT_CAM_ACTION_ROTATE;
            // Clear inertia when starting a new drag
            cam->velocity_azimuth = 0.0f;
            cam->velocity_elevation = 0.0f;
        }
    }

    // Process drag only if it started in viewport
    if (cam->drag_started_in_viewport) {
        cam->was_dragging = true;

        if (cam->current_action == ORBIT_CAM_ACTION_ROTATE) {
            cam->azimuth -= dx * ORBIT_CAM_ROTATE_SPEED;
            cam->elevation += dy * ORBIT_CAM_ROTATE_SPEED;

            // Store velocity for inertia
            cam->velocity_azimuth = -dx * ORBIT_CAM_ROTATE_SPEED;
            cam->velocity_elevation = dy * ORBIT_CAM_ROTATE_SPEED;

            // Clamp elevation
            if (cam->elevation > ORBIT_CAM_MAX_ELEVATION) cam->elevation = ORBIT_CAM_MAX_ELEVATION;
            if (cam->elevation < ORBIT_CAM_MIN_ELEVATION) cam->elevation = ORBIT_CAM_MIN_ELEVATION;
        }
        else if (cam->current_action == ORBIT_CAM_ACTION_PAN) {
            // Calculate camera right and up vectors for panning
            float ca = cosf(cam->azimuth);
            float sa = sinf(cam->azimuth);

            // Right vector (perpendicular to view direction in XZ plane)
            vec3_t right = { ca, 0.0f, -sa };

            // Up vector (world up for simplicity)
            vec3_t up = { 0.0f, 1.0f, 0.0f };

            // Pan the target
            cam->target.x -= right.x * dx * ORBIT_CAM_PAN_SPEED * cam->distance * 0.5f;
            cam->target.z -= right.z * dx * ORBIT_CAM_PAN_SPEED * cam->distance * 0.5f;
            cam->target.y += up.y * dy * ORBIT_CAM_PAN_SPEED * cam->distance * 0.5f;
        }
    }
}

// Update camera state (apply inertia)
static inline void orbit_camera_update(orbit_camera_t* cam, float dt) {
    // Apply inertia only when not actively dragging
    if (cam->current_action == ORBIT_CAM_ACTION_NONE) {
        // Apply velocity
        cam->azimuth += cam->velocity_azimuth;
        cam->elevation += cam->velocity_elevation;

        // Clamp elevation
        if (cam->elevation > ORBIT_CAM_MAX_ELEVATION) cam->elevation = ORBIT_CAM_MAX_ELEVATION;
        if (cam->elevation < ORBIT_CAM_MIN_ELEVATION) cam->elevation = ORBIT_CAM_MIN_ELEVATION;

        // Apply frame-rate independent damping
        float damping = powf(ORBIT_CAM_INERTIA_DAMPING, dt * 60.0f);
        cam->velocity_azimuth *= damping;
        cam->velocity_elevation *= damping;

        // Stop if velocity is very small
        if (fabsf(cam->velocity_azimuth) < 0.0001f) cam->velocity_azimuth = 0.0f;
        if (fabsf(cam->velocity_elevation) < 0.0001f) cam->velocity_elevation = 0.0f;
    }
}

#endif // ORBIT_CAMERA_H
