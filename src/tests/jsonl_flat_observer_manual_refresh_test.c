#include "../ecs/ecs_world.h"
#include "../ecs/ecs_scene.h"
#include "../jsonl_import_job.h"
#include "../jsonl_observer_system.h"
#include "../scene_serializer.h"
#include "../selection.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif
void sokol_main(void) {}
#ifdef __cplusplus
}
#endif

static bool write_jsonl_fixture_lines(const char *path, const char *const *lines, int line_count) {
    FILE *f = fopen(path, "wb");
    if (!f) return false;
    for (int i = 0; i < line_count; i++) {
        if (fputs(lines[i], f) < 0 || fputs("\n", f) < 0) {
            fclose(f);
            return false;
        }
    }
    fclose(f);
    return true;
}

static bool write_jsonl_point_fixture(const char *path, int point_count) {
    if (!path || point_count <= 0) return false;
    FILE *f = fopen(path, "wb");
    if (!f) return false;
    for (int i = 0; i < point_count; i++) {
        if (fprintf(f,
                    "{\"Name\":\"Entry_%d\",\"Elements\":[{\"Name\":\"P_%d\",\"Description\":\"\",\"Colour\":\"White\",\"Element\":{\"$type\":\"Geo.Point3D\",\"X\":%d,\"Y\":0,\"Z\":0}}]}\n",
                    i, i, i) < 0) {
            fclose(f);
            return false;
        }
    }
    fclose(f);
    return true;
}

static bool write_jsonl_polyline_fixture(const char *path, int point_count, float y_offset) {
    if (!path || point_count < 2) return false;
    FILE *f = fopen(path, "wb");
    if (!f) return false;

    if (fputs("{\"Name\":\"Entry\",\"Elements\":[{\"Name\":\"Polyline\",\"Description\":\"\",\"Colour\":\"White\",\"Element\":{\"$type\":\"Geo.PolyLine3D\",\"Points\":[", f) < 0) {
        fclose(f);
        return false;
    }

    for (int i = 0; i < point_count; i++) {
        if (i > 0 && fputs(",", f) < 0) {
            fclose(f);
            return false;
        }
        float y = y_offset + ((i % 2) ? 1.0f : 0.0f);
        if (fprintf(f, "{\"X\":%d,\"Y\":%.3f,\"Z\":0}", i, (double)y) < 0) {
            fclose(f);
            return false;
        }
    }

    if (fputs("]}}]}\n", f) < 0) {
        fclose(f);
        return false;
    }

    fclose(f);
    return true;
}

static bool run_flat_import_to_completion(ecs_scene_t *scene,
                                          const char *path,
                                          bool link_enabled,
                                          float scale,
                                          bool use_jsonl_colours,
                                          bool shift_to_center,
                                          float rotation_x,
                                          float rotation_y,
                                          float rotation_z,
                                          int mesh_mode,
                                          jsonl_import_job_t *out_job) {
    if (!scene || !path) return false;

    jsonl_import_job_t job = {0};
    jsonl_import_job_init(&job);
    jsonl_import_job_set_mesh_mode(&job, mesh_mode);

    bool started = jsonl_import_job_start(&job,
                                          path,
                                          scale,
                                          use_jsonl_colours,
                                          vec4_make(1, 1, 1, 1),
                                          shift_to_center,
                                          rotation_x,
                                          rotation_y,
                                          rotation_z);
    if (!started) return false;

    jsonl_import_job_set_observer_contract(&job, link_enabled, path);

    int guard = 20000;
    while (!jsonl_import_job_tick(&job, scene)) {
        if (--guard <= 0) {
            jsonl_import_job_cancel(&job, scene);
            return false;
        }
    }

    if (job.state != JSONL_JOB_COMPLETE || job.root_entity == 0) return false;

    if (out_job) *out_job = job;
    return true;
}

static ecs_entity_t find_first_geometry_under_root(ecs_scene_t *scene, ecs_entity_t root) {
    if (!scene || root == 0) return 0;

    int root_child_count = scene_count_children(scene, root);
    if (root_child_count <= 0) return 0;
    ecs_entity_t *root_children = (ecs_entity_t *)malloc(sizeof(ecs_entity_t) * (size_t)root_child_count);
    if (!root_children) return 0;
    int collected = scene_get_children(scene, root, root_children, root_child_count);

    ecs_entity_t found = 0;
    for (int i = 0; i < collected && found == 0; i++) {
        ecs_entity_t entry = root_children[i];
        int entry_child_count = scene_count_children(scene, entry);
        if (entry_child_count <= 0) continue;
        ecs_entity_t *entry_children = (ecs_entity_t *)malloc(sizeof(ecs_entity_t) * (size_t)entry_child_count);
        if (!entry_children) continue;
        int entry_collected = scene_get_children(scene, entry, entry_children, entry_child_count);
        for (int j = 0; j < entry_collected; j++) {
            ecs_entity_t candidate = entry_children[j];
            if (ecs_world_get_geometry(scene->world, candidate)) {
                found = candidate;
                break;
            }
        }
        free(entry_children);
    }

    free(root_children);
    return found;
}

