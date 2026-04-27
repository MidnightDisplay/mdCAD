#include "../ecs/ecs_world.h"
#include "../ecs/ecs_scene.h"
#include "../jsonl_import_job.h"
#include "../jsonl_observer_system.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

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

static int test_flat_auto_scaffold_initializes_and_ticks(void) {
    const char *fixture = "jsonl_flat_observer_auto_scaffold.jsonl";
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
    if (!run_flat_import_to_completion(&scene, fixture, true, &job)) {
        failed = 1;
    }

    JsonlObserverComp *obs = NULL;
    if (!failed) {
        obs = ecs_world_get_jsonl_observer(&world, job.root_entity);
        if (!obs) failed = 1;
    }
    if (!failed && !obs->linked) failed = 1;
    if (!failed && obs->source_path[0] == '\0') failed = 1;

    if (!failed) {
        uint64_t now_ms = scene_solver_now_ms();
        for (int i = 0; i < 4; i++) {
            jsonl_observer_system_tick(&scene, now_ms + (uint64_t)i * 16u);
            jsonl_observer_tick_flat_refreshes(&scene);
        }
    }

    if (!failed && jsonl_observer_is_flat_refresh_running(&scene, job.root_entity)) {
        failed = 1;
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
        { "test_flat_auto_scaffold_initializes_and_ticks", test_flat_auto_scaffold_initializes_and_ticks },
    };

    for (size_t i = 0; i < (sizeof(tests) / sizeof(tests[0])); ++i) {
        if (tests[i].fn() != 0) {
            fprintf(stderr, "FAILED: %s\n", tests[i].name);
            return 1;
        }
    }
    return 0;
}
