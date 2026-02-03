//------------------------------------------------------------------------------
// ui_slot_buffer_debug.h - Entity slot buffer debug viewer (header-only)
//
// Displays a grid view of instance buffer slots with color coding by entity type.
// Shows entity info in tooltips when hovering over slots.
//------------------------------------------------------------------------------
#ifndef UI_SLOT_BUFFER_DEBUG_H
#define UI_SLOT_BUFFER_DEBUG_H

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"
#include "../ecs/ecs_scene.h"
#include "../gpu/instance_buffer.h"
#include "../components/geometry_comp.h"

//------------------------------------------------------------------------------
// Configuration
//------------------------------------------------------------------------------

#define SLOT_DEBUG_SIZE_SMALL   10
#define SLOT_DEBUG_SIZE_MEDIUM  20
#define SLOT_DEBUG_SIZE_LARGE   40

#define SLOT_DEBUG_GAP          1   // Gap between cells in pixels

// Slots per page based on cell size
#define SLOT_DEBUG_PAGE_SMALL   256
#define SLOT_DEBUG_PAGE_MEDIUM  128
#define SLOT_DEBUG_PAGE_LARGE   64

//------------------------------------------------------------------------------
// Slot type for color coding (extensible)
//------------------------------------------------------------------------------

typedef enum {
    SLOT_TYPE_EMPTY = 0,
    SLOT_TYPE_POINT,
    SLOT_TYPE_LINE,
    SLOT_TYPE_POLYLINE_SEGMENT,
    SLOT_TYPE_ARC_SEGMENT,
    SLOT_TYPE_BEZIER_SEGMENT,
    SLOT_TYPE_HELIX_SEGMENT,
    SLOT_TYPE_POLYGON_SEGMENT,
    SLOT_TYPE_JOIN,
    SLOT_TYPE_COUNT
} slot_type_t;

//------------------------------------------------------------------------------
// Types
//------------------------------------------------------------------------------

typedef struct {
    bool window_open;
    int current_tab;           // 0=lines, 1=points
    int cell_size_preset;      // 0=small, 1=medium, 2=large
    int cell_size;             // Actual size (5-50 range)
    int page_lines;            // Current page for lines tab
    int page_points;           // Current page for points tab

    // Pointer to ECS scene (for accessing batches and entity data)
    ecs_scene_t *scene;
} ui_slot_buffer_debug_state_t;

//------------------------------------------------------------------------------
// Color mapping (extensible via lookup table)
//------------------------------------------------------------------------------

// Helper macro for RGBA32 colors (cimgui doesn't have IM_COL32)
#define SLOT_COL32(R, G, B, A) ((ImU32)(((A)&0xFF)<<24) | (((B)&0xFF)<<16) | (((G)&0xFF)<<8) | ((R)&0xFF))

static inline ImU32 slot_debug_get_color(slot_type_t type) {
    switch (type) {
        case SLOT_TYPE_EMPTY:           return SLOT_COL32(26, 26, 26, 255);       // Dark gray #1a1a1a
        case SLOT_TYPE_POINT:           return SLOT_COL32(255, 68, 68, 255);      // Red #ff4444
        case SLOT_TYPE_LINE:            return SLOT_COL32(68, 255, 68, 255);      // Green #44ff44
        case SLOT_TYPE_POLYLINE_SEGMENT:return SLOT_COL32(68, 136, 255, 255);     // Blue #4488ff
        case SLOT_TYPE_ARC_SEGMENT:     return SLOT_COL32(68, 136, 255, 255);     // Blue #4488ff
        case SLOT_TYPE_BEZIER_SEGMENT:  return SLOT_COL32(68, 136, 255, 255);     // Blue #4488ff
        case SLOT_TYPE_HELIX_SEGMENT:   return SLOT_COL32(68, 136, 255, 255);     // Blue #4488ff
        case SLOT_TYPE_POLYGON_SEGMENT: return SLOT_COL32(255, 255, 68, 255);     // Yellow #ffff44
        case SLOT_TYPE_JOIN:            return SLOT_COL32(68, 255, 255, 255);     // Cyan #44ffff
        default:                        return SLOT_COL32(26, 26, 26, 255);
    }
}

