#ifndef MDCAD_MATH_IMPORT_H
#define MDCAD_MATH_IMPORT_H

#include "cglm_entry.h"
#include "../math3d.h"

#include <math.h>
#include <stdbool.h>
#include <string.h>

static inline vec3s mdcad_import_vec3s_from_legacy(vec3_t legacy) {
    return (vec3s){ { legacy.x, legacy.y, legacy.z } };
}

static inline mat4s mdcad_import_mat4s_from_legacy(mat4_t legacy) {
    mat4s result;
    memcpy(result.raw, legacy.m, sizeof(legacy.m));
    return result;
}

static inline mat4_t mdcad_import_mat4_from_mat4s(mat4s matrix) {
    mat4_t result;
    memcpy(result.m, matrix.raw, sizeof(result.m));
    return result;
}

static inline vec3_t mdcad_import_vec3_make(float x, float y, float z) {
    return (vec3_t){ x, y, z };
}

static inline vec3_t mdcad_import_vec3_add(vec3_t lhs, vec3_t rhs) {
    vec3s result = glms_vec3_add(mdcad_import_vec3s_from_legacy(lhs),
                                 mdcad_import_vec3s_from_legacy(rhs));
    return mdcad_import_vec3_make(result.raw[0], result.raw[1], result.raw[2]);
}

static inline vec3_t mdcad_import_vec3_sub(vec3_t lhs, vec3_t rhs) {
    vec3s result = glms_vec3_sub(mdcad_import_vec3s_from_legacy(lhs),
                                 mdcad_import_vec3s_from_legacy(rhs));
    return mdcad_import_vec3_make(result.raw[0], result.raw[1], result.raw[2]);
}

static inline vec3_t mdcad_import_vec3_scale(vec3_t value, float scalar) {
    vec3s result = glms_vec3_scale(mdcad_import_vec3s_from_legacy(value), scalar);
    return mdcad_import_vec3_make(result.raw[0], result.raw[1], result.raw[2]);
}

static inline float mdcad_import_vec3_dot(vec3_t lhs, vec3_t rhs) {
    return glms_vec3_dot(mdcad_import_vec3s_from_legacy(lhs),
                         mdcad_import_vec3s_from_legacy(rhs));
}

static inline vec3_t mdcad_import_vec3_cross(vec3_t lhs, vec3_t rhs) {
    vec3s result = glms_vec3_cross(mdcad_import_vec3s_from_legacy(lhs),
                                   mdcad_import_vec3s_from_legacy(rhs));
    return mdcad_import_vec3_make(result.raw[0], result.raw[1], result.raw[2]);
}

static inline float mdcad_import_vec3_length(vec3_t value) {
    return glms_vec3_norm(mdcad_import_vec3s_from_legacy(value));
}

static inline vec3_t mdcad_import_vec3_normalize_safe(vec3_t value) {
    float length = mdcad_import_vec3_length(value);

    if (!isfinite(length) || length <= 0.0001f) {
        return mdcad_import_vec3_make(0.0f, 0.0f, 0.0f);
    }

    return mdcad_import_vec3_scale(value, 1.0f / length);
}

static inline vec3_t mdcad_import_vec3_average(vec3_t sum, int count) {
    if (count <= 0) {
        return mdcad_import_vec3_make(0.0f, 0.0f, 0.0f);
    }
    return mdcad_import_vec3_scale(sum, 1.0f / (float)count);
}

static inline mat4_t mdcad_import_mat4_identity(void) {
    mat4s identity = GLMS_MAT4_IDENTITY;
    return mdcad_import_mat4_from_mat4s(identity);
}

static inline mat4_t mdcad_import_mat4_mul(mat4_t lhs, mat4_t rhs) {
    mat4s result = glms_mat4_mul(mdcad_import_mat4s_from_legacy(lhs),
                                 mdcad_import_mat4s_from_legacy(rhs));
    return mdcad_import_mat4_from_mat4s(result);
}

