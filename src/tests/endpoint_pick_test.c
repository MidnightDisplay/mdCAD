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

typedef int (*test_fn_t)(void);
typedef struct { const char *name; test_fn_t fn; } test_case_t;

int main(void) {
    static const test_case_t tests[] = {
        { "test_endpoint_pick_id_encode_decode_roundtrip", test_endpoint_pick_id_encode_decode_roundtrip },
        { "test_endpoint_pick_overlay_precedence_contract", test_endpoint_pick_overlay_precedence_contract },
    };

    for (size_t i = 0; i < (sizeof(tests) / sizeof(tests[0])); ++i) {
        if (tests[i].fn() != 0) {
            fprintf(stderr, "FAILED: %s\n", tests[i].name);
            return 1;
        }
    }
    return 0;
}