static int count_geometry_under_root(ecs_scene_t *scene, ecs_entity_t root) {
    if (!scene || root == 0) return 0;

    int root_child_count = scene_count_children(scene, root);
    if (root_child_count <= 0) return 0;
    ecs_entity_t *root_children = (ecs_entity_t *)malloc(sizeof(ecs_entity_t) * (size_t)root_child_count);
    if (!root_children) return 0;
    int root_collected = scene_get_children(scene, root, root_children, root_child_count);

    int geometry_count = 0;
    for (int i = 0; i < root_collected; i++) {
        ecs_entity_t entry = root_children[i];
        int entry_child_count = scene_count_children(scene, entry);
        if (entry_child_count <= 0) continue;
        ecs_entity_t *entry_children = (ecs_entity_t *)malloc(sizeof(ecs_entity_t) * (size_t)entry_child_count);
        if (!entry_children) continue;
        int entry_collected = scene_get_children(scene, entry, entry_children, entry_child_count);
        for (int j = 0; j < entry_collected; j++) {
            if (ecs_world_get_geometry(scene->world, entry_children[j])) geometry_count++;
        }
        free(entry_children);
    }

    free(root_children);
    return geometry_count;
}

typedef struct {
    int line_slots;
    int point_slots;
} slot_footprint_t;

static void accumulate_renderable_slot_footprint(const RenderableComp *r, slot_footprint_t *footprint) {
    if (!r || !footprint || r->instance_slot == 0xFFFFFFFF) return;

    switch ((geometry_type_t)r->batch_id) {
        case GEOM_LINE:
            footprint->line_slots += 1;
            break;
        case GEOM_POINT:
            footprint->point_slots += 1;
            break;
        case GEOM_POLYLINE:
        case GEOM_POLYGON:
        case GEOM_ARC:
        case GEOM_BEZIER:
        case GEOM_HELIX:
            footprint->line_slots += (int)r->segment_count;
            footprint->point_slots += (int)r->join_count;
            break;
        case GEOM_POINT_CLOUD:
            footprint->point_slots += (int)r->segment_count;
            break;
        default:
            break;
    }
}

static slot_footprint_t count_slot_footprint_under_root(ecs_scene_t *scene, ecs_entity_t root) {
    slot_footprint_t footprint = {0};
    if (!scene || root == 0) return footprint;

    int root_child_count = scene_count_children(scene, root);
    if (root_child_count <= 0) return footprint;

    ecs_entity_t *root_children = (ecs_entity_t *)malloc(sizeof(ecs_entity_t) * (size_t)root_child_count);
    if (!root_children) return footprint;
    int root_collected = scene_get_children(scene, root, root_children, root_child_count);

    for (int i = 0; i < root_collected; i++) {
        ecs_entity_t entry = root_children[i];
        int entry_child_count = scene_count_children(scene, entry);
        if (entry_child_count <= 0) continue;

        ecs_entity_t *entry_children = (ecs_entity_t *)malloc(sizeof(ecs_entity_t) * (size_t)entry_child_count);
        if (!entry_children) continue;
        int entry_collected = scene_get_children(scene, entry, entry_children, entry_child_count);
        for (int j = 0; j < entry_collected; j++) {
            RenderableComp *r = ecs_world_get_renderable(scene->world, entry_children[j]);
            accumulate_renderable_slot_footprint(r, &footprint);
        }
        free(entry_children);
    }

    free(root_children);
    return footprint;
}

static bool tick_refresh_until_idle(ecs_scene_t *scene, ecs_entity_t root_entity, int max_ticks) {
    for (int i = 0; i < max_ticks; i++) {
        if (!jsonl_observer_is_flat_refresh_running(scene, root_entity)) {
            return true;
        }
        jsonl_observer_tick_flat_refreshes(scene);
    }
    return !jsonl_observer_is_flat_refresh_running(scene, root_entity);
}

static bool observer_contains_message(const JsonlObserverComp *observer, const char *needle) {
    if (!observer || !needle || needle[0] == '\0') return false;
    for (uint32_t i = 0u; i < observer->message_count && i < JSONL_OBSERVER_MESSAGE_HISTORY; i++) {
        if (strstr(observer->messages[i], needle) != NULL) return true;
    }
    return false;
}

static bool run_flat_import_until_parenting(ecs_scene_t *scene,
                                            const char *path,
                                            bool link_enabled,
                                            float scale,
                                            bool use_jsonl_colours,
                                            bool shift_to_center,
                                            float rotation_x,
                                            float rotation_y,
                                            float rotation_z,
                                            int mesh_mode,
                                            jsonl_import_job_t *out_job) {
    if (!scene || !path || !out_job) return false;

    jsonl_import_job_t job = {0};
    jsonl_import_job_init(&job);
    jsonl_import_job_set_mesh_mode(&job, mesh_mode);

    bool started = jsonl_import_job_start(&job,
                                          path,
                                          scale,
                                          use_jsonl_colours,
                                          vec4_make(1, 1, 1, 1),
                                          shift_to_center,
                                          rotation_x,
                                          rotation_y,
                                          rotation_z);
    if (!started) return false;

    jsonl_import_job_set_observer_contract(&job, link_enabled, path);

    int guard = 20000;
    while (job.state != JSONL_JOB_PARENTING_ENTITIES) {
        if (jsonl_import_job_tick(&job, scene)) {
            return false;
        }
        if (--guard <= 0) {
            jsonl_import_job_cancel(&job, scene);
            return false;
        }
    }

    *out_job = job;
    return true;
}

