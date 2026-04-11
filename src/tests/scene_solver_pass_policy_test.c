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

static bool vec3_exact_eq(vec3_t a, vec3_t b) {
    return a.x == b.x && a.y == b.y && a.z == b.z;
}

static bool vec3_close(vec3_t a, vec3_t b, float eps) {
    return fabsf(a.x - b.x) <= eps &&
           fabsf(a.y - b.y) <= eps &&
           fabsf(a.z - b.z) <= eps;
}

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

static int test_recalculate_along_tolerance_stop_contract_D11(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(1, 1, 1, 1));
    ecs_entity_t p1 = scene_add_point_to_sketch(&scene, sketch, vec3_make(1.0f, 0.0f, 0.0f), vec4_make(1, 1, 1, 1), 0.01f);
    ecs_entity_t p2 = scene_add_point_to_sketch(&scene, sketch, vec3_make(1.00005f, 0.0f, 0.0f), vec4_make(1, 1, 1, 1), 0.01f);
    if (!sketch || !p1 || !p2) return 1;

    ecs_entity_t participants[2] = { p1, p2 };
    ecs_entity_t c = scene_add_constraint_to_sketch(&scene, sketch, CONSTRAINT_ALONG_X, participants, 2, 0.0f, false);
    if (!c) return 1;
    if (!scene_solver_set_position_tolerance(&scene, sketch, 1e-4f)) return 1;
    if (!scene_solver_set_max_passes(&scene, sketch, 10u)) return 1;

    bool solved = scene_solver_request_recalculate(&scene, sketch);
    SketchComp *sk = ecs_world_get_sketch(scene.world, sketch);
    int ok = (solved && sk && sk->status == SKETCH_STATUS_SOLVED);
    ecs_world_shutdown(&world);
    return ok ? 0 : 1;
}

static int test_recalculate_along_max_pass_explicit_diagnostic_contract_D11(void) {
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
                                                   vec3_make(0.0f, 0.0f, 0.0f),
                                                   vec3_make(1.0f, 2.0f, 0.0f),
                                                   vec4_make(1, 1, 1, 1), 1.0f);
    if (!sketch || !line_a || !line_b) return 1;

    constraint_participant_descriptor_t along_desc[2] = {
        constraint_participant_descriptor_make((uint64_t)line_a, CONSTRAINT_PARTICIPANT_ROLE_POINT_B, 0),
        constraint_participant_descriptor_make((uint64_t)line_b, CONSTRAINT_PARTICIPANT_ROLE_POINT_B, 0),
    };
    ecs_entity_t c_along = scene_add_constraint_to_sketch_with_descriptors(&scene, sketch, CONSTRAINT_ALONG_X, along_desc, 2, 0.0f, false);
    ecs_entity_t angle_participants[2] = { line_a, line_b };
    ecs_entity_t c_angle = scene_add_constraint_to_sketch(&scene, sketch, CONSTRAINT_ANGLE, angle_participants, 2, 1.57079632679f, false);
    if (!c_along || !c_angle) return 1;

    if (!scene_solver_set_position_tolerance(&scene, sketch, 1e-9f)) return 1;
    if (!scene_solver_set_angle_tolerance(&scene, sketch, 1e-9f)) return 1;
    if (!scene_solver_set_max_passes(&scene, sketch, 1u)) return 1;

    bool solved = scene_solver_request_recalculate(&scene, sketch);
    int diag_count = scene_solver_diagnostic_count(&scene, sketch);
    const sketch_solver_diagnostic_t *last =
        scene_solver_diagnostic_at(&scene, sketch, diag_count > 0 ? diag_count - 1 : -1);

    int ok = (!solved &&
              diag_count > 0 &&
              last && strcmp(last->message, "max passes reached") == 0);
    ecs_world_shutdown(&world);
    return ok ? 0 : 1;
}

