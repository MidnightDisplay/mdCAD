#include "math/math_bench.h"
#include "math/math_compare.h"
#include "math/math_interaction.h"
#include "math/math_validate.h"
#include "components/transform_comp.h"
#include "ecs/ecs_scene.h"
#include "orbit_camera.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    MDCAD_HARNESS_MODE_COMPARE = 0,
    MDCAD_HARNESS_MODE_BENCH,
} mdcad_harness_mode_t;

typedef bool (*mdcad_compare_case_fn)(mdcad_validation_report_t *report);

typedef struct {
    const char *id;
    mdcad_compare_case_fn fn;
} mdcad_compare_case_t;

typedef struct {
    const char *id;
    mdcad_bench_fn fn;
    void *ctx;
} mdcad_bench_case_t;

typedef struct {
    mat4_t lhs;
    mat4_t rhs;
} mdcad_legacy_mat4_mul_ctx_t;

typedef struct {
    mat4_t matrix;
} mdcad_legacy_mat4_inv_ctx_t;

typedef struct {
    float ndc_x;
    float ndc_y;
    float ndc_step;
    mat4_t inv_vp;
} mdcad_legacy_screen_ray_ctx_t;

typedef struct {
    mat4s lhs;
    mat4s rhs;
} mdcad_cglm_mat4_mul_ctx_t;

typedef struct {
    mat4s matrix;
} mdcad_cglm_mat4_inv_ctx_t;

typedef struct {
    vec3s near_pos;
    vec3s far_pos;
    float x_step;
    mat4s inv_vp;
    vec4s viewport;
} mdcad_cglm_screen_ray_ctx_t;

static volatile float mdcad_bench_sink = 0.0f;

static orbit_camera_t mdcad_harness_make_camera(void);
static vec3_t mdcad_harness_legacy_orbit_eye(const orbit_camera_t *camera);
static mat4_t mdcad_harness_legacy_orbit_view(const orbit_camera_t *camera);
static TransformComp mdcad_harness_make_transform(vec3_t position, vec3_t rotation, vec3_t scale);
static mat4_t mdcad_harness_legacy_transform_from_components(vec3_t position, vec3_t rotation, vec3_t scale);
static mat4_t mdcad_harness_legacy_projection(void);
static mat4_t mdcad_harness_legacy_pick_mvp_window(float center_x,
                                                   float center_y,
                                                   float viewport_width,
                                                   float viewport_height,
                                                   float pick_buffer_size,
                                                   float *out_zoom_factor,
                                                   mat4_t view,
                                                   mat4_t proj);
static vec3_t mdcad_harness_legacy_world_delta_to_local(mat4_t world_matrix, vec3_t world_delta);
static mat4s mdcad_harness_cglm_projection(void);
static bool mdcad_harness_compare_orbit_camera_view(mdcad_validation_report_t *report);
static bool mdcad_harness_compare_view_projection_roundtrip(mdcad_validation_report_t *report);
static bool mdcad_harness_compare_screen_ray_unproject(mdcad_validation_report_t *report);
static bool mdcad_harness_compare_pick_mvp_window(mdcad_validation_report_t *report);
static bool mdcad_harness_compare_gizmo_axis_drag(mdcad_validation_report_t *report);
static bool mdcad_harness_compare_gizmo_plane_drag(mdcad_validation_report_t *report);
static bool mdcad_harness_compare_gizmo_vertex_local_delta(mdcad_validation_report_t *report);
static bool mdcad_harness_compare_transform_compose(mdcad_validation_report_t *report);
static bool mdcad_harness_compare_hierarchy_world_transform(mdcad_validation_report_t *report);
static void mdcad_harness_bench_legacy_mat4_mul(void *ctx);
static void mdcad_harness_bench_cglm_mat4_mul(void *ctx);
static void mdcad_harness_bench_legacy_mat4_inverse(void *ctx);
static void mdcad_harness_bench_cglm_mat4_inv(void *ctx);
static void mdcad_harness_bench_legacy_screen_ray(void *ctx);
static void mdcad_harness_bench_cglm_screen_ray(void *ctx);

static mdcad_legacy_mat4_mul_ctx_t mdcad_legacy_mat4_mul_ctx;
static mdcad_legacy_mat4_inv_ctx_t mdcad_legacy_mat4_inv_ctx;
static mdcad_legacy_screen_ray_ctx_t mdcad_legacy_screen_ray_ctx;
static mdcad_cglm_mat4_mul_ctx_t mdcad_cglm_mat4_mul_ctx;
static mdcad_cglm_mat4_inv_ctx_t mdcad_cglm_mat4_inv_ctx;
static mdcad_cglm_screen_ray_ctx_t mdcad_cglm_screen_ray_ctx;

/* Keep the Phase 3 migration hotspots at the front of the default compare run. */
static const mdcad_compare_case_t mdcad_compare_cases[] = {
    { "orbit-camera-view", mdcad_harness_compare_orbit_camera_view },
    { "view-projection-roundtrip", mdcad_harness_compare_view_projection_roundtrip },
    { "transform-compose", mdcad_harness_compare_transform_compose },
    { "hierarchy-world-transform", mdcad_harness_compare_hierarchy_world_transform },
    { "screen-ray-unproject", mdcad_harness_compare_screen_ray_unproject },
    { "pick-mvp-window", mdcad_harness_compare_pick_mvp_window },
    { "gizmo-axis-drag", mdcad_harness_compare_gizmo_axis_drag },
    { "gizmo-plane-drag", mdcad_harness_compare_gizmo_plane_drag },
    { "gizmo-vertex-local-delta", mdcad_harness_compare_gizmo_vertex_local_delta },
};

