//------------------------------------------------------------------------------
// ui_file_browser.h - Simple ImGui file browser (header-only)
//
// Cross-platform file browser widget for open/save dialogs.
//------------------------------------------------------------------------------
#ifndef UI_FILE_BROWSER_H
#define UI_FILE_BROWSER_H

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Platform-specific includes
#ifdef _WIN32
    #include <windows.h>
    #include <direct.h>
    #define getcwd _getcwd
    #define chdir _chdir
    #define PATH_SEP '\\'
    #define PATH_SEP_STR "\\"
#else
    #include <unistd.h>
    #include <dirent.h>
    #include <sys/stat.h>
    #define PATH_SEP '/'
    #define PATH_SEP_STR "/"
#endif

//------------------------------------------------------------------------------
// Configuration
//------------------------------------------------------------------------------

#define FILE_BROWSER_MAX_PATH 512
#define FILE_BROWSER_MAX_FILENAME 256
#define FILE_BROWSER_MAX_ENTRIES 1024
#define FILE_BROWSER_MAX_FILTER 64

//------------------------------------------------------------------------------
// Types
//------------------------------------------------------------------------------

typedef enum {
    FILE_BROWSER_MODE_OPEN,
    FILE_BROWSER_MODE_SAVE
} file_browser_mode_t;

typedef enum {
    FILE_ENTRY_DIRECTORY,
    FILE_ENTRY_FILE
} file_entry_type_t;

typedef struct {
    char name[FILE_BROWSER_MAX_FILENAME];
    file_entry_type_t type;
    size_t size;
} file_entry_t;

typedef struct {
    // Mode
    file_browser_mode_t mode;
    bool is_open;

    // Current path
    char current_path[FILE_BROWSER_MAX_PATH];
    char filename[FILE_BROWSER_MAX_FILENAME];

    // Filter (e.g., ".json")
    char filter[FILE_BROWSER_MAX_FILTER];

    // Directory contents
    file_entry_t *entries;
    int entry_count;
    int entry_capacity;

    // Selection
    int selected_index;

    // Result
    bool result_ready;
    char result_path[FILE_BROWSER_MAX_PATH];

    // Title for the modal
    char title[64];

    // Needs refresh
    bool needs_refresh;
} file_browser_t;

//------------------------------------------------------------------------------
// Internal: Path manipulation
//------------------------------------------------------------------------------

static inline void file_browser_normalize_path(char *path) {
    // Replace backslashes with forward slashes for consistency
    for (char *p = path; *p; p++) {
        if (*p == '\\') *p = '/';
    }
    // Remove trailing slash (except for root)
    size_t len = strlen(path);
    if (len > 1 && path[len - 1] == '/') {
        path[len - 1] = '\0';
    }
}

static inline void file_browser_get_parent(const char *path, char *parent) {
    strcpy(parent, path);
    file_browser_normalize_path(parent);

    char *last_sep = strrchr(parent, '/');
    if (last_sep) {
        if (last_sep == parent) {
            // Root directory
            parent[1] = '\0';
        } else {
            *last_sep = '\0';
        }
    }
}

static inline void file_browser_join_path(char *dest, const char *base, const char *name) {
    size_t base_len = strlen(base);
    if (base_len > 0 && base[base_len - 1] == '/') {
        snprintf(dest, FILE_BROWSER_MAX_PATH, "%s%s", base, name);
    } else {
        snprintf(dest, FILE_BROWSER_MAX_PATH, "%s/%s", base, name);
    }
    file_browser_normalize_path(dest);
}

static inline bool file_browser_matches_filter(const char *filename, const char *filter) {
    if (!filter || filter[0] == '\0') return true;

    const char *ext = strrchr(filename, '.');
    if (!ext) return false;

    return strcasecmp(ext, filter) == 0;
}

// Case-insensitive string comparison (cross-platform)
#ifdef _WIN32
#define strcasecmp _stricmp
#endif

//------------------------------------------------------------------------------
// Internal: Directory listing
//------------------------------------------------------------------------------

static inline int file_browser_compare_entries(const void *a, const void *b) {
    const file_entry_t *ea = (const file_entry_t *)a;
    const file_entry_t *eb = (const file_entry_t *)b;

    // Directories first
    if (ea->type != eb->type) {
        return ea->type == FILE_ENTRY_DIRECTORY ? -1 : 1;
    }

    // Then alphabetically (case-insensitive)
    return strcasecmp(ea->name, eb->name);
}

