#include "../ecs/ecs_world.h"
#include "../ecs/ecs_scene.h"
#include "../jsonl_import_job.h"
#include "../jsonl_observer_system.h"
#include <stdbool.h>
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

static bool run_flat_import_to_completion(ecs_scene_t *scene,
                                          const char *path,
                                          bool link_enabled,
                                          jsonl_import_job_t *out_job) {
    if (!scene || !path) return false;

    jsonl_import_job_t job = {0};
    jsonl_import_job_init(&job);

    bool started = jsonl_import_job_start(&job,
                                          path,
                                          1.0f,
                                          true,
                                          vec4_make(1, 1, 1, 1),
                                          false,
                                          0.0f,
                                          0.0f,
                                          0.0f);
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

static int test_flat_observe_missing_path_warns_without_retry_budget(void) {
    const char *fixture = "jsonl_flat_observer_auto_missing_path.jsonl";
    const char *lines[] = {
        "{\"Name\":\"Entry\",\"Elements\":[{\"Name\":\"P\",\"Description\":\"\",\"Colour\":\"White\",\"Element\":{\"$type\":\"Geo.Point3D\",\"X\":1,\"Y\":2,\"Z\":3}}]}"
    };
    if (!write_jsonl_fixture_lines(fixture, lines, 1)) return 1;

    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    int failed = 0;
    jsonl_import_job_t job = {0};
    if (!run_flat_import_to_completion(&scene, fixture, true, &job)) {
        failed = 1;
    }

    JsonlObserverComp *observer = NULL;
    if (!failed) {
        observer = ecs_world_get_jsonl_observer(&world, job.root_entity);
        if (!observer) failed = 1;
    }

    if (!failed) {
        observer->linked = true;
        observer->observe_enabled = true;
        observer->source_path[0] = '\0';
        observer->retry_count = 0u;
        observer->max_retries = 3u;
        observer->interval_ms = 1u;
        observer->next_retry_at_ms = 0u;

        uint64_t now_ms = scene_solver_now_ms();
        jsonl_observer_system_tick(&scene, now_ms);
        jsonl_observer_system_tick(&scene, now_ms + 2u);

        if (observer->retry_count != 0u) failed = 1;
        if (!observer->observe_enabled) failed = 1;
        if (!observer_contains_message(observer, "Observe is ON but source path is missing")) failed = 1;
    }

    ecs_scene_shutdown(&scene);
    ecs_world_shutdown(&world);
    remove(fixture);
    return failed;
}

static int test_flat_observe_auto_disables_after_retries_exhausted(void) {
    const char *fixture = "jsonl_flat_observer_auto_disable.jsonl";
    const char *missing = "jsonl_flat_observer_auto_disable_missing.jsonl";
    const char *lines[] = {
        "{\"Name\":\"Entry\",\"Elements\":[{\"Name\":\"L\",\"Description\":\"\",\"Colour\":\"White\",\"Element\":{\"$type\":\"Geo.Line3D\",\"StartPoint\":{\"X\":0,\"Y\":0,\"Z\":0},\"EndPoint\":{\"X\":1,\"Y\":0,\"Z\":0}}}]}"
    };
    if (!write_jsonl_fixture_lines(fixture, lines, 1)) return 1;
    remove(missing);

    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    int failed = 0;
    jsonl_import_job_t job = {0};
    if (!run_flat_import_to_completion(&scene, fixture, true, &job)) {
        failed = 1;
    }

    JsonlObserverComp *observer = NULL;
    if (!failed) {
        observer = ecs_world_get_jsonl_observer(&world, job.root_entity);
        if (!observer) failed = 1;
    }

    if (!failed) {
        jsonl_observer_comp_set_path(observer, missing);
        observer->linked = true;
        observer->observe_enabled = true;
        observer->retry_count = 0u;
        observer->max_retries = 2u;
        observer->interval_ms = 1u;
        observer->next_retry_at_ms = 0u;

        uint64_t now_ms = scene_solver_now_ms();
        jsonl_observer_system_tick(&scene, now_ms);
        jsonl_observer_system_tick(&scene, now_ms + 2u);

        if (observer->retry_count != 2u) failed = 1;
        if (observer->observe_enabled) failed = 1;
        if (!observer_contains_message(observer, "Observe auto-disabled after max retries.")) failed = 1;
    }

    ecs_scene_shutdown(&scene);
    ecs_world_shutdown(&world);
    remove(fixture);
    return failed;
}

static int test_flat_observe_changed_source_runs_one_refresh_and_stamps_state(void) {
    const char *fixture = "jsonl_flat_observer_auto_changed.jsonl";
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
    if (!run_flat_import_to_completion(&scene, fixture, true, &job)) {
        failed = 1;
    }

    ecs_entity_t root = job.root_entity;
    JsonlObserverComp *observer = NULL;
    int initial_geometry_count = 0;
    if (!failed) {
        observer = ecs_world_get_jsonl_observer(&world, root);
        if (!observer) failed = 1;
    }
    if (!failed) {
        initial_geometry_count = count_geometry_under_root(&scene, root);
        if (initial_geometry_count <= 0) failed = 1;
    }
    if (!failed) {
        observer->linked = true;
        observer->observe_enabled = true;
        observer->interval_ms = 1u;
        observer->max_retries = 5u;
        observer->retry_count = 0u;
        observer->next_retry_at_ms = 0u;
        if (!jsonl_observer_stamp_source_state(observer, fixture)) failed = 1;
    }

    if (!failed && !write_jsonl_fixture_lines(fixture, second_lines, 1)) failed = 1;

    if (!failed) {
        uint64_t now_ms = scene_solver_now_ms();
        jsonl_observer_system_tick(&scene, now_ms);
        if (!jsonl_observer_is_flat_refresh_running(&scene, root)) failed = 1;

        jsonl_observer_system_tick(&scene, now_ms + 1u);
        if (!tick_refresh_until_idle(&scene, root, 20000)) failed = 1;
        if (count_geometry_under_root(&scene, root) <= initial_geometry_count) failed = 1;
        if (observer->retry_count != 0u) failed = 1;
        if (!observer->observe_enabled) failed = 1;

        jsonl_observer_system_tick(&scene, now_ms + 5u);
        if (jsonl_observer_is_flat_refresh_running(&scene, root)) failed = 1;
    }

    ecs_scene_shutdown(&scene);
    ecs_world_shutdown(&world);
    remove(fixture);
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
        { "test_flat_observe_missing_path_warns_without_retry_budget",
          test_flat_observe_missing_path_warns_without_retry_budget },
        { "test_flat_observe_auto_disables_after_retries_exhausted",
          test_flat_observe_auto_disables_after_retries_exhausted },
        { "test_flat_observe_changed_source_runs_one_refresh_and_stamps_state",
          test_flat_observe_changed_source_runs_one_refresh_and_stamps_state },
    };

    for (size_t i = 0; i < (sizeof(tests) / sizeof(tests[0])); ++i) {
        if (tests[i].fn() != 0) {
            fprintf(stderr, "FAILED: %s\n", tests[i].name);
            return 1;
        }
    }
    return 0;
}
