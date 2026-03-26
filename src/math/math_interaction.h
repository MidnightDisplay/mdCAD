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

static inline vec3s mdcad_interaction_vec3s_from_legacy(vec3_t legacy) {
    return (vec3s){ { legacy.x, legacy.y, legacy.z } };
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
    mat4s view_s;
    mat4s proj_s;
    mat4s vp_s;
    mat4s inv_vp_s;
    vec4s near_clip;
    vec4s far_clip;
    vec4s near_world;
    vec4s far_world;
    vec3_t near_point;
    vec3_t far_point;
    vec3_t delta;
    float delta_len;
    float near_w;
    float far_w;

    ray.origin = vec3_make(0.0f, 0.0f, 0.0f);
    ray.direction = vec3_make(0.0f, 0.0f, -1.0f);

    if (viewport_width <= 0.0f || viewport_height <= 0.0f) {
        return ray;
    }

    if (!isfinite(viewport_x) || !isfinite(viewport_y)) {
        return ray;
    }

    view_s = mdcad_interaction_mat4s_from_legacy(view);
    proj_s = mdcad_interaction_mat4s_from_legacy(proj);
    vp_s = glms_mat4_mul(proj_s, view_s);
    if (!mdcad_interaction_is_invertible(vp_s)) {
        return ray;
    }

    inv_vp_s = glms_mat4_inv(vp_s);
    near_clip = (vec4s){
        {
            viewport_x * 2.0f - 1.0f,
            (1.0f - viewport_y) * 2.0f - 1.0f,
            -1.0f,
            1.0f,
        }
    };
    far_clip = (vec4s){
        {
            near_clip.raw[0],
            near_clip.raw[1],
            1.0f,
            1.0f,
        }
    };

    near_world = glms_mat4_mulv(inv_vp_s, near_clip);
    far_world = glms_mat4_mulv(inv_vp_s, far_clip);
    near_w = near_world.raw[3];
    far_w = far_world.raw[3];
    if (!isfinite(near_w) || !isfinite(far_w) || fabsf(near_w) <= 1e-6f || fabsf(far_w) <= 1e-6f) {
        return ray;
    }

    near_point = vec3_make(near_world.raw[0] / near_w,
                           near_world.raw[1] / near_w,
                           near_world.raw[2] / near_w);
    far_point = vec3_make(far_world.raw[0] / far_w,
                          far_world.raw[1] / far_w,
                          far_world.raw[2] / far_w);

    delta = vec3_sub(far_point, near_point);
    delta_len = vec3_length(delta);
    if (!isfinite(delta_len) || delta_len <= 1e-8f) {
        return ray;
    }

    ray.origin = near_point;
    ray.direction = vec3_scale(delta, 1.0f / delta_len);
    return ray;
}

static inline float mdcad_interaction_ray_axis_closest_t(ray_t ray,
                                                          vec3_t axis_origin,
                                                          vec3_t axis_dir) {
    vec3s ray_origin_s = mdcad_interaction_vec3s_from_legacy(ray.origin);
    vec3s ray_direction_s = mdcad_interaction_vec3s_from_legacy(ray.direction);
    vec3s axis_origin_s = mdcad_interaction_vec3s_from_legacy(axis_origin);
    vec3s axis_dir_s = mdcad_interaction_vec3s_from_legacy(axis_dir);
    vec3s w = glms_vec3_sub(axis_origin_s, ray_origin_s);
    float a = glms_vec3_dot(axis_dir_s, axis_dir_s);
    float b = glms_vec3_dot(axis_dir_s, ray_direction_s);
    float c = glms_vec3_dot(ray_direction_s, ray_direction_s);
    float d = glms_vec3_dot(axis_dir_s, w);
    float e = glms_vec3_dot(ray_direction_s, w);
    float denom = a * c - b * b;

    if (!isfinite(denom) || fabsf(denom) < 1e-6f) {
        return 0.0f;
    }

    {
        float t = (b * e - c * d) / denom;
        if (!isfinite(t)) {
            return 0.0f;
        }
        return t;
    }
}

static inline bool mdcad_interaction_ray_plane_intersect(ray_t ray,
                                                          vec3_t plane_point,
                                                          vec3_t plane_normal,
                                                          vec3_t *out_hit,
                                                          float *out_t) {
    vec3s ray_origin_s = mdcad_interaction_vec3s_from_legacy(ray.origin);
    vec3s ray_direction_s = mdcad_interaction_vec3s_from_legacy(ray.direction);
    vec3s plane_point_s = mdcad_interaction_vec3s_from_legacy(plane_point);
    vec3s plane_normal_s = mdcad_interaction_vec3s_from_legacy(plane_normal);
    float denom = glms_vec3_dot(plane_normal_s, ray_direction_s);

    if (out_t) {
        *out_t = 0.0f;
    }
    if (out_hit) {
        *out_hit = vec3_make(0.0f, 0.0f, 0.0f);
    }

    if (!isfinite(denom) || fabsf(denom) < 1e-6f) {
        return false;
    }

    {
        vec3s point_delta = glms_vec3_sub(plane_point_s, ray_origin_s);
        float t = glms_vec3_dot(point_delta, plane_normal_s) / denom;

        if (!isfinite(t)) {
            return false;
        }

        if (out_t) {
            *out_t = t;
        }
        if (out_hit) {
            vec3s hit_s = glms_vec3_add(ray_origin_s, glms_vec3_scale(ray_direction_s, t));
            if (!mdcad_interaction_isfinite_vec3s(hit_s)) {
                return false;
            }

            *out_hit = vec3_make(hit_s.raw[0], hit_s.raw[1], hit_s.raw[2]);
        }
    }

    return true;
}

static inline vec3_t mdcad_interaction_world_delta_to_local(mat4_t world_matrix,
                                                             vec3_t world_delta) {
    mat4s world_matrix_s = mdcad_interaction_mat4s_from_legacy(world_matrix);
    mat4 world_matrix_raw;
    mat4 inv_world_raw;
    mat4s inv_world_s;
    vec4s world_delta4;
    vec4s local_delta4;

    if (!isfinite(world_delta.x) || !isfinite(world_delta.y) || !isfinite(world_delta.z)) {
        return vec3_make(0.0f, 0.0f, 0.0f);
    }

    if (!mdcad_interaction_is_invertible(world_matrix_s)) {
        return vec3_make(0.0f, 0.0f, 0.0f);
    }

    memcpy(world_matrix_raw, world_matrix_s.raw, sizeof(world_matrix_raw));
    glm_mat4_inv(world_matrix_raw, inv_world_raw);
    memcpy(inv_world_s.raw, inv_world_raw, sizeof(inv_world_raw));

    world_delta4 = (vec4s){ { world_delta.x, world_delta.y, world_delta.z, 0.0f } };
    local_delta4 = glms_mat4_mulv(inv_world_s, world_delta4);
    if (!isfinite(local_delta4.raw[0]) || !isfinite(local_delta4.raw[1]) || !isfinite(local_delta4.raw[2])) {
        return vec3_make(0.0f, 0.0f, 0.0f);
    }

    return vec3_make(local_delta4.raw[0], local_delta4.raw[1], local_delta4.raw[2]);
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