static inline void file_browser_refresh(file_browser_t *fb) {
    fb->entry_count = 0;
    fb->selected_index = -1;

    // Ensure we have capacity
    if (!fb->entries) {
        fb->entry_capacity = 256;
        fb->entries = (file_entry_t *)malloc(fb->entry_capacity * sizeof(file_entry_t));
    }

#ifdef _WIN32
    // Windows directory listing
    char search_path[FILE_BROWSER_MAX_PATH];
    snprintf(search_path, sizeof(search_path), "%s\\*", fb->current_path);

    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(search_path, &fd);
    if (h != INVALID_HANDLE_VALUE) {
        do {
            // Skip . and ..
            if (strcmp(fd.cFileName, ".") == 0) continue;

            if (fb->entry_count >= fb->entry_capacity) {
                fb->entry_capacity *= 2;
                fb->entries = (file_entry_t *)realloc(fb->entries,
                    fb->entry_capacity * sizeof(file_entry_t));
            }

            file_entry_t *entry = &fb->entries[fb->entry_count];
            strncpy(entry->name, fd.cFileName, FILE_BROWSER_MAX_FILENAME - 1);
            entry->name[FILE_BROWSER_MAX_FILENAME - 1] = '\0';

            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                entry->type = FILE_ENTRY_DIRECTORY;
                entry->size = 0;
                fb->entry_count++;
            } else {
                // Check filter for files
                if (fb->mode == FILE_BROWSER_MODE_SAVE ||
                    file_browser_matches_filter(entry->name, fb->filter)) {
                    entry->type = FILE_ENTRY_FILE;
                    entry->size = ((size_t)fd.nFileSizeHigh << 32) | fd.nFileSizeLow;
                    fb->entry_count++;
                }
            }
        } while (FindNextFileA(h, &fd));
        FindClose(h);
    }
#else
    // POSIX directory listing
    DIR *dir = opendir(fb->current_path);
    if (dir) {
        struct dirent *ent;
        while ((ent = readdir(dir)) != NULL) {
            // Skip .
            if (strcmp(ent->d_name, ".") == 0) continue;

            if (fb->entry_count >= fb->entry_capacity) {
                fb->entry_capacity *= 2;
                fb->entries = (file_entry_t *)realloc(fb->entries,
                    fb->entry_capacity * sizeof(file_entry_t));
            }

            file_entry_t *entry = &fb->entries[fb->entry_count];
            strncpy(entry->name, ent->d_name, FILE_BROWSER_MAX_FILENAME - 1);
            entry->name[FILE_BROWSER_MAX_FILENAME - 1] = '\0';

            // Get file info
            char full_path[FILE_BROWSER_MAX_PATH];
            file_browser_join_path(full_path, fb->current_path, ent->d_name);

            struct stat st;
            if (stat(full_path, &st) == 0) {
                if (S_ISDIR(st.st_mode)) {
                    entry->type = FILE_ENTRY_DIRECTORY;
                    entry->size = 0;
                    fb->entry_count++;
                } else if (S_ISREG(st.st_mode)) {
                    // Check filter for files
                    if (fb->mode == FILE_BROWSER_MODE_SAVE ||
                        file_browser_matches_filter(entry->name, fb->filter)) {
                        entry->type = FILE_ENTRY_FILE;
                        entry->size = st.st_size;
                        fb->entry_count++;
                    }
                }
            }
        }
        closedir(dir);
    }
#endif

    // Sort entries
    if (fb->entry_count > 0) {
        qsort(fb->entries, fb->entry_count, sizeof(file_entry_t), file_browser_compare_entries);
    }

    fb->needs_refresh = false;
}

//------------------------------------------------------------------------------
// Public API
//------------------------------------------------------------------------------

static inline void file_browser_init(file_browser_t *fb) {
    memset(fb, 0, sizeof(*fb));
    fb->selected_index = -1;
    fb->entry_capacity = 256;
    fb->entries = (file_entry_t *)malloc(fb->entry_capacity * sizeof(file_entry_t));
}

static inline void file_browser_shutdown(file_browser_t *fb) {
    if (fb->entries) {
        free(fb->entries);
        fb->entries = NULL;
    }
}

