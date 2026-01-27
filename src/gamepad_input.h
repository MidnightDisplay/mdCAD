//------------------------------------------------------------------------------
// gamepad_input.h - Gamepad input handling (header-only)
//------------------------------------------------------------------------------
#ifndef GAMEPAD_INPUT_H
#define GAMEPAD_INPUT_H

#include "platform.h"
#include "sokol_app.h"
#include <math.h>
#include <stdbool.h>

//------------------------------------------------------------------------------
// Constants
//------------------------------------------------------------------------------
#define GAMEPAD_DEADZONE 0.15f           // 15% stick deadzone
#define GAMEPAD_TRIGGER_THRESHOLD 0.1f   // Trigger activation threshold

// Sensitivity multipliers
#define GAMEPAD_ORBIT_SPEED   2.0f       // Orbit rotation speed
#define GAMEPAD_PAN_SPEED     2.0f       // Pan movement speed
#define GAMEPAD_ZOOM_SPEED    3.0f       // Zoom speed

// Button hold thresholds
#define GAMEPAD_RESET_HOLD_TIME 0.5f     // Hold time for reset action

//------------------------------------------------------------------------------
// Types
//------------------------------------------------------------------------------

// Button indices (Android AKEYCODE mappings via Sokol)
typedef enum {
    GAMEPAD_BUTTON_A = 0,           // AKEYCODE_BUTTON_A (96)
    GAMEPAD_BUTTON_B,               // AKEYCODE_BUTTON_B (97)
    GAMEPAD_BUTTON_X,               // AKEYCODE_BUTTON_X (99)
    GAMEPAD_BUTTON_Y,               // AKEYCODE_BUTTON_Y (100)
    GAMEPAD_BUTTON_L1,              // AKEYCODE_BUTTON_L1 (102)
    GAMEPAD_BUTTON_R1,              // AKEYCODE_BUTTON_R1 (103)
    GAMEPAD_BUTTON_SELECT,          // AKEYCODE_BUTTON_SELECT (109)
    GAMEPAD_BUTTON_START,           // AKEYCODE_BUTTON_START (108)
    GAMEPAD_BUTTON_L3,              // AKEYCODE_BUTTON_THUMBL (106)
    GAMEPAD_BUTTON_R3,              // AKEYCODE_BUTTON_THUMBR (107)
    GAMEPAD_BUTTON_DPAD_UP,         // AKEYCODE_DPAD_UP (19)
    GAMEPAD_BUTTON_DPAD_DOWN,       // AKEYCODE_DPAD_DOWN (20)
    GAMEPAD_BUTTON_DPAD_LEFT,       // AKEYCODE_DPAD_LEFT (21)
    GAMEPAD_BUTTON_DPAD_RIGHT,      // AKEYCODE_DPAD_RIGHT (22)
    GAMEPAD_BUTTON_COUNT
} gamepad_button_t;

typedef struct {
    // Analog sticks (after deadzone, -1.0 to 1.0)
    float left_stick_x;
    float left_stick_y;
    float right_stick_x;
    float right_stick_y;

    // Triggers (0.0 to 1.0)
    float left_trigger;
    float right_trigger;

    // Button states (current frame)
    bool buttons[GAMEPAD_BUTTON_COUNT];

    // Button states (previous frame, for edge detection)
    bool prev_buttons[GAMEPAD_BUTTON_COUNT];

    // Hold timers for buttons (for press-and-hold actions)
    float button_hold_time[GAMEPAD_BUTTON_COUNT];

    // Is a gamepad connected/active?
    bool connected;

    // Raw axis values (before deadzone processing)
    float raw_left_x;
    float raw_left_y;
    float raw_right_x;
    float raw_right_y;
} gamepad_state_t;

//------------------------------------------------------------------------------
// Internal: Apply deadzone and response curve
//------------------------------------------------------------------------------
static inline float _gamepad_apply_deadzone(float value, float deadzone) {
    float sign = (value >= 0.0f) ? 1.0f : -1.0f;
    float abs_value = fabsf(value);

    if (abs_value < deadzone) {
        return 0.0f;
    }

    // Remap from [deadzone, 1.0] to [0.0, 1.0]
    float remapped = (abs_value - deadzone) / (1.0f - deadzone);

    // Apply quadratic response curve for smoother control
    return sign * remapped * remapped;
}