static mdcad_bench_case_t mdcad_bench_cases[] = {
    { "legacy-mat4-mul", mdcad_harness_bench_legacy_mat4_mul, &mdcad_legacy_mat4_mul_ctx },
    { "cglm-mat4-mul", mdcad_harness_bench_cglm_mat4_mul, &mdcad_cglm_mat4_mul_ctx },
    { "legacy-mat4-inverse", mdcad_harness_bench_legacy_mat4_inverse, &mdcad_legacy_mat4_inv_ctx },
    { "cglm-mat4-inv", mdcad_harness_bench_cglm_mat4_inv, &mdcad_cglm_mat4_inv_ctx },
    { "legacy-screen-ray", mdcad_harness_bench_legacy_screen_ray, &mdcad_legacy_screen_ray_ctx },
    { "cglm-screen-ray", mdcad_harness_bench_cglm_screen_ray, &mdcad_cglm_screen_ray_ctx },
};

static vec3s mdcad_harness_vec3s(vec3_t value) {
    return (vec3s){ { value.x, value.y, value.z } };
}

static vec4s mdcad_harness_vec4s(float x, float y, float z, float w) {
    return (vec4s){ { x, y, z, w } };
}

static bool mdcad_harness_expect(mdcad_validation_report_t *report, const char *label, bool condition) {
    mdcad_validation_expect(report, condition, label);
    return condition;
}

static bool mdcad_harness_compare_mat4_to_cglm(mdcad_validation_report_t *report,
                                               const char *label,
                                               mat4_t legacy,
                                               mat4s candidate,
                                               float epsilon) {
    return mdcad_harness_expect(report, label, mdcad_compare_mat4_close(legacy, (const float *)candidate.raw, epsilon));
}

static bool mdcad_harness_compare_vec3_to_cglm(mdcad_validation_report_t *report,
                                               const char *label,
                                               vec3_t legacy,
                                               vec3s candidate,
                                               float epsilon) {
    return mdcad_harness_expect(report, label, mdcad_compare_vec3_close(legacy, (const float *)candidate.raw, epsilon));
}

static bool mdcad_harness_compare_ray_to_cglm(mdcad_validation_report_t *report,
                                              const char *label,
                                              ray_t legacy,
                                              vec3s origin,
                                              vec3s direction,
                                              float epsilon) {
    return mdcad_harness_expect(report,
                                label,
                                mdcad_compare_ray_close(legacy,
                                                        (const float *)origin.raw,
                                                        (const float *)direction.raw,
                                                        epsilon));
}

static bool mdcad_harness_emit_compare_result(const char *id, const mdcad_validation_report_t *report) {
    if (report->checks_failed == 0) {
        printf("COMPARE PASS %s checks=%d\n", id, report->checks_run);
        return true;
    }

    printf("COMPARE FAIL %s failed=%d/%d\n", id, report->checks_failed, report->checks_run);
    return false;
}

static void mdcad_harness_mul_vec4_legacy(mat4_t matrix,
                                          float x,
                                          float y,
                                          float z,
                                          float w,
                                          float out[4]) {
    out[0] = matrix.m[0] * x + matrix.m[4] * y + matrix.m[8]  * z + matrix.m[12] * w;
    out[1] = matrix.m[1] * x + matrix.m[5] * y + matrix.m[9]  * z + matrix.m[13] * w;
    out[2] = matrix.m[2] * x + matrix.m[6] * y + matrix.m[10] * z + matrix.m[14] * w;
    out[3] = matrix.m[3] * x + matrix.m[7] * y + matrix.m[11] * z + matrix.m[15] * w;
}

static bool mdcad_harness_project_ndc_xy_legacy(mat4_t view_projection,
                                                vec3_t point,
                                                float *out_x,
                                                float *out_y) {
    float clip[4];

    mdcad_harness_mul_vec4_legacy(view_projection, point.x, point.y, point.z, 1.0f, clip);
    if (fabsf(clip[3]) <= 1e-6f) {
        return false;
    }

    *out_x = clip[0] / clip[3];
    *out_y = clip[1] / clip[3];
    return true;
}

static bool mdcad_harness_project_ndc_xy_cglm(mat4s view_projection,
                                              vec3s point,
                                              float *out_x,
                                              float *out_y) {
    vec4s clip = glms_mat4_mulv(view_projection, glms_vec4(point, 1.0f));
    float clip_w = clip.raw[3];

    if (fabsf(clip_w) <= 1e-6f) {
        return false;
    }

    *out_x = clip.raw[0] / clip_w;
    *out_y = clip.raw[1] / clip_w;
    return true;
}

static void mdcad_harness_screen_sample(float *screen_x,
                                        float *screen_y_top,
                                        float *screen_y_bottom,
                                        float *ndc_x,
                                        float *ndc_y) {
    *screen_x = 960.0f;
    *screen_y_top = 180.0f;
    *screen_y_bottom = 720.0f - *screen_y_top;
    *ndc_x = ((2.0f * *screen_x) / 1280.0f) - 1.0f;
    *ndc_y = 1.0f - ((2.0f * *screen_y_top) / 720.0f);
}

static orbit_camera_t mdcad_harness_make_camera(void) {
    orbit_camera_t camera;

    orbit_camera_init(&camera);
    camera.distance = 6.5f;
    camera.azimuth = 1.1f;
    camera.elevation = -0.35f;
    camera.target = vec3_make(1.25f, -0.5f, 2.0f);
    return camera;
}

static vec3_t mdcad_harness_legacy_orbit_eye(const orbit_camera_t *camera) {
    vec3_t eye;

    eye.x = camera->target.x + camera->distance * cosf(camera->elevation) * sinf(camera->azimuth);
    eye.y = camera->target.y + camera->distance * sinf(camera->elevation);
    eye.z = camera->target.z + camera->distance * cosf(camera->elevation) * cosf(camera->azimuth);
    return eye;
}

