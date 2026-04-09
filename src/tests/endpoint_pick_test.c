#include "../ecs/ecs_world.h"
#include "../ecs/ecs_scene.h"
#include "../scene_serializer.h"
#include "../scripting/sketch_script_runtime.h"
#include "../scripting/sketch_script_contract.h"
#include "../scripting/sketch_script_parse.h"
#include "../scripting/sketch_script_apply.h"
#include "../scripting/sketch_script_emit.h"
#include "../undo_redo_exec.h"
#include "../gizmo/gizmo.h"
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

static bool test_vec3_close(vec3_t a, vec3_t b, float eps) {
    return fabsf(a.x - b.x) <= eps &&
           fabsf(a.y - b.y) <= eps &&
           fabsf(a.z - b.z) <= eps;
}

static bool test_line_endpoint_entity(ecs_world_state_t *world,
                                      ecs_entity_t line,
                                      constraint_participant_role_t role,
                                      ecs_entity_t *out_endpoint_entity) {
    if (!world || line == 0 || !out_endpoint_entity) return false;
    EndPointsComp *endpoints = ecs_world_get_endpoints(world, line);
    endpoint_binding_t binding = {0};
    if (!endpoints || !endpoints_comp_find_binding(endpoints, role, &binding)) return false;
    ecs_entity_t endpoint = (ecs_entity_t)binding.endpoint_entity;
    if (endpoint == 0 || !ecs_is_alive(world->world, endpoint)) return false;
    *out_endpoint_entity = endpoint;
    return true;
}

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

static int test_geometry_manager_selection_sync_maps_viewport_selection_to_rows(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(0.2f, 0.7f, 1.0f, 1.0f));
    ecs_entity_t line = scene_add_line_to_sketch(
        &scene, sketch, vec3_make(0.0f, 0.0f, 0.0f), vec3_make(1.0f, 0.0f, 0.0f),
        vec4_make(1.0f, 1.0f, 1.0f, 1.0f), 1.0f);
    ecs_entity_t non_sketch_line = scene_add_line(
        &scene, vec3_make(2.0f, 0.0f, 0.0f), vec3_make(3.0f, 0.0f, 0.0f),
        vec4_make(1.0f, 1.0f, 1.0f, 1.0f), 1.0f);
    if (!sketch || !line || !non_sketch_line) return 1;

    EndPointsComp *line_endpoints = ecs_world_get_endpoints(&world, line);
    endpoint_binding_t binding_a = {0};
    if (!line_endpoints || !endpoints_comp_find_binding(line_endpoints, CONSTRAINT_PARTICIPANT_ROLE_POINT_A, &binding_a)) {
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }
    ecs_entity_t endpoint_a = (ecs_entity_t)binding_a.endpoint_entity;
    if (endpoint_a == 0) {
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }

    selection_buffer_t selection = {0};
    selection_init(&selection, &world);
    selection_add(&selection, line);
    selection_add(&selection, endpoint_a);   // selecting endpoint in viewport should map to owner line row
    selection_add(&selection, non_sketch_line);

    ecs_entity_t row_selection[UI_GEOMETRY_MANAGER_MAX_ROWS] = {0};
    int row_selection_count = 0;
    ui_geometry_manager_sync_selection_from_scene(&world, &selection, sketch,
                                                  row_selection, &row_selection_count);

    int line_hits = 0;
    int non_sketch_hits = 0;
    for (int i = 0; i < row_selection_count; i++) {
        if (row_selection[i] == line) line_hits++;
        if (row_selection[i] == non_sketch_line) non_sketch_hits++;
    }

    selection_shutdown(&selection);
    ecs_scene_shutdown(&scene);
    ecs_world_shutdown(&world);

    if (line_hits != 1) return 1;
    if (non_sketch_hits != 0) return 1;
    return 0;
}

