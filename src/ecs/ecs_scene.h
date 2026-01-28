//------------------------------------------------------------------------------
// ecs_scene.h - High-level scene API for ECS entity management (header-only)
//
// Provides a simple API for creating and managing ECS entities with automatic
// instance buffer slot allocation and GPU rendering integration.
//------------------------------------------------------------------------------
#ifndef ECS_SCENE_H
#define ECS_SCENE_H

#include "ecs_world.h"
#include "../gpu/geometry_batch.h"
#include "../gpu/pick_buffer.h"
#include "../components/geometry_comp.h"
#include "../components/transform_comp.h"
#include "../components/renderable_comp.h"
#include "../components/selectable_comp.h"

//------------------------------------------------------------------------------
// Types
//------------------------------------------------------------------------------

// Complete ECS scene state
typedef struct {
    ecs_world_state_t *world;           // Pointer to ECS world
    geometry_batch_manager_t batches;   // GPU batch manager

    // Visibility flag for ECS rendering
    bool visible;
} ecs_scene_t;

//------------------------------------------------------------------------------
// Scene Initialization
//------------------------------------------------------------------------------

static inline void ecs_scene_init(ecs_scene_t *scene, ecs_world_state_t *world) {
    scene->world = world;
    scene->visible = true;
    geometry_batch_manager_init(&scene->batches);
}

static inline void ecs_scene_shutdown(ecs_scene_t *scene) {
    geometry_batch_manager_shutdown(&scene->batches);
}

//------------------------------------------------------------------------------
// Entity Creation API
//------------------------------------------------------------------------------

// Create a line entity
static inline ecs_entity_t scene_add_line(ecs_scene_t *scene,
                                           vec3_t a, vec3_t b,
                                           vec4_t color, float width) {
    // Create base entity with transform, renderable, selectable
    ecs_entity_t e = ecs_world_create_entity(scene->world);

    // Set geometry
    GeometryComp g = geometry_comp_line(a, b, color, width);
    ecs_world_set_geometry(scene->world, e, &g);

    // Allocate instance buffer slot
    int slot = geom_line_batch_alloc(&scene->batches.lines);
    if (slot >= 0) {
        // Get transform for position (identity for now)
        TransformComp *t = ecs_world_get_transform(scene->world, e);
        vec3_t world_a = mat4_transform_point(t->world_matrix, a);
        vec3_t world_b = mat4_transform_point(t->world_matrix, b);

        // Set instance data
        geom_line_batch_set(&scene->batches.lines, slot, world_a, world_b, color);

        // Update renderable with slot info
        RenderableComp *r = ecs_world_get_renderable(scene->world, e);
        if (r) {
            r->batch_id = GEOM_LINE;
            r->instance_slot = (uint32_t)slot;
            r->instance_dirty = false;  // Already set
        }
    }

    return e;
}

// Create a point entity
static inline ecs_entity_t scene_add_point(ecs_scene_t *scene,
                                            vec3_t pos,
                                            vec4_t color, float size) {
    ecs_entity_t e = ecs_world_create_entity(scene->world);

    GeometryComp g = geometry_comp_point(pos, color, size);
    ecs_world_set_geometry(scene->world, e, &g);

    int slot = geom_point_batch_alloc(&scene->batches.points);
    if (slot >= 0) {
        TransformComp *t = ecs_world_get_transform(scene->world, e);
        vec3_t world_pos = mat4_transform_point(t->world_matrix, pos);

        geom_point_batch_set(&scene->batches.points, slot, world_pos, color);

        RenderableComp *r = ecs_world_get_renderable(scene->world, e);
        if (r) {
            r->batch_id = GEOM_POINT;
            r->instance_slot = (uint32_t)slot;
            r->instance_dirty = false;
        }
    }

    return e;
}

