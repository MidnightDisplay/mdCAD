//------------------------------------------------------------------------------
// imgui_storage.h - ImGui settings persistence (header-only)
//------------------------------------------------------------------------------
#ifndef IMGUI_STORAGE_H
#define IMGUI_STORAGE_H

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"

//------------------------------------------------------------------------------
// Web localStorage persistence
//------------------------------------------------------------------------------
#ifdef PLATFORM_WEB
#include <emscripten.h>
#include <stdlib.h>

static bool g_imgui_should_save = false;

EM_JS(char*, imgui_storage_load_js, (), {
    var ini = localStorage.getItem('imgui_ini');
    if (!ini) return 0;
    var len = lengthBytesUTF8(ini) + 1;
    var buf = _malloc(len);
    stringToUTF8(ini, buf, len);
    return buf;
});

EM_JS(void, imgui_storage_save_js, (const char* ini_data), {
    var ini = UTF8ToString(ini_data);
    localStorage.setItem('imgui_ini', ini);
});

EM_JS(void, imgui_storage_setup_handlers_js, (), {
    document.addEventListener('visibilitychange', function() {
        if (document.hidden) {
            Module._imgui_storage_mark_should_save();
        }
    });
});

EMSCRIPTEN_KEEPALIVE void imgui_storage_mark_should_save(void) {
    g_imgui_should_save = true;
}

#endif // PLATFORM_WEB

//------------------------------------------------------------------------------
// Unified API
//------------------------------------------------------------------------------

// Initialize ImGui settings persistence
// Call after simgui_setup() and setting ConfigFlags
static inline void imgui_storage_init(void) {
#ifdef PLATFORM_WEB
    char* ini_data = imgui_storage_load_js();
    if (ini_data) {
        igLoadIniSettingsFromMemory(ini_data, 0);
        free(ini_data);
    }
    imgui_storage_setup_handlers_js();
#endif
    // Native: simgui_setup() handles ini_filename automatically
}

// Call once per frame (early in frame)
// Handles deferred saves triggered by visibility change
static inline void imgui_storage_frame(void) {
#ifdef PLATFORM_WEB
    if (g_imgui_should_save) {
        g_imgui_should_save = false;
        const char* ini_data = igSaveIniSettingsToMemory(NULL);
        if (ini_data) {
            imgui_storage_save_js(ini_data);
        }
    }
#endif
}

// Save settings before shutdown
// Call before simgui_shutdown()
static inline void imgui_storage_shutdown(void) {
#ifdef PLATFORM_WEB
    const char* ini_data = igSaveIniSettingsToMemory(NULL);
    if (ini_data) {
        imgui_storage_save_js(ini_data);
    }
#endif
    // Native: simgui_shutdown() handles saving automatically
}

#endif // IMGUI_STORAGE_H
