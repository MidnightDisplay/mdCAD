//------------------------------------------------------------------------------
// renderable_comp.h - Renderable component for ECS entities
//------------------------------------------------------------------------------
#ifndef RENDERABLE_COMP_H
#define RENDERABLE_COMP_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    bool visible;               // Is entity visible
    int layer;                  // Render layer (for ordering)
    uint32_t batch_id;          // Which GPU batch this belongs to
    uint32_t instance_slot;     // Slot index in the instance buffer (or first segment slot)
    bool instance_dirty;        // Needs GPU buffer update

    // Multi-slot support for polylines/arcs/beziers/etc
    uint32_t segment_count;     // Number of segment slots used (0 for point, 1 for line)
    uint32_t join_slot_start;   // Starting slot for joins in point batch
    uint32_t join_count;        // Number of join slots (N-2 for polyline with N points)
} RenderableComp;

// Default renderable (visible, layer 0, no batch assigned)
static inline RenderableComp renderable_comp_default(void) {
    return (RenderableComp){
        .visible = true,
        .layer = 0,
        .batch_id = 0,
        .instance_slot = 0xFFFFFFFF,  // Invalid slot
        .instance_dirty = true,
        .segment_count = 0,
        .join_slot_start = 0xFFFFFFFF,
        .join_count = 0
    };
}

// Mark renderable as needing GPU update
static inline void renderable_comp_mark_dirty(RenderableComp *r) {
    r->instance_dirty = true;
}

#endif // RENDERABLE_COMP_H
