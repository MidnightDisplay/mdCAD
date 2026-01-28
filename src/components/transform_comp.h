//------------------------------------------------------------------------------
// transform_comp.h - Transform component for ECS entities
//------------------------------------------------------------------------------
#ifndef TRANSFORM_COMP_H
#define TRANSFORM_COMP_H

#include "component_types.h"
#include <stdbool.h>

typedef struct {
    vec3_t position;      // World position
    vec3_t rotation;      // Euler angles (radians)
    vec3_t scale;         // Non-uniform scale
    mat4_t world_matrix;  // Cached world transform (computed by system)
    bool dirty;           // Needs recalculation
} TransformComp;

// Initialize transform to identity
static inline TransformComp transform_comp_default(void) {
    return (TransformComp){
        .position = { 0.0f, 0.0f, 0.0f },
        .rotation = { 0.0f, 0.0f, 0.0f },
        .scale = { 1.0f, 1.0f, 1.0f },
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

// Recalculate world matrix from position, rotation, scale
static inline void transform_comp_update(TransformComp *t) {
    if (!t->dirty) return;

    // Build matrix: T * Rz * Ry * Rx * S
    mat4_t translation = mat4_translate(t->position.x, t->position.y, t->position.z);
    mat4_t rot_x = mat4_rotate_x(t->rotation.x);
    mat4_t rot_y = mat4_rotate_y(t->rotation.y);
    mat4_t rot_z = mat4_rotate_z(t->rotation.z);
    mat4_t scale = mat4_scale(t->scale.x, t->scale.y, t->scale.z);

    // Combine: T * Rz * Ry * Rx * S
    mat4_t rotation = mat4_mul(rot_z, mat4_mul(rot_y, rot_x));
    t->world_matrix = mat4_mul(translation, mat4_mul(rotation, scale));
    t->dirty = false;
}

#endif // TRANSFORM_COMP_H
