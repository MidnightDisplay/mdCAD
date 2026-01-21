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
    bool show_cube;
    bool show_lines;
    bool show_instanced_lines;
    bool show_instanced_polylines;
    bool show_alpha_lines;
    bool show_gcode_path;
} ui_visibility_state_t;

//------------------------------------------------------------------------------
// Functions
//------------------------------------------------------------------------------

static inline void ui_visibility_init(ui_visibility_state_t* vis) {
    vis->show_cube = false;
    vis->show_lines = false;
    vis->show_instanced_lines = false;
    vis->show_instanced_polylines = false;
    vis->show_alpha_lines = true;      // Alpha lines visible by default
    vis->show_gcode_path = true;       // G-code path visible by default
}

static inline void ui_visibility_draw(ui_visibility_state_t* vis) {
    igBegin("Visibility", NULL, 0);

    igSeparatorText("Opaque");
    igCheckbox("Show Cube", &vis->show_cube);
    igCheckbox("Show Lines (thin)", &vis->show_lines);
    igCheckbox("Show Instanced Lines (thick)", &vis->show_instanced_lines);
    igCheckbox("Show Instanced Polylines", &vis->show_instanced_polylines);

    igSeparatorText("Alpha-Blended");
    igCheckbox("Show Alpha Lines", &vis->show_alpha_lines);
    igCheckbox("Show G-code Path", &vis->show_gcode_path);

    igEnd();
}

#endif // UI_VISIBILITY_H
