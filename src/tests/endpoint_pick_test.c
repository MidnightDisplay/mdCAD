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
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif
void sokol_main(void) {}
#ifdef __cplusplus
}
#endif

static int test_endpoint_pick_id_encode_decode_roundtrip(void) {
    uint32_t entity_pick_id = 42u;
    uint32_t endpoint_a_pick = 0u;
    uint32_t endpoint_b_pick = 0u;

    if (!endpoint_pick_encode(entity_pick_id, CONSTRAINT_PARTICIPANT_ROLE_POINT_A, &endpoint_a_pick)) {
        return 1;
    }
    if (!endpoint_pick_encode(entity_pick_id, CONSTRAINT_PARTICIPANT_ROLE_POINT_B, &endpoint_b_pick)) {
        return 1;
    }
    if (endpoint_a_pick == endpoint_b_pick) {
        return 1;
    }
    if (!endpoint_pick_is_encoded(endpoint_a_pick) || !endpoint_pick_is_encoded(endpoint_b_pick)) {
        return 1;
    }

    uint32_t decoded_entity_pick = 0u;
    constraint_participant_role_t decoded_role = CONSTRAINT_PARTICIPANT_ROLE_UNSPECIFIED;
    if (!endpoint_pick_decode(endpoint_a_pick, &decoded_entity_pick, &decoded_role)) {
        return 1;
    }
    if (decoded_entity_pick != entity_pick_id) {
        return 1;
    }
    if (decoded_role != CONSTRAINT_PARTICIPANT_ROLE_POINT_A) {
        return 1;
    }
    return 0;
}

static int test_endpoint_pick_native_endpoint_entity_resolution(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(0.2f, 0.7f, 1.0f, 1.0f));
    if (sketch == 0) return 1;

    ecs_entity_t line = scene_add_line_to_sketch(
        &scene, sketch, vec3_make(0.0f, 0.0f, 0.0f), vec3_make(1.0f, 0.0f, 0.0f),
        vec4_make(1.0f, 1.0f, 1.0f, 1.0f), 1.0f);
    if (line == 0) return 1;

    EndPointsComp *line_endpoints = ecs_world_get_endpoints(&world, line);
    if (!line_endpoints || line_endpoints->is_endpoint_point || line_endpoints->endpoint_count != 2) {
        return 1;
    }

    endpoint_binding_t endpoint_a = {0};
    if (!endpoints_comp_find_binding(line_endpoints, CONSTRAINT_PARTICIPANT_ROLE_POINT_A, &endpoint_a)) {
        return 1;
    }
    ecs_entity_t endpoint_entity = (ecs_entity_t)endpoint_a.endpoint_entity;
    if (endpoint_entity == 0) return 1;

    SelectableComp *endpoint_sel = ecs_world_get_selectable(&world, endpoint_entity);
    if (!endpoint_sel || endpoint_sel->pick_id == 0 || !endpoint_sel->pickable) {
        return 1;
    }

    constraint_participant_role_t role = CONSTRAINT_PARTICIPANT_ROLE_UNSPECIFIED;
    uint8_t sub_index = 255u;
    ecs_entity_t owner = scene_constraint_participant_entity_for_pick(&scene, endpoint_sel->pick_id, &role, &sub_index);
    if (owner != line) return 1;
    if (role != CONSTRAINT_PARTICIPANT_ROLE_POINT_A) return 1;
    if (sub_index != endpoint_a.sub_index) return 1;

    ecs_scene_shutdown(&scene);
    ecs_world_shutdown(&world);
    return 0;
}

static int test_endpoint_pick_non_sketch_line_has_no_native_endpoints(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t line = scene_add_line(
        &scene, vec3_make(0.0f, 0.0f, 0.0f), vec3_make(1.0f, 0.0f, 0.0f),
        vec4_make(1.0f, 1.0f, 1.0f, 1.0f), 1.0f);
    if (line == 0) return 1;

    EndPointsComp *line_endpoints = ecs_world_get_endpoints(&world, line);
    if (line_endpoints != NULL) return 1;

    ecs_scene_shutdown(&scene);
    ecs_world_shutdown(&world);
    return 0;
}