static mat4_t mdcad_harness_legacy_orbit_view(const orbit_camera_t *camera) {
    return mat4_lookat(mdcad_harness_legacy_orbit_eye(camera),
                       camera->target,
                       vec3_make(0.0f, 1.0f, 0.0f));
}

static mat4_t mdcad_harness_legacy_projection(void) {
    return mat4_perspective(0.785398f, 1280.0f / 720.0f, 0.1f, 100.0f);
}

static mat4_t mdcad_harness_legacy_pick_mvp_window(float center_x,
                                                   float center_y,
                                                   float viewport_width,
                                                   float viewport_height,
                                                   float pick_buffer_size,
                                                   float *out_zoom_factor,
                                                   mat4_t view,
                                                   mat4_t proj) {
    mat4_t vp = mat4_mul(proj, view);
    float zoom = 1.0f;

    if (out_zoom_factor) {
        *out_zoom_factor = 1.0f;
    }

    if (viewport_width <= 0.0f || viewport_height <= 0.0f || pick_buffer_size <= 0.0f) {
        return vp;
    }

    {
        float zoom_x = viewport_width / pick_buffer_size;
        float zoom_y = viewport_height / pick_buffer_size;
        float ndc_x = center_x * 2.0f - 1.0f;
        float ndc_y = (1.0f - center_y) * 2.0f - 1.0f;
        mat4_t pick_proj = mat4_identity();

        zoom = (zoom_x < zoom_y) ? zoom_x : zoom_y;
        if (zoom < 1.0f) {
            zoom = 1.0f;
        }

        if (out_zoom_factor) {
            *out_zoom_factor = zoom;
        }

        pick_proj.m[0] = zoom;
        pick_proj.m[5] = zoom;
        pick_proj.m[12] = -ndc_x * zoom;
        pick_proj.m[13] = -ndc_y * zoom;
        return mat4_mul(pick_proj, vp);
    }
}

static vec3_t mdcad_harness_legacy_world_delta_to_local(mat4_t world_matrix, vec3_t world_delta) {
    mat4_t inv = mat4_inverse(world_matrix);

    return vec3_make(
        inv.m[0] * world_delta.x + inv.m[4] * world_delta.y + inv.m[8]  * world_delta.z,
        inv.m[1] * world_delta.x + inv.m[5] * world_delta.y + inv.m[9]  * world_delta.z,
        inv.m[2] * world_delta.x + inv.m[6] * world_delta.y + inv.m[10] * world_delta.z
    );
}

static mat4s mdcad_harness_cglm_projection(void) {
    return glms_perspective_rh_zo(0.785398f, 1280.0f / 720.0f, 0.1f, 100.0f);
}

static TransformComp mdcad_harness_make_transform(vec3_t position, vec3_t rotation, vec3_t scale) {
    TransformComp transform = transform_comp_default();

    transform.position = position;
    transform.rotation = rotation;
    transform.scale = scale;
    return transform;
}

static mat4_t mdcad_harness_legacy_transform_from_components(vec3_t position, vec3_t rotation, vec3_t scale) {
    mat4_t translation = mat4_translate(position.x, position.y, position.z);
    mat4_t rotate_x = mat4_rotate_x(rotation.x);
    mat4_t rotate_y = mat4_rotate_y(rotation.y);
    mat4_t rotate_z = mat4_rotate_z(rotation.z);
    mat4_t scale_mat = mat4_scale(scale.x, scale.y, scale.z);

    return mat4_mul(translation,
                    mat4_mul(rotate_z,
                             mat4_mul(rotate_y,
                                      mat4_mul(rotate_x, scale_mat))));
}

static mat4_t mdcad_harness_legacy_transform(void) {
    return mdcad_harness_legacy_transform_from_components(vec3_make(1.5f, -2.0f, 0.75f),
                                                          vec3_make(0.3f, -0.7f, 1.2f),
                                                          vec3_make(2.0f, 0.5f, 1.25f));
}

static bool mdcad_harness_compare_orbit_camera_view(mdcad_validation_report_t *report) {
    orbit_camera_t camera = mdcad_harness_make_camera();
    mat4_t legacy_view = mdcad_harness_legacy_orbit_view(&camera);
    mat4s cglm_view = orbit_camera_get_view_matrix_cglm(&camera);

    return mdcad_harness_compare_mat4_to_cglm(report,
                                              "orbit-camera-view matrix",
                                              legacy_view,
                                              cglm_view,
                                              1e-4f);
}

