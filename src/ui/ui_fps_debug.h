//------------------------------------------------------------------------------
// ui_fps_debug.h - FPS counter debug window with rolling plot (header-only)
//
// Displays live FPS counter with a rolling plot of FPS history.
//------------------------------------------------------------------------------
#ifndef UI_FPS_DEBUG_H
#define UI_FPS_DEBUG_H

#include "../platform.h"
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"
#include "sokol_app.h"
#include "sokol_time.h"
#include <string.h>

//------------------------------------------------------------------------------
// Configuration
//------------------------------------------------------------------------------
#define FPS_DEBUG_SAMPLE_COUNT     100      // Number of samples to display
#define FPS_DEBUG_SAMPLE_INTERVAL  2.0f     // Seconds between samples

//------------------------------------------------------------------------------
// Types
//------------------------------------------------------------------------------

typedef struct {
    bool window_open;
    float fps_samples[FPS_DEBUG_SAMPLE_COUNT];
    int sample_index;           // Next sample position (circular buffer)
    int sample_count;           // Total samples collected (up to FPS_DEBUG_SAMPLE_COUNT)
    float time_accumulator;     // Time since last sample
    float current_fps;          // Most recent FPS value
    float max_fps;              // Max FPS in current samples (for autoscale)
} ui_fps_debug_state_t;

//------------------------------------------------------------------------------
// Initialization
//------------------------------------------------------------------------------

static inline void ui_fps_debug_init(ui_fps_debug_state_t *state) {
    state->window_open = true;  // Open by default
    memset(state->fps_samples, 0, sizeof(state->fps_samples));
    state->sample_index = 0;
    state->sample_count = 0;
    state->time_accumulator = 0.0f;
    state->current_fps = 0.0f;
    state->max_fps = 60.0f;  // Initial scale
}

//------------------------------------------------------------------------------
// Update (call every frame)
//------------------------------------------------------------------------------

static inline void ui_fps_debug_update(ui_fps_debug_state_t *state) {
    // Get frame duration from sokol_app
    float frame_duration = (float)sapp_frame_duration();
    
    // Calculate instantaneous FPS
    if (frame_duration > 0.0f) {
        state->current_fps = 1.0f / frame_duration;
    }
    
    // Accumulate time for sampling
    state->time_accumulator += frame_duration;
    
    // Take a sample every FPS_DEBUG_SAMPLE_INTERVAL seconds
    if (state->time_accumulator >= FPS_DEBUG_SAMPLE_INTERVAL) {
        state->time_accumulator = 0.0f;
        
        // Store sample in circular buffer
        state->fps_samples[state->sample_index] = state->current_fps;
        state->sample_index = (state->sample_index + 1) % FPS_DEBUG_SAMPLE_COUNT;
        if (state->sample_count < FPS_DEBUG_SAMPLE_COUNT) {
            state->sample_count++;
        }
        
        // Recalculate max FPS for autoscaling
        state->max_fps = 1.0f;  // Minimum to avoid division by zero
        for (int i = 0; i < state->sample_count; i++) {
            if (state->fps_samples[i] > state->max_fps) {
                state->max_fps = state->fps_samples[i];
            }
        }
        // Add 10% headroom
        state->max_fps *= 1.1f;
    }
}

//------------------------------------------------------------------------------
// Draw
//------------------------------------------------------------------------------

static inline void ui_fps_debug_draw(ui_fps_debug_state_t *state) {
    if (!state->window_open) return;

    // Window flags for clean appearance
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
    
    igSetNextWindowSize((ImVec2_c){300, 200}, ImGuiCond_FirstUseEver);
    
    if (igBegin("FPS Debug", &state->window_open, flags)) {
        // Get content region for plot sizing
        ImVec2_c content_avail = igGetContentRegionAvail();
        float plot_width = content_avail.x;
        float plot_height = content_avail.y;
        
        // Build samples array in display order (oldest to newest)
        float display_samples[FPS_DEBUG_SAMPLE_COUNT];
        int display_count = state->sample_count;
        
        if (display_count > 0) {
            // Circular buffer: oldest sample is at sample_index (if full)
            // or at 0 (if not yet full)
            int oldest_index = (state->sample_count == FPS_DEBUG_SAMPLE_COUNT) 
                             ? state->sample_index 
                             : 0;
            
            for (int i = 0; i < display_count; i++) {
                int src_index = (oldest_index + i) % FPS_DEBUG_SAMPLE_COUNT;
                display_samples[i] = state->fps_samples[src_index];
            }
        }
        
        // Get cursor position before drawing plot (for overlay positioning)
        ImVec2_c plot_pos = igGetCursorScreenPos();
        
        // Draw the plot filling the window
        ImVec2_c plot_size = { plot_width, plot_height };
        igPlotLines_FloatPtr(
            "##fps_plot",       // Label (hidden with ##)
            display_samples,
            display_count,
            0,                  // values_offset
            NULL,               // overlay_text (we'll draw our own)
            0.0f,               // scale_min
            state->max_fps,     // scale_max (autoscaled)
            plot_size,
            sizeof(float)
        );
        
        // Draw FPS counter overlaid at top-middle of plot
        // Format FPS string
        char fps_str[32];
        snprintf(fps_str, sizeof(fps_str), "%.1f FPS", state->current_fps);
        
        // Calculate text size for centering
        ImVec2_c text_size = igCalcTextSize(fps_str, NULL, false, 0.0f);
        
        ImVec2_c text_pos = {
            plot_pos.x + plot_width * 0.5f - text_size.x * 0.5f,
            plot_pos.y + 5.0f
        };
        
        // Draw background for readability
        ImDrawList* draw_list = igGetWindowDrawList();
        ImVec2_c bg_min = { text_pos.x - 4, text_pos.y - 2 };
        ImVec2_c bg_max = { text_pos.x + text_size.x + 4, text_pos.y + text_size.y + 2 };
        ImDrawList_AddRectFilled(draw_list, bg_min, bg_max, 
            igColorConvertFloat4ToU32((ImVec4_c){0.1f, 0.1f, 0.1f, 0.8f}), 4.0f, 0);
        
        // Draw FPS text
        ImDrawList_AddText_Vec2(draw_list, text_pos, 
            igColorConvertFloat4ToU32((ImVec4_c){0.0f, 1.0f, 0.3f, 1.0f}), 
            fps_str, NULL);
    }
    igEnd();
}

#endif // UI_FPS_DEBUG_H
