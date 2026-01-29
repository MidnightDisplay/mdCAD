//------------------------------------------------------------------------------
// transform_comp.h - Transform component for ECS entities
//
// Supports hierarchical transforms: local transform relative to parent,
// world transform computed as parent_world * local.
//------------------------------------------------------------------------------
#ifndef TRANSFORM_COMP_H
#define TRANSFORM_COMP_H

#include "component_types.h"
#include <stdbool.h>

typedef struct {
    // Local transform (relative to parent, or world if no parent)
    vec3_t position;      // Local position
    vec3_t rotation;      // Euler angles (radians)
    vec3_t scale;         // Non-uniform scale

    // Cached matrices
    mat4_t local_matrix;  // Cached local transform matrix
    mat4_t world_matrix;  // Cached world transform (parent_world * local)

    bool dirty;           // Needs recalculation
} TransformComp;

// Initialize transform to identity
static inline TransformComp transform_comp_default(void) {
    return (TransformComp){
        .position = { 0.0f, 0.0f, 0.0f },
        .rotation = { 0.0f, 0.0f, 0.0f },
        .scale = { 1.0f, 1.0f, 1.0f },
        .local_matrix = mat4_identity(),
        .world_matrix = mat4_identity(),
        .dirty = true
    };
}

// Create transform with position only
static inline TransformComp transform_comp_position(vec3_t pos) {
    TransformComp t = transform_comp_default();
    t.position = pos;
    return t;
}

// Mark transform as needing update
static inline void transform_comp_set_dirty(TransformComp *t) {
    t->dirty = true;
}

// Compute the local matrix from position, rotation, scale
static inline mat4_t transform_comp_compute_local(const TransformComp *t) {
    // Build matrix: T * Rz * Ry * Rx * S
    mat4_t translation = mat4_translate(t->position.x, t->position.y, t->position.z);
    mat4_t rot_x = mat4_rotate_x(t->rotation.x);
    mat4_t rot_y = mat4_rotate_y(t->rotation.y);
    mat4_t rot_z = mat4_rotate_z(t->rotation.z);
    mat4_t scale_mat = mat4_scale(t->scale.x, t->scale.y, t->scale.z);

    // Combine: T * Rz * Ry * Rx * S
    mat4_t rotation = mat4_mul(rot_z, mat4_mul(rot_y, rot_x));
    return mat4_mul(translation, mat4_mul(rotation, scale_mat));
}

// Update local matrix only (when entity has no parent)
// This is the old behavior - local == world
static inline void transform_comp_update(TransformComp *t) {
    if (!t->dirty) return;

    t->local_matrix = transform_comp_compute_local(t);
    t->world_matrix = t->local_matrix;  // No parent, so world == local
    t->dirty = false;
}

// Update with parent's world matrix (for hierarchical transforms)
// world_matrix = parent_world * local
static inline void transform_comp_update_with_parent(TransformComp *t, const mat4_t *parent_world) {
    if (!t->dirty) return;

    t->local_matrix = transform_comp_compute_local(t);

    if (parent_world) {
        t->world_matrix = mat4_mul(*parent_world, t->local_matrix);
    } else {
        t->world_matrix = t->local_matrix;
    }
    t->dirty = false;
}

#endif // TRANSFORM_COMP_H
