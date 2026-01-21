//------------------------------------------------------------------------------
// instanced_lines_alpha.h - Alpha-blended instanced thick line rendering (header-only)
//
// Renders animated thick lines with round caps and alpha blending support.
// Uses same instancing technique as instanced_lines.h but with:
// - Alpha blending enabled
// - Depth write disabled for proper transparency
// - Per-line alpha values
//------------------------------------------------------------------------------
#ifndef INSTANCED_LINES_ALPHA_H
#define INSTANCED_LINES_ALPHA_H

#include "platform.h"
#include "sokol_gfx.h"
#include "math3d.h"
#include "shaders/instanced_line_shaders.h"
#include <math.h>
#include <stdlib.h>

//------------------------------------------------------------------------------
// Configuration
//------------------------------------------------------------------------------
#define ALPHA_LINES_COUNT 2000
#define ALPHA_LINES_CAP_SEGMENTS 8
#define ALPHA_LINES_DEFAULT_WIDTH 0.03f
#define ALPHA_LINES_DEFAULT_SPHERE_RADIUS 5.0f
#define ALPHA_LINES_DEFAULT_WIGGLE_AMPLITUDE 0.3f
#define ALPHA_LINES_DEFAULT_WIGGLE_FREQUENCY 2.0f
#define ALPHA_LINES_DEFAULT_LINE_LENGTH 0.3f
#define ALPHA_LINES_DEFAULT_BASE_ALPHA 0.4f

//------------------------------------------------------------------------------
// Types
//------------------------------------------------------------------------------

typedef struct {
    float x, y, z;
} alpha_template_vertex_t;

typedef struct {
    float ax, ay, az;  // Point A position
    float bx, by, bz;  // Point B position
    float r, g, b, a;  // Color with alpha
} alpha_line_instance_t;

typedef struct {
    vec3_t base_pos;
    vec3_t direction;
    vec3_t motion_dir;
    float phase;
    float r, g, b;
    float alpha;  // Per-line alpha
} alpha_line_data_t;

typedef struct {
    mat4_t mvp;
    float line_width;
    float aspect_ratio;
    float _pad[2];
} alpha_line_params_t;

typedef struct {
    // GPU resources
    sg_pipeline pip;
    sg_buffer template_vbuf;
    sg_buffer template_ibuf;
    sg_buffer instance_buf;
    sg_shader shd;

    // Template geometry info
    int template_vertex_count;
    int template_index_count;

    // CPU data
    alpha_line_data_t lines[ALPHA_LINES_COUNT];
    alpha_line_instance_t instances[ALPHA_LINES_COUNT];

    // Parameters
    float line_width;
    float sphere_radius;
    float wiggle_amplitude;
    float wiggle_frequency;
    float line_length;
    float base_alpha;  // Base transparency

    // RNG
    uint32_t rng_state;
    uint32_t seed_counter;
} instanced_lines_alpha_t;

//------------------------------------------------------------------------------
// Random number generation
//------------------------------------------------------------------------------
static inline float alpha_lines_randf(instanced_lines_alpha_t* il) {
    il->rng_state = il->rng_state * 1103515245u + 12345u;
    return (float)(il->rng_state & 0x7FFFFFFFu) / (float)0x7FFFFFFFu;
}

static inline vec3_t alpha_lines_random_point_in_sphere(instanced_lines_alpha_t* il, float radius) {
    vec3_t p;
    do {
        p.x = alpha_lines_randf(il) * 2.0f - 1.0f;
        p.y = alpha_lines_randf(il) * 2.0f - 1.0f;
        p.z = alpha_lines_randf(il) * 2.0f - 1.0f;
    } while (p.x * p.x + p.y * p.y + p.z * p.z > 1.0f);
    p.x *= radius;
    p.y *= radius;
    p.z *= radius;
    return p;
}

