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

static int test_recalculate_updates_solver_counters(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(1, 1, 1, 1));
    if (sketch == 0) return 1;

    SketchComp *sk = ecs_world_get_sketch(scene.world, sketch);
    if (!sk) return 1;
    uint32_t before_request = sk->solve_request_serial;
    uint32_t before_complete = sk->solve_completed_serial;

    if (!scene_solver_request_recalculate(&scene, sketch)) return 1;

    sk = ecs_world_get_sketch(scene.world, sketch);
    int ok = (sk &&
              sk->solve_request_serial == (before_request + 1) &&
              sk->solve_completed_serial == (before_complete + 1) &&
              sk->last_solve_timestamp_ms > 0);
    ecs_world_shutdown(&world);
    return ok ? 0 : 1;
}

static int test_failure_implication_dedupes_participants(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(1, 1, 1, 1));
    ecs_entity_t p1 = scene_add_point_to_sketch(&scene, sketch, vec3_make(0, 0, 0), vec4_make(1, 1, 1, 1), 0.01f);
    ecs_entity_t p2 = scene_add_point_to_sketch(&scene, sketch, vec3_make(1, 0, 0), vec4_make(1, 1, 1, 1), 0.01f);
    if (!sketch || !p1 || !p2) return 1;

    ecs_entity_t participants[2] = { p1, p2 };
    ecs_entity_t c1 = scene_add_constraint_to_sketch(&scene, sketch, CONSTRAINT_COINCIDENT, participants, 2, 0.0f, false);
    ecs_entity_t c2 = scene_add_constraint_to_sketch(&scene, sketch, CONSTRAINT_COINCIDENT, participants, 2, 0.0f, false);
    if (!c1 || !c2) return 1;

    ecs_entity_t implicated[2] = { c1, c2 };
    if (!scene_solver_set_failure_implication(&scene, sketch, implicated, 2, "baseline failure")) return 1;
    const scene_solver_failure_implication_t *imp = scene_solver_failure_implication(&scene);
    if (!imp || !imp->active) return 1;

    int ok = (imp->implicated_constraint_count == 2 &&
              imp->participant_count == 2 &&
              strcmp(imp->reason, "baseline failure") == 0);
    ecs_world_shutdown(&world);
    return ok ? 0 : 1;
}

typedef int (*test_fn_t)(void);
typedef struct { const char *name; test_fn_t fn; } test_case_t;

int main(void) {
    static const test_case_t tests[] = {
        { "test_recalculate_updates_solver_counters", test_recalculate_updates_solver_counters },
        { "test_failure_implication_dedupes_participants", test_failure_implication_dedupes_participants },
    };

    for (size_t i = 0; i < (sizeof(tests) / sizeof(tests[0])); ++i) {
        if (tests[i].fn() != 0) {
            fprintf(stderr, "FAILED: %s\n", tests[i].name);
            return 1;
        }
    }
    return 0;
}
