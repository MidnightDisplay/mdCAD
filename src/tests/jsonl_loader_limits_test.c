#include "../jsonl_loader.h"
#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif
void sokol_main(void) {}
#ifdef __cplusplus
}
#endif

static int write_repeated_char(FILE *f, char ch, int count) {
    for (int i = 0; i < count; i++) {
        if (fputc(ch, f) == EOF) return 0;
    }
    return 1;
}

static int write_large_valid_jsonl_line(const char *path, int payload_chars) {
    FILE *f = fopen(path, "wb");
    if (!f) return 0;

    const char *prefix =
        "{\"Name\":\"Entry\",\"Description\":\"";
    const char *mid =
        "\",\"Elements\":[{\"Name\":\"P\",\"Description\":\"\",\"Colour\":\"White\","
        "\"Element\":{\"$type\":\"Geo.Point3D\",\"X\":1,\"Y\":2,\"Z\":3}}]}";

    if (fputs(prefix, f) < 0) {
        fclose(f);
        return 0;
    }
    if (!write_repeated_char(f, 'A', payload_chars)) {
        fclose(f);
        return 0;
    }
    if (fputs(mid, f) < 0 || fputc('\n', f) == EOF) {
        fclose(f);
        return 0;
    }

    fclose(f);
    return 1;
}

static int write_invalid_jsonl_line(const char *path) {
    FILE *f = fopen(path, "wb");
    if (!f) return 0;
    if (fputs("{\"Name\":\"Bad\",\"Elements\":[\n", f) < 0) {
        fclose(f);
        return 0;
    }
    fclose(f);
    return 1;
}

static int test_parse_large_single_line_over_4mb(void) {
    const char *fixture = "jsonl_large_line_over_4mb.jsonl";
    // Build >4MB line to prove no fixed parser cap at 4MB line length.
    if (!write_large_valid_jsonl_line(fixture, (5 * 1024 * 1024))) return 1;

    jsonl_parse_state_t parse = {0};
    jsonl_error_t open_err = jsonl_open(fixture, &parse);
    if (open_err != JSONL_OK) {
        remove(fixture);
        return 1;
    }

    while (!jsonl_is_complete(&parse) && parse.error == JSONL_OK) {
        (void)jsonl_parse_lines_chunk(&parse, 32);
    }
    jsonl_close(&parse);

    int failed = 0;
    if (parse.error != JSONL_OK) failed = 1;
    if (parse.data.entry_count != 1) failed = 1;
    if (parse.data.total_elements != 1) failed = 1;

    jsonl_parse_state_free(&parse);
    remove(fixture);
    return failed;
}

static int test_parse_invalid_line_returns_explicit_error(void) {
    const char *fixture = "jsonl_invalid_line_should_error.jsonl";
    if (!write_invalid_jsonl_line(fixture)) return 1;

    jsonl_parse_state_t parse = {0};
    jsonl_error_t open_err = jsonl_open(fixture, &parse);
    if (open_err != JSONL_OK) {
        remove(fixture);
        return 1;
    }

    while (!jsonl_is_complete(&parse) && parse.error == JSONL_OK) {
        (void)jsonl_parse_lines_chunk(&parse, 32);
    }
    jsonl_close(&parse);

    int failed = 0;
    if (parse.error != JSONL_ERROR_PARSE_ERROR) failed = 1;
    if (parse.data.entry_count != 0) failed = 1;
    if (parse.data.total_elements != 0) failed = 1;

    jsonl_parse_state_free(&parse);
    remove(fixture);
    return failed;
}

typedef int (*test_fn_t)(void);
typedef struct {
    const char *name;
    test_fn_t fn;
} test_case_t;

int main(void) {
    static const test_case_t tests[] = {
        {"test_parse_large_single_line_over_4mb", test_parse_large_single_line_over_4mb},
        {"test_parse_invalid_line_returns_explicit_error", test_parse_invalid_line_returns_explicit_error},
    };

    for (size_t i = 0; i < (sizeof(tests) / sizeof(tests[0])); ++i) {
        if (tests[i].fn() != 0) {
            fprintf(stderr, "FAILED: %s\n", tests[i].name);
            return 1;
        }
    }
    return 0;
}
