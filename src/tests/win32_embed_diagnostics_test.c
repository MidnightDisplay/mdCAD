#include "../platform/win32_embed.h"

#include "win32_embed_test_stub.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *read_file_text(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return NULL;
    }
    long size = ftell(f);
    if (size < 0 || fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        return NULL;
    }
    char *buffer = (char *)malloc((size_t)size + 1u);
    if (!buffer) {
        fclose(f);
        return NULL;
    }
    if (fread(buffer, 1, (size_t)size, f) != (size_t)size) {
        free(buffer);
        fclose(f);
        return NULL;
    }
    buffer[size] = '\0';
    fclose(f);
    return buffer;
}

static int test_embed_failure_record_overwrites_and_points_to_log(void) {
    mdcad_win32_embed_failure_info_t info = {0};
    char summary[256] = {0};
    char log_path[260] = {0};

    info.failure_class = "startup";
    info.detail = "Embedded startup failed: bad parent hwnd";
    if (!mdcad_win32_embed_write_failure_record(&info, summary, sizeof(summary), log_path, sizeof(log_path))) {
        return 1;
    }

    char *first = read_file_text(log_path);
    if (!first || strstr(first, "bad parent hwnd") == NULL || strstr(summary, "mdcad-embed-crash.log") == NULL) {
        if (first) free(first);
        return 1;
    }
    free(first);

    info.detail = "Embedded startup failed: missing runtime root";
    if (!mdcad_win32_embed_write_failure_record(&info, summary, sizeof(summary), log_path, sizeof(log_path))) {
        return 1;
    }

    char *second = read_file_text(log_path);
    int failed = !second ||
        strstr(second, "missing runtime root") == NULL ||
        strstr(second, "bad parent hwnd") != NULL ||
        strstr(second, "failure_class: startup") == NULL ||
        strstr(second, "summary: ") == NULL;
    if (second) free(second);
    remove(log_path);
    return failed;
}

static int test_embed_runtime_summary_reports_only_known_facts(void) {
    mdcad_win32_embed_failure_info_t info = {0};
    char summary[256] = {0};

    info.failure_class = "runtime";
    info.detail = "mdCAD exited unexpectedly";
    info.has_exception_code = true;
    info.exception_code = 0xC0000005u;
    mdcad_win32_embed_format_failure_summary(&info, summary, sizeof(summary));
    if (strstr(summary, "exception=0xC0000005") == NULL || strstr(summary, "exit_code=") != NULL) {
        return 1;
    }

    memset(summary, 0, sizeof(summary));
    info.has_exception_code = false;
    info.has_exit_code = true;
    info.exit_code = 37u;
    mdcad_win32_embed_format_failure_summary(&info, summary, sizeof(summary));
    return strstr(summary, "exit_code=37") == NULL || strstr(summary, "exception=") != NULL;
}

static int test_parse_failure_diagnostics_apply_only_to_embedded_launches(void) {
    char *embedded_argv[] = { "mdcad.exe", "--embedded", "--parent-hwnd", "0x1234" };
    char *plain_argv[] = { "mdcad.exe", "--jsonl", "C:\\fixture.jsonl" };

    if (!mdcad_win32_embed_args_request_diagnostics(4, embedded_argv)) {
        return 1;
    }
    if (mdcad_win32_embed_args_request_diagnostics(3, plain_argv)) {
        return 1;
    }
    return 0;
}

static int test_app_contract_uses_embed_helpers(void) {
    static const char *paths[] = {
        "src\\app.c",
        "..\\src\\app.c",
        "..\\..\\src\\app.c",
        "..\\..\\..\\src\\app.c",
        "..\\..\\..\\..\\src\\app.c"
    };
    char *src = NULL;
    for (size_t i = 0; i < (sizeof(paths) / sizeof(paths[0])); ++i) {
        src = read_file_text(paths[i]);
        if (src) break;
    }
    if (!src) return 1;

    int failed = 0;
    if (strstr(src, "mdcad_win32_embed_args_request_diagnostics(argc, argv)") == NULL) {
        fprintf(stderr, "missing args helper call\n");
        failed = 1;
    }
    if (strstr(src, "mdcad_win32_embed_fail_startup(") == NULL) {
        fprintf(stderr, "missing startup helper call\n");
        failed = 1;
    }
    if (strstr(src, "mdcad_win32_embed_install_fatal_handlers();") == NULL) {
        fprintf(stderr, "missing fatal handler install\n");
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
        { "test_embed_failure_record_overwrites_and_points_to_log", test_embed_failure_record_overwrites_and_points_to_log },
        { "test_embed_runtime_summary_reports_only_known_facts", test_embed_runtime_summary_reports_only_known_facts },
        { "test_parse_failure_diagnostics_apply_only_to_embedded_launches", test_parse_failure_diagnostics_apply_only_to_embedded_launches },
        { "test_app_contract_uses_embed_helpers", test_app_contract_uses_embed_helpers },
    };

    for (size_t i = 0; i < (sizeof(tests) / sizeof(tests[0])); ++i) {
        if (tests[i].fn() != 0) {
            fprintf(stderr, "FAILED: %s\n", tests[i].name);
            return 1;
        }
    }

    return 0;
}
