//------------------------------------------------------------------------------
// ui_gcode_controls.h - G-code viewer control panel (header-only)
//
// Provides UI controls for the G-code path viewer:
// - Scale, line width, color
// - Timeline position, base alpha, fade width
//------------------------------------------------------------------------------
#ifndef UI_GCODE_CONTROLS_H
#define UI_GCODE_CONTROLS_H

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"
#include "../instanced_alpha_polylines.h"
#include "../instanced_lines_alpha.h"

//------------------------------------------------------------------------------
// Types
//------------------------------------------------------------------------------
typedef struct {
    instanced_alpha_polylines_t* gcode;
    instanced_lines_alpha_t* alpha_lines;

    // UI state
    float color[3];
    float timeline_percent;
    float base_alpha_percent;
    float fade_width_percent;
} ui_gcode_controls_state_t;

//------------------------------------------------------------------------------
// Functions
//------------------------------------------------------------------------------

static inline void ui_gcode_controls_init(
    ui_gcode_controls_state_t* state,
    instanced_alpha_polylines_t* gcode,
    instanced_lines_alpha_t* alpha_lines
) {
    state->gcode = gcode;
    state->alpha_lines = alpha_lines;

    // Initialize UI state from gcode parameters
    if (gcode) {
        state->color[0] = gcode->color_r;
        state->color[1] = gcode->color_g;
        state->color[2] = gcode->color_b;
        state->timeline_percent = gcode->timeline_position * 100.0f;
        state->base_alpha_percent = gcode->base_alpha * 100.0f;
        state->fade_width_percent = gcode->highlight_width * 100.0f;
    } else {
        // Default values
        state->color[0] = 0.729f;
        state->color[1] = 0.733f;
        state->color[2] = 0.945f;
        state->timeline_percent = 50.0f;
        state->base_alpha_percent = 15.0f;
        state->fade_width_percent = 2.0f;
    }
}

static inline void ui_gcode_controls_draw(ui_gcode_controls_state_t* state) {
    if (!igBegin("G-code Viewer", NULL, 0)) {
        igEnd();
        return;
    }

    // ===== G-code Path Section =====
    if (state->gcode) {
        igSeparatorText("G-code Path");

        // Path info
        igText("Points: %d", state->gcode->path.count);
        igText("Segments: %d", state->gcode->intermediate_count + 2);  // +2 for terminal segments
        igText("Path Length: %.1f mm", state->gcode->path.total_length);

        float width, height, depth;
        gcode_path_get_bounds_size(&state->gcode->path, &width, &height, &depth);
        igText("Size: %.1f x %.1f x %.1f mm", width, height, depth);

        igSpacing();

        // Scale control
        if (igSliderFloat("Scale", &state->gcode->scale, 0.01f, 0.2f, "%.3f", 0)) {
            // Scale changed, update will be called automatically
        }

        // Line width control
        igSliderFloat("Line Width", &state->gcode->line_width, 0.005f, 0.1f, "%.3f", 0);

        // Miter angle limit (for round joins)
        igSliderFloat("Miter Limit", &state->gcode->miter_angle_limit, 90.0f, 180.0f, "%.0f deg", 0);
        if (igIsItemHovered(0)) {
            igSetTooltip("Above this angle, joins become semicircles");
        }

        // Debug colors checkbox
        igCheckbox("Debug Colors", &state->gcode->debug_colors);

        igSpacing();

        // Color picker
        if (igColorEdit3("Path Color", state->color, ImGuiColorEditFlags_None)) {
            state->gcode->color_r = state->color[0];
            state->gcode->color_g = state->color[1];
            state->gcode->color_b = state->color[2];
        }

        igSpacing();
        igSeparatorText("Timeline");

        // Timeline position slider
        if (igSliderFloat("Position", &state->timeline_percent, 0.0f, 100.0f, "%.1f%%", 0)) {
            state->gcode->timeline_position = state->timeline_percent / 100.0f;
        }

        // Base alpha slider
        if (igSliderFloat("Base Alpha", &state->base_alpha_percent, 0.0f, 100.0f, "%.0f%%", 0)) {
            state->gcode->base_alpha = state->base_alpha_percent / 100.0f;
        }

        // Fade width slider
        if (igSliderFloat("Fade Width", &state->fade_width_percent, 0.5f, 50.0f, "%.1f%%", 0)) {
            state->gcode->highlight_width = state->fade_width_percent / 100.0f;
        }

        igSpacing();

        // Quick preset buttons
        igText("Presets:");
        igSameLine(0, -1);
        if (igSmallButton("Start")) {
            state->timeline_percent = 0.0f;
            state->gcode->timeline_position = 0.0f;
        }
        igSameLine(0, -1);
        if (igSmallButton("25%%")) {
            state->timeline_percent = 25.0f;
            state->gcode->timeline_position = 0.25f;
        }
        igSameLine(0, -1);
        if (igSmallButton("50%%")) {
            state->timeline_percent = 50.0f;
            state->gcode->timeline_position = 0.5f;
        }
        igSameLine(0, -1);
        if (igSmallButton("75%%")) {
            state->timeline_percent = 75.0f;
            state->gcode->timeline_position = 0.75f;
        }
        igSameLine(0, -1);
        if (igSmallButton("End")) {
            state->timeline_percent = 100.0f;
            state->gcode->timeline_position = 1.0f;
        }
    } else {
        igTextColored((ImVec4){1.0f, 0.5f, 0.5f, 1.0f}, "G-code path not loaded");
    }

    // ===== Alpha Lines Section =====
    if (state->alpha_lines) {
        igSpacing();
        igSeparatorText("Alpha-Blended Lines");

        igText("Lines: %d", ALPHA_LINES_COUNT);

        // Line width
        igSliderFloat("AL Line Width", &state->alpha_lines->line_width, 0.01f, 0.1f, "%.3f", 0);

        // Sphere radius
        igSliderFloat("Sphere Radius", &state->alpha_lines->sphere_radius, 1.0f, 10.0f, "%.1f", 0);

        // Line length
        igSliderFloat("Line Length", &state->alpha_lines->line_length, 0.05f, 1.0f, "%.2f", 0);

        // Wiggle amplitude
        igSliderFloat("Wiggle Amp", &state->alpha_lines->wiggle_amplitude, 0.0f, 1.0f, "%.2f", 0);

        // Wiggle frequency
        igSliderFloat("Wiggle Freq", &state->alpha_lines->wiggle_frequency, 0.1f, 5.0f, "%.1f", 0);

        // Base alpha
        float alpha_percent = state->alpha_lines->base_alpha * 100.0f;
        if (igSliderFloat("AL Base Alpha", &alpha_percent, 10.0f, 100.0f, "%.0f%%", 0)) {
            state->alpha_lines->base_alpha = alpha_percent / 100.0f;
            alpha_lines_regenerate_colors(state->alpha_lines);  // Regenerate with new alpha
        }

        igSpacing();

        // Regeneration buttons
        if (igButton("Regenerate All", (ImVec2){0, 0})) {
            alpha_lines_regenerate_all(state->alpha_lines);
        }
        igSameLine(0, -1);
        if (igButton("Colors", (ImVec2){0, 0})) {
            alpha_lines_regenerate_colors(state->alpha_lines);
        }
        igSameLine(0, -1);
        if (igButton("Positions", (ImVec2){0, 0})) {
            alpha_lines_regenerate_positions(state->alpha_lines);
        }
    }

    igEnd();
}

#endif // UI_GCODE_CONTROLS_H
