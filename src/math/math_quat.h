#ifndef MDCAD_MATH_QUAT_H
#define MDCAD_MATH_QUAT_H

#include "cglm_entry.h"
#include "../math3d.h"

#include <math.h>

typedef struct {
    float x;
    float y;
    float z;
    float w;
} mdcad_quat_t;

static inline mdcad_quat_t mdcad_quat_make(float x, float y, float z, float w) {
    return (mdcad_quat_t){ x, y, z, w };
}

static inline versors mdcad_quat_to_versors(mdcad_quat_t quat) {
    return glms_quat_init(quat.x, quat.y, quat.z, quat.w);
}

static inline mdcad_quat_t mdcad_quat_from_versors(versors quat) {
    return mdcad_quat_make(quat.raw[0], quat.raw[1], quat.raw[2], quat.raw[3]);
}

static inline mdcad_quat_t mdcad_quat_from_axis_angle(vec3_t axis, float radians) {
    float axis_len = vec3_length(axis);
    vec3_t axis_normalized;
    versors quat;

    if (!isfinite(radians) || !isfinite(axis_len) || axis_len <= 1e-8f) {
        return mdcad_quat_make(0.0f, 0.0f, 0.0f, 1.0f);
    }

    axis_normalized = vec3_scale(axis, 1.0f / axis_len);
    quat = glms_quat(radians, axis_normalized.x, axis_normalized.y, axis_normalized.z);
    quat = glms_quat_normalize(quat);
    return mdcad_quat_from_versors(quat);
}

static inline mdcad_quat_t mdcad_quat_normalize(mdcad_quat_t quat_value) {
    versors quat = mdcad_quat_to_versors(quat_value);
    float norm = glms_quat_norm(quat);

    if (!isfinite(norm) || norm <= 1e-8f) {
        return mdcad_quat_make(0.0f, 0.0f, 0.0f, 1.0f);
    }

    return mdcad_quat_from_versors(glms_quat_normalize(quat));
}

static inline vec3_t mdcad_quat_rotate_vec3(mdcad_quat_t quat_value, vec3_t vector) {
    versors quat = mdcad_quat_to_versors(mdcad_quat_normalize(quat_value));
    vec3s vec = (vec3s){ { vector.x, vector.y, vector.z } };
    vec3s rotated = glms_quat_rotatev(quat, vec);

    if (!isfinite(rotated.raw[0]) || !isfinite(rotated.raw[1]) || !isfinite(rotated.raw[2])) {
        return vec3_make(0.0f, 0.0f, 0.0f);
    }

    return vec3_make(rotated.raw[0], rotated.raw[1], rotated.raw[2]);
}

static inline mdcad_quat_t mdcad_quat_mul(mdcad_quat_t lhs, mdcad_quat_t rhs) {
    versors lhs_q = mdcad_quat_to_versors(lhs);
    versors rhs_q = mdcad_quat_to_versors(rhs);
    versors product = glms_quat_mul(lhs_q, rhs_q);

    return mdcad_quat_from_versors(product);
}

#endif
