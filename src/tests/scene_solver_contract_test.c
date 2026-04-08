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

static int test_sketch_solver_defaults_and_manual_recalc_clears_queue_metadata(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(1, 1, 1, 1));
    if (!sketch) return 1;

    SketchComp *sk = ecs_world_get_sketch(scene.world, sketch);
    if (!sk) return 1;

    int ok = (sk->auto_solve_debounce_ms == 0u &&
              sk->solver_max_passes == 400u &&
              fabsf(sk->solver_position_tolerance - 1e-4f) <= 1e-7f &&
              fabsf(sk->solver_angle_tolerance - 1e-4f) <= 1e-7f);
    if (!ok) {
        ecs_world_shutdown(&world);
        return 1;
    }

    sk->auto_solve_pending = true;
    sk->auto_solve_queued_at_ms = 1234u;
    sk->auto_solve_queue_token = 77u;

    if (!scene_solver_request_recalculate(&scene, sketch)) {
        ecs_world_shutdown(&world);
        return 1;
    }

    sk = ecs_world_get_sketch(scene.world, sketch);
    ok = (sk &&
          !sk->auto_solve_pending &&
          sk->auto_solve_queued_at_ms == 0u &&
          sk->auto_solve_queue_token == 0u);
    ecs_world_shutdown(&world);
    return ok ? 0 : 1;
}

static int test_debounce_setter_clamps_range_and_preserves_zero(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(1, 1, 1, 1));
    if (!sketch) return 1;

    SketchComp *sk = ecs_world_get_sketch(scene.world, sketch);
    if (!sk) {
        ecs_world_shutdown(&world);
        return 1;
    }
    if (sk->auto_solve_debounce_ms != 0u) {
        ecs_world_shutdown(&world);
        return 1;
    }

    if (!scene_solver_set_auto_solve_debounce_ms(&scene, sketch, 0u)) {
        ecs_world_shutdown(&world);
        return 1;
    }
    if (scene_solver_auto_solve_debounce_ms(&scene, sketch) != 0u) {
        ecs_world_shutdown(&world);
        return 1;
    }

    if (!scene_solver_set_auto_solve_debounce_ms(&scene, sketch, 101u)) {
        ecs_world_shutdown(&world);
        return 1;
    }
    if (scene_solver_auto_solve_debounce_ms(&scene, sketch) != 100u) {
        ecs_world_shutdown(&world);
        return 1;
    }

    ecs_world_shutdown(&world);
    return 0;
}

static bool vec3_close(vec3_t a, vec3_t b, float eps) {
    return fabsf(a.x - b.x) <= eps &&
           fabsf(a.y - b.y) <= eps &&
           fabsf(a.z - b.z) <= eps;
}

static int test_recalculate_commits_coincident_solution_for_points(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(1, 1, 1, 1));
    ecs_entity_t p1 = scene_add_point_to_sketch(&scene, sketch, vec3_make(0, 0, 0), vec4_make(1, 1, 1, 1), 0.01f);
    ecs_entity_t p2 = scene_add_point_to_sketch(&scene, sketch, vec3_make(2, 0, 0), vec4_make(1, 1, 1, 1), 0.01f);
    if (!sketch || !p1 || !p2) return 1;

    ecs_entity_t participants[2] = { p1, p2 };
    ecs_entity_t c = scene_add_constraint_to_sketch(&scene, sketch, CONSTRAINT_COINCIDENT, participants, 2, 0.0f, false);
    if (!c) return 1;

    GeometryComp *g1 = ecs_world_get_geometry(scene.world, p1);
    GeometryComp *g2 = ecs_world_get_geometry(scene.world, p2);
    vec3_t p1_before = g1->data.point.point;
    vec3_t p2_before = g2->data.point.point;

    bool solved = scene_solver_request_recalculate(&scene, sketch);

    g1 = ecs_world_get_geometry(scene.world, p1);
    g2 = ecs_world_get_geometry(scene.world, p2);
    vec3_t p1_after = g1->data.point.point;
    vec3_t p2_after = g2->data.point.point;
    ecs_world_shutdown(&world);

    if (!solved) return 1;
    if (!vec3_close(p1_after, p2_after, 1e-4f)) return 1;
    if (vec3_close(p1_before, p1_after, 1e-4f) && vec3_close(p2_before, p2_after, 1e-4f)) return 1;
    return 0;
}

static int test_recalculate_unsat_is_transactional_and_deterministic(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(1, 1, 1, 1));
    ecs_entity_t p1 = scene_add_point_to_sketch(&scene, sketch, vec3_make(0, 0, 0), vec4_make(1, 1, 1, 1), 0.01f);
    ecs_entity_t p2 = scene_add_point_to_sketch(&scene, sketch, vec3_make(5, 0, 0), vec4_make(1, 1, 1, 1), 0.01f);
    if (!sketch || !p1 || !p2) return 1;

    SketchGeometryStateComp *s1 = ecs_world_get_sketch_geometry_state(scene.world, p1);
    SketchGeometryStateComp *s2 = ecs_world_get_sketch_geometry_state(scene.world, p2);
    if (!s1 || !s2) return 1;
    s1->fixed = true;
    s2->fixed = true;

    ecs_entity_t participants[2] = { p1, p2 };
    ecs_entity_t c = scene_add_constraint_to_sketch(&scene, sketch, CONSTRAINT_COINCIDENT, participants, 2, 0.0f, false);
    if (!c) return 1;

    GeometryComp *g1 = ecs_world_get_geometry(scene.world, p1);
    GeometryComp *g2 = ecs_world_get_geometry(scene.world, p2);
    vec3_t p1_before = g1->data.point.point;
    vec3_t p2_before = g2->data.point.point;

    bool solved = scene_solver_request_recalculate(&scene, sketch);
    const scene_solver_failure_implication_t *imp = scene_solver_failure_implication(&scene);

    g1 = ecs_world_get_geometry(scene.world, p1);
    g2 = ecs_world_get_geometry(scene.world, p2);
    vec3_t p1_after = g1->data.point.point;
    vec3_t p2_after = g2->data.point.point;

    int ok = (!solved &&
              vec3_close(p1_before, p1_after, 1e-6f) &&
              vec3_close(p2_before, p2_after, 1e-6f) &&
              imp && imp->active &&
              imp->first_constraint == c &&
              strcmp(imp->reason, "Unsatisfied coincident constraint.") == 0);
    ecs_world_shutdown(&world);
    return ok ? 0 : 1;
}

static int test_participant_descriptor_supports_sub_entity_contract(void) {
    constraint_participant_descriptor_t descriptor =
        constraint_participant_descriptor_make((ecs_entity_t)44, CONSTRAINT_PARTICIPANT_ROLE_POINT_B, 1);
    if (descriptor.entity != (uint64_t)44) return 1;
    if (descriptor.role != CONSTRAINT_PARTICIPANT_ROLE_POINT_B) return 1;
    if (descriptor.sub_index != 1) return 1;
    return 0;
}