//------------------------------------------------------------------------------
// Initialize gamepad state
//------------------------------------------------------------------------------
static inline void gamepad_init(gamepad_state_t* gp) {
    gp->left_stick_x = 0.0f;
    gp->left_stick_y = 0.0f;
    gp->right_stick_x = 0.0f;
    gp->right_stick_y = 0.0f;
    gp->left_trigger = 0.0f;
    gp->right_trigger = 0.0f;
    gp->connected = false;
    gp->raw_left_x = 0.0f;
    gp->raw_left_y = 0.0f;
    gp->raw_right_x = 0.0f;
    gp->raw_right_y = 0.0f;

    for (int i = 0; i < GAMEPAD_BUTTON_COUNT; i++) {
        gp->buttons[i] = false;
        gp->prev_buttons[i] = false;
        gp->button_hold_time[i] = 0.0f;
    }
}

//------------------------------------------------------------------------------
// Begin frame: save previous button states
//------------------------------------------------------------------------------
static inline void gamepad_begin_frame(gamepad_state_t* gp) {
    for (int i = 0; i < GAMEPAD_BUTTON_COUNT; i++) {
        gp->prev_buttons[i] = gp->buttons[i];
    }
}

//------------------------------------------------------------------------------
// Update hold timers and process deadzone
//------------------------------------------------------------------------------
static inline void gamepad_update(gamepad_state_t* gp, float dt) {
    // Update button hold timers
    for (int i = 0; i < GAMEPAD_BUTTON_COUNT; i++) {
        if (gp->buttons[i]) {
            gp->button_hold_time[i] += dt;
        } else {
            gp->button_hold_time[i] = 0.0f;
        }
    }

    // Apply deadzone to raw axis values
    gp->left_stick_x = _gamepad_apply_deadzone(gp->raw_left_x, GAMEPAD_DEADZONE);
    gp->left_stick_y = _gamepad_apply_deadzone(gp->raw_left_y, GAMEPAD_DEADZONE);
    gp->right_stick_x = _gamepad_apply_deadzone(gp->raw_right_x, GAMEPAD_DEADZONE);
    gp->right_stick_y = _gamepad_apply_deadzone(gp->raw_right_y, GAMEPAD_DEADZONE);
}

//------------------------------------------------------------------------------
// Button state queries
//------------------------------------------------------------------------------
static inline bool gamepad_button_pressed(const gamepad_state_t* gp, gamepad_button_t btn) {
    return gp->buttons[btn] && !gp->prev_buttons[btn];
}

static inline bool gamepad_button_released(const gamepad_state_t* gp, gamepad_button_t btn) {
    return !gp->buttons[btn] && gp->prev_buttons[btn];
}

static inline bool gamepad_button_held(const gamepad_state_t* gp, gamepad_button_t btn) {
    return gp->buttons[btn];
}

static inline bool gamepad_button_held_for(const gamepad_state_t* gp, gamepad_button_t btn, float time) {
    return gp->button_hold_time[btn] >= time;
}

//------------------------------------------------------------------------------
// Handle Sokol events (key events from Android)
//
// On Android, gamepad buttons generate key events with specific keycodes.
// This function maps them to our gamepad button states.
//------------------------------------------------------------------------------
static inline bool gamepad_handle_event(gamepad_state_t* gp, const sapp_event* ev) {
    if (ev->type != SAPP_EVENTTYPE_KEY_DOWN && ev->type != SAPP_EVENTTYPE_KEY_UP) {
        return false;
    }

    bool pressed = (ev->type == SAPP_EVENTTYPE_KEY_DOWN);
    gamepad_button_t btn = GAMEPAD_BUTTON_COUNT; // Invalid

    // Map Sokol keycodes to gamepad buttons
    // Note: Sokol uses GLFW-style keycodes, but on Android these come from AKEYCODE values
    // We handle both the standard keyboard arrows and gamepad-specific codes
    switch (ev->key_code) {
        // D-pad (works on all platforms via arrow keys)
        case SAPP_KEYCODE_UP:    btn = GAMEPAD_BUTTON_DPAD_UP;    break;
        case SAPP_KEYCODE_DOWN:  btn = GAMEPAD_BUTTON_DPAD_DOWN;  break;
        case SAPP_KEYCODE_LEFT:  btn = GAMEPAD_BUTTON_DPAD_LEFT;  break;
        case SAPP_KEYCODE_RIGHT: btn = GAMEPAD_BUTTON_DPAD_RIGHT; break;

        // Enter/Space as A button (for keyboard/remote users)
        case SAPP_KEYCODE_ENTER:
        case SAPP_KEYCODE_SPACE: btn = GAMEPAD_BUTTON_A; break;

        // Escape as B button (back)
        case SAPP_KEYCODE_ESCAPE: btn = GAMEPAD_BUTTON_B; break;

        // Tab cycles through (maps to Y for theme cycling)
        case SAPP_KEYCODE_TAB: btn = GAMEPAD_BUTTON_Y; break;

        // Shift keys as bumpers
        case SAPP_KEYCODE_LEFT_SHIFT:  btn = GAMEPAD_BUTTON_L1; break;
        case SAPP_KEYCODE_RIGHT_SHIFT: btn = GAMEPAD_BUTTON_R1; break;

        default: break;
    }

    // Note: Full Android gamepad support (AKEYCODE_BUTTON_A, etc.) requires
    // applying patches/sokol_android_gamepad.patch to sokol_app.h.
    // Without the patch, only keyboard/remote inputs work via the mappings above.

    if (btn < GAMEPAD_BUTTON_COUNT) {
        gp->buttons[btn] = pressed;
        gp->connected = true;
        return true;
    }

    return false;
}

