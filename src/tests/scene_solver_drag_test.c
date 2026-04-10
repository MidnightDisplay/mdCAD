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
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif
void sokol_main(void) {}
#ifdef __cplusplus
}
#endif

static bool vec3_close(vec3_t a, vec3_t b, float eps) {
    return fabsf(a.x - b.x) <= eps &&
           fabsf(a.y - b.y) <= eps &&
           fabsf(a.z - b.z) <= eps;
}

static bool test_setup_tangency_scene(ecs_world_state_t *world,
                                      ecs_scene_t *scene,
                                      ecs_entity_t *out_sketch,
                                      ecs_entity_t *out_line,
                                      ecs_entity_t *out_arc) {
    if (!world || !scene || !out_sketch || !out_line || !out_arc) return false;
    memset(world, 0, sizeof(*world));
    memset(scene, 0, sizeof(*scene));
    ecs_world_init(world);
    ecs_scene_init(scene, world);

    ecs_entity_t sketch = scene_add_sketch(scene, "Sketch", "", vec4_make(1, 1, 1, 1));
    ecs_entity_t line = scene_add_line_to_sketch(scene, sketch,
                                                 vec3_make(2.0f, -2.0f, 0.0f),
                                                 vec3_make(2.0f, 2.0f, 0.0f),
                                                 vec4_make(1, 1, 1, 1), 1.0f);
    ecs_entity_t arc = scene_add_arc_to_sketch(scene, sketch,
                                               vec3_make(0.0f, 0.0f, 0.0f), 2.0f,
                                               0.0f, 1.0f, vec3_make(0.0f, 0.0f, 1.0f),
                                               vec4_make(1, 1, 1, 1), 1.0f);
    if (!sketch || !line || !arc) return false;

    constraint_participant_descriptor_t desc[2] = {
        constraint_participant_descriptor_make((uint64_t)line, CONSTRAINT_PARTICIPANT_ROLE_POINT_A, 0),
        constraint_participant_descriptor_make((uint64_t)arc, CONSTRAINT_PARTICIPANT_ROLE_POINT_A, 0),
    };
    ecs_entity_t c = scene_add_constraint_with_paired_coincident(
        scene, sketch, CONSTRAINT_LINE_ARC_ENDPOINT_TANGENCY, desc, 2, 0.0f, false);
    if (!c) return false;
    if (!scene_solver_request_recalculate(scene, sketch)) return false;

    *out_sketch = sketch;
    *out_line = line;
    *out_arc = arc;
    return true;
}

static bool test_get_endpoint_entity(ecs_scene_t *scene,
                                     ecs_entity_t owner,
                                     constraint_participant_role_t role,
                                     ecs_entity_t *out_endpoint) {
    if (!scene || !out_endpoint) return false;
    EndPointsComp *owner_endpoints = ecs_world_get_endpoints(scene->world, owner);
    endpoint_binding_t binding = {0};
    if (!owner_endpoints || !endpoints_comp_find_binding(owner_endpoints, role, &binding)) {
        return false;
    }
    ecs_entity_t endpoint = (ecs_entity_t)binding.endpoint_entity;
    if (!endpoint || !ecs_is_alive(scene->world->world, endpoint)) return false;
    *out_endpoint = endpoint;
    return true;
}

static bool test_assert_tangency_still_satisfied(ecs_scene_t *scene, ecs_entity_t line, ecs_entity_t arc) {
    GeometryComp *line_geom = ecs_world_get_geometry(scene->world, line);
    GeometryComp *arc_geom = ecs_world_get_geometry(scene->world, arc);
    if (!line_geom || !arc_geom || line_geom->type != GEOM_LINE || arc_geom->type != GEOM_ARC) return false;

    vec3_t line_a = line_geom->data.line.a;
    vec3_t line_b = line_geom->data.line.b;
    vec3_t arc_a = vec3_make(0, 0, 0);
    if (!scene_entity_participant_subpoint(arc_geom, CONSTRAINT_PARTICIPANT_ROLE_POINT_A, &arc_a, NULL)) return false;

    vec3_t line_dir = vec3_normalize(vec3_sub(line_b, line_a));
    vec3_t radial = vec3_normalize(vec3_sub(line_a, arc_geom->data.arc.center));
    vec3_t tangent = vec3_normalize(vec3_cross(vec3_normalize(arc_geom->data.arc.normal), radial));
    float tangency_dot = fabsf(vec3_dot(line_dir, tangent));

    return vec3_close(line_a, arc_a, 1e-4f) && tangency_dot >= 0.999f;
}

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