static int test_scene_constraint_creation_accepts_directional_descriptor_participants_D01_D06(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(1, 1, 1, 1));
    ecs_entity_t p = scene_add_point_to_sketch(&scene, sketch, vec3_make(0.0f, 0.0f, 0.0f), vec4_make(1, 1, 1, 1), 0.01f);
    ecs_entity_t line = scene_add_line_to_sketch(&scene, sketch,
                                                 vec3_make(0.0f, 0.0f, 0.0f),
                                                 vec3_make(2.0f, 0.0f, 0.0f),
                                                 vec4_make(1, 1, 1, 1), 1.0f);
    ecs_entity_t arc = scene_add_arc_to_sketch(&scene, sketch,
                                               vec3_make(1.0f, 1.0f, 0.0f), 1.0f,
                                               0.0f, 1.5707963f, vec3_make(0.0f, 0.0f, 1.0f),
                                               vec4_make(1, 1, 1, 1), 1.0f);
    if (!sketch || !p || !line || !arc) {
        ecs_world_shutdown(&world);
        return 1;
    }

    constraint_participant_descriptor_t mixed[3] = {
        constraint_participant_descriptor_make((uint64_t)p, CONSTRAINT_PARTICIPANT_ROLE_ENTITY, 0),
        constraint_participant_descriptor_make((uint64_t)line, CONSTRAINT_PARTICIPANT_ROLE_POINT_A, 0),
        constraint_participant_descriptor_make((uint64_t)arc, CONSTRAINT_PARTICIPANT_ROLE_CENTER, 0),
    };

    ecs_entity_t along = scene_add_constraint_to_sketch_with_descriptors(&scene, sketch, CONSTRAINT_ALONG_X, mixed, 3, 0.0f, false);
    if (!along) {
        ecs_world_shutdown(&world);
        return 1;
    }

    ConstraintComp *constraint = ecs_world_get_constraint(scene.world, along);
    int ok = (constraint &&
              constraint->participant_count == 3 &&
              constraint->participant_descriptors[0].role == CONSTRAINT_PARTICIPANT_ROLE_ENTITY &&
              constraint->participant_descriptors[1].role == CONSTRAINT_PARTICIPANT_ROLE_POINT_A &&
              constraint->participant_descriptors[2].role == CONSTRAINT_PARTICIPANT_ROLE_CENTER);

    ecs_world_shutdown(&world);
    return ok ? 0 : 1;
}

static int test_scene_constraint_creation_rejects_directional_raw_entity_signatures_D03_D06(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(1, 1, 1, 1));
    ecs_entity_t line_a = scene_add_line_to_sketch(&scene, sketch,
                                                   vec3_make(0.0f, 0.0f, 0.0f),
                                                   vec3_make(1.0f, 0.0f, 0.0f),
                                                   vec4_make(1, 1, 1, 1), 1.0f);
    ecs_entity_t line_b = scene_add_line_to_sketch(&scene, sketch,
                                                   vec3_make(0.0f, 1.0f, 0.0f),
                                                   vec3_make(1.0f, 1.0f, 0.0f),
                                                   vec4_make(1, 1, 1, 1), 1.0f);
    ecs_entity_t arc_a = scene_add_arc_to_sketch(&scene, sketch,
                                                 vec3_make(0.0f, 0.0f, 0.0f), 1.0f,
                                                 0.0f, 1.5707963f, vec3_make(0.0f, 0.0f, 1.0f),
                                                 vec4_make(1, 1, 1, 1), 1.0f);
    ecs_entity_t arc_b = scene_add_arc_to_sketch(&scene, sketch,
                                                 vec3_make(2.0f, 0.0f, 0.0f), 1.0f,
                                                 0.0f, 1.5707963f, vec3_make(0.0f, 0.0f, 1.0f),
                                                 vec4_make(1, 1, 1, 1), 1.0f);
    if (!sketch || !line_a || !line_b || !arc_a || !arc_b) {
        ecs_world_shutdown(&world);
        return 1;
    }

    constraint_participant_descriptor_t raw_lines[2] = {
        constraint_participant_descriptor_make((uint64_t)line_a, CONSTRAINT_PARTICIPANT_ROLE_ENTITY, 0),
        constraint_participant_descriptor_make((uint64_t)line_b, CONSTRAINT_PARTICIPANT_ROLE_ENTITY, 0),
    };
    constraint_participant_descriptor_t raw_arcs[2] = {
        constraint_participant_descriptor_make((uint64_t)arc_a, CONSTRAINT_PARTICIPANT_ROLE_ENTITY, 0),
        constraint_participant_descriptor_make((uint64_t)arc_b, CONSTRAINT_PARTICIPANT_ROLE_ENTITY, 0),
    };

    ecs_entity_t along_raw_line =
        scene_add_constraint_to_sketch_with_descriptors(&scene, sketch, CONSTRAINT_ALONG_Y, raw_lines, 2, 0.0f, false);
    ecs_entity_t along_raw_arc =
        scene_add_constraint_to_sketch_with_descriptors(&scene, sketch, CONSTRAINT_ALONG_Z, raw_arcs, 2, 0.0f, false);

    int ok = (along_raw_line == 0 && along_raw_arc == 0);
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

static int test_endpoint_replay_undo_redo_triggers_sketch_solver_and_script_side_effects(void) {
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

    EndPointsComp *line_endpoints = ecs_world_get_endpoints(scene.world, line);
    endpoint_binding_t binding_a = {0};
    if (!line_endpoints || !endpoints_comp_find_binding(line_endpoints, CONSTRAINT_PARTICIPANT_ROLE_POINT_A, &binding_a)) {
        ecs_world_shutdown(&world);
        return 1;
    }

    SketchComp *sk = ecs_world_get_sketch(scene.world, sketch);
    if (!sk) {
        ecs_world_shutdown(&world);
        return 1;
    }

    sk->auto_solve_pending = false;
    uint32_t before_request = sk->solve_request_serial;
    uint64_t before_revision = scene_script_emit_revision(&scene);

    undo_redo_t undo = {0};
    undo_redo_init(&undo, &scene, 8);
    undo_cmd_move_endpoint_participant(&undo,
                                       line,
                                       CONSTRAINT_PARTICIPANT_ROLE_POINT_A,
                                       binding_a.sub_index,
                                       vec3_make(0.0f, 0.0f, 0.0f),
                                       vec3_make(0.3f, 0.1f, 0.0f));

    if (!undo_redo_undo(&undo)) {
        undo_redo_shutdown(&undo);
        ecs_world_shutdown(&world);
        return 1;
    }

    sk = ecs_world_get_sketch(scene.world, sketch);
    if (!sk ||
        sk->solve_request_serial <= before_request ||
        !sk->auto_solve_pending ||
        scene_script_emit_revision(&scene) <= before_revision) {
        undo_redo_shutdown(&undo);
        ecs_world_shutdown(&world);
        return 1;
    }

    uint32_t after_undo_request = sk->solve_request_serial;
    uint64_t after_undo_revision = scene_script_emit_revision(&scene);

    if (!undo_redo_redo(&undo)) {
        undo_redo_shutdown(&undo);
        ecs_world_shutdown(&world);
        return 1;
    }

    sk = ecs_world_get_sketch(scene.world, sketch);
    int ok = (sk &&
              sk->solve_request_serial >= after_undo_request &&
              sk->auto_solve_pending &&
              scene_script_emit_revision(&scene) > after_undo_revision);

    undo_redo_shutdown(&undo);
    ecs_world_shutdown(&world);
    return ok ? 0 : 1;
}

static int test_endpoint_replay_non_sketch_owner_does_not_emit_script_revision(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t line = scene_add_line(&scene,
                                       vec3_make(0.0f, 0.0f, 0.0f),
                                       vec3_make(1.0f, 0.0f, 0.0f),
                                       vec4_make(1, 1, 1, 1), 1.0f);
    if (!line) return 1;

    uint64_t before_revision = scene_script_emit_revision(&scene);

    undo_redo_t undo = {0};
    undo_redo_init(&undo, &scene, 8);
    undo_cmd_move_endpoint_participant(&undo,
                                       line,
                                       CONSTRAINT_PARTICIPANT_ROLE_POINT_A,
                                       0u,
                                       vec3_make(0.0f, 0.0f, 0.0f),
                                       vec3_make(0.2f, 0.0f, 0.0f));

    if (!undo_redo_undo(&undo) || !undo_redo_redo(&undo)) {
        undo_redo_shutdown(&undo);
        ecs_world_shutdown(&world);
        return 1;
    }

    int ok = (scene_script_emit_revision(&scene) == before_revision);
    undo_redo_shutdown(&undo);
    ecs_world_shutdown(&world);
    return ok ? 0 : 1;
}