static bool mdcad_harness_compare_view_projection_roundtrip(mdcad_validation_report_t *report) {
    static const vec3_t sample_points[] = {
        { 1.25f, -0.5f, 2.0f },
        { 2.1f, 0.3f, 1.1f },
        { 0.45f, -1.0f, 3.25f },
    };
    orbit_camera_t camera = mdcad_harness_make_camera();
    mat4_t legacy_view = mdcad_harness_legacy_orbit_view(&camera);
    mat4_t legacy_proj = mdcad_harness_legacy_projection();
    mat4_t legacy_vp = mat4_mul(legacy_proj, legacy_view);
    mat4s cglm_view = orbit_camera_get_view_matrix_cglm(&camera);
    mat4s cglm_proj = mdcad_harness_cglm_projection();
    mat4s cglm_vp = glms_mat4_mul(cglm_proj, cglm_view);
    size_t i;

    for (i = 0; i < (sizeof(sample_points) / sizeof(sample_points[0])); ++i) {
        float legacy_x;
        float legacy_y;
        float cglm_x;
        float cglm_y;
        char label[96];
        bool projected_legacy = mdcad_harness_project_ndc_xy_legacy(legacy_vp, sample_points[i], &legacy_x, &legacy_y);
        bool projected_cglm = mdcad_harness_project_ndc_xy_cglm(cglm_vp, mdcad_harness_vec3s(sample_points[i]), &cglm_x, &cglm_y);

        snprintf(label, sizeof(label), "view-projection-roundtrip point-%zu projects", i + 1);
        mdcad_harness_expect(report, label, projected_legacy && projected_cglm);
        if (!projected_legacy || !projected_cglm) {
            continue;
        }

        snprintf(label, sizeof(label), "view-projection-roundtrip point-%zu ndc-x", i + 1);
        mdcad_harness_expect(report, label, mdcad_compare_float_close(legacy_x, cglm_x, 1e-4f));
        snprintf(label, sizeof(label), "view-projection-roundtrip point-%zu ndc-y", i + 1);
        mdcad_harness_expect(report, label, mdcad_compare_float_close(legacy_y, cglm_y, 1e-4f));
    }

    return report->checks_failed == 0;
}

static bool mdcad_harness_compare_screen_ray_unproject(mdcad_validation_report_t *report) {
    orbit_camera_t camera = mdcad_harness_make_camera();
    mat4_t legacy_view = mdcad_harness_legacy_orbit_view(&camera);
    mat4_t legacy_proj = mdcad_harness_legacy_projection();
    mat4_t legacy_inv_vp = mat4_inverse(mat4_mul(legacy_proj, legacy_view));
    float screen_x;
    float screen_y_top;
    float screen_y_bottom;
    float ndc_x;
    float ndc_y;
    float vp_x;
    float vp_y;
    ray_t legacy_ray;
    ray_t candidate_ray;

    mdcad_harness_screen_sample(&screen_x, &screen_y_top, &screen_y_bottom, &ndc_x, &ndc_y);
    (void)screen_y_bottom;
    vp_x = screen_x / 1280.0f;
    vp_y = screen_y_top / 720.0f;
    legacy_ray = ray_from_screen(ndc_x, ndc_y, legacy_inv_vp);
    candidate_ray = mdcad_interaction_screen_ray_from_viewport(vp_x,
                                                                vp_y,
                                                                1280.0f,
                                                                720.0f,
                                                                legacy_view,
                                                                legacy_proj);

    return mdcad_harness_compare_ray_to_cglm(report,
                                             "screen-ray-unproject ray",
                                             legacy_ray,
                                             mdcad_harness_vec3s(candidate_ray.origin),
                                             mdcad_harness_vec3s(candidate_ray.direction),
                                             1e-4f);
}

static bool mdcad_harness_compare_pick_mvp_window(mdcad_validation_report_t *report) {
    static const vec3_t sample_point = { 1.75f, -0.1f, 2.5f };
    orbit_camera_t camera = mdcad_harness_make_camera();
    mat4_t legacy_view = mdcad_harness_legacy_orbit_view(&camera);
    mat4_t legacy_proj = mdcad_harness_legacy_projection();
    float legacy_zoom = 1.0f;
    float candidate_zoom = 1.0f;
    float legacy_ndc_x;
    float legacy_ndc_y;
    float candidate_ndc_x;
    float candidate_ndc_y;
    mat4_t legacy_mvp = mdcad_harness_legacy_pick_mvp_window(0.62f,
                                                              0.35f,
                                                              1280.0f,
                                                              720.0f,
                                                              20.0f,
                                                              &legacy_zoom,
                                                              legacy_view,
                                                              legacy_proj);
    mat4_t candidate_mvp = mdcad_interaction_compute_pick_mvp(0.62f,
                                                              0.35f,
                                                              1280.0f,
                                                              720.0f,
                                                              20.0f,
                                                              &candidate_zoom,
                                                              legacy_view,
                                                              legacy_proj);
    bool legacy_projected = mdcad_harness_project_ndc_xy_legacy(legacy_mvp,
                                                                 sample_point,
                                                                 &legacy_ndc_x,
                                                                 &legacy_ndc_y);
    bool candidate_projected = mdcad_harness_project_ndc_xy_legacy(candidate_mvp,
                                                                    sample_point,
                                                                    &candidate_ndc_x,
                                                                    &candidate_ndc_y);

    mdcad_harness_expect(report,
                         "pick-mvp-window zoom",
                         mdcad_compare_float_close(legacy_zoom, candidate_zoom, 1e-6f));
    mdcad_harness_expect(report,
                         "pick-mvp-window matrix",
                         mdcad_compare_mat4_close(legacy_mvp, candidate_mvp.m, 1e-4f));
    mdcad_harness_expect(report,
                         "pick-mvp-window sample projects",
                         legacy_projected && candidate_projected);
    if (legacy_projected && candidate_projected) {
        mdcad_harness_expect(report,
                             "pick-mvp-window sample ndc-x",
                             mdcad_compare_float_close(legacy_ndc_x, candidate_ndc_x, 1e-4f));
        mdcad_harness_expect(report,
                             "pick-mvp-window sample ndc-y",
                             mdcad_compare_float_close(legacy_ndc_y, candidate_ndc_y, 1e-4f));
    }

    return report->checks_failed == 0;
}

