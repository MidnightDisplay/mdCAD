//------------------------------------------------------------------------------
// ecs_world.h - ECS World initialization and management (header-only)
//------------------------------------------------------------------------------
#ifndef ECS_WORLD_H
#define ECS_WORLD_H

#include "flecs.h"
#include "../components/transform_comp.h"
#include "../components/geometry_comp.h"
#include "../components/renderable_comp.h"
#include "../components/selectable_comp.h"

//------------------------------------------------------------------------------
// Types
//------------------------------------------------------------------------------

// Global ECS world state
typedef struct {
    ecs_world_t *world;

    // Component IDs (registered at init)
    ecs_entity_t TransformComp_id;
    ecs_entity_t GeometryComp_id;
    ecs_entity_t RenderableComp_id;
    ecs_entity_t SelectableComp_id;

    // Tag component IDs (no data, just markers)
    ecs_entity_t Selected_tag;      // Entity is currently selected
    ecs_entity_t Hovered_tag;       // Entity is under cursor

    // Pick ID allocator
    uint32_t next_pick_id;          // Start at 1 (0 = no entity)
    uint32_t *free_pick_ids;        // Recycled IDs
    int free_pick_count;
    int free_pick_capacity;
} ecs_world_state_t;

//------------------------------------------------------------------------------
// Pick ID Allocator
//------------------------------------------------------------------------------

static inline uint32_t ecs_world_alloc_pick_id(ecs_world_state_t *s) {
    // First check free list
    if (s->free_pick_count > 0) {
        return s->free_pick_ids[--s->free_pick_count];
    }
    // Allocate new ID
    return s->next_pick_id++;
}

static inline void ecs_world_free_pick_id(ecs_world_state_t *s, uint32_t id) {
    if (id == 0) return;  // 0 is invalid/background

    // Grow free list if needed
    if (s->free_pick_count >= s->free_pick_capacity) {
        s->free_pick_capacity = s->free_pick_capacity ? s->free_pick_capacity * 2 : 64;
        s->free_pick_ids = (uint32_t*)realloc(s->free_pick_ids,
                                               sizeof(uint32_t) * s->free_pick_capacity);
    }
    s->free_pick_ids[s->free_pick_count++] = id;
}

//------------------------------------------------------------------------------
// World Initialization
//------------------------------------------------------------------------------

static inline void ecs_world_init(ecs_world_state_t *s) {
    // Create world
    s->world = ecs_init();

    // Register components
    s->TransformComp_id = ecs_component_init(s->world, &(ecs_component_desc_t){
        .entity = ecs_entity(s->world, { .name = "TransformComp" }),
        .type.size = sizeof(TransformComp),
        .type.alignment = ECS_ALIGNOF(TransformComp)
    });

    s->GeometryComp_id = ecs_component_init(s->world, &(ecs_component_desc_t){
        .entity = ecs_entity(s->world, { .name = "GeometryComp" }),
        .type.size = sizeof(GeometryComp),
        .type.alignment = ECS_ALIGNOF(GeometryComp)
    });

    s->RenderableComp_id = ecs_component_init(s->world, &(ecs_component_desc_t){
        .entity = ecs_entity(s->world, { .name = "RenderableComp" }),
        .type.size = sizeof(RenderableComp),
        .type.alignment = ECS_ALIGNOF(RenderableComp)
    });

    s->SelectableComp_id = ecs_component_init(s->world, &(ecs_component_desc_t){
        .entity = ecs_entity(s->world, { .name = "SelectableComp" }),
        .type.size = sizeof(SelectableComp),
        .type.alignment = ECS_ALIGNOF(SelectableComp)
    });

    // Register tag components (zero-size)
    s->Selected_tag = ecs_entity(s->world, { .name = "Selected" });
    s->Hovered_tag = ecs_entity(s->world, { .name = "Hovered" });

    // Initialize pick ID allocator
    s->next_pick_id = 1;  // 0 is reserved for "no entity"
    s->free_pick_ids = NULL;
    s->free_pick_count = 0;
    s->free_pick_capacity = 0;
}

//------------------------------------------------------------------------------
// World Progress
//------------------------------------------------------------------------------

static inline void ecs_world_progress(ecs_world_state_t *s, float dt) {
    ecs_progress(s->world, dt);
}

