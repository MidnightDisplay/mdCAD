#include "../embed_layout_state.h"

#include <stdio.h>
#include <string.h>

static int test_embed_layout_standalone_uses_shared_ini(void) {
    embed_layout_state_t state = embed_layout_state_make(false, false, false);
    embed_layout_policy_t policy = embed_layout_state_resolve(&state);

    if (policy.embedded_mode) return 1;
    if (policy.use_manual_persistence) return 1;
    if (policy.should_seed_default_layout) return 1;
    if (!policy.automatic_ini_filename) return 1;
    if (strcmp(policy.automatic_ini_filename, "imgui.ini") != 0) return 1;
    if (policy.manual_store_filename != NULL) return 1;
    return 0;
}

static int test_embed_layout_embedded_first_run_uses_separate_store_and_seed(void) {
    embed_layout_state_t state = embed_layout_state_make(true, false, false);
    embed_layout_policy_t policy = embed_layout_state_resolve(&state);

    if (!policy.embedded_mode) return 1;
    if (!policy.use_manual_persistence) return 1;
    if (!policy.should_seed_default_layout) return 1;
    if (policy.automatic_ini_filename != NULL) return 1;
    if (!policy.manual_store_filename) return 1;
    if (strcmp(policy.manual_store_filename, "imgui.embedded.ini") != 0) return 1;
    return 0;
}

static int test_embed_layout_embedded_existing_store_skips_seed(void) {
    embed_layout_state_t state = embed_layout_state_make(true, false, true);
    embed_layout_policy_t policy = embed_layout_state_resolve(&state);

    if (!policy.use_manual_persistence) return 1;
    if (policy.should_seed_default_layout) return 1;
    if (!policy.manual_store_filename) return 1;
    if (strcmp(policy.manual_store_filename, "imgui.embedded.ini") != 0) return 1;
    return 0;
}

static int test_embed_layout_viewport_only_first_run_uses_dedicated_store_and_seed(void) {
    embed_layout_state_t state = embed_layout_state_make(true, true, false);
    embed_layout_policy_t policy = embed_layout_state_resolve(&state);

    if (!policy.embedded_mode) return 1;
    if (!policy.use_manual_persistence) return 1;
    if (!policy.should_seed_default_layout) return 1;
    if (policy.automatic_ini_filename != NULL) return 1;
    if (!policy.manual_store_filename) return 1;
    if (strcmp(policy.manual_store_filename, embed_layout_state_viewport_only_store_filename()) != 0) return 1;
    return 0;
}

static int test_embed_layout_viewport_only_existing_store_skips_seed(void) {
    embed_layout_state_t state = embed_layout_state_make(true, true, true);
    embed_layout_policy_t policy = embed_layout_state_resolve(&state);

    if (!policy.use_manual_persistence) return 1;
    if (policy.should_seed_default_layout) return 1;
    if (!policy.manual_store_filename) return 1;
    if (strcmp(policy.manual_store_filename, embed_layout_state_viewport_only_store_filename()) != 0) return 1;
    return 0;
}

typedef int (*embed_layout_test_fn_t)(void);
typedef struct {
    const char* name;
    embed_layout_test_fn_t fn;
} embed_layout_test_case_t;

int main(void) {
    static const embed_layout_test_case_t tests[] = {
        { "test_embed_layout_standalone_uses_shared_ini", test_embed_layout_standalone_uses_shared_ini },
        { "test_embed_layout_embedded_first_run_uses_separate_store_and_seed", test_embed_layout_embedded_first_run_uses_separate_store_and_seed },
        { "test_embed_layout_embedded_existing_store_skips_seed", test_embed_layout_embedded_existing_store_skips_seed },
        { "test_embed_layout_viewport_only_first_run_uses_dedicated_store_and_seed", test_embed_layout_viewport_only_first_run_uses_dedicated_store_and_seed },
        { "test_embed_layout_viewport_only_existing_store_skips_seed", test_embed_layout_viewport_only_existing_store_skips_seed },
    };

    for (size_t i = 0; i < (sizeof(tests) / sizeof(tests[0])); ++i) {
        if (tests[i].fn() != 0) {
            fprintf(stderr, "FAILED: %s\n", tests[i].name);
            return 1;
        }
    }
    return 0;
}
