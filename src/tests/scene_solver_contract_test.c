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
              sk->solve_request_serial > after_undo_request &&
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

typedef int (*test_fn_t)(void);
typedef struct { const char *name; test_fn_t fn; } test_case_t;

int main(void) {
    static const test_case_t tests[] = {
        { "test_recalculate_updates_solver_counters", test_recalculate_updates_solver_counters },
        { "test_recalculate_commits_coincident_solution_for_points", test_recalculate_commits_coincident_solution_for_points },
        { "test_recalculate_unsat_is_transactional_and_deterministic", test_recalculate_unsat_is_transactional_and_deterministic },
        { "test_participant_descriptor_supports_sub_entity_contract", test_participant_descriptor_supports_sub_entity_contract },
        { "test_failure_implication_dedupes_participants", test_failure_implication_dedupes_participants },
        { "test_endpoint_replay_undo_redo_triggers_sketch_solver_and_script_side_effects",
          test_endpoint_replay_undo_redo_triggers_sketch_solver_and_script_side_effects },
        { "test_endpoint_replay_non_sketch_owner_does_not_emit_script_revision",
          test_endpoint_replay_non_sketch_owner_does_not_emit_script_revision },
    };

    for (size_t i = 0; i < (sizeof(tests) / sizeof(tests[0])); ++i) {
        if (tests[i].fn() != 0) {
            fprintf(stderr, "FAILED: %s\n", tests[i].name);
            return 1;
        }
    }
    return 0;
}
