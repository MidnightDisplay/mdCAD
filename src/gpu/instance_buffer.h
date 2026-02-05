//------------------------------------------------------------------------------
// instance_buffer.h - Dynamic GPU instance buffer with slot allocation (header-only)
//
// Manages a dynamic GPU buffer for instanced rendering with:
// - Slot allocation/deallocation with free list recycling
// - Capacity-doubling growth strategy
// - CPU staging buffer for efficient updates
// - Dirty tracking and per-frame upload
//------------------------------------------------------------------------------
#ifndef INSTANCE_BUFFER_H
#define INSTANCE_BUFFER_H

#include "../platform.h"
#include "sokol_gfx.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

//------------------------------------------------------------------------------
// Configuration
//------------------------------------------------------------------------------
#define INSTANCE_BUFFER_INITIAL_CAPACITY  64
#define INSTANCE_BUFFER_MAX_CAPACITY      (1 << 21)  // 2M instances max (~56MB per buffer type)

//------------------------------------------------------------------------------
// Types
//------------------------------------------------------------------------------

typedef struct {
    // GPU buffer
    sg_buffer gpu_buffer;

    // CPU staging data
    void *staging;              // CPU-side staging array
    size_t instance_size;       // Size of each instance in bytes

    // Capacity management
    int capacity;               // Max instances (power of 2)
    int count;                  // Current used count (highest slot + 1)

    // Slot management (free list stack)
    int *free_slots;            // Stack of free slot indices
    int free_count;             // Number of free slots in stack
    int free_capacity;          // Capacity of free_slots array

    // Slot-to-entity mapping (for debug viewer)
    uint64_t *slot_to_entity;   // Entity ID per slot (0 = empty)
    uint8_t *slot_geom_type;    // Geometry type per slot (for color coding)

    // Dirty tracking
    bool needs_upload;          // Buffer needs GPU upload this frame
    int dirty_min;              // Minimum dirty slot index
    int dirty_max;              // Maximum dirty slot index (inclusive)

    // Debug label
    const char *label;
} instance_buffer_t;

//------------------------------------------------------------------------------
// Initialization
//------------------------------------------------------------------------------

static inline void instance_buffer_init(instance_buffer_t *ib,
                                         size_t instance_size,
                                         int initial_capacity,
                                         const char *label) {
    if (initial_capacity < INSTANCE_BUFFER_INITIAL_CAPACITY) {
        initial_capacity = INSTANCE_BUFFER_INITIAL_CAPACITY;
    }

    // Round up to power of 2
    int cap = 1;
    while (cap < initial_capacity) cap *= 2;

    ib->instance_size = instance_size;
    ib->capacity = cap;
    ib->count = 0;
    ib->label = label;

    // Allocate CPU staging buffer
    ib->staging = malloc(cap * instance_size);
    memset(ib->staging, 0, cap * instance_size);

    // Initialize free list (all slots initially free)
    ib->free_capacity = cap;
    ib->free_slots = (int*)malloc(cap * sizeof(int));
    ib->free_count = 0;  // No free slots yet (slots allocated sequentially)

    // Initialize slot-to-entity mapping (for debug viewer)
    ib->slot_to_entity = (uint64_t*)malloc(cap * sizeof(uint64_t));
    ib->slot_geom_type = (uint8_t*)malloc(cap * sizeof(uint8_t));
    memset(ib->slot_to_entity, 0, cap * sizeof(uint64_t));
    memset(ib->slot_geom_type, 0, cap * sizeof(uint8_t));

    // Dirty tracking
    ib->needs_upload = false;
    ib->dirty_min = cap;
    ib->dirty_max = -1;

    // Create GPU buffer with stream update for per-frame updates
    ib->gpu_buffer = sg_make_buffer(&(sg_buffer_desc){
        .usage.vertex_buffer = true,
        .usage.stream_update = true,
        .size = cap * instance_size,
        .label = label
    });
}

//------------------------------------------------------------------------------
// Slot Allocation
//------------------------------------------------------------------------------