static inline const char* slot_debug_get_type_name(slot_type_t type) {
    switch (type) {
        case SLOT_TYPE_EMPTY:           return "Empty";
        case SLOT_TYPE_POINT:           return "Point";
        case SLOT_TYPE_LINE:            return "Line";
        case SLOT_TYPE_POLYLINE_SEGMENT:return "Polyline Seg";
        case SLOT_TYPE_ARC_SEGMENT:     return "Arc Seg";
        case SLOT_TYPE_BEZIER_SEGMENT:  return "Bezier Seg";
        case SLOT_TYPE_HELIX_SEGMENT:   return "Helix Seg";
        case SLOT_TYPE_POLYGON_SEGMENT: return "Polygon Seg";
        case SLOT_TYPE_JOIN:            return "Join";
        default:                        return "Unknown";
    }
}

// Map geometry_type_t to slot_type_t for line buffer
static inline slot_type_t slot_debug_geom_to_slot_type_line(uint8_t geom_type) {
    switch ((geometry_type_t)geom_type) {
        case GEOM_LINE:     return SLOT_TYPE_LINE;
        case GEOM_POLYLINE: return SLOT_TYPE_POLYLINE_SEGMENT;
        case GEOM_ARC:      return SLOT_TYPE_ARC_SEGMENT;
        case GEOM_BEZIER:   return SLOT_TYPE_BEZIER_SEGMENT;
        case GEOM_HELIX:    return SLOT_TYPE_HELIX_SEGMENT;
        case GEOM_POLYGON:  return SLOT_TYPE_POLYGON_SEGMENT;
        default:            return SLOT_TYPE_EMPTY;
    }
}

// Map geometry_type_t to slot_type_t for point buffer
static inline slot_type_t slot_debug_geom_to_slot_type_point(uint8_t geom_type) {
    switch ((geometry_type_t)geom_type) {
        case GEOM_POINT:    return SLOT_TYPE_POINT;
        case GEOM_POLYLINE: return SLOT_TYPE_JOIN;
        case GEOM_ARC:      return SLOT_TYPE_JOIN;
        case GEOM_BEZIER:   return SLOT_TYPE_JOIN;
        case GEOM_HELIX:    return SLOT_TYPE_JOIN;
        case GEOM_POLYGON:  return SLOT_TYPE_JOIN;
        default:            return SLOT_TYPE_EMPTY;
    }
}

//------------------------------------------------------------------------------
// Functions
//------------------------------------------------------------------------------

static inline void ui_slot_buffer_debug_init(ui_slot_buffer_debug_state_t* state, ecs_scene_t* scene) {
    state->window_open = true;
    state->current_tab = 0;
    state->cell_size_preset = 1;  // Medium by default
    state->cell_size = SLOT_DEBUG_SIZE_MEDIUM;
    state->page_lines = 0;
    state->page_points = 0;
    state->scene = scene;
}

static inline int slot_debug_get_slots_per_page(int cell_size) {
    if (cell_size <= 15) return SLOT_DEBUG_PAGE_SMALL;
    if (cell_size <= 25) return SLOT_DEBUG_PAGE_MEDIUM;
    return SLOT_DEBUG_PAGE_LARGE;
}