static bool mdcad_harness_compare_gizmo_axis_drag(mdcad_validation_report_t *report) {
    vec3_t axis_origin = vec3_make(1.2f, -0.3f, 2.4f);
    vec3_t axis = vec3_make(1.0f, 0.0f, 0.0f);
    ray_t start_ray = {
        vec3_make(-3.0f, 2.0f, 4.0f),
        vec3_normalize(vec3_make(0.85f, -0.42f, -0.31f)),
    };
    ray_t update_ray = {
        vec3_make(-2.2f, 1.8f, 3.6f),
        vec3_normalize(vec3_make(0.91f, -0.37f, -0.18f)),
    };
    ray_t parallel_ray = {
        vec3_make(0.0f, 3.0f, -1.0f),
        vec3_make(1.0f, 0.0f, 0.0f),
    };
    float legacy_start_t = ray_axis_closest_t(start_ray, axis_origin, axis);
    float legacy_update_t = ray_axis_closest_t(update_ray, axis_origin, axis);
    float candidate_start_t = mdcad_interaction_ray_axis_closest_t(start_ray, axis_origin, axis);
    float candidate_update_t = mdcad_interaction_ray_axis_closest_t(update_ray, axis_origin, axis);
    float legacy_parallel_t = ray_axis_closest_t(parallel_ray, axis_origin, axis);
    float candidate_parallel_t = mdcad_interaction_ray_axis_closest_t(parallel_ray, axis_origin, axis);
    vec3_t legacy_total = vec3_scale(axis, legacy_update_t - legacy_start_t);
    vec3_t candidate_total = vec3_scale(axis, candidate_update_t - candidate_start_t);
    vec3_t legacy_increment = vec3_sub(legacy_total, vec3_make(0.0f, 0.0f, 0.0f));
    vec3_t candidate_increment = vec3_sub(candidate_total, vec3_make(0.0f, 0.0f, 0.0f));

    mdcad_harness_expect(report,
                         "gizmo-axis-drag start-t",
                         mdcad_compare_float_close(legacy_start_t, candidate_start_t, 1e-5f));
    mdcad_harness_expect(report,
                         "gizmo-axis-drag update-t",
                         mdcad_compare_float_close(legacy_update_t, candidate_update_t, 1e-5f));
    mdcad_harness_expect(report,
                         "gizmo-axis-drag total-delta",
                         mdcad_compare_vec3_close(legacy_total, &candidate_total.x, 1e-5f));
    mdcad_harness_expect(report,
                         "gizmo-axis-drag incremental-delta",
                         mdcad_compare_vec3_close(legacy_increment, &candidate_increment.x, 1e-5f));
    mdcad_harness_expect(report,
                         "gizmo-axis-drag parallel-legacy-fallback",
                         mdcad_compare_float_close(legacy_parallel_t, 0.0f, 1e-6f));
    mdcad_harness_expect(report,
                         "gizmo-axis-drag parallel-candidate-fallback",
                         mdcad_compare_float_close(candidate_parallel_t, 0.0f, 1e-6f));

    return report->checks_failed == 0;
}

static bool mdcad_harness_compare_gizmo_plane_drag(mdcad_validation_report_t *report) {
    vec3_t plane_point = vec3_make(1.2f, -0.3f, 2.4f);
    vec3_t plane_normal = vec3_make(0.0f, 1.0f, 0.0f);
    ray_t start_ray = {
        vec3_make(0.4f, 3.0f, 2.8f),
        vec3_normalize(vec3_make(0.2f, -1.0f, -0.1f)),
    };
    ray_t update_ray = {
        vec3_make(0.9f, 2.7f, 3.1f),
        vec3_normalize(vec3_make(0.1f, -1.0f, -0.3f)),
    };
    ray_t parallel_ray = {
        vec3_make(-1.0f, 0.0f, 0.5f),
        vec3_make(1.0f, 0.0f, 0.0f),
    };
    vec3_t legacy_start_hit = vec3_make(0.0f, 0.0f, 0.0f);
    vec3_t legacy_update_hit = vec3_make(0.0f, 0.0f, 0.0f);
    vec3_t candidate_start_hit = vec3_make(0.0f, 0.0f, 0.0f);
    vec3_t candidate_update_hit = vec3_make(0.0f, 0.0f, 0.0f);
    float legacy_start_t = ray_plane_intersect(start_ray, plane_point, plane_normal, &legacy_start_hit);
    float legacy_update_t = ray_plane_intersect(update_ray, plane_point, plane_normal, &legacy_update_hit);
    float legacy_parallel_t = ray_plane_intersect(parallel_ray, plane_point, plane_normal, NULL);
    float candidate_start_t = 0.0f;
    float candidate_update_t = 0.0f;
    float candidate_parallel_t = 0.0f;
    bool candidate_start_ok = mdcad_interaction_ray_plane_intersect(start_ray,
                                                                     plane_point,
                                                                     plane_normal,
                                                                     &candidate_start_hit,
                                                                     &candidate_start_t);
    bool candidate_update_ok = mdcad_interaction_ray_plane_intersect(update_ray,
                                                                      plane_point,
                                                                      plane_normal,
                                                                      &candidate_update_hit,
                                                                      &candidate_update_t);
    bool candidate_parallel_ok = mdcad_interaction_ray_plane_intersect(parallel_ray,
                                                                        plane_point,
                                                                        plane_normal,
                                                                        NULL,
                                                                        &candidate_parallel_t);
    vec3_t legacy_total = vec3_sub(legacy_update_hit, legacy_start_hit);
    vec3_t candidate_total = vec3_sub(candidate_update_hit, candidate_start_hit);
    vec3_t legacy_increment = vec3_sub(legacy_total, vec3_make(0.0f, 0.0f, 0.0f));
    vec3_t candidate_increment = vec3_sub(candidate_total, vec3_make(0.0f, 0.0f, 0.0f));

    mdcad_harness_expect(report, "gizmo-plane-drag legacy-start-hit", legacy_start_t >= 0.0f);
    mdcad_harness_expect(report, "gizmo-plane-drag legacy-update-hit", legacy_update_t >= 0.0f);
    mdcad_harness_expect(report, "gizmo-plane-drag candidate-start-hit", candidate_start_ok && candidate_start_t >= 0.0f);
    mdcad_harness_expect(report, "gizmo-plane-drag candidate-update-hit", candidate_update_ok && candidate_update_t >= 0.0f);
    mdcad_harness_expect(report,
                         "gizmo-plane-drag start-t",
                         mdcad_compare_float_close(legacy_start_t, candidate_start_t, 1e-5f));
    mdcad_harness_expect(report,
                         "gizmo-plane-drag update-t",
                         mdcad_compare_float_close(legacy_update_t, candidate_update_t, 1e-5f));
    mdcad_harness_expect(report,
                         "gizmo-plane-drag start-hit",
                         mdcad_compare_vec3_close(legacy_start_hit, &candidate_start_hit.x, 1e-5f));
    mdcad_harness_expect(report,
                         "gizmo-plane-drag update-hit",
                         mdcad_compare_vec3_close(legacy_update_hit, &candidate_update_hit.x, 1e-5f));
    mdcad_harness_expect(report,
                         "gizmo-plane-drag total-delta",
                         mdcad_compare_vec3_close(legacy_total, &candidate_total.x, 1e-5f));
    mdcad_harness_expect(report,
                         "gizmo-plane-drag incremental-delta",
                         mdcad_compare_vec3_close(legacy_increment, &candidate_increment.x, 1e-5f));
    mdcad_harness_expect(report, "gizmo-plane-drag parallel-legacy", legacy_parallel_t < 0.0f);
    mdcad_harness_expect(report, "gizmo-plane-drag parallel-candidate", !candidate_parallel_ok);
    mdcad_harness_expect(report,
                         "gizmo-plane-drag parallel-candidate-t-fallback",
                         mdcad_compare_float_close(candidate_parallel_t, 0.0f, 1e-6f));

    return report->checks_failed == 0;
}