// Allocate a slot, returns slot index (grows buffer if needed)
static inline int instance_buffer_alloc_slot(instance_buffer_t *ib) {
    // If all allocated slots have been freed, reset to start fresh
    // This allows efficient reuse after clearing the scene
    if (ib->free_count > 0 && ib->free_count == ib->count) {
        ib->count = 0;
        ib->free_count = 0;
    }

    // First try free list
    if (ib->free_count > 0) {
        return ib->free_slots[--ib->free_count];
    }

    // Allocate new slot at end
    int slot = ib->count;

    // Grow if needed
    if (slot >= ib->capacity) {
        int new_capacity = ib->capacity * 2;
        if (new_capacity > INSTANCE_BUFFER_MAX_CAPACITY) {
            // Buffer full - return invalid slot
            return -1;
        }

        // Grow CPU staging buffer
        void *new_staging = malloc(new_capacity * ib->instance_size);
        memcpy(new_staging, ib->staging, ib->capacity * ib->instance_size);
        memset((char*)new_staging + ib->capacity * ib->instance_size, 0,
               (new_capacity - ib->capacity) * ib->instance_size);
        free(ib->staging);
        ib->staging = new_staging;

        // Destroy old GPU buffer, create new
        sg_destroy_buffer(ib->gpu_buffer);
        ib->gpu_buffer = sg_make_buffer(&(sg_buffer_desc){
            .usage.vertex_buffer = true,
            .usage.stream_update = true,
            .size = new_capacity * ib->instance_size,
            .label = ib->label
        });

        // Grow free list capacity
        ib->free_slots = (int*)realloc(ib->free_slots, new_capacity * sizeof(int));
        ib->free_capacity = new_capacity;

        // Grow slot-to-entity mapping
        ib->slot_to_entity = (uint64_t*)realloc(ib->slot_to_entity, new_capacity * sizeof(uint64_t));
        ib->slot_geom_type = (uint8_t*)realloc(ib->slot_geom_type, new_capacity * sizeof(uint8_t));
        memset(ib->slot_to_entity + ib->capacity, 0, (new_capacity - ib->capacity) * sizeof(uint64_t));
        memset(ib->slot_geom_type + ib->capacity, 0, (new_capacity - ib->capacity) * sizeof(uint8_t));

        ib->capacity = new_capacity;

        // Mark entire buffer dirty after resize (need full upload)
        ib->dirty_min = 0;
        ib->dirty_max = ib->count > 0 ? ib->count - 1 : 0;
        ib->needs_upload = true;
    }

    ib->count++;
    return slot;
}

// Allocate n contiguous slots (for multi-slot entities like polylines)
// Skips free list to guarantee contiguous allocation
// Returns first slot index, or -1 if buffer full
static inline int instance_buffer_alloc_contiguous(instance_buffer_t *ib, int n) {
    if (n <= 0) return -1;

    // If all allocated slots have been freed, reset to start fresh
    // This allows efficient reuse after clearing the scene
    if (ib->free_count > 0 && ib->free_count == ib->count) {
        ib->count = 0;
        ib->free_count = 0;
    }

    int first_slot = ib->count;

    // Grow if needed
    while (first_slot + n > ib->capacity) {
        int new_capacity = ib->capacity * 2;
        if (new_capacity > INSTANCE_BUFFER_MAX_CAPACITY) {
            return -1;  // Buffer full
        }

        // Grow CPU staging buffer
        void *new_staging = malloc(new_capacity * ib->instance_size);
        memcpy(new_staging, ib->staging, ib->capacity * ib->instance_size);
        memset((char*)new_staging + ib->capacity * ib->instance_size, 0,
               (new_capacity - ib->capacity) * ib->instance_size);
        free(ib->staging);
        ib->staging = new_staging;

        // Destroy old GPU buffer, create new
        sg_destroy_buffer(ib->gpu_buffer);
        ib->gpu_buffer = sg_make_buffer(&(sg_buffer_desc){
            .usage.vertex_buffer = true,
            .usage.stream_update = true,
            .size = new_capacity * ib->instance_size,
            .label = ib->label
        });

        // Grow free list capacity
        ib->free_slots = (int*)realloc(ib->free_slots, new_capacity * sizeof(int));
        ib->free_capacity = new_capacity;

        // Grow slot-to-entity mapping
        ib->slot_to_entity = (uint64_t*)realloc(ib->slot_to_entity, new_capacity * sizeof(uint64_t));
        ib->slot_geom_type = (uint8_t*)realloc(ib->slot_geom_type, new_capacity * sizeof(uint8_t));
        memset(ib->slot_to_entity + ib->capacity, 0, (new_capacity - ib->capacity) * sizeof(uint64_t));
        memset(ib->slot_geom_type + ib->capacity, 0, (new_capacity - ib->capacity) * sizeof(uint8_t));

        ib->capacity = new_capacity;

        // Mark entire buffer dirty after resize
        ib->dirty_min = 0;
        ib->dirty_max = ib->count > 0 ? ib->count - 1 : 0;
        ib->needs_upload = true;
    }

    ib->count += n;
    return first_slot;
}

