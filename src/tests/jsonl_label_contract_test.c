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

static int test_jsonl_label_contract_tracks_filename_and_path_on_relink(void) {
    fprintf(stderr,
            "RED: D-16 label contract pending - label name/description filename/fullpath relink behavior not implemented.\n");
    return 1;
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
