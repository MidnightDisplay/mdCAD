//------------------------------------------------------------------------------
// jsonl_sketch_import_job.h - JSONL linked-sketch import/reparse helpers
//------------------------------------------------------------------------------
#ifndef JSONL_SKETCH_IMPORT_JOB_H
#define JSONL_SKETCH_IMPORT_JOB_H

#include "jsonl_loader.h"
#include "ecs/ecs_scene.h"
#include "components/jsonl_observer_comp.h"
#include "components/label_comp.h"
#include "scripting/sketch_script_apply.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    uint32_t geometry_count;
    uint32_t constraint_count;
} jsonl_sketch_counts_t;

static inline uint64_t jsonl_observer_now_ms(void) {
    return scene_solver_now_ms();
}

static inline void jsonl_observer_label_from_path(const char *filepath,
                                                  char *out_name,
                                                  size_t out_name_size) {
    if (!out_name || out_name_size == 0) return;
    out_name[0] = '\0';
    if (!filepath || filepath[0] == '\0') return;

    const char *file = filepath;
    for (const char *p = filepath; *p; p++) {
        if (*p == '/' || *p == '\\') file = p + 1;
    }

    const char *dot = strrchr(file, '.');
    if (dot && dot > file) {
        size_t stem_len = (size_t)(dot - file);
        if (stem_len >= out_name_size) stem_len = out_name_size - 1;
        memcpy(out_name, file, stem_len);
        out_name[stem_len] = '\0';
        return;
    }

    strncpy(out_name, file, out_name_size - 1);
    out_name[out_name_size - 1] = '\0';
}

static inline void jsonl_observer_apply_label_contract(ecs_scene_t *scene,
                                                       ecs_entity_t sketch,
                                                       const char *filepath) {
    if (!scene || sketch == 0) return;
    char stem[LABEL_NAME_MAX] = {0};
    jsonl_observer_label_from_path(filepath, stem, sizeof(stem));
    LabelComp label = label_comp_make(stem[0] ? stem : "Sketch", filepath ? filepath : "");
    ecs_world_set_label(scene->world, sketch, &label);
}

static inline void jsonl_observer_snapshot_settings(const JsonlObserverComp *src,
                                                    JsonlObserverComp *dst) {
    if (!src || !dst) return;
    *dst = *src;
}

static inline void jsonl_observer_restore_settings(JsonlObserverComp *dst,
                                                   const JsonlObserverComp *src) {
    if (!src || !dst) return;
    // Preserve persisted user options and linkage state; runtime counters/messages can be replaced too.
    *dst = *src;
}

static inline bool jsonl_sketch_import_apply_element_to_sketch(ecs_scene_t *scene,
                                                               ecs_entity_t sketch,
                                                               const jsonl_element_t *elem,
                                                               const JsonlObserverComp *settings) {
    if (!scene || sketch == 0 || !elem) return false;

    float scale = (settings && settings->scale > 0.0f) ? settings->scale : 1.0f;
    float rx = settings ? settings->rotation_x : 0.0f;
    float ry = settings ? settings->rotation_y : 0.0f;
    float rz = settings ? settings->rotation_z : 0.0f;
    bool shift = settings ? settings->shift_to_center : false;
    (void)rx; (void)ry; (void)rz; (void)shift; // Reserved for full transform matrix parity.

    switch (elem->type) {
        case JSONL_GEOM_POINT: {
            vec3_t p = vec3_scale(elem->data.point.point, scale);
            return scene_add_point_to_sketch(scene, sketch, p, elem->colour, 0.01f) != 0;
        }
        case JSONL_GEOM_LINE: {
            vec3_t a = vec3_scale(elem->data.line.start, scale);
            vec3_t b = vec3_scale(elem->data.line.end, scale);
            return scene_add_line_to_sketch(scene, sketch, a, b, elem->colour, 1.0f) != 0;
        }
        case JSONL_GEOM_ARC: {
            vec3_t c = vec3_scale(elem->data.arc.center, scale);
            float r = elem->data.arc.radius * scale;
            return scene_add_arc_to_sketch(scene, sketch, c, r,
                                           elem->data.arc.start_angle, elem->data.arc.end_angle,
                                           elem->data.arc.normal, elem->colour, 1.0f) != 0;
        }
        case JSONL_GEOM_POLYLINE:
        case JSONL_GEOM_POLYGON: {
            int count = elem->data.polyline.count;
            if (count < 2) return true;
            int segment_count = (elem->type == JSONL_GEOM_POLYGON) ? count : (count - 1);
            for (int i = 0; i < segment_count; i++) {
                int j = (i + 1) % count;
                vec3_t a = vec3_scale(elem->data.polyline.points[i], scale);
                vec3_t b = vec3_scale(elem->data.polyline.points[j], scale);
                if (!scene_add_line_to_sketch(scene, sketch, a, b, elem->colour, 1.0f)) {
                    return false;
                }
            }
            return true;
        }
        case JSONL_GEOM_MESH:
            // Explicitly ignored in sketch import path (D-19).
            return true;
        default:
            return true;
    }
}

