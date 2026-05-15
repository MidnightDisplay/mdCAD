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

static int test_startup_jsonl_app_state_and_init_contract(void) {
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
    if (!find_required(src, "startup_jsonl_import_controller_t startup_jsonl_import_controller;")) failed = 1;
    if (!find_required(src, "#include \"startup_jsonl_import_controller.h\"")) failed = 1;
    if (!find_required(src, "startup_jsonl_import_controller_init(&state.startup_jsonl_import_controller);")) failed = 1;
    if (!find_required(src, "if (state.launch.startup_jsonl_path[0] != '\\0') {")) failed = 1;
    if (!find_required(src, "startup_jsonl_import_controller_arm(&state.startup_jsonl_import_controller,")) failed = 1;

    free(src);
    return failed;
}

static int test_startup_jsonl_app_frame_ownership_contract(void) {
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
    const char *new_frame = find_required(src, "simgui_new_frame(&(simgui_frame_desc_t){");
    const char *tick = find_required(src,
        "bool startup_jsonl_scene_dirty = startup_jsonl_import_controller_tick(&state.startup_jsonl_import_controller, &state.ecs_scene);");
    const char *mark_dirty = find_required(src, "if (startup_jsonl_scene_dirty) {");
    const char *ui_gate = find_required(src, "if (state.ui_visible) {");
    if (!new_frame || !tick || !mark_dirty || !ui_gate) {
        failed = 1;
    } else if (!(new_frame < tick && tick < mark_dirty && mark_dirty < ui_gate)) {
        failed = 1;
    }

    free(src);
    return failed;
}

static int test_startup_jsonl_app_overlay_contract(void) {
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
    const char *helper = find_required(src, "static void mdcad_draw_startup_jsonl_import_overlay(void) {");
    const char *title = find_required(src, "igBegin(\"Startup JSONL Import\", NULL,");
    const char *dismiss = find_required(src, "startup_jsonl_import_controller_dismiss_error(&state.startup_jsonl_import_controller);");
    const char *fps_draw = find_required(src, "ui_fps_debug_draw(&state.fps_debug);");
    const char *overlay_call = find_required(src, "mdcad_draw_startup_jsonl_import_overlay();");
    const char *constraint_menu = find_required(src, "mdcad_draw_constraint_context_menu();");
    if (!helper || !title || !dismiss || !fps_draw || !overlay_call || !constraint_menu) {
        failed = 1;
    } else if (!(fps_draw < overlay_call && overlay_call < constraint_menu)) {
        failed = 1;
    }

    free(src);
    return failed;
}

static int test_startup_jsonl_app_cleanup_contract(void) {
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
    const char *reset = find_required(src,
        "startup_jsonl_import_controller_reset(&state.startup_jsonl_import_controller, &state.ecs_scene);");
    const char *scene_shutdown = find_required(src, "ecs_scene_shutdown(&state.ecs_scene);");
    if (!reset || !scene_shutdown) {
        failed = 1;
    } else if (!(reset < scene_shutdown)) {
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
        { "test_startup_jsonl_app_state_and_init_contract", test_startup_jsonl_app_state_and_init_contract },
        { "test_startup_jsonl_app_frame_ownership_contract", test_startup_jsonl_app_frame_ownership_contract },
        { "test_startup_jsonl_app_overlay_contract", test_startup_jsonl_app_overlay_contract },
        { "test_startup_jsonl_app_cleanup_contract", test_startup_jsonl_app_cleanup_contract },
    };

    for (size_t i = 0; i < (sizeof(tests) / sizeof(tests[0])); ++i) {
        if (tests[i].fn() != 0) {
            fprintf(stderr, "FAILED: %s\n", tests[i].name);
            return 1;
        }
    }
    return 0;
}
