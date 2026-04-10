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

static int test_arci01_unsat_reports_family_specific_diagnostic(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(1, 1, 1, 1));
    ecs_entity_t line = scene_add_line_to_sketch(&scene, sketch,
                                                 vec3_make(0.0f, 0.0f, 0.0f),
                                                 vec3_make(1.0f, 0.0f, 0.0f),
                                                 vec4_make(1, 1, 1, 1), 1.0f);
    ecs_entity_t arc = scene_add_arc_to_sketch(&scene, sketch,
                                               vec3_make(0.0f, 0.0f, 0.0f), 2.0f,
                                               0.0f, 1.0f, vec3_make(0.0f, 0.0f, 1.0f),
                                               vec4_make(1, 1, 1, 1), 1.0f);
    if (!sketch || !line || !arc) return 1;

    SketchGeometryStateComp *line_state = ecs_world_get_sketch_geometry_state(scene.world, line);
    SketchGeometryStateComp *arc_state = ecs_world_get_sketch_geometry_state(scene.world, arc);
    if (!line_state || !arc_state) return 1;
    line_state->fixed = true;
    arc_state->fixed = true;

    ecs_entity_t participants[2] = { line, arc };
    ecs_entity_t c = scene_add_constraint_to_sketch(&scene, sketch, CONSTRAINT_ARC_AXIS_LINE, participants, 2, 0.0f, false);
    if (!c) return 1;

    bool solved = scene_solver_request_recalculate(&scene, sketch);
    int diag_count = scene_solver_diagnostic_count(&scene, sketch);
    const sketch_solver_diagnostic_t *last =
        scene_solver_diagnostic_at(&scene, sketch, diag_count > 0 ? diag_count - 1 : -1);
    int ok = (!solved &&
              diag_count > 0 &&
              last &&
              strcmp(last->message, "Unsatisfied arc-axis-vs-line constraint.") == 0);
    ecs_world_shutdown(&world);
    return ok ? 0 : 1;
}

static int test_arci02_unsat_reports_family_specific_diagnostic(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(1, 1, 1, 1));
    ecs_entity_t line = scene_add_line_to_sketch(&scene, sketch,
                                                 vec3_make(2.0f, -2.0f, 0.0f),
                                                 vec3_make(2.0f, 2.0f, 0.0f),
                                                 vec4_make(1, 1, 1, 1), 1.0f);
    ecs_entity_t arc = scene_add_arc_to_sketch(&scene, sketch,
                                               vec3_make(0.0f, 0.0f, 0.0f), 2.0f,
                                               0.0f, 1.0f, vec3_make(0.0f, 0.0f, 1.0f),
                                               vec4_make(1, 1, 1, 1), 1.0f);
    if (!sketch || !line || !arc) return 1;

    SketchGeometryStateComp *line_state = ecs_world_get_sketch_geometry_state(scene.world, line);
    SketchGeometryStateComp *arc_state = ecs_world_get_sketch_geometry_state(scene.world, arc);
    if (!line_state || !arc_state) return 1;
    line_state->fixed = true;
    arc_state->fixed = true;

    constraint_participant_descriptor_t desc[2] = {
        constraint_participant_descriptor_make((uint64_t)line, CONSTRAINT_PARTICIPANT_ROLE_POINT_A, 0),
        constraint_participant_descriptor_make((uint64_t)arc, CONSTRAINT_PARTICIPANT_ROLE_POINT_A, 0),
    };
    ecs_entity_t c = scene_add_constraint_to_sketch_with_descriptors(
        &scene, sketch, CONSTRAINT_LINE_ARC_ENDPOINT_TANGENCY, desc, 2, 0.0f, false);
    if (!c) return 1;

    bool solved = scene_solver_request_recalculate(&scene, sketch);
    int diag_count = scene_solver_diagnostic_count(&scene, sketch);
    const sketch_solver_diagnostic_t *last =
        scene_solver_diagnostic_at(&scene, sketch, diag_count > 0 ? diag_count - 1 : -1);
    int ok = (!solved &&
              diag_count > 0 &&
              last &&
              strcmp(last->message, "Unsatisfied line-arc endpoint tangency constraint.") == 0);
    ecs_world_shutdown(&world);
    return ok ? 0 : 1;
}

