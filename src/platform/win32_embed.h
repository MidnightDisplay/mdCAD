#ifndef MDCAD_WIN32_EMBED_H
#define MDCAD_WIN32_EMBED_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "app_launch_config.h"

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

typedef struct {
    bool enabled;
    HWND parent_hwnd;
    uint32_t runtime_cancel_generation;
    bool fatal_handlers_installed;
    LPTOP_LEVEL_EXCEPTION_FILTER previous_exception_filter;
} mdcad_win32_embed_state_t;

extern mdcad_win32_embed_state_t g_mdcad_win32_embed_state;

typedef struct {
    const char* failure_class;
    const char* detail;
    bool has_exit_code;
    uint32_t exit_code;
    bool has_exception_code;
    uint32_t exception_code;
} mdcad_win32_embed_failure_info_t;

static inline const char* mdcad_win32_embed_log_filename(void) {
    return "mdcad-embed-crash.log";
}

static inline bool mdcad_win32_embed_args_request_diagnostics(int argc, char** argv) {
    if (argc <= 0 || !argv) return false;
    for (int i = 1; i < argc; i++) {
        const char* arg = argv[i];
        if (!arg) continue;
        if (strcmp(arg, "--embedded") == 0 || strcmp(arg, "--parent-hwnd") == 0) {
            return true;
        }
    }
    return false;
}

static inline void mdcad_win32_embed_copy_log_path(char* out_log_path, size_t out_log_path_size) {
    if (!out_log_path || out_log_path_size == 0u) return;
    snprintf(out_log_path, out_log_path_size, "%s", mdcad_win32_embed_log_filename());
}

static inline void mdcad_win32_embed_format_failure_summary(const mdcad_win32_embed_failure_info_t* info,
                                                            char* out_summary,
                                                            size_t out_summary_size) {
    if (!out_summary || out_summary_size == 0u) return;
    const char* failure_class = (info && info->failure_class && info->failure_class[0] != '\0')
        ? info->failure_class
        : "runtime";
    const char* detail = (info && info->detail && info->detail[0] != '\0')
        ? info->detail
        : "Embedded runtime failure";

    snprintf(out_summary, out_summary_size, "Embedded %s failure: %s", failure_class, detail);

    size_t used = strlen(out_summary);
    if (info && info->has_exit_code && used < out_summary_size) {
        snprintf(out_summary + used, out_summary_size - used, " (exit_code=%u)", info->exit_code);
        used = strlen(out_summary);
    }
    if (info && info->has_exception_code && used < out_summary_size) {
        snprintf(out_summary + used, out_summary_size - used, " (exception=0x%08X)", info->exception_code);
        used = strlen(out_summary);
    }
    if (used < out_summary_size) {
        snprintf(out_summary + used, out_summary_size - used, ". See %s", mdcad_win32_embed_log_filename());
    }
}

static inline bool mdcad_win32_embed_write_failure_record(const mdcad_win32_embed_failure_info_t* info,
                                                          char* out_summary,
                                                          size_t out_summary_size,
                                                          char* out_log_path,
                                                          size_t out_log_path_size) {
    char summary[512] = {0};
    char log_path[MAX_PATH] = {0};
    time_t now = time(NULL);
    struct tm tm_now = {0};
    char timestamp[64] = {0};

    mdcad_win32_embed_format_failure_summary(info, summary, sizeof(summary));
    mdcad_win32_embed_copy_log_path(log_path, sizeof(log_path));

    if (out_summary && out_summary_size > 0u) {
        snprintf(out_summary, out_summary_size, "%s", summary);
    }
    if (out_log_path && out_log_path_size > 0u) {
        snprintf(out_log_path, out_log_path_size, "%s", log_path);
    }

    if (localtime_s(&tm_now, &now) == 0) {
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%SZ", &tm_now);
    }

    FILE* log_file = fopen(log_path, "wb");
    if (!log_file) {
        return false;
    }

    fprintf(log_file, "timestamp: %s\n", timestamp[0] ? timestamp : "unknown");
    fprintf(log_file, "failure_class: %s\n",
            (info && info->failure_class && info->failure_class[0] != '\0') ? info->failure_class : "runtime");
    fprintf(log_file, "summary: %s\n", summary);
    fprintf(log_file, "detail: %s\n",
            (info && info->detail && info->detail[0] != '\0') ? info->detail : "Embedded runtime failure");
    if (info && info->has_exit_code) {
        fprintf(log_file, "exit_code: %u\n", info->exit_code);
    }
    if (info && info->has_exception_code) {
        fprintf(log_file, "exception_code: 0x%08X\n", info->exception_code);
    }
    fclose(log_file);
    return true;
}

