//------------------------------------------------------------------------------
// math3d.h - Simple 3D math library (header-only)
//------------------------------------------------------------------------------
#ifndef MATH3D_H
#define MATH3D_H

#include <math.h>

#if defined(MDCAD_MATH3D_ALLOW_INTERACTION_DEPRECATED_USAGE)
#define MDCAD_MATH3D_INTERACTION_DEPRECATED
#elif defined(_MSC_VER)
#define MDCAD_MATH3D_INTERACTION_DEPRECATED __declspec(deprecated)
#elif defined(__clang__) || defined(__GNUC__)
#define MDCAD_MATH3D_INTERACTION_DEPRECATED __attribute__((deprecated))
#else
#define MDCAD_MATH3D_INTERACTION_DEPRECATED
#endif

//------------------------------------------------------------------------------
// Types
//------------------------------------------------------------------------------
typedef struct { float x, y, z; } vec3_t;
typedef struct { float m[16]; } mat4_t;  // column-major

//------------------------------------------------------------------------------
// Vector3 functions
//------------------------------------------------------------------------------
static inline vec3_t vec3_make(float x, float y, float z) {
    return (vec3_t){ x, y, z };
}

static inline vec3_t vec3_add(vec3_t a, vec3_t b) {
    return (vec3_t){ a.x + b.x, a.y + b.y, a.z + b.z };
}

static inline vec3_t vec3_sub(vec3_t a, vec3_t b) {
    return (vec3_t){ a.x - b.x, a.y - b.y, a.z - b.z };
}

static inline vec3_t vec3_scale(vec3_t v, float s) {
    return (vec3_t){ v.x * s, v.y * s, v.z * s };
}