static int verify_loaded_observer_metadata(const char *json, const char *source_path) {
    if (!json || !source_path) return 1;

    ecs_world_state_t *world = (ecs_world_state_t *)calloc(1, sizeof(ecs_world_state_t));
    ecs_scene_t *scene = (ecs_scene_t *)calloc(1, sizeof(ecs_scene_t));
    if (!world || !scene) {
        free(world);
        free(scene);
        return 1;
    }
    ecs_world_init(world);
    ecs_scene_init(scene, world);

    int failed = 0;
    if (scene_load_from_string(scene, json, true) <= 0) {
        fprintf(stderr, "  fail: scene_load_from_string failed\n");
        failed = 1;
    }

    if (!failed) {
        char *resaved = scene_save_to_string(scene);
        if (!resaved) {
            fprintf(stderr, "  fail: re-save after load returned null\n");
            failed = 1;
        } else {
            if (strstr(resaved, "\"jsonl_observer\"") == NULL) {
                fprintf(stderr, "  fail: re-saved scene missing jsonl_observer\n");
                failed = 1;
            }
            if (strstr(resaved, "\"linked\": false") == NULL) {
                fprintf(stderr, "  fail: re-saved scene missing linked=false\n");
                failed = 1;
            }
            if (strstr(resaved, "\"use_jsonl_colours\": false") == NULL) {
                fprintf(stderr, "  fail: re-saved scene missing use_jsonl_colours=false\n");
                failed = 1;
            }
            if (strstr(resaved, "\"mesh_import_mode\": 1") == NULL) {
                fprintf(stderr, "  fail: re-saved scene missing mesh_import_mode=1\n");
                failed = 1;
            }
            char expected_path_snippet[256];
            snprintf(expected_path_snippet, sizeof(expected_path_snippet), "\"source_path\": \"%s\"", source_path);
            if (strstr(resaved, expected_path_snippet) == NULL) {
                fprintf(stderr, "  fail: re-saved scene missing source_path '%s'\n", source_path);
                failed = 1;
            }
            free(resaved);
        }
    }

    ecs_scene_shutdown(scene);
    ecs_world_shutdown(world);
    free(scene);
    free(world);
    return failed;
}

static int test_flat_observer_metadata_persisted_when_unlinked(void) {
    const char *fixture = "jsonl_flat_observer_metadata.jsonl";
    const char *lines[] = {
        "{\"Name\":\"Entry\",\"Elements\":[{\"Name\":\"P\",\"Description\":\"\",\"Colour\":\"White\",\"Element\":{\"$type\":\"Geo.Point3D\",\"X\":1,\"Y\":2,\"Z\":3}}]}"
    };
    if (!write_jsonl_fixture_lines(fixture, lines, 1)) return 1;
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    jsonl_import_job_t job = {0};
    int failed = 0;
    if (!run_flat_import_to_completion(&scene, fixture, false, 0.5f, false, true, 0.1f, 0.2f, 0.3f, 1, &job)) {
        failed = 1;
    }
    JsonlObserverComp *observer = NULL;
    if (!failed) {
        if (!ecs_is_alive(world.world, job.root_entity)) {
            fprintf(stderr, "  fail: root entity not alive\n");
            failed = 1;
        }
    }
    if (!failed) {
        observer = ecs_world_get_jsonl_observer(&world, job.root_entity);
        if (!observer) {
            fprintf(stderr, "  fail: missing observer on root\n");
            failed = 1;
        }
    }
    if (!failed && observer->linked) {
        fprintf(stderr, "  fail: linked should be false\n");
        failed = 1;
    }
    if (!failed && strcmp(observer->source_path, fixture) != 0) {
        fprintf(stderr, "  fail: source_path mismatch: '%s'\n", observer->source_path);
        failed = 1;
    }
    if (!failed && observer->scale != 0.5f) {
        fprintf(stderr, "  fail: scale mismatch %f\n", observer->scale);
        failed = 1;
    }
    if (!failed && observer->rotation_x != 0.1f) {
        fprintf(stderr, "  fail: rotation_x mismatch %f\n", observer->rotation_x);
        failed = 1;
    }
    if (!failed && observer->rotation_y != 0.2f) {
        fprintf(stderr, "  fail: rotation_y mismatch %f\n", observer->rotation_y);
        failed = 1;
    }
    if (!failed && observer->rotation_z != 0.3f) {
        fprintf(stderr, "  fail: rotation_z mismatch %f\n", observer->rotation_z);
        failed = 1;
    }
    if (!failed && !observer->shift_to_center) {
        fprintf(stderr, "  fail: shift_to_center expected true\n");
        failed = 1;
    }
    if (!failed && observer->use_jsonl_colours) {
        fprintf(stderr, "  fail: use_jsonl_colours expected false\n");
        failed = 1;
    }
    if (!failed && observer->mesh_import_mode != 1) {
        fprintf(stderr, "  fail: mesh_import_mode expected 1 got %d\n", observer->mesh_import_mode);
        failed = 1;
    }
    char *json = NULL;
    if (!failed) {
        json = scene_save_to_string(&scene);
        if (!json) {
            fprintf(stderr, "  fail: scene_save_to_string returned null\n");
            failed = 1;
        }
    }
    if (!failed && strstr(json, "\"jsonl_observer\"") == NULL) {
        fprintf(stderr, "  fail: missing jsonl_observer in save json\n");
        failed = 1;
    }
    if (!failed && strstr(json, "\"linked\": false") == NULL) {
        fprintf(stderr, "  fail: missing linked false in save json\n");
        failed = 1;
    }
    if (!failed && strstr(json, "\"use_jsonl_colours\": false") == NULL) {
        fprintf(stderr, "  fail: missing use_jsonl_colours false in save json\n");
        failed = 1;
    }
    if (!failed && strstr(json, "\"mesh_import_mode\": 1") == NULL) {
        fprintf(stderr, "  fail: missing mesh_import_mode in save json\n");
        failed = 1;
    }
    if (!failed && strstr(json, "\"source_path\": \"jsonl_flat_observer_metadata.jsonl\"") == NULL) {
        fprintf(stderr, "  fail: missing source_path in save json\n");
        failed = 1;
    }

    if (!failed && verify_loaded_observer_metadata(json, fixture) != 0) {
        fprintf(stderr, "  fail: roundtrip metadata mismatch\n");
        failed = 1;
    }

    free(json);
    ecs_scene_shutdown(&scene);
    ecs_world_shutdown(&world);
    remove(fixture);
    return failed;
}

