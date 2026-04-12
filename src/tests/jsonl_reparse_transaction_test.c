#include "../ecs/ecs_world.h"
#include "../ecs/ecs_scene.h"
#include "../jsonl_observer_system.h"
#include "../jsonl_sketch_import_job.h"
#include <stdio.h>

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

static ecs_entity_t create_test_sketch(ecs_world_state_t *world, const char *name) {
    ecs_entity_t sketch = ecs_world_create_anchor_entity(world);
    if (sketch == 0) return 0;
    SketchComp sk = sketch_comp_default();
    ecs_world_set_sketch(world, sketch, &sk);
    ecs_world_set_label(world, sketch, &(LabelComp){0});
    LabelComp *label = ecs_world_get_label(world, sketch);
    if (label && name) {
        snprintf(label->name, sizeof(label->name), "%s", name);
    }
    return sketch;
}

static int test_jsonl_reparse_transaction_replaces_authoritatively_on_success(void) {
    // authoritative replace: successful reparse removes prior sketch content and recreates from source.
    const char *fixture = "jsonl_reparse_success.jsonl";
    if (!write_jsonl_fixture(
            fixture,
            "{\"Name\":\"Entry\",\"Elements\":["
            "{\"Name\":\"P1\",\"Description\":\"\",\"Colour\":\"White\","
            "\"Element\":{\"$type\":\"Geo.Point3D\",\"X\":0,\"Y\":0,\"Z\":0}},"
            "{\"Name\":\"P2\",\"Description\":\"\",\"Colour\":\"White\","
            "\"Element\":{\"$type\":\"Geo.Point3D\",\"X\":2,\"Y\":0,\"Z\":0}}]}")) {
        return 1;
    }

    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = create_test_sketch(&world, "Sketch");
    if (sketch == 0) return 1;

    // old sketch content that should be removed by authoritative replace
    if (!scene_add_line_to_sketch(&scene, sketch,
                                  vec3_make(0, 0, 0), vec3_make(10, 0, 0),
                                  vec4_make(1, 1, 1, 1), 1.0f)) {
        return 1;
    }
    int before_geom = scene_count_sketch_geometry(&scene, sketch);
    if (before_geom < 1) return 1;

    if (!jsonl_observer_link_sketch(&scene, sketch, fixture)) return 1;
    JsonlObserverComp *obs = ecs_world_get_jsonl_observer(&world, sketch);
    if (!obs) return 1;

    jsonl_reparse_result_t result = jsonl_sketch_reparse_transactional(&scene, sketch, obs);
    if (!result.success) return 1;

    int after_geom = scene_count_sketch_geometry(&scene, sketch);
    int after_constraints = scene_count_sketch_constraints(&scene, sketch);
    if (after_geom != 2) return 1;
    if (after_constraints != 0) return 1; // D-05 old constraints removed
    if (result.replaced_geometry_count != 2u) return 1;

    // D-14 script follows recreated state (re-emitted non-empty revision)
    if (scene_script_emit_revision(&scene) == 0u) return 1;

    ecs_scene_shutdown(&scene);
    ecs_world_shutdown(&world);
    remove(fixture);
    return 0;
}

static int test_jsonl_reparse_transaction_preserves_last_good_on_failure(void) {
    const char *good_fixture = "jsonl_reparse_good.jsonl";
    const char *missing_fixture = "jsonl_reparse_missing.jsonl";
    if (!write_jsonl_fixture(
            good_fixture,
            "{\"Name\":\"Entry\",\"Elements\":["
            "{\"Name\":\"L\",\"Description\":\"\",\"Colour\":\"White\","
            "\"Element\":{\"$type\":\"Geo.Line3D\",\"StartPoint\":{\"X\":0,\"Y\":0,\"Z\":0},"
            "\"EndPoint\":{\"X\":1,\"Y\":0,\"Z\":0}}}]}")) {
        return 1;
    }

    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = create_test_sketch(&world, "Sketch");
    if (sketch == 0) return 1;
    if (!jsonl_observer_link_sketch(&scene, sketch, good_fixture)) return 1;
    JsonlObserverComp *obs = ecs_world_get_jsonl_observer(&world, sketch);
    if (!obs) return 1;

    jsonl_reparse_result_t ok = jsonl_sketch_reparse_transactional(&scene, sketch, obs);
    if (!ok.success) return 1;
    int last_good_geom = scene_count_sketch_geometry(&scene, sketch);

    // Relink to missing path and force failure
    if (!jsonl_observer_relink(&scene, sketch, missing_fixture)) return 1;
    jsonl_reparse_result_t fail = jsonl_sketch_reparse_transactional(&scene, sketch, obs);
    if (fail.success) return 1;

    // rollback contract (D-06): keep last good state on parse/read failure.
    if (scene_count_sketch_geometry(&scene, sketch) != last_good_geom) return 1;
    if (scene_count_sketch_constraints(&scene, sketch) != 0) return 1;

    jsonl_observer_comp_push_message(obs, JSONL_OBSERVER_MSG_WARNING, fail.error_message);
    if (obs->message_count == 0) return 1;

    ecs_scene_shutdown(&scene);
    ecs_world_shutdown(&world);
    remove(good_fixture);
    return 0;
}

typedef int (*jsonl_test_fn_t)(void);
typedef struct {
    const char *name;
    jsonl_test_fn_t fn;
} jsonl_test_case_t;

int main(void) {
    static const jsonl_test_case_t tests[] = {
        { "test_jsonl_reparse_transaction_replaces_authoritatively_on_success",
          test_jsonl_reparse_transaction_replaces_authoritatively_on_success },
        { "test_jsonl_reparse_transaction_preserves_last_good_on_failure",
          test_jsonl_reparse_transaction_preserves_last_good_on_failure },
    };

    for (size_t i = 0; i < (sizeof(tests) / sizeof(tests[0])); ++i) {
        if (tests[i].fn() != 0) {
            fprintf(stderr, "FAILED: %s\n", tests[i].name);
            return 1;
        }
    }
    return 0;
}