// Open the file browser for opening a file
static inline void file_browser_open_file(file_browser_t *fb, const char *title,
                                           const char *filter, const char *default_path) {
    fb->mode = FILE_BROWSER_MODE_OPEN;
    fb->is_open = true;
    fb->result_ready = false;
    fb->selected_index = -1;

    strncpy(fb->title, title, sizeof(fb->title) - 1);
    strncpy(fb->filter, filter ? filter : "", sizeof(fb->filter) - 1);
    fb->filename[0] = '\0';

    // Set initial path
    if (default_path && default_path[0]) {
        strncpy(fb->current_path, default_path, FILE_BROWSER_MAX_PATH - 1);
        // If default_path is a file, extract directory and filename
        char *last_sep = strrchr(fb->current_path, '/');
        if (!last_sep) last_sep = strrchr(fb->current_path, '\\');
        if (last_sep) {
            // Check if it's a file by looking for extension
            char *ext = strrchr(last_sep, '.');
            if (ext) {
                strncpy(fb->filename, last_sep + 1, FILE_BROWSER_MAX_FILENAME - 1);
                *last_sep = '\0';
            }
        }
    } else {
        getcwd(fb->current_path, FILE_BROWSER_MAX_PATH);
    }
    file_browser_normalize_path(fb->current_path);

    fb->needs_refresh = true;
}

// Open the file browser for saving a file
static inline void file_browser_save_file(file_browser_t *fb, const char *title,
                                           const char *filter, const char *default_name) {
    fb->mode = FILE_BROWSER_MODE_SAVE;
    fb->is_open = true;
    fb->result_ready = false;
    fb->selected_index = -1;

    strncpy(fb->title, title, sizeof(fb->title) - 1);
    strncpy(fb->filter, filter ? filter : "", sizeof(fb->filter) - 1);

    if (default_name && default_name[0]) {
        // Check if default_name is a full path
        if (strchr(default_name, '/') || strchr(default_name, '\\')) {
            strncpy(fb->current_path, default_name, FILE_BROWSER_MAX_PATH - 1);
            char *last_sep = strrchr(fb->current_path, '/');
            if (!last_sep) last_sep = strrchr(fb->current_path, '\\');
            if (last_sep) {
                strncpy(fb->filename, last_sep + 1, FILE_BROWSER_MAX_FILENAME - 1);
                *last_sep = '\0';
            }
        } else {
            strncpy(fb->filename, default_name, FILE_BROWSER_MAX_FILENAME - 1);
            getcwd(fb->current_path, FILE_BROWSER_MAX_PATH);
        }
    } else {
        fb->filename[0] = '\0';
        getcwd(fb->current_path, FILE_BROWSER_MAX_PATH);
    }
    file_browser_normalize_path(fb->current_path);

    fb->needs_refresh = true;
}

