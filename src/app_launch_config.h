#ifndef APP_LAUNCH_CONFIG_H
#define APP_LAUNCH_CONFIG_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define MDCAD_STARTUP_JSONL_PATH_MAX 512

typedef struct {
    bool embedded;
    uintptr_t parent_hwnd_value;
    bool startup_jsonl_live_refresh;
    char startup_jsonl_path[MDCAD_STARTUP_JSONL_PATH_MAX];
} mdcad_launch_config_t;

static inline bool mdcad_launch_config_set_error(char* error_buffer,
                                                 size_t error_buffer_size,
                                                 const char* message) {
    if (error_buffer && (error_buffer_size > 0)) {
        snprintf(error_buffer, error_buffer_size, "%s", message ? message : "Unknown launch configuration error");
        error_buffer[error_buffer_size - 1] = '\0';
    }
    return false;
}

static inline bool mdcad_launch_config_parse_parent_hwnd(const char* value,
                                                         uintptr_t* out_value,
                                                         char* error_buffer,
                                                         size_t error_buffer_size) {
    if (!value || !out_value) {
        return mdcad_launch_config_set_error(error_buffer, error_buffer_size, "Invalid --parent-hwnd");
    }
    errno = 0;
    char* end = NULL;
    unsigned long long parsed = strtoull(value, &end, 0);
    if ((errno != 0) || (end == value) || (*end != '\0') || (parsed == 0ull) || (parsed > (unsigned long long)UINTPTR_MAX)) {
        return mdcad_launch_config_set_error(error_buffer, error_buffer_size, "Invalid --parent-hwnd");
    }
    *out_value = (uintptr_t)parsed;
    return true;
}

static inline bool mdcad_launch_config_path_is_absolute(const char* path) {
    if (!path || path[0] == '\0') {
        return false;
    }
#if defined(_WIN32)
    return ((strlen(path) > 2u) && path[1] == ':' && (path[2] == '\\' || path[2] == '/')) ||
           (path[0] == '\\' && path[1] == '\\');
#else
    return path[0] == '/';
#endif
}

static inline bool mdcad_launch_config_copy_startup_jsonl_path(const char* value,
                                                               char* out_path,
                                                               size_t out_path_size,
                                                               char* error_buffer,
                                                               size_t error_buffer_size) {
    if (!value || !out_path || (out_path_size == 0u)) {
        return mdcad_launch_config_set_error(error_buffer, error_buffer_size, "Invalid --jsonl");
    }
    if (!mdcad_launch_config_path_is_absolute(value)) {
        return mdcad_launch_config_set_error(error_buffer, error_buffer_size, "--jsonl requires an absolute path");
    }
    int written = snprintf(out_path, out_path_size, "%s", value);
    if ((written < 0) || ((size_t)written >= out_path_size)) {
        return mdcad_launch_config_set_error(error_buffer, error_buffer_size, "--jsonl path is too long");
    }
    return true;
}

static inline bool mdcad_launch_config_parse(int argc,
                                             char** argv,
                                             mdcad_launch_config_t* out_cfg,
                                             char* error_buffer,
                                             size_t error_buffer_size) {
    if (!out_cfg) {
        return mdcad_launch_config_set_error(error_buffer, error_buffer_size, "Launch config output is required");
    }

    mdcad_launch_config_t cfg = {0};
    bool embedded_seen = false;
    bool parent_seen = false;
    bool jsonl_seen = false;
    bool jsonl_live_refresh_seen = false;

    if (error_buffer && (error_buffer_size > 0)) {
        error_buffer[0] = '\0';
    }

    for (int i = 1; i < argc; ++i) {
        const char* arg = argv[i];
        if (!arg) {
            continue;
        }

        if (strcmp(arg, "--embedded") == 0) {
            if (embedded_seen) {
                return mdcad_launch_config_set_error(error_buffer, error_buffer_size, "Duplicate --embedded flag");
            }
            embedded_seen = true;
            cfg.embedded = true;
            continue;
        }

        if (strcmp(arg, "--parent-hwnd") == 0) {
            if (parent_seen) {
                return mdcad_launch_config_set_error(error_buffer, error_buffer_size, "Duplicate --parent-hwnd flag");
            }
            if ((i + 1) >= argc || !argv[i + 1] || (argv[i + 1][0] == '\0') ||
                ((argv[i + 1][0] == '-') && (argv[i + 1][1] == '-'))) {
                return mdcad_launch_config_set_error(error_buffer, error_buffer_size, "Missing value for --parent-hwnd");
            }
            if (!mdcad_launch_config_parse_parent_hwnd(argv[i + 1], &cfg.parent_hwnd_value, error_buffer, error_buffer_size)) {
                return false;
            }
            parent_seen = true;
            ++i;
            continue;
        }

        if (strcmp(arg, "--jsonl") == 0) {
            if (jsonl_seen) {
                return mdcad_launch_config_set_error(error_buffer, error_buffer_size, "Duplicate --jsonl flag");
            }
            if ((i + 1) >= argc || !argv[i + 1] || (argv[i + 1][0] == '\0') ||
                ((argv[i + 1][0] == '-') && (argv[i + 1][1] == '-'))) {
                return mdcad_launch_config_set_error(error_buffer, error_buffer_size, "Missing value for --jsonl");
            }
            if (!mdcad_launch_config_copy_startup_jsonl_path(argv[i + 1],
                                                             cfg.startup_jsonl_path,
                                                             sizeof(cfg.startup_jsonl_path),
                                                             error_buffer,
                                                             error_buffer_size)) {
                return false;
            }
            jsonl_seen = true;
            ++i;
            continue;
        }

        if (strcmp(arg, "--jsonl-live-refresh") == 0) {
            if (jsonl_live_refresh_seen) {
                return mdcad_launch_config_set_error(error_buffer, error_buffer_size, "Duplicate --jsonl-live-refresh flag");
            }
            cfg.startup_jsonl_live_refresh = true;
            jsonl_live_refresh_seen = true;
        }
    }

    if (cfg.embedded && !parent_seen) {
        return mdcad_launch_config_set_error(error_buffer, error_buffer_size, "Embedded mode requires --parent-hwnd");
    }
    if (!cfg.embedded && parent_seen) {
        return mdcad_launch_config_set_error(error_buffer, error_buffer_size, "--parent-hwnd requires --embedded");
    }
    if (cfg.startup_jsonl_live_refresh && !jsonl_seen) {
        return mdcad_launch_config_set_error(error_buffer, error_buffer_size, "--jsonl-live-refresh requires --jsonl");
    }

    *out_cfg = cfg;
    return true;
}

#endif // APP_LAUNCH_CONFIG_H
