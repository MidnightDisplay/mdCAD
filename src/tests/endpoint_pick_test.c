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

typedef int (*test_fn_t)(void);
typedef struct { const char *name; test_fn_t fn; } test_case_t;

int main(void) {
    static const test_case_t tests[] = {
        { "test_endpoint_pick_id_encode_decode_roundtrip", test_endpoint_pick_id_encode_decode_roundtrip },
        { "test_endpoint_pick_native_endpoint_entity_resolution", test_endpoint_pick_native_endpoint_entity_resolution },
        { "test_endpoint_pick_non_sketch_line_has_no_native_endpoints", test_endpoint_pick_non_sketch_line_has_no_native_endpoints },
        { "test_endpoint_pick_overlay_precedence_contract", test_endpoint_pick_overlay_precedence_contract },
        { "test_endpoint_pick_collision_prefers_overlay", test_endpoint_pick_collision_prefers_overlay },
        { "test_endpoint_pick_collision_deterministic_repeated_sampling", test_endpoint_pick_collision_deterministic_repeated_sampling },
        { "test_endpoint_point_context_filters_line_only_constraints", test_endpoint_point_context_filters_line_only_constraints },
        { "test_endpoint_point_context_keeps_coincident_for_endpoint_pairs", test_endpoint_point_context_keeps_coincident_for_endpoint_pairs },
    };

    for (size_t i = 0; i < (sizeof(tests) / sizeof(tests[0])); ++i) {
        if (tests[i].fn() != 0) {
            fprintf(stderr, "FAILED: %s\n", tests[i].name);
            return 1;
        }
    }
    return 0;
}
