//------------------------------------------------------------------------------
// ui_visibility.h - Visibility controls panel (header-only)
//
// Provides checkboxes to show/hide renderable objects.
// Easy to extend when adding new drawable objects.
//------------------------------------------------------------------------------
#ifndef UI_VISIBILITY_H
#define UI_VISIBILITY_H

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"

//------------------------------------------------------------------------------
// Types
//------------------------------------------------------------------------------

typedef struct {
    // ECS Scene
    bool show_ecs_entities;
    bool *show_pick_debug;       // Pointer to external pick debug window state
    bool *show_slot_buffer_debug; // Pointer to external slot buffer debug window state

    // ECS thickness controls (pointers to external state)
    float *ecs_line_width;
    float *ecs_point_size;
    float *pick_thickness_multiplier;  // Multiplier >= 1.0 for pick buffer

    // Selection state (pointer to external)
    int *selection_count;
} ui_visibility_state_t;

//------------------------------------------------------------------------------
// Functions
//------------------------------------------------------------------------------

static inline void ui_visibility_init(ui_visibility_state_t* vis) {
    vis->show_ecs_entities = true;      // ECS entities visible by default
    vis->show_pick_debug = NULL;        // Set via setter
    vis->show_slot_buffer_debug = NULL; // Set via setter
    vis->ecs_line_width = NULL;
    vis->ecs_point_size = NULL;
    vis->pick_thickness_multiplier = NULL;
    vis->selection_count = NULL;
}

static inline void ui_visibility_set_pick_debug_ptr(ui_visibility_state_t* vis, bool *pick_debug_open) {
    vis->show_pick_debug = pick_debug_open;
}

static inline void ui_visibility_set_slot_buffer_debug_ptr(ui_visibility_state_t* vis, bool *slot_debug_open) {
    vis->show_slot_buffer_debug = slot_debug_open;
}

static inline void ui_visibility_set_ecs_thickness_ptrs(ui_visibility_state_t* vis,
                                                         float *line_width,
                                                         float *point_size,
                                                         float *pick_multiplier) {
    vis->ecs_line_width = line_width;
    vis->ecs_point_size = point_size;
    vis->pick_thickness_multiplier = pick_multiplier;
}

static inline void ui_visibility_set_selection_count_ptr(ui_visibility_state_t* vis, int *count) {
    vis->selection_count = count;
}

static inline void ui_visibility_draw(ui_visibility_state_t* vis) {
    igBegin("Visibility", NULL, 0);

    igSeparatorText("ECS Scene");
    igCheckbox("Show ECS Entities", &vis->show_ecs_entities);

    // Selection count
    if (vis->selection_count) {
        int count = *vis->selection_count;
        if (count == 0) {
            igTextDisabled("No selection");
        } else if (count == 1) {
            igText("1 entity selected");
        } else {
            igText("%d entities selected", count);
        }
    }

    // ECS thickness controls
    if (vis->ecs_line_width) {
        igSliderFloat("ECS Line Width", vis->ecs_line_width, 0.005f, 0.1f, "%.3f", 0);
    }
    if (vis->ecs_point_size) {
        igSliderFloat("ECS Point Size", vis->ecs_point_size, 0.01f, 0.2f, "%.3f", 0);
    }
    if (vis->pick_thickness_multiplier) {
        igSliderFloat("Pick Thickness x", vis->pick_thickness_multiplier, 1.0f, 5.0f, "%.1f", 0);
        igSameLine(0, 5);
        igTextDisabled("(?)");
        if (igIsItemHovered(0)) {
            igSetTooltip("Multiplier for pick buffer line/point thickness.\nHigher values make entities easier to pick.");
        }
    }

    igSeparatorText("Debug Windows");
    if (vis->show_pick_debug) {
        igCheckbox("Pick Buffer Debug", vis->show_pick_debug);
    }
    if (vis->show_slot_buffer_debug) {
        igCheckbox("Slot Buffer Debug", vis->show_slot_buffer_debug);
    }

    igEnd();
}

#endif // UI_VISIBILITY_H
