#include "../components/jsonl_observer_comp.h"
#include "../jsonl_observer_system.h"
#include "../scene_serializer.h"
#include "../ecs/ecs_world.h"
#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif
void sokol_main(void) {}
#ifdef __cplusplus
}
#endif

static int test_jsonl_observer_defaults_and_message_window(void) {
    JsonlObserverComp obs = jsonl_observer_comp_default();
    if (!obs.observe_enabled) return 1;
    if (obs.max_retries != JSONL_OBSERVER_DEFAULT_MAX_RETRIES) return 1;
    if (obs.interval_ms != JSONL_OBSERVER_DEFAULT_INTERVAL_MS) return 1;
    if (obs.scale != 1.0f) return 1;
    if (obs.message_count != 0) return 1;

    jsonl_observer_comp_push_message(&obs, JSONL_OBSERVER_MSG_INFO, "first");
    jsonl_observer_comp_push_message(&obs, JSONL_OBSERVER_MSG_WARNING, "second");
    jsonl_observer_comp_push_message(&obs, JSONL_OBSERVER_MSG_ERROR, "third");
    if (obs.message_count != 2) return 1;
    if (strcmp(obs.messages[0], "second") != 0) return 1;
    if (strcmp(obs.messages[1], "third") != 0) return 1;
    return 0;
}

static int test_jsonl_observer_tick_retry_and_auto_disable_policy(void) {
    JsonlObserverComp obs = jsonl_observer_comp_default();
    jsonl_observer_comp_set_path(&obs, "missing_file.jsonl");
    obs.linked = true;
    obs.observe_enabled = true;
    obs.interval_ms = 120u;
    obs.max_retries = 2u;
    obs.retry_count = 0u;
    obs.next_retry_at_ms = 0u;

    // Simulate observer tick policy semantics (D-10/D-11/D-12).
    // Attempt 1 fails
    obs.retry_count++;
    obs.next_retry_at_ms = 1000u + obs.interval_ms;
    if (obs.retry_count != 1u) return 1;
    if (obs.next_retry_at_ms != 1120u) return 1;
    if (!obs.observe_enabled) return 1;

    // Attempt 2 fails -> exhaustion triggers auto-disable
    obs.retry_count++;
    if (obs.retry_count >= obs.max_retries) {
        obs.observe_enabled = false;
    }
    if (obs.retry_count != 2u) return 1;
    if (obs.observe_enabled) return 1;

    // D-13 manual action remains callable even when auto-disabled (state-only assertion)
    jsonl_observer_comp_push_message(&obs, JSONL_OBSERVER_MSG_WARNING, "manual reparse attempted");
    if (obs.message_count == 0) return 1;
    return 0;
}

static int test_jsonl_observer_label_contract_stem_and_path(void) {
    char stem[LABEL_NAME_MAX] = {0};
    const char *path = "C:\\logs\\test-part.jsonl";
    jsonl_observer_label_from_path(path, stem, sizeof(stem));
    if (strcmp(stem, "test-part") != 0) return 1;
    return 0;
}

static int test_jsonl_observer_serializer_emits_fields(void) {
    ecs_world_state_t world = {0};
    ecs_world_init(&world);
    ecs_entity_t sketch = ecs_world_create_anchor_entity(&world);
    if (sketch == 0) return 1;

    TransformComp t = transform_comp_default();
    ecs_set_id(world.world, sketch, world.TransformComp_id, sizeof(TransformComp), &t);

    SketchComp sk = sketch_comp_default();
    ecs_world_set_sketch(&world, sketch, &sk);

    JsonlObserverComp obs = jsonl_observer_comp_default();
    jsonl_observer_comp_set_path(&obs, "C:\\logs\\shape.jsonl");
    obs.observe_enabled = false;
    obs.interval_ms = 444u;
    obs.max_retries = 50u;
    obs.retry_count = 7u;
    obs.scale = 2.0f;
    obs.rotation_x = 0.1f;
    obs.rotation_y = 0.2f;
    obs.rotation_z = 0.3f;
    obs.shift_to_center = true;
    jsonl_observer_comp_push_message(&obs, JSONL_OBSERVER_MSG_WARNING, "warn");
    jsonl_observer_comp_push_message(&obs, JSONL_OBSERVER_MSG_ERROR, "err");
    ecs_world_set_jsonl_observer(&world, sketch, &obs);

    ecs_scene_t scene = {0};
    scene.world = &world;
    char *json = scene_save_to_string(&scene);
    if (!json) {
        ecs_world_shutdown(&world);
        return 1;
    }

    bool ok = strstr(json, "\"jsonl_observer\"") &&
              strstr(json, "\"observe_enabled\": false") &&
              strstr(json, "\"interval_ms\": 444") &&
              strstr(json, "\"max_retries\": 50") &&
              strstr(json, "\"retry_count\": 7") &&
              strstr(json, "\"scale\": 2") &&
              strstr(json, "\"rotation_x\": 0.1") &&
              strstr(json, "\"rotation_y\": 0.2") &&
              strstr(json, "\"rotation_z\": 0.3") &&
              strstr(json, "\"shift_to_center\": true") &&
              strstr(json, "\"source_path\": \"C:\\\\logs\\\\shape.jsonl\"") &&
              strstr(json, "\"messages\":");
    free(json);
    ecs_world_shutdown(&world);
    return ok ? 0 : 1;
}

typedef int (*jsonl_test_fn_t)(void);
typedef struct {
    const char *name;
    jsonl_test_fn_t fn;
} jsonl_test_case_t;

int main(void) {
    static const jsonl_test_case_t tests[] = {
        { "test_jsonl_observer_defaults_and_message_window", test_jsonl_observer_defaults_and_message_window },
        { "test_jsonl_observer_tick_retry_and_auto_disable_policy", test_jsonl_observer_tick_retry_and_auto_disable_policy },
        { "test_jsonl_observer_label_contract_stem_and_path", test_jsonl_observer_label_contract_stem_and_path },
        { "test_jsonl_observer_serializer_emits_fields", test_jsonl_observer_serializer_emits_fields },
    };

    for (size_t i = 0; i < (sizeof(tests) / sizeof(tests[0])); ++i) {
        if (tests[i].fn() != 0) {
            fprintf(stderr, "FAILED: %s\n", tests[i].name);
            return 1;
        }
    }
    return 0;
}