static int test_selection_prune_dead_removes_deleted_selection_entries(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t line = scene_add_line(
        &scene, vec3_make(0.0f, 0.0f, 0.0f), vec3_make(1.0f, 0.0f, 0.0f),
        vec4_make(1.0f, 1.0f, 1.0f, 1.0f), 1.0f);
    if (!line) return 1;

    selection_buffer_t selection = {0};
    selection_init(&selection, &world);
    selection_add(&selection, line);
    if (selection_count(&selection) != 1) {
        selection_shutdown(&selection);
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }

    scene_remove_entity(&scene, line);
    selection_prune_dead(&selection);
    int ok = (selection_count(&selection) == 0);

    selection_shutdown(&selection);
    ecs_scene_shutdown(&scene);
    ecs_world_shutdown(&world);
    return ok ? 0 : 1;
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

static int test_directional_legality_accepts_point_pair_group_signatures_D01_D04(void) {
    constraint_selection_signature_t pair_sig = {0};
    pair_sig.count = 2;
    pair_sig.geometry_types[0] = GEOM_POINT;
    pair_sig.roles[0] = CONSTRAINT_PARTICIPANT_ROLE_ENTITY;
    pair_sig.geometry_types[1] = GEOM_POINT;
    pair_sig.roles[1] = CONSTRAINT_PARTICIPANT_ROLE_ENTITY;

    if (!constraint_type_is_selection_legal(&pair_sig, CONSTRAINT_ALONG_X)) return 1;
    if (!constraint_type_is_selection_legal(&pair_sig, CONSTRAINT_ALONG_Y)) return 1;
    if (!constraint_type_is_selection_legal(&pair_sig, CONSTRAINT_ALONG_Z)) return 1;

    constraint_selection_signature_t group_sig = {0};
    group_sig.count = 3;
    group_sig.geometry_types[0] = GEOM_POINT;
    group_sig.roles[0] = CONSTRAINT_PARTICIPANT_ROLE_ENTITY;
    group_sig.geometry_types[1] = GEOM_POINT;
    group_sig.roles[1] = CONSTRAINT_PARTICIPANT_ROLE_ENTITY;
    group_sig.geometry_types[2] = GEOM_POINT;
    group_sig.roles[2] = CONSTRAINT_PARTICIPANT_ROLE_ENTITY;

    if (!constraint_type_is_selection_legal(&group_sig, CONSTRAINT_ALONG_X)) return 1;
    if (!constraint_type_is_selection_legal(&group_sig, CONSTRAINT_ALONG_Y)) return 1;
    if (!constraint_type_is_selection_legal(&group_sig, CONSTRAINT_ALONG_Z)) return 1;
    return 0;
}

static int test_directional_legality_accepts_line_and_arc_landmark_roles_D02_D05(void) {
    constraint_selection_signature_t line_arc_sig = {0};
    line_arc_sig.count = 3;
    line_arc_sig.geometry_types[0] = GEOM_LINE;
    line_arc_sig.roles[0] = CONSTRAINT_PARTICIPANT_ROLE_POINT_A;
    line_arc_sig.geometry_types[1] = GEOM_LINE;
    line_arc_sig.roles[1] = CONSTRAINT_PARTICIPANT_ROLE_POINT_B;
    line_arc_sig.geometry_types[2] = GEOM_ARC;
    line_arc_sig.roles[2] = CONSTRAINT_PARTICIPANT_ROLE_CENTER;

    if (!constraint_type_is_selection_legal(&line_arc_sig, CONSTRAINT_ALONG_X)) return 1;
    if (!constraint_type_is_selection_legal(&line_arc_sig, CONSTRAINT_ALONG_Y)) return 1;
    if (!constraint_type_is_selection_legal(&line_arc_sig, CONSTRAINT_ALONG_Z)) return 1;

    constraint_selection_signature_t mixed_sig = {0};
    mixed_sig.count = 3;
    mixed_sig.geometry_types[0] = GEOM_POINT;
    mixed_sig.roles[0] = CONSTRAINT_PARTICIPANT_ROLE_ENTITY;
    mixed_sig.geometry_types[1] = GEOM_LINE;
    mixed_sig.roles[1] = CONSTRAINT_PARTICIPANT_ROLE_POINT_A;
    mixed_sig.geometry_types[2] = GEOM_ARC;
    mixed_sig.roles[2] = CONSTRAINT_PARTICIPANT_ROLE_POINT_B;

    if (!constraint_type_is_selection_legal(&mixed_sig, CONSTRAINT_ALONG_X)) return 1;
    if (!constraint_type_is_selection_legal(&mixed_sig, CONSTRAINT_ALONG_Y)) return 1;
    if (!constraint_type_is_selection_legal(&mixed_sig, CONSTRAINT_ALONG_Z)) return 1;
    return 0;
}

static int test_directional_legality_rejects_raw_line_and_arc_entity_signatures_D03_D06(void) {
    constraint_selection_signature_t raw_line_pair = {0};
    raw_line_pair.count = 2;
    raw_line_pair.geometry_types[0] = GEOM_LINE;
    raw_line_pair.roles[0] = CONSTRAINT_PARTICIPANT_ROLE_ENTITY;
    raw_line_pair.geometry_types[1] = GEOM_LINE;
    raw_line_pair.roles[1] = CONSTRAINT_PARTICIPANT_ROLE_ENTITY;

    if (constraint_type_is_selection_legal(&raw_line_pair, CONSTRAINT_ALONG_X)) return 1;
    if (constraint_type_is_selection_legal(&raw_line_pair, CONSTRAINT_ALONG_Y)) return 1;
    if (constraint_type_is_selection_legal(&raw_line_pair, CONSTRAINT_ALONG_Z)) return 1;

    constraint_selection_signature_t raw_arc_pair = {0};
    raw_arc_pair.count = 2;
    raw_arc_pair.geometry_types[0] = GEOM_ARC;
    raw_arc_pair.roles[0] = CONSTRAINT_PARTICIPANT_ROLE_ENTITY;
    raw_arc_pair.geometry_types[1] = GEOM_ARC;
    raw_arc_pair.roles[1] = CONSTRAINT_PARTICIPANT_ROLE_ENTITY;

    if (constraint_type_is_selection_legal(&raw_arc_pair, CONSTRAINT_ALONG_X)) return 1;
    if (constraint_type_is_selection_legal(&raw_arc_pair, CONSTRAINT_ALONG_Y)) return 1;
    if (constraint_type_is_selection_legal(&raw_arc_pair, CONSTRAINT_ALONG_Z)) return 1;

    constraint_selection_signature_t legacy_line = {0};
    legacy_line.count = 1;
    legacy_line.geometry_types[0] = GEOM_LINE;
    legacy_line.roles[0] = CONSTRAINT_PARTICIPANT_ROLE_ENTITY;
    if (!constraint_type_is_selection_legal(&legacy_line, CONSTRAINT_ALONG_X)) return 1;
    if (!constraint_type_is_selection_legal(&legacy_line, CONSTRAINT_ALONG_Y)) return 1;
    if (!constraint_type_is_selection_legal(&legacy_line, CONSTRAINT_ALONG_Z)) return 1;

    return 0;
}

static int test_directional_legality_keeps_legacy_single_line_entity_signature_parity_alin01_alin03(void) {
    constraint_selection_signature_t legacy_line = {0};
    legacy_line.count = 1;
    legacy_line.geometry_types[0] = GEOM_LINE;
    legacy_line.roles[0] = CONSTRAINT_PARTICIPANT_ROLE_ENTITY;

    if (!constraint_type_is_selection_legal(&legacy_line, CONSTRAINT_ALONG_X)) return 1;
    if (!constraint_type_is_selection_legal(&legacy_line, CONSTRAINT_ALONG_Y)) return 1;
    if (!constraint_type_is_selection_legal(&legacy_line, CONSTRAINT_ALONG_Z)) return 1;
    return 0;
}

static int test_arci01_legality_accepts_only_line_arc_entity_pair_D01(void) {
    constraint_selection_signature_t legal_sig = {0};
    legal_sig.count = 2;
    legal_sig.geometry_types[0] = GEOM_LINE;
    legal_sig.roles[0] = CONSTRAINT_PARTICIPANT_ROLE_ENTITY;
    legal_sig.geometry_types[1] = GEOM_ARC;
    legal_sig.roles[1] = CONSTRAINT_PARTICIPANT_ROLE_ENTITY;
    legal_sig.entities[0] = 1001;
    legal_sig.entities[1] = 1002;
    if (!constraint_type_is_selection_legal(&legal_sig, CONSTRAINT_ARC_AXIS_LINE)) return 1;

    constraint_selection_signature_t illegal_endpoint_sig = legal_sig;
    illegal_endpoint_sig.roles[0] = CONSTRAINT_PARTICIPANT_ROLE_POINT_A;
    if (constraint_type_is_selection_legal(&illegal_endpoint_sig, CONSTRAINT_ARC_AXIS_LINE)) return 1;

    constraint_selection_signature_t illegal_arc_arc = legal_sig;
    illegal_arc_arc.geometry_types[0] = GEOM_ARC;
    if (constraint_type_is_selection_legal(&illegal_arc_arc, CONSTRAINT_ARC_AXIS_LINE)) return 1;

    return 0;
}

static int test_arci02_legality_accepts_only_line_endpoint_and_arc_endpoint_D05(void) {
    constraint_selection_signature_t legal_sig = {0};
    legal_sig.count = 2;
    legal_sig.geometry_types[0] = GEOM_LINE;
    legal_sig.roles[0] = CONSTRAINT_PARTICIPANT_ROLE_POINT_A;
    legal_sig.geometry_types[1] = GEOM_ARC;
    legal_sig.roles[1] = CONSTRAINT_PARTICIPANT_ROLE_POINT_B;
    legal_sig.entities[0] = 2001;
    legal_sig.entities[1] = 2002;
    if (!constraint_type_is_selection_legal(&legal_sig, CONSTRAINT_LINE_ARC_ENDPOINT_TANGENCY)) return 1;

    constraint_selection_signature_t swapped_legal = legal_sig;
    swapped_legal.geometry_types[0] = GEOM_ARC;
    swapped_legal.roles[0] = CONSTRAINT_PARTICIPANT_ROLE_POINT_A;
    swapped_legal.geometry_types[1] = GEOM_LINE;
    swapped_legal.roles[1] = CONSTRAINT_PARTICIPANT_ROLE_POINT_B;
    if (!constraint_type_is_selection_legal(&swapped_legal, CONSTRAINT_LINE_ARC_ENDPOINT_TANGENCY)) return 1;

    constraint_selection_signature_t illegal_entity = legal_sig;
    illegal_entity.roles[0] = CONSTRAINT_PARTICIPANT_ROLE_ENTITY;
    if (constraint_type_is_selection_legal(&illegal_entity, CONSTRAINT_LINE_ARC_ENDPOINT_TANGENCY)) return 1;

    constraint_selection_signature_t illegal_center = legal_sig;
    illegal_center.roles[1] = CONSTRAINT_PARTICIPANT_ROLE_CENTER;
    if (constraint_type_is_selection_legal(&illegal_center, CONSTRAINT_LINE_ARC_ENDPOINT_TANGENCY)) return 1;

    return 0;
}

static int test_arci03_legality_accepts_same_arc_ordered_endpoint_pair_D09(void) {
    constraint_selection_signature_t legal_sig = {0};
    legal_sig.count = 2;
    legal_sig.geometry_types[0] = GEOM_ARC;
    legal_sig.roles[0] = CONSTRAINT_PARTICIPANT_ROLE_POINT_A;
    legal_sig.geometry_types[1] = GEOM_ARC;
    legal_sig.roles[1] = CONSTRAINT_PARTICIPANT_ROLE_POINT_B;
    legal_sig.entities[0] = 3001;
    legal_sig.entities[1] = 3001;
    if (!constraint_type_is_selection_legal(&legal_sig, CONSTRAINT_ARC_ENDPOINT_ANGLE)) return 1;

    constraint_selection_signature_t reversed_sig = legal_sig;
    reversed_sig.roles[0] = CONSTRAINT_PARTICIPANT_ROLE_POINT_B;
    reversed_sig.roles[1] = CONSTRAINT_PARTICIPANT_ROLE_POINT_A;
    if (!constraint_type_is_selection_legal(&reversed_sig, CONSTRAINT_ARC_ENDPOINT_ANGLE)) return 1;

    constraint_selection_signature_t mixed_arc_sig = legal_sig;
    mixed_arc_sig.entities[1] = 3002;
    if (constraint_type_is_selection_legal(&mixed_arc_sig, CONSTRAINT_ARC_ENDPOINT_ANGLE)) return 1;

    constraint_selection_signature_t illegal_center = legal_sig;
    illegal_center.roles[1] = CONSTRAINT_PARTICIPANT_ROLE_CENTER;
    if (constraint_type_is_selection_legal(&illegal_center, CONSTRAINT_ARC_ENDPOINT_ANGLE)) return 1;

    return 0;
}

static int test_line_line_legality_allows_pair_parallel_and_perpendicular_lcon05(void) {
    constraint_selection_signature_t sig = {0};
    sig.count = 2;
    sig.geometry_types[0] = GEOM_LINE;
    sig.roles[0] = CONSTRAINT_PARTICIPANT_ROLE_ENTITY;
    sig.entities[0] = 1101;
    sig.geometry_types[1] = GEOM_LINE;
    sig.roles[1] = CONSTRAINT_PARTICIPANT_ROLE_ENTITY;
    sig.entities[1] = 1102;

    if (!constraint_type_is_selection_legal(&sig, CONSTRAINT_PARALLEL)) return 1;
    if (!constraint_type_is_selection_legal(&sig, CONSTRAINT_PERPENDICULAR)) return 1;
    return 0;
}

static int test_line_line_legality_rejects_point_roles_for_parallel_and_perpendicular_lcon05(void) {
    constraint_selection_signature_t endpoint_sig = {0};
    endpoint_sig.count = 2;
    endpoint_sig.geometry_types[0] = GEOM_LINE;
    endpoint_sig.roles[0] = CONSTRAINT_PARTICIPANT_ROLE_POINT_A;
    endpoint_sig.entities[0] = 1201;
    endpoint_sig.geometry_types[1] = GEOM_LINE;
    endpoint_sig.roles[1] = CONSTRAINT_PARTICIPANT_ROLE_POINT_B;
    endpoint_sig.entities[1] = 1202;

    if (constraint_type_is_selection_legal(&endpoint_sig, CONSTRAINT_PARALLEL)) return 1;
    if (constraint_type_is_selection_legal(&endpoint_sig, CONSTRAINT_PERPENDICULAR)) return 1;
    return 0;
}

static int test_line_line_legality_allows_group_parallel_and_perpendicular_lcon04(void) {
    constraint_selection_signature_t sig = {0};
    sig.count = 3;
    sig.geometry_types[0] = GEOM_LINE;
    sig.roles[0] = CONSTRAINT_PARTICIPANT_ROLE_ENTITY;
    sig.entities[0] = 1301;
    sig.geometry_types[1] = GEOM_LINE;
    sig.roles[1] = CONSTRAINT_PARTICIPANT_ROLE_ENTITY;
    sig.entities[1] = 1302;
    sig.geometry_types[2] = GEOM_LINE;
    sig.roles[2] = CONSTRAINT_PARTICIPANT_ROLE_ENTITY;
    sig.entities[2] = 1303;

    if (!constraint_type_is_selection_legal(&sig, CONSTRAINT_PARALLEL)) return 1;
    if (!constraint_type_is_selection_legal(&sig, CONSTRAINT_PERPENDICULAR)) return 1;
    return 0;
}

static int test_line_line_legality_rejects_group_perpendicular_with_non_line_participant_lcon05(void) {
    constraint_selection_signature_t sig = {0};
    sig.count = 3;
    sig.geometry_types[0] = GEOM_LINE;
    sig.roles[0] = CONSTRAINT_PARTICIPANT_ROLE_ENTITY;
    sig.entities[0] = 1401;
    sig.geometry_types[1] = GEOM_LINE;
    sig.roles[1] = CONSTRAINT_PARTICIPANT_ROLE_ENTITY;
    sig.entities[1] = 1402;
    sig.geometry_types[2] = GEOM_ARC;
    sig.roles[2] = CONSTRAINT_PARTICIPANT_ROLE_ENTITY;
    sig.entities[2] = 1403;

    if (constraint_type_is_selection_legal(&sig, CONSTRAINT_PERPENDICULAR)) return 1;
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

static int test_arc_slot_reallocation_for_expanded_span(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t arc = scene_add_arc(
        &scene, vec3_make(0.0f, 0.0f, 0.0f), 1.0f,
        0.0f, 0.2f, vec3_make(0.0f, 0.0f, 1.0f),
        vec4_make(1.0f, 1.0f, 1.0f, 1.0f), 1.0f);
    if (arc == 0) return 1;

    RenderableComp *r = ecs_world_get_renderable(&world, arc);
    GeometryComp *g = ecs_world_get_geometry(&world, arc);
    if (!r || !g || g->type != GEOM_ARC) return 1;
    uint32_t before_segments = r->segment_count;
    uint32_t before_joins = r->join_count;

    g->data.arc.end_angle = 3.2f;
    if (!scene_update_arc_renderable_slots(&scene, arc, r, g)) return 1;
    if (r->segment_count <= before_segments) return 1;
    if (r->join_count <= before_joins) return 1;

    ecs_scene_shutdown(&scene);
    ecs_world_shutdown(&world);
    return 0;
}

static int test_endpoint_undo_command_contract_roundtrip(void) {
    undo_redo_t undo = {0};
    undo_redo_init(&undo, NULL, 8);

    vec3_t old_local = vec3_make(0.0f, 0.0f, 0.0f);
    vec3_t new_local = vec3_make(1.25f, -0.5f, 0.0f);
    undo_cmd_move_endpoint_participant(&undo,
                                       (ecs_entity_t)77,
                                       CONSTRAINT_PARTICIPANT_ROLE_POINT_A,
                                       0u,
                                       old_local,
                                       new_local);

    if (undo.count != 1 || undo.current != 1) {
        undo_redo_shutdown(&undo);
        return 1;
    }

    const undo_command_t *cmd = &undo.commands[0];
    if (cmd->type != CMD_MOVE_ENDPOINT_PARTICIPANT) {
        undo_redo_shutdown(&undo);
        return 1;
    }

    if ((ecs_entity_t)cmd->data.move_endpoint_participant.owner_entity_id != (ecs_entity_t)77) {
        undo_redo_shutdown(&undo);
        return 1;
    }
    if (cmd->data.move_endpoint_participant.role != CONSTRAINT_PARTICIPANT_ROLE_POINT_A) {
        undo_redo_shutdown(&undo);
        return 1;
    }
    if (cmd->data.move_endpoint_participant.sub_index != 0u) {
        undo_redo_shutdown(&undo);
        return 1;
    }
    if (fabsf(cmd->data.move_endpoint_participant.old_local_point.x - old_local.x) > 1e-6f ||
        fabsf(cmd->data.move_endpoint_participant.new_local_point.x - new_local.x) > 1e-6f) {
        undo_redo_shutdown(&undo);
        return 1;
    }

    undo_redo_shutdown(&undo);
    return 0;
}

static int test_undo_legacy_position_and_point_commands_unchanged(void) {
    undo_redo_t undo = {0};
    undo_redo_init(&undo, NULL, 8);

    undo_cmd_set_position(&undo, (ecs_entity_t)10,
                          vec3_make(1.0f, 2.0f, 3.0f),
                          vec3_make(4.0f, 5.0f, 6.0f));
    undo_cmd_set_point_position(&undo, (ecs_entity_t)11,
                                vec3_make(-1.0f, -2.0f, -3.0f),
                                vec3_make(2.0f, 3.0f, 4.0f));

    if (undo.count != 2 || undo.current != 2) {
        undo_redo_shutdown(&undo);
        return 1;
    }
    if (undo.commands[0].type != CMD_SET_POSITION || undo.commands[1].type != CMD_SET_POINT_POSITION) {
        undo_redo_shutdown(&undo);
        return 1;
    }
    if ((ecs_entity_t)undo.commands[0].data.set_vec3.entity_id != (ecs_entity_t)10 ||
        (ecs_entity_t)undo.commands[1].data.set_vec3.entity_id != (ecs_entity_t)11) {
        undo_redo_shutdown(&undo);
        return 1;
    }

    undo_redo_shutdown(&undo);
    return 0;
}

static int test_gizmo_endpoint_drag_records_single_endpoint_undo_at_release(void) {
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
    if (!line_endpoints || !endpoints_comp_find_binding(line_endpoints, CONSTRAINT_PARTICIPANT_ROLE_POINT_A, &binding_a)) {
        return 1;
    }
    ecs_entity_t endpoint_a = (ecs_entity_t)binding_a.endpoint_entity;
    GeometryComp *endpoint_geom = ecs_world_get_geometry(&world, endpoint_a);
    if (!endpoint_geom || endpoint_geom->type != GEOM_POINT) return 1;
    vec3_t old_local = endpoint_geom->data.point.point;

    ecs_entity_t selected[1] = { endpoint_a };
    if (!scene_apply_transform_delta_for_selection(&scene, selected, 1, vec3_make(0.3f, 0.0f, 0.0f))) return 1;
    if (!scene_apply_transform_delta_for_selection(&scene, selected, 1, vec3_make(0.2f, 0.0f, 0.0f))) return 1;

    endpoint_geom = ecs_world_get_geometry(&world, endpoint_a);
    if (!endpoint_geom || endpoint_geom->type != GEOM_POINT) return 1;
    vec3_t new_local = endpoint_geom->data.point.point;

    undo_redo_t undo = {0};
    undo_redo_init(&undo, &scene, 8);
    if (!undo_cmd_record_endpoint_participant_move_if_changed(&undo, &scene, endpoint_a, old_local, new_local)) {
        undo_redo_shutdown(&undo);
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }
    if (undo.count != 1 || undo.commands[0].type != CMD_MOVE_ENDPOINT_PARTICIPANT) {
        undo_redo_shutdown(&undo);
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }

    undo_redo_shutdown(&undo);
    ecs_scene_shutdown(&scene);
    ecs_world_shutdown(&world);
    return 0;
}

static int test_inspector_endpoint_edit_records_only_at_commit_boundary(void) {
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
    if (!line_endpoints || !endpoints_comp_find_binding(line_endpoints, CONSTRAINT_PARTICIPANT_ROLE_POINT_A, &binding_a)) {
        return 1;
    }
    ecs_entity_t endpoint_a = (ecs_entity_t)binding_a.endpoint_entity;
    GeometryComp *endpoint_geom = ecs_world_get_geometry(&world, endpoint_a);
    if (!endpoint_geom || endpoint_geom->type != GEOM_POINT) return 1;
    vec3_t old_local = endpoint_geom->data.point.point;
    vec3_t new_local = vec3_add(old_local, vec3_make(0.4f, -0.1f, 0.0f));

    undo_redo_t undo = {0};
    undo_redo_init(&undo, &scene, 8);
    if (!undo_cmd_record_endpoint_participant_move_if_changed(&undo, &scene, endpoint_a, old_local, new_local)) {
        undo_redo_shutdown(&undo);
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }
    if (undo.count != 1 || undo.commands[0].type != CMD_MOVE_ENDPOINT_PARTICIPANT) {
        undo_redo_shutdown(&undo);
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }

    if (undo_cmd_record_endpoint_participant_move_if_changed(&undo, &scene, endpoint_a, new_local, new_local)) {
        undo_redo_shutdown(&undo);
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }
    if (undo.count != 1) {
        undo_redo_shutdown(&undo);
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }

    undo_redo_shutdown(&undo);
    ecs_scene_shutdown(&scene);
    ecs_world_shutdown(&world);
    return 0;
}

static int test_endpoint_undo_replay_resyncs_owner_and_endpoint_entity(void) {
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
    if (!line_endpoints || !endpoints_comp_find_binding(line_endpoints, CONSTRAINT_PARTICIPANT_ROLE_POINT_A, &binding_a)) {
        return 1;
    }

    ecs_entity_t endpoint_a = (ecs_entity_t)binding_a.endpoint_entity;
    GeometryComp *endpoint_geom = ecs_world_get_geometry(&world, endpoint_a);
    GeometryComp *line_geom = ecs_world_get_geometry(&world, line);
    if (!endpoint_geom || endpoint_geom->type != GEOM_POINT || !line_geom || line_geom->type != GEOM_LINE) return 1;

    vec3_t old_local = endpoint_geom->data.point.point;
    ecs_entity_t selected[1] = { endpoint_a };
    if (!scene_apply_transform_delta_for_selection(&scene, selected, 1, vec3_make(0.6f, 0.2f, 0.0f))) return 1;
    endpoint_geom = ecs_world_get_geometry(&world, endpoint_a);
    if (!endpoint_geom || endpoint_geom->type != GEOM_POINT) return 1;
    vec3_t new_local = endpoint_geom->data.point.point;

    undo_redo_t undo = {0};
    undo_redo_init(&undo, &scene, 8);
    if (!undo_cmd_record_endpoint_participant_move_if_changed(&undo, &scene, endpoint_a, old_local, new_local)) {
        undo_redo_shutdown(&undo);
        return 1;
    }

    if (!undo_redo_undo(&undo)) {
        undo_redo_shutdown(&undo);
        return 1;
    }

    endpoint_geom = ecs_world_get_geometry(&world, endpoint_a);
    line_geom = ecs_world_get_geometry(&world, line);
    if (!endpoint_geom || !line_geom) {
        undo_redo_shutdown(&undo);
        return 1;
    }
    if (fabsf(line_geom->data.line.a.x - old_local.x) > 1e-5f ||
        fabsf(line_geom->data.line.a.y - old_local.y) > 1e-5f ||
        fabsf(endpoint_geom->data.point.point.x - old_local.x) > 1e-5f ||
        fabsf(endpoint_geom->data.point.point.y - old_local.y) > 1e-5f) {
        undo_redo_shutdown(&undo);
        return 1;
    }

    undo_redo_shutdown(&undo);
    ecs_scene_shutdown(&scene);
    ecs_world_shutdown(&world);
    return 0;
}

static int test_endpoint_redo_replay_resyncs_owner_and_endpoint_entity(void) {
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
    if (!line_endpoints || !endpoints_comp_find_binding(line_endpoints, CONSTRAINT_PARTICIPANT_ROLE_POINT_A, &binding_a)) {
        return 1;
    }

    ecs_entity_t endpoint_a = (ecs_entity_t)binding_a.endpoint_entity;
    GeometryComp *endpoint_geom = ecs_world_get_geometry(&world, endpoint_a);
    GeometryComp *line_geom = ecs_world_get_geometry(&world, line);
    if (!endpoint_geom || endpoint_geom->type != GEOM_POINT || !line_geom || line_geom->type != GEOM_LINE) return 1;

    vec3_t old_local = endpoint_geom->data.point.point;
    ecs_entity_t selected[1] = { endpoint_a };
    if (!scene_apply_transform_delta_for_selection(&scene, selected, 1, vec3_make(0.45f, -0.35f, 0.0f))) return 1;
    endpoint_geom = ecs_world_get_geometry(&world, endpoint_a);
    if (!endpoint_geom || endpoint_geom->type != GEOM_POINT) return 1;
    vec3_t new_local = endpoint_geom->data.point.point;

    undo_redo_t undo = {0};
    undo_redo_init(&undo, &scene, 8);
    if (!undo_cmd_record_endpoint_participant_move_if_changed(&undo, &scene, endpoint_a, old_local, new_local)) {
        undo_redo_shutdown(&undo);
        return 1;
    }

    if (!undo_redo_undo(&undo) || !undo_redo_redo(&undo)) {
        undo_redo_shutdown(&undo);
        return 1;
    }

    endpoint_geom = ecs_world_get_geometry(&world, endpoint_a);
    line_geom = ecs_world_get_geometry(&world, line);
    if (!endpoint_geom || !line_geom) {
        undo_redo_shutdown(&undo);
        return 1;
    }
    if (fabsf(line_geom->data.line.a.x - new_local.x) > 1e-5f ||
        fabsf(line_geom->data.line.a.y - new_local.y) > 1e-5f ||
        fabsf(endpoint_geom->data.point.point.x - new_local.x) > 1e-5f ||
        fabsf(endpoint_geom->data.point.point.y - new_local.y) > 1e-5f) {
        undo_redo_shutdown(&undo);
        return 1;
    }

    undo_redo_shutdown(&undo);
    ecs_scene_shutdown(&scene);
    ecs_world_shutdown(&world);
    return 0;
}

static int test_non_sketch_drag_release_records_legacy_position_undo(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t line = scene_add_line(
        &scene, vec3_make(0.0f, 0.0f, 0.0f), vec3_make(1.0f, 0.0f, 0.0f),
        vec4_make(1.0f, 1.0f, 1.0f, 1.0f), 1.0f);
    if (line == 0) return 1;

    TransformComp *before_xform = ecs_world_get_transform(&world, line);
    GeometryComp *before_geom = ecs_world_get_geometry(&world, line);
    if (!before_xform || !before_geom || before_geom->type != GEOM_LINE) return 1;
    vec3_t old_pos = before_xform->position;
    vec3_t geom_a_before = before_geom->data.line.a;
    vec3_t geom_b_before = before_geom->data.line.b;

    ecs_entity_t selected[1] = { line };
    if (!scene_apply_transform_delta_for_selection(&scene, selected, 1, vec3_make(0.35f, -0.2f, 0.0f))) return 1;

    TransformComp *after_xform = ecs_world_get_transform(&world, line);
    GeometryComp *after_geom = ecs_world_get_geometry(&world, line);
    if (!after_xform || !after_geom || after_geom->type != GEOM_LINE) return 1;
    vec3_t new_pos = after_xform->position;

    undo_redo_t undo = {0};
    undo_redo_init(&undo, &scene, 8);
    if (!record_drag_end_move_for_entity(&undo, &scene, line, old_pos, new_pos)) {
        undo_redo_shutdown(&undo);
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }

    if (undo.count != 1 || undo.commands[0].type != CMD_SET_POSITION) {
        undo_redo_shutdown(&undo);
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }

    if (fabsf(after_geom->data.line.a.x - geom_a_before.x) > 1e-6f ||
        fabsf(after_geom->data.line.a.y - geom_a_before.y) > 1e-6f ||
        fabsf(after_geom->data.line.b.x - geom_b_before.x) > 1e-6f ||
        fabsf(after_geom->data.line.b.y - geom_b_before.y) > 1e-6f) {
        undo_redo_shutdown(&undo);
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }

    undo_redo_shutdown(&undo);
    ecs_scene_shutdown(&scene);
    ecs_world_shutdown(&world);
    return 0;
}

static int test_endpoint_drag_records_only_on_release_boundary(void) {
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
    vec3_t old_local = endpoint_geom->data.point.point;

    undo_redo_t undo = {0};
    undo_redo_init(&undo, &scene, 8);
    if (undo.count != 0) return 1;

    ecs_entity_t selected[1] = { endpoint_a };
    if (!scene_apply_transform_delta_for_selection(&scene, selected, 1, vec3_make(0.2f, 0.0f, 0.0f))) return 1;
    if (!scene_apply_transform_delta_for_selection(&scene, selected, 1, vec3_make(0.1f, 0.15f, 0.0f))) return 1;
    if (undo.count != 0) {
        undo_redo_shutdown(&undo);
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }

    endpoint_geom = ecs_world_get_geometry(&world, endpoint_a);
    if (!endpoint_geom || endpoint_geom->type != GEOM_POINT) return 1;
    vec3_t new_local = endpoint_geom->data.point.point;
    if (!record_drag_end_move_for_entity(&undo, &scene, endpoint_a, old_local, new_local)) {
        undo_redo_shutdown(&undo);
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }

    if (undo.count != 1 || undo.commands[0].type != CMD_MOVE_ENDPOINT_PARTICIPANT) {
        undo_redo_shutdown(&undo);
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }

    if (record_drag_end_move_for_entity(&undo, &scene, endpoint_a, new_local, new_local)) {
        undo_redo_shutdown(&undo);
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }
    if (undo.count != 1) {
        undo_redo_shutdown(&undo);
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }

    undo_redo_shutdown(&undo);
    ecs_scene_shutdown(&scene);
    ecs_world_shutdown(&world);
    return 0;
}

static int test_endpoint_drag_start_snapshot_uses_endpoint_geometry_position(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(0.2f, 0.7f, 1.0f, 1.0f));
    if (sketch == 0) return 1;
    ecs_entity_t line = scene_add_line_to_sketch(
        &scene, sketch, vec3_make(0.5f, -0.25f, 0.0f), vec3_make(1.5f, -0.25f, 0.0f),
        vec4_make(1.0f, 1.0f, 1.0f, 1.0f), 1.0f);
    if (line == 0) return 1;

    EndPointsComp *line_endpoints = ecs_world_get_endpoints(&world, line);
    endpoint_binding_t binding_a = {0};
    if (!line_endpoints || !endpoints_comp_find_binding(line_endpoints, CONSTRAINT_PARTICIPANT_ROLE_POINT_A, &binding_a)) {
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }

    ecs_entity_t endpoint_a = (ecs_entity_t)binding_a.endpoint_entity;
    GeometryComp *endpoint_geom = ecs_world_get_geometry(&world, endpoint_a);
    TransformComp *endpoint_xform = ecs_world_get_transform(&world, endpoint_a);
    if (!endpoint_geom || endpoint_geom->type != GEOM_POINT || !endpoint_xform) {
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }

    vec3_t old_local = endpoint_geom->data.point.point;
    if (fabsf(endpoint_xform->position.x) > 1e-6f ||
        fabsf(endpoint_xform->position.y) > 1e-6f ||
        fabsf(endpoint_xform->position.z) > 1e-6f) {
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }

    undo_redo_t undo = {0};
    undo_redo_init(&undo, &scene, 8);
    vec3_t drag_start_snapshot = record_drag_start_position_for_entity(&scene, endpoint_a);

    ecs_entity_t selected[1] = { endpoint_a };
    if (!scene_apply_transform_delta_for_selection(&scene, selected, 1, vec3_make(0.4f, 0.3f, 0.0f))) {
        undo_redo_shutdown(&undo);
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }

    if (!record_drag_end_move_for_entity(&undo, &scene, endpoint_a, drag_start_snapshot, vec3_make(0, 0, 0))) {
        undo_redo_shutdown(&undo);
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }
    if (undo.count != 1 || undo.commands[0].type != CMD_MOVE_ENDPOINT_PARTICIPANT) {
        undo_redo_shutdown(&undo);
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }

    if (!undo_redo_undo(&undo)) {
        undo_redo_shutdown(&undo);
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }

    endpoint_geom = ecs_world_get_geometry(&world, endpoint_a);
    GeometryComp *line_geom = ecs_world_get_geometry(&world, line);
    if (!endpoint_geom || endpoint_geom->type != GEOM_POINT || !line_geom || line_geom->type != GEOM_LINE) {
        undo_redo_shutdown(&undo);
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }

    if (fabsf(endpoint_geom->data.point.point.x - old_local.x) > 1e-5f ||
        fabsf(endpoint_geom->data.point.point.y - old_local.y) > 1e-5f ||
        fabsf(endpoint_geom->data.point.point.z - old_local.z) > 1e-5f ||
        fabsf(line_geom->data.line.a.x - old_local.x) > 1e-5f ||
        fabsf(line_geom->data.line.a.y - old_local.y) > 1e-5f ||
        fabsf(line_geom->data.line.a.z - old_local.z) > 1e-5f) {
        undo_redo_shutdown(&undo);
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }

    undo_redo_shutdown(&undo);
    ecs_scene_shutdown(&scene);
    ecs_world_shutdown(&world);
    return 0;
}

static int test_standalone_sketch_point_drag_records_geometry_vertex_undo(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(0.2f, 0.7f, 1.0f, 1.0f));
    if (sketch == 0) return 1;
    ecs_entity_t point = scene_add_point_to_sketch(
        &scene, sketch, vec3_make(0.25f, -0.5f, 0.0f),
        vec4_make(1.0f, 1.0f, 1.0f, 1.0f), 0.06f);
    if (point == 0) return 1;

    GeometryComp *point_geom = ecs_world_get_geometry(&world, point);
    TransformComp *point_xform = ecs_world_get_transform(&world, point);
    EndPointsComp *point_endpoint_meta = ecs_world_get_endpoints(&world, point);
    if (!point_geom || point_geom->type != GEOM_POINT || !point_xform) return 1;
    if (point_endpoint_meta && point_endpoint_meta->is_endpoint_point) return 1;

    vec3_t old_local = point_geom->data.point.point;
    if (fabsf(point_xform->position.x) > 1e-6f ||
        fabsf(point_xform->position.y) > 1e-6f ||
        fabsf(point_xform->position.z) > 1e-6f) {
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }

    if (!scene_apply_standalone_sketch_point_world_delta(&scene, point, vec3_make(0.35f, 0.2f, 0.0f))) {
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }
    if (!scene_apply_standalone_sketch_point_world_delta(&scene, point, vec3_make(-0.1f, 0.15f, 0.0f))) {
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }

    point_geom = ecs_world_get_geometry(&world, point);
    point_xform = ecs_world_get_transform(&world, point);
    if (!point_geom || point_geom->type != GEOM_POINT || !point_xform) {
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }
    vec3_t new_local = point_geom->data.point.point;
    if (fabsf(new_local.x - old_local.x) <= 1e-6f &&
        fabsf(new_local.y - old_local.y) <= 1e-6f &&
        fabsf(new_local.z - old_local.z) <= 1e-6f) {
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }

    if (fabsf(point_xform->position.x) > 1e-6f ||
        fabsf(point_xform->position.y) > 1e-6f ||
        fabsf(point_xform->position.z) > 1e-6f) {
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }

    undo_redo_t undo = {0};
    undo_redo_init(&undo, &scene, 8);
    int vertex_index = 0;
    undo_cmd_set_geometry_vertices(&undo, point, &vertex_index, &old_local, &new_local, 1);
    if (undo.count != 1 || undo.commands[0].type != CMD_SET_GEOMETRY_VERTICES) {
        undo_redo_shutdown(&undo);
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }

    if (!undo_redo_undo(&undo)) {
        undo_redo_shutdown(&undo);
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }
    point_geom = ecs_world_get_geometry(&world, point);
    if (!point_geom || point_geom->type != GEOM_POINT) {
        undo_redo_shutdown(&undo);
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }
    if (fabsf(point_geom->data.point.point.x - old_local.x) > 1e-5f ||
        fabsf(point_geom->data.point.point.y - old_local.y) > 1e-5f ||
        fabsf(point_geom->data.point.point.z - old_local.z) > 1e-5f) {
        undo_redo_shutdown(&undo);
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }

    if (!undo_redo_redo(&undo)) {
        undo_redo_shutdown(&undo);
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }
    point_geom = ecs_world_get_geometry(&world, point);
    if (!point_geom || point_geom->type != GEOM_POINT) {
        undo_redo_shutdown(&undo);
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }
    if (fabsf(point_geom->data.point.point.x - new_local.x) > 1e-5f ||
        fabsf(point_geom->data.point.point.y - new_local.y) > 1e-5f ||
        fabsf(point_geom->data.point.point.z - new_local.z) > 1e-5f) {
        undo_redo_shutdown(&undo);
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }

    undo_redo_shutdown(&undo);
    ecs_scene_shutdown(&scene);
    ecs_world_shutdown(&world);
    return 0;
}