static int test_flat_linked_import_baseline_failure_keeps_geometry_and_disables_observe(void) {
    const char *fixture = "jsonl_flat_linked_import_baseline_failure.jsonl";
    const char *lines[] = {
        "{\"Name\":\"Entry\",\"Elements\":[{\"Name\":\"P\",\"Description\":\"\",\"Colour\":\"White\",\"Element\":{\"$type\":\"Geo.Point3D\",\"X\":1,\"Y\":2,\"Z\":3}}]}"
    };
    if (!write_jsonl_fixture_lines(fixture, lines, 1)) return 1;

    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    jsonl_import_job_t job = {0};
    int failed = 0;
    if (!run_flat_import_until_parenting(&scene, fixture, true, 1.0f, true, false, 0.0f, 0.0f, 0.0f, 0, &job)) {
        fprintf(stderr, "  fail: could not drive linked import to parenting\n");
        failed = 1;
    }

    if (!failed && remove(fixture) != 0) {
        fprintf(stderr, "  fail: could not remove fixture before baseline arming\n");
        failed = 1;
    }

    if (!failed) {
        if (!jsonl_import_job_tick(&job, &scene)) {
            fprintf(stderr, "  fail: final parenting tick did not complete import\n");
            failed = 1;
        } else if (job.state != JSONL_JOB_COMPLETE || job.root_entity == 0) {
            fprintf(stderr, "  fail: linked import did not complete after baseline failure\n");
            failed = 1;
        }
    }

    JsonlObserverComp *observer = NULL;
    if (!failed) {
        if (!ecs_is_alive(world.world, job.root_entity)) {
            fprintf(stderr, "  fail: root entity not alive after baseline failure\n");
            failed = 1;
        }
    }
    if (!failed) {
        observer = ecs_world_get_jsonl_observer(&world, job.root_entity);
        if (!observer) {
            fprintf(stderr, "  fail: missing observer on root after baseline failure\n");
            failed = 1;
        }
    }
    if (!failed && count_geometry_under_root(&scene, job.root_entity) != 1) {
        fprintf(stderr, "  fail: geometry count changed after baseline failure fallback\n");
        failed = 1;
    }
    if (!failed && strcmp(observer->source_path, fixture) != 0) {
        fprintf(stderr, "  fail: source_path mismatch after baseline failure: '%s'\n", observer->source_path);
        failed = 1;
    }
    if (!failed && !observer->linked) {
        fprintf(stderr, "  fail: linked flag should stay true after baseline failure\n");
        failed = 1;
    }
    if (!failed && observer->observe_enabled) {
        fprintf(stderr, "  fail: observe should auto-disable after baseline failure\n");
        failed = 1;
    }
    if (!failed && observer->source_state_valid) {
        fprintf(stderr, "  fail: source_state_valid should stay false after failed baseline arm\n");
        failed = 1;
    }
    if (!failed && !observer_contains_message(observer, "observe disabled")) {
        fprintf(stderr, "  fail: missing observer warning about disabled observe\n");
        failed = 1;
    }
    if (!failed && strstr(job.status_message, "observe disabled") == NULL) {
        fprintf(stderr, "  fail: import status_message missing safe fallback text: '%s'\n", job.status_message);
        failed = 1;
    }

    ecs_scene_shutdown(&scene);
    ecs_world_shutdown(&world);
    remove(fixture);
    return failed;
}

