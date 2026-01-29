//------------------------------------------------------------------------------
// component_types.h - Common types used by ECS components
//------------------------------------------------------------------------------
#ifndef COMPONENT_TYPES_H
#define COMPONENT_TYPES_H

#include "../math3d.h"

// 4-component vector for colors
typedef struct { float x, y, z, w; } vec4_t;

// Helper to create vec4 from components
static inline vec4_t vec4_make(float x, float y, float z, float w) {
    return (vec4_t){ x, y, z, w };
}

// vec3_make is defined in math3d.h

// Transform a point by a matrix
static inline vec3_t mat4_transform_point(mat4_t m, vec3_t p) {
    float w = m.m[3] * p.x + m.m[7] * p.y + m.m[11] * p.z + m.m[15];
    return (vec3_t){
        (m.m[0] * p.x + m.m[4] * p.y + m.m[8]  * p.z + m.m[12]) / w,
        (m.m[1] * p.x + m.m[5] * p.y + m.m[9]  * p.z + m.m[13]) / w,
        (m.m[2] * p.x + m.m[6] * p.y + m.m[10] * p.z + m.m[14]) / w
    };
}

#endif // COMPONENT_TYPES_H
