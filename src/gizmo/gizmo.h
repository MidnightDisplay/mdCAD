//------------------------------------------------------------------------------
// gizmo.h - 3D manipulator gizmo (header-only)
//
// Self-contained plain-C system with own GPU resources, pick IDs, and
// interaction state machine. Two modes: transform (entity) and geometry (vertex).
//------------------------------------------------------------------------------
#ifndef GIZMO_H
#define GIZMO_H

#include "../math3d.h"
#include "../math/math_interaction.h"
#include "../components/selectable_comp.h"
#include "../components/geometry_comp.h"
#include "../components/transform_comp.h"
#include "../components/renderable_comp.h"
#include "../gpu/pick_buffer.h"
#include "../selection.h"
#include "gizmo_rendering.h"
#include "gizmo_vertex_mode.h"

#include <math.h>

//------------------------------------------------------------------------------
// Handle IDs and enums
//------------------------------------------------------------------------------
typedef enum {
    GIZMO_HANDLE_NONE = -1,
    GIZMO_HANDLE_X = 0,
    GIZMO_HANDLE_Y = 1,
    GIZMO_HANDLE_Z = 2,
    GIZMO_HANDLE_XY = 3,
    GIZMO_HANDLE_XZ = 4,
    GIZMO_HANDLE_YZ = 5,
} gizmo_handle_t;

typedef enum {
    GIZMO_MODE_HIDDEN,
    GIZMO_MODE_IDLE,
    GIZMO_MODE_HOVER,
    GIZMO_MODE_DRAGGING,
} gizmo_mode_t;

typedef enum {
    GIZMO_TRANSFORM_MODE,   // Move whole entities
    GIZMO_GEOMETRY_MODE,    // Move individual vertices
} gizmo_edit_mode_t;

//------------------------------------------------------------------------------
// Gizmo state
//------------------------------------------------------------------------------
typedef struct {
    gizmo_rendering_t rendering;
    gizmo_vertex_mode_t vertex_mode;
    gizmo_edit_mode_t edit_mode;
    gizmo_mode_t mode;
    gizmo_handle_t hovered_handle;
    gizmo_handle_t active_handle;
    int hovered_vertex;                 // Geometry mode: which vertex hovered (-1 = none)
    vec3_t center;                      // Gizmo world position
    float scale;                        // Current world-space scale for constant screen size

    // Drag state
    vec3_t drag_origin;                 // Gizmo center at drag start (fixed reference)
    float drag_start_t;                 // Axis: parameter at drag start
    vec3_t drag_start_hit;              // Plane: hit point at drag start
    vec3_t drag_accumulated;            // Total delta applied so far

    // Transform handle line slots (in rendering buffer)
    // 3 axis lines + 12 plane handle lines (4 per plane) = 15 lines
    // 3 axis tip points + 3 plane center points = 6 points (initially just tips)
    int axis_line_slots[3];             // X, Y, Z axis lines
    int plane_line_slots[3][4];         // XY, XZ, YZ plane handle outlines (4 lines each)
    int tip_point_slots[3];             // X, Y, Z tip dots

    // Colors
    float axis_colors[3][3];            // RGB per axis: [X][r,g,b], [Y][...], [Z][...]
    float plane_colors[3][3];           // RGB per plane handle
} gizmo_t;

//------------------------------------------------------------------------------
// Axis/plane definitions
//------------------------------------------------------------------------------
static inline vec3_t gizmo_axis_dir(int axis) {
    switch (axis) {
        case 0: return vec3_make(1, 0, 0);
        case 1: return vec3_make(0, 1, 0);
        case 2: return vec3_make(0, 0, 1);
        default: return vec3_make(0, 0, 0);
    }
}

