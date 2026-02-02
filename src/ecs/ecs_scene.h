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

// For theme-aware hover colors (cimgui already defined in demo.c before this include)
#ifndef CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#endif
#include "cimgui.h"

//------------------------------------------------------------------------------
// Configuration
//------------------------------------------------------------------------------

#define ECS_SCENE_ARC_SEGMENTS_PER_RAD 8   // Arc tessellation density
#define ECS_SCENE_BEZIER_DEFAULT_SEGMENTS 16
#define ECS_SCENE_HELIX_DEFAULT_SEGMENTS 32

//------------------------------------------------------------------------------
// Tessellation Helpers
//------------------------------------------------------------------------------

// Calculate number of segments needed for an arc
static inline int ecs_scene_arc_segment_count(float start_angle, float end_angle) {
    float angle_span = fabsf(end_angle - start_angle);
    int segments = (int)(angle_span * ECS_SCENE_ARC_SEGMENTS_PER_RAD) + 1;
    if (segments < 2) segments = 2;
    if (segments > 128) segments = 128;
    return segments;
}

// Tessellate arc to points array (caller must free)
// Returns allocated points array and sets point_count
static inline vec3_t* ecs_scene_tessellate_arc(
    vec3_t center, float radius, float start_angle, float end_angle, vec3_t normal,
    int *point_count
) {
    int segments = ecs_scene_arc_segment_count(start_angle, end_angle);
    *point_count = segments + 1;

    vec3_t *points = (vec3_t*)malloc((*point_count) * sizeof(vec3_t));
    if (!points) return NULL;

    // Compute local coordinate system on the arc plane
    // normal is the Z axis, we need X and Y axes in the plane
    vec3_t up = normal;
    vec3_t arbitrary = (fabsf(up.y) < 0.9f) ? vec3_make(0, 1, 0) : vec3_make(1, 0, 0);
    vec3_t x_axis = vec3_normalize(vec3_cross(arbitrary, up));
    vec3_t y_axis = vec3_cross(up, x_axis);

    for (int i = 0; i <= segments; i++) {
        float t = (float)i / (float)segments;
        float angle = start_angle + t * (end_angle - start_angle);

        float cx = cosf(angle) * radius;
        float cy = sinf(angle) * radius;

        points[i] = vec3_add(center,
                             vec3_add(vec3_scale(x_axis, cx),
                                      vec3_scale(y_axis, cy)));
    }

    return points;
}

// Tessellate cubic bezier to points array (caller must free)
static inline vec3_t* ecs_scene_tessellate_bezier(
    vec3_t p0, vec3_t p1, vec3_t p2, vec3_t p3,
    int segments, int *point_count
) {
    if (segments < 2) segments = 2;
    *point_count = segments + 1;

    vec3_t *points = (vec3_t*)malloc((*point_count) * sizeof(vec3_t));
    if (!points) return NULL;

    for (int i = 0; i <= segments; i++) {
        float t = (float)i / (float)segments;
        float t2 = t * t;
        float t3 = t2 * t;
        float mt = 1.0f - t;
        float mt2 = mt * mt;
        float mt3 = mt2 * mt;

        // Cubic bezier: B(t) = (1-t)³P0 + 3(1-t)²tP1 + 3(1-t)t²P2 + t³P3
        vec3_t pt;
        pt.x = mt3 * p0.x + 3.0f * mt2 * t * p1.x + 3.0f * mt * t2 * p2.x + t3 * p3.x;
        pt.y = mt3 * p0.y + 3.0f * mt2 * t * p1.y + 3.0f * mt * t2 * p2.y + t3 * p3.y;
        pt.z = mt3 * p0.z + 3.0f * mt2 * t * p1.z + 3.0f * mt * t2 * p2.z + t3 * p3.z;
        points[i] = pt;
    }

    return points;
}

// Tessellate helix to points array (caller must free)
static inline vec3_t* ecs_scene_tessellate_helix(
    vec3_t axis_start, vec3_t axis_end,
    float radius, float turns, int segments,
    int *point_count
) {
    if (segments < 4) segments = 4;
    *point_count = segments + 1;

    vec3_t *points = (vec3_t*)malloc((*point_count) * sizeof(vec3_t));
    if (!points) return NULL;

    // Axis direction
    vec3_t axis = vec3_sub(axis_end, axis_start);
    float axis_length = vec3_length(axis);
    vec3_t axis_dir = (axis_length > 0.0001f) ? vec3_scale(axis, 1.0f / axis_length) : vec3_make(0, 1, 0);

    // Create perpendicular vectors
    vec3_t arbitrary = (fabsf(axis_dir.y) < 0.9f) ? vec3_make(0, 1, 0) : vec3_make(1, 0, 0);
    vec3_t x_axis = vec3_normalize(vec3_cross(arbitrary, axis_dir));
    vec3_t y_axis = vec3_cross(axis_dir, x_axis);

    for (int i = 0; i <= segments; i++) {
        float t = (float)i / (float)segments;
        float angle = t * turns * 2.0f * 3.14159265359f;

        // Position along axis
        vec3_t axis_pos = vec3_add(axis_start, vec3_scale(axis, t));

        // Radial offset
        float cx = cosf(angle) * radius;
        float cy = sinf(angle) * radius;
        vec3_t radial = vec3_add(vec3_scale(x_axis, cx), vec3_scale(y_axis, cy));

        points[i] = vec3_add(axis_pos, radial);
    }

    return points;
}

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

        // Set entity mapping for debug viewer
        geom_line_batch_set_entity(&scene->batches.lines, slot, (uint64_t)e, (uint8_t)GEOM_LINE);

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