//------------------------------------------------------------------------------
// World Shutdown
//------------------------------------------------------------------------------

static inline void ecs_world_shutdown(ecs_world_state_t *s) {
    // Free pick ID allocator
    if (s->free_pick_ids) {
        free(s->free_pick_ids);
        s->free_pick_ids = NULL;
    }

    // Destroy world
    ecs_fini(s->world);
    s->world = NULL;
}

//------------------------------------------------------------------------------
// Entity Creation Helpers
//------------------------------------------------------------------------------

// Create an entity with all standard components
static inline ecs_entity_t ecs_world_create_entity(ecs_world_state_t *s) {
    ecs_entity_t e = ecs_new(s->world);

    // Add transform component
    TransformComp t = transform_comp_default();
    ecs_set_id(s->world, e, s->TransformComp_id, sizeof(TransformComp), &t);

    // Add renderable component
    RenderableComp r = renderable_comp_default();
    ecs_set_id(s->world, e, s->RenderableComp_id, sizeof(RenderableComp), &r);

    // Add selectable component with unique pick ID
    SelectableComp sel = selectable_comp_with_id(ecs_world_alloc_pick_id(s));
    ecs_set_id(s->world, e, s->SelectableComp_id, sizeof(SelectableComp), &sel);

    return e;
}

// Set geometry component on an entity
static inline void ecs_world_set_geometry(ecs_world_state_t *s, ecs_entity_t e, GeometryComp *g) {
    ecs_set_id(s->world, e, s->GeometryComp_id, sizeof(GeometryComp), g);

    // Mark renderable as dirty
    RenderableComp *r = (RenderableComp*)ecs_get_id(s->world, e, s->RenderableComp_id);
    if (r) {
        r->instance_dirty = true;
    }
}

// Delete an entity and clean up resources
static inline void ecs_world_delete_entity(ecs_world_state_t *s, ecs_entity_t e) {
    // Free pick ID
    const SelectableComp *sel = (const SelectableComp*)ecs_get_id(s->world, e, s->SelectableComp_id);
    if (sel) {
        ecs_world_free_pick_id(s, sel->pick_id);
    }

    // Free geometry dynamic allocations
    const GeometryComp *g = (const GeometryComp*)ecs_get_id(s->world, e, s->GeometryComp_id);
    if (g) {
        // Note: We cast away const here because we need to free memory
        // This is safe because we're about to delete the entity
        geometry_comp_free((GeometryComp*)g);
    }

    // Delete entity
    ecs_delete(s->world, e);
}

//------------------------------------------------------------------------------
// Selection Helpers
//------------------------------------------------------------------------------

static inline bool ecs_world_is_selected(ecs_world_state_t *s, ecs_entity_t e) {
    return ecs_has_id(s->world, e, s->Selected_tag);
}

static inline void ecs_world_select(ecs_world_state_t *s, ecs_entity_t e) {
    ecs_add_id(s->world, e, s->Selected_tag);
}

static inline void ecs_world_deselect(ecs_world_state_t *s, ecs_entity_t e) {
    ecs_remove_id(s->world, e, s->Selected_tag);
}

static inline bool ecs_world_is_hovered(ecs_world_state_t *s, ecs_entity_t e) {
    return ecs_has_id(s->world, e, s->Hovered_tag);
}

static inline void ecs_world_set_hovered(ecs_world_state_t *s, ecs_entity_t e) {
    ecs_add_id(s->world, e, s->Hovered_tag);
}

static inline void ecs_world_clear_hovered(ecs_world_state_t *s, ecs_entity_t e) {
    ecs_remove_id(s->world, e, s->Hovered_tag);
}

//------------------------------------------------------------------------------
// Component Access Helpers
//------------------------------------------------------------------------------

static inline TransformComp* ecs_world_get_transform(ecs_world_state_t *s, ecs_entity_t e) {
    return (TransformComp*)ecs_get_id(s->world, e, s->TransformComp_id);
}

static inline GeometryComp* ecs_world_get_geometry(ecs_world_state_t *s, ecs_entity_t e) {
    return (GeometryComp*)ecs_get_id(s->world, e, s->GeometryComp_id);
}

