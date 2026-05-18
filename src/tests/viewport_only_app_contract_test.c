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

static const char *find_required(const char *haystack, const char *needle) {
    if (!haystack || !needle) return NULL;
    return strstr(haystack, needle);
}

static int count_occurrences(const char *haystack, const char *needle) {
    int count = 0;
    const char *cursor = haystack;
    size_t needle_len = strlen(needle);
    if (!haystack || !needle || needle_len == 0u) return 0;
    while ((cursor = strstr(cursor, needle)) != NULL) {
        count++;
        cursor += needle_len;
    }
    return count;
}

static int test_viewport_only_app_init_and_layout_contract(void) {
    static const char *paths[] = {
        "src\\app.c",
        "..\\src\\app.c",
        "..\\..\\src\\app.c",
        "..\\..\\..\\src\\app.c",
        "..\\..\\..\\..\\src\\app.c"
    };

    char *src = read_file_from_candidates(paths, sizeof(paths) / sizeof(paths[0]));
    if (!src) return 1;

    int failed = 0;
    if (!find_required(src, "state.launch = launch_config;")) failed = 1;
    if (!find_required(src, "embed_layout_state_t embed_layout_state = embed_layout_state_make(")) failed = 1;
    if (!find_required(src, "state.launch.embedded && state.launch.viewport_only,")) failed = 1;
    if (!find_required(src, "embed_layout_state_viewport_only_store_filename()")) failed = 1;
    if (!find_required(src, "state.ui_visible = !(state.launch.embedded && state.launch.viewport_only);")) failed = 1;

    free(src);
    return failed;
}

static int test_viewport_only_dock_seed_contract(void) {
    static const char *paths[] = {
        "src\\app.c",
        "..\\src\\app.c",
        "..\\..\\src\\app.c",
        "..\\..\\..\\src\\app.c",
        "..\\..\\..\\..\\src\\app.c"
    };

    char *src = read_file_from_candidates(paths, sizeof(paths) / sizeof(paths[0]));
    if (!src) return 1;

    int failed = 0;
    const char *seed_fn = find_required(src, "static void mdcad_seed_embedded_dock_layout(ImGuiID dockspace_id) {");
    const char *viewport_branch = find_required(src, "if (state.launch.viewport_only) {");
    const char *viewport_only_dock = find_required(src, "igDockBuilderDockWindow(\"3D Viewport\", dockspace_id);");
    const char *normal_scene = find_required(src, "igDockBuilderDockWindow(\"Scene Hierarchy\", dock_bottom_left_id);");
    if (!seed_fn || !viewport_branch || !viewport_only_dock || !normal_scene) {
        failed = 1;
    } else if (!(seed_fn < viewport_branch && viewport_branch < normal_scene)) {
        failed = 1;
    }

    free(src);
    return failed;
}

static int test_viewport_only_draw_gate_contract(void) {
    static const char *paths[] = {
        "src\\app.c",
        "..\\src\\app.c",
        "..\\..\\src\\app.c",
        "..\\..\\..\\src\\app.c",
        "..\\..\\..\\..\\src\\app.c"
    };

    char *src = read_file_from_candidates(paths, sizeof(paths) / sizeof(paths[0]));
    if (!src) return 1;

    int failed = 0;
    const char *ui_gate = find_required(src, "if (state.ui_visible) {");
    const char *controls = find_required(src, "ui_controls_draw(&state.controls);");
    const char *viewport = find_required(src, "ui_viewport_draw(&state.viewport);");
    if (!ui_gate || !controls || !viewport) {
        failed = 1;
    } else if (!(ui_gate < controls && controls < viewport)) {
        failed = 1;
    }
    if (count_occurrences(src, "ui_viewport_draw(&state.viewport);") != 1) {
        failed = 1;
    }

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
        { "test_viewport_only_app_init_and_layout_contract", test_viewport_only_app_init_and_layout_contract },
        { "test_viewport_only_dock_seed_contract", test_viewport_only_dock_seed_contract },
        { "test_viewport_only_draw_gate_contract", test_viewport_only_draw_gate_contract },
    };

    for (size_t i = 0; i < (sizeof(tests) / sizeof(tests[0])); ++i) {
        if (tests[i].fn() != 0) {
            fprintf(stderr, "FAILED: %s\n", tests[i].name);
            return 1;
        }
    }
    return 0;
}