// Plane normal for plane handles: XY->Z, XZ->Y, YZ->X
static inline vec3_t gizmo_plane_normal(int plane_idx) {
    switch (plane_idx) {
        case 0: return vec3_make(0, 0, 1);  // XY plane -> Z normal
        case 1: return vec3_make(0, 1, 0);  // XZ plane -> Y normal
        case 2: return vec3_make(1, 0, 0);  // YZ plane -> X normal
        default: return vec3_make(0, 0, 0);
    }
}

// Two axes that define a plane handle
static inline void gizmo_plane_axes(int plane_idx, int *a1, int *a2) {
    switch (plane_idx) {
        case 0: *a1 = 0; *a2 = 1; break;  // XY
        case 1: *a1 = 0; *a2 = 2; break;  // XZ
        case 2: *a1 = 1; *a2 = 2; break;  // YZ
        default: *a1 = 0; *a2 = 1; break;
    }
}

//------------------------------------------------------------------------------
// Init
//------------------------------------------------------------------------------
static inline void gizmo_init(gizmo_t *g) {
    memset(g, 0, sizeof(*g));
    g->mode = GIZMO_MODE_HIDDEN;
    g->edit_mode = GIZMO_TRANSFORM_MODE;
    g->hovered_handle = GIZMO_HANDLE_NONE;
    g->active_handle = GIZMO_HANDLE_NONE;
    g->hovered_vertex = -1;

    // Default axis colors: red, green, blue
    g->axis_colors[0][0] = 1.0f; g->axis_colors[0][1] = 0.2f; g->axis_colors[0][2] = 0.2f;
    g->axis_colors[1][0] = 0.2f; g->axis_colors[1][1] = 1.0f; g->axis_colors[1][2] = 0.2f;
    g->axis_colors[2][0] = 0.2f; g->axis_colors[2][1] = 0.2f; g->axis_colors[2][2] = 1.0f;

    // Plane handle colors: blended
    g->plane_colors[0][0] = 0.8f; g->plane_colors[0][1] = 0.8f; g->plane_colors[0][2] = 0.2f; // XY = yellow
    g->plane_colors[1][0] = 0.8f; g->plane_colors[1][1] = 0.2f; g->plane_colors[1][2] = 0.8f; // XZ = magenta
    g->plane_colors[2][0] = 0.2f; g->plane_colors[2][1] = 0.8f; g->plane_colors[2][2] = 0.8f; // YZ = cyan

    gizmo_rendering_init(&g->rendering);
    gizmo_vertex_mode_init(&g->vertex_mode);
}

