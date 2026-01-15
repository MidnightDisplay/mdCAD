//------------------------------------------------------------------------------
// math3d.h - Simple 3D math library (header-only)
//------------------------------------------------------------------------------
#ifndef MATH3D_H
#define MATH3D_H

#include <math.h>

//------------------------------------------------------------------------------
// Types
//------------------------------------------------------------------------------
typedef struct { float x, y, z; } vec3_t;
typedef struct { float m[16]; } mat4_t;  // column-major

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

#endif // MATH3D_H
