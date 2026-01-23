//------------------------------------------------------------------------------
// instanced_polylines.h - Instanced thick polyline rendering (header-only)
//
// Renders animated polyline strips with thick lines and round joins.
// Each polyline is converted to multiple line segment instances plus
// circle instances at interior vertices for round joins.
//------------------------------------------------------------------------------
#ifndef INSTANCED_POLYLINES_H
#define INSTANCED_POLYLINES_H

#include "platform.h"
#include "sokol_gfx.h"
#include "math3d.h"
#include "shaders/instanced_line_shaders.h"
#include "shaders/join_shaders.h"
#include <math.h>
#include <stdlib.h>

//------------------------------------------------------------------------------
// Configuration
//------------------------------------------------------------------------------
#define POLYLINES_COUNT 5
#define POLYLINE_POINTS 15
#define POLYLINES_MAX_SEGMENTS (POLYLINES_COUNT * (POLYLINE_POINTS - 1))
#define POLYLINES_MAX_JOINS (POLYLINES_COUNT * (POLYLINE_POINTS - 2))
#define POLYLINES_CAP_SEGMENTS 12
#define POLYLINES_JOIN_SEGMENTS 16
#define POLYLINES_DEFAULT_WIDTH 0.08f
#define POLYLINES_DEFAULT_SCALE 4.0f
#define POLYLINES_DEFAULT_SPEED 0.5f

//------------------------------------------------------------------------------
// Types
//------------------------------------------------------------------------------

typedef struct {
    float x, y, z;
} polyline_template_vertex_t;

typedef struct {
    float ax, ay, az;
    float bx, by, bz;
    float r, g, b, a;
} polyline_segment_instance_t;

// Circle join instance (just a point and color, rendered as full circle)
typedef struct {
    float px, py, pz;
    float r, g, b, a;
} polyline_join_instance_t;

typedef struct {
    mat4_t mvp;
    float line_width;
    float aspect_ratio;
    float _pad[2];
} polyline_params_t;

// Per-polyline animation parameters
typedef struct {
    float freq_x, freq_y, freq_z;
    float phase_x, phase_y, phase_z;
    float r, g, b;
} polyline_anim_t;

// Main state
typedef struct {
    // Line segment rendering
    sg_pipeline segment_pip;
    sg_buffer segment_template_vbuf;
    sg_buffer segment_template_ibuf;
    sg_buffer segment_instance_buf;
    sg_shader segment_shd;
    int segment_template_vertex_count;
    int segment_template_index_count;

    // Join circle rendering
    sg_pipeline join_pip;
    sg_buffer join_template_vbuf;
    sg_buffer join_template_ibuf;
    sg_buffer join_instance_buf;
    sg_shader join_shd;
    int join_template_vertex_count;
    int join_template_index_count;

    // Animation data
    polyline_anim_t animations[POLYLINES_COUNT];

    // Instance data (updated each frame)
    polyline_segment_instance_t segments[POLYLINES_MAX_SEGMENTS];
    polyline_join_instance_t joins[POLYLINES_MAX_JOINS];
    int segment_count;
    int join_count;

    // Parameters
    float line_width;
    float scale;
    float speed;

    uint32_t rng_state;
} instanced_polylines_t;

//------------------------------------------------------------------------------
// RNG
//------------------------------------------------------------------------------
static inline float polylines_randf(instanced_polylines_t* pl) {
    pl->rng_state = pl->rng_state * 1103515245u + 12345u;
    return (float)(pl->rng_state & 0x7FFFFFFFu) / (float)0x7FFFFFFFu;
}

