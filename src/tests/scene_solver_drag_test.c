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

static int test_drag_fixed_geometry_is_unsat_with_zero_projection(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(1, 1, 1, 1));
    ecs_entity_t p = scene_add_point_to_sketch(&scene, sketch, vec3_make(0, 0, 0), vec4_make(1, 1, 1, 1), 0.01f);
    if (!sketch || !p) return 1;

    SketchGeometryStateComp *state = ecs_world_get_sketch_geometry_state(scene.world, p);
    if (!state) return 1;
    state->fixed = true;

    scene_solver_drag_decision_t decision = {0};
    ecs_entity_t drag_entities[1] = { p };
    bool ok_call = scene_solver_can_apply_drag(&scene, sketch, drag_entities, 1, vec3_make(1.0f, 0.0f, 0.0f), &decision);
    ecs_world_shutdown(&world);
    if (!ok_call) return 1;

    return (decision.result == SCENE_SOLVER_DRAG_UNSATISFIABLE &&
            decision.projected_delta.x == 0.0f &&
            decision.projected_delta.y == 0.0f &&
            decision.projected_delta.z == 0.0f) ? 0 : 1;
}

static int test_drag_feasible_large_delta_is_bounded_projection(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(1, 1, 1, 1));
    ecs_entity_t p = scene_add_point_to_sketch(&scene, sketch, vec3_make(0, 0, 0), vec4_make(1, 1, 1, 1), 0.01f);
    if (!sketch || !p) return 1;

    scene_solver_drag_decision_t decision = {0};
    ecs_entity_t drag_entities[1] = { p };
    bool ok_call = scene_solver_can_apply_drag(&scene, sketch, drag_entities, 1, vec3_make(10.0f, 0.0f, 0.0f), &decision);
    ecs_world_shutdown(&world);
    if (!ok_call) return 1;

    return (decision.result == SCENE_SOLVER_DRAG_FEASIBLE &&
            decision.projected_delta.x > 0.0f &&
            decision.projected_delta.x < 10.0f &&
            decision.projected_delta.y == 0.0f &&
            decision.projected_delta.z == 0.0f) ? 0 : 1;
}

static int test_drag_unsat_preserves_last_valid_geometry_state(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(1, 1, 1, 1));
    ecs_entity_t p = scene_add_point_to_sketch(&scene, sketch, vec3_make(2.0f, 3.0f, 0.0f), vec4_make(1, 1, 1, 1), 0.01f);
    if (!sketch || !p) return 1;

    SketchGeometryStateComp *geom_state = ecs_world_get_sketch_geometry_state(scene.world, p);
    GeometryComp *g = ecs_world_get_geometry(scene.world, p);
    if (!geom_state || !g || g->type != GEOM_POINT) return 1;
    vec3_t before = g->data.point.point;
    geom_state->fixed = true;

    scene_solver_drag_decision_t decision = {0};
    ecs_entity_t drag_entities[1] = { p };
    bool ok_call = scene_solver_can_apply_drag(&scene, sketch, drag_entities, 1, vec3_make(5.0f, -2.0f, 0.0f), &decision);
    if (!ok_call) return 1;

    vec3_t after = g->data.point.point;
    int ok = (decision.result == SCENE_SOLVER_DRAG_UNSATISFIABLE &&
              before.x == after.x &&
              before.y == after.y &&
              before.z == after.z);
    ecs_world_shutdown(&world);
    return ok ? 0 : 1;
}

static int test_drag_rejected_diagnostic_payload_baseline(void) {
    scene_solver_drag_decision_t decision = {0};
    decision.result = SCENE_SOLVER_DRAG_UNSATISFIABLE;
    decision.first_implicated_constraint = (ecs_entity_t)77;

    scene_solver_drag_diagnostic_event_t event = {0};
    if (!scene_solver_drag_make_rejected_diagnostic(&decision, &event)) return 1;
    return (event.severity == SKETCH_SOLVER_DIAG_WARNING &&
            event.implicated_constraint == (ecs_entity_t)77 &&
            strstr(event.message, "Drag rejected") != NULL) ? 0 : 1;
}

typedef int (*test_fn_t)(void);
typedef struct { const char *name; test_fn_t fn; } test_case_t;

int main(void) {
    static const test_case_t tests[] = {
        { "test_drag_fixed_geometry_is_unsat_with_zero_projection", test_drag_fixed_geometry_is_unsat_with_zero_projection },
        { "test_drag_feasible_large_delta_is_bounded_projection", test_drag_feasible_large_delta_is_bounded_projection },
        { "test_drag_unsat_preserves_last_valid_geometry_state", test_drag_unsat_preserves_last_valid_geometry_state },
        { "test_drag_rejected_diagnostic_payload_baseline", test_drag_rejected_diagnostic_payload_baseline },
    };

    for (size_t i = 0; i < (sizeof(tests) / sizeof(tests[0])); ++i) {
        if (tests[i].fn() != 0) {
            fprintf(stderr, "FAILED: %s\n", tests[i].name);
            return 1;
        }
    }
    return 0;
}
