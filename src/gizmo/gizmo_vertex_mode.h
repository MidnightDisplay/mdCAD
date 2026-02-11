//------------------------------------------------------------------------------
// gizmo_vertex_mode.h - Geometry mode: vertex handles, selection, editing
//
// Manages vertex overlay dots for the selected entity when in geometry edit
// mode. Reads positions from GeometryComp, renders as overlay points.
//------------------------------------------------------------------------------
#ifndef GIZMO_VERTEX_MODE_H
#define GIZMO_VERTEX_MODE_H

#include "../math3d.h"
#include "../components/geometry_comp.h"
#include "../components/transform_comp.h"
#include "../components/renderable_comp.h"
#include "../components/selectable_comp.h"
#include "gizmo_rendering.h"

#include <stdlib.h>
#include <string.h>

// Forward declaration
struct ecs_scene_t;

//------------------------------------------------------------------------------
// Types
//------------------------------------------------------------------------------
typedef struct {
    uint64_t target_entity;         // Entity being edited (0 = none)
    int vertex_count;               // Total vertices in geometry
    int *selected_vertices;         // Indices of selected vertices
    int selected_count;
    int selected_capacity;
    int point_slot_start;           // First slot in gizmo point buffer
    bool active;
} gizmo_vertex_mode_t;

//------------------------------------------------------------------------------
// Helpers: get vertex positions from GeometryComp
//------------------------------------------------------------------------------

// Get number of editable vertices for a geometry type
static inline int gizmo_vertex_mode_get_vertex_count(const GeometryComp *geom) {
    switch (geom->type) {
        case GEOM_POINT:    return 1;
        case GEOM_LINE:     return 2;
        case GEOM_POLYLINE: return geom->data.polyline.count;
        case GEOM_POLYGON:  return geom->data.polygon.count;
        case GEOM_TRIANGLE: return 3;
        case GEOM_MESH:     return geom->data.mesh.vertex_count;
        default:            return 0;  // Not supported for geometry mode
    }
}

// Get vertex position in local space by index
static inline vec3_t gizmo_vertex_mode_get_local_pos(const GeometryComp *geom, int idx) {
    switch (geom->type) {
        case GEOM_POINT:    return geom->data.point.point;
        case GEOM_LINE:     return (idx == 0) ? geom->data.line.a : geom->data.line.b;
        case GEOM_POLYLINE: return geom->data.polyline.points[idx];
        case GEOM_POLYGON:  return geom->data.polygon.points[idx];
        case GEOM_TRIANGLE:
            if (idx == 0) return geom->data.triangle.a;
            if (idx == 1) return geom->data.triangle.b;
            return geom->data.triangle.c;
        case GEOM_MESH:
            if (idx >= 0 && idx < geom->data.mesh.vertex_count)
                return geom->data.mesh.vertices[idx];
            return vec3_make(0, 0, 0);
        default:            return vec3_make(0, 0, 0);
    }
}

// Set vertex position in local space by index
static inline void gizmo_vertex_mode_set_local_pos(GeometryComp *geom, int idx, vec3_t pos) {
    switch (geom->type) {
        case GEOM_POINT:    geom->data.point.point = pos; break;
        case GEOM_LINE:
            if (idx == 0) geom->data.line.a = pos;
            else          geom->data.line.b = pos;
            break;
        case GEOM_POLYLINE: geom->data.polyline.points[idx] = pos; break;
        case GEOM_POLYGON:  geom->data.polygon.points[idx] = pos;  break;
        case GEOM_TRIANGLE:
            if (idx == 0) geom->data.triangle.a = pos;
            else if (idx == 1) geom->data.triangle.b = pos;
            else geom->data.triangle.c = pos;
            break;
        case GEOM_MESH:
            if (idx >= 0 && idx < geom->data.mesh.vertex_count)
                geom->data.mesh.vertices[idx] = pos;
            break;
        default: break;
    }
}