static inline RenderableComp* ecs_world_get_renderable(ecs_world_state_t *s, ecs_entity_t e) {
    return (RenderableComp*)ecs_get_id(s->world, e, s->RenderableComp_id);
}

static inline SelectableComp* ecs_world_get_selectable(ecs_world_state_t *s, ecs_entity_t e) {
    return (SelectableComp*)ecs_get_id(s->world, e, s->SelectableComp_id);
}

//------------------------------------------------------------------------------
// Parent-Child Relationship Helpers (using Flecs built-in EcsChildOf)
//------------------------------------------------------------------------------

// Get parent of an entity (returns 0 if no parent)
static inline ecs_entity_t ecs_world_get_parent(ecs_world_state_t *s, ecs_entity_t e) {
    if (!ecs_is_alive(s->world, e)) return 0;
    return ecs_get_parent(s->world, e);
}

// Check if entity has a parent
static inline bool ecs_world_has_parent(ecs_world_state_t *s, ecs_entity_t e) {
    return ecs_world_get_parent(s, e) != 0;
}

// Get children of an entity (returns count, fills out_children up to max_count)
static inline int ecs_world_get_children(ecs_world_state_t *s, ecs_entity_t parent,
                                          ecs_entity_t *out_children, int max_count) {
    if (!ecs_is_alive(s->world, parent) || !out_children || max_count <= 0) return 0;

    int count = 0;

    // Iterate over entities that are children of the parent
    ecs_iter_t it = ecs_children(s->world, parent);
    while (ecs_children_next(&it)) {
        for (int i = 0; i < it.count && count < max_count; i++) {
            out_children[count++] = it.entities[i];
        }
        if (count >= max_count) {
            ecs_iter_fini(&it);
            break;
        }
    }

    return count;
}

// Check if entity has any children
static inline bool ecs_world_has_children(ecs_world_state_t *s, ecs_entity_t parent) {
    if (!ecs_is_alive(s->world, parent)) return false;

    ecs_iter_t it = ecs_children(s->world, parent);
    bool has_children = ecs_children_next(&it);
    if (has_children) {
        ecs_iter_fini(&it);
    }
    return has_children;
}

// Mark all descendants as dirty (recursive)
static inline void ecs_world_mark_descendants_dirty(ecs_world_state_t *s, ecs_entity_t parent) {
    ecs_entity_t children[64];  // Reasonable batch size
    int count = ecs_world_get_children(s, parent, children, 64);

    for (int i = 0; i < count; i++) {
        ecs_entity_t child = children[i];

        // Mark this child's transform dirty
        TransformComp *t = (TransformComp*)ecs_get_id(s->world, child, s->TransformComp_id);
        if (t) {
            t->dirty = true;
        }

        RenderableComp *r = (RenderableComp*)ecs_get_id(s->world, child, s->RenderableComp_id);
        if (r) {
            r->instance_dirty = true;
        }

        // Recursively mark this child's descendants
        ecs_world_mark_descendants_dirty(s, child);
    }
}

// Set parent of an entity (use 0 to unparent)
static inline void ecs_world_set_parent(ecs_world_state_t *s, ecs_entity_t child, ecs_entity_t parent) {
    if (!ecs_is_alive(s->world, child)) return;

    // Remove existing parent relationship if any
    ecs_entity_t current_parent = ecs_get_parent(s->world, child);
    if (current_parent != 0) {
        ecs_remove_pair(s->world, child, EcsChildOf, current_parent);
    }

    // Add new parent relationship
    if (parent != 0 && ecs_is_alive(s->world, parent)) {
        ecs_add_pair(s->world, child, EcsChildOf, parent);

        // Mark child's transform as dirty so it recalculates world matrix
        TransformComp *t = (TransformComp*)ecs_get_id(s->world, child, s->TransformComp_id);
        if (t) {
            t->dirty = true;
        }

        // Mark renderable as dirty
        RenderableComp *r = (RenderableComp*)ecs_get_id(s->world, child, s->RenderableComp_id);
        if (r) {
            r->instance_dirty = true;
        }

        // Also mark all descendants dirty - the entire subtree needs to update
        // its world matrices since it now has a new ancestor
        ecs_world_mark_descendants_dirty(s, child);
    }
}

#endif // ECS_WORLD_H