//------------------------------------------------------------------------------
// Build transform handle geometry (3 axis lines + 3 tip points + 3 plane rects)
//------------------------------------------------------------------------------
static inline void gizmo_build_transform_handles(gizmo_t *g) {
    float s = g->scale;
    vec3_t c = g->center;

    // Axis lines: center → tip
    for (int i = 0; i < 3; i++) {
        vec3_t dir = gizmo_axis_dir(i);
        vec3_t tip = vec3_add(c, vec3_scale(dir, s));
        float *col = g->axis_colors[i];
        float brightness = 1.0f;
        // Highlight hovered axis
        if (g->hovered_handle == (gizmo_handle_t)i) brightness = 1.5f;
        if (g->active_handle == (gizmo_handle_t)i) brightness = 1.5f;
        float r = fminf(col[0] * brightness, 1.0f);
        float gv = fminf(col[1] * brightness, 1.0f);
        float b = fminf(col[2] * brightness, 1.0f);
        g->axis_line_slots[i] = gizmo_rendering_add_line(&g->rendering, c, tip, r, gv, b, 1.0f);
    }

    // Tip points at end of each axis
    for (int i = 0; i < 3; i++) {
        vec3_t dir = gizmo_axis_dir(i);
        vec3_t tip = vec3_add(c, vec3_scale(dir, s));
        float *col = g->axis_colors[i];
        float brightness = 1.0f;
        if (g->hovered_handle == (gizmo_handle_t)i) brightness = 1.5f;
        if (g->active_handle == (gizmo_handle_t)i) brightness = 1.5f;
        float r = fminf(col[0] * brightness, 1.0f);
        float gv = fminf(col[1] * brightness, 1.0f);
        float b = fminf(col[2] * brightness, 1.0f);
        g->tip_point_slots[i] = gizmo_rendering_add_point(&g->rendering, tip, r, gv, b, 1.0f);
    }

    // Plane handle rectangles: small squares offset along two axes
    float plane_offset = s * 0.25f;
    float plane_size = s * 0.15f;
    for (int p = 0; p < 3; p++) {
        int a1, a2;
        gizmo_plane_axes(p, &a1, &a2);
        vec3_t d1 = gizmo_axis_dir(a1);
        vec3_t d2 = gizmo_axis_dir(a2);

        // Square corners
        vec3_t base = vec3_add(c, vec3_add(vec3_scale(d1, plane_offset), vec3_scale(d2, plane_offset)));
        vec3_t p0 = base;
        vec3_t p1 = vec3_add(base, vec3_scale(d1, plane_size));
        vec3_t p2 = vec3_add(base, vec3_add(vec3_scale(d1, plane_size), vec3_scale(d2, plane_size)));
        vec3_t p3 = vec3_add(base, vec3_scale(d2, plane_size));

        float *col = g->plane_colors[p];
        float brightness = 1.0f;
        if (g->hovered_handle == (gizmo_handle_t)(GIZMO_HANDLE_XY + p)) brightness = 1.5f;
        if (g->active_handle == (gizmo_handle_t)(GIZMO_HANDLE_XY + p)) brightness = 1.5f;
        float r = fminf(col[0] * brightness, 1.0f);
        float gv = fminf(col[1] * brightness, 1.0f);
        float bv = fminf(col[2] * brightness, 1.0f);

        g->plane_line_slots[p][0] = gizmo_rendering_add_line(&g->rendering, p0, p1, r, gv, bv, 1.0f);
        g->plane_line_slots[p][1] = gizmo_rendering_add_line(&g->rendering, p1, p2, r, gv, bv, 1.0f);
        g->plane_line_slots[p][2] = gizmo_rendering_add_line(&g->rendering, p2, p3, r, gv, bv, 1.0f);
        g->plane_line_slots[p][3] = gizmo_rendering_add_line(&g->rendering, p3, p0, r, gv, bv, 1.0f);
    }
}