static bool mdcad_harness_compare_gizmo_vertex_local_delta(mdcad_validation_report_t *report) {
    mat4_t world_matrix = mdcad_harness_legacy_transform_from_components(vec3_make(1.5f, -2.0f, 0.75f),
                                                                         vec3_make(0.3f, -0.7f, 1.2f),
                                                                         vec3_make(2.0f, 0.5f, 1.25f));
    vec3_t world_delta = vec3_make(0.45f, -1.2f, 0.33f);
    vec3_t legacy_local_delta = mdcad_harness_legacy_world_delta_to_local(world_matrix, world_delta);
    vec3_t candidate_local_delta = mdcad_interaction_world_delta_to_local(world_matrix, world_delta);
    mat4_t singular_world_matrix = mdcad_harness_legacy_transform_from_components(vec3_make(0.0f, 0.0f, 0.0f),
                                                                                   vec3_make(0.0f, 0.0f, 0.0f),
                                                                                   vec3_make(0.0f, 1.0f, 1.0f));
    vec3_t singular_local_delta = mdcad_interaction_world_delta_to_local(singular_world_matrix, world_delta);

    mdcad_harness_expect(report,
                         "gizmo-vertex-local-delta converted",
                         mdcad_compare_vec3_close(legacy_local_delta, &candidate_local_delta.x, 1e-5f));
    mdcad_harness_expect(report,
                         "gizmo-vertex-local-delta singular-fallback",
                         mdcad_compare_vec3_close(vec3_make(0.0f, 0.0f, 0.0f), &singular_local_delta.x, 1e-6f));

    return report->checks_failed == 0;
}

static bool mdcad_harness_compare_transform_compose(mdcad_validation_report_t *report) {
    static const vec3_t sample_points[] = {
        { 0.0f, 0.0f, 0.0f },
        { 1.0f, -2.0f, 0.5f },
        { -0.75f, 0.25f, 2.0f },
    };
    mat4_t legacy_transform = mdcad_harness_legacy_transform();
    TransformComp production_transform = mdcad_harness_make_transform(vec3_make(1.5f, -2.0f, 0.75f),
                                                                      vec3_make(0.3f, -0.7f, 1.2f),
                                                                      vec3_make(2.0f, 0.5f, 1.25f));
    mat4_t candidate_transform = transform_comp_compute_local(&production_transform);
    size_t i;

    mdcad_harness_expect(report,
                         "transform-compose matrix",
                         mdcad_compare_mat4_close(legacy_transform, candidate_transform.m, 1e-4f));

    for (i = 0; i < (sizeof(sample_points) / sizeof(sample_points[0])); ++i) {
        vec3_t legacy_point = mat4_transform_point(legacy_transform, sample_points[i]);
        vec3_t candidate_point = ecs_scene_transform_point_world(&candidate_transform, sample_points[i]);
        char label[96];

        snprintf(label, sizeof(label), "transform-compose point-%zu", i + 1);
        mdcad_harness_expect(report,
                             label,
                             mdcad_compare_vec3_close(legacy_point, &candidate_point.x, 1e-4f));
    }

    return report->checks_failed == 0;
}