static int test_arci02_repeated_recalc_is_deterministic(void) {
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
    ecs_entity_t c = scene_add_constraint_with_paired_coincident(
        &scene, sketch, CONSTRAINT_LINE_ARC_ENDPOINT_TANGENCY, desc, 2, 0.0f, false);
    if (!c) return 1;

    bool solved_first = scene_solver_request_recalculate(&scene, sketch);
    GeometryComp *line_geom = ecs_world_get_geometry(scene.world, line);
    GeometryComp *arc_geom = ecs_world_get_geometry(scene.world, arc);
    if (!solved_first || !line_geom || !arc_geom) {
        ecs_world_shutdown(&world);
        return 1;
    }
    vec3_t line_a_first = line_geom->data.line.a;
    vec3_t line_b_first = line_geom->data.line.b;
    vec3_t arc_center_first = arc_geom->data.arc.center;
    float arc_start_first = arc_geom->data.arc.start_angle;
    float arc_end_first = arc_geom->data.arc.end_angle;
    vec3_t arc_normal_first = arc_geom->data.arc.normal;

    bool solved_second = scene_solver_request_recalculate(&scene, sketch);
    line_geom = ecs_world_get_geometry(scene.world, line);
    arc_geom = ecs_world_get_geometry(scene.world, arc);
    int ok = (solved_second &&
              line_geom && arc_geom &&
              line_a_first.x == line_geom->data.line.a.x &&
              line_a_first.y == line_geom->data.line.a.y &&
              line_a_first.z == line_geom->data.line.a.z &&
              line_b_first.x == line_geom->data.line.b.x &&
              line_b_first.y == line_geom->data.line.b.y &&
              line_b_first.z == line_geom->data.line.b.z &&
              arc_center_first.x == arc_geom->data.arc.center.x &&
              arc_center_first.y == arc_geom->data.arc.center.y &&
              arc_center_first.z == arc_geom->data.arc.center.z &&
              arc_start_first == arc_geom->data.arc.start_angle &&
              arc_end_first == arc_geom->data.arc.end_angle &&
              arc_normal_first.x == arc_geom->data.arc.normal.x &&
              arc_normal_first.y == arc_geom->data.arc.normal.y &&
              arc_normal_first.z == arc_geom->data.arc.normal.z);
    ecs_world_shutdown(&world);
    return ok ? 0 : 1;
}

static int test_arci02_repeated_recalc_is_deterministic_after_anchor_drag(void) {
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
    ecs_entity_t c = scene_add_constraint_with_paired_coincident(
        &scene, sketch, CONSTRAINT_LINE_ARC_ENDPOINT_TANGENCY, desc, 2, 0.0f, false);
    if (!c) return 1;
    if (!scene_solver_request_recalculate(&scene, sketch)) return 1;

    EndPointsComp *line_endpoints = ecs_world_get_endpoints(scene.world, line);
    endpoint_binding_t line_a_binding = {0};
    if (!line_endpoints ||
        !endpoints_comp_find_binding(line_endpoints, CONSTRAINT_PARTICIPANT_ROLE_POINT_A, &line_a_binding)) {
        ecs_world_shutdown(&world);
        return 1;
    }
    ecs_entity_t line_a_endpoint = (ecs_entity_t)line_a_binding.endpoint_entity;
    if (!scene_apply_endpoint_point_world_delta(&scene, line_a_endpoint, vec3_make(0.4f, 0.2f, 0.0f))) {
        ecs_world_shutdown(&world);
        return 1;
    }

    bool solved_first = scene_solver_request_recalculate(&scene, sketch);
    GeometryComp *line_geom = ecs_world_get_geometry(scene.world, line);
    GeometryComp *arc_geom = ecs_world_get_geometry(scene.world, arc);
    if (!solved_first || !line_geom || !arc_geom) {
        ecs_world_shutdown(&world);
        return 1;
    }
    vec3_t line_a_first = line_geom->data.line.a;
    vec3_t line_b_first = line_geom->data.line.b;
    vec3_t arc_center_first = arc_geom->data.arc.center;
    float arc_start_first = arc_geom->data.arc.start_angle;
    float arc_end_first = arc_geom->data.arc.end_angle;
    vec3_t arc_normal_first = arc_geom->data.arc.normal;

    bool solved_second = scene_solver_request_recalculate(&scene, sketch);
    line_geom = ecs_world_get_geometry(scene.world, line);
    arc_geom = ecs_world_get_geometry(scene.world, arc);
    int ok = (solved_second &&
              line_geom && arc_geom &&
              line_a_first.x == line_geom->data.line.a.x &&
              line_a_first.y == line_geom->data.line.a.y &&
              line_a_first.z == line_geom->data.line.a.z &&
              line_b_first.x == line_geom->data.line.b.x &&
              line_b_first.y == line_geom->data.line.b.y &&
              line_b_first.z == line_geom->data.line.b.z &&
              arc_center_first.x == arc_geom->data.arc.center.x &&
              arc_center_first.y == arc_geom->data.arc.center.y &&
              arc_center_first.z == arc_geom->data.arc.center.z &&
              arc_start_first == arc_geom->data.arc.start_angle &&
              arc_end_first == arc_geom->data.arc.end_angle &&
              arc_normal_first.x == arc_geom->data.arc.normal.x &&
              arc_normal_first.y == arc_geom->data.arc.normal.y &&
              arc_normal_first.z == arc_geom->data.arc.normal.z);
    ecs_world_shutdown(&world);
    return ok ? 0 : 1;
}

