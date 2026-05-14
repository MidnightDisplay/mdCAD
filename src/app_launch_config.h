#ifndef APP_LAUNCH_CONFIG_H
#define APP_LAUNCH_CONFIG_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

typedef struct {
    bool embedded;
    uintptr_t parent_hwnd_value;
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
        }
    }

    if (cfg.embedded && !parent_seen) {
        return mdcad_launch_config_set_error(error_buffer, error_buffer_size, "Embedded mode requires --parent-hwnd");
    }
    if (!cfg.embedded && parent_seen) {
        return mdcad_launch_config_set_error(error_buffer, error_buffer_size, "--parent-hwnd requires --embedded");
    }

    *out_cfg = cfg;
    return true;
}

#endif // APP_LAUNCH_CONFIG_H
