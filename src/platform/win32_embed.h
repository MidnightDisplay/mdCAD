#ifndef MDCAD_WIN32_EMBED_H
#define MDCAD_WIN32_EMBED_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

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
} mdcad_win32_embed_state_t;

extern mdcad_win32_embed_state_t g_mdcad_win32_embed_state;

static inline void mdcad_win32_embed_set_config(const mdcad_launch_config_t* cfg) {
    g_mdcad_win32_embed_state.enabled = (cfg != NULL) && cfg->embedded;
    g_mdcad_win32_embed_state.parent_hwnd = g_mdcad_win32_embed_state.enabled
        ? (HWND)cfg->parent_hwnd_value
        : NULL;
    g_mdcad_win32_embed_state.runtime_cancel_generation = 0u;
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
    const char* message = (reason && reason[0] != '\0')
        ? reason
        : "Embedded startup failed: unknown child-window startup error";
    fprintf(stderr, "%s\n", message);
    fflush(stderr);
    exit(2);
}

#else

typedef void* HWND;

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