static int test_arci03_unsat_reports_family_specific_diagnostic(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(1, 1, 1, 1));
    ecs_entity_t arc = scene_add_arc_to_sketch(&scene, sketch,
                                               vec3_make(0.0f, 0.0f, 0.0f), 2.0f,
                                               0.0f, 1.0f, vec3_make(0.0f, 0.0f, 1.0f),
                                               vec4_make(1, 1, 1, 1), 1.0f);
    if (!sketch || !arc) return 1;

    SketchGeometryStateComp *arc_state = ecs_world_get_sketch_geometry_state(scene.world, arc);
    if (!arc_state) return 1;
    arc_state->fixed = true;

    constraint_participant_descriptor_t desc[2] = {
        constraint_participant_descriptor_make((uint64_t)arc, CONSTRAINT_PARTICIPANT_ROLE_POINT_A, 0),
        constraint_participant_descriptor_make((uint64_t)arc, CONSTRAINT_PARTICIPANT_ROLE_POINT_B, 1),
    };
    ecs_entity_t c = scene_add_constraint_to_sketch_with_descriptors(
        &scene, sketch, CONSTRAINT_ARC_ENDPOINT_ANGLE, desc, 2, 0.5f, false);
    if (!c) return 1;

    bool solved = scene_solver_request_recalculate(&scene, sketch);
    int diag_count = scene_solver_diagnostic_count(&scene, sketch);
    const sketch_solver_diagnostic_t *last =
        scene_solver_diagnostic_at(&scene, sketch, diag_count > 0 ? diag_count - 1 : -1);
    int ok = (!solved &&
              diag_count > 0 &&
              last &&
              strcmp(last->message, "Unsatisfied arc endpoint-angle constraint.") == 0);
    ecs_world_shutdown(&world);
    return ok ? 0 : 1;
}

static int test_line_line_parallel_unsat_reports_family_specific_diagnostic_lcon05(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(1, 1, 1, 1));
    ecs_entity_t line_a = scene_add_line_to_sketch(&scene, sketch,
                                                   vec3_make(0.0f, 0.0f, 0.0f),
                                                   vec3_make(2.0f, 0.0f, 0.0f),
                                                   vec4_make(1, 1, 1, 1), 1.0f);
    ecs_entity_t line_b = scene_add_line_to_sketch(&scene, sketch,
                                                   vec3_make(0.0f, 1.0f, 0.0f),
                                                   vec3_make(2.0f, 2.0f, 0.0f),
                                                   vec4_make(1, 1, 1, 1), 1.0f);
    if (!sketch || !line_a || !line_b) return 1;

    SketchGeometryStateComp *state_a = ecs_world_get_sketch_geometry_state(scene.world, line_a);
    SketchGeometryStateComp *state_b = ecs_world_get_sketch_geometry_state(scene.world, line_b);
    if (!state_a || !state_b) return 1;
    state_a->fixed = true;
    state_b->fixed = true;

    ecs_entity_t participants[2] = { line_a, line_b };
    ecs_entity_t c = scene_add_constraint_to_sketch(&scene, sketch, CONSTRAINT_PARALLEL, participants, 2, 0.0f, false);
    if (!c) return 1;

    bool solved = scene_solver_request_recalculate(&scene, sketch);
    int diag_count = scene_solver_diagnostic_count(&scene, sketch);
    const sketch_solver_diagnostic_t *last =
        scene_solver_diagnostic_at(&scene, sketch, diag_count > 0 ? diag_count - 1 : -1);
    int ok = (!solved &&
              diag_count > 0 &&
              last &&
              strcmp(last->message, "Unsatisfied parallel constraint.") == 0);
    ecs_world_shutdown(&world);
    return ok ? 0 : 1;
}