// Create an arc entity
static inline ecs_entity_t scene_add_arc(ecs_scene_t *scene,
                                          vec3_t center, float radius,
                                          float start_angle, float end_angle,
                                          vec3_t normal,
                                          vec4_t color, float width) {
    ecs_entity_t e = ecs_world_create_entity(scene->world);

    GeometryComp g = geometry_comp_arc(center, radius, start_angle, end_angle, normal, color, width);
    ecs_world_set_geometry(scene->world, e, &g);

    // TODO: Arc rendering via polyline tessellation (Phase 6)
    RenderableComp *r = ecs_world_get_renderable(scene->world, e);
    if (r) {
        r->batch_id = GEOM_ARC;
        r->instance_slot = 0xFFFFFFFF;  // Not yet implemented
        r->instance_dirty = true;
    }

    return e;
}

// Create a bezier curve entity
static inline ecs_entity_t scene_add_bezier(ecs_scene_t *scene,
                                             vec3_t p0, vec3_t p1, vec3_t p2, vec3_t p3,
                                             int segments,
                                             vec4_t color, float width) {
    ecs_entity_t e = ecs_world_create_entity(scene->world);

    GeometryComp g = geometry_comp_bezier(p0, p1, p2, p3, segments, color, width);
    ecs_world_set_geometry(scene->world, e, &g);

    // TODO: Bezier rendering via polyline tessellation (Phase 6)
    RenderableComp *r = ecs_world_get_renderable(scene->world, e);
    if (r) {
        r->batch_id = GEOM_BEZIER;
        r->instance_slot = 0xFFFFFFFF;  // Not yet implemented
        r->instance_dirty = true;
    }

    return e;
}

// Create a helix entity
static inline ecs_entity_t scene_add_helix(ecs_scene_t *scene,
                                            vec3_t axis_start, vec3_t axis_end,
                                            float radius, float turns, int segments,
                                            vec4_t color, float width) {
    ecs_entity_t e = ecs_world_create_entity(scene->world);

    GeometryComp g = geometry_comp_helix(axis_start, axis_end, radius, turns, segments, color, width);
    ecs_world_set_geometry(scene->world, e, &g);

    // TODO: Helix rendering via polyline tessellation (Phase 6)
    RenderableComp *r = ecs_world_get_renderable(scene->world, e);
    if (r) {
        r->batch_id = GEOM_HELIX;
        r->instance_slot = 0xFFFFFFFF;  // Not yet implemented
        r->instance_dirty = true;
    }

    return e;
}

//------------------------------------------------------------------------------
// Entity Deletion
//------------------------------------------------------------------------------

static inline void scene_remove_entity(ecs_scene_t *scene, ecs_entity_t e) {
    // Get renderable to find which batch and slot
    RenderableComp *r = ecs_world_get_renderable(scene->world, e);
    if (r && r->instance_slot != 0xFFFFFFFF) {
        // Free the instance slot based on batch type
        switch (r->batch_id) {
            case GEOM_LINE:
                geom_line_batch_free(&scene->batches.lines, (int)r->instance_slot);
                break;
            case GEOM_POINT:
                geom_point_batch_free(&scene->batches.points, (int)r->instance_slot);
                break;
            // TODO: Other geometry types
            default:
                break;
        }
    }

    // Delete entity (also frees pick ID and geometry allocations)
    ecs_world_delete_entity(scene->world, e);
}

//------------------------------------------------------------------------------
// Entity Property Modification
//------------------------------------------------------------------------------

// Update entity color
static inline void scene_set_color(ecs_scene_t *scene, ecs_entity_t e, vec4_t color) {
    GeometryComp *g = ecs_world_get_geometry(scene->world, e);
    RenderableComp *r = ecs_world_get_renderable(scene->world, e);
    if (!g || !r) return;

    g->color = color;
    r->instance_dirty = true;
}

// Update entity visibility
static inline void scene_set_visible(ecs_scene_t *scene, ecs_entity_t e, bool visible) {
    RenderableComp *r = ecs_world_get_renderable(scene->world, e);
    if (r) {
        r->visible = visible;
        // Note: Hidden entities still occupy slots, just don't render
        // For true removal, use scene_remove_entity
    }
}

// Update entity position (sets transform, marks dirty)
static inline void scene_set_position(ecs_scene_t *scene, ecs_entity_t e, vec3_t pos) {
    TransformComp *t = ecs_world_get_transform(scene->world, e);
    RenderableComp *r = ecs_world_get_renderable(scene->world, e);
    if (t && r) {
        t->position = pos;
        t->dirty = true;
        r->instance_dirty = true;
    }
}