//------------------------------------------------------------------------------
// Update gizmo each frame
//------------------------------------------------------------------------------
static inline void gizmo_update(gizmo_t *g,
                                  const selection_buffer_t *selection,
                                  void *scene_ptr,  // ecs_scene_t*
                                  vec3_t cam_pos,
                                  float fov,
                                  float ecs_point_size) {
    // Import ecs_scene_t through opaque pointer to avoid circular includes
    // The caller passes scene as void*, we cast it here knowing the layout
    typedef struct { void *world; } scene_header_t;
    #define SCENE_WORLD(s) (((scene_header_t*)(s))->world)

    gizmo_rendering_clear(&g->rendering);

    // Check if we have a selection
    if (selection->count == 0) {
        if (g->vertex_mode.active) gizmo_vertex_mode_exit(&g->vertex_mode);
        g->mode = GIZMO_MODE_HIDDEN;
        return;
    }

    // Compute gizmo center based on mode
    ecs_world_state_t *world = (ecs_world_state_t*)SCENE_WORLD(scene_ptr);

    if (g->edit_mode == GIZMO_GEOMETRY_MODE && g->vertex_mode.active) {
        // Geometry mode: center at selected vertices
        ecs_entity_t entity = (ecs_entity_t)g->vertex_mode.target_entity;
        const GeometryComp *geom = ecs_world_get_geometry(world, entity);
        const TransformComp *xform = ecs_world_get_transform(world, entity);
        if (geom && xform) {
            if (g->vertex_mode.selected_count > 0) {
                g->center = gizmo_vertex_mode_get_center(&g->vertex_mode, geom, xform->world_matrix);
            } else {
                // No vertices selected — center at entity position
                g->center = xform->world_matrix.m[12] != 0 || xform->world_matrix.m[13] != 0 || xform->world_matrix.m[14] != 0
                    ? vec3_make(xform->world_matrix.m[12], xform->world_matrix.m[13], xform->world_matrix.m[14])
                    : xform->position;
            }
        }
    } else {
        // Transform mode: center at average of selected entity positions
        vec3_t sum = vec3_make(0, 0, 0);
        for (int i = 0; i < selection->count; i++) {
            ecs_entity_t e = selection->entities[i];
            const TransformComp *xform = ecs_world_get_transform(world, e);
            if (xform) {
                sum = vec3_add(sum, vec3_make(xform->world_matrix.m[12],
                                               xform->world_matrix.m[13],
                                               xform->world_matrix.m[14]));
            }
        }
        g->center = vec3_scale(sum, 1.0f / (float)selection->count);
    }

    // Constant screen size: scale = dist * tan(fov/2) * 0.15
    float dist = vec3_length(vec3_sub(cam_pos, g->center));
    g->scale = dist * tanf(fov * 0.5f) * 0.15f;
    if (g->scale < 0.001f) g->scale = 0.001f;

    // Set mode to idle if not dragging
    if (g->mode != GIZMO_MODE_DRAGGING) {
        g->mode = (g->hovered_handle != GIZMO_HANDLE_NONE)
            ? GIZMO_MODE_HOVER : GIZMO_MODE_IDLE;
    }

    // Build transform handle geometry
    gizmo_build_transform_handles(g);

    // In geometry mode, add vertex overlay dots
    if (g->edit_mode == GIZMO_GEOMETRY_MODE && g->vertex_mode.active) {
        ecs_entity_t entity = (ecs_entity_t)g->vertex_mode.target_entity;
        const GeometryComp *geom = ecs_world_get_geometry(world, entity);
        const TransformComp *xform = ecs_world_get_transform(world, entity);
        if (geom && xform) {
            gizmo_vertex_mode_update(&g->vertex_mode, &g->rendering,
                                     geom, xform->world_matrix,
                                     g->hovered_vertex, ecs_point_size);
        }
    }

    #undef SCENE_WORLD
}

//------------------------------------------------------------------------------
// Set edit mode (switch between transform / geometry)
//------------------------------------------------------------------------------
static inline void gizmo_set_edit_mode(gizmo_t *g,
                                         gizmo_edit_mode_t mode,
                                         void *scene_ptr,
                                         const selection_buffer_t *selection) {
    typedef struct { void *world; } scene_header_t;
    ecs_world_state_t *world = (ecs_world_state_t*)(((scene_header_t*)scene_ptr)->world);

    if (mode == GIZMO_GEOMETRY_MODE) {
        // Enter geometry mode for first selected entity
        if (selection->count > 0) {
            ecs_entity_t entity = selection->entities[0];
            const GeometryComp *geom = ecs_world_get_geometry(world, entity);
            if (geom && gizmo_vertex_mode_enter(&g->vertex_mode, (uint64_t)entity, geom)) {
                g->edit_mode = GIZMO_GEOMETRY_MODE;
                // Auto-select first vertex
                gizmo_vertex_mode_select(&g->vertex_mode, 0, false, false);
            }
            // If geometry type not supported, stay in transform mode
        }
    } else {
        // Return to transform mode
        if (g->vertex_mode.active) gizmo_vertex_mode_exit(&g->vertex_mode);
        g->edit_mode = GIZMO_TRANSFORM_MODE;
    }
}