static bool vec3_exact_eq(vec3_t a, vec3_t b) {
    return a.x == b.x && a.y == b.y && a.z == b.z;
}

static int test_recalculate_idempotent_on_unchanged_sketch(void) {
    // idempotent: repeated recalc on unchanged solved sketch keeps status/geometry stable.
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(1, 1, 1, 1));
    ecs_entity_t p1 = scene_add_point_to_sketch(&scene, sketch, vec3_make(0, 0, 0), vec4_make(1, 1, 1, 1), 0.01f);
    ecs_entity_t p2 = scene_add_point_to_sketch(&scene, sketch, vec3_make(4, 0, 0), vec4_make(1, 1, 1, 1), 0.01f);
    if (!sketch || !p1 || !p2) return 1;

    ecs_entity_t participants[2] = { p1, p2 };
    ecs_entity_t c = scene_add_constraint_to_sketch(&scene, sketch, CONSTRAINT_COINCIDENT, participants, 2, 0.0f, false);
    if (!c) return 1;

    bool first_ok = scene_solver_request_recalculate(&scene, sketch);
    GeometryComp *g1 = ecs_world_get_geometry(scene.world, p1);
    GeometryComp *g2 = ecs_world_get_geometry(scene.world, p2);
    SketchComp *sk = ecs_world_get_sketch(scene.world, sketch);
    if (!first_ok || !g1 || !g2 || !sk) return 1;
    vec3_t p1_after_first = g1->data.point.point;
    vec3_t p2_after_first = g2->data.point.point;
    sketch_status_t status_after_first = sk->status;
    uint32_t request_after_first = sk->solve_request_serial;
    uint32_t complete_after_first = sk->solve_completed_serial;

    bool second_ok = scene_solver_request_recalculate(&scene, sketch);
    g1 = ecs_world_get_geometry(scene.world, p1);
    g2 = ecs_world_get_geometry(scene.world, p2);
    sk = ecs_world_get_sketch(scene.world, sketch);
    int ok = (second_ok &&
              g1 && g2 && sk &&
              vec3_exact_eq(g1->data.point.point, p1_after_first) &&
              vec3_exact_eq(g2->data.point.point, p2_after_first) &&
              sk->status == status_after_first &&
              sk->solve_request_serial == (request_after_first + 1) &&
              sk->solve_completed_serial == (complete_after_first + 1));
    ecs_world_shutdown(&world);
    return ok ? 0 : 1;
}

static int test_driving_length_angle_unsat_no_mutation_contract_D09(void) {
    // D-09: unsatisfiable driving LENGTH/ANGLE yields explicit failure implication and no mutation.
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(1, 1, 1, 1));
    ecs_entity_t line_a = scene_add_line_to_sketch(&scene, sketch,
                                                   vec3_make(0.0f, 0.0f, 0.0f),
                                                   vec3_make(1.0f, 0.0f, 0.0f),
                                                   vec4_make(1, 1, 1, 1), 1.0f);
    ecs_entity_t line_b = scene_add_line_to_sketch(&scene, sketch,
                                                   vec3_make(0.0f, 0.0f, 0.0f),
                                                   vec3_make(0.0f, 1.0f, 0.0f),
                                                   vec4_make(1, 1, 1, 1), 1.0f);
    if (!sketch || !line_a || !line_b) return 1;

    ecs_entity_t angle_participants[2] = { line_a, line_b };
    ecs_entity_t c_angle = scene_add_constraint_to_sketch(&scene, sketch, CONSTRAINT_ANGLE,
                                                           angle_participants, 2, 0.5f, false);
    if (!c_angle) return 1;

    SketchGeometryStateComp *sa = ecs_world_get_sketch_geometry_state(scene.world, line_a);
    SketchGeometryStateComp *sb = ecs_world_get_sketch_geometry_state(scene.world, line_b);
    if (!sa || !sb) return 1;
    sa->fixed = true;
    sb->fixed = true;

    GeometryComp *ga = ecs_world_get_geometry(scene.world, line_a);
    GeometryComp *gb = ecs_world_get_geometry(scene.world, line_b);
    if (!ga || !gb || ga->type != GEOM_LINE || gb->type != GEOM_LINE) return 1;
    vec3_t a0_before = ga->data.line.a;
    vec3_t a1_before = ga->data.line.b;
    vec3_t b0_before = gb->data.line.a;
    vec3_t b1_before = gb->data.line.b;

    bool solved = scene_solver_request_recalculate(&scene, sketch);
    const scene_solver_failure_implication_t *imp = scene_solver_failure_implication(&scene);
    ga = ecs_world_get_geometry(scene.world, line_a);
    gb = ecs_world_get_geometry(scene.world, line_b);

    int ok = (!solved &&
              ga && gb &&
              vec3_exact_eq(ga->data.line.a, a0_before) &&
              vec3_exact_eq(ga->data.line.b, a1_before) &&
              vec3_exact_eq(gb->data.line.a, b0_before) &&
              vec3_exact_eq(gb->data.line.b, b1_before) &&
              imp && imp->active &&
              imp->first_constraint == c_angle &&
              imp->reason[0] != '\0');
    ecs_world_shutdown(&world);
    return ok ? 0 : 1;
}

static int test_driving_length_atomic_success_contract_D10(void) {
    // D-10: satisfiable driving dimensional solve commits geometry atomically in one completion.
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(1, 1, 1, 1));
    ecs_entity_t line_a = scene_add_line_to_sketch(&scene, sketch,
                                                   vec3_make(0.0f, 0.0f, 0.0f),
                                                   vec3_make(2.0f, 0.0f, 0.0f),
                                                   vec4_make(1, 1, 1, 1), 1.0f);
    ecs_entity_t p1 = scene_add_point_to_sketch(&scene, sketch, vec3_make(0.0f, 0.0f, 0.0f), vec4_make(1, 1, 1, 1), 0.01f);
    ecs_entity_t p2 = scene_add_point_to_sketch(&scene, sketch, vec3_make(3.0f, 0.0f, 0.0f), vec4_make(1, 1, 1, 1), 0.01f);
    if (!sketch || !line_a || !p1 || !p2) return 1;

    ecs_entity_t length_participants[1] = { line_a };
    ecs_entity_t c_length = scene_add_constraint_to_sketch(&scene, sketch, CONSTRAINT_LENGTH,
                                                            length_participants, 1, 5.0f, false);
    ecs_entity_t coincident_participants[2] = { p1, p2 };
    ecs_entity_t c_coincident = scene_add_constraint_to_sketch(&scene, sketch, CONSTRAINT_COINCIDENT,
                                                                coincident_participants, 2, 0.0f, false);
    if (!c_length || !c_coincident) return 1;

    GeometryComp *g1 = ecs_world_get_geometry(scene.world, p1);
    GeometryComp *g2 = ecs_world_get_geometry(scene.world, p2);
    if (!g1 || !g2 || g1->type != GEOM_POINT || g2->type != GEOM_POINT) return 1;
    vec3_t before_p1 = g1->data.point.point;
    vec3_t before_p2 = g2->data.point.point;
    bool solved = scene_solver_request_recalculate(&scene, sketch);
    g1 = ecs_world_get_geometry(scene.world, p1);
    g2 = ecs_world_get_geometry(scene.world, p2);
    SketchComp *sk = ecs_world_get_sketch(scene.world, sketch);

    int ok = (solved &&
              g1 && g2 && sk &&
              (!vec3_exact_eq(g1->data.point.point, before_p1) ||
               !vec3_exact_eq(g2->data.point.point, before_p2)) &&
              vec3_close(g1->data.point.point, g2->data.point.point, 1e-4f) &&
              sk->solve_completed_serial == sk->solve_request_serial);
    ecs_world_shutdown(&world);
    return ok ? 0 : 1;
}