static inline vec3_t alpha_lines_random_direction(instanced_lines_alpha_t* il) {
    vec3_t d = alpha_lines_random_point_in_sphere(il, 1.0f);
    float len = sqrtf(d.x * d.x + d.y * d.y + d.z * d.z);
    if (len > 0.0001f) {
        d.x /= len;
        d.y /= len;
        d.z /= len;
    } else {
        d = (vec3_t){0.0f, 1.0f, 0.0f};
    }
    return d;
}

//------------------------------------------------------------------------------
// Regeneration
//------------------------------------------------------------------------------
static inline void alpha_lines_regenerate_positions(instanced_lines_alpha_t* il) {
    il->rng_state = 12345u + il->seed_counter++ * 111u;
    for (int i = 0; i < ALPHA_LINES_COUNT; i++) {
        il->lines[i].base_pos = alpha_lines_random_point_in_sphere(il, il->sphere_radius);
    }
}

static inline void alpha_lines_regenerate_directions(instanced_lines_alpha_t* il) {
    il->rng_state = 67890u + il->seed_counter++ * 222u;
    for (int i = 0; i < ALPHA_LINES_COUNT; i++) {
        il->lines[i].direction = alpha_lines_random_direction(il);
        il->lines[i].motion_dir = alpha_lines_random_direction(il);
        il->lines[i].phase = alpha_lines_randf(il) * 6.28318530718f;
    }
}

static inline void alpha_lines_regenerate_colors(instanced_lines_alpha_t* il) {
    il->rng_state = 13579u + il->seed_counter++ * 333u;
    for (int i = 0; i < ALPHA_LINES_COUNT; i++) {
        float hue = alpha_lines_randf(il);
        float sat = 0.7f + alpha_lines_randf(il) * 0.3f;
        float val = 0.8f + alpha_lines_randf(il) * 0.2f;

        float h = hue * 6.0f;
        int hi = (int)h % 6;
        float f = h - (float)hi;
        float p = val * (1.0f - sat);
        float q = val * (1.0f - sat * f);
        float t = val * (1.0f - sat * (1.0f - f));

        float r, g, b;
        switch (hi) {
            case 0: r = val; g = t;   b = p;   break;
            case 1: r = q;   g = val; b = p;   break;
            case 2: r = p;   g = val; b = t;   break;
            case 3: r = p;   g = q;   b = val; break;
            case 4: r = t;   g = p;   b = val; break;
            default: r = val; g = p;   b = q;   break;
        }

        il->lines[i].r = r;
        il->lines[i].g = g;
        il->lines[i].b = b;

        // Vary alpha slightly per line
        il->lines[i].alpha = il->base_alpha + (alpha_lines_randf(il) - 0.5f) * 0.2f;
        if (il->lines[i].alpha < 0.1f) il->lines[i].alpha = 0.1f;
        if (il->lines[i].alpha > 1.0f) il->lines[i].alpha = 1.0f;
    }
}

static inline void alpha_lines_regenerate_all(instanced_lines_alpha_t* il) {
    alpha_lines_regenerate_positions(il);
    alpha_lines_regenerate_directions(il);
    alpha_lines_regenerate_colors(il);
}