static int test_flat_manual_refresh_success_replaces_subtree_and_fallbacks_selection(void) {
    const char *fixture = "jsonl_flat_observer_refresh_success.jsonl";
    const char *first_lines[] = {
        "{\"Name\":\"Entry\",\"Elements\":[{\"Name\":\"P1\",\"Description\":\"\",\"Colour\":\"White\",\"Element\":{\"$type\":\"Geo.Point3D\",\"X\":1,\"Y\":0,\"Z\":0}}]}"
    };
    const char *second_lines[] = {
        "{\"Name\":\"Entry\",\"Elements\":[{\"Name\":\"P1\",\"Description\":\"\",\"Colour\":\"White\",\"Element\":{\"$type\":\"Geo.Point3D\",\"X\":1,\"Y\":0,\"Z\":0}},{\"Name\":\"P2\",\"Description\":\"\",\"Colour\":\"White\",\"Element\":{\"$type\":\"Geo.Point3D\",\"X\":2,\"Y\":0,\"Z\":0}}]}",
        "{\"Name\":\"Entry2\",\"Elements\":[{\"Name\":\"L1\",\"Description\":\"\",\"Colour\":\"White\",\"Element\":{\"$type\":\"Geo.Line3D\",\"StartPoint\":{\"X\":0,\"Y\":0,\"Z\":0},\"EndPoint\":{\"X\":3,\"Y\":0,\"Z\":0}}}]}"
    };

    if (!write_jsonl_fixture_lines(fixture, first_lines, 1)) return 1;

    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    selection_buffer_t selection = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);
    selection_init(&selection, &world);

    jsonl_import_job_t job = {0};
    int failed = 0;
    if (!run_flat_import_to_completion(&scene, fixture, true, 1.0f, true, false, 0.0f, 0.0f, 0.0f, 0, &job)) {
        failed = 1;
    }

    ecs_entity_t root = job.root_entity;
    ecs_entity_t old_geometry = 0;
    if (!failed) {
        old_geometry = find_first_geometry_under_root(&scene, root);
        if (old_geometry == 0) failed = 1;
    }

    if (!failed) {
        selection_set_single(&selection, old_geometry);
        if (selection_count(&selection) != 1 || selection_get(&selection, 0) != old_geometry) failed = 1;
    }

    if (!failed && !write_jsonl_fixture_lines(fixture, second_lines, 2)) failed = 1;

    if (!failed && !jsonl_observer_request_flat_refresh(&scene, root, &selection)) failed = 1;
    if (!failed && jsonl_observer_request_flat_refresh(&scene, root, &selection)) failed = 1;
    if (!failed && !tick_refresh_until_idle(&scene, root, 10000)) failed = 1;

    if (!failed && !ecs_is_alive(world.world, root)) failed = 1;
    if (!failed && ecs_is_alive(world.world, old_geometry)) failed = 1;
    if (!failed && count_geometry_under_root(&scene, root) != 3) failed = 1;
    if (!failed && (selection_count(&selection) != 1 || selection_get(&selection, 0) != root)) failed = 1;

    selection_shutdown(&selection);
    ecs_scene_shutdown(&scene);
    ecs_world_shutdown(&world);
    remove(fixture);
    return failed;
}

static int test_flat_manual_refresh_failure_preserves_last_good_and_relinks_label(void) {
    const char *fixture = "jsonl_flat_observer_refresh_good.jsonl";
    const char *missing = "jsonl_flat_observer_missing.jsonl";
    const char *lines[] = {
        "{\"Name\":\"Entry\",\"Elements\":[{\"Name\":\"L\",\"Description\":\"\",\"Colour\":\"White\",\"Element\":{\"$type\":\"Geo.Line3D\",\"StartPoint\":{\"X\":0,\"Y\":0,\"Z\":0},\"EndPoint\":{\"X\":1,\"Y\":0,\"Z\":0}}}]}"
    };
    if (!write_jsonl_fixture_lines(fixture, lines, 1)) return 1;

    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    jsonl_import_job_t job = {0};
    int failed = 0;
    if (!run_flat_import_to_completion(&scene, fixture, true, 1.0f, true, false, 0.0f, 0.0f, 0.0f, 0, &job)) {
        failed = 1;
    }

    ecs_entity_t root = job.root_entity;
    ecs_entity_t old_geometry = 0;
    int old_geometry_count = 0;
    if (!failed) {
        old_geometry = find_first_geometry_under_root(&scene, root);
        old_geometry_count = count_geometry_under_root(&scene, root);
        if (old_geometry == 0 || old_geometry_count <= 0) failed = 1;
    }
    if (!failed && !jsonl_observer_relink(&scene, root, missing)) failed = 1;

    if (!failed) {
        LabelComp *label = ecs_world_get_label(&world, root);
        char expected_name[LABEL_NAME_MAX] = {0};
        jsonl_observer_label_from_path(missing, expected_name, sizeof(expected_name));
        if (!label) {
            fprintf(stderr, "  fail: missing label after relink\n");
            failed = 1;
        }
        if (!failed && strcmp(label->name, expected_name) != 0) {
            fprintf(stderr, "  fail: relink label name mismatch expected='%s' got='%s'\n", expected_name, label->name);
            failed = 1;
        }
        if (!failed && strcmp(label->description, missing) != 0) {
            fprintf(stderr, "  fail: relink label desc mismatch expected='%s' got='%s'\n", missing, label->description);
            failed = 1;
        }
    }

    if (!failed) {
        bool requested = jsonl_observer_request_flat_refresh(&scene, root, NULL);
        if (requested) {
            bool completed = tick_refresh_until_idle(&scene, root, 10000);
            if (!completed) failed = 1;
        }
    }

    if (!failed && !ecs_is_alive(world.world, old_geometry)) {
        fprintf(stderr, "  fail: old geometry should still be alive after failed refresh\n");
        failed = 1;
    }
    if (!failed && count_geometry_under_root(&scene, root) != old_geometry_count) {
        fprintf(stderr, "  fail: geometry count changed after failed refresh\n");
        failed = 1;
    }

    ecs_scene_shutdown(&scene);
    ecs_world_shutdown(&world);
    remove(fixture);
    return failed;
}