static int test_line_line_perpendicular_unsat_reports_family_specific_diagnostic_lcon05(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(1, 1, 1, 1));
    ecs_entity_t line_a = scene_add_line_to_sketch(&scene, sketch,
                                                   vec3_make(0.0f, 0.0f, 0.0f),
                                                   vec3_make(2.0f, 0.0f, 0.0f),
                                                   vec4_make(1, 1, 1, 1), 1.0f);
    ecs_entity_t line_b = scene_add_line_to_sketch(&scene, sketch,
                                                   vec3_make(1.0f, 1.0f, 0.0f),
                                                   vec3_make(2.0f, 2.0f, 0.0f),
                                                   vec4_make(1, 1, 1, 1), 1.0f);
    if (!sketch || !line_a || !line_b) return 1;

    SketchGeometryStateComp *state_a = ecs_world_get_sketch_geometry_state(scene.world, line_a);
    SketchGeometryStateComp *state_b = ecs_world_get_sketch_geometry_state(scene.world, line_b);
    if (!state_a || !state_b) return 1;
    state_a->fixed = true;
    state_b->fixed = true;

    ecs_entity_t participants[2] = { line_a, line_b };
    ecs_entity_t c = scene_add_constraint_to_sketch(&scene, sketch, CONSTRAINT_PERPENDICULAR, participants, 2, 0.0f, false);
    if (!c) return 1;

    bool solved = scene_solver_request_recalculate(&scene, sketch);
    int diag_count = scene_solver_diagnostic_count(&scene, sketch);
    const sketch_solver_diagnostic_t *last =
        scene_solver_diagnostic_at(&scene, sketch, diag_count > 0 ? diag_count - 1 : -1);
    int ok = (!solved &&
              diag_count > 0 &&
              last &&
              strcmp(last->message, "Unsatisfied perpendicular constraint.") == 0);
    ecs_world_shutdown(&world);
    return ok ? 0 : 1;
}

static int test_line_line_group_perpendicular_unsat_reports_family_specific_diagnostic_lcon05(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(1, 1, 1, 1));
    ecs_entity_t anchor = scene_add_line_to_sketch(&scene, sketch,
                                                   vec3_make(0.0f, 0.0f, 0.0f),
                                                   vec3_make(3.0f, 0.0f, 0.0f),
                                                   vec4_make(1, 1, 1, 1), 1.0f);
    ecs_entity_t line_b = scene_add_line_to_sketch(&scene, sketch,
                                                   vec3_make(0.0f, 2.0f, 0.0f),
                                                   vec3_make(2.0f, 3.0f, 0.0f),
                                                   vec4_make(1, 1, 1, 1), 1.0f);
    ecs_entity_t line_c = scene_add_line_to_sketch(&scene, sketch,
                                                   vec3_make(0.0f, -2.0f, 0.0f),
                                                   vec3_make(2.0f, -1.0f, 0.0f),
                                                   vec4_make(1, 1, 1, 1), 1.0f);
    if (!sketch || !anchor || !line_b || !line_c) return 1;

    SketchGeometryStateComp *state_a = ecs_world_get_sketch_geometry_state(scene.world, anchor);
    SketchGeometryStateComp *state_b = ecs_world_get_sketch_geometry_state(scene.world, line_b);
    SketchGeometryStateComp *state_c = ecs_world_get_sketch_geometry_state(scene.world, line_c);
    if (!state_a || !state_b || !state_c) return 1;
    state_a->fixed = true;
    state_b->fixed = true;
    state_c->fixed = true;

    ecs_entity_t participants[3] = { line_c, anchor, line_b };
    ecs_entity_t c = scene_add_constraint_to_sketch(&scene, sketch, CONSTRAINT_PERPENDICULAR, participants, 3, 0.0f, false);
    if (!c) return 1;

    bool solved = scene_solver_request_recalculate(&scene, sketch);
    int diag_count = scene_solver_diagnostic_count(&scene, sketch);
    const sketch_solver_diagnostic_t *last =
        scene_solver_diagnostic_at(&scene, sketch, diag_count > 0 ? diag_count - 1 : -1);
    int ok = (!solved &&
              diag_count > 0 &&
              last &&
              strcmp(last->message, "Unsatisfied perpendicular constraint.") == 0);
    ecs_world_shutdown(&world);
    return ok ? 0 : 1;
}

