#include "../ecs/ecs_world.h"
#include "../ecs/ecs_scene.h"
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif
void sokol_main(void) {}
#ifdef __cplusplus
}
#endif

static int test_jsonl_sketch_mapping_converts_supported_shapes(void) {
    fprintf(stderr,
            "RED: D-17/D-18 mapping contract pending - Point/Line/Arc/Circle/PolyLine/Polygon conversion not implemented.\n");
    return 1;
}

static int test_jsonl_sketch_mapping_ignores_mesh_and_leaves_unconstrained(void) {
    fprintf(stderr,
            "RED: D-19/D-20 mapping contract pending - mesh ignore and unconstrained output not implemented.\n");
    return 1;
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
