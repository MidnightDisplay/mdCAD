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
#include "../components/label_comp.h"
#include "../components/light_comp.h"
#include "../components/sketch_comp.h"
#include "../components/sketch_geometry_state_comp.h"
#include "../components/constraint_comp.h"
#include "../components/constraint_participant_comp.h"
#include "../components/script_identity_comp.h"
#include "../components/endpoints_comp.h"

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
    ecs_entity_t LabelComp_id;
    ecs_entity_t LightComp_id;
    ecs_entity_t SketchComp_id;
    ecs_entity_t SketchGeometryStateComp_id;
    ecs_entity_t ConstraintComp_id;
    ecs_entity_t ConstraintParticipantComp_id;
    ecs_entity_t ScriptIdentityComp_id;
    ecs_entity_t EndPointsComp_id;

    // Tag component IDs (no data, just markers)
    ecs_entity_t Selected_tag;      // Entity is currently selected
    ecs_entity_t Hovered_tag;       // Entity is under cursor
    ecs_entity_t ImportPending_tag; // Entity being imported (excluded from update)

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
    // Cap below reserved overlay ranges (constraint glyphs + gizmo/vertex handles)
    if (s->next_pick_id >= CONSTRAINT_GLYPH_PICK_BASE) return 0;
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

    s->LabelComp_id = ecs_component_init(s->world, &(ecs_component_desc_t){
        .entity = ecs_entity(s->world, { .name = "LabelComp" }),
        .type.size = sizeof(LabelComp),
        .type.alignment = ECS_ALIGNOF(LabelComp)
    });

    s->LightComp_id = ecs_component_init(s->world, &(ecs_component_desc_t){
        .entity = ecs_entity(s->world, { .name = "LightComp" }),
        .type.size = sizeof(LightComp),
        .type.alignment = ECS_ALIGNOF(LightComp)
    });

    s->SketchComp_id = ecs_component_init(s->world, &(ecs_component_desc_t){
        .entity = ecs_entity(s->world, { .name = "SketchComp" }),
        .type.size = sizeof(SketchComp),
        .type.alignment = ECS_ALIGNOF(SketchComp)
    });

    s->SketchGeometryStateComp_id = ecs_component_init(s->world, &(ecs_component_desc_t){
        .entity = ecs_entity(s->world, { .name = "SketchGeometryStateComp" }),
        .type.size = sizeof(SketchGeometryStateComp),
        .type.alignment = ECS_ALIGNOF(SketchGeometryStateComp)
    });

    s->ConstraintComp_id = ecs_component_init(s->world, &(ecs_component_desc_t){
        .entity = ecs_entity(s->world, { .name = "ConstraintComp" }),
        .type.size = sizeof(ConstraintComp),
        .type.alignment = ECS_ALIGNOF(ConstraintComp)
    });

    s->ConstraintParticipantComp_id = ecs_component_init(s->world, &(ecs_component_desc_t){
        .entity = ecs_entity(s->world, { .name = "ConstraintParticipantComp" }),
        .type.size = sizeof(ConstraintParticipantComp),
        .type.alignment = ECS_ALIGNOF(ConstraintParticipantComp)
    });

    s->ScriptIdentityComp_id = ecs_component_init(s->world, &(ecs_component_desc_t){
        .entity = ecs_entity(s->world, { .name = "ScriptIdentityComp" }),
        .type.size = sizeof(ScriptIdentityComp),
        .type.alignment = ECS_ALIGNOF(ScriptIdentityComp)
    });

    s->EndPointsComp_id = ecs_component_init(s->world, &(ecs_component_desc_t){
        .entity = ecs_entity(s->world, { .name = "EndPointsComp" }),
        .type.size = sizeof(EndPointsComp),
        .type.alignment = ECS_ALIGNOF(EndPointsComp)
    });

    // Register tag components (zero-size)
    s->Selected_tag = ecs_entity(s->world, { .name = "Selected" });
    s->Hovered_tag = ecs_entity(s->world, { .name = "Hovered" });
    s->ImportPending_tag = ecs_entity(s->world, { .name = "ImportPending" });

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

