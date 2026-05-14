#ifndef EMBED_INPUT_STATE_H
#define EMBED_INPUT_STATE_H

#include <stdbool.h>

typedef enum {
    EMBED_INPUT_TRIGGER_ATTACH = 0,
    EMBED_INPUT_TRIGGER_VIEWER_FOCUSED,
    EMBED_INPUT_TRIGGER_VIEWER_MOUSE_DOWN,
    EMBED_INPUT_TRIGGER_POINTER_LEFT_VIEWER,
    EMBED_INPUT_TRIGGER_HOST_FOCUS_GAINED,
    EMBED_INPUT_TRIGGER_HOST_DEACTIVATED,
    EMBED_INPUT_TRIGGER_INTERACTION_BEGAN,
    EMBED_INPUT_TRIGGER_INTERACTION_ENDED
} embed_input_trigger_t;

typedef struct {
    bool embedded_mode;
    bool viewer_focused;
    bool keyboard_owned;
    bool interaction_active;
} embed_input_state_t;

typedef struct {
    bool keyboard_owned;
    bool allow_shortcuts;
    bool interaction_active;
    bool cancel_interaction;
} embed_input_state_result_t;

static inline embed_input_state_t embed_input_state_make(bool embedded_mode) {
    embed_input_state_t state = {0};
    state.embedded_mode = embedded_mode;
    return state;
}

static inline bool embed_input_state_allows_shortcuts(const embed_input_state_t* state) {
    return state && (!state->embedded_mode || state->keyboard_owned);
}

static inline embed_input_state_result_t embed_input_state_snapshot(const embed_input_state_t* state) {
    embed_input_state_result_t result = {0};
    if (!state) {
        return result;
    }

    result.keyboard_owned = state->keyboard_owned;
    result.allow_shortcuts = embed_input_state_allows_shortcuts(state);
    result.interaction_active = state->interaction_active;
    return result;
}

static inline embed_input_state_result_t embed_input_state_apply(embed_input_state_t* state,
                                                                 embed_input_trigger_t trigger) {
    embed_input_state_result_t result = {0};
    if (!state) {
        return result;
    }

    switch (trigger) {
        case EMBED_INPUT_TRIGGER_ATTACH:
            state->viewer_focused = false;
            state->keyboard_owned = false;
            state->interaction_active = false;
            break;
        case EMBED_INPUT_TRIGGER_VIEWER_FOCUSED:
            state->viewer_focused = true;
            break;
        case EMBED_INPUT_TRIGGER_VIEWER_MOUSE_DOWN:
            state->viewer_focused = true;
            if (state->embedded_mode) {
                state->keyboard_owned = true;
            }
            break;
        case EMBED_INPUT_TRIGGER_POINTER_LEFT_VIEWER:
            break;
        case EMBED_INPUT_TRIGGER_HOST_FOCUS_GAINED:
        case EMBED_INPUT_TRIGGER_HOST_DEACTIVATED:
            state->viewer_focused = false;
            state->keyboard_owned = false;
            if (state->interaction_active) {
                state->interaction_active = false;
                result.cancel_interaction = true;
            }
            break;
        case EMBED_INPUT_TRIGGER_INTERACTION_BEGAN:
            if (!state->embedded_mode || state->keyboard_owned) {
                state->interaction_active = true;
            }
            break;
        case EMBED_INPUT_TRIGGER_INTERACTION_ENDED:
            state->interaction_active = false;
            break;
        default:
            break;
    }

    result.keyboard_owned = state->keyboard_owned;
    result.allow_shortcuts = embed_input_state_allows_shortcuts(state);
    result.interaction_active = state->interaction_active;
    return result;
}

#endif // EMBED_INPUT_STATE_H