static int test_endpoint_arc_undo_preserves_angle_branch_without_inversion(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(0.2f, 0.7f, 1.0f, 1.0f));
    if (sketch == 0) return 1;
    ecs_entity_t arc = scene_add_arc_to_sketch(
        &scene, sketch, vec3_make(0.0f, 0.0f, 0.0f), 1.0f,
        3.10f, 3.12f, vec3_make(0.0f, 0.0f, 1.0f),
        vec4_make(1.0f, 1.0f, 1.0f, 1.0f), 1.0f);
    if (arc == 0) return 1;

    EndPointsComp *arc_endpoints = ecs_world_get_endpoints(&world, arc);
    endpoint_binding_t start_binding = {0};
    if (!arc_endpoints || !endpoints_comp_find_binding(arc_endpoints, CONSTRAINT_PARTICIPANT_ROLE_POINT_A, &start_binding)) {
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }

    ecs_entity_t endpoint_a = (ecs_entity_t)start_binding.endpoint_entity;
    GeometryComp *arc_geom = ecs_world_get_geometry(&world, arc);
    GeometryComp *endpoint_geom = ecs_world_get_geometry(&world, endpoint_a);
    if (!arc_geom || arc_geom->type != GEOM_ARC) {
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }
    if (!endpoint_geom || endpoint_geom->type != GEOM_POINT) {
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }
    float old_start = arc_geom->data.arc.start_angle;
    vec3_t old_local = endpoint_geom->data.point.point;

    undo_redo_t undo = {0};
    undo_redo_init(&undo, &scene, 8);

    vec3_t new_local = vec3_make(-0.999f, -0.045f, 0.0f); // equivalent angle near -pi branch
    if (!undo_cmd_record_endpoint_participant_move_if_changed(&undo, &scene, endpoint_a,
                                                               old_local,
                                                               new_local)) {
        undo_redo_shutdown(&undo);
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }
    if (undo.count != 1 || undo.commands[0].type != CMD_MOVE_ENDPOINT_PARTICIPANT) {
        undo_redo_shutdown(&undo);
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }

    if (!undo_redo_undo(&undo)) {
        undo_redo_shutdown(&undo);
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }
    arc_geom = ecs_world_get_geometry(&world, arc);
    if (!arc_geom || arc_geom->type != GEOM_ARC) {
        undo_redo_shutdown(&undo);
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }
    if (fabsf(arc_geom->data.arc.start_angle - old_start) > 0.02f) {
        undo_redo_shutdown(&undo);
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }

    if (!undo_redo_redo(&undo)) {
        undo_redo_shutdown(&undo);
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }
    arc_geom = ecs_world_get_geometry(&world, arc);
    if (!arc_geom || arc_geom->type != GEOM_ARC) {
        undo_redo_shutdown(&undo);
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }
    // Redo should stay in same branch neighborhood near +pi instead of flipping to negative branch.
    if (arc_geom->data.arc.start_angle < 2.8f || arc_geom->data.arc.start_angle > 3.4f) {
        undo_redo_shutdown(&undo);
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }

    undo_redo_shutdown(&undo);
    ecs_scene_shutdown(&scene);
    ecs_world_shutdown(&world);
    return 0;
}

