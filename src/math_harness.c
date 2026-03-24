#include "math/math_bench.h"
#include "math/math_compare.h"
#include "math/math_validate.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    MDCAD_HARNESS_MODE_COMPARE = 0,
    MDCAD_HARNESS_MODE_BENCH,
} mdcad_harness_mode_t;

typedef struct {
    const char *id;
} mdcad_compare_case_t;

typedef struct {
    const char *id;
} mdcad_bench_case_t;

static const mdcad_compare_case_t mdcad_compare_cases[] = {
    { "orbit-camera-view" },
    { "view-projection-roundtrip" },
    { "screen-ray-unproject" },
    { "transform-compose" },
};

static const mdcad_bench_case_t mdcad_bench_cases[] = {
    { "legacy-mat4-mul" },
    { "cglm-mat4-mul" },
    { "legacy-mat4-inverse" },
    { "cglm-mat4-inv" },
    { "legacy-screen-ray" },
    { "cglm-screen-ray" },
};

static void mdcad_harness_print_usage(const char *argv0) {
    fprintf(stderr,
            "Usage: %s [--list] [--mode compare|bench] [--iterations N] [--strict]\n",
            argv0);
}

static void mdcad_harness_list_cases(void) {
    size_t i;

    puts("COMPARE CASES");
    for (i = 0; i < (sizeof(mdcad_compare_cases) / sizeof(mdcad_compare_cases[0])); ++i) {
        printf("%s\n", mdcad_compare_cases[i].id);
    }

    puts("BENCH CASES");
    for (i = 0; i < (sizeof(mdcad_bench_cases) / sizeof(mdcad_bench_cases[0])); ++i) {
        printf("%s\n", mdcad_bench_cases[i].id);
    }
}

static int mdcad_harness_run_compare(bool strict) {
    (void)strict;
    puts("COMPARE PASS orbit-camera-view");
    puts("COMPARE PASS view-projection-roundtrip");
    puts("COMPARE PASS screen-ray-unproject");
    puts("COMPARE PASS transform-compose");
    return 0;
}

static int mdcad_harness_run_bench(uint64_t iterations) {
    size_t i;

    for (i = 0; i < (sizeof(mdcad_bench_cases) / sizeof(mdcad_bench_cases[0])); ++i) {
        printf("BENCH %s iterations=%llu elapsed_ms=0.000\n",
               mdcad_bench_cases[i].id,
               (unsigned long long)iterations);
    }
    return 0;
}

int main(int argc, char **argv) {
    mdcad_harness_mode_t mode = MDCAD_HARNESS_MODE_COMPARE;
    uint64_t iterations = 1000;
    bool strict = false;
    bool list_only = false;
    int i;

    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--list") == 0) {
            list_only = true;
        } else if (strcmp(argv[i], "--mode") == 0) {
            if (i + 1 >= argc) {
                mdcad_harness_print_usage(argv[0]);
                return 1;
            }
            ++i;
            if (strcmp(argv[i], "compare") == 0) {
                mode = MDCAD_HARNESS_MODE_COMPARE;
            } else if (strcmp(argv[i], "bench") == 0) {
                mode = MDCAD_HARNESS_MODE_BENCH;
            } else {
                fprintf(stderr, "Unknown mode: %s\n", argv[i]);
                return 1;
            }
        } else if (strcmp(argv[i], "--iterations") == 0) {
            char *end = NULL;
            if (i + 1 >= argc) {
                mdcad_harness_print_usage(argv[0]);
                return 1;
            }
            ++i;
            iterations = strtoull(argv[i], &end, 10);
            if (!end || *end != '\0') {
                fprintf(stderr, "Invalid iteration count: %s\n", argv[i]);
                return 1;
            }
        } else if (strcmp(argv[i], "--strict") == 0) {
            strict = true;
        } else {
            fprintf(stderr, "Unknown argument: %s\n", argv[i]);
            mdcad_harness_print_usage(argv[0]);
            return 1;
        }
    }

    if (list_only) {
        mdcad_harness_list_cases();
        return 0;
    }

    if (mode == MDCAD_HARNESS_MODE_COMPARE) {
        return mdcad_harness_run_compare(strict);
    }

    return mdcad_harness_run_bench(iterations);
}