// Create a polyline entity (allocates N-1 segment slots and N-2 join slots)
static inline ecs_entity_t scene_add_polyline(ecs_scene_t *scene,
                                               vec3_t *points, int point_count,
                                               vec4_t color, float width) {
    if (point_count < 2) return 0;  // Need at least 2 points

    ecs_entity_t e = ecs_world_create_entity(scene->world);

    // Create geometry component (copies points)
    GeometryComp g = geometry_comp_polyline(points, point_count, color, width);
    ecs_world_set_geometry(scene->world, e, &g);

    // Allocate CONTIGUOUS segment slots (N-1 for N points)
    int num_segments = point_count - 1;
    int first_segment_slot = geom_line_batch_alloc_contiguous(&scene->batches.lines, num_segments);

    if (first_segment_slot >= 0) {
        GeometryComp *geom = ecs_world_get_geometry(scene->world, e);
        TransformComp *t = ecs_world_get_transform(scene->world, e);
        if (geom && t) {
            for (int i = 0; i < num_segments; i++) {
                vec3_t world_a = mat4_transform_point(t->world_matrix, geom->data.polyline.points[i]);
                vec3_t world_b = mat4_transform_point(t->world_matrix, geom->data.polyline.points[i + 1]);
                geom_line_batch_set(&scene->batches.lines, first_segment_slot + i, world_a, world_b, color);
                // Set entity mapping for debug viewer
                geom_line_batch_set_entity(&scene->batches.lines, first_segment_slot + i, (uint64_t)e, (uint8_t)GEOM_POLYLINE);
            }
        }
    }

    // Allocate CONTIGUOUS join slots (N-2 for N points, at interior vertices)
    int num_joins = (point_count > 2) ? (point_count - 2) : 0;
    int first_join_slot = -1;

    if (num_joins > 0) {
        first_join_slot = geom_point_batch_alloc_contiguous(&scene->batches.points, num_joins);
        if (first_join_slot >= 0) {
            GeometryComp *geom = ecs_world_get_geometry(scene->world, e);
            TransformComp *t = ecs_world_get_transform(scene->world, e);
            if (geom && t) {
                for (int i = 0; i < num_joins; i++) {
                    // Join is at interior vertex i+1 (vertices 1 to N-2)
                    vec3_t world_pos = mat4_transform_point(t->world_matrix, geom->data.polyline.points[i + 1]);
                    geom_point_batch_set(&scene->batches.points, first_join_slot + i, world_pos, color);
                    // Set entity mapping for debug viewer
                    geom_point_batch_set_entity(&scene->batches.points, first_join_slot + i, (uint64_t)e, (uint8_t)GEOM_POLYLINE);
                }
            }
        }
    }

    // Update renderable with slot info
    RenderableComp *r = ecs_world_get_renderable(scene->world, e);
    if (r) {
        r->batch_id = GEOM_POLYLINE;
        r->instance_slot = (first_segment_slot >= 0) ? (uint32_t)first_segment_slot : 0xFFFFFFFF;
        r->segment_count = (uint32_t)num_segments;
        r->join_slot_start = (first_join_slot >= 0) ? (uint32_t)first_join_slot : 0xFFFFFFFF;
        r->join_count = (uint32_t)num_joins;
        r->instance_dirty = false;  // Already set
    }

    return e;
}

// Create a polygon entity (closed polyline: N segments and N joins for N points)
static inline ecs_entity_t scene_add_polygon(ecs_scene_t *scene,
                                              vec3_t *points, int point_count,
                                              vec4_t color, float width) {
    if (point_count < 3) return 0;  // Need at least 3 points for a polygon

    ecs_entity_t e = ecs_world_create_entity(scene->world);

    // Create geometry component (copies points)
    GeometryComp g = geometry_comp_polygon(points, point_count, color, width);
    ecs_world_set_geometry(scene->world, e, &g);

    // Allocate CONTIGUOUS segment slots (N for N points - includes closing segment)
    int num_segments = point_count;
    int first_segment_slot = geom_line_batch_alloc_contiguous(&scene->batches.lines, num_segments);

    if (first_segment_slot >= 0) {
        GeometryComp *geom = ecs_world_get_geometry(scene->world, e);
        TransformComp *t = ecs_world_get_transform(scene->world, e);
        if (geom && t) {
            for (int i = 0; i < num_segments; i++) {
                int next = (i + 1) % point_count;
                vec3_t world_a = mat4_transform_point(t->world_matrix, geom->data.polygon.points[i]);
                vec3_t world_b = mat4_transform_point(t->world_matrix, geom->data.polygon.points[next]);
                geom_line_batch_set(&scene->batches.lines, first_segment_slot + i, world_a, world_b, color);
                // Set entity mapping for debug viewer
                geom_line_batch_set_entity(&scene->batches.lines, first_segment_slot + i, (uint64_t)e, (uint8_t)GEOM_POLYGON);
            }
        }
    }

    // Allocate CONTIGUOUS join slots (N for N points - all vertices get joins in closed polygon)
    int num_joins = point_count;
    int first_join_slot = geom_point_batch_alloc_contiguous(&scene->batches.points, num_joins);

    if (first_join_slot >= 0) {
        GeometryComp *geom = ecs_world_get_geometry(scene->world, e);
        TransformComp *t = ecs_world_get_transform(scene->world, e);
        if (geom && t) {
            for (int i = 0; i < num_joins; i++) {
                vec3_t world_pos = mat4_transform_point(t->world_matrix, geom->data.polygon.points[i]);
                geom_point_batch_set(&scene->batches.points, first_join_slot + i, world_pos, color);
                // Set entity mapping for debug viewer
                geom_point_batch_set_entity(&scene->batches.points, first_join_slot + i, (uint64_t)e, (uint8_t)GEOM_POLYGON);
            }
        }
    }

    // Update renderable with slot info
    RenderableComp *r = ecs_world_get_renderable(scene->world, e);
    if (r) {
        r->batch_id = GEOM_POLYGON;
        r->instance_slot = (first_segment_slot >= 0) ? (uint32_t)first_segment_slot : 0xFFFFFFFF;
        r->segment_count = (uint32_t)num_segments;
        r->join_slot_start = (first_join_slot >= 0) ? (uint32_t)first_join_slot : 0xFFFFFFFF;
        r->join_count = (uint32_t)num_joins;
        r->instance_dirty = false;
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

        // Set entity mapping for debug viewer
        geom_point_batch_set_entity(&scene->batches.points, slot, (uint64_t)e, (uint8_t)GEOM_POINT);

        RenderableComp *r = ecs_world_get_renderable(scene->world, e);
        if (r) {
            r->batch_id = GEOM_POINT;
            r->instance_slot = (uint32_t)slot;
            r->instance_dirty = false;
        }
    }

    return e;
}

