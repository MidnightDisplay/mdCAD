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

static int test_camera_shortcut_uses_embedded_and_standalone_shortcut_seams(void) {
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
    if (!find_required(src, "mdcad_embedded_shortcut_pressed(ImGuiKey_F, dockspace_id)")) failed = 1;
    if (!find_required(src, "igIsKeyPressed_Bool(ImGuiKey_F, false)")) failed = 1;
    if (count_occurrences(src, "orbit_camera_reset(&state.camera);") != 2) failed = 1;

    free(src);
    return failed;
}

static int test_camera_shortcut_does_not_reopen_focus_or_direct_camera_resets(void) {
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
    if (find_required(src, "state.camera.distance =")) failed = 1;
    if (find_required(src, "state.camera.azimuth =")) failed = 1;
    if (find_required(src, "state.camera.elevation =")) failed = 1;
    if (find_required(src, "SetFocus(")) failed = 1;
    if (find_required(src, "SetActiveWindow(")) failed = 1;

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
        { "test_camera_shortcut_uses_embedded_and_standalone_shortcut_seams", test_camera_shortcut_uses_embedded_and_standalone_shortcut_seams },
        { "test_camera_shortcut_does_not_reopen_focus_or_direct_camera_resets", test_camera_shortcut_does_not_reopen_focus_or_direct_camera_resets },
    };

    for (size_t i = 0; i < (sizeof(tests) / sizeof(tests[0])); ++i) {
        if (tests[i].fn() != 0) {
            fprintf(stderr, "FAILED: %s\n", tests[i].name);
            return 1;
        }
    }
    return 0;
}
