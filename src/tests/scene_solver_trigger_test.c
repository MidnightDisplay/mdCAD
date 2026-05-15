#include "../ecs/ecs_world.h"
#include "../ecs/ecs_scene.h"
#include "win32_embed_test_stub.h"
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif
void sokol_main(void) {}
#ifdef __cplusplus
}
#endif

static int test_auto_request_coalesces_and_only_flushes_after_debounce(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(1, 1, 1, 1));
    if (!sketch) return 1;
    SketchComp *sk = ecs_world_get_sketch(scene.world, sketch);
    if (!sk) return 1;

    if (!scene_solver_request_auto(&scene, sketch)) return 1;
    uint32_t serial_after_first = sk->solve_request_serial;
    if (!scene_solver_request_auto(&scene, sketch)) return 1;
    if (!scene_solver_request_auto(&scene, sketch)) return 1;

    if (!sk->auto_solve_pending || sk->solve_request_serial != serial_after_first) {
        ecs_world_shutdown(&world);
        return 1;
    }

    uint32_t completed_before = sk->solve_completed_serial;
    sk->auto_solve_queued_at_ms = 0;
    scene_solver_process_auto_queue(&scene);
    sk = ecs_world_get_sketch(scene.world, sketch);

    int ok = (sk &&
              !sk->auto_solve_pending &&
              sk->solve_completed_serial > completed_before);
    ecs_world_shutdown(&world);
    return ok ? 0 : 1;
}

static int test_manual_recalculate_cancels_pending_auto_queue(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(1, 1, 1, 1));
    if (!sketch) return 1;
    SketchComp *sk = ecs_world_get_sketch(scene.world, sketch);
    if (!sk) return 1;

    if (!scene_solver_request_auto(&scene, sketch)) return 1;
    if (!sk->auto_solve_pending) return 1;
    sk->auto_solve_queued_at_ms = 0;

    if (!scene_solver_request_recalculate(&scene, sketch)) {
        ecs_world_shutdown(&world);
        return 1;
    }
    uint32_t completed_after_manual = sk->solve_completed_serial;

    scene_solver_process_auto_queue(&scene);
    sk = ecs_world_get_sketch(scene.world, sketch);
    int ok = (sk &&
              !sk->auto_solve_pending &&
              sk->solve_completed_serial == completed_after_manual);
    ecs_world_shutdown(&world);
    return ok ? 0 : 1;
}

static int test_zero_debounce_flushes_auto_queue_immediately(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(1, 1, 1, 1));
    if (!sketch) return 1;
    SketchComp *sk = ecs_world_get_sketch(scene.world, sketch);
    if (!sk) return 1;

    if (!scene_solver_set_auto_solve_debounce_ms(&scene, sketch, 0u)) {
        ecs_world_shutdown(&world);
        return 1;
    }
    if (scene_solver_auto_solve_debounce_ms(&scene, sketch) != 0u) {
        ecs_world_shutdown(&world);
        return 1;
    }

    if (!scene_solver_request_auto(&scene, sketch)) {
        ecs_world_shutdown(&world);
        return 1;
    }

    uint32_t completed_before = sk->solve_completed_serial;
    scene_solver_process_auto_queue(&scene);
    sk = ecs_world_get_sketch(scene.world, sketch);

    int ok = (sk &&
              !sk->auto_solve_pending &&
              sk->solve_completed_serial > completed_before);
    ecs_world_shutdown(&world);
    return ok ? 0 : 1;
}

typedef int (*test_fn_t)(void);
typedef struct { const char *name; test_fn_t fn; } test_case_t;

int main(void) {
    static const test_case_t tests[] = {
        { "test_auto_request_coalesces_and_only_flushes_after_debounce",
          test_auto_request_coalesces_and_only_flushes_after_debounce },
        { "test_manual_recalculate_cancels_pending_auto_queue",
          test_manual_recalculate_cancels_pending_auto_queue },
        { "test_zero_debounce_flushes_auto_queue_immediately",
          test_zero_debounce_flushes_auto_queue_immediately },
    };

    for (size_t i = 0; i < (sizeof(tests) / sizeof(tests[0])); ++i) {
        if (tests[i].fn() != 0) {
            fprintf(stderr, "FAILED: %s\n", tests[i].name);
            return 1;
        }
    }
    return 0;
}