// Draw a single buffer tab
static inline void ui_slot_buffer_debug_draw_buffer(
    ui_slot_buffer_debug_state_t* state,
    instance_buffer_t* buffer,
    int* page,
    bool is_point_buffer  // true for points, false for lines
) {
    int total_slots = buffer->count;
    int slots_per_page = slot_debug_get_slots_per_page(state->cell_size);
    int total_pages = (total_slots + slots_per_page - 1) / slots_per_page;
    if (total_pages < 1) total_pages = 1;

    // Clamp page to valid range
    if (*page >= total_pages) *page = total_pages - 1;
    if (*page < 0) *page = 0;

    // Buffer stats
    igText("Total slots: %d  |  Capacity: %d  |  Free: %d",
           buffer->count, buffer->capacity, buffer->free_count);
    igSeparator();

    // Page navigation row
    igBeginDisabled(*page <= 0);
    if (igButton("|<", (ImVec2){25, 0})) *page = 0;
    igSameLine(0, 2);
    if (igButton("<", (ImVec2){25, 0})) (*page)--;
    igEndDisabled();

    igSameLine(0, 5);

    // Page input
    igSetNextItemWidth(60);
    int display_page = *page + 1;
    if (igInputInt("##page", &display_page, 0, 0, 0)) {
        *page = display_page - 1;
        if (*page < 0) *page = 0;
        if (*page >= total_pages) *page = total_pages - 1;
    }
    igSameLine(0, 2);
    igText("/ %d", total_pages);

    igSameLine(0, 5);

    igBeginDisabled(*page >= total_pages - 1);
    if (igButton(">", (ImVec2){25, 0})) (*page)++;
    igSameLine(0, 2);
    if (igButton(">|", (ImVec2){25, 0})) *page = total_pages - 1;
    igEndDisabled();

    // Page slider
    igSameLine(0, 10);
    igSetNextItemWidth(igGetContentRegionAvail().x);
    int slider_page = *page;
    if (igSliderInt("##pageslider", &slider_page, 0, total_pages - 1, "Page %d", 0)) {
        *page = slider_page;
    }

    igSeparator();

    // Calculate grid dimensions
    ImVec2_c avail = igGetContentRegionAvail();
    int cell_with_gap = state->cell_size + SLOT_DEBUG_GAP;
    int cols = (int)(avail.x / cell_with_gap);
    if (cols < 1) cols = 1;

    // Calculate slot range for current page
    int start_slot = *page * slots_per_page;
    int end_slot = start_slot + slots_per_page;
    if (end_slot > total_slots) end_slot = total_slots;

    // Get draw list and cursor position
    ImDrawList* draw_list = igGetWindowDrawList();
    ImVec2_c cursor_screen = igGetCursorScreenPos();

    // Track mouse position for hover detection
    ImVec2_c mouse_pos = igGetMousePos();

    int hovered_slot = -1;

    // Draw grid
    int row = 0;
    int col = 0;
    for (int slot = start_slot; slot < end_slot; slot++) {
        float x = cursor_screen.x + col * cell_with_gap;
        float y = cursor_screen.y + row * cell_with_gap;

        // Determine slot type and color
        slot_type_t slot_type = SLOT_TYPE_EMPTY;
        uint64_t entity_id = instance_buffer_get_entity(buffer, slot);

        if (entity_id != 0) {
            uint8_t geom_type = instance_buffer_get_geom_type(buffer, slot);
            if (is_point_buffer) {
                slot_type = slot_debug_geom_to_slot_type_point(geom_type);
            } else {
                slot_type = slot_debug_geom_to_slot_type_line(geom_type);
            }
        }

        ImU32 color = slot_debug_get_color(slot_type);

        // Draw cell
        ImVec2 p_min = {x, y};
        ImVec2 p_max = {x + state->cell_size, y + state->cell_size};
        ImDrawList_AddRectFilled(draw_list, p_min, p_max, color, 0.0f, 0);

        // Check hover
        if (mouse_pos.x >= p_min.x && mouse_pos.x < p_max.x &&
            mouse_pos.y >= p_min.y && mouse_pos.y < p_max.y) {
            hovered_slot = slot;
            // Draw hover outline
            ImDrawList_AddRect(draw_list, p_min, p_max, SLOT_COL32(255, 255, 255, 200), 0.0f, 0, 2.0f);
        }

        // Advance to next cell
        col++;
        if (col >= cols) {
            col = 0;
            row++;
        }
    }

    // Reserve space for the grid
    int rows_drawn = (end_slot - start_slot + cols - 1) / cols;
    igDummy((ImVec2){avail.x, rows_drawn * cell_with_gap});

    // Draw tooltip if hovering
    if (hovered_slot >= 0) {
        igBeginTooltip();
        igText("Slot: %d", hovered_slot);

        uint64_t entity_id = instance_buffer_get_entity(buffer, hovered_slot);
        if (entity_id != 0) {
            uint8_t geom_type = instance_buffer_get_geom_type(buffer, hovered_slot);
            slot_type_t slot_type = is_point_buffer
                ? slot_debug_geom_to_slot_type_point(geom_type)
                : slot_debug_geom_to_slot_type_line(geom_type);

            igText("Entity: #%llu", (unsigned long long)entity_id);
            igText("Type: %s", slot_debug_get_type_name(slot_type));

            // Get instance data for position/color info
            void* data = instance_buffer_get(buffer, hovered_slot);
            if (data) {
                if (is_point_buffer) {
                    geom_point_instance_t* pt = (geom_point_instance_t*)data;
                    igText("Position: (%.2f, %.2f, %.2f)", pt->x, pt->y, pt->z);
                    igText("Color: (%.2f, %.2f, %.2f, %.2f)", pt->r, pt->g, pt->b, pt->a);
                } else {
                    geom_line_instance_t* ln = (geom_line_instance_t*)data;
                    igText("Point A: (%.2f, %.2f, %.2f)", ln->ax, ln->ay, ln->az);
                    igText("Point B: (%.2f, %.2f, %.2f)", ln->bx, ln->by, ln->bz);
                    igText("Color: (%.2f, %.2f, %.2f, %.2f)", ln->r, ln->g, ln->b, ln->a);
                }
            }

            // Show parent if entity has one
            ecs_entity_t e = (ecs_entity_t)entity_id;
            if (state->scene && ecs_is_alive(state->scene->world->world, e)) {
                ecs_entity_t parent = scene_get_parent(state->scene, e);
                if (parent != 0) {
                    igText("Parent: #%llu", (unsigned long long)parent);
                }
            }
        } else {
            igTextDisabled("(empty slot)");
        }

        igEndTooltip();
    }
}