//------------------------------------------------------------------------------
// Scene Update (sync dirty entities to GPU)
//------------------------------------------------------------------------------

static inline void ecs_scene_update(ecs_scene_t *scene) {
    ecs_world_state_t *w = scene->world;

    // Query all entities with geometry and renderable
    ecs_query_t *q = ecs_query(w->world, {
        .terms = {
            { .id = w->GeometryComp_id },
            { .id = w->RenderableComp_id },
            { .id = w->TransformComp_id }
        }
    });

    ecs_iter_t it = ecs_query_iter(w->world, q);
    while (ecs_query_next(&it)) {
        GeometryComp *geoms = ecs_field(&it, GeometryComp, 0);
        RenderableComp *renderables = ecs_field(&it, RenderableComp, 1);
        TransformComp *transforms = ecs_field(&it, TransformComp, 2);

        for (int i = 0; i < it.count; i++) {
            RenderableComp *r = &renderables[i];
            if (!r->instance_dirty) continue;
            if (r->instance_slot == 0xFFFFFFFF) continue;  // No slot allocated

            GeometryComp *g = &geoms[i];
            TransformComp *t = &transforms[i];

            // Recalculate world matrix if transform dirty
            if (t->dirty) {
                // Create translation matrix and apply to identity
                mat4_t trans = mat4_translate(t->position.x, t->position.y, t->position.z);
                t->world_matrix = trans;
                // TODO: rotation, scale
                t->dirty = false;
            }

            // Update instance data based on geometry type
            switch (g->type) {
                case GEOM_LINE: {
                    vec3_t world_a = mat4_transform_point(t->world_matrix, g->data.line.a);
                    vec3_t world_b = mat4_transform_point(t->world_matrix, g->data.line.b);
                    geom_line_batch_set(&scene->batches.lines, (int)r->instance_slot,
                                        world_a, world_b, g->color);
                    break;
                }
                case GEOM_POINT: {
                    vec3_t world_pos = mat4_transform_point(t->world_matrix, g->data.point.point);
                    geom_point_batch_set(&scene->batches.points, (int)r->instance_slot,
                                         world_pos, g->color);
                    break;
                }
                // TODO: Other geometry types (Phase 6)
                default:
                    break;
            }

            r->instance_dirty = false;
        }
    }

    ecs_query_fini(q);

    // Upload instance data to GPU
    geometry_batch_manager_upload(&scene->batches);
}

//------------------------------------------------------------------------------
// Scene Rendering
//------------------------------------------------------------------------------

static inline void ecs_scene_draw(ecs_scene_t *scene, mat4_t mvp, float aspect_ratio) {
    if (!scene->visible) return;
    geometry_batch_manager_draw(&scene->batches, mvp, aspect_ratio);
}

//------------------------------------------------------------------------------
// Utility Functions
//------------------------------------------------------------------------------

static inline int ecs_scene_line_count(ecs_scene_t *scene) {
    return instance_buffer_count(&scene->batches.lines.instances);
}

static inline int ecs_scene_point_count(ecs_scene_t *scene) {
    return instance_buffer_count(&scene->batches.points.instances);
}

//------------------------------------------------------------------------------
// GPU Picking Support
//------------------------------------------------------------------------------