static int test_endpoint_pick_transform_delta_non_sketch_line_does_not_crash(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t line = scene_add_line(
        &scene, vec3_make(0.0f, 0.0f, 0.0f), vec3_make(1.0f, 0.0f, 0.0f),
        vec4_make(1.0f, 1.0f, 1.0f, 1.0f), 1.0f);
    if (line == 0) return 1;

    GeometryComp *before = ecs_world_get_geometry(&world, line);
    if (!before || before->type != GEOM_LINE) return 1;
    vec3_t before_a = before->data.line.a;
    vec3_t before_b = before->data.line.b;

    ecs_entity_t selected[1] = { line };
    if (!scene_apply_transform_delta_for_selection(&scene, selected, 1, vec3_make(0.25f, 0.5f, 0.0f))) return 1;

    TransformComp *xform = ecs_world_get_transform(&world, line);
    if (!xform) return 1;
    if (fabsf(xform->position.x - 0.25f) > 1e-6f || fabsf(xform->position.y - 0.5f) > 1e-6f) return 1;

    GeometryComp *after = ecs_world_get_geometry(&world, line);
    if (!after || after->type != GEOM_LINE) return 1;
    if (fabsf(after->data.line.a.x - before_a.x) > 1e-6f ||
        fabsf(after->data.line.a.y - before_a.y) > 1e-6f ||
        fabsf(after->data.line.a.z - before_a.z) > 1e-6f) return 1;
    if (fabsf(after->data.line.b.x - before_b.x) > 1e-6f ||
        fabsf(after->data.line.b.y - before_b.y) > 1e-6f ||
        fabsf(after->data.line.b.z - before_b.z) > 1e-6f) return 1;

    EndPointsComp *line_endpoints = ecs_world_get_endpoints(&world, line);
    if (line_endpoints != NULL) return 1;

    ecs_scene_shutdown(&scene);
    ecs_world_shutdown(&world);
    return 0;
}

static int test_endpoint_pick_overlay_precedence_contract(void) {
    pick_buffer_layer_event_t events[8] = {0};
    int event_count = 0;
    pick_buffer_layer_record_line(events, 8, &event_count, 10u);
    pick_buffer_layer_record_line(events, 8, &event_count, 11u);
    pick_buffer_layer_record_overlay_point(events, 8, &event_count, 100u);
    pick_buffer_layer_record_overlay_point(events, 8, &event_count, 101u);
    if (event_count != 4) return 1;

    if (events[0].layer != PICK_BUFFER_LAYER_BASE || events[1].layer != PICK_BUFFER_LAYER_BASE) return 1;
    if (events[2].layer != PICK_BUFFER_LAYER_OVERLAY || events[3].layer != PICK_BUFFER_LAYER_OVERLAY) return 1;
    if (events[2].pick_id != 100u || events[3].pick_id != 101u) return 1;
    return 0;
}

static int test_endpoint_pick_collision_prefers_overlay(void) {
    pick_buffer_layer_event_t events[4] = {0};
    int event_count = 0;
    pick_buffer_layer_record_line(events, 4, &event_count, 12u);
    pick_buffer_layer_record_line(events, 4, &event_count, 13u);
    pick_buffer_layer_record_overlay_point(events, 4, &event_count, 200u);
    if (pick_buffer_resolve_preferred_pick(events, event_count) != 200u) {
        return 1;
    }
    return 0;
}