//------------------------------------------------------------------------------
// Init / Shutdown
//------------------------------------------------------------------------------
static inline void gizmo_vertex_mode_init(gizmo_vertex_mode_t *vm) {
    memset(vm, 0, sizeof(*vm));
    vm->selected_capacity = 16;
    vm->selected_vertices = (int*)malloc(vm->selected_capacity * sizeof(int));
}

static inline void gizmo_vertex_mode_shutdown(gizmo_vertex_mode_t *vm) {
    free(vm->selected_vertices);
    memset(vm, 0, sizeof(*vm));
}

//------------------------------------------------------------------------------
// Enter / Exit geometry mode for an entity
//------------------------------------------------------------------------------
static inline bool gizmo_vertex_mode_enter(gizmo_vertex_mode_t *vm,
                                            uint64_t entity,
                                            const GeometryComp *geom) {
    if (!geom) return false;
    int vc = gizmo_vertex_mode_get_vertex_count(geom);
    if (vc == 0) return false;
    if (vc > VERTEX_HANDLE_MAX) vc = VERTEX_HANDLE_MAX;

    vm->target_entity = entity;
    vm->vertex_count = vc;
    vm->selected_count = 0;
    vm->point_slot_start = -1;  // Will be set during update
    vm->active = true;
    return true;
}

static inline void gizmo_vertex_mode_exit(gizmo_vertex_mode_t *vm) {
    vm->target_entity = 0;
    vm->vertex_count = 0;
    vm->selected_count = 0;
    vm->point_slot_start = -1;
    vm->active = false;
}

//------------------------------------------------------------------------------
// Vertex selection
//------------------------------------------------------------------------------
static inline bool gizmo_vertex_mode_is_selected(const gizmo_vertex_mode_t *vm, int idx) {
    for (int i = 0; i < vm->selected_count; i++) {
        if (vm->selected_vertices[i] == idx) return true;
    }
    return false;
}

static inline void gizmo_vertex_mode_select(gizmo_vertex_mode_t *vm, int vertex_idx,
                                              bool shift, bool ctrl) {
    if (vertex_idx < 0 || vertex_idx >= vm->vertex_count) return;

    if (ctrl) {
        // Toggle
        bool found = false;
        for (int i = 0; i < vm->selected_count; i++) {
            if (vm->selected_vertices[i] == vertex_idx) {
                // Remove by shifting
                for (int j = i; j < vm->selected_count - 1; j++)
                    vm->selected_vertices[j] = vm->selected_vertices[j + 1];
                vm->selected_count--;
                found = true;
                break;
            }
        }
        if (!found) {
            // Add
            if (vm->selected_count >= vm->selected_capacity) {
                vm->selected_capacity *= 2;
                vm->selected_vertices = (int*)realloc(vm->selected_vertices,
                    vm->selected_capacity * sizeof(int));
            }
            vm->selected_vertices[vm->selected_count++] = vertex_idx;
        }
    } else if (shift) {
        // Add to selection (no toggle)
        if (!gizmo_vertex_mode_is_selected(vm, vertex_idx)) {
            if (vm->selected_count >= vm->selected_capacity) {
                vm->selected_capacity *= 2;
                vm->selected_vertices = (int*)realloc(vm->selected_vertices,
                    vm->selected_capacity * sizeof(int));
            }
            vm->selected_vertices[vm->selected_count++] = vertex_idx;
        }
    } else {
        // Replace selection
        vm->selected_count = 1;
        vm->selected_vertices[0] = vertex_idx;
    }
}

//------------------------------------------------------------------------------
// Get center of selected vertices (world space)
//------------------------------------------------------------------------------
static inline vec3_t gizmo_vertex_mode_get_center(const gizmo_vertex_mode_t *vm,
                                                    const GeometryComp *geom,
                                                    mat4_t world_matrix) {
    if (vm->selected_count == 0) return vec3_make(0, 0, 0);
    vec3_t sum = vec3_make(0, 0, 0);
    for (int i = 0; i < vm->selected_count; i++) {
        vec3_t local = gizmo_vertex_mode_get_local_pos(geom, vm->selected_vertices[i]);
        vec3_t world = mat4_mul_point(world_matrix, local);
        sum = vec3_add(sum, world);
    }
    return vec3_scale(sum, 1.0f / (float)vm->selected_count);
}

