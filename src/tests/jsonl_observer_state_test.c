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

static int test_jsonl_observer_defaults_and_retry_policy(void) {
    fprintf(stderr,
            "RED: D-01/D-03/D-10/D-11/D-12/D-13 observer defaults/persistence/retry contract pending.\n");
    return 1;
}

typedef int (*jsonl_test_fn_t)(void);
typedef struct {
    const char *name;
    jsonl_test_fn_t fn;
} jsonl_test_case_t;

int main(void) {
    static const jsonl_test_case_t tests[] = {
        { "test_jsonl_observer_defaults_and_retry_policy", test_jsonl_observer_defaults_and_retry_policy },
    };

    for (size_t i = 0; i < (sizeof(tests) / sizeof(tests[0])); ++i) {
        if (tests[i].fn() != 0) {
            fprintf(stderr, "FAILED: %s\n", tests[i].name);
            return 1;
        }
    }

    return 0;
}