static int test_flat_manual_refresh_works_after_auto_disable(void) {
    const char *fixture = "jsonl_flat_observer_manual_after_disable.jsonl";
    const char *first_lines[] = {
        "{\"Name\":\"Entry\",\"Elements\":[{\"Name\":\"P1\",\"Description\":\"\",\"Colour\":\"White\",\"Element\":{\"$type\":\"Geo.Point3D\",\"X\":1,\"Y\":0,\"Z\":0}}]}"
    };
    const char *second_lines[] = {
        "{\"Name\":\"Entry\",\"Elements\":[{\"Name\":\"P1\",\"Description\":\"\",\"Colour\":\"White\",\"Element\":{\"$type\":\"Geo.Point3D\",\"X\":1,\"Y\":0,\"Z\":0}},{\"Name\":\"P2\",\"Description\":\"\",\"Colour\":\"White\",\"Element\":{\"$type\":\"Geo.Point3D\",\"X\":2,\"Y\":0,\"Z\":0}}]}"
    };
    if (!write_jsonl_fixture_lines(fixture, first_lines, 1)) return 1;

    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    int failed = 0;
    jsonl_import_job_t job = {0};
    if (!run_flat_import_to_completion(&scene, fixture, true, 1.0f, true, false, 0.0f, 0.0f, 0.0f, 0, &job)) {
        failed = 1;
    }

    ecs_entity_t root = job.root_entity;
    JsonlObserverComp *observer = NULL;
    if (!failed) {
        observer = ecs_world_get_jsonl_observer(&world, root);
        if (!observer) failed = 1;
    }
    if (!failed && count_geometry_under_root(&scene, root) != 1) failed = 1;

    if (!failed) {
        observer->observe_enabled = false;
        observer->max_retries = 2u;
        observer->retry_count = observer->max_retries;
    }

    if (!failed && !write_jsonl_fixture_lines(fixture, second_lines, 1)) failed = 1;
    if (!failed && !jsonl_observer_request_flat_refresh(&scene, root, NULL)) failed = 1;
    if (!failed && !tick_refresh_until_idle(&scene, root, 10000)) failed = 1;

    if (!failed && count_geometry_under_root(&scene, root) != 2) failed = 1;
    if (!failed && observer->observe_enabled) failed = 1;

    ecs_scene_shutdown(&scene);
    ecs_world_shutdown(&world);
    remove(fixture);
    return failed;
}

static int test_flat_manual_refresh_repeated_cycles_return_to_exact_live_footprint(void) {
    const char *fixture = "jsonl_flat_observer_manual_exact_footprint.jsonl";
    const int cycle_points[] = { 4, 4, 6 };
    const float cycle_offsets[] = { 0.0f, 3.0f, 6.0f };
    if (!write_jsonl_polyline_fixture(fixture, cycle_points[0], cycle_offsets[0])) return 1;

    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    int failed = 0;
    jsonl_import_job_t job = {0};
    if (!run_flat_import_to_completion(&scene, fixture, true, 1.0f, true, false, 0.0f, 0.0f, 0.0f, 0, &job)) {
        fprintf(stderr, "  fail: initial linked import failed\n");
        failed = 1;
    }

    ecs_entity_t root = job.root_entity;
    for (int cycle = 0; cycle < 3 && !failed; cycle++) {
        if (!write_jsonl_polyline_fixture(fixture, cycle_points[cycle], cycle_offsets[cycle])) {
            fprintf(stderr, "  fail: write fixture for cycle %d failed\n", cycle);
            failed = 1;
            break;
        }
        if (!jsonl_observer_request_flat_refresh(&scene, root, NULL)) {
            fprintf(stderr, "  fail: manual refresh request %d failed\n", cycle);
            failed = 1;
            break;
        }
        if (!tick_refresh_until_idle(&scene, root, 50000)) {
            fprintf(stderr, "  fail: manual refresh %d did not settle\n", cycle);
            failed = 1;
            break;
        }

        slot_footprint_t expected = count_slot_footprint_under_root(&scene, root);
        int live_line_slots = instance_buffer_live_count(&scene.batches.lines.instances);
        int live_point_slots = instance_buffer_live_count(&scene.batches.points.instances);
        if (live_line_slots != expected.line_slots) {
            fprintf(stderr, "  fail: cycle %d live line slots mismatch expected=%d got=%d\n",
                    cycle, expected.line_slots, live_line_slots);
            failed = 1;
            break;
        }
        if (live_point_slots != expected.point_slots) {
            fprintf(stderr, "  fail: cycle %d live point slots mismatch expected=%d got=%d\n",
                    cycle, expected.point_slots, live_point_slots);
            failed = 1;
            break;
        }
        if (scene.batches.lines.instances.count != expected.line_slots) {
            fprintf(stderr, "  fail: cycle %d line slot span mismatch expected=%d got=%d\n",
                    cycle, expected.line_slots, scene.batches.lines.instances.count);
            failed = 1;
            break;
        }
        if (scene.batches.points.instances.count != expected.point_slots) {
            fprintf(stderr, "  fail: cycle %d point slot span mismatch expected=%d got=%d\n",
                    cycle, expected.point_slots, scene.batches.points.instances.count);
            failed = 1;
            break;
        }
        if (scene.batches.lines.instances.free_count != 0) {
            fprintf(stderr, "  fail: cycle %d line buffer retained %d free slots\n",
                    cycle, scene.batches.lines.instances.free_count);
            failed = 1;
            break;
        }
        if (scene.batches.points.instances.free_count != 0) {
            fprintf(stderr, "  fail: cycle %d point buffer retained %d free slots\n",
                    cycle, scene.batches.points.instances.free_count);
            failed = 1;
            break;
        }
    }

    ecs_scene_shutdown(&scene);
    ecs_world_shutdown(&world);
    remove(fixture);
    return failed;
}