//------------------------------------------------------------------------------
// Template generation for line segments (same as instanced_lines)
//------------------------------------------------------------------------------
static inline void polylines_generate_segment_template(
    polyline_template_vertex_t* vertices, int* vertex_count,
    uint16_t* indices, int* index_count,
    int cap_segments
) {
    int vi = 0;
    int ii = 0;

    // Main rectangle
    vertices[vi++] = (polyline_template_vertex_t){ 0.0f, -0.5f, 0.0f };
    vertices[vi++] = (polyline_template_vertex_t){ 0.0f,  0.5f, 0.0f };
    vertices[vi++] = (polyline_template_vertex_t){ 0.0f, -0.5f, 1.0f };
    vertices[vi++] = (polyline_template_vertex_t){ 0.0f,  0.5f, 1.0f };

    indices[ii++] = 0; indices[ii++] = 2; indices[ii++] = 1;
    indices[ii++] = 1; indices[ii++] = 2; indices[ii++] = 3;

    // Left semicircle cap
    int cap_a_center = vi;
    vertices[vi++] = (polyline_template_vertex_t){ 0.0f, 0.0f, 0.0f };
    for (int i = 0; i <= cap_segments; i++) {
        float angle = 3.14159265359f * 0.5f + 3.14159265359f * (float)i / (float)cap_segments;
        float x = cosf(angle) * 0.5f;
        float y = sinf(angle) * 0.5f;
        vertices[vi++] = (polyline_template_vertex_t){ x, y, 0.0f };
    }
    for (int i = 0; i < cap_segments; i++) {
        indices[ii++] = cap_a_center;
        indices[ii++] = cap_a_center + 1 + i;
        indices[ii++] = cap_a_center + 2 + i;
    }

    // Right semicircle cap
    int cap_b_center = vi;
    vertices[vi++] = (polyline_template_vertex_t){ 0.0f, 0.0f, 1.0f };
    for (int i = 0; i <= cap_segments; i++) {
        float angle = -3.14159265359f * 0.5f + 3.14159265359f * (float)i / (float)cap_segments;
        float x = cosf(angle) * 0.5f;
        float y = sinf(angle) * 0.5f;
        vertices[vi++] = (polyline_template_vertex_t){ x, y, 1.0f };
    }
    for (int i = 0; i < cap_segments; i++) {
        indices[ii++] = cap_b_center;
        indices[ii++] = cap_b_center + 1 + i;
        indices[ii++] = cap_b_center + 2 + i;
    }

    *vertex_count = vi;
    *index_count = ii;
}

//------------------------------------------------------------------------------
// Template generation for join circles (full circle, centered at origin)
//------------------------------------------------------------------------------
static inline void polylines_generate_join_template(
    polyline_template_vertex_t* vertices, int* vertex_count,
    uint16_t* indices, int* index_count,
    int segments
) {
    int vi = 0;
    int ii = 0;

    // Center vertex (z=0.5 to place at the center between A and B semantically, but we use just the point)
    vertices[vi++] = (polyline_template_vertex_t){ 0.0f, 0.0f, 0.0f };

    // Circle edge vertices
    for (int i = 0; i <= segments; i++) {
        float angle = 2.0f * 3.14159265359f * (float)i / (float)segments;
        float x = cosf(angle) * 0.5f;
        float y = sinf(angle) * 0.5f;
        vertices[vi++] = (polyline_template_vertex_t){ x, y, 0.0f };
    }

    // Triangle fan
    for (int i = 0; i < segments; i++) {
        indices[ii++] = 0;
        indices[ii++] = 1 + i;
        indices[ii++] = 2 + i;
    }

    *vertex_count = vi;
    *index_count = ii;
}

//------------------------------------------------------------------------------
// Initialization
//------------------------------------------------------------------------------
static inline void instanced_polylines_regenerate(instanced_polylines_t* pl) {
    pl->rng_state = 54321u;

    for (int i = 0; i < POLYLINES_COUNT; i++) {
        polyline_anim_t* a = &pl->animations[i];
        // Lissajous-like frequencies
        a->freq_x = 1.0f + polylines_randf(pl) * 2.0f;
        a->freq_y = 1.0f + polylines_randf(pl) * 2.0f;
        a->freq_z = 0.5f + polylines_randf(pl) * 1.0f;
        a->phase_x = polylines_randf(pl) * 6.28318530718f;
        a->phase_y = polylines_randf(pl) * 6.28318530718f;
        a->phase_z = polylines_randf(pl) * 6.28318530718f;

        // Distinct colors for each polyline
        float hue = (float)i / (float)POLYLINES_COUNT;
        float h = hue * 6.0f;
        int hi = (int)h % 6;
        float f = h - (float)hi;
        float p = 0.3f;
        float q = 1.0f - 0.7f * f;
        float t = 1.0f - 0.7f * (1.0f - f);
        switch (hi) {
            case 0: a->r = 1.0f; a->g = t;    a->b = p;    break;
            case 1: a->r = q;    a->g = 1.0f; a->b = p;    break;
            case 2: a->r = p;    a->g = 1.0f; a->b = t;    break;
            case 3: a->r = p;    a->g = q;    a->b = 1.0f; break;
            case 4: a->r = t;    a->g = p;    a->b = 1.0f; break;
            default: a->r = 1.0f; a->g = p;   a->b = q;    break;
        }
    }
}