static int test_along_x_unsat_reports_family_specific_diagnostic_alin04(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(1, 1, 1, 1));
    ecs_entity_t line = scene_add_line_to_sketch(&scene, sketch,
                                                 vec3_make(0.0f, 1.0f, 2.0f),
                                                 vec3_make(5.0f, 3.0f, 4.0f),
                                                 vec4_make(1, 1, 1, 1), 1.0f);
    if (!sketch || !line) return 1;

    SketchGeometryStateComp *line_state = ecs_world_get_sketch_geometry_state(scene.world, line);
    if (!line_state) return 1;
    line_state->fixed = true;

    ecs_entity_t participants[1] = { line };
    ecs_entity_t c = scene_add_constraint_to_sketch(&scene, sketch, CONSTRAINT_ALONG_X, participants, 1, 0.0f, false);
    if (!c) return 1;

    bool solved = scene_solver_request_recalculate(&scene, sketch);
    int diag_count = scene_solver_diagnostic_count(&scene, sketch);
    const sketch_solver_diagnostic_t *last =
        scene_solver_diagnostic_at(&scene, sketch, diag_count > 0 ? diag_count - 1 : -1);
    int ok = (!solved &&
              diag_count > 0 &&
              last &&
              strcmp(last->message, "Unsatisfied driving ALONG X constraint.") == 0);
    ecs_world_shutdown(&world);
    return ok ? 0 : 1;
}

static int test_along_y_unsat_reports_family_specific_diagnostic_alin04(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(1, 1, 1, 1));
    ecs_entity_t line = scene_add_line_to_sketch(&scene, sketch,
                                                 vec3_make(1.0f, 0.0f, 2.0f),
                                                 vec3_make(3.0f, 5.0f, 4.0f),
                                                 vec4_make(1, 1, 1, 1), 1.0f);
    if (!sketch || !line) return 1;

    SketchGeometryStateComp *line_state = ecs_world_get_sketch_geometry_state(scene.world, line);
    if (!line_state) return 1;
    line_state->fixed = true;

    ecs_entity_t participants[1] = { line };
    ecs_entity_t c = scene_add_constraint_to_sketch(&scene, sketch, CONSTRAINT_ALONG_Y, participants, 1, 0.0f, false);
    if (!c) return 1;

    bool solved = scene_solver_request_recalculate(&scene, sketch);
    int diag_count = scene_solver_diagnostic_count(&scene, sketch);
    const sketch_solver_diagnostic_t *last =
        scene_solver_diagnostic_at(&scene, sketch, diag_count > 0 ? diag_count - 1 : -1);
    int ok = (!solved &&
              diag_count > 0 &&
              last &&
              strcmp(last->message, "Unsatisfied driving ALONG Y constraint.") == 0);
    ecs_world_shutdown(&world);
    return ok ? 0 : 1;
}

static int test_along_z_unsat_reports_family_specific_diagnostic_alin04(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(1, 1, 1, 1));
    ecs_entity_t line = scene_add_line_to_sketch(&scene, sketch,
                                                 vec3_make(1.0f, 2.0f, 0.0f),
                                                 vec3_make(3.0f, 4.0f, 5.0f),
                                                 vec4_make(1, 1, 1, 1), 1.0f);
    if (!sketch || !line) return 1;

    SketchGeometryStateComp *line_state = ecs_world_get_sketch_geometry_state(scene.world, line);
    if (!line_state) return 1;
    line_state->fixed = true;

    ecs_entity_t participants[1] = { line };
    ecs_entity_t c = scene_add_constraint_to_sketch(&scene, sketch, CONSTRAINT_ALONG_Z, participants, 1, 0.0f, false);
    if (!c) return 1;

    bool solved = scene_solver_request_recalculate(&scene, sketch);
    int diag_count = scene_solver_diagnostic_count(&scene, sketch);
    const sketch_solver_diagnostic_t *last =
        scene_solver_diagnostic_at(&scene, sketch, diag_count > 0 ? diag_count - 1 : -1);
    int ok = (!solved &&
              diag_count > 0 &&
              last &&
              strcmp(last->message, "Unsatisfied driving ALONG Z constraint.") == 0);
    ecs_world_shutdown(&world);
    return ok ? 0 : 1;
}

