#include "../embed_input_state.h"

#include <stdio.h>

static int test_embed_attach_and_focus_do_not_grant_keyboard(void) {
    embed_input_state_t state = embed_input_state_make(true);
    embed_input_state_result_t result = embed_input_state_apply(&state, EMBED_INPUT_TRIGGER_ATTACH);
    if (result.allow_shortcuts) return 1;
    if (result.keyboard_owned) return 1;
    if (state.viewer_focused) return 1;

    result = embed_input_state_apply(&state, EMBED_INPUT_TRIGGER_VIEWER_FOCUSED);
    if (!state.viewer_focused) return 1;
    if (result.allow_shortcuts) return 1;
    if (result.keyboard_owned) return 1;
    return 0;
}

static int test_embed_first_click_grants_keyboard_ownership(void) {
    embed_input_state_t state = embed_input_state_make(true);
    embed_input_state_apply(&state, EMBED_INPUT_TRIGGER_ATTACH);
    embed_input_state_apply(&state, EMBED_INPUT_TRIGGER_VIEWER_FOCUSED);

    embed_input_state_result_t result = embed_input_state_apply(&state, EMBED_INPUT_TRIGGER_VIEWER_MOUSE_DOWN);
    if (!state.viewer_focused) return 1;
    if (!result.allow_shortcuts) return 1;
    if (!result.keyboard_owned) return 1;
    return 0;
}

static int test_embed_host_focus_return_clears_keyboard_ownership(void) {
    embed_input_state_t state = embed_input_state_make(true);
    embed_input_state_apply(&state, EMBED_INPUT_TRIGGER_ATTACH);
    embed_input_state_apply(&state, EMBED_INPUT_TRIGGER_VIEWER_MOUSE_DOWN);

    embed_input_state_result_t result = embed_input_state_apply(&state, EMBED_INPUT_TRIGGER_HOST_FOCUS_GAINED);
    if (state.viewer_focused) return 1;
    if (result.allow_shortcuts) return 1;
    if (result.keyboard_owned) return 1;
    if (result.cancel_interaction) return 1;
    return 0;
}

static int test_embed_pointer_leave_keeps_active_drag_while_host_is_active(void) {
    embed_input_state_t state = embed_input_state_make(true);
    embed_input_state_apply(&state, EMBED_INPUT_TRIGGER_ATTACH);
    embed_input_state_apply(&state, EMBED_INPUT_TRIGGER_VIEWER_MOUSE_DOWN);

    embed_input_state_result_t result = embed_input_state_apply(&state, EMBED_INPUT_TRIGGER_INTERACTION_BEGAN);
    if (!result.interaction_active) return 1;

    result = embed_input_state_apply(&state, EMBED_INPUT_TRIGGER_POINTER_LEFT_VIEWER);
    if (!result.interaction_active) return 1;
    if (result.cancel_interaction) return 1;
    return 0;
}

static int test_embed_host_deactivation_requests_cancel_reset(void) {
    embed_input_state_t state = embed_input_state_make(true);
    embed_input_state_apply(&state, EMBED_INPUT_TRIGGER_ATTACH);
    embed_input_state_apply(&state, EMBED_INPUT_TRIGGER_VIEWER_MOUSE_DOWN);
    embed_input_state_apply(&state, EMBED_INPUT_TRIGGER_INTERACTION_BEGAN);

    embed_input_state_result_t result = embed_input_state_apply(&state, EMBED_INPUT_TRIGGER_HOST_DEACTIVATED);
    if (!result.cancel_interaction) return 1;
    if (result.interaction_active) return 1;
    if (result.allow_shortcuts) return 1;
    if (result.keyboard_owned) return 1;
    return 0;
}

typedef int (*embed_input_test_fn_t)(void);
typedef struct {
    const char* name;
    embed_input_test_fn_t fn;
} embed_input_test_case_t;

int main(void) {
    static const embed_input_test_case_t tests[] = {
        { "test_embed_attach_and_focus_do_not_grant_keyboard", test_embed_attach_and_focus_do_not_grant_keyboard },
        { "test_embed_first_click_grants_keyboard_ownership", test_embed_first_click_grants_keyboard_ownership },
        { "test_embed_host_focus_return_clears_keyboard_ownership", test_embed_host_focus_return_clears_keyboard_ownership },
        { "test_embed_pointer_leave_keeps_active_drag_while_host_is_active", test_embed_pointer_leave_keeps_active_drag_while_host_is_active },
        { "test_embed_host_deactivation_requests_cancel_reset", test_embed_host_deactivation_requests_cancel_reset },
    };

    for (size_t i = 0; i < (sizeof(tests) / sizeof(tests[0])); ++i) {
        if (tests[i].fn() != 0) {
            fprintf(stderr, "FAILED: %s\n", tests[i].name);
            return 1;
        }
    }
    return 0;
}