//------------------------------------------------------------------------------
// Populate pick buffer with gizmo handles + vertex dots
//------------------------------------------------------------------------------
static inline void gizmo_populate_pick_buffer(gizmo_t *g,
                                                pick_buffer_t *pb,
                                                void *scene_ptr) {
    if (g->mode == GIZMO_MODE_HIDDEN) return;

    typedef struct { void *world; } scene_header_t;
    ecs_world_state_t *world = (ecs_world_state_t*)(((scene_header_t*)scene_ptr)->world);

    float s = g->scale;
    vec3_t c = g->center;

    // Add axis lines to pick buffer (overlay — depth-always, on top)
    for (int i = 0; i < 3; i++) {
        vec3_t dir = gizmo_axis_dir(i);
        vec3_t tip = vec3_add(c, vec3_scale(dir, s));
        uint32_t pick_id = GIZMO_HANDLE_BASE + (uint32_t)i;
        pick_buffer_add_overlay_line(pb, c, tip, pick_id);
    }

    // Add plane handle lines to pick buffer (overlay)
    float plane_offset = s * 0.25f;
    float plane_size = s * 0.15f;
    for (int p = 0; p < 3; p++) {
        int a1, a2;
        gizmo_plane_axes(p, &a1, &a2);
        vec3_t d1 = gizmo_axis_dir(a1);
        vec3_t d2 = gizmo_axis_dir(a2);

        vec3_t base = vec3_add(c, vec3_add(vec3_scale(d1, plane_offset), vec3_scale(d2, plane_offset)));
        vec3_t p0 = base;
        vec3_t p1 = vec3_add(base, vec3_scale(d1, plane_size));
        vec3_t p2 = vec3_add(base, vec3_add(vec3_scale(d1, plane_size), vec3_scale(d2, plane_size)));
        vec3_t p3 = vec3_add(base, vec3_scale(d2, plane_size));

        uint32_t pick_id = GIZMO_HANDLE_BASE + (uint32_t)(GIZMO_HANDLE_XY + p);
        pick_buffer_add_overlay_line(pb, p0, p1, pick_id);
        pick_buffer_add_overlay_line(pb, p1, p2, pick_id);
        pick_buffer_add_overlay_line(pb, p2, p3, pick_id);
        pick_buffer_add_overlay_line(pb, p3, p0, pick_id);
    }

    // Add tip points to pick buffer (overlay)
    for (int i = 0; i < 3; i++) {
        vec3_t dir = gizmo_axis_dir(i);
        vec3_t tip = vec3_add(c, vec3_scale(dir, s));
        uint32_t pick_id = GIZMO_HANDLE_BASE + (uint32_t)i;
        pick_buffer_add_overlay_point(pb, tip, pick_id);
    }

    // In geometry mode, add vertex handles to pick buffer
    if (g->edit_mode == GIZMO_GEOMETRY_MODE && g->vertex_mode.active) {
        ecs_entity_t entity = (ecs_entity_t)g->vertex_mode.target_entity;
        const GeometryComp *geom = ecs_world_get_geometry(world, entity);
        const TransformComp *xform = ecs_world_get_transform(world, entity);
        if (geom && xform) {
            gizmo_vertex_mode_populate_pick(&g->vertex_mode, pb, geom, xform->world_matrix);
        }
    }
}

//------------------------------------------------------------------------------
// Handle hover from pick result
//------------------------------------------------------------------------------
static inline void gizmo_handle_hover(gizmo_t *g, uint32_t pick_id) {
    g->hovered_handle = GIZMO_HANDLE_NONE;
    g->hovered_vertex = -1;

    if (g->mode == GIZMO_MODE_HIDDEN || g->mode == GIZMO_MODE_DRAGGING) return;

    if (pick_id >= VERTEX_HANDLE_BASE && pick_id < VERTEX_HANDLE_BASE + VERTEX_HANDLE_MAX) {
        g->hovered_vertex = (int)(pick_id - VERTEX_HANDLE_BASE);
    } else if (pick_id >= GIZMO_HANDLE_BASE && pick_id < GIZMO_HANDLE_BASE + GIZMO_HANDLE_COUNT) {
        int handle = (int)(pick_id - GIZMO_HANDLE_BASE);
        if (handle <= GIZMO_HANDLE_YZ) {
            g->hovered_handle = (gizmo_handle_t)handle;
        }
    }
}

