#include "../jsonl_import_job.h"
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

static int write_fixture_lines(const char *path, int line_count) {
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

static int test_flat_import_options_to_job_start_contract(void) {
    const char *fixture = "jsonl_flat_options_contract_tiny.jsonl";
    if (!write_fixture_lines(fixture, 1)) return 1;

    jsonl_import_job_t job = {0};
    jsonl_import_job_init(&job);

    vec4_t default_colour = vec4_make(0.2f, 0.4f, 0.8f, 1.0f);
    bool started = jsonl_import_job_start(&job,
                                          fixture,
                                          0.001f,
                                          false,
                                          default_colour,
                                          true,
                                          0.11f, 0.22f, 0.33f);
    if (!started) {
        remove(fixture);
        return 1;
    }

    jsonl_import_job_set_mesh_mode(&job, 1);

    int failed = 0;
    if (job.scale != 0.001f) failed = 1;
    if (job.use_jsonl_colours) failed = 1;
    if (job.default_colour.x != 0.2f || job.default_colour.y != 0.4f || job.default_colour.z != 0.8f) failed = 1;
    if (!job.shift_to_com) failed = 1;
    if (job.rotation_x != 0.11f || job.rotation_y != 0.22f || job.rotation_z != 0.33f) failed = 1;
    if (job.mesh_import_mode != 1) failed = 1;
    if (!jsonl_import_job_should_sync(&job)) failed = 1;

    jsonl_parse_state_free(&job.parse_state);
    remove(fixture);
    return failed;
}

static int test_flat_import_sync_async_contract_threshold(void) {
    const char *tiny = "jsonl_flat_threshold_tiny.jsonl";
    const char *large = "jsonl_flat_threshold_large.jsonl";
    if (!write_fixture_lines(tiny, 1)) return 1;
    if (!write_fixture_lines(large, 6)) {
        remove(tiny);
        return 1;
    }

    jsonl_import_job_t tiny_job = {0};
    jsonl_import_job_t large_job = {0};
    jsonl_import_job_init(&tiny_job);
    jsonl_import_job_init(&large_job);

    vec4_t default_colour = vec4_make(1.0f, 1.0f, 1.0f, 1.0f);
    bool tiny_ok = jsonl_import_job_start(&tiny_job, tiny, 1.0f, true, default_colour, false, 0.0f, 0.0f, 0.0f);
    bool large_ok = jsonl_import_job_start(&large_job, large, 1.0f, true, default_colour, false, 0.0f, 0.0f, 0.0f);

    int failed = 0;
    if (!tiny_ok || !large_ok) failed = 1;
    if (!jsonl_import_job_should_sync(&tiny_job)) failed = 1;
    if (jsonl_import_job_should_sync(&large_job)) failed = 1;

    jsonl_parse_state_free(&tiny_job.parse_state);
    jsonl_parse_state_free(&large_job.parse_state);
    remove(tiny);
    remove(large);
    return failed;
}

static int test_flat_import_observer_contract_api_declared(void) {
    static const char *paths[] = {
        "src\\jsonl_import_job.h",
        "..\\src\\jsonl_import_job.h",
        "..\\..\\src\\jsonl_import_job.h",
        "..\\..\\..\\src\\jsonl_import_job.h",
        "..\\..\\..\\..\\src\\jsonl_import_job.h"
    };

    char *src = read_file_from_candidates(paths, sizeof(paths) / sizeof(paths[0]));
    if (!src) return 1;

    int failed = 0;
    if (strstr(src, "jsonl_import_job_set_observer_contract") == NULL) failed = 1;
    if (strstr(src, "observer_contract") == NULL) failed = 1;
    if (strstr(src, "JSONL_OBSERVER_PATH_MAX") == NULL) failed = 1;

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
        { "test_flat_import_options_to_job_start_contract", test_flat_import_options_to_job_start_contract },
        { "test_flat_import_sync_async_contract_threshold", test_flat_import_sync_async_contract_threshold },
        { "test_flat_import_observer_contract_api_declared", test_flat_import_observer_contract_api_declared },
    };

    for (size_t i = 0; i < (sizeof(tests) / sizeof(tests[0])); ++i) {
        if (tests[i].fn() != 0) {
            fprintf(stderr, "FAILED: %s\n", tests[i].name);
            return 1;
        }
    }
    return 0;
}