// Draw the file browser modal
// Returns true if a file was selected (check fb->result_path)
static inline bool file_browser_draw(file_browser_t *fb) {
    if (!fb->is_open) return false;

    bool result = false;
    bool should_close = false;

    // Refresh if needed
    if (fb->needs_refresh) {
        file_browser_refresh(fb);
    }

    // Open popup
    igOpenPopup_Str(fb->title, ImGuiPopupFlags_None);

    // Center the modal
    ImVec2 center;
    ImGuiViewport* viewport = igGetMainViewport();
    center.x = viewport->WorkPos.x + viewport->WorkSize.x * 0.5f;
    center.y = viewport->WorkPos.y + viewport->WorkSize.y * 0.5f;
    igSetNextWindowPos(center, ImGuiCond_Appearing, (ImVec2){0.5f, 0.5f});
    igSetNextWindowSize((ImVec2){600, 450}, ImGuiCond_Appearing);

    if (igBeginPopupModal(fb->title, &fb->is_open, ImGuiWindowFlags_None)) {
        // Current path display with edit capability
        igText("Location:");
        igSameLine(0, 5);
        igSetNextItemWidth(-1);
        if (igInputText("##path", fb->current_path, FILE_BROWSER_MAX_PATH,
                        ImGuiInputTextFlags_EnterReturnsTrue, NULL, NULL)) {
            fb->needs_refresh = true;
        }

        // Navigation buttons
        if (igButton("^ Up", (ImVec2){60, 0})) {
            char parent[FILE_BROWSER_MAX_PATH];
            file_browser_get_parent(fb->current_path, parent);
            if (strcmp(parent, fb->current_path) != 0) {
                strcpy(fb->current_path, parent);
                fb->needs_refresh = true;
            }
        }
        igSameLine(0, 5);
        if (igButton("Refresh", (ImVec2){60, 0})) {
            fb->needs_refresh = true;
        }
        igSameLine(0, 10);
        igText("Filter: %s", fb->filter[0] ? fb->filter : "*.*");

        igSeparator();

        // File list
        ImVec2 list_size = {-1, -70};  // Leave room for filename input and buttons
        if (igBeginChild_Str("##filelist", list_size, ImGuiChildFlags_Borders, ImGuiWindowFlags_None)) {
            for (int i = 0; i < fb->entry_count; i++) {
                file_entry_t *entry = &fb->entries[i];
                bool is_selected = (fb->selected_index == i);

                // Format display name
                char display[FILE_BROWSER_MAX_FILENAME + 10];
                if (entry->type == FILE_ENTRY_DIRECTORY) {
                    snprintf(display, sizeof(display), "[DIR] %s", entry->name);
                } else {
                    if (entry->size < 1024) {
                        snprintf(display, sizeof(display), "%s (%zu B)", entry->name, entry->size);
                    } else if (entry->size < 1024 * 1024) {
                        snprintf(display, sizeof(display), "%s (%.1f KB)", entry->name, entry->size / 1024.0);
                    } else {
                        snprintf(display, sizeof(display), "%s (%.1f MB)", entry->name, entry->size / (1024.0 * 1024.0));
                    }
                }

                if (igSelectable_Bool(display, is_selected, ImGuiSelectableFlags_AllowDoubleClick, (ImVec2){0, 0})) {
                    fb->selected_index = i;

                    if (entry->type == FILE_ENTRY_FILE) {
                        strcpy(fb->filename, entry->name);
                    }

                    // Double-click handling
                    if (igIsMouseDoubleClicked_Nil(ImGuiMouseButton_Left)) {
                        if (entry->type == FILE_ENTRY_DIRECTORY) {
                            // Navigate into directory
                            char new_path[FILE_BROWSER_MAX_PATH];
                            file_browser_join_path(new_path, fb->current_path, entry->name);
                            strcpy(fb->current_path, new_path);
                            fb->needs_refresh = true;
                        } else {
                            // Select file
                            strcpy(fb->filename, entry->name);
                            file_browser_join_path(fb->result_path, fb->current_path, fb->filename);
                            fb->result_ready = true;
                            result = true;
                            should_close = true;
                        }
                    }
                }
            }
        }
        igEndChild();

        // Filename input
        igText("Filename:");
        igSameLine(0, 5);
        igSetNextItemWidth(-120);
        bool enter_pressed = igInputText("##filename", fb->filename, FILE_BROWSER_MAX_FILENAME,
                                          ImGuiInputTextFlags_EnterReturnsTrue, NULL, NULL);

        igSameLine(0, 5);

        // OK button
        const char *ok_label = (fb->mode == FILE_BROWSER_MODE_SAVE) ? "Save" : "Open";
        bool can_confirm = fb->filename[0] != '\0';

        if (!can_confirm) {
            igBeginDisabled(true);
        }

        if (igButton(ok_label, (ImVec2){50, 0}) || (enter_pressed && can_confirm)) {
            // Add extension if missing (for save mode)
            if (fb->mode == FILE_BROWSER_MODE_SAVE && fb->filter[0]) {
                char *ext = strrchr(fb->filename, '.');
                if (!ext || strcasecmp(ext, fb->filter) != 0) {
                    // Append filter extension
                    size_t len = strlen(fb->filename);
                    size_t filter_len = strlen(fb->filter);
                    if (len + filter_len < FILE_BROWSER_MAX_FILENAME - 1) {
                        strcat(fb->filename, fb->filter);
                    }
                }
            }

            file_browser_join_path(fb->result_path, fb->current_path, fb->filename);
            fb->result_ready = true;
            result = true;
            should_close = true;
        }

        if (!can_confirm) {
            igEndDisabled();
        }

        igSameLine(0, 5);

        // Cancel button
        if (igButton("Cancel", (ImVec2){60, 0})) {
            should_close = true;
        }

        if (should_close) {
            igCloseCurrentPopup();
            fb->is_open = false;
        }

        igEndPopup();
    } else {
        // Popup was closed externally
        fb->is_open = false;
    }

    return result;
}

// Check if file browser has a result ready
static inline bool file_browser_has_result(file_browser_t *fb) {
    return fb->result_ready;
}

// Get the result path (only valid if has_result returns true)
static inline const char* file_browser_get_result(file_browser_t *fb) {
    return fb->result_path;
}

// Clear the result flag after handling
static inline void file_browser_clear_result(file_browser_t *fb) {
    fb->result_ready = false;
    fb->result_path[0] = '\0';
}

#endif // UI_FILE_BROWSER_H