static int test_endpoint_pick_collision_deterministic_repeated_sampling(void) {
    pick_buffer_layer_event_t events[6] = {0};
    int event_count = 0;
    pick_buffer_layer_record_line(events, 6, &event_count, 50u);
    pick_buffer_layer_record_overlay_point(events, 6, &event_count, 250u);
    pick_buffer_layer_record_overlay_point(events, 6, &event_count, 251u);

    uint32_t first = pick_buffer_resolve_preferred_pick(events, event_count);
    for (int i = 0; i < 16; i++) {
        if (pick_buffer_resolve_preferred_pick(events, event_count) != first) {
            return 1;
        }
    }
    if (first != 251u) {
        return 1;
    }
    return 0;
}

static int test_endpoint_point_context_filters_line_only_constraints(void) {
    constraint_selection_signature_t sig = {0};
    sig.count = 1;
    sig.geometry_types[0] = GEOM_LINE;
    sig.roles[0] = CONSTRAINT_PARTICIPANT_ROLE_POINT_A;

    if (constraint_type_is_selection_legal(&sig, CONSTRAINT_FIXED)) return 1;
    if (constraint_type_is_selection_legal(&sig, CONSTRAINT_ALONG_X)) return 1;
    if (constraint_type_is_selection_legal(&sig, CONSTRAINT_ALONG_Y)) return 1;
    if (constraint_type_is_selection_legal(&sig, CONSTRAINT_ALONG_Z)) return 1;
    if (constraint_type_is_selection_legal(&sig, CONSTRAINT_LENGTH)) return 1;
    return 0;
}

static int test_endpoint_point_context_keeps_coincident_for_endpoint_pairs(void) {
    constraint_selection_signature_t sig = {0};
    sig.count = 2;
    sig.geometry_types[0] = GEOM_LINE;
    sig.roles[0] = CONSTRAINT_PARTICIPANT_ROLE_POINT_A;
    sig.geometry_types[1] = GEOM_ARC;
    sig.roles[1] = CONSTRAINT_PARTICIPANT_ROLE_POINT_B;

    if (!constraint_type_is_selection_legal(&sig, CONSTRAINT_COINCIDENT)) return 1;
    if (constraint_type_is_selection_legal(&sig, CONSTRAINT_PARALLEL)) return 1;
    if (constraint_type_is_selection_legal(&sig, CONSTRAINT_ANGLE)) return 1;
    return 0;
}

static int test_endpoint_line_endpoint_to_owner_bidirectional_sync(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(0.2f, 0.7f, 1.0f, 1.0f));
    if (sketch == 0) return 1;
    ecs_entity_t line = scene_add_line_to_sketch(
        &scene, sketch, vec3_make(0.0f, 0.0f, 0.0f), vec3_make(1.0f, 0.0f, 0.0f),
        vec4_make(1.0f, 1.0f, 1.0f, 1.0f), 1.0f);
    if (line == 0) return 1;

    EndPointsComp *line_endpoints = ecs_world_get_endpoints(&world, line);
    if (!line_endpoints) return 1;
    endpoint_binding_t binding_a = {0};
    if (!endpoints_comp_find_binding(line_endpoints, CONSTRAINT_PARTICIPANT_ROLE_POINT_A, &binding_a)) return 1;
    ecs_entity_t endpoint_a = (ecs_entity_t)binding_a.endpoint_entity;
    if (endpoint_a == 0) return 1;

    ecs_entity_t selected[1] = { endpoint_a };
    if (!scene_apply_transform_delta_for_selection(&scene, selected, 1, vec3_make(0.5f, 0.0f, 0.0f))) return 1;

    GeometryComp *line_geom = ecs_world_get_geometry(&world, line);
    if (!line_geom || line_geom->type != GEOM_LINE) return 1;
    if (fabsf(line_geom->data.line.a.x - 0.5f) > 1e-5f) return 1;

    line_geom->data.line.b = vec3_make(2.0f, 0.0f, 0.0f);
    scene_sync_endpoint_entities_for_owner(&scene, line);
    line_endpoints = ecs_world_get_endpoints(&world, line);
    endpoint_binding_t binding_b = {0};
    if (!line_endpoints || !endpoints_comp_find_binding(line_endpoints, CONSTRAINT_PARTICIPANT_ROLE_POINT_B, &binding_b)) return 1;
    GeometryComp *endpoint_b_geom = ecs_world_get_geometry(&world, (ecs_entity_t)binding_b.endpoint_entity);
    if (!endpoint_b_geom || endpoint_b_geom->type != GEOM_POINT) return 1;
    if (fabsf(endpoint_b_geom->data.point.point.x - 2.0f) > 1e-5f) return 1;

    ecs_scene_shutdown(&scene);
    ecs_world_shutdown(&world);
    return 0;
}

