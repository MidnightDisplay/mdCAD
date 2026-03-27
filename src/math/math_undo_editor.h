#ifndef MDCAD_MATH_UNDO_EDITOR_H
#define MDCAD_MATH_UNDO_EDITOR_H

#include "cglm_entry.h"
#include "../math3d.h"

#include <math.h>
#include <stdbool.h>
#include <string.h>

static inline vec3s mdcad_undo_editor_vec3s_from_legacy(vec3_t legacy) {
    return (vec3s){ { legacy.x, legacy.y, legacy.z } };
}

static inline mat4s mdcad_undo_editor_mat4s_from_legacy(mat4_t legacy) {
    mat4s result;
    memcpy(result.raw, legacy.m, sizeof(legacy.m));
    return result;
}

static inline bool mdcad_undo_editor_isfinite_vec3s(vec3s value) {
    const float *v = (const float *)value.raw;
    return isfinite(v[0]) && isfinite(v[1]) && isfinite(v[2]);
}

static inline bool mdcad_undo_editor_is_invertible(mat4s matrix) {
    mat4 raw;
    float det;

    memcpy(raw, matrix.raw, sizeof(raw));
    det = glm_mat4_det(raw);
    return isfinite(det) && fabsf(det) > 1e-8f;
}

static inline vec3_t mdcad_undo_editor_vec3_make(float x, float y, float z) {
    return (vec3_t){ x, y, z };
}

static inline vec3_t mdcad_undo_editor_vec3_add(vec3_t lhs, vec3_t rhs) {
    vec3s result = glms_vec3_add(mdcad_undo_editor_vec3s_from_legacy(lhs),
                                 mdcad_undo_editor_vec3s_from_legacy(rhs));

    if (!mdcad_undo_editor_isfinite_vec3s(result)) {
        return mdcad_undo_editor_vec3_make(0.0f, 0.0f, 0.0f);
    }
    return mdcad_undo_editor_vec3_make(result.raw[0], result.raw[1], result.raw[2]);
}

static inline vec3_t mdcad_undo_editor_vec3_scale(vec3_t value, float scalar) {
    vec3s result = glms_vec3_scale(mdcad_undo_editor_vec3s_from_legacy(value), scalar);

    if (!mdcad_undo_editor_isfinite_vec3s(result)) {
        return mdcad_undo_editor_vec3_make(0.0f, 0.0f, 0.0f);
    }
    return mdcad_undo_editor_vec3_make(result.raw[0], result.raw[1], result.raw[2]);
}

static inline vec3_t mdcad_undo_editor_world_point_from_local(mat4_t world_matrix, vec3_t local_point) {
    mat4s world_matrix_s = mdcad_undo_editor_mat4s_from_legacy(world_matrix);
    vec4s point4 = (vec4s){ { local_point.x, local_point.y, local_point.z, 1.0f } };
    vec4s world4 = glms_mat4_mulv(world_matrix_s, point4);
    float w = world4.raw[3];

    if (!isfinite(w) || fabsf(w) <= 1e-6f) {
        return mdcad_undo_editor_vec3_make(0.0f, 0.0f, 0.0f);
    }

    return mdcad_undo_editor_vec3_make(world4.raw[0] / w,
                                       world4.raw[1] / w,
                                       world4.raw[2] / w);
}

static inline vec3_t mdcad_undo_editor_world_delta_to_local(mat4_t world_matrix, vec3_t world_delta) {
    mat4s world_matrix_s = mdcad_undo_editor_mat4s_from_legacy(world_matrix);
    mat4 world_matrix_raw;
    mat4 inv_world_raw;
    mat4s inv_world_s;
    vec4s world_delta4;
    vec4s local_delta4;

    if (!isfinite(world_delta.x) || !isfinite(world_delta.y) || !isfinite(world_delta.z)) {
        return mdcad_undo_editor_vec3_make(0.0f, 0.0f, 0.0f);
    }

    if (!mdcad_undo_editor_is_invertible(world_matrix_s)) {
        return mdcad_undo_editor_vec3_make(0.0f, 0.0f, 0.0f);
    }

    memcpy(world_matrix_raw, world_matrix_s.raw, sizeof(world_matrix_raw));
    glm_mat4_inv(world_matrix_raw, inv_world_raw);
    memcpy(inv_world_s.raw, inv_world_raw, sizeof(inv_world_raw));

    world_delta4 = (vec4s){ { world_delta.x, world_delta.y, world_delta.z, 0.0f } };
    local_delta4 = glms_mat4_mulv(inv_world_s, world_delta4);

    if (!isfinite(local_delta4.raw[0]) || !isfinite(local_delta4.raw[1]) || !isfinite(local_delta4.raw[2])) {
        return mdcad_undo_editor_vec3_make(0.0f, 0.0f, 0.0f);
    }

    return mdcad_undo_editor_vec3_make(local_delta4.raw[0], local_delta4.raw[1], local_delta4.raw[2]);
}

static inline float mdcad_undo_editor_rad_to_deg(float radians) {
    return radians * (180.0f / 3.14159265358979323846f);
}

static inline float mdcad_undo_editor_deg_to_rad(float degrees) {
    return degrees * (3.14159265358979323846f / 180.0f);
}

/* Phase8 deferred glue: runtime/editor migrated touchpoints now use mdcad_undo_editor_* and
 * mdcad_interaction_* helpers; legacy math3d interaction symbols remain for compare harness and
 * non-Phase-8 runtime call-sites, and will be finalized under Phase 9 boundary work (TRED-02). */

#endif