//------------------------------------------------------------------------------
// Template geometry generation (same as instanced_lines.h)
//------------------------------------------------------------------------------
static inline void alpha_lines_generate_template(
    alpha_template_vertex_t* vertices, int* vertex_count,
    uint16_t* indices, int* index_count,
    int cap_segments
) {
    int vi = 0;
    int ii = 0;

    // Main rectangle body
    vertices[vi++] = (alpha_template_vertex_t){ 0.0f, -0.5f, 0.0f };
    vertices[vi++] = (alpha_template_vertex_t){ 0.0f,  0.5f, 0.0f };
    vertices[vi++] = (alpha_template_vertex_t){ 0.0f, -0.5f, 1.0f };
    vertices[vi++] = (alpha_template_vertex_t){ 0.0f,  0.5f, 1.0f };

    indices[ii++] = 0; indices[ii++] = 2; indices[ii++] = 1;
    indices[ii++] = 1; indices[ii++] = 2; indices[ii++] = 3;

    // Left semicircle cap (at point A, z=0)
    int cap_a_center = vi;
    vertices[vi++] = (alpha_template_vertex_t){ 0.0f, 0.0f, 0.0f };

    for (int i = 0; i <= cap_segments; i++) {
        float angle = 3.14159265359f * 0.5f + 3.14159265359f * (float)i / (float)cap_segments;
        float x = cosf(angle) * 0.5f;
        float y = sinf(angle) * 0.5f;
        vertices[vi++] = (alpha_template_vertex_t){ x, y, 0.0f };
    }

    for (int i = 0; i < cap_segments; i++) {
        indices[ii++] = cap_a_center;
        indices[ii++] = cap_a_center + 1 + i;
        indices[ii++] = cap_a_center + 2 + i;
    }

    // Right semicircle cap (at point B, z=1)
    int cap_b_center = vi;
    vertices[vi++] = (alpha_template_vertex_t){ 0.0f, 0.0f, 1.0f };

    for (int i = 0; i <= cap_segments; i++) {
        float angle = -3.14159265359f * 0.5f + 3.14159265359f * (float)i / (float)cap_segments;
        float x = cosf(angle) * 0.5f;
        float y = sinf(angle) * 0.5f;
        vertices[vi++] = (alpha_template_vertex_t){ x, y, 1.0f };
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
// Core functions
//------------------------------------------------------------------------------
static inline void instanced_lines_alpha_init(instanced_lines_alpha_t* il) {
    // Initialize parameters
    il->line_width = ALPHA_LINES_DEFAULT_WIDTH;
    il->sphere_radius = ALPHA_LINES_DEFAULT_SPHERE_RADIUS;
    il->wiggle_amplitude = ALPHA_LINES_DEFAULT_WIGGLE_AMPLITUDE;
    il->wiggle_frequency = ALPHA_LINES_DEFAULT_WIGGLE_FREQUENCY;
    il->line_length = ALPHA_LINES_DEFAULT_LINE_LENGTH;
    il->base_alpha = ALPHA_LINES_DEFAULT_BASE_ALPHA;
    il->rng_state = 12345u;
    il->seed_counter = 0u;

    // Generate line data
    alpha_lines_regenerate_all(il);

    // Generate template geometry
    int max_vertices = 4 + 2 * (ALPHA_LINES_CAP_SEGMENTS + 2);
    int max_indices = 6 + 2 * ALPHA_LINES_CAP_SEGMENTS * 3;

    alpha_template_vertex_t* template_vertices = (alpha_template_vertex_t*)malloc(max_vertices * sizeof(alpha_template_vertex_t));
    uint16_t* template_indices = (uint16_t*)malloc(max_indices * sizeof(uint16_t));

    alpha_lines_generate_template(
        template_vertices, &il->template_vertex_count,
        template_indices, &il->template_index_count,
        ALPHA_LINES_CAP_SEGMENTS
    );

    // Create template vertex buffer
    il->template_vbuf = sg_make_buffer(&(sg_buffer_desc){
        .usage.vertex_buffer = true,
        .data = {
            .ptr = template_vertices,
            .size = il->template_vertex_count * sizeof(alpha_template_vertex_t)
        },
        .label = "alpha-lines-template-vbuf"
    });

    // Create template index buffer
    il->template_ibuf = sg_make_buffer(&(sg_buffer_desc){
        .usage.index_buffer = true,
        .data = {
            .ptr = template_indices,
            .size = il->template_index_count * sizeof(uint16_t)
        },
        .label = "alpha-lines-template-ibuf"
    });

    free(template_vertices);
    free(template_indices);

    // Create instance buffer
    il->instance_buf = sg_make_buffer(&(sg_buffer_desc){
        .usage.vertex_buffer = true,
        .usage.stream_update = true,
        .size = ALPHA_LINES_COUNT * sizeof(alpha_line_instance_t),
        .label = "alpha-lines-instance-buf"
    });

    // Create shader (same as instanced_lines)
    il->shd = sg_make_shader(&(sg_shader_desc){
        .vertex_func = {
            .source = instanced_line_vs_source,
            .entry = "vs_main",
        },
        .fragment_func = {
            .source = instanced_line_fs_source,
            .entry = "fs_main",
        },
        .uniform_blocks[0] = {
            .stage = SG_SHADERSTAGE_VERTEX,
            .size = sizeof(alpha_line_params_t),
            .layout = SG_UNIFORMLAYOUT_STD140,
            .glsl_uniforms = {
                [0] = { .type = SG_UNIFORMTYPE_MAT4, .glsl_name = "mvp" },
                [1] = { .type = SG_UNIFORMTYPE_FLOAT, .glsl_name = "line_width" },
                [2] = { .type = SG_UNIFORMTYPE_FLOAT, .glsl_name = "aspect_ratio" },
            }
        },
        .label = "alpha-lines-shader"
    });

    // Create pipeline WITH ALPHA BLENDING
    il->pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = il->shd,
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
            .write_enabled = false,  // Disable depth write for transparency
            .pixel_format = SG_PIXELFORMAT_DEPTH
        },
        .colors[0] = {
            .pixel_format = SG_PIXELFORMAT_RGBA8,
            .blend = {
                .enabled = true,
                .src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA,
                .dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                .op_rgb = SG_BLENDOP_ADD,
                .src_factor_alpha = SG_BLENDFACTOR_ONE,
                .dst_factor_alpha = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                .op_alpha = SG_BLENDOP_ADD,
            },
        },
        .cull_mode = SG_CULLMODE_NONE,
        .label = "alpha-lines-pipeline"
    });
}