static inline void instanced_polylines_init(instanced_polylines_t* pl) {
    pl->line_width = POLYLINES_DEFAULT_WIDTH;
    pl->scale = POLYLINES_DEFAULT_SCALE;
    pl->speed = POLYLINES_DEFAULT_SPEED;
    pl->rng_state = 54321u;

    instanced_polylines_regenerate(pl);

    // Generate segment template
    int max_seg_vertices = 4 + 2 * (POLYLINES_CAP_SEGMENTS + 2);
    int max_seg_indices = 6 + 2 * POLYLINES_CAP_SEGMENTS * 3;

    polyline_template_vertex_t* seg_verts = (polyline_template_vertex_t*)malloc(max_seg_vertices * sizeof(polyline_template_vertex_t));
    uint16_t* seg_indices = (uint16_t*)malloc(max_seg_indices * sizeof(uint16_t));

    polylines_generate_segment_template(
        seg_verts, &pl->segment_template_vertex_count,
        seg_indices, &pl->segment_template_index_count,
        POLYLINES_CAP_SEGMENTS
    );

    pl->segment_template_vbuf = sg_make_buffer(&(sg_buffer_desc){
        .usage.vertex_buffer = true,
        .data = { .ptr = seg_verts, .size = pl->segment_template_vertex_count * sizeof(polyline_template_vertex_t) },
        .label = "polylines-segment-template-vbuf"
    });

    pl->segment_template_ibuf = sg_make_buffer(&(sg_buffer_desc){
        .usage.index_buffer = true,
        .data = { .ptr = seg_indices, .size = pl->segment_template_index_count * sizeof(uint16_t) },
        .label = "polylines-segment-template-ibuf"
    });

    free(seg_verts);
    free(seg_indices);

    // Generate join template (full circle)
    int max_join_vertices = POLYLINES_JOIN_SEGMENTS + 2;
    int max_join_indices = POLYLINES_JOIN_SEGMENTS * 3;

    polyline_template_vertex_t* join_verts = (polyline_template_vertex_t*)malloc(max_join_vertices * sizeof(polyline_template_vertex_t));
    uint16_t* join_indices = (uint16_t*)malloc(max_join_indices * sizeof(uint16_t));

    polylines_generate_join_template(
        join_verts, &pl->join_template_vertex_count,
        join_indices, &pl->join_template_index_count,
        POLYLINES_JOIN_SEGMENTS
    );

    pl->join_template_vbuf = sg_make_buffer(&(sg_buffer_desc){
        .usage.vertex_buffer = true,
        .data = { .ptr = join_verts, .size = pl->join_template_vertex_count * sizeof(polyline_template_vertex_t) },
        .label = "polylines-join-template-vbuf"
    });

    pl->join_template_ibuf = sg_make_buffer(&(sg_buffer_desc){
        .usage.index_buffer = true,
        .data = { .ptr = join_indices, .size = pl->join_template_index_count * sizeof(uint16_t) },
        .label = "polylines-join-template-ibuf"
    });

    free(join_verts);
    free(join_indices);

    // Instance buffers
    pl->segment_instance_buf = sg_make_buffer(&(sg_buffer_desc){
        .usage.vertex_buffer = true,
        .usage.stream_update = true,
        .size = POLYLINES_MAX_SEGMENTS * sizeof(polyline_segment_instance_t),
        .label = "polylines-segment-instance-buf"
    });

    pl->join_instance_buf = sg_make_buffer(&(sg_buffer_desc){
        .usage.vertex_buffer = true,
        .usage.stream_update = true,
        .size = POLYLINES_MAX_JOINS * sizeof(polyline_join_instance_t),
        .label = "polylines-join-instance-buf"
    });

    // Segment shader (reuse instanced line shader)
    pl->segment_shd = sg_make_shader(&(sg_shader_desc){
        .vertex_func = { .source = instanced_line_vs_source, .entry = "vs_main" },
        .fragment_func = { .source = instanced_line_fs_source, .entry = "fs_main" },
        .uniform_blocks[0] = {
            .stage = SG_SHADERSTAGE_VERTEX,
            .size = sizeof(polyline_params_t),
            .layout = SG_UNIFORMLAYOUT_STD140,
            .glsl_uniforms = {
                [0] = { .type = SG_UNIFORMTYPE_MAT4, .glsl_name = "mvp" },
                [1] = { .type = SG_UNIFORMTYPE_FLOAT, .glsl_name = "line_width" },
                [2] = { .type = SG_UNIFORMTYPE_FLOAT, .glsl_name = "aspect_ratio" },
            }
        },
        .label = "polylines-segment-shader"
    });

    // Segment pipeline
    pl->segment_pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = pl->segment_shd,
        .layout = {
            .buffers = {
                [0] = { .step_func = SG_VERTEXSTEP_PER_VERTEX },
                [1] = { .step_func = SG_VERTEXSTEP_PER_INSTANCE },
            },
            .attrs = {
                [0] = { .buffer_index = 0, .format = SG_VERTEXFORMAT_FLOAT3 },
                [1] = { .buffer_index = 1, .format = SG_VERTEXFORMAT_FLOAT3, .offset = 0 },
                [2] = { .buffer_index = 1, .format = SG_VERTEXFORMAT_FLOAT3, .offset = 12 },
                [3] = { .buffer_index = 1, .format = SG_VERTEXFORMAT_FLOAT4, .offset = 24 },
            }
        },
        .index_type = SG_INDEXTYPE_UINT16,
        .primitive_type = SG_PRIMITIVETYPE_TRIANGLES,
        .depth = {
            .compare = SG_COMPAREFUNC_LESS_EQUAL,
            .write_enabled = true,
            .pixel_format = SG_PIXELFORMAT_DEPTH
        },
        .colors[0].pixel_format = SG_PIXELFORMAT_RGBA8,
        .cull_mode = SG_CULLMODE_NONE,
        .label = "polylines-segment-pipeline"
    });

    // Join shader
    pl->join_shd = sg_make_shader(&(sg_shader_desc){
        .vertex_func = { .source = join_vs_source, .entry = "vs_main" },
        .fragment_func = { .source = join_fs_source, .entry = "fs_main" },
        .uniform_blocks[0] = {
            .stage = SG_SHADERSTAGE_VERTEX,
            .size = sizeof(polyline_params_t),
            .layout = SG_UNIFORMLAYOUT_STD140,
            .glsl_uniforms = {
                [0] = { .type = SG_UNIFORMTYPE_MAT4, .glsl_name = "mvp" },
                [1] = { .type = SG_UNIFORMTYPE_FLOAT, .glsl_name = "line_width" },
                [2] = { .type = SG_UNIFORMTYPE_FLOAT, .glsl_name = "aspect_ratio" },
            }
        },
        .label = "polylines-join-shader"
    });

    // Join pipeline (different instance layout - only point + color)
    pl->join_pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = pl->join_shd,
        .layout = {
            .buffers = {
                [0] = { .step_func = SG_VERTEXSTEP_PER_VERTEX },
                [1] = { .step_func = SG_VERTEXSTEP_PER_INSTANCE },
            },
            .attrs = {
                [0] = { .buffer_index = 0, .format = SG_VERTEXFORMAT_FLOAT3 },
                [1] = { .buffer_index = 1, .format = SG_VERTEXFORMAT_FLOAT3, .offset = 0 },
                [2] = { .buffer_index = 1, .format = SG_VERTEXFORMAT_FLOAT4, .offset = 12 },
            }
        },
        .index_type = SG_INDEXTYPE_UINT16,
        .primitive_type = SG_PRIMITIVETYPE_TRIANGLES,
        .depth = {
            .compare = SG_COMPAREFUNC_LESS_EQUAL,
            .write_enabled = true,
            .pixel_format = SG_PIXELFORMAT_DEPTH
        },
        .colors[0].pixel_format = SG_PIXELFORMAT_RGBA8,
        .cull_mode = SG_CULLMODE_NONE,
        .label = "polylines-join-pipeline"
    });
}

