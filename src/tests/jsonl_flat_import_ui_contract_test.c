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

static int test_flat_import_menu_entry_contract(void) {
    static const char *paths[] = {
        "src\\ui\\ui_scene_hierarchy.h",
        "..\\src\\ui\\ui_scene_hierarchy.h",
        "..\\..\\src\\ui\\ui_scene_hierarchy.h",
        "..\\..\\..\\src\\ui\\ui_scene_hierarchy.h",
        "..\\..\\..\\..\\src\\ui\\ui_scene_hierarchy.h"
    };

    char *src = read_file_from_candidates(paths, sizeof(paths) / sizeof(paths[0]));
    if (!src) return 1;

    int failed = 0;
    if (!contains_required(src, "Import JSONL (Flat Large Dump)...")) failed = 1;
    if (!contains_required(src, "Import JSONL Geometry Log...")) failed = 1;
    if (!contains_required(src, "Import JSONL as Sketch")) failed = 1;

    free(src);
    return failed;
}

static int test_flat_import_dialog_contract_controls(void) {
    static const char *paths[] = {
        "src\\ui\\ui_scene_hierarchy.h",
        "..\\src\\ui\\ui_scene_hierarchy.h",
        "..\\..\\src\\ui\\ui_scene_hierarchy.h",
        "..\\..\\..\\src\\ui\\ui_scene_hierarchy.h",
        "..\\..\\..\\..\\src\\ui\\ui_scene_hierarchy.h"
    };

    char *src = read_file_from_candidates(paths, sizeof(paths) / sizeof(paths[0]));
    if (!src) return 1;

    int failed = 0;
    if (!contains_required(src, "Link file for refresh (optional)")) failed = 1;
    if (!contains_required(src, "Import Colours##jsonl_flat")) failed = 1;
    if (!contains_required(src, "Default Colour##jsonl_flat")) failed = 1;
    if (!contains_required(src, "Shift to Centre of Mass##jsonl_flat")) failed = 1;
    if (!contains_required(src, "X##jsonl_flat_rot")) failed = 1;
    if (!contains_required(src, "Y##jsonl_flat_rot")) failed = 1;
    if (!contains_required(src, "Z##jsonl_flat_rot")) failed = 1;
    if (!contains_required(src, "Single Mesh Entity (efficient)##jsonl_flat")) failed = 1;
    if (!contains_required(src, "Individual Triangles (selectable)##jsonl_flat")) failed = 1;

    free(src);
    return failed;
}

static int test_flat_import_dedicated_state_and_submit_wiring_contract(void) {
    static const char *paths[] = {
        "src\\ui\\ui_scene_hierarchy.h",
        "..\\src\\ui\\ui_scene_hierarchy.h",
        "..\\..\\src\\ui\\ui_scene_hierarchy.h",
        "..\\..\\..\\src\\ui\\ui_scene_hierarchy.h",
        "..\\..\\..\\..\\src\\ui\\ui_scene_hierarchy.h"
    };

    char *src = read_file_from_candidates(paths, sizeof(paths) / sizeof(paths[0]));
    if (!src) return 1;

    int failed = 0;
    if (!contains_required(src, "jsonl_flat_browser")) failed = 1;
    if (!contains_required(src, "jsonl_flat_import_popup_open")) failed = 1;
    if (!contains_required(src, "jsonl_flat_import_job")) failed = 1;
    if (!contains_required(src, "jsonl_quick_scan_mesh(path")) failed = 1;
    if (!contains_required(src, "jsonl_import_job_set_observer_contract(&state->jsonl_flat_import_job")) failed = 1;
    if (!contains_required(src, "jsonl_import_job_should_sync(&state->jsonl_flat_import_job)")) failed = 1;

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
        { "test_flat_import_menu_entry_contract", test_flat_import_menu_entry_contract },
        { "test_flat_import_dialog_contract_controls", test_flat_import_dialog_contract_controls },
        { "test_flat_import_dedicated_state_and_submit_wiring_contract",
          test_flat_import_dedicated_state_and_submit_wiring_contract },
    };

    for (size_t i = 0; i < (sizeof(tests) / sizeof(tests[0])); ++i) {
        if (tests[i].fn() != 0) {
            fprintf(stderr, "FAILED: %s\n", tests[i].name);
            return 1;
        }
    }
    return 0;
}