static inline void instanced_lines_alpha_update(instanced_lines_alpha_t* il, float time) {
    for (int i = 0; i < ALPHA_LINES_COUNT; i++) {
        alpha_line_data_t* line = &il->lines[i];

        // Sin-wave wiggle
        float wave = sinf(time * il->wiggle_frequency + line->phase);
        float offset_x = line->motion_dir.x * wave * il->wiggle_amplitude;
        float offset_y = line->motion_dir.y * wave * il->wiggle_amplitude;
        float offset_z = line->motion_dir.z * wave * il->wiggle_amplitude;

        float cx = line->base_pos.x + offset_x;
        float cy = line->base_pos.y + offset_y;
        float cz = line->base_pos.z + offset_z;

        float dx = line->direction.x * il->line_length;
        float dy = line->direction.y * il->line_length;
        float dz = line->direction.z * il->line_length;

        alpha_line_instance_t* inst = &il->instances[i];
        inst->ax = cx - dx;
        inst->ay = cy - dy;
        inst->az = cz - dz;
        inst->bx = cx + dx;
        inst->by = cy + dy;
        inst->bz = cz + dz;
        inst->r = line->r;
        inst->g = line->g;
        inst->b = line->b;
        inst->a = line->alpha;
    }

    sg_update_buffer(il->instance_buf, &(sg_range){
        .ptr = il->instances,
        .size = ALPHA_LINES_COUNT * sizeof(alpha_line_instance_t)
    });
}

static inline void instanced_lines_alpha_draw(instanced_lines_alpha_t* il, mat4_t mvp, float aspect_ratio) {
    alpha_line_params_t params = {
        .mvp = mvp,
        .line_width = il->line_width,
        .aspect_ratio = aspect_ratio,
    };

    sg_apply_pipeline(il->pip);
    sg_apply_bindings(&(sg_bindings){
        .vertex_buffers = {
            [0] = il->template_vbuf,
            [1] = il->instance_buf,
        },
        .index_buffer = il->template_ibuf,
    });
    sg_apply_uniforms(0, &SG_RANGE(params));
    sg_draw(0, il->template_index_count, ALPHA_LINES_COUNT);
}

static inline void instanced_lines_alpha_shutdown(instanced_lines_alpha_t* il) {
    sg_destroy_pipeline(il->pip);
    sg_destroy_shader(il->shd);
    sg_destroy_buffer(il->template_vbuf);
    sg_destroy_buffer(il->template_ibuf);
    sg_destroy_buffer(il->instance_buf);
}

#endif // INSTANCED_LINES_ALPHA_H