static int test_bulk_delete_undo_restores_endpoint_owner_sync_metadata(void) {
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

    undo_redo_t undo = {0};
    undo_redo_init(&undo, &scene, 8);

    ecs_entity_t to_delete[1] = { line };
    undo_cmd_bulk_delete_entities(&undo, to_delete, 1);
    scene_remove_entity(&scene, line);

    if (!undo_redo_undo(&undo)) {
        undo_redo_shutdown(&undo);
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }
    if (undo.count != 1 || undo.commands[0].type != CMD_BULK_DELETE_ENTITIES) {
        undo_redo_shutdown(&undo);
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }

    ecs_entity_t restored_line = 0;
    undo_command_t *cmd = &undo.commands[0];
    for (int i = 0; i < cmd->data.bulk_delete.count; i++) {
        undo_entity_snapshot_t *snap = &cmd->data.bulk_delete.snapshots[i];
        if (!snap->has_geometry) continue;
        if (snap->geom_type != UNDO_GEOM_LINE) continue;
        if (snap->parent_id != (uint64_t)sketch) continue;
        restored_line = (ecs_entity_t)cmd->data.bulk_delete.entity_ids[i];
        break;
    }
    if (restored_line == 0 || !ecs_is_alive(world.world, restored_line)) {
        undo_redo_shutdown(&undo);
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }

    EndPointsComp *restored_owner_endpoints = ecs_world_get_endpoints(&world, restored_line);
    endpoint_binding_t binding_a = {0};
    if (!restored_owner_endpoints ||
        restored_owner_endpoints->is_endpoint_point ||
        !endpoints_comp_find_binding(restored_owner_endpoints, CONSTRAINT_PARTICIPANT_ROLE_POINT_A, &binding_a)) {
        undo_redo_shutdown(&undo);
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }

    ecs_entity_t endpoint_a = (ecs_entity_t)binding_a.endpoint_entity;
    EndPointsComp *endpoint_meta = ecs_world_get_endpoints(&world, endpoint_a);
    if (!endpoint_meta || !endpoint_meta->is_endpoint_point ||
        (ecs_entity_t)endpoint_meta->owner_entity != restored_line) {
        undo_redo_shutdown(&undo);
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }
    ecs_entity_t restored_line_children[8] = {0};
    int restored_line_child_count = scene_get_children(&scene, restored_line, restored_line_children, 8);
    if (restored_line_child_count != 2) {
        undo_redo_shutdown(&undo);
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }
    for (int i = 0; i < restored_line_child_count; i++) {
        EndPointsComp *child_endpoint_meta = ecs_world_get_endpoints(&world, restored_line_children[i]);
        if (!child_endpoint_meta || !child_endpoint_meta->is_endpoint_point ||
            (ecs_entity_t)child_endpoint_meta->owner_entity != restored_line) {
            undo_redo_shutdown(&undo);
            ecs_scene_shutdown(&scene);
            ecs_world_shutdown(&world);
            return 1;
        }
    }

    GeometryComp *line_geom_before = ecs_world_get_geometry(&world, restored_line);
    TransformComp *endpoint_xform_before = ecs_world_get_transform(&world, endpoint_a);
    if (!line_geom_before || line_geom_before->type != GEOM_LINE || !endpoint_xform_before) {
        undo_redo_shutdown(&undo);
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }
    vec3_t old_a = line_geom_before->data.line.a;

    ecs_entity_t selected[1] = { endpoint_a };
    if (!scene_apply_transform_delta_for_selection(&scene, selected, 1, vec3_make(0.3f, 0.2f, 0.0f))) {
        undo_redo_shutdown(&undo);
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }

    GeometryComp *line_geom_after = ecs_world_get_geometry(&world, restored_line);
    TransformComp *endpoint_xform_after = ecs_world_get_transform(&world, endpoint_a);
    if (!line_geom_after || !endpoint_xform_after) {
        undo_redo_shutdown(&undo);
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }
    if (fabsf(line_geom_after->data.line.a.x - old_a.x) <= 1e-6f &&
        fabsf(line_geom_after->data.line.a.y - old_a.y) <= 1e-6f) {
        undo_redo_shutdown(&undo);
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }
    if (fabsf(endpoint_xform_after->position.x) > 1e-6f ||
        fabsf(endpoint_xform_after->position.y) > 1e-6f ||
        fabsf(endpoint_xform_after->position.z) > 1e-6f) {
        undo_redo_shutdown(&undo);
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }

    undo_redo_shutdown(&undo);
    ecs_scene_shutdown(&scene);
    ecs_world_shutdown(&world);
    return 0;
}