// Free a slot for reuse
static inline void instance_buffer_free_slot(instance_buffer_t *ib, int slot) {
    if (slot < 0 || slot >= ib->count) return;

    // Add to free list
    if (ib->free_count >= ib->free_capacity) {
        ib->free_capacity = ib->free_capacity * 2;
        ib->free_slots = (int*)realloc(ib->free_slots, ib->free_capacity * sizeof(int));
    }
    ib->free_slots[ib->free_count++] = slot;

    // Clear entity mapping
    ib->slot_to_entity[slot] = 0;
    ib->slot_geom_type[slot] = 0;

    // Set slot data to far-away values so freed instances render off-screen
    // (GPU frustum culling makes this essentially free)
    // Using a large float value that won't cause precision issues
    float *slot_data = (float*)((char*)ib->staging + slot * ib->instance_size);
    size_t num_floats = ib->instance_size / sizeof(float);
    for (size_t i = 0; i < num_floats; i++) {
        slot_data[i] = 1e10f;
    }

    // Mark slot dirty so the zeroed data gets uploaded
    if (slot < ib->dirty_min) ib->dirty_min = slot;
    if (slot > ib->dirty_max) ib->dirty_max = slot;
    ib->needs_upload = true;
}

//------------------------------------------------------------------------------
// Data Updates
//------------------------------------------------------------------------------

// Set instance data at slot
static inline void instance_buffer_set(instance_buffer_t *ib, int slot, const void *data) {
    if (slot < 0 || slot >= ib->count) return;

    memcpy((char*)ib->staging + slot * ib->instance_size, data, ib->instance_size);

    // Track dirty range
    if (slot < ib->dirty_min) ib->dirty_min = slot;
    if (slot > ib->dirty_max) ib->dirty_max = slot;
    ib->needs_upload = true;
}

// Get pointer to instance data at slot (for direct modification)
static inline void* instance_buffer_get(instance_buffer_t *ib, int slot) {
    if (slot < 0 || slot >= ib->count) return NULL;
    return (char*)ib->staging + slot * ib->instance_size;
}

// Mark a slot as dirty (after direct modification via instance_buffer_get)
static inline void instance_buffer_mark_dirty(instance_buffer_t *ib, int slot) {
    if (slot < 0 || slot >= ib->count) return;

    if (slot < ib->dirty_min) ib->dirty_min = slot;
    if (slot > ib->dirty_max) ib->dirty_max = slot;
    ib->needs_upload = true;
}

//------------------------------------------------------------------------------
// Slot-to-Entity Mapping (for debug viewer)
//------------------------------------------------------------------------------

// Set entity ID and geometry type for a slot
static inline void instance_buffer_set_entity(instance_buffer_t *ib, int slot,
                                               uint64_t entity_id, uint8_t geom_type) {
    if (slot < 0 || slot >= ib->count) return;
    ib->slot_to_entity[slot] = entity_id;
    ib->slot_geom_type[slot] = geom_type;
}

// Get entity ID for a slot (returns 0 if empty/invalid)
static inline uint64_t instance_buffer_get_entity(instance_buffer_t *ib, int slot) {
    if (slot < 0 || slot >= ib->count) return 0;
    return ib->slot_to_entity[slot];
}