static inline bool jsonl_sketch_import_apply_data_to_sketch(ecs_scene_t *scene,
                                                             ecs_entity_t sketch,
                                                             const jsonl_data_t *data,
                                                             const JsonlObserverComp *settings,
                                                             jsonl_sketch_counts_t *out_counts) {
    if (!scene || sketch == 0 || !data) return false;
    jsonl_sketch_counts_t counts = {0};
    for (int e = 0; e < data->entry_count; e++) {
        const jsonl_log_entry_t *entry = &data->entries[e];
        for (int i = 0; i < entry->element_count; i++) {
            if (!jsonl_sketch_import_apply_element_to_sketch(scene, sketch, &entry->elements[i], settings)) {
                return false;
            }
            if (entry->elements[i].type != JSONL_GEOM_MESH) {
                if (entry->elements[i].type == JSONL_GEOM_POLYGON) {
                    counts.geometry_count += (uint32_t)entry->elements[i].data.polyline.count;
                } else if (entry->elements[i].type == JSONL_GEOM_POLYLINE) {
                    counts.geometry_count += (uint32_t)(entry->elements[i].data.polyline.count - 1);
                } else {
                    counts.geometry_count += 1u;
                }
            }
        }
    }
    if (out_counts) *out_counts = counts;
    return true;
}

typedef struct {
    bool success;
    uint32_t replaced_geometry_count;
    uint32_t replaced_constraint_count;
    char error_message[JSONL_OBSERVER_MESSAGE_MAX];
} jsonl_reparse_result_t;

static inline bool jsonl_sketch_snapshot_children(ecs_scene_t *scene,
                                                  ecs_entity_t sketch,
                                                  ecs_entity_t **out_children,
                                                  int *out_count) {
    if (!scene || sketch == 0 || !out_children || !out_count) return false;
    *out_children = NULL;
    *out_count = 0;

    int cap = 32;
    int count = 0;
    ecs_entity_t *children = (ecs_entity_t*)malloc(sizeof(ecs_entity_t) * cap);
    if (!children) return false;

    ecs_iter_t it = ecs_children(scene->world->world, sketch);
    while (ecs_children_next(&it)) {
        for (int i = 0; i < it.count; i++) {
            ecs_entity_t child = it.entities[i];
            if (!ecs_world_get_geometry(scene->world, child) &&
                !ecs_world_get_constraint(scene->world, child)) {
                continue;
            }
            if (count >= cap) {
                cap *= 2;
                ecs_entity_t *next = (ecs_entity_t*)realloc(children, sizeof(ecs_entity_t) * cap);
                if (!next) {
                    free(children);
                    return false;
                }
                children = next;
            }
            children[count++] = child;
        }
    }

    *out_children = children;
    *out_count = count;
    return true;
}

static inline void jsonl_sketch_remove_children(ecs_scene_t *scene,
                                                const ecs_entity_t *children,
                                                int count) {
    if (!scene || !children) return;
    for (int i = 0; i < count; i++) {
        if (children[i] != 0 && ecs_is_alive(scene->world->world, children[i])) {
            scene_remove_entity(scene, children[i]);
        }
    }
}

