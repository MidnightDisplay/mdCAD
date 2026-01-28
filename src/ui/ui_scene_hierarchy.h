//------------------------------------------------------------------------------
// ui_scene_hierarchy.h - Scene hierarchy panel (header-only)
//
// Lists all ECS entities and allows selection via click.
// Features:
// - Cached sorted entity list (only re-sorts when entities added/removed)
// - Search/filter by type name or entity ID
// - Pagination for large entity counts
// - Entity creation menu
//------------------------------------------------------------------------------
#ifndef UI_SCENE_HIERARCHY_H
#define UI_SCENE_HIERARCHY_H

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"
#include "../selection.h"
#include "../ecs/ecs_scene.h"

#include <stdlib.h>  // for rand(), qsort(), malloc(), realloc(), free()
#include <string.h>  // for strstr(), strlen()
#include <ctype.h>   // for tolower()

//------------------------------------------------------------------------------
// Configuration
//------------------------------------------------------------------------------

#define UI_HIERARCHY_ITEMS_PER_PAGE 100
#define UI_HIERARCHY_FILTER_MAX_LEN 64

//------------------------------------------------------------------------------
// Entity cache entry
//------------------------------------------------------------------------------

typedef struct {
    ecs_entity_t entity;
    geometry_type_t type;
} ui_hierarchy_entry_t;

//------------------------------------------------------------------------------
// Types
//------------------------------------------------------------------------------

typedef struct {
    selection_buffer_t *selection;
    ecs_scene_t *scene;

    // Cached sorted entity list
    ui_hierarchy_entry_t *cache;
    int cache_count;
    int cache_capacity;
    bool cache_dirty;

    // Filter state
    char filter_text[UI_HIERARCHY_FILTER_MAX_LEN];
    bool filter_active;

    // Pagination state
    int current_page;
    int items_per_page;
} ui_scene_hierarchy_state_t;

//------------------------------------------------------------------------------
// Initialization / Shutdown
//------------------------------------------------------------------------------

static inline void ui_scene_hierarchy_init(ui_scene_hierarchy_state_t *state,
                                            selection_buffer_t *selection,
                                            ecs_scene_t *scene) {
    state->selection = selection;
    state->scene = scene;

    // Initialize cache
    state->cache = NULL;
    state->cache_count = 0;
    state->cache_capacity = 0;
    state->cache_dirty = true;  // Force initial build

    // Initialize filter
    state->filter_text[0] = '\0';
    state->filter_active = false;

    // Initialize pagination
    state->current_page = 0;
    state->items_per_page = UI_HIERARCHY_ITEMS_PER_PAGE;
}

static inline void ui_scene_hierarchy_shutdown(ui_scene_hierarchy_state_t *state) {
    if (state->cache) {
        free(state->cache);
        state->cache = NULL;
    }
    state->cache_count = 0;
    state->cache_capacity = 0;
}

//------------------------------------------------------------------------------
// Cache invalidation - call when entities are added/removed
//------------------------------------------------------------------------------

static inline void ui_scene_hierarchy_mark_dirty(ui_scene_hierarchy_state_t *state) {
    state->cache_dirty = true;
}

//------------------------------------------------------------------------------
// Internal: Comparison function for qsort (sort by entity ID ascending)
//------------------------------------------------------------------------------

static inline int ui_hierarchy_compare_entries(const void *a, const void *b) {
    const ui_hierarchy_entry_t *ea = (const ui_hierarchy_entry_t*)a;
    const ui_hierarchy_entry_t *eb = (const ui_hierarchy_entry_t*)b;
    if (ea->entity < eb->entity) return -1;
    if (ea->entity > eb->entity) return 1;
    return 0;
}

//------------------------------------------------------------------------------
// Internal: Case-insensitive substring search
//------------------------------------------------------------------------------

static inline bool ui_hierarchy_str_contains_ci(const char *haystack, const char *needle) {
    if (!needle[0]) return true;  // Empty filter matches everything

    size_t needle_len = strlen(needle);
    size_t haystack_len = strlen(haystack);

    if (needle_len > haystack_len) return false;

    for (size_t i = 0; i <= haystack_len - needle_len; i++) {
        bool match = true;
        for (size_t j = 0; j < needle_len; j++) {
            if (tolower((unsigned char)haystack[i + j]) != tolower((unsigned char)needle[j])) {
                match = false;
                break;
            }
        }
        if (match) return true;
    }
    return false;
}