static int test_endpoint_pick_lookup_decodes_encoded_pick_ids(void) {
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

    SelectableComp *owner_sel = ecs_world_get_selectable(&world, line);
    if (!owner_sel || owner_sel->pick_id == 0) {
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }

    uint32_t encoded_endpoint_pick = 0;
    if (!endpoint_pick_encode(owner_sel->pick_id, CONSTRAINT_PARTICIPANT_ROLE_POINT_A, &encoded_endpoint_pick)) {
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }

    ecs_entity_t looked_up = ecs_scene_find_entity_by_pick_id(&scene, encoded_endpoint_pick);
    if (looked_up == 0) {
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }

    EndPointsComp *endpoint_meta = ecs_world_get_endpoints(&world, looked_up);
    if (!endpoint_meta || !endpoint_meta->is_endpoint_point ||
        (ecs_entity_t)endpoint_meta->owner_entity != line ||
        endpoint_meta->role != CONSTRAINT_PARTICIPANT_ROLE_POINT_A) {
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }

    ecs_scene_shutdown(&scene);
    ecs_world_shutdown(&world);
    return 0;
}

static int test_bulk_delete_undo_restores_arc_endpoint_sync_for_center_drag(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(0.2f, 0.7f, 1.0f, 1.0f));
    if (sketch == 0) return 1;
    ecs_entity_t arc = scene_add_arc_to_sketch(
        &scene, sketch, vec3_make(0.0f, 0.0f, 0.0f), 1.0f, 0.0f, 1.57079633f, vec3_make(0.0f, 0.0f, 1.0f),
        vec4_make(1.0f, 1.0f, 1.0f, 1.0f), 1.0f);
    if (arc == 0) return 1;

    undo_redo_t undo = {0};
    undo_redo_init(&undo, &scene, 8);
    ecs_entity_t to_delete[1] = { arc };
    undo_cmd_bulk_delete_entities(&undo, to_delete, 1);
    scene_remove_entity(&scene, arc);
    if (!undo_redo_undo(&undo)) {
        undo_redo_shutdown(&undo);
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }

    ecs_entity_t restored_arc = 0;
    undo_command_t *cmd = &undo.commands[0];
    for (int i = 0; i < cmd->data.bulk_delete.count; i++) {
        undo_entity_snapshot_t *snap = &cmd->data.bulk_delete.snapshots[i];
        if (!snap->has_geometry) continue;
        if (snap->geom_type != UNDO_GEOM_ARC) continue;
        if (snap->parent_id != (uint64_t)sketch) continue;
        restored_arc = (ecs_entity_t)cmd->data.bulk_delete.entity_ids[i];
        break;
    }
    if (restored_arc == 0 || !ecs_is_alive(world.world, restored_arc)) {
        undo_redo_shutdown(&undo);
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }

    EndPointsComp *owner_endpoints = ecs_world_get_endpoints(&world, restored_arc);
    endpoint_binding_t center_binding = {0};
    if (!owner_endpoints ||
        !endpoints_comp_find_binding(owner_endpoints, CONSTRAINT_PARTICIPANT_ROLE_CENTER, &center_binding)) {
        undo_redo_shutdown(&undo);
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }
    ecs_entity_t center_endpoint = (ecs_entity_t)center_binding.endpoint_entity;
    EndPointsComp *center_meta = ecs_world_get_endpoints(&world, center_endpoint);
    if (!center_meta || !center_meta->is_endpoint_point ||
        center_meta->role != CONSTRAINT_PARTICIPANT_ROLE_CENTER ||
        (ecs_entity_t)center_meta->owner_entity != restored_arc) {
        undo_redo_shutdown(&undo);
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }
    ecs_entity_t restored_arc_children[12] = {0};
    int restored_arc_child_count = scene_get_children(&scene, restored_arc, restored_arc_children, 12);
    if (restored_arc_child_count != 3) {
        undo_redo_shutdown(&undo);
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }
    for (int i = 0; i < restored_arc_child_count; i++) {
        EndPointsComp *child_endpoint_meta = ecs_world_get_endpoints(&world, restored_arc_children[i]);
        if (!child_endpoint_meta || !child_endpoint_meta->is_endpoint_point ||
            (ecs_entity_t)child_endpoint_meta->owner_entity != restored_arc) {
            undo_redo_shutdown(&undo);
            ecs_scene_shutdown(&scene);
            ecs_world_shutdown(&world);
            return 1;
        }
    }

    GeometryComp *arc_before = ecs_world_get_geometry(&world, restored_arc);
    if (!arc_before || arc_before->type != GEOM_ARC) {
        undo_redo_shutdown(&undo);
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }
    vec3_t center_before = arc_before->data.arc.center;

    ecs_entity_t selected[1] = { center_endpoint };
    if (!scene_apply_transform_delta_for_selection(&scene, selected, 1, vec3_make(0.25f, -0.15f, 0.0f))) {
        undo_redo_shutdown(&undo);
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }

    GeometryComp *arc_after = ecs_world_get_geometry(&world, restored_arc);
    TransformComp *center_xform = ecs_world_get_transform(&world, center_endpoint);
    if (!arc_after || !center_xform) {
        undo_redo_shutdown(&undo);
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }
    if (fabsf(arc_after->data.arc.center.x - center_before.x) <= 1e-6f &&
        fabsf(arc_after->data.arc.center.y - center_before.y) <= 1e-6f) {
        undo_redo_shutdown(&undo);
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }
    if (fabsf(center_xform->position.x) > 1e-6f ||
        fabsf(center_xform->position.y) > 1e-6f ||
        fabsf(center_xform->position.z) > 1e-6f) {
        undo_redo_shutdown(&undo);
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }

    undo_redo_shutdown(&undo);
    ecs_scene_shutdown(&scene);
    ecs_world_shutdown(&world);
    return 0;
}