//------------------------------------------------------------------------------
// Update
//------------------------------------------------------------------------------
static inline void instanced_polylines_update(instanced_polylines_t* pl, float time) {
    pl->segment_count = 0;
    pl->join_count = 0;

    float t = time * pl->speed;

    for (int p = 0; p < POLYLINES_COUNT; p++) {
        polyline_anim_t* a = &pl->animations[p];

        // Generate points along a Lissajous curve
        float prev_x = 0, prev_y = 0, prev_z = 0;

        for (int i = 0; i < POLYLINE_POINTS; i++) {
            float u = (float)i / (float)(POLYLINE_POINTS - 1);
            float angle = u * 2.0f * 3.14159265359f;

            float x = sinf(a->freq_x * angle + a->phase_x + t) * pl->scale;
            float y = sinf(a->freq_y * angle + a->phase_y + t * 1.3f) * pl->scale;
            float z = sinf(a->freq_z * angle + a->phase_z + t * 0.7f) * pl->scale * 0.5f;

            // Offset each polyline slightly
            float offset = (float)p * 0.3f;
            y += offset;

            if (i > 0) {
                // Add segment
                polyline_segment_instance_t* seg = &pl->segments[pl->segment_count++];
                seg->ax = prev_x;
                seg->ay = prev_y;
                seg->az = prev_z;
                seg->bx = x;
                seg->by = y;
                seg->bz = z;
                seg->r = a->r;
                seg->g = a->g;
                seg->b = a->b;
                seg->a = 1.0f;
            }

            if (i > 0 && i < POLYLINE_POINTS - 1) {
                // Add join at interior vertex
                polyline_join_instance_t* join = &pl->joins[pl->join_count++];
                join->px = x;
                join->py = y;
                join->pz = z;
                join->r = a->r;
                join->g = a->g;
                join->b = a->b;
                join->a = 1.0f;
            }

            prev_x = x;
            prev_y = y;
            prev_z = z;
        }
    }

    // Upload instance data
    if (pl->segment_count > 0) {
        sg_update_buffer(pl->segment_instance_buf, &(sg_range){
            .ptr = pl->segments,
            .size = pl->segment_count * sizeof(polyline_segment_instance_t)
        });
    }

    if (pl->join_count > 0) {
        sg_update_buffer(pl->join_instance_buf, &(sg_range){
            .ptr = pl->joins,
            .size = pl->join_count * sizeof(polyline_join_instance_t)
        });
    }
}

