#ifndef MDCAD_MATH_INTERACTION_H
#define MDCAD_MATH_INTERACTION_H

#include "cglm_entry.h"
#include "../math3d.h"

#include <math.h>
#include <stdbool.h>
#include <string.h>

static inline bool mdcad_interaction_isfinite_vec3s(vec3s value) {
    const float *v = (const float *)value.raw;

    return isfinite(v[0]) && isfinite(v[1]) && isfinite(v[2]);
}

static inline mat4s mdcad_interaction_mat4s_from_legacy(mat4_t legacy) {
    mat4s result;

    memcpy(result.raw, legacy.m, sizeof(legacy.m));
    return result;
}

static inline mat4_t mdcad_interaction_mat4_legacy_from_mat4s(mat4s matrix) {
    mat4_t result;

    memcpy(result.m, matrix.raw, sizeof(result.m));
    return result;
}

static inline bool mdcad_interaction_is_invertible(mat4s matrix) {
    mat4 raw;
    float det;

    memcpy(raw, matrix.raw, sizeof(raw));
    det = glm_mat4_det(raw);
    return isfinite(det) && fabsf(det) > 1e-8f;
}

static inline vec3_t mdcad_interaction_unproject_point(float screen_x,
                                                       float screen_y_from_top,
                                                       float depth,
                                                       float viewport_width,
                                                       float viewport_height,
                                                       mat4_t view,
                                                       mat4_t proj) {
    mat4s view_s;
    mat4s proj_s;
    mat4s vp_s;
    mat4s inv_vp_s;
    vec4s viewport;
    vec3s screen_point;
    vec3s world_point;

    if (viewport_width <= 0.0f || viewport_height <= 0.0f) {
        return vec3_make(0.0f, 0.0f, 0.0f);
    }

    if (!isfinite(screen_x) || !isfinite(screen_y_from_top) || !isfinite(depth)) {
        return vec3_make(0.0f, 0.0f, 0.0f);
    }

    view_s = mdcad_interaction_mat4s_from_legacy(view);
    proj_s = mdcad_interaction_mat4s_from_legacy(proj);
    vp_s = glms_mat4_mul(proj_s, view_s);

    if (!mdcad_interaction_is_invertible(vp_s)) {
        return vec3_make(0.0f, 0.0f, 0.0f);
    }

    inv_vp_s = glms_mat4_inv(vp_s);
    viewport = (vec4s){ { 0.0f, 0.0f, viewport_width, viewport_height } };
    screen_point = (vec3s){ { screen_x, viewport_height - screen_y_from_top, depth } };
    world_point = glms_unprojecti_zo(screen_point, inv_vp_s, viewport);

    if (!mdcad_interaction_isfinite_vec3s(world_point)) {
        return vec3_make(0.0f, 0.0f, 0.0f);
    }

    return vec3_make(world_point.raw[0], world_point.raw[1], world_point.raw[2]);
}

static inline ray_t mdcad_interaction_screen_ray_from_viewport(float viewport_x,
                                                                float viewport_y,
                                                                float viewport_width,
                                                                float viewport_height,
                                                                mat4_t view,
                                                                mat4_t proj) {
    ray_t ray;
    vec3_t near_point;
    vec3_t far_point;
    vec3_t delta;
    float delta_len;

    ray.origin = vec3_make(0.0f, 0.0f, 0.0f);
    ray.direction = vec3_make(0.0f, 0.0f, -1.0f);

    if (viewport_width <= 0.0f || viewport_height <= 0.0f) {
        return ray;
    }

    if (!isfinite(viewport_x) || !isfinite(viewport_y)) {
        return ray;
    }

    near_point = mdcad_interaction_unproject_point(viewport_x * viewport_width,
                                                    viewport_y * viewport_height,
                                                    0.0f,
                                                    viewport_width,
                                                    viewport_height,
                                                    view,
                                                    proj);
    far_point = mdcad_interaction_unproject_point(viewport_x * viewport_width,
                                                   viewport_y * viewport_height,
                                                   1.0f,
                                                   viewport_width,
                                                   viewport_height,
                                                   view,
                                                   proj);

    delta = vec3_sub(far_point, near_point);
    delta_len = vec3_length(delta);
    if (!isfinite(delta_len) || delta_len <= 1e-8f) {
        return ray;
    }

    ray.origin = near_point;
    ray.direction = vec3_scale(delta, 1.0f / delta_len);
    return ray;
}

static inline mat4_t mdcad_interaction_compute_pick_mvp(float center_x,
                                                         float center_y,
                                                         float viewport_width,
                                                         float viewport_height,
                                                         float pick_buffer_size,
                                                         float *out_zoom_factor,
                                                         mat4_t view,
                                                         mat4_t proj) {
    mat4s view_s = mdcad_interaction_mat4s_from_legacy(view);
    mat4s proj_s = mdcad_interaction_mat4s_from_legacy(proj);
    mat4s vp_s = glms_mat4_mul(proj_s, view_s);
    float zoom = 1.0f;

    if (out_zoom_factor) {
        *out_zoom_factor = 1.0f;
    }

    if (viewport_width <= 0.0f || viewport_height <= 0.0f || pick_buffer_size <= 0.0f) {
        return mdcad_interaction_mat4_legacy_from_mat4s(vp_s);
    }

    if (!isfinite(center_x) || !isfinite(center_y)) {
        return mdcad_interaction_mat4_legacy_from_mat4s(vp_s);
    }

    {
        float zoom_x = viewport_width / pick_buffer_size;
        float zoom_y = viewport_height / pick_buffer_size;

        zoom = (zoom_x < zoom_y) ? zoom_x : zoom_y;
        if (!isfinite(zoom) || zoom < 1.0f) {
            zoom = 1.0f;
        }
    }

    if (out_zoom_factor) {
        *out_zoom_factor = zoom;
    }

    {
        float ndc_x = center_x * 2.0f - 1.0f;
        float ndc_y = (1.0f - center_y) * 2.0f - 1.0f;
        mat4s pick_proj = GLMS_MAT4_IDENTITY;
        float *pick_proj_raw = (float *)pick_proj.raw;
        mat4s mvp_s;

        if (!isfinite(ndc_x) || !isfinite(ndc_y)) {
            return mdcad_interaction_mat4_legacy_from_mat4s(vp_s);
        }

        pick_proj_raw[0] = zoom;
        pick_proj_raw[5] = zoom;
        pick_proj_raw[12] = -ndc_x * zoom;
        pick_proj_raw[13] = -ndc_y * zoom;

        mvp_s = glms_mat4_mul(pick_proj, vp_s);
        return mdcad_interaction_mat4_legacy_from_mat4s(mvp_s);
    }
}

#endif
