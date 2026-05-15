#include "../ecs/ecs_world.h"
#include "../ecs/ecs_scene.h"
#include "../startup_jsonl_import_controller.h"

#include "win32_embed_test_stub.h"

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

static int make_absolute_path(const char *relative_path, char *buffer, size_t buffer_size) {
    if (!relative_path || !buffer || buffer_size == 0u) {
        return 0;
    }
#if defined(_WIN32)
    return _fullpath(buffer, relative_path, buffer_size) != NULL;
#else
    char cwd[1024];
    if (!getcwd(cwd, sizeof(cwd))) {
        return 0;
    }
    snprintf(buffer, buffer_size, "%s/%s", cwd, relative_path);
    return 1;
#endif
}

static int write_jsonl_fixture_lines(const char *path, int line_count) {
    FILE *f = fopen(path, "wb");
    if (!f) return 0;
    for (int i = 0; i < line_count; ++i) {
        if (fputs("{\"Name\":\"Entry\",\"Elements\":[{\"Name\":\"P\",\"Description\":\"\",\"Colour\":\"White\",\"Element\":{\"$type\":\"Geo.Point3D\",\"X\":1,\"Y\":2,\"Z\":3}}]}\n", f) < 0) {
            fclose(f);
            return 0;
        }
    }
    fclose(f);
    return 1;
}

static int test_startup_jsonl_controller_uses_flat_import_defaults(void) {
    const char *relative_fixture = "startup_jsonl_controller_tiny.jsonl";
    char absolute_fixture[MDCAD_STARTUP_JSONL_PATH_MAX] = {0};
    if (!write_jsonl_fixture_lines(relative_fixture, 1)) return 1;
    if (!make_absolute_path(relative_fixture, absolute_fixture, sizeof(absolute_fixture))) {
        remove(relative_fixture);
        return 1;
    }

    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    startup_jsonl_import_controller_t controller;
    startup_jsonl_import_controller_init(&controller);
    int failed = 0;

    if (!startup_jsonl_import_controller_arm(&controller, absolute_fixture)) failed = 1;
    if (!failed) {
        bool scene_dirty = startup_jsonl_import_controller_tick(&controller, &scene);
        if (!scene_dirty) failed = 1;
        if (controller.state != STARTUP_JSONL_IMPORT_COMPLETE) failed = 1;
        if (controller.job.scale != 1.0f) failed = 1;
        if (!controller.job.use_jsonl_colours) failed = 1;
        if (controller.job.default_colour.x != 1.0f || controller.job.default_colour.y != 1.0f ||
            controller.job.default_colour.z != 1.0f || controller.job.default_colour.w != 1.0f) failed = 1;
        if (controller.job.shift_to_com) failed = 1;
        if (controller.job.rotation_x != 0.0f || controller.job.rotation_y != 0.0f || controller.job.rotation_z != 0.0f) failed = 1;
        if (controller.job.mesh_import_mode != 0) failed = 1;
        if (!controller.job.observer_contract.captured) failed = 1;
        if (controller.job.observer_contract.link_enabled) failed = 1;
        if (strcmp(controller.job.observer_contract.source_path, absolute_fixture) != 0) failed = 1;
    }

    startup_jsonl_import_controller_reset(&controller, &scene);
    ecs_scene_shutdown(&scene);
    ecs_world_shutdown(&world);
    remove(relative_fixture);
    return failed;
}

static int test_startup_jsonl_controller_latches_runtime_error_for_missing_file(void) {
    char absolute_missing[MDCAD_STARTUP_JSONL_PATH_MAX] = {0};
    if (!make_absolute_path("startup_jsonl_controller_missing.jsonl", absolute_missing, sizeof(absolute_missing))) {
        return 1;
    }
    remove(absolute_missing);

    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    startup_jsonl_import_controller_t controller;
    startup_jsonl_import_controller_init(&controller);

    int failed = 0;
    if (!startup_jsonl_import_controller_arm(&controller, absolute_missing)) failed = 1;
    if (!failed) {
        bool scene_dirty = startup_jsonl_import_controller_tick(&controller, &scene);
        if (scene_dirty) failed = 1;
        if (controller.state != STARTUP_JSONL_IMPORT_ERROR) failed = 1;
        if (controller.error_text[0] == '\0') failed = 1;
    }

    startup_jsonl_import_controller_reset(&controller, &scene);
    ecs_scene_shutdown(&scene);
    ecs_world_shutdown(&world);
    return failed;
}

static int test_startup_jsonl_controller_reset_clears_latched_error(void) {
    char absolute_missing[MDCAD_STARTUP_JSONL_PATH_MAX] = {0};
    if (!make_absolute_path("startup_jsonl_controller_missing_reset.jsonl", absolute_missing, sizeof(absolute_missing))) {
        return 1;
    }
    remove(absolute_missing);

    ecs_world_state_t world = {0};
    ecs_scene_t scene = {0};
    ecs_world_init(&world);
    ecs_scene_init(&scene, &world);

    startup_jsonl_import_controller_t controller;
    startup_jsonl_import_controller_init(&controller);

    int failed = 0;
    if (!startup_jsonl_import_controller_arm(&controller, absolute_missing)) failed = 1;
    if (!failed) {
        (void)startup_jsonl_import_controller_tick(&controller, &scene);
        startup_jsonl_import_controller_reset(&controller, &scene);
        if (controller.state != STARTUP_JSONL_IMPORT_IDLE) failed = 1;
        if (controller.error_text[0] != '\0') failed = 1;
        if (controller.status_text[0] != '\0') failed = 1;
        if (controller.requested_path[0] != '\0') failed = 1;
    }

    ecs_scene_shutdown(&scene);
    ecs_world_shutdown(&world);
    return failed;
}

typedef int (*test_fn_t)(void);
typedef struct {
    const char *name;
    test_fn_t fn;
} test_case_t;

int main(void) {
    static const test_case_t tests[] = {
        { "test_startup_jsonl_controller_uses_flat_import_defaults", test_startup_jsonl_controller_uses_flat_import_defaults },
        { "test_startup_jsonl_controller_latches_runtime_error_for_missing_file", test_startup_jsonl_controller_latches_runtime_error_for_missing_file },
        { "test_startup_jsonl_controller_reset_clears_latched_error", test_startup_jsonl_controller_reset_clears_latched_error },
    };

    for (size_t i = 0; i < (sizeof(tests) / sizeof(tests[0])); ++i) {
        if (tests[i].fn() != 0) {
            fprintf(stderr, "FAILED: %s\n", tests[i].name);
            return 1;
        }
    }
    return 0;
}
