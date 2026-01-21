//------------------------------------------------------------------------------
// ui_thick_lines_controls.h - ImGui controls for thick lines (header-only)
//------------------------------------------------------------------------------
#ifndef UI_THICK_LINES_CONTROLS_H
#define UI_THICK_LINES_CONTROLS_H

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"
#include "../instanced_lines.h"
#include "../instanced_polylines.h"

//------------------------------------------------------------------------------
// Types
//------------------------------------------------------------------------------
typedef struct {
    instanced_lines_t* lines;
    instanced_polylines_t* polylines;
} ui_thick_lines_controls_state_t;

//------------------------------------------------------------------------------
// Functions
//------------------------------------------------------------------------------
static inline void ui_thick_lines_controls_init(
    ui_thick_lines_controls_state_t* state,
    instanced_lines_t* lines,
    instanced_polylines_t* polylines
) {
    state->lines = lines;
    state->polylines = polylines;
}

static inline void ui_thick_lines_controls_draw(ui_thick_lines_controls_state_t* state) {
    if (igBegin("Thick Lines Controls", NULL, 0)) {
        instanced_lines_t* il = state->lines;
        instanced_polylines_t* pl = state->polylines;

        // Instanced Lines section
        if (igCollapsingHeader_BoolPtr("Instanced Lines", NULL, ImGuiTreeNodeFlags_DefaultOpen)) {
            igText("Instances: %d", INSTANCED_LINES_COUNT);
            igText("Template verts: %d, indices: %d", il->template_vertex_count, il->template_index_count);
            igSeparator();

            // Regeneration buttons
            igText("Regenerate");
            if (igButton("All##lines", (ImVec2){0, 0})) {
                instanced_lines_regenerate_all(il);
            }
            igSameLine(0.0f, 4.0f);
            if (igButton("Positions##lines", (ImVec2){0, 0})) {
                instanced_lines_regenerate_positions(il);
            }
            igSameLine(0.0f, 4.0f);
            if (igButton("Colors##lines", (ImVec2){0, 0})) {
                instanced_lines_regenerate_colors(il);
            }
            igSameLine(0.0f, 4.0f);
            if (igButton("Motion##lines", (ImVec2){0, 0})) {
                instanced_lines_regenerate_directions(il);
            }

            igSeparator();
            igText("Parameters");

            igSliderFloat("Line Width##lines", &il->line_width, 0.01f, 0.3f, "%.3f", 0);

            bool positions_changed = false;
            positions_changed |= igSliderFloat("Sphere Radius##lines", &il->sphere_radius, 1.0f, 15.0f, "%.1f", 0);
            if (positions_changed) {
                instanced_lines_regenerate_positions(il);
            }

            igSliderFloat("Line Length##lines", &il->line_length, 0.05f, 1.0f, "%.2f", 0);
            igSliderFloat("Wiggle Amplitude##lines", &il->wiggle_amplitude, 0.0f, 2.0f, "%.2f", 0);
            igSliderFloat("Wiggle Frequency##lines", &il->wiggle_frequency, 0.1f, 10.0f, "%.1f", 0);

            if (igButton("Reset Defaults##lines", (ImVec2){0, 0})) {
                il->line_width = INSTANCED_LINES_DEFAULT_WIDTH;
                il->sphere_radius = INSTANCED_LINES_DEFAULT_SPHERE_RADIUS;
                il->wiggle_amplitude = INSTANCED_LINES_DEFAULT_WIGGLE_AMPLITUDE;
                il->wiggle_frequency = INSTANCED_LINES_DEFAULT_WIGGLE_FREQUENCY;
                il->line_length = INSTANCED_LINES_DEFAULT_LINE_LENGTH;
                instanced_lines_regenerate_all(il);
            }
        }

        igSpacing();

        // Instanced Polylines section
        if (igCollapsingHeader_BoolPtr("Instanced Polylines", NULL, ImGuiTreeNodeFlags_DefaultOpen)) {
            igText("Polylines: %d, Points/line: %d", POLYLINES_COUNT, POLYLINE_POINTS);
            igText("Segments: %d, Joins: %d", pl->segment_count, pl->join_count);
            igSeparator();

            if (igButton("Regenerate##polylines", (ImVec2){0, 0})) {
                instanced_polylines_regenerate(pl);
            }

            igSeparator();
            igText("Parameters");

            igSliderFloat("Line Width##polylines", &pl->line_width, 0.01f, 0.3f, "%.3f", 0);
            igSliderFloat("Scale##polylines", &pl->scale, 1.0f, 10.0f, "%.1f", 0);
            igSliderFloat("Speed##polylines", &pl->speed, 0.1f, 3.0f, "%.2f", 0);

            if (igButton("Reset Defaults##polylines", (ImVec2){0, 0})) {
                pl->line_width = POLYLINES_DEFAULT_WIDTH;
                pl->scale = POLYLINES_DEFAULT_SCALE;
                pl->speed = POLYLINES_DEFAULT_SPEED;
                instanced_polylines_regenerate(pl);
            }
        }
    }
    igEnd();
}

#endif // UI_THICK_LINES_CONTROLS_H