//------------------------------------------------------------------------------
// Set axis values (called from platform-specific code)
// On Android, these come from AMotionEvent axis values
//------------------------------------------------------------------------------
static inline void gamepad_set_axis(gamepad_state_t* gp,
                                     float left_x, float left_y,
                                     float right_x, float right_y,
                                     float left_trigger, float right_trigger) {
    gp->raw_left_x = left_x;
    gp->raw_left_y = left_y;
    gp->raw_right_x = right_x;
    gp->raw_right_y = right_y;
    gp->left_trigger = left_trigger;
    gp->right_trigger = right_trigger;
    gp->connected = true;
}

//------------------------------------------------------------------------------
// Camera control helpers
//------------------------------------------------------------------------------

// Get orbit input (from left stick or D-pad)
static inline void gamepad_get_orbit_input(const gamepad_state_t* gp, float* out_x, float* out_y) {
    *out_x = gp->left_stick_x * GAMEPAD_ORBIT_SPEED;
    *out_y = gp->left_stick_y * GAMEPAD_ORBIT_SPEED;

    // D-pad can also orbit
    if (gp->buttons[GAMEPAD_BUTTON_DPAD_LEFT])  *out_x -= 1.0f;
    if (gp->buttons[GAMEPAD_BUTTON_DPAD_RIGHT]) *out_x += 1.0f;
    if (gp->buttons[GAMEPAD_BUTTON_DPAD_UP])    *out_y -= 1.0f;
    if (gp->buttons[GAMEPAD_BUTTON_DPAD_DOWN])  *out_y += 1.0f;
}

// Get pan input (from right stick)
static inline void gamepad_get_pan_input(const gamepad_state_t* gp, float* out_x, float* out_y) {
    *out_x = gp->right_stick_x * GAMEPAD_PAN_SPEED;
    *out_y = gp->right_stick_y * GAMEPAD_PAN_SPEED;
}

// Get zoom input (from triggers: R2 = zoom in, L2 = zoom out)
static inline float gamepad_get_zoom_input(const gamepad_state_t* gp) {
    float zoom = 0.0f;

    // Triggers
    if (gp->right_trigger > GAMEPAD_TRIGGER_THRESHOLD) {
        zoom += gp->right_trigger * GAMEPAD_ZOOM_SPEED;  // Zoom in
    }
    if (gp->left_trigger > GAMEPAD_TRIGGER_THRESHOLD) {
        zoom -= gp->left_trigger * GAMEPAD_ZOOM_SPEED;   // Zoom out
    }

    return zoom;
}

// Check if reset should be triggered (L1 + R1 held)
static inline bool gamepad_should_reset(const gamepad_state_t* gp) {
    return gamepad_button_held(gp, GAMEPAD_BUTTON_L1) &&
           gamepad_button_held(gp, GAMEPAD_BUTTON_R1) &&
           (gp->button_hold_time[GAMEPAD_BUTTON_L1] >= GAMEPAD_RESET_HOLD_TIME ||
            gp->button_hold_time[GAMEPAD_BUTTON_R1] >= GAMEPAD_RESET_HOLD_TIME);
}

#endif // GAMEPAD_INPUT_H
