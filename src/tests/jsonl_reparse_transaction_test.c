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

static int test_jsonl_reparse_transaction_preserves_last_good_on_failure(void) {
    fprintf(stderr,
            "RED: D-04/D-05/D-06/D-14 reparse transaction contract pending - failure should keep last-good state.\n");
    return 1;
}

static int test_jsonl_reparse_transaction_replaces_authoritatively_on_success(void) {
    fprintf(stderr,
            "RED: D-04/D-05/D-14 reparse transaction contract pending - success should replace sketch authoritatively.\n");
    return 1;
}

typedef int (*jsonl_test_fn_t)(void);
typedef struct {
    const char *name;
    jsonl_test_fn_t fn;
} jsonl_test_case_t;

int main(void) {
    static const jsonl_test_case_t tests[] = {
        { "test_jsonl_reparse_transaction_preserves_last_good_on_failure",
          test_jsonl_reparse_transaction_preserves_last_good_on_failure },
        { "test_jsonl_reparse_transaction_replaces_authoritatively_on_success",
          test_jsonl_reparse_transaction_replaces_authoritatively_on_success },
    };

    for (size_t i = 0; i < (sizeof(tests) / sizeof(tests[0])); ++i) {
        if (tests[i].fn() != 0) {
            fprintf(stderr, "FAILED: %s\n", tests[i].name);
            return 1;
        }
    }

    return 0;
}