//------------------------------------------------------------------------------
// Internal: Check if entry matches filter
//------------------------------------------------------------------------------

static inline bool ui_hierarchy_entry_matches_filter(ui_hierarchy_entry_t *entry,
                                                      const char *filter) {
    if (!filter[0]) return true;  // Empty filter matches all

    // Check if filter matches type name
    const char *type_name = geometry_type_name(entry->type);
    if (ui_hierarchy_str_contains_ci(type_name, filter)) {
        return true;
    }

    // Check if filter matches entity ID (with or without # prefix)
    char id_str[32];
    snprintf(id_str, sizeof(id_str), "%llu", (unsigned long long)entry->entity);
    if (ui_hierarchy_str_contains_ci(id_str, filter)) {
        return true;
    }

    // Also try with # prefix stripped from filter
    if (filter[0] == '#' && filter[1]) {
        if (ui_hierarchy_str_contains_ci(id_str, filter + 1)) {
            return true;
        }
    }

    return false;
}

//------------------------------------------------------------------------------
// Internal: Rebuild cache from ECS world
//------------------------------------------------------------------------------

static inline void ui_scene_hierarchy_rebuild_cache(ui_scene_hierarchy_state_t *state) {
    ecs_world_state_t *w = state->scene->world;

    // First pass: count entities
    int total_count = 0;
    ecs_query_t *q = ecs_query(w->world, {
        .terms = {
            { .id = w->GeometryComp_id }
        }
    });

    ecs_iter_t it = ecs_query_iter(w->world, q);
    while (ecs_query_next(&it)) {
        total_count += it.count;
    }
    ecs_query_fini(q);

    // Ensure cache has enough capacity
    if (total_count > state->cache_capacity) {
        int new_capacity = state->cache_capacity ? state->cache_capacity * 2 : 64;
        while (new_capacity < total_count) {
            new_capacity *= 2;
        }
        state->cache = (ui_hierarchy_entry_t*)realloc(state->cache,
                                                       new_capacity * sizeof(ui_hierarchy_entry_t));
        state->cache_capacity = new_capacity;
    }

    // Second pass: collect entities
    state->cache_count = 0;
    q = ecs_query(w->world, {
        .terms = {
            { .id = w->GeometryComp_id }
        }
    });

    it = ecs_query_iter(w->world, q);
    while (ecs_query_next(&it)) {
        GeometryComp *geoms = ecs_field(&it, GeometryComp, 0);

        for (int i = 0; i < it.count; i++) {
            state->cache[state->cache_count].entity = it.entities[i];
            state->cache[state->cache_count].type = geoms[i].type;
            state->cache_count++;
        }
    }
    ecs_query_fini(q);

    // Sort by entity ID
    if (state->cache_count > 1) {
        qsort(state->cache, state->cache_count, sizeof(ui_hierarchy_entry_t),
              ui_hierarchy_compare_entries);
    }

    state->cache_dirty = false;
}

//------------------------------------------------------------------------------
// Internal: Random color generation
//------------------------------------------------------------------------------

static inline vec4_t ui_scene_hierarchy_random_color(void) {
    // Generate vibrant random colors using HSV->RGB
    float h = (float)(rand() % 360) / 360.0f;
    float s = 0.6f + (float)(rand() % 40) / 100.0f;  // 0.6-1.0
    float v = 0.8f + (float)(rand() % 20) / 100.0f;  // 0.8-1.0

    // Simple HSV to RGB conversion
    float c = v * s;
    float x = c * (1.0f - fabsf(fmodf(h * 6.0f, 2.0f) - 1.0f));
    float m = v - c;

    float r, g, b;
    int sector = (int)(h * 6.0f);
    switch (sector % 6) {
        case 0: r = c; g = x; b = 0; break;
        case 1: r = x; g = c; b = 0; break;
        case 2: r = 0; g = c; b = x; break;
        case 3: r = 0; g = x; b = c; break;
        case 4: r = x; g = 0; b = c; break;
        default: r = c; g = 0; b = x; break;
    }

    return vec4_make(r + m, g + m, b + m, 1.0f);
}

//------------------------------------------------------------------------------
// Internal: Add Entity Menu
//------------------------------------------------------------------------------