static inline void mdcad_win32_embed_emit_failure_summary(const char* summary) {
    if (!summary || summary[0] == '\0') return;
    fprintf(stderr, "%s\n", summary);
    fflush(stderr);
}

static inline void mdcad_win32_embed_report_failure(const mdcad_win32_embed_failure_info_t* info) {
    char summary[512] = {0};
    char log_path[MAX_PATH] = {0};
    if (!mdcad_win32_embed_write_failure_record(info, summary, sizeof(summary), log_path, sizeof(log_path))) {
        mdcad_win32_embed_format_failure_summary(info, summary, sizeof(summary));
    }
    mdcad_win32_embed_emit_failure_summary(summary);
}

static LONG WINAPI mdcad_win32_embed_unhandled_exception_filter(EXCEPTION_POINTERS* exception_info) {
    mdcad_win32_embed_failure_info_t info = {0};
    info.failure_class = "runtime";
    info.detail = "mdCAD terminated with an unhandled embedded exception";
    if (exception_info && exception_info->ExceptionRecord) {
        info.has_exception_code = true;
        info.exception_code = exception_info->ExceptionRecord->ExceptionCode;
    }
    mdcad_win32_embed_report_failure(&info);
    return EXCEPTION_EXECUTE_HANDLER;
}

static inline void mdcad_win32_embed_install_fatal_handlers(void) {
    if (!g_mdcad_win32_embed_state.enabled || g_mdcad_win32_embed_state.fatal_handlers_installed) {
        return;
    }
    g_mdcad_win32_embed_state.previous_exception_filter =
        SetUnhandledExceptionFilter(mdcad_win32_embed_unhandled_exception_filter);
    g_mdcad_win32_embed_state.fatal_handlers_installed = true;
}

static inline void mdcad_win32_embed_set_config(const mdcad_launch_config_t* cfg) {
    g_mdcad_win32_embed_state.enabled = (cfg != NULL) && cfg->embedded;
    g_mdcad_win32_embed_state.parent_hwnd = g_mdcad_win32_embed_state.enabled
        ? (HWND)cfg->parent_hwnd_value
        : NULL;
    g_mdcad_win32_embed_state.runtime_cancel_generation = 0u;
    g_mdcad_win32_embed_state.fatal_handlers_installed = false;
    g_mdcad_win32_embed_state.previous_exception_filter = NULL;
}

static inline bool mdcad_win32_embed_is_enabled(void) {
    return g_mdcad_win32_embed_state.enabled;
}

static inline HWND mdcad_win32_embed_parent_hwnd(void) {
    return g_mdcad_win32_embed_state.parent_hwnd;
}

static inline void mdcad_win32_embed_note_runtime_cancel(void) {
    if (g_mdcad_win32_embed_state.enabled) {
        g_mdcad_win32_embed_state.runtime_cancel_generation++;
    }
}

static inline bool mdcad_win32_embed_consume_runtime_cancel(uint32_t* last_seen_generation) {
    if (!last_seen_generation || !g_mdcad_win32_embed_state.enabled) {
        return false;
    }
    if (g_mdcad_win32_embed_state.runtime_cancel_generation == *last_seen_generation) {
        return false;
    }
    *last_seen_generation = g_mdcad_win32_embed_state.runtime_cancel_generation;
    return true;
}

