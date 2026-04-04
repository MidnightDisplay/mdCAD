//------------------------------------------------------------------------------
// selection.h - Entity selection buffer management (header-only)
//
// Manages a buffer of selected entities with support for:
// - Single selection (replace)
// - Multi-selection via Ctrl+click (toggle)
// - Additive selection via Shift+click (add)
//------------------------------------------------------------------------------
#ifndef SELECTION_H
#define SELECTION_H

#include <stdlib.h>
#include <string.h>
#include "ecs/ecs_world.h"  // Already includes flecs.h

//------------------------------------------------------------------------------
// Constants
//------------------------------------------------------------------------------

#define SELECTION_INITIAL_CAPACITY 64

//------------------------------------------------------------------------------
// Types
//------------------------------------------------------------------------------

typedef struct {
    ecs_entity_t *entities;       // Array of selected entities
    int count;                    // Number of selected entities
    int capacity;                 // Allocated capacity

    // Pointer to ECS world for tag management
    ecs_world_state_t *world;
} selection_buffer_t;

//------------------------------------------------------------------------------
// Initialization / Shutdown
//------------------------------------------------------------------------------

static inline void selection_init(selection_buffer_t *sel, ecs_world_state_t *world) {
    sel->entities = (ecs_entity_t*)malloc(SELECTION_INITIAL_CAPACITY * sizeof(ecs_entity_t));
    sel->count = 0;
    sel->capacity = SELECTION_INITIAL_CAPACITY;
    sel->world = world;
}

static inline void selection_shutdown(selection_buffer_t *sel) {
    if (sel->entities) {
        free(sel->entities);
        sel->entities = NULL;
    }
    sel->count = 0;
    sel->capacity = 0;
    sel->world = NULL;
}

//------------------------------------------------------------------------------
// Query Functions
//------------------------------------------------------------------------------

// Get number of selected entities
static inline int selection_count(selection_buffer_t *sel) {
    return sel->count;
}

// Check if an entity is selected
static inline bool selection_contains(selection_buffer_t *sel, ecs_entity_t e) {
    for (int i = 0; i < sel->count; i++) {
        if (sel->entities[i] == e) {
            return true;
        }
    }
    return false;
}

// Get entity at index (returns 0 if out of bounds)
static inline ecs_entity_t selection_get(selection_buffer_t *sel, int index) {
    if (index < 0 || index >= sel->count) return 0;
    return sel->entities[index];
}

//------------------------------------------------------------------------------
// Internal: Grow capacity if needed
//------------------------------------------------------------------------------

static inline void selection_grow_if_needed(selection_buffer_t *sel) {
    if (sel->count >= sel->capacity) {
        sel->capacity *= 2;
        sel->entities = (ecs_entity_t*)realloc(sel->entities,
                                                sel->capacity * sizeof(ecs_entity_t));
    }
}

//------------------------------------------------------------------------------
// Internal: Mark entity renderable as dirty for re-render
//------------------------------------------------------------------------------

static inline void selection_mark_dirty(selection_buffer_t *sel, ecs_entity_t e) {
    if (!sel->world) return;
    RenderableComp *r = ecs_world_get_renderable(sel->world, e);
    if (r) {
        r->instance_dirty = true;
    }
}

//------------------------------------------------------------------------------
// Modification Functions
//------------------------------------------------------------------------------

// Clear all selections
static inline void selection_clear(selection_buffer_t *sel) {
    // Remove Selected tag from all entities and mark them dirty
    for (int i = 0; i < sel->count; i++) {
        ecs_entity_t e = sel->entities[i];
        if (sel->world && ecs_is_alive(sel->world->world, e)) {
            ecs_world_deselect(sel->world, e);
            selection_mark_dirty(sel, e);
        }
    }
    sel->count = 0;
}

// Add entity to selection (does nothing if already selected)
static inline void selection_add(selection_buffer_t *sel, ecs_entity_t e) {
    if (e == 0) return;
    if (selection_contains(sel, e)) return;

    selection_grow_if_needed(sel);
    sel->entities[sel->count++] = e;

    // Add Selected tag to entity
    if (sel->world && ecs_is_alive(sel->world->world, e)) {
        ecs_world_select(sel->world, e);
        selection_mark_dirty(sel, e);
    }
}

// Remove entity from selection (does nothing if not selected)
static inline void selection_remove(selection_buffer_t *sel, ecs_entity_t e) {
    for (int i = 0; i < sel->count; i++) {
        if (sel->entities[i] == e) {
            // Remove by swapping with last element
            sel->entities[i] = sel->entities[sel->count - 1];
            sel->count--;

            // Remove Selected tag from entity
            if (sel->world && ecs_is_alive(sel->world->world, e)) {
                ecs_world_deselect(sel->world, e);
                selection_mark_dirty(sel, e);
            }
            return;
        }
    }
}

// Remove dead entities from selection buffer (safety after scene-side deletions)
static inline void selection_prune_dead(selection_buffer_t *sel) {
    if (!sel || !sel->world) return;
    for (int i = 0; i < sel->count; ) {
        ecs_entity_t e = sel->entities[i];
        if (!ecs_is_alive(sel->world->world, e)) {
            sel->entities[i] = sel->entities[sel->count - 1];
            sel->count--;
            continue;
        }
        i++;
    }
}

// Toggle entity selection state
static inline void selection_toggle(selection_buffer_t *sel, ecs_entity_t e) {
    if (e == 0) return;
    if (selection_contains(sel, e)) {
        selection_remove(sel, e);
    } else {
        selection_add(sel, e);
    }
}

// Replace selection with a single entity (clears existing selection)
static inline void selection_set_single(selection_buffer_t *sel, ecs_entity_t e) {
    selection_clear(sel);
    if (e != 0) {
        selection_add(sel, e);
    }
}

//------------------------------------------------------------------------------
// Input Handling Helper
//------------------------------------------------------------------------------

// Handle selection based on click and modifier keys
// - clicked_entity: entity under cursor (0 if clicked empty space)
// - shift_held: add to selection
// - ctrl_held: toggle selection
static inline void selection_handle_click(selection_buffer_t *sel,
                                          ecs_entity_t clicked_entity,
                                          bool shift_held,
                                          bool ctrl_held) {
    if (clicked_entity == 0) {
        // Clicked on empty space
        if (!shift_held && !ctrl_held) {
            selection_clear(sel);
        }
        return;
    }

    if (ctrl_held) {
        // Ctrl+click: toggle selection
        selection_toggle(sel, clicked_entity);
    } else if (shift_held) {
        // Shift+click: add to selection
        selection_add(sel, clicked_entity);
    } else {
        // Normal click: replace selection
        selection_set_single(sel, clicked_entity);
    }
}

//------------------------------------------------------------------------------
// Bulk Operations
//------------------------------------------------------------------------------

// Get array of selected entities (returns pointer to internal array)
// Use count from selection_count() to iterate
static inline ecs_entity_t* selection_get_entities(selection_buffer_t *sel) {
    return sel->entities;
}

// Copy selected entities to provided array (caller must ensure sufficient capacity)
// Returns number of entities copied
static inline int selection_copy_entities(selection_buffer_t *sel, ecs_entity_t *out, int max_count) {
    int count = (sel->count < max_count) ? sel->count : max_count;
    for (int i = 0; i < count; i++) {
        out[i] = sel->entities[i];
    }
    return count;
}

#endif // SELECTION_H
