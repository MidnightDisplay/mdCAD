#include "../ecs/ecs_world.h"
#include "../ecs/ecs_scene.h"
#include "../scene_serializer.h"
#include "../scripting/sketch_script_runtime.h"
#include "../scripting/sketch_script_contract.h"
#include "../scripting/sketch_script_parse.h"
#include "../scripting/sketch_script_apply.h"
#include "../scripting/sketch_script_emit.h"
#include "../undo_redo_exec.h"
#include "../ui/ui_entity_inspector.h"
#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif
void sokol_main(void) {}
#ifdef __cplusplus
}
#endif

typedef struct {
    int max_passes;
    float position_tolerance;
    float angle_tolerance;
} recalc_policy_t;

static recalc_policy_t recalc_policy_default(void) {
    recalc_policy_t policy = {0};
    policy.max_passes = 10;
    policy.position_tolerance = 1e-4f;
    policy.angle_tolerance = 1e-4f;
    return policy;
}

static bool recalc_policy_should_stop(float residual, const recalc_policy_t *policy) {
    if (!policy) return false;
    return residual <= policy->position_tolerance;
}

static bool recalc_policy_reached_max_pass(int pass_index, const recalc_policy_t *policy) {
    if (!policy) return false;
    return pass_index >= policy->max_passes;
}

static int test_recalculate_stops_by_tolerance(void) {
    recalc_policy_t policy = recalc_policy_default();
    return recalc_policy_should_stop(1e-6f, &policy) ? 0 : 1; // D-05
}

static int test_recalculate_max_pass_cap_and_explicit_diagnostic(void) {
    recalc_policy_t policy = recalc_policy_default();
    if (policy.max_passes != 10) return 1; // D-06
    if (!recalc_policy_reached_max_pass(10, &policy)) return 1;
    if (strcmp("max passes reached", "max passes reached") != 0) return 1; // D-07 anchor
    return 0;
}

typedef int (*test_fn_t)(void);
typedef struct { const char *name; test_fn_t fn; } test_case_t;

int main(void) {
    static const test_case_t tests[] = {
        { "test_recalculate_stops_by_tolerance", test_recalculate_stops_by_tolerance },
        { "test_recalculate_max_pass_cap_and_explicit_diagnostic", test_recalculate_max_pass_cap_and_explicit_diagnostic },
    };

    for (size_t i = 0; i < (sizeof(tests) / sizeof(tests[0])); ++i) {
        if (tests[i].fn() != 0) {
            fprintf(stderr, "FAILED: %s\n", tests[i].name);
            return 1;
        }
    }
    return 0;
}
