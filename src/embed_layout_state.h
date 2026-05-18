#ifndef EMBED_LAYOUT_STATE_H
#define EMBED_LAYOUT_STATE_H

#include <stdbool.h>

typedef struct {
    bool embedded_mode;
    bool viewport_only_mode;
    bool embedded_store_exists;
} embed_layout_state_t;

typedef struct {
    bool embedded_mode;
    bool viewport_only_mode;
    bool use_manual_persistence;
    bool should_seed_default_layout;
    const char* automatic_ini_filename;
    const char* manual_store_filename;
} embed_layout_policy_t;

static inline embed_layout_state_t embed_layout_state_make(bool embedded_mode,
                                                           bool viewport_only_mode,
                                                           bool embedded_store_exists) {
    embed_layout_state_t state = {0};
    state.embedded_mode = embedded_mode;
    state.viewport_only_mode = embedded_mode && viewport_only_mode;
    state.embedded_store_exists = embedded_store_exists;
    return state;
}

static inline const char* embed_layout_state_standalone_ini_filename(void) {
    return "imgui.ini";
}

static inline const char* embed_layout_state_embedded_store_filename(void) {
    return "imgui.embedded.ini";
}

static inline const char* embed_layout_state_viewport_only_store_filename(void) {
    return "imgui.embedded.viewport-only.ini";
}

static inline embed_layout_policy_t embed_layout_state_resolve(const embed_layout_state_t* state) {
    embed_layout_policy_t policy = {0};
    if (!state) {
        return policy;
    }

    policy.embedded_mode = state->embedded_mode;
    if (state->embedded_mode) {
        policy.viewport_only_mode = state->viewport_only_mode;
        policy.use_manual_persistence = true;
        policy.should_seed_default_layout = !state->embedded_store_exists;
        policy.manual_store_filename = state->viewport_only_mode
            ? embed_layout_state_viewport_only_store_filename()
            : embed_layout_state_embedded_store_filename();
        return policy;
    }

    policy.automatic_ini_filename = embed_layout_state_standalone_ini_filename();
    return policy;
}

#endif // EMBED_LAYOUT_STATE_H
