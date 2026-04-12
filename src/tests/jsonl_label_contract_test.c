#include "../ecs/ecs_world.h"
#include "../ecs/ecs_scene.h"
#include "../jsonl_observer_system.h"
#include "../components/jsonl_observer_comp.h"
#include "../components/label_comp.h"
#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif
void sokol_main(void) {}
#ifdef __cplusplus
}
#endif

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

static int test_jsonl_label_contract_tracks_filename_and_path_on_relink(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    scene.world = &world;

    ecs_entity_t sketch = create_test_sketch(&world, "Sketch");
    if (sketch == 0) return 1;

    const char *path_a = "C:\\very\\long\\folder\\alpha_part.jsonl";
    const char *path_b = "D:\\data\\beta_part.jsonl";

    if (!jsonl_observer_link_sketch(&scene, sketch, path_a)) return 1;
    LabelComp *label = ecs_world_get_label(&world, sketch);
    JsonlObserverComp *obs = ecs_world_get_jsonl_observer(&world, sketch);
    if (!label || !obs) return 1;

    // Label.name contract: filename stem.
    if (strcmp(label->name, "alpha_part") != 0) return 1;
    // Label.description contract: full path.
    if (strcmp(label->description, path_a) != 0) return 1;
    if (strcmp(obs->source_path, path_a) != 0) return 1; // full path kept in observer metadata

    if (!jsonl_observer_relink(&scene, sketch, path_b)) return 1;
    if (strcmp(label->name, "beta_part") != 0) return 1;
    if (strcmp(label->description, path_b) != 0) return 1;
    if (strcmp(obs->source_path, path_b) != 0) return 1;

    ecs_world_shutdown(&world);
    return 0;
}

typedef int (*jsonl_test_fn_t)(void);
typedef struct {
    const char *name;
    jsonl_test_fn_t fn;
} jsonl_test_case_t;

int main(void) {
    static const jsonl_test_case_t tests[] = {
        { "test_jsonl_label_contract_tracks_filename_and_path_on_relink",
          test_jsonl_label_contract_tracks_filename_and_path_on_relink },
    };

    for (size_t i = 0; i < (sizeof(tests) / sizeof(tests[0])); ++i) {
        if (tests[i].fn() != 0) {
            fprintf(stderr, "FAILED: %s\n", tests[i].name);
            return 1;
        }
    }
    return 0;
}