static inline bool mdcad_win32_embed_parent_chain_valid(HWND child_hwnd) {
    if (!g_mdcad_win32_embed_state.enabled) {
        return true;
    }
    if (!IsWindow(g_mdcad_win32_embed_state.parent_hwnd)) {
        return false;
    }
    if (!child_hwnd || !IsWindow(child_hwnd)) {
        return false;
    }
    return GetParent(child_hwnd) == g_mdcad_win32_embed_state.parent_hwnd;
}

static inline void mdcad_win32_embed_fail_startup(const char* reason) {
    mdcad_win32_embed_failure_info_t info = {0};
    info.failure_class = "startup";
    info.detail = (reason && reason[0] != '\0')
        ? reason
        : "Embedded startup failed: unknown child-window startup error";
    mdcad_win32_embed_report_failure(&info);
    exit(2);
}

#else

typedef void* HWND;
typedef struct {
    const char* failure_class;
    const char* detail;
    bool has_exit_code;
    uint32_t exit_code;
    bool has_exception_code;
    uint32_t exception_code;
} mdcad_win32_embed_failure_info_t;

static inline const char* mdcad_win32_embed_log_filename(void) {
    return "mdcad-embed-crash.log";
}

static inline bool mdcad_win32_embed_args_request_diagnostics(int argc, char** argv) {
    (void)argc;
    (void)argv;
    return false;
}

static inline void mdcad_win32_embed_copy_log_path(char* out_log_path, size_t out_log_path_size) {
    if (!out_log_path || out_log_path_size == 0u) return;
    snprintf(out_log_path, out_log_path_size, "%s", mdcad_win32_embed_log_filename());
}

static inline void mdcad_win32_embed_format_failure_summary(const mdcad_win32_embed_failure_info_t* info,
                                                            char* out_summary,
                                                            size_t out_summary_size) {
    (void)info;
    if (!out_summary || out_summary_size == 0u) return;
    snprintf(out_summary, out_summary_size, "Embedded runtime failure. See %s", mdcad_win32_embed_log_filename());
}

static inline bool mdcad_win32_embed_write_failure_record(const mdcad_win32_embed_failure_info_t* info,
                                                          char* out_summary,
                                                          size_t out_summary_size,
                                                          char* out_log_path,
                                                          size_t out_log_path_size) {
    (void)info;
    mdcad_win32_embed_format_failure_summary(info, out_summary, out_summary_size);
    mdcad_win32_embed_copy_log_path(out_log_path, out_log_path_size);
    return false;
}

static inline void mdcad_win32_embed_emit_failure_summary(const char* summary) {
    (void)summary;
}

static inline void mdcad_win32_embed_report_failure(const mdcad_win32_embed_failure_info_t* info) {
    (void)info;
}

static inline void mdcad_win32_embed_install_fatal_handlers(void) {
}

static inline void mdcad_win32_embed_set_config(const mdcad_launch_config_t* cfg) {
    (void)cfg;
}

static inline bool mdcad_win32_embed_is_enabled(void) {
    return false;
}

static inline HWND mdcad_win32_embed_parent_hwnd(void) {
    return NULL;
}

static inline void mdcad_win32_embed_note_runtime_cancel(void) {
}

static inline bool mdcad_win32_embed_consume_runtime_cancel(uint32_t* last_seen_generation) {
    (void)last_seen_generation;
    return false;
}

static inline bool mdcad_win32_embed_parent_chain_valid(HWND child_hwnd) {
    (void)child_hwnd;
    return true;
}

static inline void mdcad_win32_embed_fail_startup(const char* reason) {
    (void)reason;
}

#endif

#endif // MDCAD_WIN32_EMBED_H