static int test_bulk_delete_undo_restores_endpoint_pick_id_mapping(void) {
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

    undo_redo_t undo = {0};
    undo_redo_init(&undo, &scene, 8);
    ecs_entity_t to_delete[1] = { line };
    undo_cmd_bulk_delete_entities(&undo, to_delete, 1);
    scene_remove_entity(&scene, line);
    if (!undo_redo_undo(&undo)) {
        undo_redo_shutdown(&undo);
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }

    ecs_entity_t restored_line = 0;
    undo_command_t *cmd = &undo.commands[0];
    for (int i = 0; i < cmd->data.bulk_delete.count; i++) {
        undo_entity_snapshot_t *snap = &cmd->data.bulk_delete.snapshots[i];
        if (!snap->has_geometry) continue;
        if (snap->geom_type != UNDO_GEOM_LINE) continue;
        if (snap->parent_id != (uint64_t)sketch) continue;
        restored_line = (ecs_entity_t)cmd->data.bulk_delete.entity_ids[i];
        break;
    }
    if (restored_line == 0 || !ecs_is_alive(world.world, restored_line)) {
        undo_redo_shutdown(&undo);
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }

    SelectableComp *restored_owner_sel = ecs_world_get_selectable(&world, restored_line);
    if (!restored_owner_sel || restored_owner_sel->pick_id == 0) {
        undo_redo_shutdown(&undo);
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }

    uint32_t encoded_endpoint_pick = 0;
    if (!endpoint_pick_encode(restored_owner_sel->pick_id, CONSTRAINT_PARTICIPANT_ROLE_POINT_A, &encoded_endpoint_pick)) {
        undo_redo_shutdown(&undo);
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }

    ecs_entity_t resolved_owner = scene_constraint_participant_entity_for_pick(&scene, encoded_endpoint_pick, NULL, NULL);
    if (resolved_owner != restored_line) {
        undo_redo_shutdown(&undo);
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        return 1;
    }

    undo_redo_shutdown(&undo);
    ecs_scene_shutdown(&scene);
    ecs_world_shutdown(&world);
    return 0;
}

