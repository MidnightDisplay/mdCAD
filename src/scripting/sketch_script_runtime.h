//------------------------------------------------------------------------------
// sketch_script_runtime.h - Lua runtime baseline checks for scripting
//------------------------------------------------------------------------------
#ifndef SKETCH_SCRIPT_RUNTIME_H
#define SKETCH_SCRIPT_RUNTIME_H

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define SKETCH_SCRIPT_LUA_REQUIRED_MAJOR 5
#define SKETCH_SCRIPT_LUA_REQUIRED_MINOR 4

typedef struct {
    int major;
    int minor;
    int patch;
    char text[32];
} sketch_script_lua_version_t;

static inline sketch_script_lua_version_t sketch_script_lua_version_make(int major, int minor, int patch) {
    sketch_script_lua_version_t v;
    v.major = major;
    v.minor = minor;
    v.patch = patch;
    snprintf(v.text, sizeof(v.text), "Lua %d.%d.%d", major, minor, patch);
    v.text[sizeof(v.text) - 1] = '\0';
    return v;
}

static inline bool sketch_script_lua_parse_version_text(const char *text, sketch_script_lua_version_t *out) {
    if (!text || !out) return false;
    int major = 0, minor = 0, patch = 0;
    if (sscanf(text, "Lua %d.%d.%d", &major, &minor, &patch) != 3) return false;
    *out = sketch_script_lua_version_make(major, minor, patch);
    return true;
}

static inline bool sketch_script_lua_is_supported(sketch_script_lua_version_t v) {
    return v.major == SKETCH_SCRIPT_LUA_REQUIRED_MAJOR &&
           v.minor == SKETCH_SCRIPT_LUA_REQUIRED_MINOR;
}

static inline bool sketch_script_runtime_assert_lua54(sketch_script_lua_version_t v,
                                                      char *error_text,
                                                      size_t error_text_size) {
    if (sketch_script_lua_is_supported(v)) return true;
    if (error_text && error_text_size > 0) {
        snprintf(error_text, error_text_size,
                 "Unsupported Lua runtime: %s. Expected Lua %d.%d.x baseline.",
                 v.text,
                 SKETCH_SCRIPT_LUA_REQUIRED_MAJOR,
                 SKETCH_SCRIPT_LUA_REQUIRED_MINOR);
        error_text[error_text_size - 1] = '\0';
    }
    return false;
}

#endif // SKETCH_SCRIPT_RUNTIME_H
