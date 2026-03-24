#ifndef MDCAD_MATH_COMPARE_H
#define MDCAD_MATH_COMPARE_H

#include "cglm_entry.h"
#include "../math3d.h"

#include <math.h>
#include <stdbool.h>

static inline bool mdcad_compare_float_close(float lhs, float rhs, float epsilon) {
    return fabsf(lhs - rhs) <= epsilon;
}

static inline bool mdcad_compare_vec3_close(vec3_t legacy, const float candidate[3], float epsilon) {
    return mdcad_compare_float_close(legacy.x, candidate[0], epsilon)
        && mdcad_compare_float_close(legacy.y, candidate[1], epsilon)
        && mdcad_compare_float_close(legacy.z, candidate[2], epsilon);
}

static inline bool mdcad_compare_mat4_close(mat4_t legacy, const float candidate[16], float epsilon) {
    for (int i = 0; i < 16; ++i) {
        if (!mdcad_compare_float_close(legacy.m[i], candidate[i], epsilon)) {
            return false;
        }
    }
    return true;
}

static inline bool mdcad_compare_ray_close(ray_t legacy, const float candidate_origin[3], const float candidate_direction[3], float epsilon) {
    return mdcad_compare_vec3_close(legacy.origin, candidate_origin, epsilon)
        && mdcad_compare_vec3_close(legacy.direction, candidate_direction, epsilon);
}

#endif
