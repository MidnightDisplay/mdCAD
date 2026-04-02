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

#ifdef __cplusplus
extern "C" {
#endif
void sokol_main(void) {}
#ifdef __cplusplus
}
#endif

static int test_endpoint_priority_contract_baseline(void) {
    // Wave-0 baseline for D-10/D-12: endpoint control points must remain first-class pick targets.
    // Full pick-layering behavior is validated in later implementation plans.
    return 0;
}

typedef int (*test_fn_t)(void);
typedef struct { const char *name; test_fn_t fn; } test_case_t;

int main(void) {
    static const test_case_t tests[] = {
        { "test_endpoint_priority_contract_baseline", test_endpoint_priority_contract_baseline },
    };

    for (size_t i = 0; i < (sizeof(tests) / sizeof(tests[0])); ++i) {
        if (tests[i].fn() != 0) {
            fprintf(stderr, "FAILED: %s\n", tests[i].name);
            return 1;
        }
    }
    return 0;
}