static int test_endpoint_arc_endpoint_center_to_owner_bidirectional_sync(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(0.2f, 0.7f, 1.0f, 1.0f));
    if (sketch == 0) return 1;
    ecs_entity_t arc = scene_add_arc_to_sketch(
        &scene, sketch, vec3_make(0.0f, 0.0f, 0.0f), 1.0f, 0.0f, 1.5707963f, vec3_make(0.0f, 0.0f, 1.0f),
        vec4_make(1.0f, 1.0f, 1.0f, 1.0f), 1.0f);
    if (arc == 0) return 1;

    EndPointsComp *arc_endpoints = ecs_world_get_endpoints(&world, arc);
    if (!arc_endpoints) return 1;
    endpoint_binding_t center_binding = {0};
    if (!endpoints_comp_find_binding(arc_endpoints, CONSTRAINT_PARTICIPANT_ROLE_CENTER, &center_binding)) return 1;

    ecs_entity_t selected_center[1] = { (ecs_entity_t)center_binding.endpoint_entity };
    if (!scene_apply_transform_delta_for_selection(&scene, selected_center, 1, vec3_make(1.0f, 0.0f, 0.0f))) return 1;

    GeometryComp *arc_geom = ecs_world_get_geometry(&world, arc);
    if (!arc_geom || arc_geom->type != GEOM_ARC) return 1;
    if (fabsf(arc_geom->data.arc.center.x - 1.0f) > 1e-5f) return 1;

    arc_geom->data.arc.start_angle = 3.1415926f;
    scene_sync_endpoint_entities_for_owner(&scene, arc);
    arc_endpoints = ecs_world_get_endpoints(&world, arc);
    endpoint_binding_t start_binding = {0};
    if (!arc_endpoints || !endpoints_comp_find_binding(arc_endpoints, CONSTRAINT_PARTICIPANT_ROLE_POINT_A, &start_binding)) return 1;
    GeometryComp *start_geom = ecs_world_get_geometry(&world, (ecs_entity_t)start_binding.endpoint_entity);
    if (!start_geom || start_geom->type != GEOM_POINT) return 1;
    if (fabsf(start_geom->data.point.point.x - 0.0f) > 1e-4f) return 1;

    ecs_scene_shutdown(&scene);
    ecs_world_shutdown(&world);
    return 0;
}