typedef struct {
    int outcome_class;
    vec3_t line_a;
    vec3_t line_b;
    vec3_t arc_center;
    vec3_t arc_point_a;
} mirrored_tangency_pass_policy_result_t;

static bool build_arci02_mirrored_pass_policy_case_D07_D08(bool mirrored,
                                                           bool reverse_participant_order,
                                                           bool adjacent_drag,
                                                           bool force_unsat,
                                                           mirrored_tangency_pass_policy_result_t *out_result) {
    if (!out_result) return false;

    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    float mirror = mirrored ? -1.0f : 1.0f;
    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(1, 1, 1, 1));
    ecs_entity_t line = scene_add_line_to_sketch(&scene, sketch,
                                                 vec3_make(mirror * 2.0f, -2.0f, 0.0f),
                                                 vec3_make(mirror * 2.0f, 2.0f, 0.0f),
                                                 vec4_make(1, 1, 1, 1), 1.0f);
    ecs_entity_t arc = scene_add_arc_to_sketch(&scene, sketch,
                                               vec3_make(0.0f, 0.0f, 0.0f), 2.0f,
                                               mirrored ? 3.14159265359f : 0.0f,
                                               mirrored ? 2.14159265359f : 1.0f,
                                               vec3_make(0.0f, 0.0f, 1.0f),
                                               vec4_make(1, 1, 1, 1), 1.0f);
    if (!sketch || !line || !arc) {
        ecs_world_shutdown(&world);
        return false;
    }

    constraint_participant_descriptor_t desc[2] = {
        constraint_participant_descriptor_make((uint64_t)line, CONSTRAINT_PARTICIPANT_ROLE_POINT_A, 0),
        constraint_participant_descriptor_make((uint64_t)arc, CONSTRAINT_PARTICIPANT_ROLE_POINT_A, 0),
    };
    if (reverse_participant_order) {
        constraint_participant_descriptor_t tmp = desc[0];
        desc[0] = desc[1];
        desc[1] = tmp;
    }
    ecs_entity_t c = scene_add_constraint_with_paired_coincident(
        &scene, sketch, CONSTRAINT_LINE_ARC_ENDPOINT_TANGENCY, desc, 2, 0.0f, false);
    if (!c || !scene_solver_request_recalculate(&scene, sketch)) {
        ecs_world_shutdown(&world);
        return false;
    }

    if (force_unsat) {
        SketchGeometryStateComp *line_state = ecs_world_get_sketch_geometry_state(scene.world, line);
        SketchGeometryStateComp *arc_state = ecs_world_get_sketch_geometry_state(scene.world, arc);
        if (!line_state || !arc_state) {
            ecs_world_shutdown(&world);
            return false;
        }
        line_state->fixed = true;
        arc_state->fixed = true;
    }

    bool solved_first = false;
    if (force_unsat) {
        scene_solver_drag_decision_t decision = {0};
        ecs_entity_t drag_entities[1] = { adjacent_drag ? arc : line };
        vec3_t requested_delta = adjacent_drag
            ? vec3_make(mirror * -0.30f, 0.20f, 0.0f)
            : vec3_make(mirror * 0.40f, 0.20f, 0.0f);
        bool ok_call = scene_solver_can_apply_drag(&scene, sketch, drag_entities, 1, requested_delta, &decision);
        solved_first = (ok_call && decision.result == SCENE_SOLVER_DRAG_FEASIBLE);
    } else if (adjacent_drag) {
        EndPointsComp *arc_endpoints = ecs_world_get_endpoints(scene.world, arc);
        endpoint_binding_t center_binding = {0};
        if (!arc_endpoints ||
            !endpoints_comp_find_binding(arc_endpoints, CONSTRAINT_PARTICIPANT_ROLE_CENTER, &center_binding)) {
                ecs_world_shutdown(&world);
            return false;
        }
        ecs_entity_t center_endpoint = (ecs_entity_t)center_binding.endpoint_entity;
        vec3_t drag_delta = vec3_make(mirror * -0.30f, 0.20f, 0.0f);
        if (!scene_apply_endpoint_point_world_delta(&scene, center_endpoint, drag_delta)) {
            ecs_world_shutdown(&world);
            return false;
        }
        solved_first = scene_solver_request_recalculate(&scene, sketch);
    } else {
        EndPointsComp *line_endpoints = ecs_world_get_endpoints(scene.world, line);
        endpoint_binding_t line_a_binding = {0};
        if (!line_endpoints ||
            !endpoints_comp_find_binding(line_endpoints, CONSTRAINT_PARTICIPANT_ROLE_POINT_A, &line_a_binding)) {
            ecs_world_shutdown(&world);
            return false;
        }
        ecs_entity_t line_a_endpoint = (ecs_entity_t)line_a_binding.endpoint_entity;
        vec3_t drag_delta = vec3_make(mirror * 0.40f, 0.20f, 0.0f);
        if (!scene_apply_endpoint_point_world_delta(&scene, line_a_endpoint, drag_delta)) {
            ecs_world_shutdown(&world);
            return false;
        }
        solved_first = scene_solver_request_recalculate(&scene, sketch);
    }

    bool solved_second = scene_solver_request_recalculate(&scene, sketch);
    GeometryComp *line_geom = ecs_world_get_geometry(scene.world, line);
    GeometryComp *arc_geom = ecs_world_get_geometry(scene.world, arc);
    if (!line_geom || !arc_geom || line_geom->type != GEOM_LINE || arc_geom->type != GEOM_ARC) {
        ecs_world_shutdown(&world);
        return false;
    }
    vec3_t arc_point_a = vec3_make(0, 0, 0);
    if (!scene_entity_participant_subpoint(arc_geom, CONSTRAINT_PARTICIPANT_ROLE_POINT_A, &arc_point_a, NULL)) {
        ecs_world_shutdown(&world);
        return false;
    }

    out_result->outcome_class = (solved_first && solved_second) ? 1 : 0;
    out_result->line_a = line_geom->data.line.a;
    out_result->line_b = line_geom->data.line.b;
    out_result->arc_center = arc_geom->data.arc.center;
    out_result->arc_point_a = arc_point_a;
    ecs_world_shutdown(&world);
    return true;
}

