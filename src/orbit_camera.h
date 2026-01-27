//------------------------------------------------------------------------------
// orbit_camera.h - Orbital camera system (header-only)
//------------------------------------------------------------------------------
#ifndef ORBIT_CAMERA_H
#define ORBIT_CAMERA_H

#include "math3d.h"
#include <math.h>

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

// Calculate eye position from spherical coordinates
static inline vec3_t orbit_camera_get_eye_position(const orbit_camera_t* cam) {
    vec3_t eye;
    eye.x = cam->target.x + cam->distance * cosf(cam->elevation) * sinf(cam->azimuth);
    eye.y = cam->target.y + cam->distance * sinf(cam->elevation);
    eye.z = cam->target.z + cam->distance * cosf(cam->elevation) * cosf(cam->azimuth);
    return eye;
}

// Get the view matrix for rendering
static inline mat4_t orbit_camera_get_view_matrix(const orbit_camera_t* cam) {
    vec3_t eye = orbit_camera_get_eye_position(cam);
    vec3_t up = {0.0f, 1.0f, 0.0f};
    return mat4_lookat(eye, cam->target, up);
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

// Apply gamepad input to camera
// orbit_x, orbit_y: rotation input from left stick/D-pad (in speed units)
// pan_x, pan_y: pan input from right stick (in speed units)
// zoom: zoom input from triggers (positive = zoom in, negative = zoom out)
static inline void orbit_camera_apply_gamepad(orbit_camera_t* cam,
                                               float orbit_x, float orbit_y,
                                               float pan_x, float pan_y,
                                               float zoom,
                                               float dt) {
    // Apply orbit (same as mouse rotation but frame-rate independent)
    if (orbit_x != 0.0f || orbit_y != 0.0f) {
        cam->azimuth -= orbit_x * dt;
        cam->elevation += orbit_y * dt;

        // Clamp elevation
        if (cam->elevation > ORBIT_CAM_MAX_ELEVATION) cam->elevation = ORBIT_CAM_MAX_ELEVATION;
        if (cam->elevation < ORBIT_CAM_MIN_ELEVATION) cam->elevation = ORBIT_CAM_MIN_ELEVATION;

        // Set velocity for smooth stop (optional inertia)
        cam->velocity_azimuth = -orbit_x * dt * 0.5f;
        cam->velocity_elevation = orbit_y * dt * 0.5f;
    }

    // Apply pan
    if (pan_x != 0.0f || pan_y != 0.0f) {
        float ca = cosf(cam->azimuth);
        float sa = sinf(cam->azimuth);

        // Right vector
        vec3_t right = { ca, 0.0f, -sa };

        // Pan the target
        float pan_scale = cam->distance * 0.5f * dt;
        cam->target.x -= right.x * pan_x * pan_scale;
        cam->target.z -= right.z * pan_x * pan_scale;
        cam->target.y += pan_y * pan_scale;
    }

    // Apply zoom
    if (zoom != 0.0f) {
        cam->distance -= zoom * dt;
        if (cam->distance < ORBIT_CAM_MIN_DISTANCE) cam->distance = ORBIT_CAM_MIN_DISTANCE;
        if (cam->distance > ORBIT_CAM_MAX_DISTANCE) cam->distance = ORBIT_CAM_MAX_DISTANCE;
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