static inline LabelComp* ecs_world_get_label(ecs_world_state_t *s, ecs_entity_t e) {
    return (LabelComp*)ecs_get_id(s->world, e, s->LabelComp_id);
}

static inline void ecs_world_set_label(ecs_world_state_t *s, ecs_entity_t e, const LabelComp *label) {
    ecs_set_id(s->world, e, s->LabelComp_id, sizeof(LabelComp), label);
}

static inline LightComp* ecs_world_get_light(ecs_world_state_t *s, ecs_entity_t e) {
    return (LightComp*)ecs_get_id(s->world, e, s->LightComp_id);
}

static inline SketchComp* ecs_world_get_sketch(ecs_world_state_t *s, ecs_entity_t e) {
    return (SketchComp*)ecs_get_id(s->world, e, s->SketchComp_id);
}

static inline void ecs_world_set_sketch(ecs_world_state_t *s, ecs_entity_t e, const SketchComp *sketch) {
    ecs_set_id(s->world, e, s->SketchComp_id, sizeof(SketchComp), sketch);
}

static inline SketchGeometryStateComp* ecs_world_get_sketch_geometry_state(ecs_world_state_t *s, ecs_entity_t e) {
    return (SketchGeometryStateComp*)ecs_get_id(s->world, e, s->SketchGeometryStateComp_id);
}

static inline void ecs_world_set_sketch_geometry_state(ecs_world_state_t *s, ecs_entity_t e,
                                                        const SketchGeometryStateComp *state) {
    ecs_set_id(s->world, e, s->SketchGeometryStateComp_id, sizeof(SketchGeometryStateComp), state);
}

static inline ConstraintComp* ecs_world_get_constraint(ecs_world_state_t *s, ecs_entity_t e) {
    return (ConstraintComp*)ecs_get_id(s->world, e, s->ConstraintComp_id);
}

static inline void ecs_world_set_constraint(ecs_world_state_t *s, ecs_entity_t e, const ConstraintComp *constraint) {
    ecs_set_id(s->world, e, s->ConstraintComp_id, sizeof(ConstraintComp), constraint);
}

static inline ConstraintParticipantComp* ecs_world_get_constraint_participant(ecs_world_state_t *s, ecs_entity_t e) {
    return (ConstraintParticipantComp*)ecs_get_id(s->world, e, s->ConstraintParticipantComp_id);
}

static inline void ecs_world_set_constraint_participant(ecs_world_state_t *s, ecs_entity_t e,
                                                         const ConstraintParticipantComp *participant) {
    ecs_set_id(s->world, e, s->ConstraintParticipantComp_id, sizeof(ConstraintParticipantComp), participant);
}

static inline ScriptIdentityComp* ecs_world_get_script_identity(ecs_world_state_t *s, ecs_entity_t e) {
    return (ScriptIdentityComp*)ecs_get_id(s->world, e, s->ScriptIdentityComp_id);
}

static inline void ecs_world_set_script_identity(ecs_world_state_t *s, ecs_entity_t e,
                                                 const ScriptIdentityComp *identity) {
    ecs_set_id(s->world, e, s->ScriptIdentityComp_id, sizeof(ScriptIdentityComp), identity);
}

static inline EndPointsComp* ecs_world_get_endpoints(ecs_world_state_t *s, ecs_entity_t e) {
    return (EndPointsComp*)ecs_get_id(s->world, e, s->EndPointsComp_id);
}