// Create an arc entity (tessellated to polyline segments)
static inline ecs_entity_t scene_add_arc(ecs_scene_t *scene,
                                          vec3_t center, float radius,
                                          float start_angle, float end_angle,
                                          vec3_t normal,
                                          vec4_t color, float width) {
    // Tessellate arc to points
    int point_count = 0;
    vec3_t *points = ecs_scene_tessellate_arc(center, radius, start_angle, end_angle, normal, &point_count);
    if (!points || point_count < 2) {
        if (points) free(points);
        return 0;
    }

    ecs_entity_t e = ecs_world_create_entity(scene->world);

    GeometryComp g = geometry_comp_arc(center, radius, start_angle, end_angle, normal, color, width);
    ecs_world_set_geometry(scene->world, e, &g);

    // Allocate CONTIGUOUS segment slots (N-1 for N points)
    int num_segments = point_count - 1;
    int first_segment_slot = geom_line_batch_alloc_contiguous(&scene->batches.lines, num_segments);

    if (first_segment_slot >= 0) {
        TransformComp *t = ecs_world_get_transform(scene->world, e);
        if (t) {
            for (int i = 0; i < num_segments; i++) {
                vec3_t world_a = mat4_transform_point(t->world_matrix, points[i]);
                vec3_t world_b = mat4_transform_point(t->world_matrix, points[i + 1]);
                geom_line_batch_set(&scene->batches.lines, first_segment_slot + i, world_a, world_b, color);
                // Set entity mapping for debug viewer
                geom_line_batch_set_entity(&scene->batches.lines, first_segment_slot + i, (uint64_t)e, (uint8_t)GEOM_ARC);
            }
        }
    }

    // Allocate CONTIGUOUS join slots (N-2 for N points)
    int num_joins = (point_count > 2) ? (point_count - 2) : 0;
    int first_join_slot = -1;

    if (num_joins > 0) {
        first_join_slot = geom_point_batch_alloc_contiguous(&scene->batches.points, num_joins);
        if (first_join_slot >= 0) {
            TransformComp *t = ecs_world_get_transform(scene->world, e);
            if (t) {
                for (int i = 0; i < num_joins; i++) {
                    vec3_t world_pos = mat4_transform_point(t->world_matrix, points[i + 1]);
                    geom_point_batch_set(&scene->batches.points, first_join_slot + i, world_pos, color);
                    // Set entity mapping for debug viewer
                    geom_point_batch_set_entity(&scene->batches.points, first_join_slot + i, (uint64_t)e, (uint8_t)GEOM_ARC);
                }
            }
        }
    }

    // Update renderable with slot info
    RenderableComp *r = ecs_world_get_renderable(scene->world, e);
    if (r) {
        r->batch_id = GEOM_ARC;
        r->instance_slot = (first_segment_slot >= 0) ? (uint32_t)first_segment_slot : 0xFFFFFFFF;
        r->segment_count = (uint32_t)num_segments;
        r->join_slot_start = (first_join_slot >= 0) ? (uint32_t)first_join_slot : 0xFFFFFFFF;
        r->join_count = (uint32_t)num_joins;
        r->instance_dirty = false;
    }

    free(points);
    return e;
}

// Create a bezier curve entity (tessellated to polyline segments)
static inline ecs_entity_t scene_add_bezier(ecs_scene_t *scene,
                                             vec3_t p0, vec3_t p1, vec3_t p2, vec3_t p3,
                                             int segments,
                                             vec4_t color, float width) {
    if (segments < 2) segments = ECS_SCENE_BEZIER_DEFAULT_SEGMENTS;

    // Tessellate bezier to points
    int point_count = 0;
    vec3_t *points = ecs_scene_tessellate_bezier(p0, p1, p2, p3, segments, &point_count);
    if (!points || point_count < 2) {
        if (points) free(points);
        return 0;
    }

    ecs_entity_t e = ecs_world_create_entity(scene->world);

    GeometryComp g = geometry_comp_bezier(p0, p1, p2, p3, segments, color, width);
    ecs_world_set_geometry(scene->world, e, &g);

    // Allocate CONTIGUOUS segment slots (N-1 for N points)
    int num_segments = point_count - 1;
    int first_segment_slot = geom_line_batch_alloc_contiguous(&scene->batches.lines, num_segments);

    if (first_segment_slot >= 0) {
        TransformComp *t = ecs_world_get_transform(scene->world, e);
        if (t) {
            for (int i = 0; i < num_segments; i++) {
                vec3_t world_a = mat4_transform_point(t->world_matrix, points[i]);
                vec3_t world_b = mat4_transform_point(t->world_matrix, points[i + 1]);
                geom_line_batch_set(&scene->batches.lines, first_segment_slot + i, world_a, world_b, color);
                // Set entity mapping for debug viewer
                geom_line_batch_set_entity(&scene->batches.lines, first_segment_slot + i, (uint64_t)e, (uint8_t)GEOM_BEZIER);
            }
        }
    }

    // Allocate CONTIGUOUS join slots (N-2 for N points)
    int num_joins = (point_count > 2) ? (point_count - 2) : 0;
    int first_join_slot = -1;

    if (num_joins > 0) {
        first_join_slot = geom_point_batch_alloc_contiguous(&scene->batches.points, num_joins);
        if (first_join_slot >= 0) {
            TransformComp *t = ecs_world_get_transform(scene->world, e);
            if (t) {
                for (int i = 0; i < num_joins; i++) {
                    vec3_t world_pos = mat4_transform_point(t->world_matrix, points[i + 1]);
                    geom_point_batch_set(&scene->batches.points, first_join_slot + i, world_pos, color);
                    // Set entity mapping for debug viewer
                    geom_point_batch_set_entity(&scene->batches.points, first_join_slot + i, (uint64_t)e, (uint8_t)GEOM_BEZIER);
                }
            }
        }
    }

    // Update renderable with slot info
    RenderableComp *r = ecs_world_get_renderable(scene->world, e);
    if (r) {
        r->batch_id = GEOM_BEZIER;
        r->instance_slot = (first_segment_slot >= 0) ? (uint32_t)first_segment_slot : 0xFFFFFFFF;
        r->segment_count = (uint32_t)num_segments;
        r->join_slot_start = (first_join_slot >= 0) ? (uint32_t)first_join_slot : 0xFFFFFFFF;
        r->join_count = (uint32_t)num_joins;
        r->instance_dirty = false;
    }

    free(points);
    return e;
}