static inline void ui_scene_hierarchy_draw_add_menu(ui_scene_hierarchy_state_t *state) {
    if (igBeginMenu("Add Entity", true)) {
        if (igMenuItem_Bool("Point", NULL, false, true)) {
            vec4_t color = ui_scene_hierarchy_random_color();
            scene_add_point(state->scene,
                vec3_make(0.0f, 0.0f, 0.0f),
                color, 0.06f);
            state->cache_dirty = true;
        }

        if (igMenuItem_Bool("Line (X axis)", NULL, false, true)) {
            vec4_t color = ui_scene_hierarchy_random_color();
            scene_add_line(state->scene,
                vec3_make(-1.0f, 0.0f, 0.0f),
                vec3_make(1.0f, 0.0f, 0.0f),
                color, 0.03f);
            state->cache_dirty = true;
        }

        if (igMenuItem_Bool("Line (Y axis)", NULL, false, true)) {
            vec4_t color = ui_scene_hierarchy_random_color();
            scene_add_line(state->scene,
                vec3_make(0.0f, -1.0f, 0.0f),
                vec3_make(0.0f, 1.0f, 0.0f),
                color, 0.03f);
            state->cache_dirty = true;
        }

        if (igMenuItem_Bool("Line (Z axis)", NULL, false, true)) {
            vec4_t color = ui_scene_hierarchy_random_color();
            scene_add_line(state->scene,
                vec3_make(0.0f, 0.0f, -1.0f),
                vec3_make(0.0f, 0.0f, 1.0f),
                color, 0.03f);
            state->cache_dirty = true;
        }

        if (igMenuItem_Bool("Line (Random)", NULL, false, true)) {
            vec4_t color = ui_scene_hierarchy_random_color();
            vec3_t a = vec3_make(
                ((float)(rand() % 200) - 100.0f) / 50.0f,
                ((float)(rand() % 200) - 100.0f) / 50.0f,
                ((float)(rand() % 200) - 100.0f) / 50.0f
            );
            vec3_t b = vec3_make(
                ((float)(rand() % 200) - 100.0f) / 50.0f,
                ((float)(rand() % 200) - 100.0f) / 50.0f,
                ((float)(rand() % 200) - 100.0f) / 50.0f
            );
            scene_add_line(state->scene, a, b, color, 0.03f);
            state->cache_dirty = true;
        }

        igEndMenu();
    }
}

//------------------------------------------------------------------------------
// Internal: Draw single entity item
//------------------------------------------------------------------------------

static inline bool ui_scene_hierarchy_draw_entity(ui_scene_hierarchy_state_t *state,
                                                   ecs_entity_t e,
                                                   geometry_type_t type,
                                                   bool ctrl_held,
                                                   bool shift_held) {
    selection_buffer_t *sel = state->selection;
    bool entity_deleted = false;

    // Skip if entity was deleted
    if (!ecs_is_alive(state->scene->world->world, e)) {
        return true;  // Mark for cache refresh
    }

    bool is_selected = selection_contains(sel, e);

    // Create label: Type #EntityID
    char label[64];
    snprintf(label, sizeof(label), "%s #%llu",
             geometry_type_name(type),
             (unsigned long long)e);

    // Selectable item
    if (igSelectable_Bool(label, is_selected, ImGuiSelectableFlags_None, (ImVec2){0, 0})) {
        if (ctrl_held) {
            selection_toggle(sel, e);
        } else if (shift_held) {
            selection_add(sel, e);
        } else {
            selection_set_single(sel, e);
        }
    }

    // Right-click context menu
    if (igBeginPopupContextItem(NULL, ImGuiPopupFlags_MouseButtonRight)) {
        if (igMenuItem_Bool("Select", NULL, false, true)) {
            selection_set_single(sel, e);
        }
        if (igMenuItem_Bool("Add to Selection", NULL, false, true)) {
            selection_add(sel, e);
        }
        igSeparator();
        if (igMenuItem_Bool("Delete", "Del", false, true)) {
            selection_remove(sel, e);
            scene_remove_entity(state->scene, e);
            entity_deleted = true;
        }
        igEndPopup();
    }

    return entity_deleted;
}

//------------------------------------------------------------------------------
// Main Draw Function
//------------------------------------------------------------------------------