static int test_drag_large_jump_constrained_tangency_uses_staged_projection(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_entity_t sketch = 0, line = 0, arc = 0;
    if (!test_setup_tangency_scene(&world, &scene, &sketch, &line, &arc)) return 1;

    scene_solver_drag_decision_t decision = {0};
    ecs_entity_t drag_entities[1] = { line };
    vec3_t requested = vec3_make(1.2f, 0.0f, 0.0f);
    bool ok_call = scene_solver_can_apply_drag(&scene, sketch, drag_entities, 1, requested, &decision);

    int ok = (ok_call &&
              decision.result == SCENE_SOLVER_DRAG_FEASIBLE &&
              decision.projected_delta.x > 0.25f &&
              decision.projected_delta.x <= requested.x &&
              fabsf(decision.projected_delta.y) <= 1e-6f &&
              fabsf(decision.projected_delta.z) <= 1e-6f);
    ecs_world_shutdown(&world);
    return ok ? 0 : 1;
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

static int test_drag_tangency_shared_endpoint_drag_feasible_preserves_shared_authority(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_entity_t sketch = 0, line = 0, arc = 0;
    if (!test_setup_tangency_scene(&world, &scene, &sketch, &line, &arc)) return 1;

    GeometryComp *line_geom = ecs_world_get_geometry(scene.world, line);
    if (!line_geom || line_geom->type != GEOM_LINE) return 1;
    vec3_t line_a_before = line_geom->data.line.a;

    ecs_entity_t line_a_endpoint = 0;
    if (!test_get_endpoint_entity(&scene, line, CONSTRAINT_PARTICIPANT_ROLE_POINT_A, &line_a_endpoint)) return 1;

    vec3_t drag_delta = vec3_make(0.35f, 0.25f, 0.0f);
    vec3_t expected = vec3_add(line_a_before, drag_delta);
    if (!scene_apply_endpoint_point_world_delta(&scene, line_a_endpoint, drag_delta)) return 1;

    bool solved = scene_solver_request_recalculate(&scene, sketch);
    line_geom = ecs_world_get_geometry(scene.world, line);
    vec3_t line_a_after = line_geom ? line_geom->data.line.a : vec3_make(0, 0, 0);

    int ok = (solved &&
              line_geom &&
              vec3_close(line_a_after, expected, 1e-4f) &&
              test_assert_tangency_still_satisfied(&scene, line, arc));
    ecs_world_shutdown(&world);
    return ok ? 0 : 1;
}

static int test_drag_tangency_adjacent_endpoint_drag_feasible_preserves_adjacent_authority(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_entity_t sketch = 0, line = 0, arc = 0;
    if (!test_setup_tangency_scene(&world, &scene, &sketch, &line, &arc)) return 1;

    GeometryComp *arc_geom = ecs_world_get_geometry(scene.world, arc);
    if (!arc_geom || arc_geom->type != GEOM_ARC) return 1;
    vec3_t center_before = arc_geom->data.arc.center;

    ecs_entity_t arc_center_endpoint = 0;
    if (!test_get_endpoint_entity(&scene, arc, CONSTRAINT_PARTICIPANT_ROLE_CENTER, &arc_center_endpoint)) return 1;

    vec3_t drag_delta = vec3_make(-0.25f, 0.15f, 0.0f);
    vec3_t expected = vec3_add(center_before, drag_delta);
    if (!scene_apply_endpoint_point_world_delta(&scene, arc_center_endpoint, drag_delta)) return 1;

    bool solved = scene_solver_request_recalculate(&scene, sketch);
    arc_geom = ecs_world_get_geometry(scene.world, arc);
    vec3_t center_after = arc_geom ? arc_geom->data.arc.center : vec3_make(0, 0, 0);

    int ok = (solved &&
              arc_geom &&
              vec3_close(center_after, expected, 1e-4f) &&
              test_assert_tangency_still_satisfied(&scene, line, arc));
    ecs_world_shutdown(&world);
    return ok ? 0 : 1;
}

static int test_drag_tangency_shared_endpoint_infeasible_rolls_back_transactionally(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_entity_t sketch = 0, line = 0, arc = 0;
    if (!test_setup_tangency_scene(&world, &scene, &sketch, &line, &arc)) return 1;

    SketchGeometryStateComp *line_state = ecs_world_get_sketch_geometry_state(scene.world, line);
    SketchGeometryStateComp *arc_state = ecs_world_get_sketch_geometry_state(scene.world, arc);
    if (!line_state || !arc_state) return 1;
    line_state->fixed = true;
    arc_state->fixed = true;

    GeometryComp *line_geom = ecs_world_get_geometry(scene.world, line);
    GeometryComp *arc_geom = ecs_world_get_geometry(scene.world, arc);
    if (!line_geom || !arc_geom) return 1;
    vec3_t line_a_before = line_geom->data.line.a;
    vec3_t line_b_before = line_geom->data.line.b;
    vec3_t center_before = arc_geom->data.arc.center;
    float start_before = arc_geom->data.arc.start_angle;
    float end_before = arc_geom->data.arc.end_angle;
    vec3_t normal_before = arc_geom->data.arc.normal;

    scene_solver_drag_decision_t decision = {0};
    ecs_entity_t drag_entities[1] = { line };
    bool ok_call = scene_solver_can_apply_drag(&scene, sketch, drag_entities, 1, vec3_make(0.3f, 0.2f, 0.0f), &decision);
    if (!ok_call) return 1;
    line_geom = ecs_world_get_geometry(scene.world, line);
    arc_geom = ecs_world_get_geometry(scene.world, arc);

    int ok = (decision.result == SCENE_SOLVER_DRAG_UNSATISFIABLE &&
              line_geom &&
              arc_geom &&
              vec3_close(line_geom->data.line.a, line_a_before, 1e-6f) &&
              vec3_close(line_geom->data.line.b, line_b_before, 1e-6f) &&
              vec3_close(arc_geom->data.arc.center, center_before, 1e-6f) &&
              fabsf(arc_geom->data.arc.start_angle - start_before) <= 1e-6f &&
              fabsf(arc_geom->data.arc.end_angle - end_before) <= 1e-6f &&
              vec3_close(arc_geom->data.arc.normal, normal_before, 1e-6f));
    ecs_world_shutdown(&world);
    return ok ? 0 : 1;
}

static int test_drag_tangency_post_failure_followup_feasible_is_responsive(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_entity_t sketch = 0, line = 0, arc = 0;
    if (!test_setup_tangency_scene(&world, &scene, &sketch, &line, &arc)) return 1;

    SketchGeometryStateComp *line_state = ecs_world_get_sketch_geometry_state(scene.world, line);
    SketchGeometryStateComp *arc_state = ecs_world_get_sketch_geometry_state(scene.world, arc);
    if (!line_state || !arc_state) return 1;

    ecs_entity_t line_a_endpoint = 0;
    if (!test_get_endpoint_entity(&scene, line, CONSTRAINT_PARTICIPANT_ROLE_POINT_A, &line_a_endpoint)) return 1;

    line_state->fixed = true;
    arc_state->fixed = true;
    if (!scene_apply_endpoint_point_world_delta(&scene, line_a_endpoint, vec3_make(0.3f, 0.2f, 0.0f))) return 1;
    bool failed = !scene_solver_request_recalculate(&scene, sketch);
    const scene_solver_failure_implication_t *imp = scene_solver_failure_implication(&scene);
    if (!failed || !imp || !imp->active) return 1;

    line_state->fixed = false;
    arc_state->fixed = false;
    GeometryComp *line_geom = ecs_world_get_geometry(scene.world, line);
    if (!line_geom) return 1;
    vec3_t line_a_before = line_geom->data.line.a;
    vec3_t drag_delta = vec3_make(0.12f, 0.08f, 0.0f);
    vec3_t expected = vec3_add(line_a_before, drag_delta);

    if (!scene_apply_endpoint_point_world_delta(&scene, line_a_endpoint, drag_delta)) return 1;
    bool solved = scene_solver_request_recalculate(&scene, sketch);
    line_geom = ecs_world_get_geometry(scene.world, line);

    int ok = (solved &&
              line_geom &&
              vec3_close(line_geom->data.line.a, expected, 1e-4f) &&
              test_assert_tangency_still_satisfied(&scene, line, arc));
    ecs_world_shutdown(&world);
    return ok ? 0 : 1;
}

static int test_active_sketch_line_rigid_delta_applies_projected_delta_to_both_endpoints(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(1, 1, 1, 1));
    ecs_entity_t line = scene_add_line_to_sketch(&scene, sketch,
                                                 vec3_make(0.0f, 0.0f, 0.0f),
                                                 vec3_make(1.0f, 0.0f, 0.0f),
                                                 vec4_make(1, 1, 1, 1), 1.0f);
    if (!sketch || !line) return 1;

    GeometryComp *line_geom = ecs_world_get_geometry(scene.world, line);
    if (!line_geom || line_geom->type != GEOM_LINE) return 1;
    vec3_t old_a = line_geom->data.line.a;
    vec3_t old_b = line_geom->data.line.b;

    scene_solver_drag_decision_t decision = {0};
    ecs_entity_t drag_entities[1] = { line };
    if (!scene_solver_can_apply_drag(&scene, sketch, drag_entities, 1, vec3_make(10.0f, 0.0f, 0.0f), &decision)) {
        ecs_world_shutdown(&world);
        return 1;
    }
    if (decision.result != SCENE_SOLVER_DRAG_FEASIBLE) {
        ecs_world_shutdown(&world);
        return 1;
    }
    if (!scene_apply_active_sketch_line_world_delta(&scene, line, decision.projected_delta)) {
        ecs_world_shutdown(&world);
        return 1;
    }

    line_geom = ecs_world_get_geometry(scene.world, line);
    vec3_t expected_a = vec3_add(old_a, decision.projected_delta);
    vec3_t expected_b = vec3_add(old_b, decision.projected_delta);
    int ok = line_geom &&
             vec3_close(line_geom->data.line.a, expected_a, 1e-4f) &&
             vec3_close(line_geom->data.line.b, expected_b, 1e-4f);
    ecs_world_shutdown(&world);
    return ok ? 0 : 1;
}