//------------------------------------------------------------------------------
// Drag operations
//------------------------------------------------------------------------------

// Begin drag on the currently hovered handle
static inline bool gizmo_begin_drag(gizmo_t *g, ray_t mouse_ray) {
    if (g->hovered_handle == GIZMO_HANDLE_NONE) return false;

    g->active_handle = g->hovered_handle;
    g->mode = GIZMO_MODE_DRAGGING;
    g->drag_accumulated = vec3_make(0, 0, 0);
    g->drag_origin = g->center;  // Fixed reference point for entire drag

    if (g->active_handle <= GIZMO_HANDLE_Z) {
        // Axis constrained: record initial parameter t
        vec3_t axis = gizmo_axis_dir(g->active_handle);
        g->drag_start_t = mdcad_interaction_ray_axis_closest_t(mouse_ray, g->drag_origin, axis);
    } else {
        // Plane constrained: record initial hit point
        int plane_idx = g->active_handle - GIZMO_HANDLE_XY;
        vec3_t normal = gizmo_plane_normal(plane_idx);
        vec3_t hit;
        float t = 0.0f;

        if (!mdcad_interaction_ray_plane_intersect(mouse_ray, g->drag_origin, normal, &hit, &t) || t < 0.0f) {
            return false;
        }
        g->drag_start_hit = hit;
    }

    return true;
}

// Update drag, returns incremental world-space delta.
// Caller owns constrained interaction policy (projection/blocking via scene solver).
static inline vec3_t gizmo_update_drag(gizmo_t *g, ray_t mouse_ray) {
    if (g->mode != GIZMO_MODE_DRAGGING) return vec3_make(0, 0, 0);

    vec3_t delta = vec3_make(0, 0, 0);

    if (g->active_handle <= GIZMO_HANDLE_Z) {
        // Axis constrained — always use fixed drag_origin as axis reference
        vec3_t axis = gizmo_axis_dir(g->active_handle);
        float t = mdcad_interaction_ray_axis_closest_t(mouse_ray, g->drag_origin, axis);
        float dt = t - g->drag_start_t;
        vec3_t total = vec3_scale(axis, dt);
        delta = vec3_sub(total, g->drag_accumulated);
        g->drag_accumulated = total;
    } else {
        // Plane constrained — always use fixed drag_origin as plane reference
        int plane_idx = g->active_handle - GIZMO_HANDLE_XY;
        vec3_t normal = gizmo_plane_normal(plane_idx);
        vec3_t hit;
        float t = 0.0f;

        if (mdcad_interaction_ray_plane_intersect(mouse_ray, g->drag_origin, normal, &hit, &t) && t >= 0.0f) {
            vec3_t total = vec3_sub(hit, g->drag_start_hit);
            delta = vec3_sub(total, g->drag_accumulated);
            g->drag_accumulated = total;
        }
    }

    // Move gizmo center by delta (visual only — entities are moved by caller)
    g->center = vec3_add(g->drag_origin, g->drag_accumulated);

    return delta;
}

// End drag, returns total world-space delta
static inline vec3_t gizmo_end_drag(gizmo_t *g) {
    vec3_t total = g->drag_accumulated;
    g->active_handle = GIZMO_HANDLE_NONE;
    g->mode = GIZMO_MODE_IDLE;
    g->drag_accumulated = vec3_make(0, 0, 0);
    return total;
}

//------------------------------------------------------------------------------
// Shutdown
//------------------------------------------------------------------------------
static inline void gizmo_shutdown(gizmo_t *g) {
    gizmo_vertex_mode_shutdown(&g->vertex_mode);
    gizmo_rendering_shutdown(&g->rendering);
}

#endif // GIZMO_H