static inline mat4_t mdcad_import_mat4_rotate_x(float radians) {
    mat4_t matrix = mdcad_import_mat4_identity();
    float c = cosf(radians);
    float s = sinf(radians);

    matrix.m[5] = c;
    matrix.m[9] = -s;
    matrix.m[6] = s;
    matrix.m[10] = c;
    return matrix;
}

static inline mat4_t mdcad_import_mat4_rotate_y(float radians) {
    mat4_t matrix = mdcad_import_mat4_identity();
    float c = cosf(radians);
    float s = sinf(radians);

    matrix.m[0] = c;
    matrix.m[8] = s;
    matrix.m[2] = -s;
    matrix.m[10] = c;
    return matrix;
}

static inline mat4_t mdcad_import_mat4_rotate_z(float radians) {
    mat4_t matrix = mdcad_import_mat4_identity();
    float c = cosf(radians);
    float s = sinf(radians);

    matrix.m[0] = c;
    matrix.m[4] = -s;
    matrix.m[1] = s;
    matrix.m[5] = c;
    return matrix;
}

static inline mat4_t mdcad_import_rotation_xyz(float rx, float ry, float rz) {
    mat4_t rot_x = mdcad_import_mat4_rotate_x(rx);
    mat4_t rot_y = mdcad_import_mat4_rotate_y(ry);
    mat4_t rot_z = mdcad_import_mat4_rotate_z(rz);
    mat4_t rot_xy = mdcad_import_mat4_mul(rot_y, rot_x);
    return mdcad_import_mat4_mul(rot_z, rot_xy);
}

static inline bool mdcad_import_has_rotation(float rx, float ry, float rz) {
    return rx != 0.0f || ry != 0.0f || rz != 0.0f;
}

static inline vec3_t mdcad_import_mat4_mul_point(mat4_t matrix, vec3_t point) {
    mat4s matrix_s = mdcad_import_mat4s_from_legacy(matrix);
    vec4s point4 = (vec4s){ { point.x, point.y, point.z, 1.0f } };
    vec4s transformed = glms_mat4_mulv(matrix_s, point4);

    if (!isfinite(transformed.raw[0]) || !isfinite(transformed.raw[1]) || !isfinite(transformed.raw[2])) {
        return mdcad_import_vec3_make(0.0f, 0.0f, 0.0f);
    }

    return mdcad_import_vec3_make(transformed.raw[0], transformed.raw[1], transformed.raw[2]);
}

static inline vec3_t mdcad_import_transform_point(vec3_t point,
                                                  bool shift_to_com,
                                                  vec3_t com,
                                                  bool has_rotation,
                                                  mat4_t rotation,
                                                  float scale) {
    vec3_t result = point;

    if (shift_to_com) {
        result = mdcad_import_vec3_sub(result, com);
    }
    if (has_rotation) {
        result = mdcad_import_mat4_mul_point(rotation, result);
    }
    if (scale != 1.0f) {
        result = mdcad_import_vec3_scale(result, scale);
    }

    return result;
}

static inline vec3_t mdcad_import_select_perpendicular_axis(vec3_t up) {
    return (fabsf(up.y) < 0.9f)
        ? mdcad_import_vec3_make(0.0f, 1.0f, 0.0f)
        : mdcad_import_vec3_make(1.0f, 0.0f, 0.0f);
}

static inline void mdcad_import_bounds_reset(vec3_t *min_bounds, vec3_t *max_bounds) {
    *min_bounds = mdcad_import_vec3_make(1e30f, 1e30f, 1e30f);
    *max_bounds = mdcad_import_vec3_make(-1e30f, -1e30f, -1e30f);
}

static inline void mdcad_import_bounds_expand(vec3_t *min_bounds, vec3_t *max_bounds, vec3_t point) {
    if (point.x < min_bounds->x) min_bounds->x = point.x;
    if (point.y < min_bounds->y) min_bounds->y = point.y;
    if (point.z < min_bounds->z) min_bounds->z = point.z;
    if (point.x > max_bounds->x) max_bounds->x = point.x;
    if (point.y > max_bounds->y) max_bounds->y = point.y;
    if (point.z > max_bounds->z) max_bounds->z = point.z;
}

#endif