static int test_flat_refresh_cleanup_anomaly_keeps_last_good_and_disables_observe(void) {
    const char *fixture = "jsonl_flat_observer_cleanup_anomaly.jsonl";
    const char *first_lines[] = {
        "{\"Name\":\"Entry\",\"Elements\":[{\"Name\":\"P1\",\"Description\":\"\",\"Colour\":\"White\",\"Element\":{\"$type\":\"Geo.Point3D\",\"X\":1,\"Y\":0,\"Z\":0}}]}"
    };
    const char *second_lines[] = {
        "{\"Name\":\"Entry\",\"Elements\":[{\"Name\":\"P1\",\"Description\":\"\",\"Colour\":\"White\",\"Element\":{\"$type\":\"Geo.Point3D\",\"X\":1,\"Y\":0,\"Z\":0}},{\"Name\":\"P2\",\"Description\":\"\",\"Colour\":\"White\",\"Element\":{\"$type\":\"Geo.Point3D\",\"X\":2,\"Y\":0,\"Z\":0}}]}"
    };
    if (!write_jsonl_fixture_lines(fixture, first_lines, 1)) return 1;

    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    int failed = 0;
    jsonl_import_job_t job = {0};
    if (!run_flat_import_to_completion(&scene, fixture, true, 1.0f, true, false, 0.0f, 0.0f, 0.0f, 0, &job)) {
        fprintf(stderr, "  fail: initial linked import failed\n");
        failed = 1;
    }

    ecs_entity_t root = job.root_entity;
    JsonlObserverComp *observer = NULL;
    int old_geometry_count = 0;
    if (!failed) {
        observer = ecs_world_get_jsonl_observer(&world, root);
        if (!observer) {
            fprintf(stderr, "  fail: missing observer\n");
            failed = 1;
        }
    }
    if (!failed) {
        old_geometry_count = count_geometry_under_root(&scene, root);
        if (old_geometry_count != 1) {
            fprintf(stderr, "  fail: expected initial geometry count 1, got %d\n", old_geometry_count);
            failed = 1;
        }
    }
    if (!failed && !write_jsonl_fixture_lines(fixture, second_lines, 1)) {
        fprintf(stderr, "  fail: rewrite fixture failed\n");
        failed = 1;
    }

    if (!failed) {
        jsonl_observer_force_next_flat_cleanup_failure_for_tests();
        if (!jsonl_observer_request_flat_refresh(&scene, root, NULL)) {
            fprintf(stderr, "  fail: manual refresh request failed\n");
            failed = 1;
        }
    }
    if (!failed && !tick_refresh_until_idle(&scene, root, 10000)) {
        fprintf(stderr, "  fail: refresh did not settle after forced anomaly\n");
        failed = 1;
    }

    if (!failed && count_geometry_under_root(&scene, root) != old_geometry_count) {
        fprintf(stderr, "  fail: cleanup anomaly changed committed geometry count\n");
        failed = 1;
    }
    if (!failed && observer->observe_enabled) {
        fprintf(stderr, "  fail: cleanup anomaly should disable observe\n");
        failed = 1;
    }
    if (!failed && !observer_contains_message(observer, "observe disabled")) {
        fprintf(stderr, "  fail: missing safety-disable warning after cleanup anomaly\n");
        failed = 1;
    }
    if (!failed && strcmp(observer->source_path, fixture) != 0) {
        fprintf(stderr, "  fail: cleanup anomaly should keep source_path intact\n");
        failed = 1;
    }
    if (!failed && !observer->linked) {
        fprintf(stderr, "  fail: cleanup anomaly should keep linked=true\n");
        failed = 1;
    }
    if (!failed && !jsonl_observer_request_flat_refresh(&scene, root, NULL)) {
        fprintf(stderr, "  fail: manual refresh should remain available after cleanup anomaly\n");
        failed = 1;
    }

    ecs_scene_shutdown(&scene);
    ecs_world_shutdown(&world);
    remove(fixture);
    return failed;
}

