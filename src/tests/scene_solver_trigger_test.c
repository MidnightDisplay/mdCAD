#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef struct {
    bool auto_solve_pending;
    uint32_t solve_request_serial;
    uint32_t solve_completed_serial;
    uint32_t debounce_ms;
} trigger_policy_state_t;

static trigger_policy_state_t trigger_policy_default(void) {
    trigger_policy_state_t state = {0};
    state.debounce_ms = 0; // RED: should be 50ms per D-02
    return state;
}

static void trigger_policy_request_auto(trigger_policy_state_t *state) {
    if (!state) return;
    if (!state->auto_solve_pending) {
        state->auto_solve_pending = true;
        state->solve_request_serial++;
    }
}

static void trigger_policy_manual_recalculate(trigger_policy_state_t *state) {
    if (!state) return;
    state->auto_solve_pending = false;
    state->solve_request_serial++;
    state->solve_completed_serial = state->solve_request_serial;
}

static bool trigger_policy_flush_if_due(trigger_policy_state_t *state, uint32_t elapsed_ms) {
    if (!state) return false;
    if (!state->auto_solve_pending) return false;
    if (elapsed_ms < state->debounce_ms) return false;
    state->auto_solve_pending = false;
    state->solve_completed_serial = state->solve_request_serial;
    return true;
}

static int test_auto_request_debounce_default_and_coalescing(void) {
    trigger_policy_state_t state = trigger_policy_default();
    if (state.debounce_ms != 50) return 1; // D-02

    trigger_policy_request_auto(&state);
    trigger_policy_request_auto(&state);
    trigger_policy_request_auto(&state);

    return (state.auto_solve_pending && state.solve_request_serial == 1) ? 0 : 1; // D-01, D-03
}

static int test_manual_recalculate_clears_pending_and_stale_flush(void) {
    trigger_policy_state_t state = trigger_policy_default();
    trigger_policy_request_auto(&state);
    if (!state.auto_solve_pending) return 1;

    trigger_policy_manual_recalculate(&state); // D-04
    if (state.auto_solve_pending) return 1;

    return trigger_policy_flush_if_due(&state, 100) ? 1 : 0;
}

typedef int (*test_fn_t)(void);
typedef struct { const char *name; test_fn_t fn; } test_case_t;

int main(void) {
    static const test_case_t tests[] = {
        { "test_auto_request_debounce_default_and_coalescing", test_auto_request_debounce_default_and_coalescing },
        { "test_manual_recalculate_clears_pending_and_stale_flush", test_manual_recalculate_clears_pending_and_stale_flush },
    };

    for (size_t i = 0; i < (sizeof(tests) / sizeof(tests[0])); ++i) {
        if (tests[i].fn() != 0) {
            fprintf(stderr, "FAILED: %s\n", tests[i].name);
            return 1;
        }
    }
    return 0;
}