static int test_tangency_post_failure_diagnostic_remains_family_specific_and_solver_recovers(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(1, 1, 1, 1));
    ecs_entity_t line = scene_add_line_to_sketch(&scene, sketch,
                                                 vec3_make(2.0f, -2.0f, 0.0f),
                                                 vec3_make(2.0f, 2.0f, 0.0f),
                                                 vec4_make(1, 1, 1, 1), 1.0f);
    ecs_entity_t arc = scene_add_arc_to_sketch(&scene, sketch,
                                               vec3_make(0.0f, 0.0f, 0.0f), 2.0f,
                                               0.0f, 1.0f, vec3_make(0.0f, 0.0f, 1.0f),
                                               vec4_make(1, 1, 1, 1), 1.0f);
    if (!sketch || !line || !arc) return 1;

    constraint_participant_descriptor_t desc[2] = {
        constraint_participant_descriptor_make((uint64_t)line, CONSTRAINT_PARTICIPANT_ROLE_POINT_A, 0),
        constraint_participant_descriptor_make((uint64_t)arc, CONSTRAINT_PARTICIPANT_ROLE_POINT_A, 0),
    };
    ecs_entity_t c = scene_add_constraint_to_sketch_with_descriptors(
        &scene, sketch, CONSTRAINT_LINE_ARC_ENDPOINT_TANGENCY, desc, 2, 0.0f, false);
    if (!c) return 1;
    if (!scene_solver_request_recalculate(&scene, sketch)) return 1;

    SketchGeometryStateComp *line_state = ecs_world_get_sketch_geometry_state(scene.world, line);
    SketchGeometryStateComp *arc_state = ecs_world_get_sketch_geometry_state(scene.world, arc);
    if (!line_state || !arc_state) return 1;
    line_state->fixed = true;
    arc_state->fixed = true;

    EndPointsComp *line_endpoints = ecs_world_get_endpoints(scene.world, line);
    endpoint_binding_t line_a_binding = {0};
    if (!line_endpoints ||
        !endpoints_comp_find_binding(line_endpoints, CONSTRAINT_PARTICIPANT_ROLE_POINT_A, &line_a_binding)) {
        return 1;
    }
    ecs_entity_t line_a_endpoint = (ecs_entity_t)line_a_binding.endpoint_entity;
    if (!line_a_endpoint || !ecs_is_alive(scene.world->world, line_a_endpoint)) return 1;
    if (!scene_apply_endpoint_point_world_delta(&scene, line_a_endpoint, vec3_make(0.3f, 0.2f, 0.0f))) return 1;

    bool failed = !scene_solver_request_recalculate(&scene, sketch);
    int diag_count_after_fail = scene_solver_diagnostic_count(&scene, sketch);
    const sketch_solver_diagnostic_t *last_fail =
        scene_solver_diagnostic_at(&scene, sketch, diag_count_after_fail > 0 ? diag_count_after_fail - 1 : -1);
    if (!failed || !last_fail ||
        strcmp(last_fail->message, "Unsatisfied line-arc endpoint tangency constraint.") != 0) {
        ecs_world_shutdown(&world);
        return 1;
    }

    line_state->fixed = false;
    arc_state->fixed = false;
    EndPointsComp *arc_endpoints = ecs_world_get_endpoints(scene.world, arc);
    endpoint_binding_t center_binding = {0};
    if (!arc_endpoints ||
        !endpoints_comp_find_binding(arc_endpoints, CONSTRAINT_PARTICIPANT_ROLE_CENTER, &center_binding)) {
        ecs_world_shutdown(&world);
        return 1;
    }
    ecs_entity_t center_endpoint = (ecs_entity_t)center_binding.endpoint_entity;
    if (!center_endpoint || !ecs_is_alive(scene.world->world, center_endpoint)) {
        ecs_world_shutdown(&world);
        return 1;
    }
    if (!scene_apply_endpoint_point_world_delta(&scene, center_endpoint, vec3_make(-0.2f, 0.1f, 0.0f))) {
        ecs_world_shutdown(&world);
        return 1;
    }
    bool recovered = scene_solver_request_recalculate(&scene, sketch);
    int diag_count_after_recover = scene_solver_diagnostic_count(&scene, sketch);

    int ok = (recovered &&
              diag_count_after_recover >= diag_count_after_fail);
    ecs_world_shutdown(&world);
    return ok ? 0 : 1;
}