// Populate pick buffer with all visible, pickable entities
static inline void ecs_scene_populate_pick_buffer(ecs_scene_t *scene, pick_buffer_t *pb) {
    if (!scene->visible) return;

    ecs_world_state_t *w = scene->world;

    // Query all entities with geometry, renderable, selectable, and transform
    ecs_query_t *q = ecs_query(w->world, {
        .terms = {
            { .id = w->GeometryComp_id },
            { .id = w->RenderableComp_id },
            { .id = w->SelectableComp_id },
            { .id = w->TransformComp_id }
        }
    });

    ecs_iter_t it = ecs_query_iter(w->world, q);
    while (ecs_query_next(&it)) {
        GeometryComp *geoms = ecs_field(&it, GeometryComp, 0);
        RenderableComp *renderables = ecs_field(&it, RenderableComp, 1);
        SelectableComp *selectables = ecs_field(&it, SelectableComp, 2);
        TransformComp *transforms = ecs_field(&it, TransformComp, 3);

        for (int i = 0; i < it.count; i++) {
            RenderableComp *r = &renderables[i];
            SelectableComp *s = &selectables[i];

            // Skip hidden or non-pickable entities
            if (!r->visible || !s->pickable) continue;
            if (s->pick_id == 0) continue;  // Invalid pick ID

            GeometryComp *g = &geoms[i];
            TransformComp *t = &transforms[i];

            // Add to pick buffer based on geometry type
            switch (g->type) {
                case GEOM_LINE: {
                    vec3_t world_a = mat4_transform_point(t->world_matrix, g->data.line.a);
                    vec3_t world_b = mat4_transform_point(t->world_matrix, g->data.line.b);
                    pick_buffer_add_line(pb, world_a, world_b, s->pick_id);
                    break;
                }
                case GEOM_POINT: {
                    vec3_t world_pos = mat4_transform_point(t->world_matrix, g->data.point.point);
                    pick_buffer_add_point(pb, world_pos, s->pick_id);
                    break;
                }
                // TODO: Other geometry types (Phase 6)
                default:
                    break;
            }
        }
    }

    ecs_query_fini(q);
}

// Find entity by pick ID (returns 0 if not found)
static inline ecs_entity_t ecs_scene_find_entity_by_pick_id(ecs_scene_t *scene, uint32_t pick_id) {
    if (pick_id == 0) return 0;

    ecs_world_state_t *w = scene->world;

    // Query all entities with SelectableComp
    ecs_query_t *q = ecs_query(w->world, {
        .terms = {
            { .id = w->SelectableComp_id }
        }
    });

    ecs_entity_t found = 0;

    ecs_iter_t it = ecs_query_iter(w->world, q);
    while (ecs_query_next(&it) && found == 0) {
        SelectableComp *selectables = ecs_field(&it, SelectableComp, 0);

        for (int i = 0; i < it.count; i++) {
            if (selectables[i].pick_id == pick_id) {
                found = it.entities[i];
                break;
            }
        }
    }

    ecs_query_fini(q);
    return found;
}

// Clear hovered state from all entities
static inline void ecs_scene_clear_all_hovered(ecs_scene_t *scene) {
    ecs_world_state_t *w = scene->world;

    // Query all entities with Hovered tag
    ecs_query_t *q = ecs_query(w->world, {
        .terms = {
            { .id = w->Hovered_tag }
        }
    });

    ecs_iter_t it = ecs_query_iter(w->world, q);
    while (ecs_query_next(&it)) {
        for (int i = 0; i < it.count; i++) {
            ecs_world_clear_hovered(w, it.entities[i]);
        }
    }

    ecs_query_fini(q);
}

// Update hover state based on pick buffer result
static inline void ecs_scene_update_hover(ecs_scene_t *scene, pick_buffer_t *pb) {
    // Clear previous hover
    ecs_scene_clear_all_hovered(scene);

    // Get newly hovered entity
    uint32_t pick_id = pick_buffer_get_hovered_id(pb);
    if (pick_id != 0) {
        ecs_entity_t e = ecs_scene_find_entity_by_pick_id(scene, pick_id);
        if (e != 0) {
            ecs_world_set_hovered(scene->world, e);
        }
    }
}

// Get currently hovered entity (returns 0 if none)
static inline ecs_entity_t ecs_scene_get_hovered_entity(ecs_scene_t *scene) {
    ecs_world_state_t *w = scene->world;

    // Query for entity with Hovered tag
    ecs_query_t *q = ecs_query(w->world, {
        .terms = {
            { .id = w->Hovered_tag }
        }
    });

    ecs_entity_t hovered = 0;

    ecs_iter_t it = ecs_query_iter(w->world, q);
    if (ecs_query_next(&it) && it.count > 0) {
        hovered = it.entities[0];  // Return first hovered (should be only one)
    }

    ecs_query_fini(q);
    return hovered;
}

#endif // ECS_SCENE_H
