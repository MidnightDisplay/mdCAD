//------------------------------------------------------------------------------
// ui_pick_debug.h - Pick buffer debug visualization (header-only)
//
// Displays the pick buffer contents scaled up for debugging.
// Shows the currently hovered entity pick ID.
//------------------------------------------------------------------------------
#ifndef UI_PICK_DEBUG_H
#define UI_PICK_DEBUG_H

#include "../platform.h"
#include "../gpu/pick_buffer.h"
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"
#include "sokol_imgui.h"

//------------------------------------------------------------------------------
// Types
//------------------------------------------------------------------------------

typedef struct {
    pick_buffer_t *pick_buffer;
    bool window_open;
} ui_pick_debug_state_t;

//------------------------------------------------------------------------------
// Initialization
//------------------------------------------------------------------------------

static inline void ui_pick_debug_init(ui_pick_debug_state_t *state, pick_buffer_t *pb) {
    state->pick_buffer = pb;
    state->window_open = true;  // Closed by default
}

//------------------------------------------------------------------------------
// Draw
//------------------------------------------------------------------------------

static inline void ui_pick_debug_draw(ui_pick_debug_state_t *state) {
    if (!state->window_open) return;

    if (igBegin("Pick Buffer Debug", &state->window_open, 0)) {
        pick_buffer_t *pb = state->pick_buffer;

        // Display pick buffer texture scaled 2x (100x100 -> 200x200)
        ImVec2_c size = { 200.0f, 200.0f };
        uint64_t tex_id = simgui_imtextureid_with_sampler(pb->tex_view, pb->sampler);
        ImTextureRef_c tex_ref = { ._TexID = tex_id };

        igText("Pick Buffer (%dx%d, 2x zoom):", PICK_BUFFER_SIZE, PICK_BUFFER_SIZE);

        // Get cursor position before drawing image (for crosshair)
        ImVec2_c cursor_pos = igGetCursorScreenPos();

        // Display the pick buffer (Y-flip for OpenGL)
#if defined(SOKOL_GLCORE) || defined(SOKOL_GLES3)
        igImage(tex_ref, size, (ImVec2_c){0, 1}, (ImVec2_c){1, 0});
#else
        igImage(tex_ref, size, (ImVec2_c){0, 0}, (ImVec2_c){1, 1});
#endif

        // Draw crosshair overlay at center
        ImDrawList* draw_list = igGetWindowDrawList();
        float center_x = cursor_pos.x + size.x / 2.0f;
        float center_y = cursor_pos.y + size.y / 2.0f;
        ImU32 crosshair_color = igGetColorU32_Vec4((ImVec4_c){1.0f, 1.0f, 0.0f, 0.8f});

        ImDrawList_AddLine(draw_list,
            (ImVec2_c){center_x - 10, center_y},
            (ImVec2_c){center_x + 10, center_y},
            crosshair_color, 1.0f);
        ImDrawList_AddLine(draw_list,
            (ImVec2_c){center_x, center_y - 10},
            (ImVec2_c){center_x, center_y + 10},
            crosshair_color, 1.0f);

        igSeparator();

        // Show current hover state
        uint32_t hovered_id = pb->hovered_pick_id;
        if (hovered_id == 0) {
            igTextDisabled("Hovered: None (background)");
        } else {
            igText("Hovered Pick ID: %u", hovered_id);

            // Show RGB encoding
            uint8_t r, g, b;
            pick_id_to_rgb(hovered_id, &r, &g, &b);
            igText("  RGB: (%d, %d, %d)", r, g, b);

            // Color preview
            ImVec4_c color = {
                (float)r / 255.0f,
                (float)g / 255.0f,
                (float)b / 255.0f,
                1.0f
            };
            igColorButton("##pick_color", color, 0, (ImVec2_c){20, 20});
            igSameLine(0, 5);
            igText("Pick Color");
        }

        igSeparator();

        // Show pick buffer parameters
        igText("Center: (%.3f, %.3f)", pb->center_x, pb->center_y);
        igText("Viewport: %.0f x %.0f", pb->viewport_width, pb->viewport_height);
        igText("Zoom factor: %.1fx", pb->zoom_factor);
        igText("Scaled line width: %.4f", PICK_BUFFER_LINE_WIDTH * pb->zoom_factor);

        // Instance counts
        int line_count = instance_buffer_count(&pb->line_instances);
        int point_count = instance_buffer_count(&pb->point_instances);
        igText("Lines: %d, Points: %d", line_count, point_count);
    }
    igEnd();
}

#endif // UI_PICK_DEBUG_H