static inline void ui_scene_hierarchy_draw(ui_scene_hierarchy_state_t *state) {
    if (!igBegin("Scene Hierarchy", NULL, ImGuiWindowFlags_MenuBar)) {
        igEnd();
        return;
    }

    // Menu bar with Add Entity option
    if (igBeginMenuBar()) {
        ui_scene_hierarchy_draw_add_menu(state);
        igEndMenuBar();
    }

    // Rebuild cache if dirty
    if (state->cache_dirty) {
        ui_scene_hierarchy_rebuild_cache(state);
        // Reset to first page when cache is rebuilt
        state->current_page = 0;
    }

    // Entity count summary
    int line_count = ecs_scene_line_count(state->scene);
    int point_count = ecs_scene_point_count(state->scene);
    igText("Total: %d lines, %d points", line_count, point_count);

    // Filter input
    igSetNextItemWidth(-1);  // Full width
    bool filter_changed = igInputTextWithHint(
        "##filter",
        "Filter (type or #id)...",
        state->filter_text,
        UI_HIERARCHY_FILTER_MAX_LEN,
        ImGuiInputTextFlags_None,
        NULL, NULL
    );

    // Update filter active state
    state->filter_active = (state->filter_text[0] != '\0');

    // Reset to first page when filter changes
    if (filter_changed) {
        state->current_page = 0;
    }

    // Count filtered entities and build display list
    int filtered_count = 0;
    for (int i = 0; i < state->cache_count; i++) {
        if (ui_hierarchy_entry_matches_filter(&state->cache[i], state->filter_text)) {
            filtered_count++;
        }
    }

    // Show filtered count if filter is active
    if (state->filter_active) {
        igText("Showing %d of %d", filtered_count, state->cache_count);
    }

    // Calculate pagination
    int total_pages = (filtered_count + state->items_per_page - 1) / state->items_per_page;
    if (total_pages < 1) total_pages = 1;

    // Clamp current page
    if (state->current_page >= total_pages) {
        state->current_page = total_pages - 1;
    }
    if (state->current_page < 0) {
        state->current_page = 0;
    }

    // Pagination controls (only show if more than one page)
    if (total_pages > 1) {
        // First page button
        if (igButton("|<", (ImVec2){0, 0})) {
            state->current_page = 0;
        }
        igSameLine(0, 2);

        // Previous page button
        if (igButton("<", (ImVec2){0, 0})) {
            if (state->current_page > 0) state->current_page--;
        }
        igSameLine(0, 5);

        // Page indicator
        igText("Page %d / %d", state->current_page + 1, total_pages);
        igSameLine(0, 5);

        // Next page button
        if (igButton(">", (ImVec2){0, 0})) {
            if (state->current_page < total_pages - 1) state->current_page++;
        }
        igSameLine(0, 2);

        // Last page button
        if (igButton(">|", (ImVec2){0, 0})) {
            state->current_page = total_pages - 1;
        }
    }

    igSeparator();

    // Check for Ctrl/Shift held
    ImGuiIO* io = igGetIO_Nil();
    bool ctrl_held = io->KeyCtrl;
    bool shift_held = io->KeyShift;

    // Track if we need to mark dirty due to deletion
    bool entity_deleted = false;

    // Display paginated and filtered entities
    int start_index = state->current_page * state->items_per_page;
    int displayed = 0;
    int filtered_index = 0;

    for (int i = 0; i < state->cache_count && displayed < state->items_per_page; i++) {
        ui_hierarchy_entry_t *entry = &state->cache[i];

        // Skip if doesn't match filter
        if (!ui_hierarchy_entry_matches_filter(entry, state->filter_text)) {
            continue;
        }

        // Skip entries before current page
        if (filtered_index < start_index) {
            filtered_index++;
            continue;
        }

        // Draw entity
        if (ui_scene_hierarchy_draw_entity(state, entry->entity, entry->type,
                                            ctrl_held, shift_held)) {
            entity_deleted = true;
        }

        displayed++;
        filtered_index++;
    }

    // Show message if no entities match filter
    if (filtered_count == 0 && state->filter_active) {
        igTextDisabled("No entities match filter");
    } else if (state->cache_count == 0) {
        igTextDisabled("No entities in scene");
    }

    // Mark cache dirty if any entity was deleted
    if (entity_deleted) {
        state->cache_dirty = true;
    }

    igEnd();
}

#endif // UI_SCENE_HIERARCHY_H
