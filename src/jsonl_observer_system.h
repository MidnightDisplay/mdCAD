//------------------------------------------------------------------------------
// jsonl_observer_system.h - Linked JSONL observer tick + manual reparse
//------------------------------------------------------------------------------
#ifndef JSONL_OBSERVER_SYSTEM_H
#define JSONL_OBSERVER_SYSTEM_H

#include "ecs/ecs_scene.h"
#include "selection.h"
#include "jsonl_import_job.h"
#include "jsonl_sketch_import_job.h"
#include "components/jsonl_observer_comp.h"

#define JSONL_OBSERVER_MAX_FLAT_REFRESHES 8
#define JSONL_OBSERVER_ADVISORY_REFRESH_MS 250u

typedef struct {
    bool active;
    bool pending_rerun;
    ecs_world_state_t *world;
    ecs_entity_t target_root;
    ecs_entity_t stage_root;
    uint64_t started_at_ms;
    jsonl_import_job_t job;
    selection_buffer_t *selection;
} jsonl_flat_refresh_slot_t;

static inline jsonl_flat_refresh_slot_t *jsonl_observer_flat_refresh_slots(void) {
    static jsonl_flat_refresh_slot_t slots[JSONL_OBSERVER_MAX_FLAT_REFRESHES];
    return slots;
}

static inline bool jsonl_observer_is_descendant_of(ecs_scene_t *scene,
                                                   ecs_entity_t entity,
                                                   ecs_entity_t ancestor) {
    if (!scene || entity == 0 || ancestor == 0) return false;
    ecs_entity_t current = entity;
    while (current != 0 && ecs_is_alive(scene->world->world, current)) {
        if (current == ancestor) return true;
        current = scene_get_parent(scene, current);
    }
    return false;
}

static inline bool jsonl_observer_selected_descendant_replaced(const jsonl_flat_refresh_slot_t *slot,
                                                               ecs_scene_t *scene) {
    if (!slot || !slot->selection || !scene || slot->target_root == 0) return false;
    selection_buffer_t *sel = slot->selection;
    for (int i = 0; i < selection_count(sel); i++) {
        ecs_entity_t selected = selection_get(sel, i);
        if (selected == 0 || selected == slot->target_root) continue;
        if (jsonl_observer_is_descendant_of(scene, selected, slot->target_root)) {
            return true;
        }
    }
    return false;
}

static inline jsonl_flat_refresh_slot_t *jsonl_observer_find_flat_refresh_slot(ecs_scene_t *scene,
                                                                                ecs_entity_t root_entity) {
    if (!scene || root_entity == 0) return NULL;
    jsonl_flat_refresh_slot_t *slots = jsonl_observer_flat_refresh_slots();
    for (int i = 0; i < JSONL_OBSERVER_MAX_FLAT_REFRESHES; i++) {
        if (slots[i].active && slots[i].world == scene->world && slots[i].target_root == root_entity) {
            return &slots[i];
        }
    }
    return NULL;
}

static inline jsonl_flat_refresh_slot_t *jsonl_observer_alloc_flat_refresh_slot(void) {
    jsonl_flat_refresh_slot_t *slots = jsonl_observer_flat_refresh_slots();
    for (int i = 0; i < JSONL_OBSERVER_MAX_FLAT_REFRESHES; i++) {
        if (!slots[i].active) return &slots[i];
    }
    return NULL;
}

static inline bool jsonl_observer_commit_flat_refresh(ecs_scene_t *scene, jsonl_flat_refresh_slot_t *slot) {
    if (!scene || !slot || slot->target_root == 0 || slot->stage_root == 0) return false;

    int old_child_count = scene_count_children(scene, slot->target_root);
    int new_child_count = scene_count_children(scene, slot->stage_root);

    ecs_entity_t *old_children = NULL;
    ecs_entity_t *new_children = NULL;

    if (old_child_count > 0) {
        old_children = (ecs_entity_t *)malloc(sizeof(ecs_entity_t) * (size_t)old_child_count);
        if (!old_children) return false;
    }
    if (new_child_count > 0) {
        new_children = (ecs_entity_t *)malloc(sizeof(ecs_entity_t) * (size_t)new_child_count);
        if (!new_children) {
            free(old_children);
            return false;
        }
    }

    int old_collected = 0;
    int new_collected = 0;
    if (old_child_count > 0) {
        old_collected = scene_get_children(scene, slot->target_root, old_children, old_child_count);
    }
    if (new_child_count > 0) {
        new_collected = scene_get_children(scene, slot->stage_root, new_children, new_child_count);
    }

    bool move_selection_to_root = jsonl_observer_selected_descendant_replaced(slot, scene);

    if (new_collected > 0) {
        scene_set_parent_batch(scene, new_children, new_collected, slot->target_root);
    }
    for (int i = 0; i < old_collected; i++) {
        ecs_entity_t old_child = old_children[i];
        if (old_child != 0 && ecs_is_alive(scene->world->world, old_child)) {
            scene_remove_entity(scene, old_child);
        }
    }
    if (ecs_is_alive(scene->world->world, slot->stage_root)) {
        scene_remove_entity(scene, slot->stage_root);
    }
    slot->stage_root = 0;

    if (move_selection_to_root && slot->selection) {
        selection_set_single(slot->selection, slot->target_root);
    }

    free(old_children);
    free(new_children);
    return true;
}