static bool mdcad_harness_compare_hierarchy_world_transform(mdcad_validation_report_t *report) {
    static const vec3_t sample_points[] = {
        { 0.0f, 0.0f, 0.0f },
        { 0.5f, -0.25f, 1.0f },
        { -1.25f, 0.75f, -0.5f },
    };
    TransformComp parent = mdcad_harness_make_transform(vec3_make(3.0f, -1.0f, 0.5f),
                                                        vec3_make(0.2f, 0.35f, -0.4f),
                                                        vec3_make(1.2f, 0.9f, 1.1f));
    TransformComp child = mdcad_harness_make_transform(vec3_make(-0.75f, 1.5f, 0.25f),
                                                       vec3_make(-0.15f, 0.4f, 0.9f),
                                                       vec3_make(0.6f, 1.4f, 0.8f));
    mat4_t legacy_parent = mdcad_harness_legacy_transform_from_components(parent.position, parent.rotation, parent.scale);
    mat4_t legacy_child_local = mdcad_harness_legacy_transform_from_components(child.position, child.rotation, child.scale);
    mat4_t legacy_child_world = mat4_mul(legacy_parent, legacy_child_local);
    size_t i;

    transform_comp_update(&parent);
    transform_comp_update_with_parent(&child, &parent.world_matrix);

    mdcad_harness_expect(report,
                         "hierarchy-world-transform parent matrix",
                         mdcad_compare_mat4_close(legacy_parent, parent.world_matrix.m, 1e-4f));
    mdcad_harness_expect(report,
                         "hierarchy-world-transform child matrix",
                         mdcad_compare_mat4_close(legacy_child_world, child.world_matrix.m, 1e-4f));

    for (i = 0; i < (sizeof(sample_points) / sizeof(sample_points[0])); ++i) {
        vec3_t legacy_point = mat4_transform_point(legacy_child_world, sample_points[i]);
        vec3_t candidate_point = ecs_scene_transform_point_world(&child.world_matrix, sample_points[i]);
        char label[96];

        snprintf(label, sizeof(label), "hierarchy-world-transform point-%zu", i + 1);
        mdcad_harness_expect(report,
                             label,
                             mdcad_compare_vec3_close(legacy_point, &candidate_point.x, 1e-4f));
    }

    return report->checks_failed == 0;
}

static void mdcad_harness_reset_bench_contexts(void) {
    orbit_camera_t camera = mdcad_harness_make_camera();
    mat4_t legacy_view = mdcad_harness_legacy_orbit_view(&camera);
    mat4_t legacy_proj = mdcad_harness_legacy_projection();
    mat4_t legacy_vp = mat4_mul(legacy_proj, legacy_view);
    mat4s cglm_view = orbit_camera_get_view_matrix_cglm(&camera);
    mat4s cglm_proj = mdcad_harness_cglm_projection();
    mat4s cglm_vp = glms_mat4_mul(cglm_proj, cglm_view);
    float screen_x;
    float screen_y_top;
    float screen_y_bottom;
    float ndc_x;
    float ndc_y;

    mdcad_harness_screen_sample(&screen_x, &screen_y_top, &screen_y_bottom, &ndc_x, &ndc_y);

    mdcad_legacy_mat4_mul_ctx.lhs = legacy_proj;
    mdcad_legacy_mat4_mul_ctx.rhs = legacy_view;
    mdcad_legacy_mat4_inv_ctx.matrix = legacy_vp;
    mdcad_legacy_screen_ray_ctx.ndc_x = ndc_x;
    mdcad_legacy_screen_ray_ctx.ndc_y = ndc_y;
    mdcad_legacy_screen_ray_ctx.ndc_step = 0.0005f;
    mdcad_legacy_screen_ray_ctx.inv_vp = mat4_inverse(legacy_vp);

    mdcad_cglm_mat4_mul_ctx.lhs = cglm_proj;
    mdcad_cglm_mat4_mul_ctx.rhs = cglm_view;
    mdcad_cglm_mat4_inv_ctx.matrix = cglm_vp;
    mdcad_cglm_screen_ray_ctx.viewport = mdcad_harness_vec4s(0.0f, 0.0f, 1280.0f, 720.0f);
    mdcad_cglm_screen_ray_ctx.near_pos = mdcad_harness_vec3s(vec3_make(screen_x, screen_y_bottom, 0.0f));
    mdcad_cglm_screen_ray_ctx.far_pos = mdcad_harness_vec3s(vec3_make(screen_x, screen_y_bottom, 1.0f));
    mdcad_cglm_screen_ray_ctx.x_step = 0.25f;
    mdcad_cglm_screen_ray_ctx.inv_vp = glms_mat4_inv(cglm_vp);
}

static void mdcad_harness_bench_legacy_mat4_mul(void *ctx_ptr) {
    mdcad_legacy_mat4_mul_ctx_t *ctx = (mdcad_legacy_mat4_mul_ctx_t *)ctx_ptr;
    mat4_t result = mat4_mul(ctx->lhs, ctx->rhs);

    mdcad_bench_sink += result.m[0];
    ctx->lhs = result;
}

static void mdcad_harness_bench_cglm_mat4_mul(void *ctx_ptr) {
    mdcad_cglm_mat4_mul_ctx_t *ctx = (mdcad_cglm_mat4_mul_ctx_t *)ctx_ptr;
    mat4s result = glms_mat4_mul(ctx->lhs, ctx->rhs);

    mdcad_bench_sink += ((const float *)result.raw)[0];
    ctx->lhs = result;
}

static void mdcad_harness_bench_legacy_mat4_inverse(void *ctx_ptr) {
    mdcad_legacy_mat4_inv_ctx_t *ctx = (mdcad_legacy_mat4_inv_ctx_t *)ctx_ptr;
    mat4_t result = mat4_inverse(ctx->matrix);

    mdcad_bench_sink += result.m[0];
    ctx->matrix.m[12] += 0.0001f;
}

static void mdcad_harness_bench_cglm_mat4_inv(void *ctx_ptr) {
    mdcad_cglm_mat4_inv_ctx_t *ctx = (mdcad_cglm_mat4_inv_ctx_t *)ctx_ptr;
    mat4s result = glms_mat4_inv(ctx->matrix);

    mdcad_bench_sink += ((const float *)result.raw)[0];
    ((float *)ctx->matrix.raw)[12] += 0.0001f;
}