static int test_arci02_mirrored_parity_rerun_and_ordering_shared_drag_D07_D08(void) {
    // D-07/D-08: mirrored shared-drag parity must survive immediate rerun and participant-order variants.
    mirrored_tangency_pass_policy_result_t left = {0};
    mirrored_tangency_pass_policy_result_t left_reordered = {0};
    mirrored_tangency_pass_policy_result_t right = {0};
    mirrored_tangency_pass_policy_result_t right_reordered = {0};
    if (!build_arci02_mirrored_pass_policy_case_D07_D08(false, false, false, false, &left)) return 1;
    if (!build_arci02_mirrored_pass_policy_case_D07_D08(false, true, false, false, &left_reordered)) return 1;
    if (!build_arci02_mirrored_pass_policy_case_D07_D08(true, false, false, false, &right)) return 1;
    if (!build_arci02_mirrored_pass_policy_case_D07_D08(true, true, false, false, &right_reordered)) return 1;

    return (left.outcome_class == 1 &&
            right.outcome_class == 1 &&
            left_reordered.outcome_class == left.outcome_class &&
            right_reordered.outcome_class == right.outcome_class &&
            vec3_close(left.line_a, left_reordered.line_a, 1e-5f) &&
            vec3_close(left.line_b, left_reordered.line_b, 1e-5f) &&
            vec3_close(left.arc_center, left_reordered.arc_center, 1e-5f) &&
            vec3_close(right.line_a, right_reordered.line_a, 1e-5f) &&
            vec3_close(right.line_b, right_reordered.line_b, 1e-5f) &&
            vec3_close(right.arc_center, right_reordered.arc_center, 1e-5f) &&
            vec3_close(left.line_a, left.arc_point_a, 1e-5f) &&
            vec3_close(right.line_a, right.arc_point_a, 1e-5f) &&
            fabsf(left.line_a.x + right.line_a.x) <= 1e-5f &&
            fabsf(left.line_a.y - right.line_a.y) <= 1e-5f &&
            fabsf(left.line_b.x + right.line_b.x) <= 1e-5f &&
            fabsf(left.line_b.y - right.line_b.y) <= 1e-5f &&
            fabsf(left.arc_center.x + right.arc_center.x) <= 1e-5f &&
            fabsf(left.arc_center.y - right.arc_center.y) <= 1e-5f) ? 0 : 1;
}