static inline float vec3_dot(vec3_t a, vec3_t b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static inline vec3_t vec3_cross(vec3_t a, vec3_t b) {
    return (vec3_t){
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

static inline float vec3_length(vec3_t v) {
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

static inline vec3_t vec3_normalize(vec3_t v) {
    float len = vec3_length(v);
    if (len > 0.0001f) {
        return vec3_scale(v, 1.0f / len);
    }
    return (vec3_t){ 0.0f, 0.0f, 0.0f };
}

static inline vec3_t vec3_neg(vec3_t v) {
    return (vec3_t){ -v.x, -v.y, -v.z };
}

//------------------------------------------------------------------------------
// Matrix functions
//------------------------------------------------------------------------------
static inline mat4_t mat4_identity(void) {
    mat4_t m = {0};
    m.m[0] = m.m[5] = m.m[10] = m.m[15] = 1.0f;
    return m;
}

static inline mat4_t mat4_mul(mat4_t a, mat4_t b) {
    mat4_t result = {0};
    for (int col = 0; col < 4; col++) {
        for (int row = 0; row < 4; row++) {
            result.m[col * 4 + row] =
                a.m[0 * 4 + row] * b.m[col * 4 + 0] +
                a.m[1 * 4 + row] * b.m[col * 4 + 1] +
                a.m[2 * 4 + row] * b.m[col * 4 + 2] +
                a.m[3 * 4 + row] * b.m[col * 4 + 3];
        }
    }
    return result;
}

static inline mat4_t mat4_perspective(float fovy, float aspect, float near, float far) {
    mat4_t m = {0};
    float f = 1.0f / tanf(fovy * 0.5f);
    m.m[0] = f / aspect;
    m.m[5] = f;
    m.m[10] = (far + near) / (near - far);
    m.m[11] = -1.0f;
    m.m[14] = (2.0f * far * near) / (near - far);
    return m;
}

static inline mat4_t mat4_lookat(vec3_t eye, vec3_t target, vec3_t up) {
    // Forward (from target to eye for RH coordinate system)
    vec3_t f = {
        target.x - eye.x,
        target.y - eye.y,
        target.z - eye.z
    };
    float f_len = sqrtf(f.x*f.x + f.y*f.y + f.z*f.z);
    f.x /= f_len; f.y /= f_len; f.z /= f_len;

    // Right = f x up
    vec3_t r = {
        f.y * up.z - f.z * up.y,
        f.z * up.x - f.x * up.z,
        f.x * up.y - f.y * up.x
    };
    float r_len = sqrtf(r.x*r.x + r.y*r.y + r.z*r.z);
    r.x /= r_len; r.y /= r_len; r.z /= r_len;

    // True up = r x f
    vec3_t u = {
        r.y * f.z - r.z * f.y,
        r.z * f.x - r.x * f.z,
        r.x * f.y - r.y * f.x
    };

    mat4_t m = mat4_identity();
    m.m[0] = r.x;  m.m[4] = r.y;  m.m[8]  = r.z;
    m.m[1] = u.x;  m.m[5] = u.y;  m.m[9]  = u.z;
    m.m[2] = -f.x; m.m[6] = -f.y; m.m[10] = -f.z;
    m.m[12] = -(r.x * eye.x + r.y * eye.y + r.z * eye.z);
    m.m[13] = -(u.x * eye.x + u.y * eye.y + u.z * eye.z);
    m.m[14] = (f.x * eye.x + f.y * eye.y + f.z * eye.z);
    return m;
}

static inline mat4_t mat4_rotate_x(float angle) {
    mat4_t m = mat4_identity();
    float c = cosf(angle);
    float s = sinf(angle);
    m.m[5] = c;   m.m[9] = -s;
    m.m[6] = s;   m.m[10] = c;
    return m;
}

static inline mat4_t mat4_rotate_y(float angle) {
    mat4_t m = mat4_identity();
    float c = cosf(angle);
    float s = sinf(angle);
    m.m[0] = c;   m.m[8] = s;
    m.m[2] = -s;  m.m[10] = c;
    return m;
}

static inline mat4_t mat4_rotate_z(float angle) {
    mat4_t m = mat4_identity();
    float c = cosf(angle);
    float s = sinf(angle);
    m.m[0] = c;   m.m[4] = -s;
    m.m[1] = s;   m.m[5] = c;
    return m;
}

static inline mat4_t mat4_scale(float sx, float sy, float sz) {
    mat4_t m = {0};
    m.m[0] = sx;
    m.m[5] = sy;
    m.m[10] = sz;
    m.m[15] = 1.0f;
    return m;
}

static inline mat4_t mat4_translate(float tx, float ty, float tz) {
    mat4_t m = mat4_identity();
    m.m[12] = tx;
    m.m[13] = ty;
    m.m[14] = tz;
    return m;
}

// Transform a point by a matrix (assumes w=1, returns xyz)
static inline vec3_t mat4_mul_point(mat4_t m, vec3_t p) {
    return (vec3_t){
        m.m[0] * p.x + m.m[4] * p.y + m.m[8]  * p.z + m.m[12],
        m.m[1] * p.x + m.m[5] * p.y + m.m[9]  * p.z + m.m[13],
        m.m[2] * p.x + m.m[6] * p.y + m.m[10] * p.z + m.m[14]
    };
}

//------------------------------------------------------------------------------
// Ray type and intersection functions
//------------------------------------------------------------------------------
typedef struct { vec3_t origin, direction; } ray_t;

/* Phase8 deferred glue: legacy interaction helpers below remain intentionally retained for
 * compare-harness coverage and non-Phase-8 consumers. Runtime/editor migration surfaces moved
 * to mdcad_interaction_* and mdcad_undo_editor_* in Phase 8; final boundary reduction is Phase 9. */

// General 4x4 matrix inverse (cofactor method)
static inline mat4_t mat4_inverse(mat4_t m) {
    float *a = m.m;
    mat4_t inv;
    float *o = inv.m;

    float s0 = a[0]*a[5] - a[4]*a[1];
    float s1 = a[0]*a[6] - a[4]*a[2];
    float s2 = a[0]*a[7] - a[4]*a[3];
    float s3 = a[1]*a[6] - a[5]*a[2];
    float s4 = a[1]*a[7] - a[5]*a[3];
    float s5 = a[2]*a[7] - a[6]*a[3];

    float c5 = a[10]*a[15] - a[14]*a[11];
    float c4 = a[9]*a[15]  - a[13]*a[11];
    float c3 = a[9]*a[14]  - a[13]*a[10];
    float c2 = a[8]*a[15]  - a[12]*a[11];
    float c1 = a[8]*a[14]  - a[12]*a[10];
    float c0 = a[8]*a[13]  - a[12]*a[9];

    float det = s0*c5 - s1*c4 + s2*c3 + s3*c2 - s4*c1 + s5*c0;
    if (fabsf(det) < 1e-12f) return mat4_identity();
    float invdet = 1.0f / det;

    o[0]  = ( a[5]*c5 - a[6]*c4 + a[7]*c3) * invdet;
    o[1]  = (-a[1]*c5 + a[2]*c4 - a[3]*c3) * invdet;
    o[2]  = ( a[13]*s5 - a[14]*s4 + a[15]*s3) * invdet;
    o[3]  = (-a[9]*s5  + a[10]*s4 - a[11]*s3) * invdet;
    o[4]  = (-a[4]*c5 + a[6]*c2 - a[7]*c1) * invdet;
    o[5]  = ( a[0]*c5 - a[2]*c2 + a[3]*c1) * invdet;
    o[6]  = (-a[12]*s5 + a[14]*s2 - a[15]*s1) * invdet;
    o[7]  = ( a[8]*s5  - a[10]*s2 + a[11]*s1) * invdet;
    o[8]  = ( a[4]*c4 - a[5]*c2 + a[7]*c0) * invdet;
    o[9]  = (-a[0]*c4 + a[1]*c2 - a[3]*c0) * invdet;
    o[10] = ( a[12]*s4 - a[13]*s2 + a[15]*s0) * invdet;
    o[11] = (-a[8]*s4  + a[9]*s2  - a[11]*s0) * invdet;
    o[12] = (-a[4]*c3 + a[5]*c1 - a[6]*c0) * invdet;
    o[13] = ( a[0]*c3 - a[1]*c1 + a[2]*c0) * invdet;
    o[14] = (-a[12]*s3 + a[13]*s1 - a[14]*s0) * invdet;
    o[15] = ( a[8]*s3  - a[9]*s1  + a[10]*s0) * invdet;

    return inv;
}

// Unproject screen NDC (-1..+1) to world-space ray
/* Deprecated for migrated interaction slices: use mdcad_interaction_screen_ray_from_viewport(). */
MDCAD_MATH3D_INTERACTION_DEPRECATED
static inline ray_t ray_from_screen(float ndc_x, float ndc_y, mat4_t inv_vp) {
    vec3_t near_pt = mat4_mul_point(inv_vp, vec3_make(ndc_x, ndc_y, -1.0f));
    vec3_t far_pt  = mat4_mul_point(inv_vp, vec3_make(ndc_x, ndc_y,  1.0f));
    // For perspective projection, we need proper w-divide
    // mat4_mul_point doesn't do w-divide, so do it manually
    float *m = inv_vp.m;
    {
        float w_near = m[3]*ndc_x + m[7]*ndc_y + m[11]*(-1.0f) + m[15];
        float w_far  = m[3]*ndc_x + m[7]*ndc_y + m[11]*( 1.0f) + m[15];
        if (fabsf(w_near) > 1e-6f) near_pt = vec3_scale(near_pt, 1.0f / w_near);
        if (fabsf(w_far)  > 1e-6f) far_pt  = vec3_scale(far_pt,  1.0f / w_far);
    }
    ray_t r;
    r.origin = near_pt;
    r.direction = vec3_normalize(vec3_sub(far_pt, near_pt));
    return r;
}

// Closest parameter t on an axis line to a ray (for axis-constrained dragging)
// Returns t such that axis_origin + t * axis_dir is closest to ray
/* Deprecated for migrated interaction slices: use mdcad_interaction_ray_axis_closest_t(). */
MDCAD_MATH3D_INTERACTION_DEPRECATED
static inline float ray_axis_closest_t(ray_t ray, vec3_t axis_origin, vec3_t axis_dir) {
    // w = axis_origin - ray.origin (standard convention: w0 = P1 - P2)
    vec3_t w = vec3_sub(axis_origin, ray.origin);
    float a = vec3_dot(axis_dir, axis_dir);
    float b = vec3_dot(axis_dir, ray.direction);
    float c = vec3_dot(ray.direction, ray.direction);
    float d = vec3_dot(axis_dir, w);
    float e = vec3_dot(ray.direction, w);
    float denom = a * c - b * b;
    if (fabsf(denom) < 1e-6f) return 0.0f;  // Degenerate: ray parallel to axis
    return (b * e - c * d) / denom;
}

// Ray-plane intersection. Returns distance t, writes hit point.
// Returns -1.0 if no intersection (ray parallel to plane).
/* Deprecated for migrated interaction slices: use mdcad_interaction_ray_plane_intersect(). */
MDCAD_MATH3D_INTERACTION_DEPRECATED
static inline float ray_plane_intersect(ray_t ray, vec3_t plane_pt, vec3_t plane_n, vec3_t *hit) {
    float denom = vec3_dot(plane_n, ray.direction);
    if (fabsf(denom) < 1e-6f) return -1.0f;
    float t = vec3_dot(vec3_sub(plane_pt, ray.origin), plane_n) / denom;
    if (hit) *hit = vec3_add(ray.origin, vec3_scale(ray.direction, t));
    return t;
}

#endif // MATH3D_H
