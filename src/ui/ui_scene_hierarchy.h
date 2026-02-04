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
#include "../scene_serializer.h"
#include "../undo_redo_exec.h"  // Includes undo_redo.h and provides undo/redo functions
#include "ui_file_browser.h"
#include "../ply_loader.h"
#include "../ply_import_job.h"

#include <stdlib.h>  // for rand(), qsort(), malloc(), realloc(), free()
#include <string.h>  // for strstr(), strlen()
#include <ctype.h>   // for tolower()

//------------------------------------------------------------------------------
// Configuration
//------------------------------------------------------------------------------

#define UI_HIERARCHY_ITEMS_PER_PAGE 100
#define UI_HIERARCHY_FILTER_MAX_LEN 64
#define UI_HIERARCHY_DRAG_DROP_TYPE "ENTITY_ID"

//------------------------------------------------------------------------------
// Entity cache entry
//------------------------------------------------------------------------------

typedef struct {
    ecs_entity_t entity;
    ecs_entity_t parent;  // 0 if root entity
    geometry_type_t type;
    int depth;            // Hierarchy depth (0 = root)
} ui_hierarchy_entry_t;

//------------------------------------------------------------------------------
// Types
//------------------------------------------------------------------------------

typedef struct {
    selection_buffer_t *selection;
    ecs_scene_t *scene;
    undo_redo_t *undo_redo;  // Optional, can be NULL if no undo/redo

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

    // Save/Load state
    file_browser_t file_browser;
    bool clear_on_load;
    char last_status[128];
    char last_folder[512];  // Remember last used folder

    // PLY Import state
    file_browser_t ply_browser;
    bool ply_import_popup_open;
    char ply_import_path[512];
    int ply_vertex_count;
    bool ply_has_colors;
    int ply_import_mode;           // 0 = Point Cloud Node, 1 = Editable Subtree (individual points)
    int ply_unit_index;            // 0 = Meters, 1 = Millimeters, 2 = Inches
    float ply_point_size;
    bool ply_use_colors;           // Use PLY colors or default color
    float ply_default_color[3];    // Default color when not using PLY colors

    // PLY Import job (for progress bar)
    ply_import_job_t import_job;
    bool import_progress_popup_open;
} ui_scene_hierarchy_state_t;

//------------------------------------------------------------------------------
// Initialization / Shutdown
//------------------------------------------------------------------------------

static inline void ui_scene_hierarchy_init(ui_scene_hierarchy_state_t *state,
                                            selection_buffer_t *selection,
                                            ecs_scene_t *scene) {
    state->selection = selection;
    state->scene = scene;
    state->undo_redo = NULL;  // Set via ui_scene_hierarchy_set_undo_redo

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

    // Initialize file browser
    file_browser_init(&state->file_browser);
    state->clear_on_load = true;
    state->last_status[0] = '\0';
    state->last_folder[0] = '\0';  // Will use cwd on first use

    // Initialize PLY import state
    file_browser_init(&state->ply_browser);
    state->ply_import_popup_open = false;
    state->ply_import_path[0] = '\0';
    state->ply_vertex_count = 0;
    state->ply_has_colors = false;
    state->ply_import_mode = 0;       // Point Cloud Node (default, more efficient)
    state->ply_unit_index = 0;        // Meters (default)
    state->ply_point_size = 0.01f;    // Default point size
    state->ply_use_colors = true;     // Use PLY colors by default
    state->ply_default_color[0] = 1.0f;
    state->ply_default_color[1] = 1.0f;
    state->ply_default_color[2] = 1.0f;

    // Initialize import job
    ply_import_job_init(&state->import_job);
    state->import_progress_popup_open = false;
}

// Set undo/redo system (optional, can be NULL)
static inline void ui_scene_hierarchy_set_undo_redo(ui_scene_hierarchy_state_t *state,
                                                     undo_redo_t *undo_redo) {
    state->undo_redo = undo_redo;
}

