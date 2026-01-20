//------------------------------------------------------------------------------
// ui_lines_controls.h - ImGui controls for dynamic lines (header-only)
//------------------------------------------------------------------------------
#ifndef UI_LINES_CONTROLS_H
#define UI_LINES_CONTROLS_H

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"
#include "../dynamic_lines.h"

//------------------------------------------------------------------------------
// Types
//------------------------------------------------------------------------------
typedef struct {
    dynamic_lines_t* lines;
} ui_lines_controls_state_t;

//------------------------------------------------------------------------------
// Functions
//------------------------------------------------------------------------------
static inline void ui_lines_controls_init(ui_lines_controls_state_t* state, dynamic_lines_t* lines) {
    state->lines = lines;
}

static inline void ui_lines_controls_draw(ui_lines_controls_state_t* state) {
    if (igBegin("Lines Controls", NULL, 0)) {
        dynamic_lines_t* dl = state->lines;

        igText("Lines: %d", LINES_COUNT);
        igText("Vertices: %d", LINES_COUNT * 2);
        igSeparator();

        // Regeneration buttons
        igTextColored((ImVec4){1.0f, 1.0f, 0.5f, 1.0f}, "Regenerate");
        if (igButton("All", (ImVec2){0, 0})) {
            dynamic_lines_regenerate_all(dl);
        }
        igSameLine(0.0f, 4.0f);
        if (igButton("Positions", (ImVec2){0, 0})) {
            dynamic_lines_regenerate_positions(dl);
        }
        igSameLine(0.0f, 4.0f);
        if (igButton("Colors", (ImVec2){0, 0})) {
            dynamic_lines_regenerate_colors(dl);
        }
        igSameLine(0.0f, 4.0f);
        if (igButton("Motion", (ImVec2){0, 0})) {
            dynamic_lines_regenerate_directions(dl);
        }

        igSeparator();

        // Parameter sliders
        igTextColored((ImVec4){1.0f, 1.0f, 0.5f, 1.0f}, "Parameters");

        bool positions_changed = false;
        positions_changed |= igSliderFloat("Sphere Radius", &dl->sphere_radius, 1.0f, 20.0f, "%.1f", 0);
        if (positions_changed) {
            dynamic_lines_regenerate_positions(dl);
        }

        igSliderFloat("Line Length", &dl->line_length, 0.01f, 1.0f, "%.2f", 0);
        igSliderFloat("Wiggle Amplitude", &dl->wiggle_amplitude, 0.0f, 2.0f, "%.2f", 0);
        igSliderFloat("Wiggle Frequency", &dl->wiggle_frequency, 0.1f, 10.0f, "%.1f", 0);

        igSeparator();

        // Reset to defaults button
        if (igButton("Reset to Defaults", (ImVec2){0, 0})) {
            dl->sphere_radius = LINES_DEFAULT_SPHERE_RADIUS;
            dl->wiggle_amplitude = LINES_DEFAULT_WIGGLE_AMPLITUDE;
            dl->wiggle_frequency = LINES_DEFAULT_WIGGLE_FREQUENCY;
            dl->line_length = LINES_DEFAULT_LINE_LENGTH;
            dynamic_lines_regenerate_all(dl);
        }
    }
    igEnd();
}

#endif // UI_LINES_CONTROLS_H