static int test_arci02_mirrored_parity_rerun_and_ordering_adjacent_unsat_D07_D08(void) {
    // D-07/D-08: mirrored adjacent unsat parity must survive immediate rerun and participant-order variants.
    mirrored_tangency_pass_policy_result_t left = {0};
    mirrored_tangency_pass_policy_result_t left_reordered = {0};
    mirrored_tangency_pass_policy_result_t right = {0};
    mirrored_tangency_pass_policy_result_t right_reordered = {0};
    if (!build_arci02_mirrored_pass_policy_case_D07_D08(false, false, true, true, &left)) return 1;
    if (!build_arci02_mirrored_pass_policy_case_D07_D08(false, true, true, true, &left_reordered)) return 1;
    if (!build_arci02_mirrored_pass_policy_case_D07_D08(true, false, true, true, &right)) return 1;
    if (!build_arci02_mirrored_pass_policy_case_D07_D08(true, true, true, true, &right_reordered)) return 1;

    return (left.outcome_class == 0 &&
            right.outcome_class == 0 &&
            left_reordered.outcome_class == left.outcome_class &&
            right_reordered.outcome_class == right.outcome_class &&
            vec3_close(left.line_a, left_reordered.line_a, 1e-5f) &&
            vec3_close(left.line_b, left_reordered.line_b, 1e-5f) &&
            vec3_close(left.arc_center, left_reordered.arc_center, 1e-5f) &&
            vec3_close(right.line_a, right_reordered.line_a, 1e-5f) &&
            vec3_close(right.line_b, right_reordered.line_b, 1e-5f) &&
            vec3_close(right.arc_center, right_reordered.arc_center, 1e-5f) &&
            fabsf(left.arc_center.x + right.arc_center.x) <= 1e-5f &&
            fabsf(left.arc_center.y - right.arc_center.y) <= 1e-5f) ? 0 : 1;
}