static int test_driving_angle_between_two_lines_atomic_success_contract_D10(void) {
    // D-10: satisfiable driving ANGLE between lines commits geometry atomically in one completion.
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(1, 1, 1, 1));
    ecs_entity_t line_a = scene_add_line_to_sketch(&scene, sketch,
                                                   vec3_make(0.0f, 0.0f, 0.0f),
                                                   vec3_make(1.0f, 0.0f, 0.0f),
                                                   vec4_make(1, 1, 1, 1), 1.0f);
    ecs_entity_t line_b = scene_add_line_to_sketch(&scene, sketch,
                                                   vec3_make(0.0f, 0.0f, 0.0f),
                                                   vec3_make(0.0f, 1.0f, 0.0f),
                                                   vec4_make(1, 1, 1, 1), 1.0f);
    if (!sketch || !line_a || !line_b) return 1;

    ecs_entity_t angle_participants[2] = { line_a, line_b };
    ecs_entity_t c_angle = scene_add_constraint_to_sketch(&scene, sketch, CONSTRAINT_ANGLE,
                                                           angle_participants, 2, 0.5f, false);
    if (!c_angle) return 1;

    GeometryComp *ga = ecs_world_get_geometry(scene.world, line_a);
    GeometryComp *gb = ecs_world_get_geometry(scene.world, line_b);
    if (!ga || !gb || ga->type != GEOM_LINE || gb->type != GEOM_LINE) return 1;
    vec3_t b1_before = gb->data.line.b;

    bool solved = scene_solver_request_recalculate(&scene, sketch);
    ga = ecs_world_get_geometry(scene.world, line_a);
    gb = ecs_world_get_geometry(scene.world, line_b);
    SketchComp *sk = ecs_world_get_sketch(scene.world, sketch);
    if (!solved || !ga || !gb || !sk) {
        ecs_world_shutdown(&world);
        return 1;
    }

    vec3_t va = vec3_sub(ga->data.line.b, ga->data.line.a);
    vec3_t vb = vec3_sub(gb->data.line.b, gb->data.line.a);
    float len_a = vec3_length(va);
    float len_b = vec3_length(vb);
    if (len_a <= 1e-6f || len_b <= 1e-6f) {
        ecs_world_shutdown(&world);
        return 1;
    }
    vec3_t na = vec3_scale(va, 1.0f / len_a);
    vec3_t nb = vec3_scale(vb, 1.0f / len_b);
    float cos_angle = vec3_dot(na, nb);
    if (cos_angle > 1.0f) cos_angle = 1.0f;
    if (cos_angle < -1.0f) cos_angle = -1.0f;
    float actual_angle = acosf(cos_angle);

    int ok = (solved &&
              sk->status == SKETCH_STATUS_SOLVED &&
              !vec3_exact_eq(gb->data.line.b, b1_before) &&
              fabsf(actual_angle - 0.5f) <= 1e-4f &&
              sk->solve_completed_serial == sk->solve_request_serial);
    ecs_world_shutdown(&world);
    return ok ? 0 : 1;
}

static int test_driving_angle_90_degree_default_radians_contract_D10(void) {
    // Regression: 90° UX default must feed solver as pi/2 radians.
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
                                                   vec3_make(0.5f, 2.0f, 0.0f),
                                                   vec4_make(1, 1, 1, 1), 1.0f);
    if (!sketch || !line_a || !line_b) return 1;

    ecs_entity_t angle_participants[2] = { line_a, line_b };
    const float desired_angle = 1.57079632679f; // 90 degrees
    ecs_entity_t c_angle = scene_add_constraint_to_sketch(&scene, sketch, CONSTRAINT_ANGLE,
                                                           angle_participants, 2, desired_angle, false);
    if (!c_angle) return 1;

    bool solved = scene_solver_request_recalculate(&scene, sketch);
    GeometryComp *ga = ecs_world_get_geometry(scene.world, line_a);
    GeometryComp *gb = ecs_world_get_geometry(scene.world, line_b);
    if (!solved || !ga || !gb || ga->type != GEOM_LINE || gb->type != GEOM_LINE) {
        ecs_world_shutdown(&world);
        return 1;
    }

    vec3_t va = vec3_sub(ga->data.line.b, ga->data.line.a);
    vec3_t vb = vec3_sub(gb->data.line.b, gb->data.line.a);
    float len_a = vec3_length(va);
    float len_b = vec3_length(vb);
    if (len_a <= 1e-6f || len_b <= 1e-6f) {
        ecs_world_shutdown(&world);
        return 1;
    }
    vec3_t na = vec3_scale(va, 1.0f / len_a);
    vec3_t nb = vec3_scale(vb, 1.0f / len_b);
    float cos_angle = vec3_dot(na, nb);
    if (cos_angle > 1.0f) cos_angle = 1.0f;
    if (cos_angle < -1.0f) cos_angle = -1.0f;
    float actual_angle = acosf(cos_angle);

    int ok = fabsf(actual_angle - desired_angle) <= 1e-4f;
    ecs_world_shutdown(&world);
    return ok ? 0 : 1;
}

static int test_recalculate_along_x_arc_center_descriptor_contract_D11(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(1, 1, 1, 1));
    ecs_entity_t p = scene_add_point_to_sketch(&scene, sketch, vec3_make(3.0f, 1.0f, 0.0f), vec4_make(1, 1, 1, 1), 0.01f);
    ecs_entity_t line = scene_add_line_to_sketch(&scene, sketch,
                                                 vec3_make(0.0f, 0.0f, 0.0f),
                                                 vec3_make(2.0f, 0.0f, 0.0f),
                                                 vec4_make(1, 1, 1, 1), 1.0f);
    ecs_entity_t arc = scene_add_arc_to_sketch(&scene, sketch,
                                               vec3_make(1.0f, 1.0f, 0.0f), 1.0f,
                                               0.0f, 1.5707963f, vec3_make(0.0f, 0.0f, 1.0f),
                                               vec4_make(1, 1, 1, 1), 1.0f);
    if (!sketch || !p || !line || !arc) return 1;

    constraint_participant_descriptor_t along_desc[3] = {
        constraint_participant_descriptor_make((uint64_t)p, CONSTRAINT_PARTICIPANT_ROLE_ENTITY, 0),
        constraint_participant_descriptor_make((uint64_t)line, CONSTRAINT_PARTICIPANT_ROLE_POINT_A, 0),
        constraint_participant_descriptor_make((uint64_t)arc, CONSTRAINT_PARTICIPANT_ROLE_CENTER, 0),
    };
    ecs_entity_t c_along = scene_add_constraint_to_sketch_with_descriptors(&scene, sketch, CONSTRAINT_ALONG_X, along_desc, 3, 0.0f, false);
    if (!c_along) return 1;

    bool solved = scene_solver_request_recalculate(&scene, sketch);
    GeometryComp *gp = ecs_world_get_geometry(scene.world, p);
    GeometryComp *gl = ecs_world_get_geometry(scene.world, line);
    GeometryComp *ga = ecs_world_get_geometry(scene.world, arc);
    if (!gp || !gl || !ga || gp->type != GEOM_POINT || gl->type != GEOM_LINE || ga->type != GEOM_ARC) return 1;

    int ok = (solved &&
              fabsf(gp->data.point.point.y - gl->data.line.a.y) <= 1e-4f &&
              fabsf(gp->data.point.point.y - ga->data.arc.center.y) <= 1e-4f &&
              fabsf(gp->data.point.point.z - gl->data.line.a.z) <= 1e-4f &&
              fabsf(gp->data.point.point.z - ga->data.arc.center.z) <= 1e-4f);
    ecs_world_shutdown(&world);
    return ok ? 0 : 1;
}