static inline void ui_scene_hierarchy_shutdown(ui_scene_hierarchy_state_t *state) {
    if (state->cache) {
        free(state->cache);
        state->cache = NULL;
    }
    state->cache_count = 0;
    state->cache_capacity = 0;
    file_browser_shutdown(&state->file_browser);
    file_browser_shutdown(&state->ply_browser);

    // Cleanup any running import job
    if (ply_import_job_is_running(&state->import_job)) {
        ply_import_job_cancel(&state->import_job, state->scene);
    }
    ply_import_job_reset(&state->import_job);
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
// Internal: Compute hierarchy depth
//------------------------------------------------------------------------------

static inline int ui_hierarchy_compute_depth(ecs_world_state_t *w, ecs_entity_t e) {
    int depth = 0;
    ecs_entity_t current = e;
    while (current != 0) {
        ecs_entity_t parent = ecs_get_parent(w->world, current);
        if (parent == 0) break;
        depth++;
        current = parent;
        if (depth > 100) break;  // Safety limit
    }
    return depth;
}

//------------------------------------------------------------------------------
// Internal: Check if 'potential_descendant' is a descendant of 'ancestor'
// Used to prevent creating cycles when reparenting
//------------------------------------------------------------------------------

static inline bool ui_hierarchy_is_descendant(ecs_world_state_t *w,
                                               ecs_entity_t potential_descendant,
                                               ecs_entity_t ancestor) {
    if (potential_descendant == 0 || ancestor == 0) return false;
    if (potential_descendant == ancestor) return true;  // Same entity

    // Walk up the hierarchy from potential_descendant
    ecs_entity_t current = potential_descendant;
    int depth = 0;
    while (current != 0 && depth < 100) {
        ecs_entity_t parent = ecs_get_parent(w->world, current);
        if (parent == 0) break;
        if (parent == ancestor) return true;
        current = parent;
        depth++;
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

    // Second pass: collect entities with parent info
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
            ecs_entity_t e = it.entities[i];
            state->cache[state->cache_count].entity = e;
            state->cache[state->cache_count].type = geoms[i].type;
            state->cache[state->cache_count].parent = ecs_get_parent(w->world, e);
            state->cache[state->cache_count].depth = ui_hierarchy_compute_depth(w, e);
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
        ecs_entity_t new_entity = 0;

        if (igMenuItem_Bool("Point", NULL, false, true)) {
            vec4_t color = ui_scene_hierarchy_random_color();
            new_entity = scene_add_point(state->scene,
                vec3_make(0.0f, 0.0f, 0.0f),
                color, 0.06f);
            state->cache_dirty = true;
        }

        if (igMenuItem_Bool("Line (X axis)", NULL, false, true)) {
            vec4_t color = ui_scene_hierarchy_random_color();
            new_entity = scene_add_line(state->scene,
                vec3_make(-1.0f, 0.0f, 0.0f),
                vec3_make(1.0f, 0.0f, 0.0f),
                color, 0.03f);
            state->cache_dirty = true;
        }

        if (igMenuItem_Bool("Line (Y axis)", NULL, false, true)) {
            vec4_t color = ui_scene_hierarchy_random_color();
            new_entity = scene_add_line(state->scene,
                vec3_make(0.0f, -1.0f, 0.0f),
                vec3_make(0.0f, 1.0f, 0.0f),
                color, 0.03f);
            state->cache_dirty = true;
        }

        if (igMenuItem_Bool("Line (Z axis)", NULL, false, true)) {
            vec4_t color = ui_scene_hierarchy_random_color();
            new_entity = scene_add_line(state->scene,
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
            new_entity = scene_add_line(state->scene, a, b, color, 0.03f);
            state->cache_dirty = true;
        }

        igSeparator();

        if (igMenuItem_Bool("Polyline (Zigzag)", NULL, false, true)) {
            vec4_t color = ui_scene_hierarchy_random_color();
            vec3_t points[] = {
                vec3_make(-1.5f, 0.0f, 0.0f),
                vec3_make(-0.5f, 1.0f, 0.0f),
                vec3_make(0.5f, -0.5f, 0.0f),
                vec3_make(1.5f, 0.5f, 0.0f)
            };
            new_entity = scene_add_polyline(state->scene, points, 4, color, 0.03f);
            state->cache_dirty = true;
        }

        if (igMenuItem_Bool("Arc (Quarter Circle)", NULL, false, true)) {
            vec4_t color = ui_scene_hierarchy_random_color();
            new_entity = scene_add_arc(state->scene,
                vec3_make(0.0f, 0.0f, 0.0f),  // center
                1.0f,                          // radius
                0.0f,                          // start angle
                1.5707963f,                    // end angle (PI/2)
                vec3_make(0.0f, 0.0f, 1.0f),  // normal (XY plane)
                color, 0.03f);
            state->cache_dirty = true;
        }

        if (igMenuItem_Bool("Arc (Semicircle)", NULL, false, true)) {
            vec4_t color = ui_scene_hierarchy_random_color();
            new_entity = scene_add_arc(state->scene,
                vec3_make(0.0f, 0.0f, 0.0f),
                1.0f,
                0.0f,
                3.1415926f,                    // PI
                vec3_make(0.0f, 0.0f, 1.0f),
                color, 0.03f);
            state->cache_dirty = true;
        }

        if (igMenuItem_Bool("Polygon (Triangle)", NULL, false, true)) {
            vec4_t color = ui_scene_hierarchy_random_color();
            vec3_t points[] = {
                vec3_make(0.0f, 1.0f, 0.0f),
                vec3_make(-0.866f, -0.5f, 0.0f),
                vec3_make(0.866f, -0.5f, 0.0f)
            };
            new_entity = scene_add_polygon(state->scene, points, 3, color, 0.03f);
            state->cache_dirty = true;
        }

        if (igMenuItem_Bool("Polygon (Square)", NULL, false, true)) {
            vec4_t color = ui_scene_hierarchy_random_color();
            vec3_t points[] = {
                vec3_make(-0.7f, -0.7f, 0.0f),
                vec3_make(0.7f, -0.7f, 0.0f),
                vec3_make(0.7f, 0.7f, 0.0f),
                vec3_make(-0.7f, 0.7f, 0.0f)
            };
            new_entity = scene_add_polygon(state->scene, points, 4, color, 0.03f);
            state->cache_dirty = true;
        }

        if (igMenuItem_Bool("Polygon (Pentagon)", NULL, false, true)) {
            vec4_t color = ui_scene_hierarchy_random_color();
            vec3_t points[5];
            for (int i = 0; i < 5; i++) {
                float angle = (float)i * 1.2566370f - 1.5707963f;  // 2*PI/5, offset by -PI/2
                points[i] = vec3_make(cosf(angle), sinf(angle), 0.0f);
            }
            new_entity = scene_add_polygon(state->scene, points, 5, color, 0.03f);
            state->cache_dirty = true;
        }

        igSeparator();

        if (igMenuItem_Bool("Bezier Curve", NULL, false, true)) {
            vec4_t color = ui_scene_hierarchy_random_color();
            new_entity = scene_add_bezier(state->scene,
                vec3_make(-1.5f, 0.0f, 0.0f),   // p0
                vec3_make(-0.5f, 1.5f, 0.0f),   // p1 (control)
                vec3_make(0.5f, -1.5f, 0.0f),   // p2 (control)
                vec3_make(1.5f, 0.0f, 0.0f),    // p3
                16,                              // segments
                color, 0.03f);
            state->cache_dirty = true;
        }

        if (igMenuItem_Bool("Helix", NULL, false, true)) {
            vec4_t color = ui_scene_hierarchy_random_color();
            new_entity = scene_add_helix(state->scene,
                vec3_make(0.0f, -1.0f, 0.0f),   // axis start
                vec3_make(0.0f, 1.0f, 0.0f),    // axis end
                0.5f,                            // radius
                2.0f,                            // turns
                32,                              // segments
                color, 0.03f);
            state->cache_dirty = true;
        }

        // Record undo command for entity creation
        if (new_entity != 0 && state->undo_redo) {
            undo_cmd_create_entity(state->undo_redo, new_entity);
        }

        igEndMenu();
    }
}

//------------------------------------------------------------------------------
// Internal: Find cache entry for entity
//------------------------------------------------------------------------------

static inline ui_hierarchy_entry_t* ui_hierarchy_find_entry(ui_scene_hierarchy_state_t *state,
                                                             ecs_entity_t e) {
    for (int i = 0; i < state->cache_count; i++) {
        if (state->cache[i].entity == e) {
            return &state->cache[i];
        }
    }
    return NULL;
}

//------------------------------------------------------------------------------
// Internal: Draw single entity as leaf (no children)
//------------------------------------------------------------------------------

static inline bool ui_scene_hierarchy_draw_entity_leaf(ui_scene_hierarchy_state_t *state,
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

    // Leaf node flags
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    if (is_selected) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    igTreeNodeEx_Str(label, flags);

    // Handle click on tree node
    if (igIsItemClicked(ImGuiMouseButton_Left)) {
        if (ctrl_held) {
            selection_toggle(sel, e);
        } else if (shift_held) {
            selection_add(sel, e);
        } else {
            selection_set_single(sel, e);
        }
    }

    // Drag source: allow dragging this entity
    if (igBeginDragDropSource(ImGuiDragDropFlags_None)) {
        igSetDragDropPayload(UI_HIERARCHY_DRAG_DROP_TYPE, &e, sizeof(ecs_entity_t), ImGuiCond_Once);
        igText("Move %s #%llu", geometry_type_name(type), (unsigned long long)e);
        igEndDragDropSource();
    }

    // Drop target: allow dropping other entities onto this one (making them children)
    if (igBeginDragDropTarget()) {
        const ImGuiPayload* payload = igAcceptDragDropPayload(UI_HIERARCHY_DRAG_DROP_TYPE, ImGuiDragDropFlags_None);
        if (payload) {
            ecs_entity_t dragged_entity = *(ecs_entity_t*)payload->Data;

            // Validate drop:
            // 1. Can't drop onto self
            // 2. Can't drop parent onto its own descendant (would create cycle)
            if (dragged_entity != e &&
                !ui_hierarchy_is_descendant(state->scene->world, e, dragged_entity)) {
                scene_set_parent(state->scene, dragged_entity, e);
                state->cache_dirty = true;
            }
        }
        igEndDragDropTarget();
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
        if (igMenuItem_Bool("Make Child of Selected", NULL, false, sel->count == 1 && sel->entities[0] != e)) {
            ecs_entity_t parent = sel->entities[0];
            scene_set_parent(state->scene, e, parent);
            state->cache_dirty = true;
        }
        if (igMenuItem_Bool("Unparent", NULL, false, scene_has_parent(state->scene, e))) {
            scene_set_parent(state->scene, e, 0);
            state->cache_dirty = true;
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
// Internal: Draw entity as tree node (recursive)
// Returns true if any entity was deleted
//------------------------------------------------------------------------------

static inline bool ui_scene_hierarchy_draw_entity_tree(ui_scene_hierarchy_state_t *state,
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
    bool has_children = scene_has_children(state->scene, e);

    // Create label: Type #EntityID
    char label[64];
    snprintf(label, sizeof(label), "%s #%llu",
             geometry_type_name(type),
             (unsigned long long)e);

    // Tree node flags
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
    if (is_selected) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }
    if (!has_children) {
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }

    bool node_open = igTreeNodeEx_Str(label, flags);

    // Handle click on tree node
    if (igIsItemClicked(ImGuiMouseButton_Left)) {
        if (ctrl_held) {
            selection_toggle(sel, e);
        } else if (shift_held) {
            selection_add(sel, e);
        } else {
            selection_set_single(sel, e);
        }
    }

    // Drag source: allow dragging this entity
    if (igBeginDragDropSource(ImGuiDragDropFlags_None)) {
        igSetDragDropPayload(UI_HIERARCHY_DRAG_DROP_TYPE, &e, sizeof(ecs_entity_t), ImGuiCond_Once);
        igText("Move %s #%llu", geometry_type_name(type), (unsigned long long)e);
        igEndDragDropSource();
    }

    // Drop target: allow dropping other entities onto this one (making them children)
    if (igBeginDragDropTarget()) {
        const ImGuiPayload* payload = igAcceptDragDropPayload(UI_HIERARCHY_DRAG_DROP_TYPE, ImGuiDragDropFlags_None);
        if (payload) {
            ecs_entity_t dragged_entity = *(ecs_entity_t*)payload->Data;

            // Validate drop:
            // 1. Can't drop onto self
            // 2. Can't drop parent onto its own descendant (would create cycle)
            if (dragged_entity != e &&
                !ui_hierarchy_is_descendant(state->scene->world, e, dragged_entity)) {
                scene_set_parent(state->scene, dragged_entity, e);
                state->cache_dirty = true;
            }
        }
        igEndDragDropTarget();
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
        if (igMenuItem_Bool("Make Child of Selected", NULL, false, sel->count == 1 && sel->entities[0] != e)) {
            ecs_entity_t parent = sel->entities[0];
            scene_set_parent(state->scene, e, parent);
            state->cache_dirty = true;
        }
        if (igMenuItem_Bool("Unparent", NULL, false, scene_has_parent(state->scene, e))) {
            scene_set_parent(state->scene, e, 0);
            state->cache_dirty = true;
        }
        igSeparator();
        if (igMenuItem_Bool("Delete", "Del", false, true)) {
            selection_remove(sel, e);
            scene_remove_entity(state->scene, e);
            entity_deleted = true;
        }
        igEndPopup();
    }

    // Draw children if node is open
    if (has_children && node_open) {
        // Find children in cache and draw them
        for (int i = 0; i < state->cache_count; i++) {
            if (state->cache[i].parent == e) {
                if (ui_scene_hierarchy_draw_entity_tree(state, state->cache[i].entity,
                                                         state->cache[i].type, ctrl_held, shift_held)) {
                    entity_deleted = true;
                }
            }
        }
        igTreePop();
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

    // Menu bar with File, Edit, and Add Entity options
    if (igBeginMenuBar()) {
        // File menu
        if (igBeginMenu("File", true)) {
            if (igMenuItem_Bool("Save Scene...", "Ctrl+S", false, true)) {
                // Use last folder if available, otherwise will use cwd
                char default_path[520];
                if (state->last_folder[0]) {
                    snprintf(default_path, sizeof(default_path), "%s/scene.json", state->last_folder);
                } else {
                    strcpy(default_path, "scene.json");
                }
                file_browser_save_file(&state->file_browser, "Save Scene", ".json", default_path);
            }
            if (igMenuItem_Bool("Load Scene...", "Ctrl+O", false, true)) {
                // Use last folder if available
                file_browser_open_file(&state->file_browser, "Load Scene", ".json",
                                       state->last_folder[0] ? state->last_folder : NULL);
            }
            igSeparator();
            if (igMenuItem_Bool("Import PLY Point Cloud...", NULL, false, true)) {
                // Open PLY file browser
                file_browser_open_file(&state->ply_browser, "Import PLY", ".ply",
                                       state->last_folder[0] ? state->last_folder : NULL);
            }
            igSeparator();
            igCheckbox("Clear on Load", &state->clear_on_load);
            igSeparator();
            if (igMenuItem_Bool("Clear Scene", NULL, false, state->cache_count > 0)) {
                // Clear undo history when clearing scene
                if (state->undo_redo) {
                    undo_redo_clear(state->undo_redo);
                }
                // Clear all entities
                for (int i = state->cache_count - 1; i >= 0; i--) {
                    scene_remove_entity(state->scene, state->cache[i].entity);
                }
                selection_clear(state->selection);
                state->cache_dirty = true;
                strcpy(state->last_status, "Scene cleared");
            }
            igEndMenu();
        }

        // Edit menu with Undo/Redo
        if (igBeginMenu("Edit", true)) {
            // Undo
            bool can_undo = state->undo_redo && undo_redo_can_undo(state->undo_redo);
            const char *undo_name = can_undo ? undo_redo_get_undo_name(state->undo_redo) : NULL;
            char undo_label[64];
            if (undo_name) {
                snprintf(undo_label, sizeof(undo_label), "Undo %s", undo_name);
            } else {
                strcpy(undo_label, "Undo");
            }
            if (igMenuItem_Bool(undo_label, "Ctrl+Z", false, can_undo)) {
                undo_redo_undo(state->undo_redo);
                state->cache_dirty = true;
            }

            // Redo
            bool can_redo = state->undo_redo && undo_redo_can_redo(state->undo_redo);
            const char *redo_name = can_redo ? undo_redo_get_redo_name(state->undo_redo) : NULL;
            char redo_label[64];
            if (redo_name) {
                snprintf(redo_label, sizeof(redo_label), "Redo %s", redo_name);
            } else {
                strcpy(redo_label, "Redo");
            }
            if (igMenuItem_Bool(redo_label, "Ctrl+Shift+Z", false, can_redo)) {
                undo_redo_redo(state->undo_redo);
                state->cache_dirty = true;
            }

            igSeparator();

            // Show history count
            if (state->undo_redo) {
                igTextDisabled("Undo: %d, Redo: %d",
                    undo_redo_get_undo_count(state->undo_redo),
                    undo_redo_get_redo_count(state->undo_redo));
            } else {
                igTextDisabled("No undo/redo");
            }

            igEndMenu();
        }

        ui_scene_hierarchy_draw_add_menu(state);
        igEndMenuBar();
    }

    // Handle keyboard shortcuts (only when Scene Hierarchy window is focused)
    if (igIsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) {
        ImGuiIO* io_kb = igGetIO_Nil();
        if (io_kb->KeyCtrl && igIsKeyPressed_Bool(ImGuiKey_S, false)) {
            char default_path[520];
            if (state->last_folder[0]) {
                snprintf(default_path, sizeof(default_path), "%s/scene.json", state->last_folder);
            } else {
                strcpy(default_path, "scene.json");
            }
            file_browser_save_file(&state->file_browser, "Save Scene", ".json", default_path);
        }
        if (io_kb->KeyCtrl && igIsKeyPressed_Bool(ImGuiKey_O, false)) {
            file_browser_open_file(&state->file_browser, "Load Scene", ".json",
                                   state->last_folder[0] ? state->last_folder : NULL);
        }
    }

    // Draw file browser (returns true when a file is selected)
    if (file_browser_draw(&state->file_browser)) {
        const char *path = file_browser_get_result(&state->file_browser);
        bool success = false;

        if (state->file_browser.mode == FILE_BROWSER_MODE_SAVE) {
            // Save scene
            if (scene_save_to_file(state->scene, path)) {
                snprintf(state->last_status, sizeof(state->last_status),
                         "Saved %d entities", state->cache_count);
                success = true;
            } else {
                snprintf(state->last_status, sizeof(state->last_status),
                         "Failed to save");
            }
        } else {
            // Load scene
            selection_clear(state->selection);
            int count = scene_load_from_file(state->scene, path, state->clear_on_load);
            if (count >= 0) {
                snprintf(state->last_status, sizeof(state->last_status),
                         "Loaded %d entities", count);
                state->cache_dirty = true;
                success = true;
            } else {
                snprintf(state->last_status, sizeof(state->last_status),
                         "Failed to load");
            }
        }

        // Remember the folder for next time on successful operation
        if (success && path[0]) {
            // Find last path separator to extract directory
            const char *last_sep = strrchr(path, '/');
#ifdef _WIN32
            const char *last_sep_win = strrchr(path, '\\');
            if (last_sep_win > last_sep) last_sep = last_sep_win;
#endif
            if (last_sep && last_sep > path) {
                size_t dir_len = (size_t)(last_sep - path);
                if (dir_len < sizeof(state->last_folder)) {
                    memcpy(state->last_folder, path, dir_len);
                    state->last_folder[dir_len] = '\0';
                }
            }
        }

        file_browser_clear_result(&state->file_browser);
    }

    // Handle PLY file browser (opens import options popup when file is selected)
    if (file_browser_draw(&state->ply_browser)) {
        const char *path = file_browser_get_result(&state->ply_browser);
        if (path && path[0]) {
            strncpy(state->ply_import_path, path, sizeof(state->ply_import_path) - 1);
            state->ply_import_path[sizeof(state->ply_import_path) - 1] = '\0';

            // Get PLY info
            ply_error_t err = ply_get_info(path, &state->ply_vertex_count, &state->ply_has_colors);
            if (err == PLY_OK) {
                state->ply_import_popup_open = true;
                igOpenPopup_Str("Import PLY Options", ImGuiPopupFlags_None);
            } else {
                snprintf(state->last_status, sizeof(state->last_status),
                         "PLY Error: %s", ply_error_string(err));
            }

            // Remember folder
            const char *last_sep = strrchr(path, '/');
#ifdef _WIN32
            const char *last_sep_win = strrchr(path, '\\');
            if (last_sep_win > last_sep) last_sep = last_sep_win;
#endif
            if (last_sep && last_sep > path) {
                size_t dir_len = (size_t)(last_sep - path);
                if (dir_len < sizeof(state->last_folder)) {
                    memcpy(state->last_folder, path, dir_len);
                    state->last_folder[dir_len] = '\0';
                }
            }
        }
        file_browser_clear_result(&state->ply_browser);
    }

    // PLY Import Options Popup
    if (igBeginPopupModal("Import PLY Options", &state->ply_import_popup_open, ImGuiWindowFlags_AlwaysAutoResize)) {
        // File info
        igText("File: %s", state->ply_import_path);
        igText("Points: %d", state->ply_vertex_count);
        igText("Has Colors: %s", state->ply_has_colors ? "Yes" : "No");
        igSeparator();

        // Import Mode
        igText("Import Mode:");
        igRadioButton_IntPtr("Point Cloud Node (efficient)", &state->ply_import_mode, 0);
        igSameLine(0, -1);
        igTextDisabled("(?)");
        if (igIsItemHovered(ImGuiHoveredFlags_None)) {
            igSetTooltip("Single entity with all points. Best for large clouds (10k+).");
        }
        igRadioButton_IntPtr("Editable Subtree (individual)", &state->ply_import_mode, 1);
        igSameLine(0, -1);
        igTextDisabled("(?)");
        if (igIsItemHovered(ImGuiHoveredFlags_None)) {
            igSetTooltip("Each point as a separate entity. Best for small clouds (<10k).");
        }
        igSeparator();

        // Unit conversion
        igText("Units:");
        const char* unit_items[] = { "Meters (1:1)", "Millimeters (0.001)", "Inches (0.0254)" };
        igCombo_Str_arr("##units", &state->ply_unit_index, unit_items, 3, -1);
        igSeparator();

        // Point size
        igSliderFloat("Point Size", &state->ply_point_size, 0.001f, 0.1f, "%.3f", ImGuiSliderFlags_None);
        igSeparator();

        // Color options
        if (state->ply_has_colors) {
            igCheckbox("Use PLY Colors", &state->ply_use_colors);
        } else {
            state->ply_use_colors = false;
            igTextDisabled("PLY has no colors, using default");
        }
        if (!state->ply_use_colors) {
            igColorEdit3("Default Color", state->ply_default_color, ImGuiColorEditFlags_None);
        }
        igSeparator();

        // Import/Cancel buttons
        if (igButton("Import", (ImVec2){120, 0})) {
            // Calculate scale factor based on unit selection
            float scale = 1.0f;
            switch (state->ply_unit_index) {
                case 1: scale = 0.001f; break;   // Millimeters
                case 2: scale = 0.0254f; break;  // Inches
                default: scale = 1.0f; break;    // Meters
            }

            vec4_t default_color = vec4_make(
                state->ply_default_color[0],
                state->ply_default_color[1],
                state->ply_default_color[2],
                1.0f
            );

            // Start the import job
            bool started = ply_import_job_start(
                &state->import_job,
                state->ply_import_path,
                state->ply_import_mode,
                scale,
                state->ply_point_size,
                default_color,
                state->ply_use_colors
            );

            if (started) {
                // Check if we should use synchronous import for small files
                if (ply_import_job_should_sync(&state->import_job)) {
                    // Small file - import synchronously
                    while (!ply_import_job_tick(&state->import_job, state->scene)) {
                        // Keep ticking until complete
                    }

                    if (state->import_job.state == PLY_JOB_COMPLETE) {
                        snprintf(state->last_status, sizeof(state->last_status),
                                 "%s", state->import_job.status_message);
                        state->cache_dirty = true;
                    } else {
                        snprintf(state->last_status, sizeof(state->last_status),
                                 "PLY Error: %s", state->import_job.status_message);
                    }
                    ply_import_job_reset(&state->import_job);
                } else {
                    // Large file - use progress bar
                    state->import_progress_popup_open = true;
                }
            } else {
                snprintf(state->last_status, sizeof(state->last_status),
                         "PLY Error: %s", state->import_job.status_message);
            }

            state->ply_import_popup_open = false;
            igCloseCurrentPopup();
        }

        igSameLine(0, -1);
        if (igButton("Cancel", (ImVec2){120, 0})) {
            state->ply_import_popup_open = false;
            igCloseCurrentPopup();
        }

        igEndPopup();
    }

    // Progress popup for large file imports
    if (state->import_progress_popup_open) {
        igOpenPopup_Str("Importing PLY Point Cloud", ImGuiPopupFlags_None);
    }

    if (igBeginPopupModal("Importing PLY Point Cloud", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
        // Extract filename from path for display
        const char *filename = state->ply_import_path;
        const char *last_sep = strrchr(state->ply_import_path, '/');
#ifdef _WIN32
        const char *last_sep_win = strrchr(state->ply_import_path, '\\');
        if (last_sep_win > last_sep) last_sep = last_sep_win;
#endif
        if (last_sep) filename = last_sep + 1;

        igText("File: %s", filename);
        igSeparator();

        // Progress bar
        igProgressBar(state->import_job.progress, (ImVec2){300, 0}, NULL);

        // Status message
        igText("%s", state->import_job.status_message);

        // Progress percentage
        igText("%.0f%%", state->import_job.progress * 100.0f);

        igSeparator();

        // Cancel button centered
        float button_width = 120.0f;
        float avail_width = igGetContentRegionAvail().x;
        igSetCursorPosX(igGetCursorPosX() + (avail_width - button_width) * 0.5f);

        if (igButton("Cancel", (ImVec2){button_width, 0})) {
            ply_import_job_cancel(&state->import_job, state->scene);
            state->import_progress_popup_open = false;
            state->cache_dirty = true;
            snprintf(state->last_status, sizeof(state->last_status), "Import cancelled");
            igCloseCurrentPopup();
        }

        // Process one chunk of work
        if (ply_import_job_is_running(&state->import_job)) {
            bool complete = ply_import_job_tick(&state->import_job, state->scene);

            if (complete) {
                state->import_progress_popup_open = false;
                state->cache_dirty = true;

                if (state->import_job.state == PLY_JOB_COMPLETE) {
                    snprintf(state->last_status, sizeof(state->last_status),
                             "%s", state->import_job.status_message);
                } else if (state->import_job.state == PLY_JOB_ERROR) {
                    snprintf(state->last_status, sizeof(state->last_status),
                             "PLY Error: %s", state->import_job.status_message);
                }

                ply_import_job_reset(&state->import_job);
                igCloseCurrentPopup();
            }
        }

        igEndPopup();
    }

    // Show status message if any
    if (state->last_status[0] != '\0') {
        igTextColored((ImVec4){0.5f, 1.0f, 0.5f, 1.0f}, "%s", state->last_status);
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

    // Drop target for unparenting: invisible drop zone at start of list area
    // Dropping here will unparent the entity (make it a root)
    igBeginGroup();
    igDummy((ImVec2){-1, 2});  // Small invisible area
    if (igBeginDragDropTarget()) {
        const ImGuiPayload* payload = igAcceptDragDropPayload(UI_HIERARCHY_DRAG_DROP_TYPE, ImGuiDragDropFlags_None);
        if (payload) {
            ecs_entity_t dragged_entity = *(ecs_entity_t*)payload->Data;
            // Unparent: set parent to 0
            if (scene_has_parent(state->scene, dragged_entity)) {
                scene_set_parent(state->scene, dragged_entity, 0);
                state->cache_dirty = true;
            }
        }
        igEndDragDropTarget();
    }
    igEndGroup();

    if (state->filter_active) {
        // FLAT VIEW: When filter is active, show all matching entities in a flat list
        // This makes search results easier to see regardless of hierarchy position

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

            // Draw entity as leaf (flat list, no children shown)
            if (ui_scene_hierarchy_draw_entity_leaf(state, entry->entity, entry->type,
                                                     ctrl_held, shift_held)) {
                entity_deleted = true;
            }

            displayed++;
            filtered_index++;
        }

        // Show message if no entities match filter
        if (filtered_count == 0) {
            igTextDisabled("No entities match filter");
        }
    } else {
        // TREE VIEW: When no filter, show hierarchical tree structure
        // Only root entities (no parent) are shown at top level

        // Count root entities for pagination
        int root_count = 0;
        for (int i = 0; i < state->cache_count; i++) {
            if (state->cache[i].parent == 0) {
                root_count++;
            }
        }

        // Recalculate pagination for root entities only
        int root_pages = (root_count + state->items_per_page - 1) / state->items_per_page;
        if (root_pages < 1) root_pages = 1;

        int start_index = state->current_page * state->items_per_page;
        int displayed = 0;
        int root_index = 0;

        for (int i = 0; i < state->cache_count && displayed < state->items_per_page; i++) {
            ui_hierarchy_entry_t *entry = &state->cache[i];

            // Only draw root entities at top level (children drawn recursively)
            if (entry->parent != 0) {
                continue;
            }

            // Skip entries before current page
            if (root_index < start_index) {
                root_index++;
                continue;
            }

            // Draw entity as tree node
            if (ui_scene_hierarchy_draw_entity_tree(state, entry->entity, entry->type,
                                                     ctrl_held, shift_held)) {
                entity_deleted = true;
            }

            displayed++;
            root_index++;
        }

        // Show message if no entities
        if (state->cache_count == 0) {
            igTextDisabled("No entities in scene");
        }
    }

    // Mark cache dirty if any entity was deleted
    if (entity_deleted) {
        state->cache_dirty = true;
    }

    // Drop target at bottom of list: larger area for easy unparenting
    // This fills the remaining space in the window
    ImVec2_c avail = igGetContentRegionAvail();
    if (avail.y > 10.0f) {
        igBeginGroup();
        igDummy((ImVec2){avail.x, avail.y});
        if (igBeginDragDropTarget()) {
            const ImGuiPayload* payload = igAcceptDragDropPayload(UI_HIERARCHY_DRAG_DROP_TYPE, ImGuiDragDropFlags_None);
            if (payload) {
                ecs_entity_t dragged_entity = *(ecs_entity_t*)payload->Data;
                // Unparent: set parent to 0
                if (scene_has_parent(state->scene, dragged_entity)) {
                    scene_set_parent(state->scene, dragged_entity, 0);
                    state->cache_dirty = true;
                }
            }
            igEndDragDropTarget();
        }
        igEndGroup();
    }

    igEnd();
}

#endif // UI_SCENE_HIERARCHY_H
