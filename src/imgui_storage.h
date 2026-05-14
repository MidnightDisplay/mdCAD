//------------------------------------------------------------------------------
// imgui_storage.h - ImGui settings persistence (header-only)
//------------------------------------------------------------------------------
#ifndef IMGUI_STORAGE_H
#define IMGUI_STORAGE_H

#include "platform.h"
#include "embed_layout_state.h"

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//------------------------------------------------------------------------------
// iOS NSUserDefaults persistence
//------------------------------------------------------------------------------
#ifdef PLATFORM_IOS
#import <UIKit/UIKit.h>

static bool g_imgui_ios_should_save = false;
static id g_imgui_ios_observer = nil;

static inline char* imgui_storage_ios_load(void) {
    @autoreleasepool {
        NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
        NSString *ini = [defaults stringForKey:@"imgui_ini"];
        if (!ini || ini.length == 0) return NULL;

        const char *utf8 = [ini UTF8String];
        size_t len = strlen(utf8) + 1;
        char *buf = (char*)malloc(len);
        memcpy(buf, utf8, len);
        return buf;
    }
}

static inline void imgui_storage_ios_save(const char* ini_data) {
    @autoreleasepool {
        NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
        NSString *ini = [NSString stringWithUTF8String:ini_data];
        [defaults setObject:ini forKey:@"imgui_ini"];
        [defaults synchronize];
    }
}

static inline void imgui_storage_ios_mark_should_save(void) {
    g_imgui_ios_should_save = true;
}

static inline void imgui_storage_ios_setup_handlers(void) {
    @autoreleasepool {
        g_imgui_ios_observer = [[NSNotificationCenter defaultCenter]
            addObserverForName:UIApplicationWillResignActiveNotification
            object:nil
            queue:[NSOperationQueue mainQueue]
            usingBlock:^(NSNotification *note) {
                imgui_storage_ios_mark_should_save();
            }];
    }
}

static inline void imgui_storage_ios_cleanup_handlers(void) {
    @autoreleasepool {
        if (g_imgui_ios_observer) {
            [[NSNotificationCenter defaultCenter] removeObserver:g_imgui_ios_observer];
            g_imgui_ios_observer = nil;
        }
    }
}

#endif // PLATFORM_IOS

//------------------------------------------------------------------------------
// Android internal storage persistence
//------------------------------------------------------------------------------
#ifdef PLATFORM_ANDROID
#include "sokol_app.h"
#include <android/native_activity.h>
#include <android/log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IMGUI_STORAGE_LOG(...) __android_log_print(ANDROID_LOG_INFO, "imgui_storage", __VA_ARGS__)

static bool g_imgui_android_should_save = false;
static char g_imgui_android_ini_path[512] = {0};

static inline void imgui_storage_android_init_path(void) {
    const ANativeActivity* activity = (const ANativeActivity*)sapp_android_get_native_activity();
    IMGUI_STORAGE_LOG("init_path: activity=%p", (void*)activity);
    if (activity) {
        IMGUI_STORAGE_LOG("init_path: internalDataPath=%s", activity->internalDataPath ? activity->internalDataPath : "NULL");
        if (activity->internalDataPath) {
            snprintf(g_imgui_android_ini_path, sizeof(g_imgui_android_ini_path),
                     "%s/imgui.ini", activity->internalDataPath);
            IMGUI_STORAGE_LOG("init_path: ini_path=%s", g_imgui_android_ini_path);
        }
    }
}