static int test_recalculate_along_x_unsat_fixed_is_transactional_contract_D11(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(1, 1, 1, 1));
    ecs_entity_t p1 = scene_add_point_to_sketch(&scene, sketch, vec3_make(0.0f, 0.0f, 0.0f), vec4_make(1, 1, 1, 1), 0.01f);
    ecs_entity_t p2 = scene_add_point_to_sketch(&scene, sketch, vec3_make(2.0f, 1.0f, 0.0f), vec4_make(1, 1, 1, 1), 0.01f);
    if (!sketch || !p1 || !p2) return 1;

    SketchGeometryStateComp *s1 = ecs_world_get_sketch_geometry_state(scene.world, p1);
    SketchGeometryStateComp *s2 = ecs_world_get_sketch_geometry_state(scene.world, p2);
    if (!s1 || !s2) return 1;
    s1->fixed = true;
    s2->fixed = true;

    ecs_entity_t participants[2] = { p1, p2 };
    ecs_entity_t c_along = scene_add_constraint_to_sketch(&scene, sketch, CONSTRAINT_ALONG_X, participants, 2, 0.0f, false);
    if (!c_along) return 1;

    GeometryComp *g1 = ecs_world_get_geometry(scene.world, p1);
    GeometryComp *g2 = ecs_world_get_geometry(scene.world, p2);
    if (!g1 || !g2 || g1->type != GEOM_POINT || g2->type != GEOM_POINT) return 1;
    vec3_t before_1 = g1->data.point.point;
    vec3_t before_2 = g2->data.point.point;

    bool solved = scene_solver_request_recalculate(&scene, sketch);
    const scene_solver_failure_implication_t *imp = scene_solver_failure_implication(&scene);
    g1 = ecs_world_get_geometry(scene.world, p1);
    g2 = ecs_world_get_geometry(scene.world, p2);
    int ok = (!solved &&
              g1 && g2 &&
              vec3_exact_eq(g1->data.point.point, before_1) &&
              vec3_exact_eq(g2->data.point.point, before_2) &&
              imp && imp->active &&
              imp->first_constraint == c_along &&
              strcmp(imp->reason, "Unsatisfied driving ALONG X constraint.") == 0);
    ecs_world_shutdown(&world);
    return ok ? 0 : 1;
}

static int test_recalculate_along_xyz_group_points_contract_D11(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(1, 1, 1, 1));
    ecs_entity_t p1 = scene_add_point_to_sketch(&scene, sketch, vec3_make(0.0f, 5.0f, 9.0f), vec4_make(1, 1, 1, 1), 0.01f);
    ecs_entity_t p2 = scene_add_point_to_sketch(&scene, sketch, vec3_make(4.0f, 3.0f, 2.0f), vec4_make(1, 1, 1, 1), 0.01f);
    ecs_entity_t p3 = scene_add_point_to_sketch(&scene, sketch, vec3_make(8.0f, 1.0f, -1.0f), vec4_make(1, 1, 1, 1), 0.01f);
    if (!sketch || !p1 || !p2 || !p3) return 1;

    ecs_entity_t participants[3] = { p1, p2, p3 };
    ecs_entity_t c = scene_add_constraint_to_sketch(&scene, sketch, CONSTRAINT_ALONG_X, participants, 3, 0.0f, false);
    if (!c) return 1;

    bool solved = scene_solver_request_recalculate(&scene, sketch);
    GeometryComp *g1 = ecs_world_get_geometry(scene.world, p1);
    GeometryComp *g2 = ecs_world_get_geometry(scene.world, p2);
    GeometryComp *g3 = ecs_world_get_geometry(scene.world, p3);
    if (!g1 || !g2 || !g3 || g1->type != GEOM_POINT || g2->type != GEOM_POINT || g3->type != GEOM_POINT) return 1;

    int ok = (solved &&
              fabsf(g1->data.point.point.y - g2->data.point.point.y) <= 1e-4f &&
              fabsf(g2->data.point.point.y - g3->data.point.point.y) <= 1e-4f &&
              fabsf(g1->data.point.point.z - g2->data.point.point.z) <= 1e-4f &&
              fabsf(g2->data.point.point.z - g3->data.point.point.z) <= 1e-4f &&
              fabsf(g1->data.point.point.x - 0.0f) <= 1e-4f &&
              fabsf(g2->data.point.point.x - 4.0f) <= 1e-4f &&
              fabsf(g3->data.point.point.x - 8.0f) <= 1e-4f);
    ecs_world_shutdown(&world);
    return ok ? 0 : 1;
}

static int test_recalculate_along_y_group_points_contract_D11(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(1, 1, 1, 1));
    ecs_entity_t p1 = scene_add_point_to_sketch(&scene, sketch, vec3_make(0.0f, 5.0f, 9.0f), vec4_make(1, 1, 1, 1), 0.01f);
    ecs_entity_t p2 = scene_add_point_to_sketch(&scene, sketch, vec3_make(4.0f, 3.0f, 2.0f), vec4_make(1, 1, 1, 1), 0.01f);
    ecs_entity_t p3 = scene_add_point_to_sketch(&scene, sketch, vec3_make(8.0f, 1.0f, -1.0f), vec4_make(1, 1, 1, 1), 0.01f);
    if (!sketch || !p1 || !p2 || !p3) return 1;

    ecs_entity_t participants[3] = { p1, p2, p3 };
    ecs_entity_t c = scene_add_constraint_to_sketch(&scene, sketch, CONSTRAINT_ALONG_Y, participants, 3, 0.0f, false);
    if (!c) return 1;

    bool solved = scene_solver_request_recalculate(&scene, sketch);
    GeometryComp *g1 = ecs_world_get_geometry(scene.world, p1);
    GeometryComp *g2 = ecs_world_get_geometry(scene.world, p2);
    GeometryComp *g3 = ecs_world_get_geometry(scene.world, p3);
    if (!g1 || !g2 || !g3 || g1->type != GEOM_POINT || g2->type != GEOM_POINT || g3->type != GEOM_POINT) return 1;

    int ok = (solved &&
              fabsf(g1->data.point.point.x - g2->data.point.point.x) <= 1e-4f &&
              fabsf(g2->data.point.point.x - g3->data.point.point.x) <= 1e-4f &&
              fabsf(g1->data.point.point.z - g2->data.point.point.z) <= 1e-4f &&
              fabsf(g2->data.point.point.z - g3->data.point.point.z) <= 1e-4f &&
              fabsf(g1->data.point.point.y - 5.0f) <= 1e-4f &&
              fabsf(g2->data.point.point.y - 3.0f) <= 1e-4f &&
              fabsf(g3->data.point.point.y - 1.0f) <= 1e-4f);
    ecs_world_shutdown(&world);
    return ok ? 0 : 1;
}

static int test_recalculate_along_z_group_points_contract_D11(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(1, 1, 1, 1));
    ecs_entity_t p1 = scene_add_point_to_sketch(&scene, sketch, vec3_make(0.0f, 5.0f, 9.0f), vec4_make(1, 1, 1, 1), 0.01f);
    ecs_entity_t p2 = scene_add_point_to_sketch(&scene, sketch, vec3_make(4.0f, 3.0f, 2.0f), vec4_make(1, 1, 1, 1), 0.01f);
    ecs_entity_t p3 = scene_add_point_to_sketch(&scene, sketch, vec3_make(8.0f, 1.0f, -1.0f), vec4_make(1, 1, 1, 1), 0.01f);
    if (!sketch || !p1 || !p2 || !p3) return 1;

    ecs_entity_t participants[3] = { p1, p2, p3 };
    ecs_entity_t c = scene_add_constraint_to_sketch(&scene, sketch, CONSTRAINT_ALONG_Z, participants, 3, 0.0f, false);
    if (!c) return 1;

    bool solved = scene_solver_request_recalculate(&scene, sketch);
    GeometryComp *g1 = ecs_world_get_geometry(scene.world, p1);
    GeometryComp *g2 = ecs_world_get_geometry(scene.world, p2);
    GeometryComp *g3 = ecs_world_get_geometry(scene.world, p3);
    if (!g1 || !g2 || !g3 || g1->type != GEOM_POINT || g2->type != GEOM_POINT || g3->type != GEOM_POINT) return 1;

    int ok = (solved &&
              fabsf(g1->data.point.point.x - g2->data.point.point.x) <= 1e-4f &&
              fabsf(g2->data.point.point.x - g3->data.point.point.x) <= 1e-4f &&
              fabsf(g1->data.point.point.y - g2->data.point.point.y) <= 1e-4f &&
              fabsf(g2->data.point.point.y - g3->data.point.point.y) <= 1e-4f &&
              fabsf(g1->data.point.point.z - 9.0f) <= 1e-4f &&
              fabsf(g2->data.point.point.z - 2.0f) <= 1e-4f &&
              fabsf(g3->data.point.point.z + 1.0f) <= 1e-4f);
    ecs_world_shutdown(&world);
    return ok ? 0 : 1;
}

