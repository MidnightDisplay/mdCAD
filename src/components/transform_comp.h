//------------------------------------------------------------------------------
// transform_comp.h - Transform component for ECS entities
//
// Supports hierarchical transforms: local transform relative to parent,
// world transform computed as parent_world * local.
//------------------------------------------------------------------------------
#ifndef TRANSFORM_COMP_H
#define TRANSFORM_COMP_H

#include "component_types.h"
#include "../math/cglm_entry.h"
#include <stdbool.h>
#include <string.h>

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

static inline void transform_comp_mat4_to_cglm(mat4 dest, const mat4_t *src) {
    memcpy(dest, src->m, sizeof(src->m));
}

static inline mat4_t transform_comp_mat4_from_cglm(mat4 src) {
    mat4_t dest;

    memcpy(dest.m, src, sizeof(dest.m));
    return dest;
}

// Compute the local matrix from position, rotation, scale
static inline mat4_t transform_comp_compute_local(const TransformComp *t) {
    mat4 translation;
    mat4 rot_x;
    mat4 rot_y;
    mat4 rot_z;
    mat4 scale_mat;
    mat4 rotation_yx;
    mat4 rotation_zyx;
    mat4 rotated_scaled;
    mat4 local;
    vec3 position = { t->position.x, t->position.y, t->position.z };
    vec3 scale = { t->scale.x, t->scale.y, t->scale.z };

    glm_translate_make(translation, position);
    glm_rotate_x(GLM_MAT4_IDENTITY, t->rotation.x, rot_x);
    glm_rotate_y(GLM_MAT4_IDENTITY, t->rotation.y, rot_y);
    glm_rotate_z(GLM_MAT4_IDENTITY, t->rotation.z, rot_z);
    glm_scale_make(scale_mat, scale);

    glm_mat4_mul(rot_y, rot_x, rotation_yx);
    glm_mat4_mul(rot_z, rotation_yx, rotation_zyx);
    glm_mat4_mul(rotation_zyx, scale_mat, rotated_scaled);
    glm_mat4_mul(translation, rotated_scaled, local);
    return transform_comp_mat4_from_cglm(local);
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
        mat4 parent_cglm;
        mat4 local_cglm;
        mat4 world_cglm;

        transform_comp_mat4_to_cglm(parent_cglm, parent_world);
        transform_comp_mat4_to_cglm(local_cglm, &t->local_matrix);
        glm_mat4_mul(parent_cglm, local_cglm, world_cglm);
        t->world_matrix = transform_comp_mat4_from_cglm(world_cglm);
    } else {
        t->world_matrix = t->local_matrix;
    }
    t->dirty = false;
}

#endif // TRANSFORM_COMP_H
