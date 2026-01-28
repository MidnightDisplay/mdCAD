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
    uint32_t instance_slot;     // Slot index in the instance buffer
    bool instance_dirty;        // Needs GPU buffer update
} RenderableComp;

// Default renderable (visible, layer 0, no batch assigned)
static inline RenderableComp renderable_comp_default(void) {
    return (RenderableComp){
        .visible = true,
        .layer = 0,
        .batch_id = 0,
        .instance_slot = 0xFFFFFFFF,  // Invalid slot
        .instance_dirty = true
    };
}

// Mark renderable as needing GPU update
static inline void renderable_comp_mark_dirty(RenderableComp *r) {
    r->instance_dirty = true;
}

#endif // RENDERABLE_COMP_H