static int test_recalculate_along_z_mixed_landmarks_contract_D11(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(1, 1, 1, 1));
    ecs_entity_t p = scene_add_point_to_sketch(&scene, sketch, vec3_make(0.0f, 0.0f, 8.0f), vec4_make(1, 1, 1, 1), 0.01f);
    ecs_entity_t line = scene_add_line_to_sketch(&scene, sketch,
                                                 vec3_make(0.0f, 0.0f, -2.0f),
                                                 vec3_make(1.0f, 0.0f, -4.0f),
                                                 vec4_make(1, 1, 1, 1), 1.0f);
    ecs_entity_t arc = scene_add_arc_to_sketch(&scene, sketch,
                                               vec3_make(1.0f, 1.0f, 3.0f), 2.0f,
                                               0.0f, 1.5707963f, vec3_make(0.0f, 0.0f, 1.0f),
                                               vec4_make(1, 1, 1, 1), 1.0f);
    if (!sketch || !p || !line || !arc) return 1;

    constraint_participant_descriptor_t desc[3] = {
        constraint_participant_descriptor_make((uint64_t)p, CONSTRAINT_PARTICIPANT_ROLE_ENTITY, 0),
        constraint_participant_descriptor_make((uint64_t)line, CONSTRAINT_PARTICIPANT_ROLE_POINT_B, 0),
        constraint_participant_descriptor_make((uint64_t)arc, CONSTRAINT_PARTICIPANT_ROLE_POINT_A, 0),
    };
    ecs_entity_t c_along = scene_add_constraint_to_sketch_with_descriptors(&scene, sketch, CONSTRAINT_ALONG_Z, desc, 3, 0.0f, false);
    if (!c_along) return 1;

    bool solved = scene_solver_request_recalculate(&scene, sketch);
    GeometryComp *gp = ecs_world_get_geometry(scene.world, p);
    GeometryComp *gl = ecs_world_get_geometry(scene.world, line);
    GeometryComp *ga = ecs_world_get_geometry(scene.world, arc);
    if (!gp || !gl || !ga || gp->type != GEOM_POINT || gl->type != GEOM_LINE || ga->type != GEOM_ARC) return 1;

    vec3_t arc_a = vec3_make(0.0f, 0.0f, 0.0f);
    if (!scene_entity_participant_subpoint(ga, CONSTRAINT_PARTICIPANT_ROLE_POINT_A, &arc_a, NULL)) return 1;
    int ok = (solved &&
              fabsf(gp->data.point.point.x - gl->data.line.b.x) <= 1e-4f &&
              fabsf(gp->data.point.point.x - arc_a.x) <= 1e-4f &&
              fabsf(gp->data.point.point.y - gl->data.line.b.y) <= 1e-4f &&
              fabsf(gp->data.point.point.y - arc_a.y) <= 1e-4f &&
              fabsf(gp->data.point.point.z - 8.0f) <= 1e-4f &&
              fabsf(gl->data.line.b.z + 4.0f) <= 1e-4f &&
              fabsf(arc_a.z - 3.0f) <= 1e-4f);
    ecs_world_shutdown(&world);
    return ok ? 0 : 1;
}

static int test_recalculate_along_and_angle_coexistence_is_deterministic_D11(void) {
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

    bool solved_first = scene_solver_request_recalculate(&scene, sketch);
    GeometryComp *ga = ecs_world_get_geometry(scene.world, line_a);
    GeometryComp *gb = ecs_world_get_geometry(scene.world, line_b);
    if (!solved_first || !ga || !gb || ga->type != GEOM_LINE || gb->type != GEOM_LINE) return 1;
    vec3_t a1_after_first = ga->data.line.b;
    vec3_t b1_after_first = gb->data.line.b;

    bool solved_second = scene_solver_request_recalculate(&scene, sketch);
    ga = ecs_world_get_geometry(scene.world, line_a);
    gb = ecs_world_get_geometry(scene.world, line_b);
    if (!solved_second || !ga || !gb) return 1;

    int ok = (fabsf(ga->data.line.b.y - gb->data.line.b.y) <= 1e-4f &&
              fabsf(ga->data.line.b.z - gb->data.line.b.z) <= 1e-4f &&
              vec3_exact_eq(ga->data.line.b, a1_after_first) &&
              vec3_exact_eq(gb->data.line.b, b1_after_first));
    ecs_world_shutdown(&world);
    return ok ? 0 : 1;
}

static int test_arci_arc_axis_line_reorients_arc_normal_and_keeps_line_reference_D02_D03(void) {
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

    ecs_entity_t participants[2] = { line, arc };
    ecs_entity_t c = scene_add_constraint_to_sketch(&scene, sketch, CONSTRAINT_ARC_AXIS_LINE, participants, 2, 0.0f, false);
    if (!c) return 1;

    GeometryComp *line_geom = ecs_world_get_geometry(scene.world, line);
    if (!line_geom || line_geom->type != GEOM_LINE) return 1;
    vec3_t line_a_before = line_geom->data.line.a;
    vec3_t line_b_before = line_geom->data.line.b;

    bool solved = scene_solver_request_recalculate(&scene, sketch);
    GeometryComp *arc_geom = ecs_world_get_geometry(scene.world, arc);
    line_geom = ecs_world_get_geometry(scene.world, line);
    if (!solved || !arc_geom || arc_geom->type != GEOM_ARC || !line_geom || line_geom->type != GEOM_LINE) {
        ecs_world_shutdown(&world);
        return 1;
    }

    vec3_t line_dir = vec3_normalize(vec3_sub(line_geom->data.line.b, line_geom->data.line.a));
    vec3_t arc_normal = vec3_normalize(arc_geom->data.arc.normal);
    float axis_dot = fabsf(vec3_dot(line_dir, arc_normal));
    int ok = (axis_dot >= 0.999f &&
              vec3_exact_eq(line_geom->data.line.a, line_a_before) &&
              vec3_exact_eq(line_geom->data.line.b, line_b_before));
    ecs_world_shutdown(&world);
    return ok ? 0 : 1;
}

static int test_arci_arc_axis_line_fixed_fixed_unsat_is_transactional_D04(void) {
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

    GeometryComp *line_geom = ecs_world_get_geometry(scene.world, line);
    GeometryComp *arc_geom = ecs_world_get_geometry(scene.world, arc);
    if (!line_geom || !arc_geom) return 1;
    vec3_t line_a_before = line_geom->data.line.a;
    vec3_t line_b_before = line_geom->data.line.b;
    vec3_t arc_normal_before = arc_geom->data.arc.normal;

    bool solved = scene_solver_request_recalculate(&scene, sketch);
    const scene_solver_failure_implication_t *imp = scene_solver_failure_implication(&scene);
    line_geom = ecs_world_get_geometry(scene.world, line);
    arc_geom = ecs_world_get_geometry(scene.world, arc);
    int ok = (!solved &&
              line_geom && arc_geom &&
              vec3_exact_eq(line_geom->data.line.a, line_a_before) &&
              vec3_exact_eq(line_geom->data.line.b, line_b_before) &&
              vec3_exact_eq(arc_geom->data.arc.normal, arc_normal_before) &&
              imp && imp->active &&
              imp->first_constraint == c &&
              strcmp(imp->reason, "Unsatisfied arc-axis-vs-line constraint.") == 0);
    ecs_world_shutdown(&world);
    return ok ? 0 : 1;
}