static int test_flat_manual_refresh_repeated_cycles_preserve_anchor_coherence(void) {
    static const int tiers[] = { 24, 240, 1200 };
    int failed = 0;

    for (int t = 0; t < (int)(sizeof(tiers) / sizeof(tiers[0])) && !failed; t++) {
        char fixture[128];
        snprintf(fixture, sizeof(fixture), "jsonl_flat_observer_coherence_tier_%d.jsonl", tiers[t]);
        if (!write_jsonl_point_fixture(fixture, tiers[t])) return 1;

        ecs_world_state_t world = {0};
        ecs_scene_t scene = {0};
        selection_buffer_t selection = {0};
        ecs_world_init(&world);
        ecs_scene_init(&scene, &world);
        selection_init(&selection, &world);

        jsonl_import_job_t job = {0};
        ecs_entity_t root = 0;
        if (!run_flat_import_to_completion(&scene, fixture, true, 1.0f, true, false, 0.0f, 0.0f, 0.0f, 0, &job)) {
            fprintf(stderr, "  fail: initial flat import failed for tier=%d\n", tiers[t]);
            failed = 1;
        }
        if (!failed) {
            root = job.root_entity;
            if (root == 0 || !ecs_is_alive(world.world, root)) {
                fprintf(stderr, "  fail: root invalid after initial import for tier=%d root=%llu alive=%d\n",
                        tiers[t], (unsigned long long)root,
                        (int)(root != 0 && ecs_is_alive(world.world, root)));
                failed = 1;
            }
        }

        for (int cycle = 0; cycle < 3 && !failed; cycle++) {
            ecs_entity_t selected_child = find_first_geometry_under_root(&scene, root);
            if (selected_child == 0 || !ecs_is_alive(world.world, selected_child)) {
                fprintf(stderr, "  fail: selected child invalid before cycle=%d tier=%d child=%llu\n",
                        cycle, tiers[t], (unsigned long long)selected_child);
                failed = 1;
                break;
            }
            selection_set_single(&selection, selected_child);
            if (selection_count(&selection) != 1 || selection_get(&selection, 0) != selected_child) {
                fprintf(stderr, "  fail: selection did not hold child before cycle=%d tier=%d count=%d selected=%llu expected=%llu\n",
                        cycle, tiers[t], selection_count(&selection),
                        (unsigned long long)selection_get(&selection, 0),
                        (unsigned long long)selected_child);
                failed = 1;
                break;
            }

            if (!write_jsonl_point_fixture(fixture, tiers[t] + cycle + 1)) {
                fprintf(stderr, "  fail: fixture rewrite failed for cycle=%d tier=%d target_count=%d\n",
                        cycle, tiers[t], tiers[t] + cycle + 1);
                failed = 1;
                break;
            }
            if (!jsonl_observer_request_flat_refresh(&scene, root, &selection)) {
                fprintf(stderr, "  fail: refresh request rejected for cycle=%d tier=%d\n", cycle, tiers[t]);
                failed = 1;
                break;
            }
            if (!tick_refresh_until_idle(&scene, root, 50000)) {
                fprintf(stderr, "  fail: refresh did not settle for cycle=%d tier=%d\n", cycle, tiers[t]);
                failed = 1;
                break;
            }

            if (!ecs_is_alive(world.world, root)) {
                fprintf(stderr, "  fail: root died after cycle=%d tier=%d root=%llu\n",
                        cycle, tiers[t], (unsigned long long)root);
                failed = 1;
                break;
            }
            if (selection_count(&selection) != 1 || selection_get(&selection, 0) != root) {
                fprintf(stderr, "  fail: selection did not fall back to root after cycle=%d tier=%d count=%d selected=%llu expected=%llu\n",
                        cycle, tiers[t], selection_count(&selection),
                        (unsigned long long)selection_get(&selection, 0),
                        (unsigned long long)root);
                failed = 1;
                break;
            }
            if (count_geometry_under_root(&scene, root) != tiers[t] + cycle + 1) {
                fprintf(stderr, "  fail: geometry count mismatch after cycle=%d tier=%d expected=%d got=%d\n",
                        cycle, tiers[t], tiers[t] + cycle + 1,
                        count_geometry_under_root(&scene, root));
                failed = 1;
                break;
            }
        }

        selection_shutdown(&selection);
        ecs_scene_shutdown(&scene);
        ecs_world_shutdown(&world);
        remove(fixture);
    }

    return failed;
}

typedef int (*test_fn_t)(void);
typedef struct {
    const char *name;
    test_fn_t fn;
} test_case_t;

int main(void) {
    stm_setup();

    static const test_case_t tests[] = {
        { "test_flat_observer_metadata_persisted_when_unlinked", test_flat_observer_metadata_persisted_when_unlinked },
        { "test_flat_linked_import_baseline_failure_keeps_geometry_and_disables_observe",
          test_flat_linked_import_baseline_failure_keeps_geometry_and_disables_observe },
        { "test_flat_manual_refresh_success_replaces_subtree_and_fallbacks_selection",
          test_flat_manual_refresh_success_replaces_subtree_and_fallbacks_selection },
        { "test_flat_manual_refresh_failure_preserves_last_good_and_relinks_label",
          test_flat_manual_refresh_failure_preserves_last_good_and_relinks_label },
        { "test_flat_manual_refresh_works_after_auto_disable",
          test_flat_manual_refresh_works_after_auto_disable },
        { "test_flat_manual_refresh_repeated_cycles_return_to_exact_live_footprint",
          test_flat_manual_refresh_repeated_cycles_return_to_exact_live_footprint },
        { "test_flat_refresh_cleanup_anomaly_keeps_last_good_and_disables_observe",
          test_flat_refresh_cleanup_anomaly_keeps_last_good_and_disables_observe },
        { "test_flat_manual_refresh_repeated_cycles_preserve_anchor_coherence",
          test_flat_manual_refresh_repeated_cycles_preserve_anchor_coherence },
    };

    for (size_t i = 0; i < (sizeof(tests) / sizeof(tests[0])); ++i) {
        if (tests[i].fn() != 0) {
            fprintf(stderr, "FAILED: %s\n", tests[i].name);
            return 1;
        }
    }
    return 0;
}