static int test_alin04_pass_policy_mixed_along_x_length_angle_connectivity_rerun_deterministic(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(1, 1, 1, 1));
    ecs_entity_t line_a = scene_add_line_to_sketch(&scene, sketch,
                                                   vec3_make(1.0f, 2.0f, 1.0f),
                                                   vec3_make(6.0f, 2.0f, 1.0f),
                                                   vec4_make(1, 1, 1, 1), 1.0f);
    ecs_entity_t line_b = scene_add_line_to_sketch(&scene, sketch,
                                                   vec3_make(1.0f, 2.0f, 1.0f),
                                                   vec3_make(2.0f, 5.0f, 3.0f),
                                                   vec4_make(1, 1, 1, 1), 1.0f);
    if (!sketch || !line_a || !line_b) return 1;

    ecs_entity_t along_participants[1] = { line_a };
    ecs_entity_t c_along = scene_add_constraint_to_sketch(&scene, sketch, CONSTRAINT_ALONG_X,
                                                           along_participants, 1, 0.0f, false);
    ecs_entity_t length_participants[1] = { line_a };
    ecs_entity_t c_length = scene_add_constraint_to_sketch(&scene, sketch, CONSTRAINT_LENGTH,
                                                            length_participants, 1, 5.0f, false);
    ecs_entity_t angle_participants[2] = { line_a, line_b };
    ecs_entity_t c_angle = scene_add_constraint_to_sketch(&scene, sketch, CONSTRAINT_ANGLE,
                                                           angle_participants, 2, 1.57079632679f, false);
    constraint_participant_descriptor_t coincident_desc[2] = {
        constraint_participant_descriptor_make((uint64_t)line_a, CONSTRAINT_PARTICIPANT_ROLE_POINT_A, 0),
        constraint_participant_descriptor_make((uint64_t)line_b, CONSTRAINT_PARTICIPANT_ROLE_POINT_A, 0),
    };
    ecs_entity_t c_coincident = scene_add_constraint_to_sketch_with_descriptors(
        &scene, sketch, CONSTRAINT_COINCIDENT, coincident_desc, 2, 0.0f, false);
    if (!c_along || !c_length || !c_angle || !c_coincident) return 1;

    bool solved_first = scene_solver_request_recalculate(&scene, sketch);
    GeometryComp *ga = ecs_world_get_geometry(scene.world, line_a);
    GeometryComp *gb = ecs_world_get_geometry(scene.world, line_b);
    if (!solved_first || !ga || !gb || ga->type != GEOM_LINE || gb->type != GEOM_LINE) return 1;
    vec3_t a0_after_first = ga->data.line.a;
    vec3_t a1_after_first = ga->data.line.b;
    vec3_t b0_after_first = gb->data.line.a;
    vec3_t b1_after_first = gb->data.line.b;

    bool solved_second = scene_solver_request_recalculate(&scene, sketch);
    ga = ecs_world_get_geometry(scene.world, line_a);
    gb = ecs_world_get_geometry(scene.world, line_b);
    int ok = (solved_second &&
              ga && gb &&
              a0_after_first.x == ga->data.line.a.x &&
              a0_after_first.y == ga->data.line.a.y &&
              a0_after_first.z == ga->data.line.a.z &&
              a1_after_first.x == ga->data.line.b.x &&
              a1_after_first.y == ga->data.line.b.y &&
              a1_after_first.z == ga->data.line.b.z &&
              b0_after_first.x == gb->data.line.a.x &&
              b0_after_first.y == gb->data.line.a.y &&
              b0_after_first.z == gb->data.line.a.z &&
              b1_after_first.x == gb->data.line.b.x &&
              b1_after_first.y == gb->data.line.b.y &&
              b1_after_first.z == gb->data.line.b.z);
    ecs_world_shutdown(&world);
    return ok ? 0 : 1;
}

typedef struct {
    int outcome_class;
    vec3_t line_b_a;
    vec3_t line_b_b;
} parallel_along_equivalent_result_t;

static bool build_parallel_along_equivalent_pass_policy_case(bool use_parallel,
                                                             bool mirrored,
                                                             bool reordered,
                                                             parallel_along_equivalent_result_t *out_result) {
    if (!out_result) return false;
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    float mirror = mirrored ? -1.0f : 1.0f;
    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(1, 1, 1, 1));
    ecs_entity_t line_a = scene_add_line_to_sketch(&scene, sketch,
                                                   vec3_make(mirror * 0.0f, 0.0f, 0.0f),
                                                   vec3_make(mirror * 3.0f, 0.0f, 0.0f),
                                                   vec4_make(1, 1, 1, 1), 1.0f);
    ecs_entity_t line_b = scene_add_line_to_sketch(&scene, sketch,
                                                   vec3_make(mirror * 0.0f, 1.0f, 0.0f),
                                                   vec3_make(mirror * 2.0f, 2.0f, 0.0f),
                                                   vec4_make(1, 1, 1, 1), 1.0f);
    if (!sketch || !line_a || !line_b) {
        ecs_world_shutdown(&world);
        return false;
    }

    SketchGeometryStateComp *state_a = ecs_world_get_sketch_geometry_state(scene.world, line_a);
    if (!state_a) {
        ecs_world_shutdown(&world);
        return false;
    }
    state_a->fixed = true;

    if (use_parallel) {
        ecs_entity_t participants[2] = { line_a, line_b };
        if (reordered) {
            participants[0] = line_b;
            participants[1] = line_a;
        }
        if (!scene_add_constraint_to_sketch(&scene, sketch, CONSTRAINT_PARALLEL, participants, 2, 0.0f, false)) {
            ecs_world_shutdown(&world);
            return false;
        }
    } else {
        constraint_participant_descriptor_t along_desc[2] = {
            constraint_participant_descriptor_make((uint64_t)line_b, CONSTRAINT_PARTICIPANT_ROLE_POINT_A, 0),
            constraint_participant_descriptor_make((uint64_t)line_b, CONSTRAINT_PARTICIPANT_ROLE_POINT_B, 1),
        };
        if (reordered) {
            constraint_participant_descriptor_t tmp = along_desc[0];
            along_desc[0] = along_desc[1];
            along_desc[1] = tmp;
        }
        if (!scene_add_constraint_to_sketch_with_descriptors(&scene, sketch, CONSTRAINT_ALONG_X, along_desc, 2, 0.0f, false)) {
            ecs_world_shutdown(&world);
            return false;
        }
    }

    bool solved_first = scene_solver_request_recalculate(&scene, sketch);
    bool solved_second = scene_solver_request_recalculate(&scene, sketch);
    GeometryComp *line_b_geom = ecs_world_get_geometry(scene.world, line_b);
    if (!line_b_geom || line_b_geom->type != GEOM_LINE) {
        ecs_world_shutdown(&world);
        return false;
    }
    out_result->outcome_class = (solved_first && solved_second) ? 1 : 0;
    out_result->line_b_a = line_b_geom->data.line.a;
    out_result->line_b_b = line_b_geom->data.line.b;
    ecs_world_shutdown(&world);
    return true;
}