static inline void jsonl_observer_abort_flat_refresh_stage(ecs_scene_t *scene, jsonl_flat_refresh_slot_t *slot) {
    if (!scene || !slot) return;
    if (slot->stage_root != 0 && ecs_is_alive(scene->world->world, slot->stage_root)) {
        scene_remove_entity(scene, slot->stage_root);
    }
    slot->stage_root = 0;
}

static inline void jsonl_observer_reset_flat_refresh_slot(ecs_scene_t *scene, jsonl_flat_refresh_slot_t *slot) {
    if (!slot) return;
    if (scene) {
        jsonl_observer_abort_flat_refresh_stage(scene, slot);
    }
    jsonl_import_job_reset(&slot->job);
    slot->active = false;
    slot->pending_rerun = false;
    slot->world = NULL;
    slot->target_root = 0;
    slot->started_at_ms = 0u;
    slot->selection = NULL;
}

static inline void jsonl_observer_comp_push_message_unique(JsonlObserverComp *obs,
                                                           jsonl_observer_msg_severity_t severity,
                                                           const char *message) {
    if (!obs || !message) return;
    if (obs->message_count > 0u) {
        uint32_t idx = obs->message_count - 1u;
        if (idx < JSONL_OBSERVER_MESSAGE_HISTORY &&
            obs->message_severity[idx] == (uint8_t)severity &&
            strcmp(obs->messages[idx], message) == 0) {
            return;
        }
    }
    jsonl_observer_comp_push_message(obs, severity, message);
}

static inline void jsonl_observer_apply_retry_policy(JsonlObserverComp *obs,
                                                     uint64_t now_ms,
                                                     const char *warning_message) {
    if (!obs) return;
    if (obs->max_retries == 0u) obs->max_retries = JSONL_OBSERVER_DEFAULT_MAX_RETRIES;
    if (obs->interval_ms == 0u) obs->interval_ms = JSONL_OBSERVER_DEFAULT_INTERVAL_MS;

    if (obs->retry_count < obs->max_retries) {
        obs->retry_count++;
    }
    obs->next_retry_at_ms = now_ms + obs->interval_ms;

    jsonl_observer_comp_push_message_unique(
        obs,
        JSONL_OBSERVER_MSG_WARNING,
        warning_message ? warning_message : "Observer source check failed; will retry.");

    if (obs->retry_count >= obs->max_retries) {
        obs->observe_enabled = false;
        jsonl_observer_comp_push_message_unique(obs, JSONL_OBSERVER_MSG_ERROR,
                                                "Observe auto-disabled after max retries.");
    }
}

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

static inline bool jsonl_observer_request_flat_refresh(ecs_scene_t *scene,
                                                       ecs_entity_t root_entity,
                                                       selection_buffer_t *selection) {
    if (!scene || root_entity == 0 || !ecs_is_alive(scene->world->world, root_entity)) return false;
    if (ecs_world_get_sketch(scene->world, root_entity)) return false;

    JsonlObserverComp *obs = ecs_world_get_jsonl_observer(scene->world, root_entity);
    if (!obs || obs->source_path[0] == '\0') return false;

    jsonl_flat_refresh_slot_t *existing = jsonl_observer_find_flat_refresh_slot(scene, root_entity);
    if (existing) {
        if (!existing->pending_rerun) {
            existing->pending_rerun = true;
            jsonl_observer_comp_push_message_unique(obs, JSONL_OBSERVER_MSG_INFO,
                                                    "Refresh coalesced; pending rerun queued.");
        }
        return false;
    }

    jsonl_flat_refresh_slot_t *slot = jsonl_observer_alloc_flat_refresh_slot();
    if (!slot) {
        jsonl_observer_comp_push_message(obs, JSONL_OBSERVER_MSG_WARNING,
                                         "Refresh queue full; try again shortly.");
        return false;
    }

    memset(slot, 0, sizeof(*slot));
    slot->active = true;
    slot->pending_rerun = false;
    slot->world = scene->world;
    slot->target_root = root_entity;
    slot->started_at_ms = scene_solver_now_ms();
    slot->selection = selection;
    jsonl_import_job_init(&slot->job);
    jsonl_import_job_set_mesh_mode(&slot->job, obs->mesh_import_mode);

    vec4_t default_colour = vec4_make(1.0f, 1.0f, 1.0f, 1.0f);
    bool started = jsonl_import_job_start(&slot->job,
                                          obs->source_path,
                                          obs->scale,
                                          obs->use_jsonl_colours,
                                          default_colour,
                                          obs->shift_to_center,
                                          obs->rotation_x,
                                          obs->rotation_y,
                                          obs->rotation_z);
    if (!started) {
        jsonl_observer_comp_push_message(obs, JSONL_OBSERVER_MSG_WARNING,
                                         slot->job.status_message[0] ? slot->job.status_message
                                                                     : "Failed to start flat refresh.");
        jsonl_observer_reset_flat_refresh_slot(scene, slot);
        return false;
    }

    jsonl_observer_comp_push_message(obs, JSONL_OBSERVER_MSG_INFO, "Flat refresh started.");
    return true;
}