static inline jsonl_reparse_result_t jsonl_sketch_reparse_transactional(ecs_scene_t *scene,
                                                                         ecs_entity_t sketch,
                                                                         JsonlObserverComp *observer) {
    // authoritative reparse semantics:
    // - success: commit complete replacement
    // - failure: rollback by discarding staged children and keeping last-good state
    jsonl_reparse_result_t out;
    memset(&out, 0, sizeof(out));
    if (!scene || sketch == 0 || !observer || observer->source_path[0] == '\0') {
        snprintf(out.error_message, sizeof(out.error_message), "Observer source path not set.");
        return out;
    }

    jsonl_parse_state_t parse = {0};
    jsonl_error_t err = jsonl_open(observer->source_path, &parse);
    if (err != JSONL_OK) {
        snprintf(out.error_message, sizeof(out.error_message), "JSONL open failed: %s", jsonl_error_string(err));
        return out;
    }

    while (!jsonl_is_complete(&parse) && parse.error == JSONL_OK) {
        (void)jsonl_parse_lines_chunk(&parse, 64);
    }
    jsonl_close(&parse);
    if (parse.error != JSONL_OK) {
        snprintf(out.error_message, sizeof(out.error_message), "JSONL parse failed: %s", jsonl_error_string(parse.error));
        jsonl_parse_state_free(&parse);
        return out;
    }

    ecs_entity_t *old_children = NULL;
    int old_child_count = 0;
    if (!jsonl_sketch_snapshot_children(scene, sketch, &old_children, &old_child_count)) {
        snprintf(out.error_message, sizeof(out.error_message), "Failed snapshotting sketch children.");
        jsonl_parse_state_free(&parse);
        return out;
    }

    JsonlObserverComp settings_copy;
    jsonl_observer_snapshot_settings(observer, &settings_copy);

    // Stage new geometry first. Commit by deleting old only after successful build.
    ecs_entity_t *new_children = NULL;
    int new_count = 0;
    int new_cap = 0;

    bool applied = true;
    jsonl_sketch_counts_t counts = {0};
    for (int e = 0; e < parse.data.entry_count && applied; e++) {
        const jsonl_log_entry_t *entry = &parse.data.entries[e];
        for (int i = 0; i < entry->element_count && applied; i++) {
            const jsonl_element_t *elem = &entry->elements[i];
            int before = scene_count_sketch_geometry(scene, sketch);
            if (!jsonl_sketch_import_apply_element_to_sketch(scene, sketch, elem, &settings_copy)) {
                applied = false;
                break;
            }
            int after = scene_count_sketch_geometry(scene, sketch);
            if (after > before) {
                // Capture latest created geometry by scanning children and picking unlised candidates.
                ecs_iter_t it = ecs_children(scene->world->world, sketch);
                while (ecs_children_next(&it)) {
                    for (int c = 0; c < it.count; c++) {
                        ecs_entity_t child = it.entities[c];
                        if (!ecs_world_get_geometry(scene->world, child)) continue;
                        bool is_old = false;
                        for (int k = 0; k < old_child_count; k++) {
                            if (old_children[k] == child) { is_old = true; break; }
                        }
                        bool already_new = false;
                        for (int k = 0; k < new_count; k++) {
                            if (new_children[k] == child) { already_new = true; break; }
                        }
                        if (!is_old && !already_new) {
                            if (new_count >= new_cap) {
                                new_cap = new_cap ? (new_cap * 2) : 32;
                                ecs_entity_t *next = (ecs_entity_t*)realloc(new_children, sizeof(ecs_entity_t) * new_cap);
                                if (!next) {
                                    applied = false;
                                    break;
                                }
                                new_children = next;
                            }
                            new_children[new_count++] = child;
                        }
                    }
                    if (!applied) break;
                }
            }
            if (elem->type != JSONL_GEOM_MESH) {
                if (elem->type == JSONL_GEOM_POLYGON) counts.geometry_count += (uint32_t)elem->data.polyline.count;
                else if (elem->type == JSONL_GEOM_POLYLINE) counts.geometry_count += (uint32_t)(elem->data.polyline.count - 1);
                else counts.geometry_count += 1u;
            }
        }
    }

    if (!applied) {
        jsonl_sketch_remove_children(scene, new_children, new_count);
        free(new_children);
        free(old_children);
        snprintf(out.error_message, sizeof(out.error_message), "Reparse failed; rollback kept last good state.");
        jsonl_parse_state_free(&parse);
        return out;
    }

    jsonl_sketch_remove_children(scene, old_children, old_child_count);
    free(new_children);
    free(old_children);

    // Successful commit: reset retry counters, maintain observe flag/settings, refresh script.
    observer->retry_count = 0u;
    observer->next_retry_at_ms = 0u;
    jsonl_observer_restore_settings(observer, &settings_copy);
    scene_refresh_sketch_metadata(scene, sketch);
    scene_script_reemit_for_sketch(scene, sketch);

    out.success = true;
    out.replaced_geometry_count = counts.geometry_count;
    out.replaced_constraint_count = counts.constraint_count;

    jsonl_parse_state_free(&parse);
    return out;
}

#endif // JSONL_SKETCH_IMPORT_JOB_H