static inline char* imgui_storage_android_load(void) {
    IMGUI_STORAGE_LOG("load: starting");
    if (g_imgui_android_ini_path[0] == '\0') {
        imgui_storage_android_init_path();
    }
    if (g_imgui_android_ini_path[0] == '\0') {
        IMGUI_STORAGE_LOG("load: path is empty, returning NULL");
        return NULL;
    }

    IMGUI_STORAGE_LOG("load: opening %s", g_imgui_android_ini_path);
    FILE* f = fopen(g_imgui_android_ini_path, "rb");
    if (!f) {
        IMGUI_STORAGE_LOG("load: failed to open file");
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (size <= 0) {
        fclose(f);
        return NULL;
    }

    char* buf = (char*)malloc(size + 1);
    if (!buf) {
        fclose(f);
        return NULL;
    }

    size_t read = fread(buf, 1, size, f);
    fclose(f);

    buf[read] = '\0';
    return buf;
}

static inline void imgui_storage_android_save(const char* ini_data) {
    IMGUI_STORAGE_LOG("save: starting");
    if (g_imgui_android_ini_path[0] == '\0') {
        imgui_storage_android_init_path();
    }
    if (g_imgui_android_ini_path[0] == '\0') {
        IMGUI_STORAGE_LOG("save: path is empty, returning");
        return;
    }

    IMGUI_STORAGE_LOG("save: opening %s for writing", g_imgui_android_ini_path);
    FILE* f = fopen(g_imgui_android_ini_path, "wb");
    if (!f) {
        IMGUI_STORAGE_LOG("save: failed to open file for writing");
        return;
    }

    size_t len = strlen(ini_data);
    size_t written = fwrite(ini_data, 1, len, f);
    fclose(f);
    IMGUI_STORAGE_LOG("save: wrote %zu of %zu bytes", written, len);
}

static inline void imgui_storage_android_mark_should_save(void) {
    g_imgui_android_should_save = true;
}

#endif // PLATFORM_ANDROID

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
// Native desktop file persistence
//------------------------------------------------------------------------------
#if !defined(PLATFORM_IOS) && !defined(PLATFORM_ANDROID) && !defined(PLATFORM_WEB)
#if defined(_WIN32)
#include <direct.h>
#endif
#if !defined(_WIN32)
#include <unistd.h>
#endif

static embed_layout_policy_t g_imgui_native_layout_policy = {0};
static bool g_imgui_native_should_save = false;
static char g_imgui_native_manual_store_path[1024] = {0};

static inline bool imgui_storage_native_path_is_absolute(const char* path) {
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

static inline void imgui_storage_native_resolve_store_path(const char* path, char* buffer, size_t buffer_size) {
    if (!buffer || buffer_size == 0u) {
        return;
    }

    buffer[0] = '\0';
    if (!path || path[0] == '\0') {
        return;
    }

    if (imgui_storage_native_path_is_absolute(path)) {
        snprintf(buffer, buffer_size, "%s", path);
        return;
    }

#if defined(_WIN32)
    if (_fullpath(buffer, path, buffer_size) != NULL) {
        return;
    }
#else
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        snprintf(buffer, buffer_size, "%s/%s", cwd, path);
        return;
    }
#endif

    snprintf(buffer, buffer_size, "%s", path);
}

static inline const char* imgui_storage_native_manual_store_path(void) {
    return g_imgui_native_layout_policy.use_manual_persistence
        ? g_imgui_native_layout_policy.manual_store_filename
        : NULL;
}

static inline void imgui_storage_configure(const embed_layout_policy_t* policy) {
    g_imgui_native_layout_policy = policy ? *policy : (embed_layout_policy_t){0};
    g_imgui_native_should_save = false;
    g_imgui_native_manual_store_path[0] = '\0';
    if (g_imgui_native_layout_policy.use_manual_persistence) {
        imgui_storage_native_resolve_store_path(
            g_imgui_native_layout_policy.manual_store_filename,
            g_imgui_native_manual_store_path,
            sizeof(g_imgui_native_manual_store_path));
        g_imgui_native_layout_policy.manual_store_filename = g_imgui_native_manual_store_path;
    }
}

static inline bool imgui_storage_native_store_exists(const char* path) {
    char resolved_path[1024];
    imgui_storage_native_resolve_store_path(path, resolved_path, sizeof(resolved_path));
    if (resolved_path[0] == '\0') {
        return false;
    }

    FILE* f = fopen(resolved_path, "rb");
    if (!f) {
        return false;
    }

    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return false;
    }

    long size = ftell(f);
    fclose(f);
    return size > 0;
}

static inline char* imgui_storage_native_load(const char* path) {
    char resolved_path[1024];
    imgui_storage_native_resolve_store_path(path, resolved_path, sizeof(resolved_path));
    if (!imgui_storage_native_store_exists(resolved_path)) {
        return NULL;
    }

    FILE* f = fopen(resolved_path, "rb");
    if (!f) {
        return NULL;
    }

    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return NULL;
    }

    long size = ftell(f);
    if (size <= 0 || fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        return NULL;
    }

    char* buf = (char*)malloc((size_t)size + 1u);
    if (!buf) {
        fclose(f);
        return NULL;
    }

    size_t read = fread(buf, 1, (size_t)size, f);
    fclose(f);
    buf[read] = '\0';
    return buf;
}

static inline void imgui_storage_native_save(const char* path, const char* ini_data) {
    char resolved_path[1024];
    imgui_storage_native_resolve_store_path(path, resolved_path, sizeof(resolved_path));
    if (resolved_path[0] == '\0' || !ini_data) {
        return;
    }

    FILE* f = fopen(resolved_path, "wb");
    if (!f) {
        return;
    }

    size_t len = strlen(ini_data);
    (void)fwrite(ini_data, 1, len, f);
    fclose(f);
}
#else
static inline void imgui_storage_configure(const embed_layout_policy_t* policy) {
    (void)policy;
}

static inline bool imgui_storage_native_store_exists(const char* path) {
    (void)path;
    return false;
}

static inline const char* imgui_storage_native_manual_store_path(void) {
    return NULL;
}
#endif

//------------------------------------------------------------------------------
// Unified API
//------------------------------------------------------------------------------