static int test_arci_line_arc_endpoint_tangency_satisfies_coincidence_then_tangent_D06_D07(void) {
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

    bool solved = scene_solver_request_recalculate(&scene, sketch);
    GeometryComp *line_geom = ecs_world_get_geometry(scene.world, line);
    GeometryComp *arc_geom = ecs_world_get_geometry(scene.world, arc);
    if (!solved || !line_geom || !arc_geom || line_geom->type != GEOM_LINE || arc_geom->type != GEOM_ARC) {
        ecs_world_shutdown(&world);
        return 1;
    }

    vec3_t line_endpoint = line_geom->data.line.a;
    vec3_t arc_endpoint = vec3_make(0, 0, 0);
    if (!scene_entity_participant_subpoint(arc_geom, CONSTRAINT_PARTICIPANT_ROLE_POINT_A, &arc_endpoint, NULL)) {
        ecs_world_shutdown(&world);
        return 1;
    }
    vec3_t shared = line_endpoint;
    vec3_t line_dir = vec3_normalize(vec3_sub(line_geom->data.line.b, shared));
    vec3_t radial = vec3_normalize(vec3_sub(shared, arc_geom->data.arc.center));
    vec3_t tangent = vec3_normalize(vec3_cross(vec3_normalize(arc_geom->data.arc.normal), radial));
    float tangency_dot = fabsf(vec3_dot(line_dir, tangent));
    int ok = (vec3_close(line_endpoint, arc_endpoint, 1e-4f) && tangency_dot >= 0.999f);
    ecs_world_shutdown(&world);
    return ok ? 0 : 1;
}

static int test_arci_line_arc_endpoint_tangency_unsat_fixed_is_transactional_D08(void) {
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

    GeometryComp *line_geom = ecs_world_get_geometry(scene.world, line);
    GeometryComp *arc_geom = ecs_world_get_geometry(scene.world, arc);
    if (!line_geom || !arc_geom) return 1;
    vec3_t line_a_before = line_geom->data.line.a;
    vec3_t line_b_before = line_geom->data.line.b;
    vec3_t arc_center_before = arc_geom->data.arc.center;
    float arc_start_before = arc_geom->data.arc.start_angle;
    float arc_end_before = arc_geom->data.arc.end_angle;
    vec3_t arc_normal_before = arc_geom->data.arc.normal;

    bool solved = scene_solver_request_recalculate(&scene, sketch);
    const scene_solver_failure_implication_t *imp = scene_solver_failure_implication(&scene);
    line_geom = ecs_world_get_geometry(scene.world, line);
    arc_geom = ecs_world_get_geometry(scene.world, arc);
    int ok = (!solved &&
              line_geom && arc_geom &&
              vec3_exact_eq(line_geom->data.line.a, line_a_before) &&
              vec3_exact_eq(line_geom->data.line.b, line_b_before) &&
              vec3_exact_eq(arc_geom->data.arc.center, arc_center_before) &&
              fabsf(arc_geom->data.arc.start_angle - arc_start_before) <= 1e-6f &&
              fabsf(arc_geom->data.arc.end_angle - arc_end_before) <= 1e-6f &&
              vec3_exact_eq(arc_geom->data.arc.normal, arc_normal_before) &&
              imp && imp->active &&
              imp->first_constraint == c &&
              strcmp(imp->reason, "Unsatisfied line-arc endpoint tangency constraint.") == 0);
    ecs_world_shutdown(&world);
    return ok ? 0 : 1;
}

static int test_arci_line_arc_endpoint_tangency_respects_line_endpoint_drag_anchor(void) {
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
    if (!line_a_endpoint || !ecs_is_alive(scene.world->world, line_a_endpoint)) {
        ecs_world_shutdown(&world);
        return 1;
    }

    GeometryComp *line_geom = ecs_world_get_geometry(scene.world, line);
    GeometryComp *arc_geom = ecs_world_get_geometry(scene.world, arc);
    if (!line_geom || !arc_geom) return 1;
    vec3_t line_a_before = line_geom->data.line.a;
    vec3_t line_b_before = line_geom->data.line.b;
    vec3_t drag_delta = vec3_make(0.6f, 0.4f, 0.0f);
    vec3_t expected_anchor = vec3_add(line_a_before, drag_delta);

    if (!scene_apply_endpoint_point_world_delta(&scene, line_a_endpoint, drag_delta)) {
        ecs_world_shutdown(&world);
        return 1;
    }
    bool solved = scene_solver_request_recalculate(&scene, sketch);
    line_geom = ecs_world_get_geometry(scene.world, line);
    arc_geom = ecs_world_get_geometry(scene.world, arc);
    if (!solved || !line_geom || !arc_geom) {
        ecs_world_shutdown(&world);
        return 1;
    }

    vec3_t line_a_after = line_geom->data.line.a;
    vec3_t line_b_after = line_geom->data.line.b;
    vec3_t arc_a_after = vec3_make(0, 0, 0);
    if (!scene_entity_participant_subpoint(arc_geom, CONSTRAINT_PARTICIPANT_ROLE_POINT_A, &arc_a_after, NULL)) {
        ecs_world_shutdown(&world);
        return 1;
    }

    vec3_t line_dir = vec3_normalize(vec3_sub(line_b_after, line_a_after));
    vec3_t radial = vec3_normalize(vec3_sub(line_a_after, arc_geom->data.arc.center));
    vec3_t tangent = vec3_normalize(vec3_cross(vec3_normalize(arc_geom->data.arc.normal), radial));
    float tangency_dot = fabsf(vec3_dot(line_dir, tangent));

    int ok = (vec3_close(line_a_after, expected_anchor, 1e-4f) &&
              vec3_close(line_a_after, arc_a_after, 1e-4f) &&
              !vec3_close(line_b_after, line_b_before, 1e-4f) &&
              tangency_dot >= 0.999f);
    ecs_world_shutdown(&world);
    return ok ? 0 : 1;
}