static inline void ecs_world_set_endpoints(ecs_world_state_t *s, ecs_entity_t e,
                                           const EndPointsComp *endpoints) {
    ecs_set_id(s->world, e, s->EndPointsComp_id, sizeof(EndPointsComp), endpoints);
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

// Count total number of children (without limit)
static inline int ecs_world_count_children(ecs_world_state_t *s, ecs_entity_t parent) {
    if (!ecs_is_alive(s->world, parent)) return 0;

    int count = 0;
    ecs_iter_t it = ecs_children(s->world, parent);
    while (ecs_children_next(&it)) {
        count += it.count;
    }
    return count;
}

// Mark all descendants as dirty (recursive)
static inline void ecs_world_mark_descendants_dirty(ecs_world_state_t *s, ecs_entity_t parent) {
    // Use ecs_children iterator directly to handle unlimited children
    ecs_iter_t child_it = ecs_children(s->world, parent);
    while (ecs_children_next(&child_it)) {
        for (int i = 0; i < child_it.count; i++) {
            ecs_entity_t child = child_it.entities[i];

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
}

// Set visibility on all descendants (recursive)
static inline void ecs_world_set_descendants_visible(ecs_world_state_t *s, ecs_entity_t parent, bool visible) {
    ecs_iter_t child_it = ecs_children(s->world, parent);
    while (ecs_children_next(&child_it)) {
        for (int i = 0; i < child_it.count; i++) {
            ecs_entity_t child = child_it.entities[i];
            RenderableComp *r = (RenderableComp*)ecs_get_id(s->world, child, s->RenderableComp_id);
            if (r) {
                r->visible = visible;
                r->instance_dirty = true;
            }
            ecs_world_set_descendants_visible(s, child, visible);
        }
    }
}

// Create a lightweight anchor entity with only TransformComp (no geometry, no GPU slot)
static inline ecs_entity_t ecs_world_create_anchor_entity(ecs_world_state_t *s) {
    ecs_entity_t e = ecs_new(s->world);
    TransformComp t = transform_comp_default();
    ecs_set_id(s->world, e, s->TransformComp_id, sizeof(TransformComp), &t);
    return e;
}

// Batch-parent all children to a single parent (deferred for O(n) perf)
static inline void ecs_world_set_parent_batch(
    ecs_world_state_t *s,
    ecs_entity_t *children, int count,
    ecs_entity_t parent)
{
    if (count <= 0 || parent == 0 || !ecs_is_alive(s->world, parent)) return;

    // Phase 1: Batch structural changes (deferred)
    ecs_defer_begin(s->world);
    for (int i = 0; i < count; i++) {
        if (!ecs_is_alive(s->world, children[i])) continue;
        ecs_entity_t current = ecs_get_parent(s->world, children[i]);
        if (current != 0) {
            ecs_remove_pair(s->world, children[i], EcsChildOf, current);
        }
        ecs_add_pair(s->world, children[i], EcsChildOf, parent);
    }
    ecs_defer_end(s->world);

    // Phase 2: Mark dirty (after flush - pointers are valid now)
    for (int i = 0; i < count; i++) {
        if (!ecs_is_alive(s->world, children[i])) continue;
        TransformComp *t = ecs_world_get_transform(s, children[i]);
        if (t) t->dirty = true;
        RenderableComp *r = ecs_world_get_renderable(s, children[i]);
        if (r) r->instance_dirty = true;
    }
}

// Batch-parent each child to its own parent (deferred for O(n) perf)
static inline void ecs_world_set_parents_batch(
    ecs_world_state_t *s,
    ecs_entity_t *children, ecs_entity_t *parents, int count)
{
    if (count <= 0) return;

    // Phase 1: Batch structural changes
    ecs_defer_begin(s->world);
    for (int i = 0; i < count; i++) {
        if (!ecs_is_alive(s->world, children[i])) continue;
        if (parents[i] == 0 || !ecs_is_alive(s->world, parents[i])) continue;
        ecs_entity_t current = ecs_get_parent(s->world, children[i]);
        if (current != 0) {
            ecs_remove_pair(s->world, children[i], EcsChildOf, current);
        }
        ecs_add_pair(s->world, children[i], EcsChildOf, parents[i]);
    }
    ecs_defer_end(s->world);

    // Phase 2: Mark dirty
    for (int i = 0; i < count; i++) {
        if (!ecs_is_alive(s->world, children[i])) continue;
        TransformComp *t = ecs_world_get_transform(s, children[i]);
        if (t) t->dirty = true;
        RenderableComp *r = ecs_world_get_renderable(s, children[i]);
        if (r) r->instance_dirty = true;
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
