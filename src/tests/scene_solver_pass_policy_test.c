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

static int test_recalculate_stops_by_tolerance(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(1, 1, 1, 1));
    ecs_entity_t p1 = scene_add_point_to_sketch(&scene, sketch, vec3_make(0, 0, 0), vec4_make(1, 1, 1, 1), 0.01f);
    ecs_entity_t p2 = scene_add_point_to_sketch(&scene, sketch, vec3_make(0.00005f, 0, 0), vec4_make(1, 1, 1, 1), 0.01f);
    if (!sketch || !p1 || !p2) return 1;

    ecs_entity_t participants[2] = { p1, p2 };
    ecs_entity_t c = scene_add_constraint_to_sketch(&scene, sketch, CONSTRAINT_COINCIDENT, participants, 2, 0.0f, false);
    if (!c) return 1;

    SketchComp *sk = ecs_world_get_sketch(scene.world, sketch);
    if (!sk) return 1;
    if (!scene_solver_set_position_tolerance(&scene, sketch, 1e-4f)) return 1;
    if (!scene_solver_set_max_passes(&scene, sketch, 10u)) return 1;

    bool solved = scene_solver_request_recalculate(&scene, sketch);
    int ok = solved && sk->status == SKETCH_STATUS_SOLVED;
    ecs_world_shutdown(&world);
    return ok ? 0 : 1; // D-05
}

static int test_recalculate_max_pass_cap_and_explicit_diagnostic(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(1, 1, 1, 1));
    ecs_entity_t p1 = scene_add_point_to_sketch(&scene, sketch, vec3_make(0, 0, 0), vec4_make(1, 1, 1, 1), 0.01f);
    ecs_entity_t p2 = scene_add_point_to_sketch(&scene, sketch, vec3_make(2, 0, 0), vec4_make(1, 1, 1, 1), 0.01f);
    ecs_entity_t p3 = scene_add_point_to_sketch(&scene, sketch, vec3_make(4, 0, 0), vec4_make(1, 1, 1, 1), 0.01f);
    if (!sketch || !p1 || !p2 || !p3) return 1;

    ecs_entity_t c1_participants[2] = { p1, p2 };
    ecs_entity_t c2_participants[2] = { p2, p3 };
    ecs_entity_t c1 = scene_add_constraint_to_sketch(&scene, sketch, CONSTRAINT_COINCIDENT, c1_participants, 2, 0.0f, false);
    ecs_entity_t c2 = scene_add_constraint_to_sketch(&scene, sketch, CONSTRAINT_COINCIDENT, c2_participants, 2, 0.0f, false);
    if (!c1 || !c2) return 1;

    SketchComp *sk = ecs_world_get_sketch(scene.world, sketch);
    if (!sk) return 1;
    if (!scene_solver_set_max_passes(&scene, sketch, 1u)) return 1;

    bool solved = scene_solver_request_recalculate(&scene, sketch);
    int diag_count = scene_solver_diagnostic_count(&scene, sketch);
    const sketch_solver_diagnostic_t *last =
        scene_solver_diagnostic_at(&scene, sketch, diag_count > 0 ? diag_count - 1 : -1);

    int ok = (!solved &&
              sk->status == SKETCH_STATUS_ERROR &&
              diag_count > 0 &&
              last && strcmp(last->message, "max passes reached") == 0);
    ecs_world_shutdown(&world);
    return ok ? 0 : 1; // D-06, D-07
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