static inline void ui_slot_buffer_debug_draw(ui_slot_buffer_debug_state_t* state) {
    if (!state->window_open) return;
    if (!state->scene) return;

    igSetNextWindowSize((ImVec2){400, 500}, ImGuiCond_FirstUseEver);

    if (!igBegin("Slot Buffer Debug", &state->window_open, 0)) {
        igEnd();
        return;
    }

    // Size controls
    igText("Cell Size:");
    igSameLine(0, 5);

    if (igButton("Small", (ImVec2){50, 0})) {
        state->cell_size_preset = 0;
        state->cell_size = SLOT_DEBUG_SIZE_SMALL;
    }
    igSameLine(0, 2);
    if (igButton("Medium", (ImVec2){55, 0})) {
        state->cell_size_preset = 1;
        state->cell_size = SLOT_DEBUG_SIZE_MEDIUM;
    }
    igSameLine(0, 2);
    if (igButton("Large", (ImVec2){50, 0})) {
        state->cell_size_preset = 2;
        state->cell_size = SLOT_DEBUG_SIZE_LARGE;
    }
    igSameLine(0, 10);
    igSetNextItemWidth(100);
    igSliderInt("##cellsize", &state->cell_size, 5, 50, "%d px", 0);

    igSeparator();

    // Color legend (collapsed by default)
    if (igCollapsingHeader_TreeNodeFlags("Color Legend", 0)) {
        ImDrawList* legend_draw_list = igGetWindowDrawList();
        ImVec2_c cursor = igGetCursorScreenPos();

        const int legend_size = 14;
        const int legend_spacing = 120;

        for (int i = 0; i < SLOT_TYPE_COUNT; i++) {
            int col = i % 3;
            int row = i / 3;

            float x = cursor.x + col * legend_spacing;
            float y = cursor.y + row * 20;

            ImVec2 p_min = {x, y};
            ImVec2 p_max = {x + legend_size, y + legend_size};
            ImDrawList_AddRectFilled(legend_draw_list, p_min, p_max, slot_debug_get_color((slot_type_t)i), 0.0f, 0);
            ImDrawList_AddRect(legend_draw_list, p_min, p_max, SLOT_COL32(100, 100, 100, 255), 0.0f, 0, 1.0f);

            // Text label
            igSetCursorScreenPos((ImVec2){x + legend_size + 4, y});
            igText("%s", slot_debug_get_type_name((slot_type_t)i));
        }

        int legend_rows = (SLOT_TYPE_COUNT + 2) / 3;
        igDummy((ImVec2){0, (float)(legend_rows * 20 + 5)});
    }

    // Tabs for different buffers
    if (igBeginTabBar("BufferTabs", 0)) {
        // Lines tab
        if (igBeginTabItem("Lines", NULL, 0)) {
            state->current_tab = 0;
            ui_slot_buffer_debug_draw_buffer(
                state,
                &state->scene->batches.lines.instances,
                &state->page_lines,
                false  // is_point_buffer
            );
            igEndTabItem();
        }

        // Points tab
        if (igBeginTabItem("Points/Joins", NULL, 0)) {
            state->current_tab = 1;
            ui_slot_buffer_debug_draw_buffer(
                state,
                &state->scene->batches.points.instances,
                &state->page_points,
                true  // is_point_buffer
            );
            igEndTabItem();
        }

        igEndTabBar();
    }

    igEnd();
}

#endif // UI_SLOT_BUFFER_DEBUG_H
