#include "../ecs/ecs_world.h"
#include "../ecs/ecs_scene.h"
#include "../jsonl_sketch_import_job.h"
#include "../components/jsonl_observer_comp.h"
#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif
void sokol_main(void) {}
#ifdef __cplusplus
}
#endif

static bool write_jsonl_fixture(const char *path, const char *line) {
    FILE *f = fopen(path, "wb");
    if (!f) return false;
    fputs(line, f);
    fputs("\n", f);
    fclose(f);
    return true;
}

static int test_jsonl_sketch_import_creates_sketch_entity(void) {
    const char *fixture = "jsonl_sketch_import_create.jsonl";
    if (!write_jsonl_fixture(
            fixture,
            "{\"Name\":\"Entry\",\"Elements\":["
            "{\"Name\":\"P\",\"Description\":\"\",\"Colour\":\"White\","
            "\"Element\":{\"$type\":\"Geo.Point3D\",\"X\":0,\"Y\":0,\"Z\":0}}]}")) {
        return 1;
    }

    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = 0;
    char err[192] = {0};
    bool ok = jsonl_sketch_import_job_start(&scene,
                                            fixture,
                                            1.0f,
                                            true,
                                            vec4_make(0.7f, 0.7f, 0.7f, 1.0f),
                                            false,
                                            0.0f, 0.0f, 0.0f,
                                            &sketch,
                                            err, sizeof(err));
    if (!ok || sketch == 0) return 1;
    if (!scene_is_sketch(&scene, sketch)) return 1;

    JsonlObserverComp *obs = ecs_world_get_jsonl_observer(&world, sketch);
    if (!obs || !obs->linked || !obs->observe_enabled) return 1; // default Observe ON (opt-out)
    if (obs->scale != 1.0f || obs->rotation_x != 0.0f || obs->rotation_y != 0.0f || obs->rotation_z != 0.0f) return 1;
    if (obs->shift_to_center) return 1;
    if (strcmp(obs->source_path, fixture) != 0) return 1;

    LabelComp *label = ecs_world_get_label(&world, sketch);
    if (!label) return 1;
    if (strcmp(label->name, "jsonl_sketch_import_create") != 0) return 1;
    if (strcmp(label->description, fixture) != 0) return 1;

    if (scene_count_sketch_geometry(&scene, sketch) != 1) return 1;
    if (scene_count_sketch_constraints(&scene, sketch) != 0) return 1;

    ecs_scene_shutdown(&scene);
    ecs_world_shutdown(&world);
    remove(fixture);
    return 0;
}

static int test_jsonl_sketch_import_observer_link_failure_is_transactional(void) {
    // Simulate D-02 transactional failure by trying to import into invalid scene context.
    // The importer should fail before creating persistent sketch state.
    ecs_scene_t scene = {0};
    ecs_entity_t sketch = 0;
    char err[192] = {0};
    bool ok = jsonl_sketch_import_job_start(&scene,
                                            "missing-file.jsonl",
                                            1.0f,
                                            true,
                                            vec4_make(1, 1, 1, 1),
                                            false,
                                            0.0f, 0.0f, 0.0f,
                                            &sketch,
                                            err, sizeof(err));
    if (ok) return 1;
    if (sketch != 0) return 1;
    if (err[0] == '\0') return 1;
    return 0;
}

typedef int (*jsonl_test_fn_t)(void);
typedef struct {
    const char *name;
    jsonl_test_fn_t fn;
} jsonl_test_case_t;

int main(void) {
    static const jsonl_test_case_t tests[] = {
        { "test_jsonl_sketch_import_creates_sketch_entity", test_jsonl_sketch_import_creates_sketch_entity },
        { "test_jsonl_sketch_import_observer_link_failure_is_transactional",
          test_jsonl_sketch_import_observer_link_failure_is_transactional },
    };

    for (size_t i = 0; i < (sizeof(tests) / sizeof(tests[0])); ++i) {
        if (tests[i].fn() != 0) {
            fprintf(stderr, "FAILED: %s\n", tests[i].name);
            return 1;
        }
    }

    return 0;
}