// Initialize ImGui settings persistence
// Call after simgui_setup() and setting ConfigFlags
static inline void imgui_storage_init(void) {
#ifdef PLATFORM_IOS
    char* ini_data = imgui_storage_ios_load();
    if (ini_data) {
        igLoadIniSettingsFromMemory(ini_data, 0);
        free(ini_data);
    }
    imgui_storage_ios_setup_handlers();
#elif defined(PLATFORM_ANDROID)
    char* ini_data = imgui_storage_android_load();
    if (ini_data) {
        igLoadIniSettingsFromMemory(ini_data, 0);
        free(ini_data);
    }
#elif defined(PLATFORM_WEB)
    char* ini_data = imgui_storage_load_js();
    if (ini_data) {
        igLoadIniSettingsFromMemory(ini_data, 0);
        free(ini_data);
    }
    imgui_storage_setup_handlers_js();
#elif !defined(PLATFORM_IOS) && !defined(PLATFORM_ANDROID) && !defined(PLATFORM_WEB)
    if (g_imgui_native_layout_policy.use_manual_persistence) {
        char* ini_data = imgui_storage_native_load(g_imgui_native_layout_policy.manual_store_filename);
        if (ini_data) {
            igLoadIniSettingsFromMemory(ini_data, 0);
            free(ini_data);
        }
    }
#endif
    // Native macOS/Linux/Windows: simgui_setup() handles ini_filename automatically
}

// Call once per frame (early in frame)
// Handles deferred saves triggered by visibility change / app backgrounding
static inline void imgui_storage_frame(void) {
#ifdef PLATFORM_IOS
    if (g_imgui_ios_should_save) {
        g_imgui_ios_should_save = false;
        const char* ini_data = igSaveIniSettingsToMemory(NULL);
        if (ini_data) {
            imgui_storage_ios_save(ini_data);
        }
    }
#elif defined(PLATFORM_ANDROID)
    if (g_imgui_android_should_save) {
        g_imgui_android_should_save = false;
        const char* ini_data = igSaveIniSettingsToMemory(NULL);
        if (ini_data) {
            imgui_storage_android_save(ini_data);
        }
    }
#elif defined(PLATFORM_WEB)
    if (g_imgui_should_save) {
        g_imgui_should_save = false;
        const char* ini_data = igSaveIniSettingsToMemory(NULL);
        if (ini_data) {
            imgui_storage_save_js(ini_data);
        }
    }
#elif !defined(PLATFORM_IOS) && !defined(PLATFORM_ANDROID) && !defined(PLATFORM_WEB)
    if (g_imgui_native_layout_policy.use_manual_persistence && g_imgui_native_should_save) {
        g_imgui_native_should_save = false;
        const char* ini_data = igSaveIniSettingsToMemory(NULL);
        if (ini_data) {
            imgui_storage_native_save(g_imgui_native_layout_policy.manual_store_filename, ini_data);
        }
    }
#endif
}

// Save settings before shutdown
// Call before simgui_shutdown()
static inline void imgui_storage_shutdown(void) {
#ifdef PLATFORM_IOS
    const char* ini_data = igSaveIniSettingsToMemory(NULL);
    if (ini_data) {
        imgui_storage_ios_save(ini_data);
    }
    imgui_storage_ios_cleanup_handlers();
#elif defined(PLATFORM_ANDROID)
    const char* ini_data = igSaveIniSettingsToMemory(NULL);
    if (ini_data) {
        imgui_storage_android_save(ini_data);
    }
#elif defined(PLATFORM_WEB)
    const char* ini_data = igSaveIniSettingsToMemory(NULL);
    if (ini_data) {
        imgui_storage_save_js(ini_data);
    }
#elif !defined(PLATFORM_IOS) && !defined(PLATFORM_ANDROID) && !defined(PLATFORM_WEB)
    if (g_imgui_native_layout_policy.use_manual_persistence) {
        const char* ini_data = igSaveIniSettingsToMemory(NULL);
        if (ini_data) {
            imgui_storage_native_save(g_imgui_native_layout_policy.manual_store_filename, ini_data);
        }
    }
#endif
    // Native macOS/Linux/Windows: simgui_shutdown() handles saving automatically
}

// Mark that settings should be saved on next frame
// Call when app is about to be suspended/backgrounded
// Note: PLATFORM_WEB version is defined above with EMSCRIPTEN_KEEPALIVE
#ifndef PLATFORM_WEB
static inline void imgui_storage_mark_should_save(void) {
#ifdef PLATFORM_IOS
    imgui_storage_ios_mark_should_save();
#elif defined(PLATFORM_ANDROID)
    imgui_storage_android_mark_should_save();
#elif !defined(PLATFORM_WEB)
    if (g_imgui_native_layout_policy.use_manual_persistence) {
        g_imgui_native_should_save = true;
    }
#endif
    // Native platforms save automatically via ini file
}
#endif

#endif // IMGUI_STORAGE_H