static int test_gizmo_midpoint_single_active_sketch_line_center(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(0.2f, 0.7f, 1.0f, 1.0f));
    ecs_entity_t line = scene_add_line_to_sketch(
        &scene, sketch, vec3_make(2.0f, 4.0f, 0.0f), vec3_make(6.0f, 8.0f, 0.0f),
        vec4_make(1.0f, 1.0f, 1.0f, 1.0f), 1.0f);
    if (!sketch || !line) return 1;

    selection_buffer_t sel = {0};
    selection_init(&sel, &world);
    selection_add(&sel, line);
    gizmo_t gizmo = {0};
    gizmo_init(&gizmo);

    gizmo_update(&gizmo, &sel, &scene, vec3_make(0.0f, 0.0f, 10.0f), 0.785398f, 1.0f);
    vec3_t expected = vec3_make(4.0f, 6.0f, 0.0f);
    int ok = test_vec3_close(gizmo.center, expected, 1e-4f) ? 0 : 1;

    selection_shutdown(&sel);
    gizmo_shutdown(&gizmo);
    ecs_scene_shutdown(&scene);
    ecs_world_shutdown(&world);
    return ok;
}

static int test_gizmo_midpoint_multi_active_sketch_lines_average(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(0.2f, 0.7f, 1.0f, 1.0f));
    ecs_entity_t line1 = scene_add_line_to_sketch(
        &scene, sketch, vec3_make(0.0f, 0.0f, 0.0f), vec3_make(2.0f, 0.0f, 0.0f),
        vec4_make(1.0f, 1.0f, 1.0f, 1.0f), 1.0f);
    ecs_entity_t line2 = scene_add_line_to_sketch(
        &scene, sketch, vec3_make(4.0f, 0.0f, 0.0f), vec3_make(8.0f, 0.0f, 0.0f),
        vec4_make(1.0f, 1.0f, 1.0f, 1.0f), 1.0f);
    if (!sketch || !line1 || !line2) return 1;

    selection_buffer_t sel = {0};
    selection_init(&sel, &world);
    selection_add(&sel, line1);
    selection_add(&sel, line2);
    gizmo_t gizmo = {0};
    gizmo_init(&gizmo);

    gizmo_update(&gizmo, &sel, &scene, vec3_make(0.0f, 0.0f, 10.0f), 0.785398f, 1.0f);
    vec3_t expected = vec3_make(3.5f, 0.0f, 0.0f);
    int ok = test_vec3_close(gizmo.center, expected, 1e-4f) ? 0 : 1;

    selection_shutdown(&sel);
    gizmo_shutdown(&gizmo);
    ecs_scene_shutdown(&scene);
    ecs_world_shutdown(&world);
    return ok;
}

static int test_active_sketch_line_rigid_delta_single_interaction_undo(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(0.2f, 0.7f, 1.0f, 1.0f));
    ecs_entity_t line1 = scene_add_line_to_sketch(
        &scene, sketch, vec3_make(0.0f, 0.0f, 0.0f), vec3_make(2.0f, 0.0f, 0.0f),
        vec4_make(1.0f, 1.0f, 1.0f, 1.0f), 1.0f);
    ecs_entity_t line2 = scene_add_line_to_sketch(
        &scene, sketch, vec3_make(0.0f, 2.0f, 0.0f), vec3_make(2.0f, 2.0f, 0.0f),
        vec4_make(1.0f, 1.0f, 1.0f, 1.0f), 1.0f);
    if (!sketch || !line1 || !line2) return 1;

    GeometryComp *g1 = ecs_world_get_geometry(&world, line1);
    GeometryComp *g2 = ecs_world_get_geometry(&world, line2);
    if (!g1 || !g2 || g1->type != GEOM_LINE || g2->type != GEOM_LINE) return 1;
    vec3_t l1_old_a = g1->data.line.a;
    vec3_t l1_old_b = g1->data.line.b;
    vec3_t l2_old_a = g2->data.line.a;
    vec3_t l2_old_b = g2->data.line.b;

    vec3_t delta = vec3_make(0.3f, -0.4f, 0.0f);
    if (!scene_apply_active_sketch_line_world_delta(&scene, line1, delta)) return 1;
    if (!scene_apply_active_sketch_line_world_delta(&scene, line2, delta)) return 1;

    g1 = ecs_world_get_geometry(&world, line1);
    g2 = ecs_world_get_geometry(&world, line2);
    if (!g1 || !g2) return 1;
    if (!test_vec3_close(g1->data.line.a, vec3_add(l1_old_a, delta), 1e-4f) ||
        !test_vec3_close(g1->data.line.b, vec3_add(l1_old_b, delta), 1e-4f) ||
        !test_vec3_close(g2->data.line.a, vec3_add(l2_old_a, delta), 1e-4f) ||
        !test_vec3_close(g2->data.line.b, vec3_add(l2_old_b, delta), 1e-4f)) {
        return 1;
    }

    cmd_bulk_line_endpoint_item_t items[2] = {0};
    items[0].entity_id = (uint64_t)line1;
    items[0].old_a = l1_old_a;
    items[0].old_b = l1_old_b;
    items[0].new_a = g1->data.line.a;
    items[0].new_b = g1->data.line.b;
    items[1].entity_id = (uint64_t)line2;
    items[1].old_a = l2_old_a;
    items[1].old_b = l2_old_b;
    items[1].new_a = g2->data.line.a;
    items[1].new_b = g2->data.line.b;

    undo_redo_t undo = {0};
    undo_redo_init(&undo, &scene, 8);
    if (!undo_cmd_push_bulk_line_endpoints(&undo, items, 2)) {
        undo_redo_shutdown(&undo);
        return 1;
    }
    if (undo.count != 1 || undo.current != 1 || undo.commands[0].type != CMD_BULK_LINE_ENDPOINTS) {
        undo_redo_shutdown(&undo);
        return 1;
    }

    if (!undo_redo_undo(&undo)) {
        undo_redo_shutdown(&undo);
        return 1;
    }
    g1 = ecs_world_get_geometry(&world, line1);
    g2 = ecs_world_get_geometry(&world, line2);
    if (!g1 || !g2 ||
        !test_vec3_close(g1->data.line.a, l1_old_a, 1e-6f) ||
        !test_vec3_close(g1->data.line.b, l1_old_b, 1e-6f) ||
        !test_vec3_close(g2->data.line.a, l2_old_a, 1e-6f) ||
        !test_vec3_close(g2->data.line.b, l2_old_b, 1e-6f)) {
        undo_redo_shutdown(&undo);
        return 1;
    }

    undo_redo_shutdown(&undo);
    ecs_scene_shutdown(&scene);
    ecs_world_shutdown(&world);
    return 0;
}

