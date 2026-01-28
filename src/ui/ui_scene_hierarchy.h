//------------------------------------------------------------------------------
// ui_scene_hierarchy.h - Scene hierarchy panel (header-only)
//
// Lists all ECS entities and allows selection via click.
// Includes entity creation menu.
//------------------------------------------------------------------------------
#ifndef UI_SCENE_HIERARCHY_H
#define UI_SCENE_HIERARCHY_H

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"
#include "../selection.h"
#include "../ecs/ecs_scene.h"

#include <stdlib.h>  // for rand()

//------------------------------------------------------------------------------
// Types
//------------------------------------------------------------------------------

typedef struct {
    selection_buffer_t *selection;
    ecs_scene_t *scene;
} ui_scene_hierarchy_state_t;

//------------------------------------------------------------------------------
// Initialization
//------------------------------------------------------------------------------

static inline void ui_scene_hierarchy_init(ui_scene_hierarchy_state_t *state,
                                            selection_buffer_t *selection,
                                            ecs_scene_t *scene) {
    state->selection = selection;
    state->scene = scene;
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
        }

        if (igMenuItem_Bool("Line (X axis)", NULL, false, true)) {
            vec4_t color = ui_scene_hierarchy_random_color();
            scene_add_line(state->scene,
                vec3_make(-1.0f, 0.0f, 0.0f),
                vec3_make(1.0f, 0.0f, 0.0f),
                color, 0.03f);
        }

        if (igMenuItem_Bool("Line (Y axis)", NULL, false, true)) {
            vec4_t color = ui_scene_hierarchy_random_color();
            scene_add_line(state->scene,
                vec3_make(0.0f, -1.0f, 0.0f),
                vec3_make(0.0f, 1.0f, 0.0f),
                color, 0.03f);
        }

        if (igMenuItem_Bool("Line (Z axis)", NULL, false, true)) {
            vec4_t color = ui_scene_hierarchy_random_color();
            scene_add_line(state->scene,
                vec3_make(0.0f, 0.0f, -1.0f),
                vec3_make(0.0f, 0.0f, 1.0f),
                color, 0.03f);
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
        }

        igEndMenu();
    }
}

//------------------------------------------------------------------------------
// Internal: Entity entry for sorted display
//------------------------------------------------------------------------------

typedef struct {
    ecs_entity_t entity;
    geometry_type_t type;
} ui_hierarchy_entry_t;

// Comparison function for qsort (sort by entity ID ascending)
static inline int ui_hierarchy_compare_entries(const void *a, const void *b) {
    const ui_hierarchy_entry_t *ea = (const ui_hierarchy_entry_t*)a;
    const ui_hierarchy_entry_t *eb = (const ui_hierarchy_entry_t*)b;
    if (ea->entity < eb->entity) return -1;
    if (ea->entity > eb->entity) return 1;
    return 0;
}

//------------------------------------------------------------------------------
// Main Draw Function
//------------------------------------------------------------------------------

// Maximum entities to display (for static allocation)
#define UI_HIERARCHY_MAX_ENTITIES 1024

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

    ecs_world_state_t *w = state->scene->world;
    selection_buffer_t *sel = state->selection;

    // Count entities
    int line_count = ecs_scene_line_count(state->scene);
    int point_count = ecs_scene_point_count(state->scene);
    igText("Entities: %d lines, %d points", line_count, point_count);

    igSeparator();

    // Collect all entities into a sorted array for stable display order
    static ui_hierarchy_entry_t entries[UI_HIERARCHY_MAX_ENTITIES];
    int entry_count = 0;

    // Query all entities with GeometryComp
    ecs_query_t *q = ecs_query(w->world, {
        .terms = {
            { .id = w->GeometryComp_id }
        }
    });

    ecs_iter_t it = ecs_query_iter(w->world, q);
    while (ecs_query_next(&it)) {
        GeometryComp *geoms = ecs_field(&it, GeometryComp, 0);

        for (int i = 0; i < it.count && entry_count < UI_HIERARCHY_MAX_ENTITIES; i++) {
            entries[entry_count].entity = it.entities[i];
            entries[entry_count].type = geoms[i].type;
            entry_count++;
        }
    }

    ecs_query_fini(q);

    // Sort by entity ID for stable display order
    qsort(entries, entry_count, sizeof(ui_hierarchy_entry_t), ui_hierarchy_compare_entries);

    // Check for Ctrl/Shift held
    ImGuiIO* io = igGetIO_Nil();
    bool ctrl_held = io->KeyCtrl;
    bool shift_held = io->KeyShift;

    // Display sorted entities
    for (int i = 0; i < entry_count; i++) {
        ecs_entity_t e = entries[i].entity;
        geometry_type_t type = entries[i].type;

        bool is_selected = selection_contains(sel, e);

        // Create label: Type #EntityID
        char label[64];
        snprintf(label, sizeof(label), "%s #%llu",
                 geometry_type_name(type),
                 (unsigned long long)e);

        // Selectable item
        if (igSelectable_Bool(label, is_selected, ImGuiSelectableFlags_None, (ImVec2){0, 0})) {
            if (ctrl_held) {
                // Ctrl+click: toggle selection
                selection_toggle(sel, e);
            } else if (shift_held) {
                // Shift+click: add to selection
                selection_add(sel, e);
            } else {
                // Normal click: replace selection
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
                // Remove from selection if selected
                selection_remove(sel, e);
                // Delete entity
                scene_remove_entity(state->scene, e);
            }
            igEndPopup();
        }
    }

    igEnd();
}

#endif // UI_SCENE_HIERARCHY_H