// Create a helix entity (tessellated to polyline segments)
static inline ecs_entity_t scene_add_helix(ecs_scene_t *scene,
                                            vec3_t axis_start, vec3_t axis_end,
                                            float radius, float turns, int segments,
                                            vec4_t color, float width) {
    if (segments < 4) segments = ECS_SCENE_HELIX_DEFAULT_SEGMENTS;

    // Tessellate helix to points
    int point_count = 0;
    vec3_t *points = ecs_scene_tessellate_helix(axis_start, axis_end, radius, turns, segments, &point_count);
    if (!points || point_count < 2) {
        if (points) free(points);
        return 0;
    }

    ecs_entity_t e = ecs_world_create_entity(scene->world);

    GeometryComp g = geometry_comp_helix(axis_start, axis_end, radius, turns, segments, color, width);
    ecs_world_set_geometry(scene->world, e, &g);

    // Allocate CONTIGUOUS segment slots (N-1 for N points)
    int num_segments = point_count - 1;
    int first_segment_slot = geom_line_batch_alloc_contiguous(&scene->batches.lines, num_segments);

    if (first_segment_slot >= 0) {
        TransformComp *t = ecs_world_get_transform(scene->world, e);
        if (t) {
            for (int i = 0; i < num_segments; i++) {
                vec3_t world_a = mat4_transform_point(t->world_matrix, points[i]);
                vec3_t world_b = mat4_transform_point(t->world_matrix, points[i + 1]);
                geom_line_batch_set(&scene->batches.lines, first_segment_slot + i, world_a, world_b, color);
                // Set entity mapping for debug viewer
                geom_line_batch_set_entity(&scene->batches.lines, first_segment_slot + i, (uint64_t)e, (uint8_t)GEOM_HELIX);
            }
        }
    }

    // Allocate CONTIGUOUS join slots (N-2 for N points)
    int num_joins = (point_count > 2) ? (point_count - 2) : 0;
    int first_join_slot = -1;

    if (num_joins > 0) {
        first_join_slot = geom_point_batch_alloc_contiguous(&scene->batches.points, num_joins);
        if (first_join_slot >= 0) {
            TransformComp *t = ecs_world_get_transform(scene->world, e);
            if (t) {
                for (int i = 0; i < num_joins; i++) {
                    vec3_t world_pos = mat4_transform_point(t->world_matrix, points[i + 1]);
                    geom_point_batch_set(&scene->batches.points, first_join_slot + i, world_pos, color);
                    // Set entity mapping for debug viewer
                    geom_point_batch_set_entity(&scene->batches.points, first_join_slot + i, (uint64_t)e, (uint8_t)GEOM_HELIX);
                }
            }
        }
    }

    // Update renderable with slot info
    RenderableComp *r = ecs_world_get_renderable(scene->world, e);
    if (r) {
        r->batch_id = GEOM_HELIX;
        r->instance_slot = (first_segment_slot >= 0) ? (uint32_t)first_segment_slot : 0xFFFFFFFF;
        r->segment_count = (uint32_t)num_segments;
        r->join_slot_start = (first_join_slot >= 0) ? (uint32_t)first_join_slot : 0xFFFFFFFF;
        r->join_count = (uint32_t)num_joins;
        r->instance_dirty = false;
    }

    free(points);
    return e;
}

//------------------------------------------------------------------------------
// Entity Deletion
//------------------------------------------------------------------------------

// Internal helper to free instance slots for an entity (without deleting from ECS)
static inline void scene_free_entity_slots(ecs_scene_t *scene, ecs_entity_t e) {
    RenderableComp *r = ecs_world_get_renderable(scene->world, e);
    if (r && r->instance_slot != 0xFFFFFFFF) {
        // Free the instance slot(s) based on batch type
        switch (r->batch_id) {
            case GEOM_LINE:
                geom_line_batch_free(&scene->batches.lines, (int)r->instance_slot);
                break;
            case GEOM_POINT:
                geom_point_batch_free(&scene->batches.points, (int)r->instance_slot);
                break;
            case GEOM_POLYLINE:
            case GEOM_ARC:
            case GEOM_POLYGON:
            case GEOM_HELIX:
            case GEOM_BEZIER:
                // Free all segment slots
                for (uint32_t i = 0; i < r->segment_count; i++) {
                    geom_line_batch_free(&scene->batches.lines, (int)(r->instance_slot + i));
                }
                // Free all join slots
                if (r->join_slot_start != 0xFFFFFFFF) {
                    for (uint32_t i = 0; i < r->join_count; i++) {
                        geom_point_batch_free(&scene->batches.points, (int)(r->join_slot_start + i));
                    }
                }
                break;
            default:
                break;
        }
    }
}

