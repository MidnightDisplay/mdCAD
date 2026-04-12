//------------------------------------------------------------------------------
// jsonl_observer_system.h - Linked JSONL observer tick + manual reparse
//------------------------------------------------------------------------------
#ifndef JSONL_OBSERVER_SYSTEM_H
#define JSONL_OBSERVER_SYSTEM_H

#include "ecs/ecs_scene.h"
#include "jsonl_sketch_import_job.h"
#include "components/jsonl_observer_comp.h"

static inline JsonlObserverComp* jsonl_observer_ensure_for_sketch(ecs_scene_t *scene, ecs_entity_t sketch) {
    if (!scene || sketch == 0) return NULL;
    JsonlObserverComp *obs = ecs_world_get_jsonl_observer(scene->world, sketch);
    if (obs) {
        if (obs->max_retries == 0u) obs->max_retries = JSONL_OBSERVER_DEFAULT_MAX_RETRIES;
        if (obs->interval_ms == 0u) obs->interval_ms = JSONL_OBSERVER_DEFAULT_INTERVAL_MS;
        if (obs->scale <= 0.0f) obs->scale = 1.0f;
        return obs;
    }
    JsonlObserverComp init = jsonl_observer_comp_default();
    ecs_world_set_jsonl_observer(scene->world, sketch, &init);
    return ecs_world_get_jsonl_observer(scene->world, sketch);
}

static inline bool jsonl_observer_link_sketch(ecs_scene_t *scene, ecs_entity_t sketch, const char *filepath) {
    if (!scene || sketch == 0 || !filepath || filepath[0] == '\0') return false;
    JsonlObserverComp *obs = jsonl_observer_ensure_for_sketch(scene, sketch);
    if (!obs) return false;
    jsonl_observer_comp_set_path(obs, filepath);
    obs->observe_enabled = true;
    obs->retry_count = 0;
    obs->next_retry_at_ms = 0;
    if (obs->max_retries == 0u) obs->max_retries = JSONL_OBSERVER_DEFAULT_MAX_RETRIES;
    if (obs->interval_ms == 0u) obs->interval_ms = JSONL_OBSERVER_DEFAULT_INTERVAL_MS;
    if (obs->scale <= 0.0f) obs->scale = 1.0f;
    jsonl_observer_apply_label_contract(scene, sketch, filepath);
    return true;
}

static inline bool jsonl_observer_relink(ecs_scene_t *scene, ecs_entity_t sketch, const char *filepath) {
    if (!scene || sketch == 0 || !filepath || filepath[0] == '\0') return false;
    JsonlObserverComp *obs = jsonl_observer_ensure_for_sketch(scene, sketch);
    if (!obs) return false;

    bool prev_observe_enabled = obs->observe_enabled;
    uint32_t prev_interval_ms = obs->interval_ms;
    float prev_scale = obs->scale;
    float prev_rx = obs->rotation_x;
    float prev_ry = obs->rotation_y;
    float prev_rz = obs->rotation_z;
    bool prev_shift = obs->shift_to_center;

    jsonl_observer_comp_set_path(obs, filepath); // relink source path
    obs->observe_enabled = prev_observe_enabled;
    obs->interval_ms = prev_interval_ms ? prev_interval_ms : JSONL_OBSERVER_DEFAULT_INTERVAL_MS;
    obs->scale = (prev_scale > 0.0f) ? prev_scale : 1.0f;
    obs->rotation_x = prev_rx;
    obs->rotation_y = prev_ry;
    obs->rotation_z = prev_rz;
    obs->shift_to_center = prev_shift;
    obs->retry_count = 0;
    obs->next_retry_at_ms = 0;

    jsonl_observer_apply_label_contract(scene, sketch, filepath);
    // Label.name + Label.description contract is applied via helper above.
    jsonl_observer_comp_push_message(obs, JSONL_OBSERVER_MSG_INFO, "Relinked JSONL source.");
    return true;
}