typedef int (*test_fn_t)(void);
typedef struct { const char *name; test_fn_t fn; } test_case_t;

int main(void) {
    static const test_case_t tests[] = {
        { "test_drag_fixed_geometry_is_unsat_with_zero_projection", test_drag_fixed_geometry_is_unsat_with_zero_projection },
        { "test_drag_feasible_large_delta_is_bounded_projection", test_drag_feasible_large_delta_is_bounded_projection },
        { "test_drag_large_jump_constrained_tangency_uses_staged_projection",
          test_drag_large_jump_constrained_tangency_uses_staged_projection },
        { "test_drag_unsat_preserves_last_valid_geometry_state", test_drag_unsat_preserves_last_valid_geometry_state },
        { "test_drag_rejected_diagnostic_payload_baseline", test_drag_rejected_diagnostic_payload_baseline },
        { "test_drag_tangency_shared_endpoint_drag_feasible_preserves_shared_authority",
          test_drag_tangency_shared_endpoint_drag_feasible_preserves_shared_authority },
        { "test_drag_tangency_adjacent_endpoint_drag_feasible_preserves_adjacent_authority",
          test_drag_tangency_adjacent_endpoint_drag_feasible_preserves_adjacent_authority },
        { "test_drag_tangency_shared_endpoint_infeasible_rolls_back_transactionally",
          test_drag_tangency_shared_endpoint_infeasible_rolls_back_transactionally },
        { "test_drag_tangency_post_failure_followup_feasible_is_responsive",
          test_drag_tangency_post_failure_followup_feasible_is_responsive },
        { "test_active_sketch_line_rigid_delta_applies_projected_delta_to_both_endpoints",
          test_active_sketch_line_rigid_delta_applies_projected_delta_to_both_endpoints },
    };

    for (size_t i = 0; i < (sizeof(tests) / sizeof(tests[0])); ++i) {
        if (tests[i].fn() != 0) {
            fprintf(stderr, "FAILED: %s\n", tests[i].name);
            return 1;
        }
    }
    return 0;
}