static inline bool jsonl_observer_is_flat_refresh_running(ecs_scene_t *scene, ecs_entity_t root_entity) {
    return jsonl_observer_find_flat_refresh_slot(scene, root_entity) != NULL;
}

static inline void jsonl_observer_tick_flat_refreshes(ecs_scene_t *scene) {
    if (!scene) return;
    jsonl_flat_refresh_slot_t *slots = jsonl_observer_flat_refresh_slots();
    for (int i = 0; i < JSONL_OBSERVER_MAX_FLAT_REFRESHES; i++) {
        jsonl_flat_refresh_slot_t *slot = &slots[i];
        if (!slot->active) continue;

        bool done = jsonl_import_job_tick(&slot->job, scene);
        if (!done) continue;

        bool rerun_pending = slot->pending_rerun;
        ecs_entity_t rerun_root = slot->target_root;
        selection_buffer_t *rerun_selection = slot->selection;
        uint64_t elapsed_ms = 0u;
        uint64_t now_ms = scene_solver_now_ms();
        if (slot->started_at_ms != 0u && now_ms >= slot->started_at_ms) {
            elapsed_ms = now_ms - slot->started_at_ms;
        }

        JsonlObserverComp *obs = NULL;
        if (slot->target_root != 0 && ecs_is_alive(scene->world->world, slot->target_root)) {
            obs = ecs_world_get_jsonl_observer(scene->world, slot->target_root);
        }

        if (slot->job.state == JSONL_JOB_COMPLETE && slot->job.root_entity != 0) {
            slot->stage_root = slot->job.root_entity;
            if (jsonl_observer_commit_flat_refresh(scene, slot)) {
                if (obs) {
                    if (obs->source_path[0] != '\0') {
                        (void)jsonl_observer_stamp_source_state(obs, obs->source_path);
                    }
                    char msg[JSONL_OBSERVER_MESSAGE_MAX];
                    snprintf(msg, sizeof(msg), "Re-import applied (%u geometry, %llums).",
                             (unsigned)slot->job.total_entities_created,
                             (unsigned long long)elapsed_ms);
                    jsonl_observer_comp_push_message(obs, JSONL_OBSERVER_MSG_INFO, msg);
                    if (elapsed_ms >= JSONL_OBSERVER_ADVISORY_REFRESH_MS) {
                        char advisory[JSONL_OBSERVER_MESSAGE_MAX];
                        snprintf(advisory, sizeof(advisory),
                                 "Advisory: refresh took %llums (timing evidence only).",
                                 (unsigned long long)elapsed_ms);
                        jsonl_observer_comp_push_message_unique(obs, JSONL_OBSERVER_MSG_WARNING, advisory);
                    }
                }
            } else {
                jsonl_observer_abort_flat_refresh_stage(scene, slot);
                if (obs) {
                    char msg[JSONL_OBSERVER_MESSAGE_MAX];
                    snprintf(msg, sizeof(msg), "Flat refresh failed after %llums; kept last-good content.",
                             (unsigned long long)elapsed_ms);
                    jsonl_observer_comp_push_message(obs, JSONL_OBSERVER_MSG_WARNING, msg);
                }
            }
        } else {
            jsonl_observer_abort_flat_refresh_stage(scene, slot);
            if (obs) {
                if (slot->job.status_message[0]) {
                    char msg[JSONL_OBSERVER_MESSAGE_MAX];
                    snprintf(msg, sizeof(msg), "%s (%llums).", slot->job.status_message,
                             (unsigned long long)elapsed_ms);
                    jsonl_observer_comp_push_message(obs, JSONL_OBSERVER_MSG_WARNING, msg);
                } else {
                    char msg[JSONL_OBSERVER_MESSAGE_MAX];
                    snprintf(msg, sizeof(msg), "Flat refresh failed after %llums; kept last-good content.",
                             (unsigned long long)elapsed_ms);
                    jsonl_observer_comp_push_message(obs, JSONL_OBSERVER_MSG_WARNING, msg);
                }
            }
        }

        jsonl_observer_reset_flat_refresh_slot(scene, slot);
        if (rerun_pending && rerun_root != 0 && ecs_is_alive(scene->world->world, rerun_root)) {
            if (!jsonl_observer_request_flat_refresh(scene, rerun_root, rerun_selection)) {
                JsonlObserverComp *rerun_obs = ecs_world_get_jsonl_observer(scene->world, rerun_root);
                if (rerun_obs) {
                    jsonl_observer_comp_push_message_unique(rerun_obs, JSONL_OBSERVER_MSG_WARNING,
                                                            "Coalesced rerun could not start.");
                }
            }
        }
    }
}