static int test_mixed_selection_guardrail_redo_exact_endpoints(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(0.2f, 0.7f, 1.0f, 1.0f));
    ecs_entity_t active_line = scene_add_line_to_sketch(
        &scene, sketch, vec3_make(0.0f, 0.0f, 0.0f), vec3_make(1.0f, 0.0f, 0.0f),
        vec4_make(1.0f, 1.0f, 1.0f, 1.0f), 1.0f);
    ecs_entity_t non_sketch_line = scene_add_line(
        &scene, vec3_make(5.0f, 0.0f, 0.0f), vec3_make(6.0f, 0.0f, 0.0f),
        vec4_make(1.0f, 1.0f, 1.0f, 1.0f), 1.0f);
    if (!sketch || !active_line || !non_sketch_line) return 1;

    GeometryComp *active_geom = ecs_world_get_geometry(&world, active_line);
    GeometryComp *non_geom = ecs_world_get_geometry(&world, non_sketch_line);
    if (!active_geom || !non_geom || active_geom->type != GEOM_LINE || non_geom->type != GEOM_LINE) return 1;

    vec3_t old_a = active_geom->data.line.a;
    vec3_t old_b = active_geom->data.line.b;
    vec3_t non_old_a = non_geom->data.line.a;
    vec3_t non_old_b = non_geom->data.line.b;
    TransformComp *non_xform_before = ecs_world_get_transform(&world, non_sketch_line);
    if (!non_xform_before) return 1;
    vec3_t non_old_pos = non_xform_before->position;

    vec3_t delta = vec3_make(0.25f, 0.15f, 0.0f);
    if (!scene_apply_active_sketch_line_world_delta(&scene, active_line, delta)) return 1;
    ecs_entity_t single_non[1] = { non_sketch_line };
    if (!scene_apply_transform_delta_for_selection(&scene, single_non, 1, delta)) return 1;

    active_geom = ecs_world_get_geometry(&world, active_line);
    non_geom = ecs_world_get_geometry(&world, non_sketch_line);
    TransformComp *non_xform_after = ecs_world_get_transform(&world, non_sketch_line);
    if (!active_geom || !non_geom || !non_xform_after) return 1;

    vec3_t new_a = active_geom->data.line.a;
    vec3_t new_b = active_geom->data.line.b;
    if (!test_vec3_close(new_a, vec3_add(old_a, delta), 1e-4f) ||
        !test_vec3_close(new_b, vec3_add(old_b, delta), 1e-4f)) {
        return 1;
    }

    if (!test_vec3_close(non_geom->data.line.a, non_old_a, 1e-6f) ||
        !test_vec3_close(non_geom->data.line.b, non_old_b, 1e-6f) ||
        !test_vec3_close(non_xform_after->position, vec3_add(non_old_pos, delta), 1e-6f)) {
        return 1;
    }

    undo_redo_t undo = {0};
    undo_redo_init(&undo, &scene, 8);

    cmd_bulk_line_endpoint_item_t one = {0};
    one.entity_id = (uint64_t)active_line;
    one.old_a = old_a;
    one.old_b = old_b;
    one.new_a = new_a;
    one.new_b = new_b;
    if (!undo_cmd_push_bulk_line_endpoints(&undo, &one, 1)) {
        undo_redo_shutdown(&undo);
        return 1;
    }
    record_drag_end_move_for_entity(&undo, &scene, non_sketch_line, non_old_pos, vec3_make(0, 0, 0));
    if (undo.count != 2 || undo.commands[0].type != CMD_BULK_LINE_ENDPOINTS || undo.commands[1].type != CMD_SET_POSITION) {
        undo_redo_shutdown(&undo);
        return 1;
    }

    if (!undo_redo_undo(&undo) || !undo_redo_undo(&undo) || !undo_redo_redo(&undo) || !undo_redo_redo(&undo)) {
        undo_redo_shutdown(&undo);
        return 1;
    }

    active_geom = ecs_world_get_geometry(&world, active_line);
    if (!active_geom || !test_vec3_close(active_geom->data.line.a, new_a, 1e-6f) ||
        !test_vec3_close(active_geom->data.line.b, new_b, 1e-6f)) {
        undo_redo_shutdown(&undo);
        return 1;
    }

    ecs_entity_t endpoint_a = 0;
    ecs_entity_t endpoint_b = 0;
    if (!test_line_endpoint_entity(&world, active_line, CONSTRAINT_PARTICIPANT_ROLE_POINT_A, &endpoint_a) ||
        !test_line_endpoint_entity(&world, active_line, CONSTRAINT_PARTICIPANT_ROLE_POINT_B, &endpoint_b)) {
        undo_redo_shutdown(&undo);
        return 1;
    }
    GeometryComp *endpoint_a_geom = ecs_world_get_geometry(&world, endpoint_a);
    GeometryComp *endpoint_b_geom = ecs_world_get_geometry(&world, endpoint_b);
    if (!endpoint_a_geom || endpoint_a_geom->type != GEOM_POINT ||
        !endpoint_b_geom || endpoint_b_geom->type != GEOM_POINT) {
        undo_redo_shutdown(&undo);
        return 1;
    }
    if (!test_vec3_close(endpoint_a_geom->data.point.point, new_a, 1e-6f) ||
        !test_vec3_close(endpoint_b_geom->data.point.point, new_b, 1e-6f)) {
        undo_redo_shutdown(&undo);
        return 1;
    }

    undo_redo_shutdown(&undo);
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
        { "test_geometry_manager_selection_sync_maps_viewport_selection_to_rows",
          test_geometry_manager_selection_sync_maps_viewport_selection_to_rows },
        { "test_selection_prune_dead_removes_deleted_selection_entries",
          test_selection_prune_dead_removes_deleted_selection_entries },
        { "test_endpoint_pick_overlay_precedence_contract", test_endpoint_pick_overlay_precedence_contract },
        { "test_endpoint_pick_collision_prefers_overlay", test_endpoint_pick_collision_prefers_overlay },
        { "test_endpoint_pick_collision_deterministic_repeated_sampling", test_endpoint_pick_collision_deterministic_repeated_sampling },
        { "test_endpoint_point_context_filters_line_only_constraints", test_endpoint_point_context_filters_line_only_constraints },
        { "test_endpoint_point_context_keeps_coincident_for_endpoint_pairs", test_endpoint_point_context_keeps_coincident_for_endpoint_pairs },
        { "test_directional_legality_accepts_point_pair_group_signatures_D01_D04",
          test_directional_legality_accepts_point_pair_group_signatures_D01_D04 },
        { "test_directional_legality_accepts_line_and_arc_landmark_roles_D02_D05",
          test_directional_legality_accepts_line_and_arc_landmark_roles_D02_D05 },
        { "test_directional_legality_rejects_raw_line_and_arc_entity_signatures_D03_D06",
          test_directional_legality_rejects_raw_line_and_arc_entity_signatures_D03_D06 },
        { "test_directional_legality_keeps_legacy_single_line_entity_signature_parity_alin01_alin03",
          test_directional_legality_keeps_legacy_single_line_entity_signature_parity_alin01_alin03 },
        { "test_arci01_legality_accepts_only_line_arc_entity_pair_D01",
          test_arci01_legality_accepts_only_line_arc_entity_pair_D01 },
        { "test_arci02_legality_accepts_only_line_endpoint_and_arc_endpoint_D05",
          test_arci02_legality_accepts_only_line_endpoint_and_arc_endpoint_D05 },
        { "test_arci03_legality_accepts_same_arc_ordered_endpoint_pair_D09",
          test_arci03_legality_accepts_same_arc_ordered_endpoint_pair_D09 },
        { "test_line_line_legality_allows_pair_parallel_and_perpendicular_lcon05",
          test_line_line_legality_allows_pair_parallel_and_perpendicular_lcon05 },
        { "test_line_line_legality_rejects_point_roles_for_parallel_and_perpendicular_lcon05",
          test_line_line_legality_rejects_point_roles_for_parallel_and_perpendicular_lcon05 },
        { "test_line_line_legality_allows_group_parallel_and_perpendicular_lcon04",
          test_line_line_legality_allows_group_parallel_and_perpendicular_lcon04 },
        { "test_line_line_legality_rejects_group_perpendicular_with_non_line_participant_lcon05",
          test_line_line_legality_rejects_group_perpendicular_with_non_line_participant_lcon05 },
        { "test_endpoint_line_endpoint_to_owner_bidirectional_sync", test_endpoint_line_endpoint_to_owner_bidirectional_sync },
        { "test_endpoint_arc_endpoint_center_to_owner_bidirectional_sync", test_endpoint_arc_endpoint_center_to_owner_bidirectional_sync },
        { "test_endpoint_direct_geometry_edit_syncs_owner_and_entities", test_endpoint_direct_geometry_edit_syncs_owner_and_entities },
        { "test_arc_slot_reallocation_for_expanded_span", test_arc_slot_reallocation_for_expanded_span },
        { "test_endpoint_undo_command_contract_roundtrip", test_endpoint_undo_command_contract_roundtrip },
        { "test_undo_legacy_position_and_point_commands_unchanged", test_undo_legacy_position_and_point_commands_unchanged },
        { "test_gizmo_endpoint_drag_records_single_endpoint_undo_at_release", test_gizmo_endpoint_drag_records_single_endpoint_undo_at_release },
        { "test_inspector_endpoint_edit_records_only_at_commit_boundary", test_inspector_endpoint_edit_records_only_at_commit_boundary },
        { "test_endpoint_undo_replay_resyncs_owner_and_endpoint_entity", test_endpoint_undo_replay_resyncs_owner_and_endpoint_entity },
        { "test_endpoint_redo_replay_resyncs_owner_and_endpoint_entity", test_endpoint_redo_replay_resyncs_owner_and_endpoint_entity },
        { "test_non_sketch_drag_release_records_legacy_position_undo", test_non_sketch_drag_release_records_legacy_position_undo },
        { "test_endpoint_drag_records_only_on_release_boundary", test_endpoint_drag_records_only_on_release_boundary },
        { "test_endpoint_drag_start_snapshot_uses_endpoint_geometry_position", test_endpoint_drag_start_snapshot_uses_endpoint_geometry_position },
        { "test_standalone_sketch_point_drag_records_geometry_vertex_undo", test_standalone_sketch_point_drag_records_geometry_vertex_undo },
        { "test_endpoint_arc_undo_preserves_angle_branch_without_inversion", test_endpoint_arc_undo_preserves_angle_branch_without_inversion },
        { "test_bulk_delete_undo_restores_endpoint_owner_sync_metadata", test_bulk_delete_undo_restores_endpoint_owner_sync_metadata },
        { "test_endpoint_pick_lookup_decodes_encoded_pick_ids", test_endpoint_pick_lookup_decodes_encoded_pick_ids },
        { "test_bulk_delete_undo_restores_arc_endpoint_sync_for_center_drag",
          test_bulk_delete_undo_restores_arc_endpoint_sync_for_center_drag },
        { "test_bulk_delete_undo_restores_endpoint_pick_id_mapping",
          test_bulk_delete_undo_restores_endpoint_pick_id_mapping },
        { "test_gizmo_midpoint_single_active_sketch_line_center",
          test_gizmo_midpoint_single_active_sketch_line_center },
        { "test_gizmo_midpoint_multi_active_sketch_lines_average",
          test_gizmo_midpoint_multi_active_sketch_lines_average },
        { "test_active_sketch_line_rigid_delta_single_interaction_undo",
          test_active_sketch_line_rigid_delta_single_interaction_undo },
        { "test_mixed_selection_guardrail_redo_exact_endpoints",
          test_mixed_selection_guardrail_redo_exact_endpoints },
    };

    for (size_t i = 0; i < (sizeof(tests) / sizeof(tests[0])); ++i) {
        if (tests[i].fn() != 0) {
            fprintf(stderr, "FAILED: %s\n", tests[i].name);
            return 1;
        }
    }
    return 0;
}