static void mdcad_harness_bench_legacy_screen_ray(void *ctx_ptr) {
    mdcad_legacy_screen_ray_ctx_t *ctx = (mdcad_legacy_screen_ray_ctx_t *)ctx_ptr;
    ray_t ray = ray_from_screen(ctx->ndc_x, ctx->ndc_y, ctx->inv_vp);

    mdcad_bench_sink += ray.origin.x + ray.direction.y;
    ctx->ndc_x += ctx->ndc_step;
    if (ctx->ndc_x > 0.6f) {
        ctx->ndc_x = -0.4f;
    }
}

static void mdcad_harness_bench_cglm_screen_ray(void *ctx_ptr) {
    mdcad_cglm_screen_ray_ctx_t *ctx = (mdcad_cglm_screen_ray_ctx_t *)ctx_ptr;
    vec3s near_world = glms_unprojecti_zo(ctx->near_pos, ctx->inv_vp, ctx->viewport);
    vec3s far_world = glms_unprojecti_zo(ctx->far_pos, ctx->inv_vp, ctx->viewport);
    vec3s direction = glms_vec3_normalize(glms_vec3_sub(far_world, near_world));

    mdcad_bench_sink += near_world.raw[0] + direction.raw[1];
    ctx->near_pos.raw[0] += ctx->x_step;
    ctx->far_pos.raw[0] += ctx->x_step;
    if (ctx->near_pos.raw[0] > 1024.0f) {
        ctx->near_pos.raw[0] = 640.0f;
        ctx->far_pos.raw[0] = 640.0f;
    }
}

static void mdcad_harness_print_usage(const char *argv0) {
    fprintf(stderr,
            "Usage: %s [--list] [--mode compare|bench] [--iterations N] [--strict]\n",
            argv0);
}

static void mdcad_harness_list_cases(void) {
    size_t i;

    puts("COMPARE CASES");
    for (i = 0; i < (sizeof(mdcad_compare_cases) / sizeof(mdcad_compare_cases[0])); ++i) {
        printf("%s\n", mdcad_compare_cases[i].id);
    }

    puts("BENCH CASES");
    for (i = 0; i < (sizeof(mdcad_bench_cases) / sizeof(mdcad_bench_cases[0])); ++i) {
        printf("%s\n", mdcad_bench_cases[i].id);
    }
}

static int mdcad_harness_run_compare(bool strict) {
    mdcad_validation_report_t suite_report = { 0, 0 };
    size_t i;

    for (i = 0; i < (sizeof(mdcad_compare_cases) / sizeof(mdcad_compare_cases[0])); ++i) {
        mdcad_validation_report_t case_report = { 0, 0 };
        bool passed = mdcad_compare_cases[i].fn(&case_report);

        mdcad_harness_emit_compare_result(mdcad_compare_cases[i].id, &case_report);
        mdcad_validation_expect(&suite_report, passed, mdcad_compare_cases[i].id);
        if (strict && !passed) {
            break;
        }
    }

    return mdcad_validation_exit_code(&suite_report);
}

static int mdcad_harness_run_bench(uint64_t iterations) {
    size_t i;

    mdcad_harness_reset_bench_contexts();
    for (i = 0; i < (sizeof(mdcad_bench_cases) / sizeof(mdcad_bench_cases[0])); ++i) {
        mdcad_bench_result_t result = mdcad_bench_run(mdcad_bench_cases[i].id,
                                                      iterations,
                                                      mdcad_bench_cases[i].fn,
                                                      mdcad_bench_cases[i].ctx);
        double avg_ns = result.iterations == 0
            ? 0.0
            : (result.elapsed_ms * 1000000.0) / (double)result.iterations;

        printf("BENCH %s iterations=%llu elapsed_ms=%.3f avg_ns=%.3f\n",
               result.name,
               (unsigned long long)result.iterations,
               result.elapsed_ms,
               avg_ns);
    }
    return 0;
}

int main(int argc, char **argv) {
    mdcad_harness_mode_t mode = MDCAD_HARNESS_MODE_COMPARE;
    uint64_t iterations = 1000;
    bool strict = false;
    bool list_only = false;
    int i;

    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--list") == 0) {
            list_only = true;
        } else if (strcmp(argv[i], "--mode") == 0) {
            if (i + 1 >= argc) {
                mdcad_harness_print_usage(argv[0]);
                return 1;
            }
            ++i;
            if (strcmp(argv[i], "compare") == 0) {
                mode = MDCAD_HARNESS_MODE_COMPARE;
            } else if (strcmp(argv[i], "bench") == 0) {
                mode = MDCAD_HARNESS_MODE_BENCH;
            } else {
                fprintf(stderr, "Unknown mode: %s\n", argv[i]);
                return 1;
            }
        } else if (strcmp(argv[i], "--iterations") == 0) {
            char *end = NULL;
            if (i + 1 >= argc) {
                mdcad_harness_print_usage(argv[0]);
                return 1;
            }
            ++i;
            iterations = strtoull(argv[i], &end, 10);
            if (!end || *end != '\0') {
                fprintf(stderr, "Invalid iteration count: %s\n", argv[i]);
                return 1;
            }
        } else if (strcmp(argv[i], "--strict") == 0) {
            strict = true;
        } else {
            fprintf(stderr, "Unknown argument: %s\n", argv[i]);
            mdcad_harness_print_usage(argv[0]);
            return 1;
        }
    }

    if (list_only) {
        mdcad_harness_list_cases();
        return 0;
    }

    if (mode == MDCAD_HARNESS_MODE_COMPARE) {
        return mdcad_harness_run_compare(strict);
    }

    return mdcad_harness_run_bench(iterations);
}