// Recursively remove an entity and all its children
static inline void scene_remove_entity(ecs_scene_t *scene, ecs_entity_t e) {
    if (!ecs_is_alive(scene->world->world, e)) return;

    // First, recursively delete all children
    // We need to collect children first because deleting modifies the hierarchy
    ecs_entity_t children[64];
    int child_count = ecs_world_get_children(scene->world, e, children, 64);

    for (int i = 0; i < child_count; i++) {
        scene_remove_entity(scene, children[i]);
    }

    // Free this entity's instance buffer slots
    scene_free_entity_slots(scene, e);

    // Delete entity (also frees pick ID and geometry allocations)
    ecs_world_delete_entity(scene->world, e);
}

//------------------------------------------------------------------------------
// Parent-Child Relationship API
//------------------------------------------------------------------------------

// Set parent of an entity (pass 0 to unparent)
static inline void scene_set_parent(ecs_scene_t *scene, ecs_entity_t child, ecs_entity_t parent) {
    ecs_world_set_parent(scene->world, child, parent);
}

// Get parent of an entity (returns 0 if no parent)
static inline ecs_entity_t scene_get_parent(ecs_scene_t *scene, ecs_entity_t child) {
    return ecs_world_get_parent(scene->world, child);
}

// Check if entity has a parent
static inline bool scene_has_parent(ecs_scene_t *scene, ecs_entity_t e) {
    return ecs_world_has_parent(scene->world, e);
}

// Get children of an entity (returns count, fills out_children up to max_count)
static inline int scene_get_children(ecs_scene_t *scene, ecs_entity_t parent,
                                      ecs_entity_t *out_children, int max_count) {
    return ecs_world_get_children(scene->world, parent, out_children, max_count);
}