static int test_arci_arc_endpoint_angle_anchors_first_and_moves_second_D10_D11(void) {
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

    constraint_participant_descriptor_t desc[2] = {
        constraint_participant_descriptor_make((uint64_t)arc, CONSTRAINT_PARTICIPANT_ROLE_POINT_A, 0),
        constraint_participant_descriptor_make((uint64_t)arc, CONSTRAINT_PARTICIPANT_ROLE_POINT_B, 1),
    };
    const float desired_angle = 0.5f;
    ecs_entity_t c = scene_add_constraint_to_sketch_with_descriptors(
        &scene, sketch, CONSTRAINT_ARC_ENDPOINT_ANGLE, desc, 2, desired_angle, false);
    if (!c) return 1;

    GeometryComp *arc_geom = ecs_world_get_geometry(scene.world, arc);
    if (!arc_geom || arc_geom->type != GEOM_ARC) return 1;
    vec3_t before_a = vec3_make(0, 0, 0);
    vec3_t before_b = vec3_make(0, 0, 0);
    if (!scene_entity_participant_subpoint(arc_geom, CONSTRAINT_PARTICIPANT_ROLE_POINT_A, &before_a, NULL)) return 1;
    if (!scene_entity_participant_subpoint(arc_geom, CONSTRAINT_PARTICIPANT_ROLE_POINT_B, &before_b, NULL)) return 1;

    bool solved = scene_solver_request_recalculate(&scene, sketch);
    arc_geom = ecs_world_get_geometry(scene.world, arc);
    if (!solved || !arc_geom) {
        ecs_world_shutdown(&world);
        return 1;
    }
    vec3_t after_a = vec3_make(0, 0, 0);
    vec3_t after_b = vec3_make(0, 0, 0);
    if (!scene_entity_participant_subpoint(arc_geom, CONSTRAINT_PARTICIPANT_ROLE_POINT_A, &after_a, NULL)) return 1;
    if (!scene_entity_participant_subpoint(arc_geom, CONSTRAINT_PARTICIPANT_ROLE_POINT_B, &after_b, NULL)) return 1;

    vec3_t center = arc_geom->data.arc.center;
    vec3_t ra = vec3_normalize(vec3_sub(after_a, center));
    vec3_t rb = vec3_normalize(vec3_sub(after_b, center));
    float dot = vec3_dot(ra, rb);
    if (dot > 1.0f) dot = 1.0f;
    if (dot < -1.0f) dot = -1.0f;
    float actual_angle = acosf(dot);

    int ok = (vec3_close(after_a, before_a, 1e-4f) &&
              !vec3_close(after_b, before_b, 1e-4f) &&
              fabsf(actual_angle - desired_angle) <= 1e-4f);
    ecs_world_shutdown(&world);
    return ok ? 0 : 1;
}

static int test_arci_arc_endpoint_angle_unsat_fixed_target_is_transactional_D12(void) {
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

    GeometryComp *arc_geom = ecs_world_get_geometry(scene.world, arc);
    if (!arc_geom) return 1;
    vec3_t center_before = arc_geom->data.arc.center;
    float start_before = arc_geom->data.arc.start_angle;
    float end_before = arc_geom->data.arc.end_angle;
    vec3_t normal_before = arc_geom->data.arc.normal;

    bool solved = scene_solver_request_recalculate(&scene, sketch);
    const scene_solver_failure_implication_t *imp = scene_solver_failure_implication(&scene);
    arc_geom = ecs_world_get_geometry(scene.world, arc);
    int ok = (!solved &&
              arc_geom &&
              vec3_exact_eq(arc_geom->data.arc.center, center_before) &&
              fabsf(arc_geom->data.arc.start_angle - start_before) <= 1e-6f &&
              fabsf(arc_geom->data.arc.end_angle - end_before) <= 1e-6f &&
              vec3_exact_eq(arc_geom->data.arc.normal, normal_before) &&
              imp && imp->active &&
              imp->first_constraint == c &&
              strcmp(imp->reason, "Unsatisfied arc endpoint-angle constraint.") == 0);
    ecs_world_shutdown(&world);
    return ok ? 0 : 1;
}


typedef int (*test_fn_t)(void);
typedef struct { const char *name; test_fn_t fn; } test_case_t;

int main(void) {
    static const test_case_t tests[] = {
        { "test_recalculate_updates_solver_counters", test_recalculate_updates_solver_counters },
        { "test_sketch_solver_defaults_and_manual_recalc_clears_queue_metadata",
          test_sketch_solver_defaults_and_manual_recalc_clears_queue_metadata },
        { "test_debounce_setter_clamps_range_and_preserves_zero",
          test_debounce_setter_clamps_range_and_preserves_zero },
        { "test_recalculate_commits_coincident_solution_for_points", test_recalculate_commits_coincident_solution_for_points },
        { "test_recalculate_unsat_is_transactional_and_deterministic", test_recalculate_unsat_is_transactional_and_deterministic },
        { "test_participant_descriptor_supports_sub_entity_contract", test_participant_descriptor_supports_sub_entity_contract },
        { "test_scene_constraint_creation_accepts_directional_descriptor_participants_D01_D06",
          test_scene_constraint_creation_accepts_directional_descriptor_participants_D01_D06 },
        { "test_scene_constraint_creation_rejects_directional_raw_entity_signatures_D03_D06",
          test_scene_constraint_creation_rejects_directional_raw_entity_signatures_D03_D06 },
        { "test_failure_implication_dedupes_participants", test_failure_implication_dedupes_participants },
        { "test_endpoint_replay_undo_redo_triggers_sketch_solver_and_script_side_effects",
          test_endpoint_replay_undo_redo_triggers_sketch_solver_and_script_side_effects },
        { "test_endpoint_replay_non_sketch_owner_does_not_emit_script_revision",
          test_endpoint_replay_non_sketch_owner_does_not_emit_script_revision },
        { "test_recalculate_idempotent_on_unchanged_sketch", test_recalculate_idempotent_on_unchanged_sketch },
        { "test_driving_length_angle_unsat_no_mutation_contract_D09", test_driving_length_angle_unsat_no_mutation_contract_D09 },
        { "test_driving_length_atomic_success_contract_D10", test_driving_length_atomic_success_contract_D10 },
        { "test_driving_angle_between_two_lines_atomic_success_contract_D10",
          test_driving_angle_between_two_lines_atomic_success_contract_D10 },
        { "test_driving_angle_90_degree_default_radians_contract_D10",
          test_driving_angle_90_degree_default_radians_contract_D10 },
        { "test_recalculate_along_x_arc_center_descriptor_contract_D11",
          test_recalculate_along_x_arc_center_descriptor_contract_D11 },
        { "test_recalculate_along_x_unsat_fixed_is_transactional_contract_D11",
          test_recalculate_along_x_unsat_fixed_is_transactional_contract_D11 },
        { "test_recalculate_along_xyz_group_points_contract_D11",
          test_recalculate_along_xyz_group_points_contract_D11 },
        { "test_recalculate_along_y_group_points_contract_D11",
          test_recalculate_along_y_group_points_contract_D11 },
        { "test_recalculate_along_z_group_points_contract_D11",
          test_recalculate_along_z_group_points_contract_D11 },
        { "test_recalculate_along_z_mixed_landmarks_contract_D11",
          test_recalculate_along_z_mixed_landmarks_contract_D11 },
        { "test_recalculate_along_and_angle_coexistence_is_deterministic_D11",
          test_recalculate_along_and_angle_coexistence_is_deterministic_D11 },
        { "test_arci_arc_axis_line_reorients_arc_normal_and_keeps_line_reference_D02_D03",
          test_arci_arc_axis_line_reorients_arc_normal_and_keeps_line_reference_D02_D03 },
        { "test_arci_arc_axis_line_fixed_fixed_unsat_is_transactional_D04",
          test_arci_arc_axis_line_fixed_fixed_unsat_is_transactional_D04 },
        { "test_arci_line_arc_endpoint_tangency_satisfies_coincidence_then_tangent_D06_D07",
          test_arci_line_arc_endpoint_tangency_satisfies_coincidence_then_tangent_D06_D07 },
        { "test_arci_line_arc_endpoint_tangency_unsat_fixed_is_transactional_D08",
          test_arci_line_arc_endpoint_tangency_unsat_fixed_is_transactional_D08 },
        { "test_arci_line_arc_endpoint_tangency_respects_line_endpoint_drag_anchor",
          test_arci_line_arc_endpoint_tangency_respects_line_endpoint_drag_anchor },
        { "test_arci_arc_endpoint_angle_anchors_first_and_moves_second_D10_D11",
          test_arci_arc_endpoint_angle_anchors_first_and_moves_second_D10_D11 },
        { "test_arci_arc_endpoint_angle_unsat_fixed_target_is_transactional_D12",
          test_arci_arc_endpoint_angle_unsat_fixed_target_is_transactional_D12 },
    };

    for (size_t i = 0; i < (sizeof(tests) / sizeof(tests[0])); ++i) {
        if (tests[i].fn() != 0) {
            fprintf(stderr, "FAILED: %s\n", tests[i].name);
            return 1;
        }
    }
    return 0;
}
