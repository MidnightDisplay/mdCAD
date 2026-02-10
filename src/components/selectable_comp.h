//------------------------------------------------------------------------------
// selectable_comp.h - Selectable component for ECS entities
//------------------------------------------------------------------------------
#ifndef SELECTABLE_COMP_H
#define SELECTABLE_COMP_H

#include <stdint.h>
#include <stdbool.h>

//------------------------------------------------------------------------------
// Reserved pick ID ranges (top of 24-bit range for gizmo system)
//------------------------------------------------------------------------------
#define GIZMO_PICK_RESERVED_START  16767200u   // Everything >= this is gizmo territory
#define GIZMO_HANDLE_BASE          16767200u   // 16 IDs: transform gizmo handles (16767200-16767215)
#define GIZMO_HANDLE_COUNT         16
#define VERTEX_HANDLE_BASE         16767216u   // 10000 IDs: vertex handles (16767216-16777215)
#define VERTEX_HANDLE_MAX          10000

typedef struct {
    uint32_t pick_id;           // Unique ID for GPU picking (1-16777215)
    bool pickable;              // Can be picked
} SelectableComp;

// Default selectable (pickable, no ID assigned yet)
static inline SelectableComp selectable_comp_default(void) {
    return (SelectableComp){
        .pick_id = 0,           // 0 = no ID assigned
        .pickable = true
    };
}

// Create selectable with specific pick ID
static inline SelectableComp selectable_comp_with_id(uint32_t pick_id) {
    return (SelectableComp){
        .pick_id = pick_id,
        .pickable = true
    };
}

//------------------------------------------------------------------------------
// Pick ID encoding/decoding for GPU rendering
//------------------------------------------------------------------------------

// Encode entity pick_id to RGB (for rendering to pick buffer)
// ID 0 = no entity (background), valid IDs are 1-16777215
static inline void pick_id_to_rgb(uint32_t id, uint8_t *r, uint8_t *g, uint8_t *b) {
    *r = (uint8_t)((id >> 16) & 0xFF);
    *g = (uint8_t)((id >> 8) & 0xFF);
    *b = (uint8_t)(id & 0xFF);
}

// Encode pick_id to normalized floats (for shader uniform/vertex attribute)
static inline void pick_id_to_rgb_float(uint32_t id, float *r, float *g, float *b) {
    *r = (float)((id >> 16) & 0xFF) / 255.0f;
    *g = (float)((id >> 8) & 0xFF) / 255.0f;
    *b = (float)(id & 0xFF) / 255.0f;
}

// Decode RGB back to pick_id
static inline uint32_t rgb_to_pick_id(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
}

#endif // SELECTABLE_COMP_H