static inline bool jsonl_observer_tick_one_flat(ecs_scene_t *scene,
                                                ecs_entity_t root_entity,
                                                uint64_t now_ms,
                                                selection_buffer_t *selection) {
    if (!scene || root_entity == 0) return false;
    if (ecs_world_get_sketch(scene->world, root_entity)) return false;

    JsonlObserverComp *obs = ecs_world_get_jsonl_observer(scene->world, root_entity);
    if (!obs || !obs->linked || !obs->observe_enabled) return false;

    if (obs->max_retries == 0u) obs->max_retries = JSONL_OBSERVER_DEFAULT_MAX_RETRIES;
    if (obs->interval_ms == 0u) obs->interval_ms = JSONL_OBSERVER_DEFAULT_INTERVAL_MS;
    if (obs->scale <= 0.0f) obs->scale = 1.0f;

    if (obs->next_retry_at_ms != 0u && now_ms < obs->next_retry_at_ms) {
        return false;
    }

    if (obs->source_path[0] == '\0') {
        obs->next_retry_at_ms = now_ms + obs->interval_ms;
        jsonl_observer_comp_push_message_unique(
            obs,
            JSONL_OBSERVER_MSG_WARNING,
            "Observe is ON but source path is missing; waiting for a valid path.");
        return false;
    }

    bool source_changed = true;
    if (!jsonl_observer_source_changed(obs, obs->source_path, &source_changed)) {
        jsonl_observer_apply_retry_policy(obs, now_ms, "Observer source check failed; will retry.");
        return false;
    }

    if (!source_changed) {
        obs->retry_count = 0u;
        obs->next_retry_at_ms = now_ms + obs->interval_ms;
        return false;
    }

    if (jsonl_observer_is_flat_refresh_running(scene, root_entity)) {
        (void)jsonl_observer_request_flat_refresh(scene, root_entity, selection);
        obs->next_retry_at_ms = now_ms + obs->interval_ms;
        return false;
    }

    if (jsonl_observer_request_flat_refresh(scene, root_entity, selection)) {
        obs->retry_count = 0u;
        obs->next_retry_at_ms = now_ms + obs->interval_ms;
        return true;
    }

    jsonl_observer_apply_retry_policy(obs, now_ms, "Observed re-import failed.");
    return false;
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

    bool source_changed = true;
    if (jsonl_observer_source_changed(obs, obs->source_path, &source_changed)) {
        if (!source_changed) {
            obs->retry_count = 0u;
            obs->next_retry_at_ms = now_ms + obs->interval_ms;
            return false;
        }
    } else {
        if (obs->retry_count < obs->max_retries) {
            obs->retry_count++;
        }
        obs->next_retry_at_ms = now_ms + obs->interval_ms;
        jsonl_observer_comp_push_message(obs, JSONL_OBSERVER_MSG_WARNING,
                                         "Observer source check failed; will retry.");
        if (obs->retry_count >= obs->max_retries) {
            obs->observe_enabled = false;
            jsonl_observer_comp_push_message(obs, JSONL_OBSERVER_MSG_ERROR,
                                             "Observe auto-disabled after max retries.");
        }
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

static inline void jsonl_observer_system_tick(ecs_scene_t *scene,
                                              uint64_t now_ms,
                                              selection_buffer_t *selection) {
    if (!scene) return;
    ecs_world_state_t *w = scene->world;
    ecs_query_t *q = ecs_query(w->world, {
        .terms = {
            { .id = w->JsonlObserverComp_id }
        }
    });
    ecs_iter_t it = ecs_query_iter(w->world, q);
    while (ecs_query_next(&it)) {
        for (int i = 0; i < it.count; i++) {
            ecs_entity_t e = it.entities[i];
            if (ecs_world_get_sketch(w, e)) {
                (void)jsonl_observer_tick_one(scene, e, now_ms);
            } else {
                (void)jsonl_observer_tick_one_flat(scene, e, now_ms, selection);
            }
        }
    }
    ecs_query_fini(q);
}

#endif // JSONL_OBSERVER_SYSTEM_H