static int test_parallel_along_equivalent_mirrored_order_parity_pass_policy(void) {
    parallel_along_equivalent_result_t parallel_base = {0};
    parallel_along_equivalent_result_t along_base = {0};
    parallel_along_equivalent_result_t parallel_mirrored = {0};
    parallel_along_equivalent_result_t along_mirrored = {0};
    parallel_along_equivalent_result_t parallel_reordered = {0};
    parallel_along_equivalent_result_t along_reordered = {0};

    if (!build_parallel_along_equivalent_pass_policy_case(true, false, false, &parallel_base)) return 1;
    if (!build_parallel_along_equivalent_pass_policy_case(false, false, false, &along_base)) return 1;
    if (!build_parallel_along_equivalent_pass_policy_case(true, true, false, &parallel_mirrored)) return 1;
    if (!build_parallel_along_equivalent_pass_policy_case(false, true, false, &along_mirrored)) return 1;
    if (!build_parallel_along_equivalent_pass_policy_case(true, false, true, &parallel_reordered)) return 1;
    if (!build_parallel_along_equivalent_pass_policy_case(false, false, true, &along_reordered)) return 1;

    return (parallel_base.outcome_class == along_base.outcome_class &&
            parallel_mirrored.outcome_class == along_mirrored.outcome_class &&
            parallel_reordered.outcome_class == along_reordered.outcome_class &&
            parallel_base.outcome_class == parallel_reordered.outcome_class &&
            along_base.outcome_class == along_reordered.outcome_class) ? 0 : 1;
}