static int test_diagnostic_parity_reapply(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(1, 1, 1, 1));
    if (!sketch) return 1;

    const char *bad_script =
        "return {\n"
        "  entities = {\n"
        "    { id = \"geometry_1\", type = \"line\", a = {0, 0, 0}, b = {1, 0, 0}, color = {1, 1, 1, 1} },\n"
        "    { id = \"geometry_2\", type = \"arc\", center = {1, 0, 0}, radius = 1, start_angle = 0, end_angle = 1, normal = {0, 0, 1}, color = {1, 1, 1, 1} }\n"
        "  },\n"
        "  constraints = {\n"
        "    { id = \"constraint_1\", type = \"Line-End Arc-End Tangency\", participants = {\"geometry_1\", \"geometry_2\"} }\n"
        "  }\n"
        "}";

    sketch_script_error_t err_first = {0};
    if (scene_script_apply_commit(&scene, sketch, bad_script, &err_first)) {
        ecs_world_shutdown(&world);
        return 1;
    }
    sketch_script_error_t err_second = {0};
    if (scene_script_apply_commit(&scene, sketch, bad_script, &err_second)) {
        ecs_world_shutdown(&world);
        return 1;
    }

    int diag_count = scene_solver_diagnostic_count(&scene, sketch);
    int ok = (diag_count == 0 &&
              strcmp(err_first.message, err_second.message) == 0 &&
              strstr(err_first.message, "descriptor objects") != NULL);
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
        { "test_arci01_unsat_reports_family_specific_diagnostic", test_arci01_unsat_reports_family_specific_diagnostic },
        { "test_arci02_unsat_reports_family_specific_diagnostic", test_arci02_unsat_reports_family_specific_diagnostic },
        { "test_arci03_unsat_reports_family_specific_diagnostic", test_arci03_unsat_reports_family_specific_diagnostic },
        { "test_line_line_parallel_unsat_reports_family_specific_diagnostic_lcon05",
          test_line_line_parallel_unsat_reports_family_specific_diagnostic_lcon05 },
        { "test_line_line_perpendicular_unsat_reports_family_specific_diagnostic_lcon05",
          test_line_line_perpendicular_unsat_reports_family_specific_diagnostic_lcon05 },
        { "test_line_line_group_perpendicular_unsat_reports_family_specific_diagnostic_lcon05",
          test_line_line_group_perpendicular_unsat_reports_family_specific_diagnostic_lcon05 },
        { "test_along_x_unsat_reports_family_specific_diagnostic_alin04",
          test_along_x_unsat_reports_family_specific_diagnostic_alin04 },
        { "test_along_y_unsat_reports_family_specific_diagnostic_alin04",
          test_along_y_unsat_reports_family_specific_diagnostic_alin04 },
        { "test_along_z_unsat_reports_family_specific_diagnostic_alin04",
          test_along_z_unsat_reports_family_specific_diagnostic_alin04 },
        { "test_tangency_post_failure_diagnostic_remains_family_specific_and_solver_recovers",
          test_tangency_post_failure_diagnostic_remains_family_specific_and_solver_recovers },
        { "test_diagnostic_parity_reapply", test_diagnostic_parity_reapply },
    };

    for (size_t i = 0; i < (sizeof(tests) / sizeof(tests[0])); ++i) {
        if (tests[i].fn() != 0) {
            fprintf(stderr, "FAILED: %s\n", tests[i].name);
            return 1;
        }
    }
    return 0;
}
