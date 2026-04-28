#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif
void sokol_main(void) {}
#ifdef __cplusplus
}
#endif

static char *read_file_from_candidates(const char *candidates[], size_t count) {
    for (size_t i = 0; i < count; ++i) {
        FILE *f = fopen(candidates[i], "rb");
        if (!f) continue;

        if (fseek(f, 0, SEEK_END) != 0) {
            fclose(f);
            continue;
        }
        long sz = ftell(f);
        if (sz < 0 || fseek(f, 0, SEEK_SET) != 0) {
            fclose(f);
            continue;
        }

        char *buf = (char *)malloc((size_t)sz + 1u);
        if (!buf) {
            fclose(f);
            return NULL;
        }
        size_t read = fread(buf, 1, (size_t)sz, f);
        fclose(f);
        if (read != (size_t)sz) {
            free(buf);
            continue;
        }
        buf[sz] = '\0';
        return buf;
    }
    return NULL;
}

static int contains_required(const char *haystack, const char *needle) {
    if (!haystack || !needle) return 0;
    return strstr(haystack, needle) != NULL;
}

static int contains_forbidden(const char *haystack, const char *needle) {
    if (!haystack || !needle) return 0;
    return strstr(haystack, needle) != NULL;
}

static int test_flat_inspector_minimal_surface_contract(void) {
    static const char *paths[] = {
        "src\\ui\\ui_entity_inspector.h",
        "..\\src\\ui\\ui_entity_inspector.h",
        "..\\..\\src\\ui\\ui_entity_inspector.h",
        "..\\..\\..\\src\\ui\\ui_entity_inspector.h",
        "..\\..\\..\\..\\src\\ui\\ui_entity_inspector.h"
    };

    char *src = read_file_from_candidates(paths, sizeof(paths) / sizeof(paths[0]));
    if (!src) return 1;

    int failed = 0;
    if (!contains_required(src, "Link file for refresh##jsonl_flat_root_link")) failed = 1;
    if (!contains_required(src, "Source path: %s")) failed = 1;
    if (!contains_required(src, "Choose source...##jsonl_flat_root_choose_source")) failed = 1;
    if (!contains_required(src, "Observe automatically##jsonl_flat_root_observe_auto")) failed = 1;
    if (!contains_required(src, "Link is OFF; observing is paused.")) failed = 1;
    if (!contains_required(src, "Observe is ON but source path is missing; waiting for a valid path.")) failed = 1;
    if (!contains_required(src, "Observe auto-disabled after max retries.")) failed = 1;
    if (!contains_required(src, "Safety retries: %u / %u")) failed = 1;
    if (!contains_required(src, "Timing evidence is advisory only (not a hard failure gate).")) failed = 1;
    if (!contains_required(src, "Refresh in progress...")) failed = 1;
    if (!contains_required(src, "Re-import now##jsonl_flat_root_reimport_now")) failed = 1;
    if (!contains_required(src, "Last refresh result")) failed = 1;
    if (!contains_required(src, "jsonl_observer_request_flat_refresh")) failed = 1;
    if (!contains_required(src, "jsonl_observer_is_flat_refresh_running")) failed = 1;

    free(src);
    return failed;
}

static int test_flat_inspector_no_global_status_contract(void) {
    static const char *paths[] = {
        "src\\ui\\ui_entity_inspector.h",
        "..\\src\\ui\\ui_entity_inspector.h",
        "..\\..\\src\\ui\\ui_entity_inspector.h",
        "..\\..\\..\\src\\ui\\ui_entity_inspector.h",
        "..\\..\\..\\..\\src\\ui\\ui_entity_inspector.h"
    };

    char *src = read_file_from_candidates(paths, sizeof(paths) / sizeof(paths[0]));
    if (!src) return 1;

    int failed = 0;
    if (contains_forbidden(src, "state->last_status")) failed = 1;
    if (contains_forbidden(src, "JSONL Flat Import Error:")) failed = 1;
    if (contains_forbidden(src, "Importing JSONL Flat")) failed = 1;

    free(src);
    return failed;
}

typedef int (*test_fn_t)(void);
typedef struct {
    const char *name;
    test_fn_t fn;
} test_case_t;

int main(void) {
    static const test_case_t tests[] = {
        { "test_flat_inspector_minimal_surface_contract", test_flat_inspector_minimal_surface_contract },
        { "test_flat_inspector_no_global_status_contract", test_flat_inspector_no_global_status_contract },
    };

    for (size_t i = 0; i < (sizeof(tests) / sizeof(tests[0])); ++i) {
        if (tests[i].fn() != 0) {
            fprintf(stderr, "FAILED: %s\n", tests[i].name);
            return 1;
        }
    }
    return 0;
}
