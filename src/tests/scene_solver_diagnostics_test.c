#include "../ecs/ecs_world.h"
#include "../ecs/ecs_scene.h"
#include "../scene_serializer.h"
#include "../scripting/sketch_script_runtime.h"
#include "../scripting/sketch_script_contract.h"
#include "../scripting/sketch_script_parse.h"
#include "../scripting/sketch_script_apply.h"
#include "../scripting/sketch_script_emit.h"
#include "../undo_redo_exec.h"
#include "../ui/ui_entity_inspector.h"
#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif
void sokol_main(void) {}
#ifdef __cplusplus
}
#endif

static int test_diagnostics_dedupe_baseline_allows_non_identical_messages(void) {
    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    ecs_entity_t sketch = scene_add_sketch(&scene, "Sketch", "", vec4_make(1, 1, 1, 1));
    if (!sketch) return 1;

    if (!scene_solver_add_diagnostic(&scene, sketch, SKETCH_SOLVER_DIAG_ERROR, "1", "mismatch-1", 0)) return 1;
    if (!scene_solver_add_diagnostic(&scene, sketch, SKETCH_SOLVER_DIAG_ERROR, "2", "mismatch-2", 0)) return 1;

    int count = scene_solver_diagnostic_count(&scene, sketch);
    const sketch_solver_diagnostic_t *d0 = scene_solver_diagnostic_at(&scene, sketch, 0);
    const sketch_solver_diagnostic_t *d1 = scene_solver_diagnostic_at(&scene, sketch, 1);

    int ok = (count == 2 && d0 && d1 && strcmp(d0->message, d1->message) != 0);
    ecs_world_shutdown(&world);
    return ok ? 0 : 1;
}

typedef int (*test_fn_t)(void);
typedef struct { const char *name; test_fn_t fn; } test_case_t;

int main(void) {
    static const test_case_t tests[] = {
        { "test_diagnostics_dedupe_baseline_allows_non_identical_messages", test_diagnostics_dedupe_baseline_allows_non_identical_messages },
    };

    for (size_t i = 0; i < (sizeof(tests) / sizeof(tests[0])); ++i) {
        if (tests[i].fn() != 0) {
            fprintf(stderr, "FAILED: %s\n", tests[i].name);
            return 1;
        }
    }
    return 0;
}
