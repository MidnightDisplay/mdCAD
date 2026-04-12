#include "../ecs/ecs_world.h"
#include "../ecs/ecs_scene.h"
#include "../jsonl_sketch_import_job.h"
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif
void sokol_main(void) {}
#ifdef __cplusplus
}
#endif

static int test_jsonl_sketch_mapping_converts_supported_shapes(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "MapSketch", "mapping", vec4_make(1, 1, 1, 1));
    if (sketch == 0) return 1;

    JsonlObserverComp settings = jsonl_observer_comp_default();
    settings.scale = 1.0f;

    jsonl_data_t data = {0};
    data.entry_capacity = 1;
    data.entry_count = 1;
    data.entries = (jsonl_log_entry_t*)calloc(1, sizeof(jsonl_log_entry_t));
    if (!data.entries) return 1;
    jsonl_log_entry_t *entry = &data.entries[0];
    entry->element_capacity = 5;
    entry->element_count = 5;
    entry->elements = (jsonl_element_t*)calloc(5, sizeof(jsonl_element_t));
    if (!entry->elements) return 1;

    entry->elements[0].type = JSONL_GEOM_POINT;
    entry->elements[0].colour = vec4_make(1, 0, 0, 1);
    entry->elements[0].data.point.point = vec3_make(0, 0, 0);

    entry->elements[1].type = JSONL_GEOM_LINE;
    entry->elements[1].colour = vec4_make(0, 1, 0, 1);
    entry->elements[1].data.line.start = vec3_make(0, 0, 0);
    entry->elements[1].data.line.end = vec3_make(1, 0, 0);

    entry->elements[2].type = JSONL_GEOM_ARC;
    entry->elements[2].colour = vec4_make(0, 0, 1, 1);
    entry->elements[2].data.arc.center = vec3_make(0, 0, 0);
    entry->elements[2].data.arc.radius = 1.0f;
    entry->elements[2].data.arc.start_angle = 0.0f;
    entry->elements[2].data.arc.end_angle = 3.14159265359f;
    entry->elements[2].data.arc.normal = vec3_make(0, 0, 1);

    entry->elements[3].type = JSONL_GEOM_POLYLINE;
    entry->elements[3].colour = vec4_make(1, 1, 0, 1);
    entry->elements[3].data.polyline.count = 3;
    entry->elements[3].data.polyline.points = (vec3_t*)calloc(3, sizeof(vec3_t));
    entry->elements[3].data.polyline.points[0] = vec3_make(0, 0, 0);
    entry->elements[3].data.polyline.points[1] = vec3_make(0, 1, 0);
    entry->elements[3].data.polyline.points[2] = vec3_make(1, 1, 0);

    entry->elements[4].type = JSONL_GEOM_POLYGON;
    entry->elements[4].colour = vec4_make(1, 0, 1, 1);
    entry->elements[4].data.polyline.count = 3;
    entry->elements[4].data.polyline.points = (vec3_t*)calloc(3, sizeof(vec3_t));
    entry->elements[4].data.polyline.points[0] = vec3_make(0, 0, 0);
    entry->elements[4].data.polyline.points[1] = vec3_make(1, 0, 0);
    entry->elements[4].data.polyline.points[2] = vec3_make(0, 1, 0);

    jsonl_sketch_counts_t counts = {0};
    bool ok = jsonl_sketch_import_apply_data_to_sketch(&scene, sketch, &data, &settings, true,
                                                        vec4_make(0.5f, 0.5f, 0.5f, 1.0f), &counts);
    if (!ok) return 1;
    if (scene_count_sketch_geometry(&scene, sketch) != 8) return 1; // point + line + arc + 2 + 3
    if (counts.geometry_count != 8u) return 1;
    if (scene_count_sketch_constraints(&scene, sketch) != 0) return 1;

    jsonl_parse_state_free(&(jsonl_parse_state_t){ .data = data });
    ecs_scene_shutdown(&scene);
    ecs_world_shutdown(&world);
    return 0;
}

static int test_jsonl_sketch_mapping_ignores_mesh_and_leaves_unconstrained(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "MeshIgnore", "mapping", vec4_make(1, 1, 1, 1));
    if (sketch == 0) return 1;

    JsonlObserverComp settings = jsonl_observer_comp_default();
    jsonl_data_t data = {0};
    data.entry_capacity = 1;
    data.entry_count = 1;
    data.entries = (jsonl_log_entry_t*)calloc(1, sizeof(jsonl_log_entry_t));
    if (!data.entries) return 1;
    jsonl_log_entry_t *entry = &data.entries[0];
    entry->element_capacity = 2;
    entry->element_count = 2;
    entry->elements = (jsonl_element_t*)calloc(2, sizeof(jsonl_element_t));
    if (!entry->elements) return 1;

    entry->elements[0].type = JSONL_GEOM_MESH;
    entry->elements[1].type = JSONL_GEOM_LINE;
    entry->elements[1].data.line.start = vec3_make(0, 0, 0);
    entry->elements[1].data.line.end = vec3_make(1, 0, 0);
    entry->elements[1].colour = vec4_make(1, 1, 1, 1);

    jsonl_sketch_counts_t counts = {0};
    bool ok = jsonl_sketch_import_apply_data_to_sketch(&scene, sketch, &data, &settings, true,
                                                        vec4_make(1, 1, 1, 1), &counts);
    if (!ok) return 1;
    if (scene_count_sketch_geometry(&scene, sketch) != 1) return 1; // mesh ignored
    if (counts.geometry_count != 1u) return 1;
    if (scene_count_sketch_constraints(&scene, sketch) != 0) return 1;

    jsonl_parse_state_free(&(jsonl_parse_state_t){ .data = data });
    ecs_scene_shutdown(&scene);
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
        { "test_jsonl_sketch_mapping_converts_supported_shapes", test_jsonl_sketch_mapping_converts_supported_shapes },
        { "test_jsonl_sketch_mapping_ignores_mesh_and_leaves_unconstrained",
          test_jsonl_sketch_mapping_ignores_mesh_and_leaves_unconstrained },
    };

    for (size_t i = 0; i < (sizeof(tests) / sizeof(tests[0])); ++i) {
        if (tests[i].fn() != 0) {
            fprintf(stderr, "FAILED: %s\n", tests[i].name);
            return 1;
        }
    }

    return 0;
}