static int test_endpoint_direct_geometry_edit_syncs_owner_and_entities(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(0.2f, 0.7f, 1.0f, 1.0f));
    if (sketch == 0) return 1;
    ecs_entity_t line = scene_add_line_to_sketch(
        &scene, sketch, vec3_make(0.0f, 0.0f, 0.0f), vec3_make(1.0f, 0.0f, 0.0f),
        vec4_make(1.0f, 1.0f, 1.0f, 1.0f), 1.0f);
    if (line == 0) return 1;

    EndPointsComp *line_endpoints = ecs_world_get_endpoints(&world, line);
    endpoint_binding_t binding_a = {0};
    if (!line_endpoints || !endpoints_comp_find_binding(line_endpoints, CONSTRAINT_PARTICIPANT_ROLE_POINT_A, &binding_a)) return 1;

    ecs_entity_t endpoint_a = (ecs_entity_t)binding_a.endpoint_entity;
    GeometryComp *endpoint_geom = ecs_world_get_geometry(&world, endpoint_a);
    if (!endpoint_geom || endpoint_geom->type != GEOM_POINT) return 1;

    endpoint_geom->data.point.point = vec3_make(3.0f, 4.0f, 0.0f);
    if (!scene_sync_owner_geometry_from_endpoint_entity(&scene, endpoint_a)) return 1;

    GeometryComp *line_geom = ecs_world_get_geometry(&world, line);
    if (!line_geom || line_geom->type != GEOM_LINE) return 1;
    if (fabsf(line_geom->data.line.a.x - 3.0f) > 1e-5f ||
        fabsf(line_geom->data.line.a.y - 4.0f) > 1e-5f) return 1;

    line_geom->data.line.b = vec3_make(7.0f, 0.0f, 0.0f);
    scene_sync_endpoint_entities_for_owner(&scene, line);
    line_endpoints = ecs_world_get_endpoints(&world, line);
    endpoint_binding_t binding_b = {0};
    if (!line_endpoints || !endpoints_comp_find_binding(line_endpoints, CONSTRAINT_PARTICIPANT_ROLE_POINT_B, &binding_b)) return 1;
    GeometryComp *endpoint_b_geom = ecs_world_get_geometry(&world, (ecs_entity_t)binding_b.endpoint_entity);
    if (!endpoint_b_geom || endpoint_b_geom->type != GEOM_POINT) return 1;
    if (fabsf(endpoint_b_geom->data.point.point.x - 7.0f) > 1e-5f) return 1;

    ecs_scene_shutdown(&scene);
    ecs_world_shutdown(&world);
    return 0;
}

typedef int (*test_fn_t)(void);
typedef struct { const char *name; test_fn_t fn; } test_case_t;

int main(void) {
    static const test_case_t tests[] = {
        { "test_endpoint_pick_id_encode_decode_roundtrip", test_endpoint_pick_id_encode_decode_roundtrip },
        { "test_endpoint_pick_native_endpoint_entity_resolution", test_endpoint_pick_native_endpoint_entity_resolution },
        { "test_endpoint_pick_non_sketch_line_has_no_native_endpoints", test_endpoint_pick_non_sketch_line_has_no_native_endpoints },
        { "test_endpoint_pick_transform_delta_non_sketch_line_does_not_crash", test_endpoint_pick_transform_delta_non_sketch_line_does_not_crash },
        { "test_endpoint_pick_overlay_precedence_contract", test_endpoint_pick_overlay_precedence_contract },
        { "test_endpoint_pick_collision_prefers_overlay", test_endpoint_pick_collision_prefers_overlay },
        { "test_endpoint_pick_collision_deterministic_repeated_sampling", test_endpoint_pick_collision_deterministic_repeated_sampling },
        { "test_endpoint_point_context_filters_line_only_constraints", test_endpoint_point_context_filters_line_only_constraints },
        { "test_endpoint_point_context_keeps_coincident_for_endpoint_pairs", test_endpoint_point_context_keeps_coincident_for_endpoint_pairs },
        { "test_endpoint_line_endpoint_to_owner_bidirectional_sync", test_endpoint_line_endpoint_to_owner_bidirectional_sync },
        { "test_endpoint_arc_endpoint_center_to_owner_bidirectional_sync", test_endpoint_arc_endpoint_center_to_owner_bidirectional_sync },
        { "test_endpoint_direct_geometry_edit_syncs_owner_and_entities", test_endpoint_direct_geometry_edit_syncs_owner_and_entities },
    };

    for (size_t i = 0; i < (sizeof(tests) / sizeof(tests[0])); ++i) {
        if (tests[i].fn() != 0) {
            fprintf(stderr, "FAILED: %s\n", tests[i].name);
            return 1;
        }
    }
    return 0;
}