// Get geometry type for a slot
static inline uint8_t instance_buffer_get_geom_type(instance_buffer_t *ib, int slot) {
    if (slot < 0 || slot >= ib->count) return 0;
    return ib->slot_geom_type[slot];
}

// Check if slot is occupied (has an entity)
static inline bool instance_buffer_slot_occupied(instance_buffer_t *ib, int slot) {
    if (slot < 0 || slot >= ib->count) return false;
    return ib->slot_to_entity[slot] != 0;
}

//------------------------------------------------------------------------------
// GPU Upload
//------------------------------------------------------------------------------

// Upload dirty data to GPU (call once per frame before rendering)
static inline void instance_buffer_upload(instance_buffer_t *ib) {
    if (!ib->needs_upload || ib->count == 0) return;

    // Upload entire buffer (Sokol stream buffers require full update)
    sg_update_buffer(ib->gpu_buffer, &(sg_range){
        .ptr = ib->staging,
        .size = ib->count * ib->instance_size
    });

    // Reset dirty tracking
    ib->needs_upload = false;
    ib->dirty_min = ib->capacity;
    ib->dirty_max = -1;
}

//------------------------------------------------------------------------------
// Clear (reset buffer to empty state without reallocating)
//------------------------------------------------------------------------------

static inline void instance_buffer_clear(instance_buffer_t *ib) {
    // Reset count and free list
    ib->count = 0;
    ib->free_count = 0;

    // Clear dirty tracking
    ib->needs_upload = false;
    ib->dirty_min = ib->capacity;
    ib->dirty_max = -1;

    // Optionally zero out staging buffer (helps with debugging)
    memset(ib->staging, 0, ib->capacity * ib->instance_size);

    // Clear slot-to-entity mapping
    memset(ib->slot_to_entity, 0, ib->capacity * sizeof(uint64_t));
    memset(ib->slot_geom_type, 0, ib->capacity * sizeof(uint8_t));
}

//------------------------------------------------------------------------------
// Queries
//------------------------------------------------------------------------------

static inline int instance_buffer_count(instance_buffer_t *ib) {
    return ib->count;
}

static inline int instance_buffer_capacity(instance_buffer_t *ib) {
    return ib->capacity;
}

static inline int instance_buffer_max_capacity(void) {
    return INSTANCE_BUFFER_MAX_CAPACITY;
}

// Check if buffer is at maximum capacity (cannot grow further)
static inline bool instance_buffer_is_at_max(instance_buffer_t *ib) {
    return ib->capacity >= INSTANCE_BUFFER_MAX_CAPACITY;
}

// Check if allocating n more slots would exceed max capacity
static inline bool instance_buffer_would_exceed_max(instance_buffer_t *ib, int n) {
    // Account for free slots that can be reused
    int effective_count = ib->count - ib->free_count;
    return (effective_count + n) > INSTANCE_BUFFER_MAX_CAPACITY;
}

static inline sg_buffer instance_buffer_gpu_buffer(instance_buffer_t *ib) {
    return ib->gpu_buffer;
}

static inline bool instance_buffer_is_valid_slot(instance_buffer_t *ib, int slot) {
    return slot >= 0 && slot < ib->count;
}

//------------------------------------------------------------------------------
// Shutdown
//------------------------------------------------------------------------------

static inline void instance_buffer_shutdown(instance_buffer_t *ib) {
    if (ib->staging) {
        free(ib->staging);
        ib->staging = NULL;
    }

    if (ib->free_slots) {
        free(ib->free_slots);
        ib->free_slots = NULL;
    }

    if (ib->slot_to_entity) {
        free(ib->slot_to_entity);
        ib->slot_to_entity = NULL;
    }

    if (ib->slot_geom_type) {
        free(ib->slot_geom_type);
        ib->slot_geom_type = NULL;
    }

    sg_destroy_buffer(ib->gpu_buffer);

    ib->count = 0;
    ib->capacity = 0;
    ib->free_count = 0;
}

#endif // INSTANCE_BUFFER_H
