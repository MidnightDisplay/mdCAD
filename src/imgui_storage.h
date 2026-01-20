//------------------------------------------------------------------------------
// imgui_storage.h - ImGui settings persistence (header-only)
//------------------------------------------------------------------------------
#ifndef IMGUI_STORAGE_H
#define IMGUI_STORAGE_H

#include "platform.h"

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"

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
#ifdef PLATFORM_IOS
    char* ini_data = imgui_storage_ios_load();
    if (ini_data) {
        igLoadIniSettingsFromMemory(ini_data, 0);
        free(ini_data);
    }
    imgui_storage_ios_setup_handlers();
#elif defined(PLATFORM_WEB)
    char* ini_data = imgui_storage_load_js();
    if (ini_data) {
        igLoadIniSettingsFromMemory(ini_data, 0);
        free(ini_data);
    }
    imgui_storage_setup_handlers_js();
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
#elif defined(PLATFORM_WEB)
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
#ifdef PLATFORM_IOS
    const char* ini_data = igSaveIniSettingsToMemory(NULL);
    if (ini_data) {
        imgui_storage_ios_save(ini_data);
    }
    imgui_storage_ios_cleanup_handlers();
#elif defined(PLATFORM_WEB)
    const char* ini_data = igSaveIniSettingsToMemory(NULL);
    if (ini_data) {
        imgui_storage_save_js(ini_data);
    }
#endif
    // Native macOS/Linux/Windows: simgui_shutdown() handles saving automatically
}

#endif // IMGUI_STORAGE_H