//------------------------------------------------------------------------------
// Apply delta to selected vertices (delta in world space, stored in local space)
//------------------------------------------------------------------------------
static inline void gizmo_vertex_mode_apply_delta(gizmo_vertex_mode_t *vm,
                                                   GeometryComp *geom,
                                                   mat4_t world_matrix,
                                                   vec3_t world_delta) {
    // Inverse transform delta to local space
    // For translation-only transforms, we can use inverse of upper-left 3x3
    mat4_t inv = mat4_inverse(world_matrix);
    // Transform delta direction (not point) - zero out translation
    vec3_t local_delta = vec3_make(
        inv.m[0] * world_delta.x + inv.m[4] * world_delta.y + inv.m[8]  * world_delta.z,
        inv.m[1] * world_delta.x + inv.m[5] * world_delta.y + inv.m[9]  * world_delta.z,
        inv.m[2] * world_delta.x + inv.m[6] * world_delta.y + inv.m[10] * world_delta.z
    );

    for (int i = 0; i < vm->selected_count; i++) {
        int idx = vm->selected_vertices[i];
        vec3_t pos = gizmo_vertex_mode_get_local_pos(geom, idx);
        gizmo_vertex_mode_set_local_pos(geom, idx, vec3_add(pos, local_delta));
    }
}

//------------------------------------------------------------------------------
// Update vertex overlay (populate gizmo point buffer with vertex dots)
//------------------------------------------------------------------------------
static inline void gizmo_vertex_mode_update(gizmo_vertex_mode_t *vm,
                                              gizmo_rendering_t *gr,
                                              const GeometryComp *geom,
                                              mat4_t world_matrix,
                                              int hovered_vertex,
                                              float point_size_unused) {
    if (!vm->active || !geom) return;
    (void)point_size_unused;

    // Colors for vertex dots
    const float normal_r = 1.0f, normal_g = 1.0f, normal_b = 1.0f;
    const float selected_r = 1.0f, selected_g = 0.8f, selected_b = 0.0f;
    const float hovered_r = 0.5f, hovered_g = 1.0f, hovered_b = 0.5f;

    vm->point_slot_start = gr->point_count;

    for (int i = 0; i < vm->vertex_count; i++) {
        vec3_t local = gizmo_vertex_mode_get_local_pos(geom, i);
        vec3_t world = mat4_mul_point(world_matrix, local);

        float r, g, b;
        if (gizmo_vertex_mode_is_selected(vm, i)) {
            r = selected_r; g = selected_g; b = selected_b;
        } else if (i == hovered_vertex) {
            r = hovered_r; g = hovered_g; b = hovered_b;
        } else {
            r = normal_r; g = normal_g; b = normal_b;
        }
        gizmo_rendering_add_point(gr, world, r, g, b, 1.0f);
    }
}

//------------------------------------------------------------------------------
// Populate pick buffer with vertex handles
//------------------------------------------------------------------------------
static inline void gizmo_vertex_mode_populate_pick(const gizmo_vertex_mode_t *vm,
                                                     pick_buffer_t *pb,
                                                     const GeometryComp *geom,
                                                     mat4_t world_matrix) {
    if (!vm->active || !geom) return;

    for (int i = 0; i < vm->vertex_count; i++) {
        vec3_t local = gizmo_vertex_mode_get_local_pos(geom, i);
        vec3_t world = mat4_mul_point(world_matrix, local);
        uint32_t pick_id = VERTEX_HANDLE_BASE + (uint32_t)i;
        pick_buffer_add_overlay_point(pb, world, pick_id);
    }
}

#endif // GIZMO_VERTEX_MODE_H