//------------------------------------------------------------------------------
// Draw
//------------------------------------------------------------------------------
static inline void instanced_polylines_draw(instanced_polylines_t* pl, mat4_t mvp, float aspect_ratio) {
    if (pl->segment_count == 0) return;

    polyline_params_t params = {
        .mvp = mvp,
        .line_width = pl->line_width,
        .aspect_ratio = aspect_ratio,
    };

    // Draw segments
    sg_apply_pipeline(pl->segment_pip);
    sg_apply_bindings(&(sg_bindings){
        .vertex_buffers = {
            [0] = pl->segment_template_vbuf,
            [1] = pl->segment_instance_buf,
        },
        .index_buffer = pl->segment_template_ibuf,
    });
    sg_apply_uniforms(0, &SG_RANGE(params));
    sg_draw(0, pl->segment_template_index_count, pl->segment_count);

    // Draw joins
    if (pl->join_count > 0) {
        sg_apply_pipeline(pl->join_pip);
        sg_apply_bindings(&(sg_bindings){
            .vertex_buffers = {
                [0] = pl->join_template_vbuf,
                [1] = pl->join_instance_buf,
            },
            .index_buffer = pl->join_template_ibuf,
        });
        sg_apply_uniforms(0, &SG_RANGE(params));
        sg_draw(0, pl->join_template_index_count, pl->join_count);
    }
}

//------------------------------------------------------------------------------
// Shutdown
//------------------------------------------------------------------------------
static inline void instanced_polylines_shutdown(instanced_polylines_t* pl) {
    sg_destroy_pipeline(pl->segment_pip);
    sg_destroy_pipeline(pl->join_pip);
    sg_destroy_shader(pl->segment_shd);
    sg_destroy_shader(pl->join_shd);
    sg_destroy_buffer(pl->segment_template_vbuf);
    sg_destroy_buffer(pl->segment_template_ibuf);
    sg_destroy_buffer(pl->segment_instance_buf);
    sg_destroy_buffer(pl->join_template_vbuf);
    sg_destroy_buffer(pl->join_template_ibuf);
    sg_destroy_buffer(pl->join_instance_buf);
}

#endif // INSTANCED_POLYLINES_H
