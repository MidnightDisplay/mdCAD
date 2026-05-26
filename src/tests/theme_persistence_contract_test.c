#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "imgui_storage.h"

static int with_fresh_context(int (*test_fn)(void)) {
    ImGuiContext* ctx = igCreateContext(NULL);
    if (!ctx) {
        return 1;
    }

    igSetCurrentContext(ctx);
    igGetIO_Nil()->IniSavingRate = 1.0f;
    imgui_storage_configure(NULL);
    ui_theme_apply(UI_THEME_DEFAULT);
    imgui_storage_init();

    int result = test_fn();

    igDestroyContext(ctx);
    return result;
}

static int test_handler_registration_and_restore_override(void) {
    if (igFindSettingsHandler("mdCAD") == NULL) {
        return 1;
    }
    if (ui_theme_get_current() != UI_THEME_DEFAULT) {
        return 1;
    }

    const char* ini_data =
        "[mdCAD][Theme]\n"
        "Selected=tokyo-night\n";
    igLoadIniSettingsFromMemory(ini_data, 0);

    return ui_theme_get_current() == UI_THEME_TOKYO_NIGHT ? 0 : 1;
}

static int test_missing_or_invalid_theme_keeps_default(void) {
    igLoadIniSettingsFromMemory("[Window][Controls]\nPos=1,2\n", 0);
    if (ui_theme_get_current() != UI_THEME_DEFAULT) {
        return 1;
    }

    const char* invalid_ini =
        "[mdCAD][Theme]\n"
        "Selected=totally-unknown-theme\n";
    igLoadIniSettingsFromMemory(invalid_ini, 0);

    return ui_theme_get_current() == UI_THEME_DEFAULT ? 0 : 1;
}

static int test_manual_edit_tokens_roundtrip_in_ini_stream(void) {
    const char* ini_data =
        "[mdCAD][Theme]\n"
        "Selected=\tcyberpunk-2077  \n";
    igLoadIniSettingsFromMemory(ini_data, 0);
    if (ui_theme_get_current() != UI_THEME_CYBERPUNK_2077) {
        return 1;
    }

    const char* saved = igSaveIniSettingsToMemory(NULL);
    if (!saved) {
        return 1;
    }

    return strstr(saved, "[mdCAD][Theme]\nSelected=cyberpunk-2077\n") != NULL ? 0 : 1;
}

static int test_mark_settings_dirty_schedules_save(void) {
    static const char* paths[] = {
        "src/ui/ui_controls.h",
        "../src/ui/ui_controls.h",
        "../../src/ui/ui_controls.h",
        "../../../src/ui/ui_controls.h",
        "../../../../src/ui/ui_controls.h"
    };

    for (size_t i = 0; i < (sizeof(paths) / sizeof(paths[0])); ++i) {
        FILE* f = fopen(paths[i], "rb");
        if (!f) {
            continue;
        }

        if (fseek(f, 0, SEEK_END) != 0) {
            fclose(f);
            continue;
        }

        long size = ftell(f);
        if (size < 0 || fseek(f, 0, SEEK_SET) != 0) {
            fclose(f);
            continue;
        }

        char* src = (char*)malloc((size_t)size + 1u);
        if (!src) {
            fclose(f);
            return 1;
        }

        size_t read = fread(src, 1, (size_t)size, f);
        fclose(f);
        if (read != (size_t)size) {
            free(src);
            continue;
        }
        src[size] = '\0';

        const char* apply = strstr(src, "ui_theme_apply((ui_theme_t)current_theme);");
        const char* dirty = strstr(src, "ui_theme_mark_settings_dirty();");
        int result = (apply && dirty && apply < dirty) ? 0 : 1;
        free(src);
        return result;
    }

    return 1;
}

typedef int (*theme_test_fn_t)(void);
typedef struct {
    const char* name;
    theme_test_fn_t fn;
} theme_test_case_t;

int main(void) {
    static const theme_test_case_t tests[] = {
        { "test_handler_registration_and_restore_override", test_handler_registration_and_restore_override },
        { "test_missing_or_invalid_theme_keeps_default", test_missing_or_invalid_theme_keeps_default },
        { "test_manual_edit_tokens_roundtrip_in_ini_stream", test_manual_edit_tokens_roundtrip_in_ini_stream },
        { "test_mark_settings_dirty_schedules_save", test_mark_settings_dirty_schedules_save },
    };

    for (size_t i = 0; i < (sizeof(tests) / sizeof(tests[0])); ++i) {
        if (with_fresh_context(tests[i].fn) != 0) {
            fprintf(stderr, "FAILED: %s\n", tests[i].name);
            return 1;
        }
    }

    return 0;
}