static int test_parallel_along_mirrored_reordered_authority_switch_is_deterministic(void) {
    const bool use_parallel[6] = { true, false, true, false, true, false };
    const bool mirrored[6] =    { false, false, true, true, false, false };
    const bool reordered[6] =   { false, false, false, false, true, true };
    int outcomes[6] = {0};

    for (int i = 0; i < 6; i++) {
        ecs_world_state_t world = {0};
        ecs_scene_t scene = {0};
        ecs_world_init(&world);
        ecs_scene_init(&scene, &world);

        float mirror = mirrored[i] ? -1.0f : 1.0f;
        ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(1, 1, 1, 1));
        ecs_entity_t line_a = scene_add_line_to_sketch(&scene, sketch,
                                                       vec3_make(mirror * 0.0f, 0.0f, 0.0f),
                                                       vec3_make(mirror * 3.0f, 0.0f, 0.0f),
                                                       vec4_make(1, 1, 1, 1), 1.0f);
        ecs_entity_t line_b = scene_add_line_to_sketch(&scene, sketch,
                                                       vec3_make(mirror * 0.0f, 1.0f, 0.0f),
                                                       vec3_make(mirror * 2.0f, 2.0f, 0.0f),
                                                       vec4_make(1, 1, 1, 1), 1.0f);
        if (!sketch || !line_a || !line_b) {
            ecs_world_shutdown(&world);
            return 1;
        }

        bool added = false;
        if (use_parallel[i]) {
            ecs_entity_t pair[2] = { line_a, line_b };
            if (reordered[i]) {
                pair[0] = line_b;
                pair[1] = line_a;
            }
            added = scene_add_constraint_to_sketch(&scene, sketch, CONSTRAINT_PARALLEL, pair, 2, 0.0f, false) != 0;
        } else {
            constraint_participant_descriptor_t along_desc[2] = {
                constraint_participant_descriptor_make((uint64_t)line_b, CONSTRAINT_PARTICIPANT_ROLE_POINT_A, 0),
                constraint_participant_descriptor_make((uint64_t)line_b, CONSTRAINT_PARTICIPANT_ROLE_POINT_B, 1),
            };
            if (reordered[i]) {
                constraint_participant_descriptor_t tmp = along_desc[0];
                along_desc[0] = along_desc[1];
                along_desc[1] = tmp;
            }
            added = scene_add_constraint_to_sketch_with_descriptors(
                &scene, sketch, CONSTRAINT_ALONG_X, along_desc, 2, 0.0f, false) != 0;
        }
        if (!added || !scene_solver_request_recalculate(&scene, sketch)) {
            ecs_world_shutdown(&world);
            return 1;
        }

        if (!scene_solver_set_drag_anchor(&scene, sketch, line_b, CONSTRAINT_PARTICIPANT_ROLE_POINT_A, 0) ||
            !scene_apply_active_sketch_line_world_delta(&scene, line_b, vec3_make(0.0f, 0.3f, 0.0f))) {
            ecs_world_shutdown(&world);
            return 1;
        }
        bool solved_first = scene_solver_request_recalculate(&scene, sketch);
        if (!scene_solver_set_drag_anchor(&scene, sketch, line_a, CONSTRAINT_PARTICIPANT_ROLE_POINT_A, 0) ||
            !scene_apply_active_sketch_line_world_delta(&scene, line_a, vec3_make(0.0f, -0.25f, 0.0f))) {
            ecs_world_shutdown(&world);
            return 1;
        }
        bool solved_second = scene_solver_request_recalculate(&scene, sketch);
        outcomes[i] = (solved_first && solved_second) ? 1 : 0;
        ecs_world_shutdown(&world);
    }

    return (outcomes[0] == outcomes[1] &&
            outcomes[2] == outcomes[3] &&
            outcomes[4] == outcomes[5] &&
            outcomes[0] == outcomes[2] &&
            outcomes[0] == outcomes[4]) ? 0 : 1;
}

typedef int (*test_fn_t)(void);
typedef struct { const char *name; test_fn_t fn; } test_case_t;

int main(void) {
    static const test_case_t tests[] = {
        { "test_recalculate_stops_by_tolerance", test_recalculate_stops_by_tolerance },
        { "test_recalculate_max_pass_cap_and_explicit_diagnostic", test_recalculate_max_pass_cap_and_explicit_diagnostic },
        { "test_recalculate_along_tolerance_stop_contract_D11", test_recalculate_along_tolerance_stop_contract_D11 },
        { "test_recalculate_along_max_pass_explicit_diagnostic_contract_D11",
          test_recalculate_along_max_pass_explicit_diagnostic_contract_D11 },
        { "test_arci02_repeated_recalc_is_deterministic",
          test_arci02_repeated_recalc_is_deterministic },
        { "test_arci02_repeated_recalc_is_deterministic_after_anchor_drag",
          test_arci02_repeated_recalc_is_deterministic_after_anchor_drag },
        { "test_arci02_mirrored_parity_rerun_and_ordering_shared_drag_D07_D08",
          test_arci02_mirrored_parity_rerun_and_ordering_shared_drag_D07_D08 },
        { "test_arci02_mirrored_parity_rerun_and_ordering_adjacent_unsat_D07_D08",
          test_arci02_mirrored_parity_rerun_and_ordering_adjacent_unsat_D07_D08 },
        { "test_alin04_pass_policy_mixed_along_x_length_angle_connectivity_rerun_deterministic",
          test_alin04_pass_policy_mixed_along_x_length_angle_connectivity_rerun_deterministic },
        { "test_parallel_along_equivalent_mirrored_order_parity_pass_policy",
          test_parallel_along_equivalent_mirrored_order_parity_pass_policy },
        { "test_parallel_along_mirrored_reordered_authority_switch_is_deterministic",
          test_parallel_along_mirrored_reordered_authority_switch_is_deterministic },
    };

    for (size_t i = 0; i < (sizeof(tests) / sizeof(tests[0])); ++i) {
        if (tests[i].fn() != 0) {
            fprintf(stderr, "FAILED: %s\n", tests[i].name);
            return 1;
        }
    }
    return 0;
}