static inline bool jsonl_observer_manual_reparse(ecs_scene_t *scene, ecs_entity_t sketch) {
    if (!scene || sketch == 0) return false;
    JsonlObserverComp *obs = jsonl_observer_ensure_for_sketch(scene, sketch);
    if (!obs || !obs->linked || obs->source_path[0] == '\0') return false;

    jsonl_reparse_result_t result = jsonl_sketch_reparse_transactional(scene, sketch, obs);
    if (result.success) {
        char msg[JSONL_OBSERVER_MESSAGE_MAX];
        snprintf(msg, sizeof(msg), "Manual reparse applied (%u geometry).",
                 (unsigned)result.replaced_geometry_count);
        jsonl_observer_comp_push_message(obs, JSONL_OBSERVER_MSG_INFO, msg);
        return true;
    }

    jsonl_observer_comp_push_message(obs, JSONL_OBSERVER_MSG_WARNING,
                                     result.error_message[0] ? result.error_message : "Manual reparse failed.");
    return false;
}

static inline bool jsonl_observer_tick_one(ecs_scene_t *scene, ecs_entity_t sketch, uint64_t now_ms) {
    if (!scene || sketch == 0) return false;
    JsonlObserverComp *obs = jsonl_observer_ensure_for_sketch(scene, sketch);
    if (!obs || !obs->linked || !obs->observe_enabled) return false;
    if (obs->source_path[0] == '\0') return false;

    if (obs->max_retries == 0u) obs->max_retries = JSONL_OBSERVER_DEFAULT_MAX_RETRIES;
    if (obs->interval_ms == 0u) obs->interval_ms = JSONL_OBSERVER_DEFAULT_INTERVAL_MS;

    if (obs->next_retry_at_ms != 0u && now_ms < obs->next_retry_at_ms) {
        return false;
    }

    jsonl_reparse_result_t result = jsonl_sketch_reparse_transactional(scene, sketch, obs);
    if (result.success) {
        obs->retry_count = 0u;
        obs->next_retry_at_ms = now_ms + obs->interval_ms; // D-11: debounce uses same rate-limit value.
        char msg[JSONL_OBSERVER_MESSAGE_MAX];
        snprintf(msg, sizeof(msg), "Observed update applied (%u geometry).",
                 (unsigned)result.replaced_geometry_count);
        jsonl_observer_comp_push_message(obs, JSONL_OBSERVER_MSG_INFO, msg);
        return true;
    }

    if (obs->retry_count < obs->max_retries) {
        obs->retry_count++;
    }
    obs->next_retry_at_ms = now_ms + obs->interval_ms; // same slider interval
    jsonl_observer_comp_push_message(obs, JSONL_OBSERVER_MSG_WARNING,
                                     result.error_message[0] ? result.error_message : "Observed reparse failed.");

    if (obs->retry_count >= obs->max_retries) {
        obs->observe_enabled = false; // D-12 auto-disable at exhaustion.
        jsonl_observer_comp_push_message(obs, JSONL_OBSERVER_MSG_ERROR,
                                         "Observe auto-disabled after max retries.");
    }
    return false;
}

static inline void jsonl_observer_system_tick(ecs_scene_t *scene, uint64_t now_ms) {
    if (!scene) return;
    ecs_world_state_t *w = scene->world;
    ecs_query_t *q = ecs_query(w->world, {
        .terms = {
            { .id = w->SketchComp_id },
            { .id = w->JsonlObserverComp_id }
        }
    });
    ecs_iter_t it = ecs_query_iter(w->world, q);
    while (ecs_query_next(&it)) {
        for (int i = 0; i < it.count; i++) {
            (void)jsonl_observer_tick_one(scene, it.entities[i], now_ms);
        }
    }
    ecs_query_fini(q);
}

#endif // JSONL_OBSERVER_SYSTEM_H

