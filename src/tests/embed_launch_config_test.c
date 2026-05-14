#include "../app_launch_config.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define ARG(str) ((char*)(str))

static int expect_parse_success(char** argv, int argc, bool expected_embedded, uintptr_t expected_parent) {
    mdcad_launch_config_t cfg = {0};
    char error[128] = {0};
    if (!mdcad_launch_config_parse(argc, argv, &cfg, error, sizeof(error))) {
        fprintf(stderr, "Unexpected parse failure: %s\n", error);
        return 1;
    }
    if (cfg.embedded != expected_embedded) {
        fprintf(stderr, "Unexpected embedded flag state\n");
        return 1;
    }
    if (cfg.parent_hwnd_value != expected_parent) {
        fprintf(stderr, "Unexpected parent HWND value\n");
        return 1;
    }
    if (error[0] != '\0') {
        fprintf(stderr, "Unexpected error buffer content\n");
        return 1;
    }
    return 0;
}

static int expect_parse_failure_contains(char** argv, int argc, const char* expected_substring) {
    mdcad_launch_config_t cfg = {0};
    char error[128] = {0};
    if (mdcad_launch_config_parse(argc, argv, &cfg, error, sizeof(error))) {
        fprintf(stderr, "Unexpected parse success\n");
        return 1;
    }
    if (strstr(error, expected_substring) == NULL) {
        fprintf(stderr, "Expected error containing '%s' but got '%s'\n", expected_substring, error);
        return 1;
    }
    return 0;
}

static int test_embed_launch_config_no_embed_flags(void) {
    char* argv[] = { ARG("mdcad"), ARG("--jsonl"), ARG("data.jsonl") };
    return expect_parse_success(argv, 3, false, 0);
}

static int test_embed_launch_config_valid_hex_parent_hwnd(void) {
    char* argv[] = { ARG("mdcad"), ARG("--embedded"), ARG("--parent-hwnd"), ARG("0x12345678") };
    return expect_parse_success(argv, 4, true, (uintptr_t)0x12345678ull);
}

static int test_embed_launch_config_valid_decimal_parent_hwnd(void) {
    char* argv[] = { ARG("mdcad"), ARG("--embedded"), ARG("--parent-hwnd"), ARG("305419896") };
    return expect_parse_success(argv, 4, true, (uintptr_t)305419896ull);
}

static int test_embed_launch_config_missing_parent_hwnd(void) {
    char* argv[] = { ARG("mdcad"), ARG("--embedded") };
    return expect_parse_failure_contains(argv, 2, "requires --parent-hwnd");
}

static int test_embed_launch_config_parent_without_embedded(void) {
    char* argv[] = { ARG("mdcad"), ARG("--parent-hwnd"), ARG("0x12345678") };
    return expect_parse_failure_contains(argv, 3, "requires --embedded");
}

static int test_embed_launch_config_invalid_parent_hwnd(void) {
    char* argv[] = { ARG("mdcad"), ARG("--embedded"), ARG("--parent-hwnd"), ARG("0") };
    return expect_parse_failure_contains(argv, 4, "Invalid --parent-hwnd");
}

static int test_embed_launch_config_duplicate_embedded_flag(void) {
    char* argv[] = { ARG("mdcad"), ARG("--embedded"), ARG("--embedded"), ARG("--parent-hwnd"), ARG("0x12345678") };
    return expect_parse_failure_contains(argv, 5, "Duplicate --embedded flag");
}

typedef int (*test_fn_t)(void);
typedef struct {
    const char* name;
    test_fn_t fn;
} test_case_t;

int main(void) {
    static const test_case_t tests[] = {
        { "test_embed_launch_config_no_embed_flags", test_embed_launch_config_no_embed_flags },
        { "test_embed_launch_config_valid_hex_parent_hwnd", test_embed_launch_config_valid_hex_parent_hwnd },
        { "test_embed_launch_config_valid_decimal_parent_hwnd", test_embed_launch_config_valid_decimal_parent_hwnd },
        { "test_embed_launch_config_missing_parent_hwnd", test_embed_launch_config_missing_parent_hwnd },
        { "test_embed_launch_config_parent_without_embedded", test_embed_launch_config_parent_without_embedded },
        { "test_embed_launch_config_invalid_parent_hwnd", test_embed_launch_config_invalid_parent_hwnd },
        { "test_embed_launch_config_duplicate_embedded_flag", test_embed_launch_config_duplicate_embedded_flag },
    };

    for (size_t i = 0; i < (sizeof(tests) / sizeof(tests[0])); ++i) {
        if (tests[i].fn() != 0) {
            fprintf(stderr, "FAILED: %s\n", tests[i].name);
            return 1;
        }
    }
    return 0;
}
