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

static int test_diagnostics_dedupe_baseline_allows_non_identical_messages(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(1, 1, 1, 1));
    if (!sketch) return 1;

    if (!scene_solver_add_diagnostic(&scene, sketch, SKETCH_SOLVER_DIAG_ERROR, "1", "mismatch-1", 0)) return 1;
    if (!scene_solver_add_diagnostic(&scene, sketch, SKETCH_SOLVER_DIAG_ERROR, "2", "mismatch-2", 0)) return 1;

    int count = scene_solver_diagnostic_count(&scene, sketch);
    const sketch_solver_diagnostic_t *d0 = scene_solver_diagnostic_at(&scene, sketch, 0);
    const sketch_solver_diagnostic_t *d1 = scene_solver_diagnostic_at(&scene, sketch, 1);

    int ok = (count == 2 && d0 && d1 && strcmp(d0->message, d1->message) != 0);
    ecs_world_shutdown(&world);
    return ok ? 0 : 1;
}

static int test_diagnostics_dedupe_suppresses_identical_consecutive_entries(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(1, 1, 1, 1));
    if (!sketch) return 1;

    if (!scene_solver_add_diagnostic(&scene, sketch, SKETCH_SOLVER_DIAG_WARNING, "1", "same-message", 11)) return 1;
    if (!scene_solver_add_diagnostic(&scene, sketch, SKETCH_SOLVER_DIAG_WARNING, "2", "same-message", 11)) return 1;

    int count = scene_solver_diagnostic_count(&scene, sketch);
    const sketch_solver_diagnostic_t *d0 = scene_solver_diagnostic_at(&scene, sketch, 0);
    int ok = (count == 1 && d0 && strcmp(d0->timestamp, "1") == 0);
    ecs_world_shutdown(&world);
    return ok ? 0 : 1;
}

static int test_failure_implication_orders_constraints_and_participants_deterministically(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(1, 1, 1, 1));
    ecs_entity_t p1 = scene_add_point_to_sketch(&scene, sketch, vec3_make(0, 0, 0), vec4_make(1, 1, 1, 1), 0.01f);
    ecs_entity_t p2 = scene_add_point_to_sketch(&scene, sketch, vec3_make(1, 0, 0), vec4_make(1, 1, 1, 1), 0.01f);
    ecs_entity_t p3 = scene_add_point_to_sketch(&scene, sketch, vec3_make(2, 0, 0), vec4_make(1, 1, 1, 1), 0.01f);
    if (!sketch || !p1 || !p2 || !p3) return 1;

    ecs_entity_t participants_a[2] = { p1, p3 };
    ecs_entity_t participants_b[2] = { p2, p1 };
    ecs_entity_t c1 = scene_add_constraint_to_sketch(&scene, sketch, CONSTRAINT_COINCIDENT, participants_a, 2, 0.0f, false);
    ecs_entity_t c2 = scene_add_constraint_to_sketch(&scene, sketch, CONSTRAINT_COINCIDENT, participants_b, 2, 0.0f, false);
    if (!c1 || !c2) return 1;

    ecs_entity_t implicated_unsorted[2] = { c2, c1 };
    if (!scene_solver_set_failure_implication(&scene, sketch, implicated_unsorted, 2, "deterministic")) return 1;
    const scene_solver_failure_implication_t *imp = scene_solver_failure_implication(&scene);
    if (!imp || !imp->active) return 1;

    int ok = (imp->implicated_constraint_count == 2 &&
              imp->implicated_constraints[0] < imp->implicated_constraints[1] &&
              imp->first_constraint == imp->implicated_constraints[0] &&
              imp->participant_count >= 3 &&
              imp->participants[0] < imp->participants[1] &&
              imp->participants[1] < imp->participants[2]);
    ecs_world_shutdown(&world);
    return ok ? 0 : 1;
}

typedef int (*test_fn_t)(void);
typedef struct { const char *name; test_fn_t fn; } test_case_t;

int main(void) {
    static const test_case_t tests[] = {
        { "test_diagnostics_dedupe_baseline_allows_non_identical_messages", test_diagnostics_dedupe_baseline_allows_non_identical_messages },
        { "test_diagnostics_dedupe_suppresses_identical_consecutive_entries", test_diagnostics_dedupe_suppresses_identical_consecutive_entries },
        { "test_failure_implication_orders_constraints_and_participants_deterministically", test_failure_implication_orders_constraints_and_participants_deterministically },
    };

    for (size_t i = 0; i < (sizeof(tests) / sizeof(tests[0])); ++i) {
        if (tests[i].fn() != 0) {
            fprintf(stderr, "FAILED: %s\n", tests[i].name);
            return 1;
        }
    }
    return 0;
}
