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
    ecs_entity_t c = scene_add_constraint_to_sketch_with_descriptors(
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
    ecs_entity_t c = scene_add_constraint_to_sketch_with_descriptors(
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
    };

    for (size_t i = 0; i < (sizeof(tests) / sizeof(tests[0])); ++i) {
        if (tests[i].fn() != 0) {
            fprintf(stderr, "FAILED: %s\n", tests[i].name);
            return 1;
        }
    }
    return 0;
}