// Check if entity has any children
static inline bool scene_has_children(ecs_scene_t *scene, ecs_entity_t e) {
    return ecs_world_has_children(scene->world, e);
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
// Hierarchical Transform Update (internal helper)
//------------------------------------------------------------------------------

// Recursively update transforms for an entity and all its children
// Must be called on parents before children to ensure correct world matrices
// When called recursively for children, forces update even if child's local transform isn't dirty
// because the parent's world_matrix changed
static inline void ecs_scene_update_transform_recursive(ecs_scene_t *scene, ecs_entity_t e,
                                                         const mat4_t *parent_world) {
    ecs_world_state_t *w = scene->world;

    // Get this entity's transform
    TransformComp *t = (TransformComp*)ecs_get_id(w->world, e, w->TransformComp_id);
    if (!t) return;

    // Force the transform to be dirty so it updates
    // This is needed because even if local transform didn't change,
    // the world_matrix needs recalculating if parent's world_matrix changed
    t->dirty = true;

    // Update this entity's transform
    transform_comp_update_with_parent(t, parent_world);

    // Mark renderable dirty so GPU data gets updated
    RenderableComp *r = (RenderableComp*)ecs_get_id(w->world, e, w->RenderableComp_id);
    if (r) {
        r->instance_dirty = true;
    }

    // Recursively update children - they need updating since this entity's
    // world_matrix may have changed
    ecs_entity_t children[64];
    int child_count = ecs_world_get_children(w, e, children, 64);

    for (int i = 0; i < child_count; i++) {
        ecs_scene_update_transform_recursive(scene, children[i], &t->world_matrix);
    }
}

// Update all transforms in the scene respecting parent-child hierarchy
static inline void ecs_scene_update_transforms(ecs_scene_t *scene) {
    ecs_world_state_t *w = scene->world;

    // Query all entities with TransformComp
    ecs_query_t *q = ecs_query(w->world, {
        .terms = {
            { .id = w->TransformComp_id }
        }
    });

    // Process all dirty transforms
    ecs_iter_t it = ecs_query_iter(w->world, q);
    while (ecs_query_next(&it)) {
        TransformComp *transforms = ecs_field(&it, TransformComp, 0);

        for (int i = 0; i < it.count; i++) {
            ecs_entity_t e = it.entities[i];
            TransformComp *t = &transforms[i];

            // Only process if dirty
            if (!t->dirty) continue;

            // Check if this entity has a parent
            ecs_entity_t parent = ecs_get_parent(w->world, e);

            if (parent == 0) {
                // Root entity - update it and all descendants
                ecs_scene_update_transform_recursive(scene, e, NULL);
            } else {
                // Entity has a parent - get parent's world matrix
                TransformComp *parent_t = (TransformComp*)ecs_get_id(w->world, parent, w->TransformComp_id);
                if (parent_t) {
                    // If parent is also dirty, it will be processed by its own iteration
                    // and will recursively update this entity. But if parent is NOT dirty,
                    // we need to update this entity using parent's current world_matrix
                    if (!parent_t->dirty) {
                        ecs_scene_update_transform_recursive(scene, e, &parent_t->world_matrix);
                    }
                    // If parent IS dirty, this entity will be updated when parent is processed
                } else {
                    // Parent has no transform, treat as root
                    ecs_scene_update_transform_recursive(scene, e, NULL);
                }
            }
        }
    }
    ecs_query_fini(q);
}

//------------------------------------------------------------------------------
// Scene Update (sync dirty entities to GPU)
//------------------------------------------------------------------------------

static inline void ecs_scene_update(ecs_scene_t *scene) {
    ecs_world_state_t *w = scene->world;

    // First, update all transforms respecting hierarchy
    ecs_scene_update_transforms(scene);

    // Get theme-aware hover and selection colors once per frame
    // Use defaults in case ImGui not ready
    vec4_t hover_color = vec4_make(1.0f, 1.0f, 0.0f, 1.0f);      // Yellow
    vec4_t selection_color = vec4_make(1.0f, 0.5f, 0.0f, 1.0f);  // Orange

    // Try to get theme colors (may fail if ImGui not ready)
    ImGuiStyle* style = igGetStyle();
    if (style) {
        ImVec4 hover_col = style->Colors[ImGuiCol_HeaderHovered];
        hover_color = vec4_make(hover_col.x, hover_col.y, hover_col.z, 1.0f);

        ImVec4 select_col = style->Colors[ImGuiCol_HeaderActive];
        selection_color = vec4_make(select_col.x, select_col.y, select_col.z, 1.0f);
    }

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
            ecs_entity_t e = it.entities[i];
            RenderableComp *r = &renderables[i];
            if (!r->instance_dirty) continue;
            if (r->instance_slot == 0xFFFFFFFF) continue;  // No slot allocated

            GeometryComp *g = &geoms[i];
            TransformComp *t = &transforms[i];

            // Handle visibility: hidden entities get degenerate instance data
            if (!r->visible) {
                // Set degenerate instance data to effectively hide the entity
                vec3_t zero = vec3_make(0.0f, 0.0f, 0.0f);
                vec3_t far_away = vec3_make(1e10f, 1e10f, 1e10f);
                vec4_t invisible = vec4_make(0.0f, 0.0f, 0.0f, 0.0f);

                switch (g->type) {
                    case GEOM_LINE:
                        geom_line_batch_set(&scene->batches.lines, (int)r->instance_slot,
                                            zero, zero, invisible);
                        break;
                    case GEOM_POINT:
                        geom_point_batch_set(&scene->batches.points, (int)r->instance_slot,
                                             far_away, invisible);
                        break;
                    case GEOM_POLYLINE:
                    case GEOM_ARC:
                    case GEOM_POLYGON:
                    case GEOM_HELIX:
                    case GEOM_BEZIER:
                        // Hide all segment slots
                        for (uint32_t s = 0; s < r->segment_count; s++) {
                            geom_line_batch_set(&scene->batches.lines,
                                                (int)(r->instance_slot + s),
                                                zero, zero, invisible);
                        }
                        // Hide all join slots
                        if (r->join_slot_start != 0xFFFFFFFF) {
                            for (uint32_t j = 0; j < r->join_count; j++) {
                                geom_point_batch_set(&scene->batches.points,
                                                     (int)(r->join_slot_start + j),
                                                     far_away, invisible);
                            }
                        }
                        break;
                    default:
                        break;
                }
                r->instance_dirty = false;
                continue;  // Skip to next entity
            }

            // Determine render color: Selected > Hovered > Normal
            vec4_t render_color = g->color;
            if (ecs_has_id(w->world, e, w->Selected_tag)) {
                render_color = selection_color;
            } else if (ecs_has_id(w->world, e, w->Hovered_tag)) {
                render_color = hover_color;
            }

            // Update instance data based on geometry type
            switch (g->type) {
                case GEOM_LINE: {
                    vec3_t world_a = mat4_transform_point(t->world_matrix, g->data.line.a);
                    vec3_t world_b = mat4_transform_point(t->world_matrix, g->data.line.b);
                    geom_line_batch_set(&scene->batches.lines, (int)r->instance_slot,
                                        world_a, world_b, render_color);
                    break;
                }
                case GEOM_POINT: {
                    vec3_t world_pos = mat4_transform_point(t->world_matrix, g->data.point.point);
                    geom_point_batch_set(&scene->batches.points, (int)r->instance_slot,
                                         world_pos, render_color);
                    break;
                }
                case GEOM_POLYLINE: {
                    // Update all segment slots
                    int point_count = g->data.polyline.count;
                    for (int s = 0; s < point_count - 1 && s < (int)r->segment_count; s++) {
                        vec3_t world_a = mat4_transform_point(t->world_matrix, g->data.polyline.points[s]);
                        vec3_t world_b = mat4_transform_point(t->world_matrix, g->data.polyline.points[s + 1]);
                        geom_line_batch_set(&scene->batches.lines,
                                            (int)(r->instance_slot + (uint32_t)s),
                                            world_a, world_b, render_color);
                    }
                    // Update all join slots (at interior vertices)
                    if (r->join_slot_start != 0xFFFFFFFF) {
                        for (int j = 0; j < point_count - 2 && j < (int)r->join_count; j++) {
                            vec3_t world_pos = mat4_transform_point(t->world_matrix, g->data.polyline.points[j + 1]);
                            geom_point_batch_set(&scene->batches.points,
                                                 (int)(r->join_slot_start + (uint32_t)j),
                                                 world_pos, render_color);
                        }
                    }
                    break;
                }
                case GEOM_ARC: {
                    // Tessellate arc to points and update slots
                    int arc_point_count = 0;
                    vec3_t *arc_points = ecs_scene_tessellate_arc(
                        g->data.arc.center, g->data.arc.radius,
                        g->data.arc.start_angle, g->data.arc.end_angle,
                        g->data.arc.normal, &arc_point_count
                    );
                    if (arc_points && arc_point_count >= 2) {
                        // Update segment slots
                        for (int s = 0; s < arc_point_count - 1 && s < (int)r->segment_count; s++) {
                            vec3_t world_a = mat4_transform_point(t->world_matrix, arc_points[s]);
                            vec3_t world_b = mat4_transform_point(t->world_matrix, arc_points[s + 1]);
                            geom_line_batch_set(&scene->batches.lines,
                                                (int)(r->instance_slot + (uint32_t)s),
                                                world_a, world_b, render_color);
                        }
                        // Update join slots
                        if (r->join_slot_start != 0xFFFFFFFF) {
                            for (int j = 0; j < arc_point_count - 2 && j < (int)r->join_count; j++) {
                                vec3_t world_pos = mat4_transform_point(t->world_matrix, arc_points[j + 1]);
                                geom_point_batch_set(&scene->batches.points,
                                                     (int)(r->join_slot_start + (uint32_t)j),
                                                     world_pos, render_color);
                            }
                        }
                        free(arc_points);
                    }
                    break;
                }
                case GEOM_POLYGON: {
                    // Update all segment slots (closed polygon: N segments for N points)
                    int point_count = g->data.polygon.count;
                    for (int s = 0; s < point_count && s < (int)r->segment_count; s++) {
                        int next = (s + 1) % point_count;
                        vec3_t world_a = mat4_transform_point(t->world_matrix, g->data.polygon.points[s]);
                        vec3_t world_b = mat4_transform_point(t->world_matrix, g->data.polygon.points[next]);
                        geom_line_batch_set(&scene->batches.lines,
                                            (int)(r->instance_slot + (uint32_t)s),
                                            world_a, world_b, render_color);
                    }
                    // Update all join slots (all vertices in closed polygon)
                    if (r->join_slot_start != 0xFFFFFFFF) {
                        for (int j = 0; j < point_count && j < (int)r->join_count; j++) {
                            vec3_t world_pos = mat4_transform_point(t->world_matrix, g->data.polygon.points[j]);
                            geom_point_batch_set(&scene->batches.points,
                                                 (int)(r->join_slot_start + (uint32_t)j),
                                                 world_pos, render_color);
                        }
                    }
                    break;
                }
                case GEOM_BEZIER: {
                    // Tessellate bezier and update slots
                    int bezier_point_count = 0;
                    vec3_t *bezier_points = ecs_scene_tessellate_bezier(
                        g->data.bezier.p0, g->data.bezier.p1,
                        g->data.bezier.p2, g->data.bezier.p3,
                        g->data.bezier.segments, &bezier_point_count
                    );
                    if (bezier_points && bezier_point_count >= 2) {
                        for (int s = 0; s < bezier_point_count - 1 && s < (int)r->segment_count; s++) {
                            vec3_t world_a = mat4_transform_point(t->world_matrix, bezier_points[s]);
                            vec3_t world_b = mat4_transform_point(t->world_matrix, bezier_points[s + 1]);
                            geom_line_batch_set(&scene->batches.lines,
                                                (int)(r->instance_slot + (uint32_t)s),
                                                world_a, world_b, render_color);
                        }
                        if (r->join_slot_start != 0xFFFFFFFF) {
                            for (int j = 0; j < bezier_point_count - 2 && j < (int)r->join_count; j++) {
                                vec3_t world_pos = mat4_transform_point(t->world_matrix, bezier_points[j + 1]);
                                geom_point_batch_set(&scene->batches.points,
                                                     (int)(r->join_slot_start + (uint32_t)j),
                                                     world_pos, render_color);
                            }
                        }
                        free(bezier_points);
                    }
                    break;
                }
                case GEOM_HELIX: {
                    // Tessellate helix and update slots
                    int helix_point_count = 0;
                    vec3_t *helix_points = ecs_scene_tessellate_helix(
                        g->data.helix.axis_start, g->data.helix.axis_end,
                        g->data.helix.radius, g->data.helix.turns,
                        g->data.helix.segments, &helix_point_count
                    );
                    if (helix_points && helix_point_count >= 2) {
                        for (int s = 0; s < helix_point_count - 1 && s < (int)r->segment_count; s++) {
                            vec3_t world_a = mat4_transform_point(t->world_matrix, helix_points[s]);
                            vec3_t world_b = mat4_transform_point(t->world_matrix, helix_points[s + 1]);
                            geom_line_batch_set(&scene->batches.lines,
                                                (int)(r->instance_slot + (uint32_t)s),
                                                world_a, world_b, render_color);
                        }
                        if (r->join_slot_start != 0xFFFFFFFF) {
                            for (int j = 0; j < helix_point_count - 2 && j < (int)r->join_count; j++) {
                                vec3_t world_pos = mat4_transform_point(t->world_matrix, helix_points[j + 1]);
                                geom_point_batch_set(&scene->batches.points,
                                                     (int)(r->join_slot_start + (uint32_t)j),
                                                     world_pos, render_color);
                            }
                        }
                        free(helix_points);
                    }
                    break;
                }
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
                case GEOM_POLYLINE: {
                    // Add all segments to pick buffer (all with same pick_id)
                    int point_count = g->data.polyline.count;
                    for (int seg = 0; seg < point_count - 1; seg++) {
                        vec3_t world_a = mat4_transform_point(t->world_matrix, g->data.polyline.points[seg]);
                        vec3_t world_b = mat4_transform_point(t->world_matrix, g->data.polyline.points[seg + 1]);
                        pick_buffer_add_line(pb, world_a, world_b, s->pick_id);
                    }
                    // Add all joins to pick buffer (for easier picking at vertices)
                    for (int j = 1; j < point_count - 1; j++) {
                        vec3_t world_pos = mat4_transform_point(t->world_matrix, g->data.polyline.points[j]);
                        pick_buffer_add_point(pb, world_pos, s->pick_id);
                    }
                    break;
                }
                case GEOM_ARC: {
                    // Tessellate arc and add segments to pick buffer
                    int arc_point_count = 0;
                    vec3_t *arc_points = ecs_scene_tessellate_arc(
                        g->data.arc.center, g->data.arc.radius,
                        g->data.arc.start_angle, g->data.arc.end_angle,
                        g->data.arc.normal, &arc_point_count
                    );
                    if (arc_points && arc_point_count >= 2) {
                        for (int seg = 0; seg < arc_point_count - 1; seg++) {
                            vec3_t world_a = mat4_transform_point(t->world_matrix, arc_points[seg]);
                            vec3_t world_b = mat4_transform_point(t->world_matrix, arc_points[seg + 1]);
                            pick_buffer_add_line(pb, world_a, world_b, s->pick_id);
                        }
                        for (int j = 1; j < arc_point_count - 1; j++) {
                            vec3_t world_pos = mat4_transform_point(t->world_matrix, arc_points[j]);
                            pick_buffer_add_point(pb, world_pos, s->pick_id);
                        }
                        free(arc_points);
                    }
                    break;
                }
                case GEOM_POLYGON: {
                    // Add all segments (closed polygon: N segments for N points)
                    int point_count = g->data.polygon.count;
                    for (int seg = 0; seg < point_count; seg++) {
                        int next = (seg + 1) % point_count;
                        vec3_t world_a = mat4_transform_point(t->world_matrix, g->data.polygon.points[seg]);
                        vec3_t world_b = mat4_transform_point(t->world_matrix, g->data.polygon.points[next]);
                        pick_buffer_add_line(pb, world_a, world_b, s->pick_id);
                    }
                    // Add all vertices as pick points
                    for (int j = 0; j < point_count; j++) {
                        vec3_t world_pos = mat4_transform_point(t->world_matrix, g->data.polygon.points[j]);
                        pick_buffer_add_point(pb, world_pos, s->pick_id);
                    }
                    break;
                }
                case GEOM_BEZIER: {
                    // Tessellate bezier and add segments to pick buffer
                    int bezier_point_count = 0;
                    vec3_t *bezier_points = ecs_scene_tessellate_bezier(
                        g->data.bezier.p0, g->data.bezier.p1,
                        g->data.bezier.p2, g->data.bezier.p3,
                        g->data.bezier.segments, &bezier_point_count
                    );
                    if (bezier_points && bezier_point_count >= 2) {
                        for (int seg = 0; seg < bezier_point_count - 1; seg++) {
                            vec3_t world_a = mat4_transform_point(t->world_matrix, bezier_points[seg]);
                            vec3_t world_b = mat4_transform_point(t->world_matrix, bezier_points[seg + 1]);
                            pick_buffer_add_line(pb, world_a, world_b, s->pick_id);
                        }
                        for (int j = 1; j < bezier_point_count - 1; j++) {
                            vec3_t world_pos = mat4_transform_point(t->world_matrix, bezier_points[j]);
                            pick_buffer_add_point(pb, world_pos, s->pick_id);
                        }
                        free(bezier_points);
                    }
                    break;
                }
                case GEOM_HELIX: {
                    // Tessellate helix and add segments to pick buffer
                    int helix_point_count = 0;
                    vec3_t *helix_points = ecs_scene_tessellate_helix(
                        g->data.helix.axis_start, g->data.helix.axis_end,
                        g->data.helix.radius, g->data.helix.turns,
                        g->data.helix.segments, &helix_point_count
                    );
                    if (helix_points && helix_point_count >= 2) {
                        for (int seg = 0; seg < helix_point_count - 1; seg++) {
                            vec3_t world_a = mat4_transform_point(t->world_matrix, helix_points[seg]);
                            vec3_t world_b = mat4_transform_point(t->world_matrix, helix_points[seg + 1]);
                            pick_buffer_add_line(pb, world_a, world_b, s->pick_id);
                        }
                        for (int j = 1; j < helix_point_count - 1; j++) {
                            vec3_t world_pos = mat4_transform_point(t->world_matrix, helix_points[j]);
                            pick_buffer_add_point(pb, world_pos, s->pick_id);
                        }
                        free(helix_points);
                    }
                    break;
                }
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
    while (ecs_query_next(&it)) {
        SelectableComp *selectables = ecs_field(&it, SelectableComp, 0);

        for (int i = 0; i < it.count; i++) {
            if (selectables[i].pick_id == pick_id) {
                found = it.entities[i];
                // Breaking early - must call ecs_iter_fini before exiting
                ecs_iter_fini(&it);
                goto found_exit;
            }
        }
    }
    // If we get here, ecs_query_next returned false which already finalized the iterator
    // Do NOT call ecs_iter_fini again - that would be a double-finalization error

found_exit:
    ecs_query_fini(q);
    return found;
}

// Clear hovered state from all entities and mark them dirty for re-render
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
            ecs_entity_t e = it.entities[i];
            ecs_world_clear_hovered(w, e);
            // Mark as dirty so the color gets updated on next frame
            RenderableComp *r = ecs_world_get_renderable(w, e);
            if (r) {
                r->instance_dirty = true;
            }
        }
    }

    ecs_query_fini(q);
}

// Update hover state based on pick buffer result
static inline void ecs_scene_update_hover(ecs_scene_t *scene, pick_buffer_t *pb) {
    // Clear previous hover (also marks those entities as dirty)
    ecs_scene_clear_all_hovered(scene);

    // Get newly hovered entity
    uint32_t pick_id = pick_buffer_get_hovered_id(pb);
    if (pick_id != 0) {
        ecs_entity_t e = ecs_scene_find_entity_by_pick_id(scene, pick_id);
        if (e != 0) {
            ecs_world_set_hovered(scene->world, e);

            // Mark newly hovered entity as dirty so it gets the hover color
            RenderableComp *r = ecs_world_get_renderable(scene->world, e);
            if (r) {
                r->instance_dirty = true;
            }
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
    while (ecs_query_next(&it)) {
        if (it.count > 0) {
            hovered = it.entities[0];  // Return first hovered (should be only one)
            // Breaking early - must call ecs_iter_fini before exiting
            ecs_iter_fini(&it);
            break;
        }
    }
    // If we didn't break (hovered is still 0), ecs_query_next returned false
    // which already finalized the iterator - do NOT call ecs_iter_fini again

    ecs_query_fini(q);
    return hovered;
}

#endif // ECS_SCENE_H
