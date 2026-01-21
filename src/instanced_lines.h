//------------------------------------------------------------------------------
// instanced_lines.h - Instanced thick line rendering (header-only)
//
// Renders animated thick lines with round caps using GPU instancing.
// Template geometry (rectangle + semicircle caps) is instanced per line segment.
//------------------------------------------------------------------------------
#ifndef INSTANCED_LINES_H
#define INSTANCED_LINES_H

#include "platform.h"
#include "sokol_gfx.h"
#include "math3d.h"
#include "shaders/instanced_line_shaders.h"
#include <math.h>
#include <stdlib.h>

//------------------------------------------------------------------------------
// Configuration
//------------------------------------------------------------------------------
#define INSTANCED_LINES_COUNT 2000
#define INSTANCED_LINES_CAP_SEGMENTS 8
#define INSTANCED_LINES_DEFAULT_WIDTH 0.05f
#define INSTANCED_LINES_DEFAULT_SPHERE_RADIUS 5.0f
#define INSTANCED_LINES_DEFAULT_WIGGLE_AMPLITUDE 0.3f
#define INSTANCED_LINES_DEFAULT_WIGGLE_FREQUENCY 2.0f
#define INSTANCED_LINES_DEFAULT_LINE_LENGTH 0.3f

//------------------------------------------------------------------------------
// Types
//------------------------------------------------------------------------------

// Template vertex: position in template space
// x = along line direction (-0.5 to 0.5 for body, extended for caps)
// y = perpendicular (-0.5 to 0.5)
// z = endpoint selector (0.0 = point A, 1.0 = point B)
typedef struct {
    float x, y, z;
} template_vertex_t;

// Instance data for a single line segment
typedef struct {
    float ax, ay, az;  // Point A position
    float bx, by, bz;  // Point B position
    float r, g, b, a;  // Color
} line_instance_t;

// Per-line CPU data for animation
typedef struct {
    vec3_t base_pos;
    vec3_t direction;
    vec3_t motion_dir;
    float phase;
    float r, g, b;
} instanced_line_data_t;

// Uniform block for shader
typedef struct {
    mat4_t mvp;
    float line_width;
    float aspect_ratio;
    float _pad[2];  // Align to 16 bytes
} instanced_line_params_t;

// Main state
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
    instanced_line_data_t lines[INSTANCED_LINES_COUNT];
    line_instance_t instances[INSTANCED_LINES_COUNT];

    // Parameters
    float line_width;
    float sphere_radius;
    float wiggle_amplitude;
    float wiggle_frequency;
    float line_length;

    // RNG
    uint32_t rng_state;
    uint32_t seed_counter;
} instanced_lines_t;

//------------------------------------------------------------------------------
// Random number generation
//------------------------------------------------------------------------------
static inline float instanced_lines_randf(instanced_lines_t* il) {
    il->rng_state = il->rng_state * 1103515245u + 12345u;
    return (float)(il->rng_state & 0x7FFFFFFFu) / (float)0x7FFFFFFFu;
}

static inline vec3_t instanced_lines_random_point_in_sphere(instanced_lines_t* il, float radius) {
    vec3_t p;
    do {
        p.x = instanced_lines_randf(il) * 2.0f - 1.0f;
        p.y = instanced_lines_randf(il) * 2.0f - 1.0f;
        p.z = instanced_lines_randf(il) * 2.0f - 1.0f;
    } while (p.x * p.x + p.y * p.y + p.z * p.z > 1.0f);
    p.x *= radius;
    p.y *= radius;
    p.z *= radius;
    return p;
}

static inline vec3_t instanced_lines_random_direction(instanced_lines_t* il) {
    vec3_t d = instanced_lines_random_point_in_sphere(il, 1.0f);
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
static inline void instanced_lines_regenerate_positions(instanced_lines_t* il) {
    il->rng_state = 12345u + il->seed_counter++ * 111u;
    for (int i = 0; i < INSTANCED_LINES_COUNT; i++) {
        il->lines[i].base_pos = instanced_lines_random_point_in_sphere(il, il->sphere_radius);
    }
}

static inline void instanced_lines_regenerate_directions(instanced_lines_t* il) {
    il->rng_state = 67890u + il->seed_counter++ * 222u;
    for (int i = 0; i < INSTANCED_LINES_COUNT; i++) {
        il->lines[i].direction = instanced_lines_random_direction(il);
        il->lines[i].motion_dir = instanced_lines_random_direction(il);
        il->lines[i].phase = instanced_lines_randf(il) * 6.28318530718f;
    }
}

static inline void instanced_lines_regenerate_colors(instanced_lines_t* il) {
    il->rng_state = 13579u + il->seed_counter++ * 333u;
    for (int i = 0; i < INSTANCED_LINES_COUNT; i++) {
        float hue = instanced_lines_randf(il);
        float sat = 0.7f + instanced_lines_randf(il) * 0.3f;
        float val = 0.8f + instanced_lines_randf(il) * 0.2f;

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
    }
}

static inline void instanced_lines_regenerate_all(instanced_lines_t* il) {
    instanced_lines_regenerate_positions(il);
    instanced_lines_regenerate_directions(il);
    instanced_lines_regenerate_colors(il);
}

//------------------------------------------------------------------------------
// Template geometry generation
//------------------------------------------------------------------------------
static inline void instanced_lines_generate_template(
    template_vertex_t* vertices, int* vertex_count,
    uint16_t* indices, int* index_count,
    int cap_segments
) {
    int vi = 0;
    int ii = 0;

    // Main rectangle body (4 vertices, 2 triangles)
    // Bottom-left (A side)
    vertices[vi++] = (template_vertex_t){ 0.0f, -0.5f, 0.0f };
    // Top-left (A side)
    vertices[vi++] = (template_vertex_t){ 0.0f,  0.5f, 0.0f };
    // Bottom-right (B side)
    vertices[vi++] = (template_vertex_t){ 0.0f, -0.5f, 1.0f };
    // Top-right (B side)
    vertices[vi++] = (template_vertex_t){ 0.0f,  0.5f, 1.0f };

    // Rectangle triangles
    indices[ii++] = 0; indices[ii++] = 2; indices[ii++] = 1;
    indices[ii++] = 1; indices[ii++] = 2; indices[ii++] = 3;

    // Left semicircle cap (at point A, z=0)
    int cap_a_center = vi;
    vertices[vi++] = (template_vertex_t){ 0.0f, 0.0f, 0.0f };

    for (int i = 0; i <= cap_segments; i++) {
        float angle = 3.14159265359f * 0.5f + 3.14159265359f * (float)i / (float)cap_segments;
        float x = cosf(angle) * 0.5f;
        float y = sinf(angle) * 0.5f;
        vertices[vi++] = (template_vertex_t){ x, y, 0.0f };
    }

    for (int i = 0; i < cap_segments; i++) {
        indices[ii++] = cap_a_center;
        indices[ii++] = cap_a_center + 1 + i;
        indices[ii++] = cap_a_center + 2 + i;
    }

    // Right semicircle cap (at point B, z=1)
    int cap_b_center = vi;
    vertices[vi++] = (template_vertex_t){ 0.0f, 0.0f, 1.0f };

    for (int i = 0; i <= cap_segments; i++) {
        float angle = -3.14159265359f * 0.5f + 3.14159265359f * (float)i / (float)cap_segments;
        float x = cosf(angle) * 0.5f;
        float y = sinf(angle) * 0.5f;
        vertices[vi++] = (template_vertex_t){ x, y, 1.0f };
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
static inline void instanced_lines_init(instanced_lines_t* il) {
    // Initialize parameters
    il->line_width = INSTANCED_LINES_DEFAULT_WIDTH;
    il->sphere_radius = INSTANCED_LINES_DEFAULT_SPHERE_RADIUS;
    il->wiggle_amplitude = INSTANCED_LINES_DEFAULT_WIGGLE_AMPLITUDE;
    il->wiggle_frequency = INSTANCED_LINES_DEFAULT_WIGGLE_FREQUENCY;
    il->line_length = INSTANCED_LINES_DEFAULT_LINE_LENGTH;
    il->rng_state = 12345u;
    il->seed_counter = 0u;

    // Generate line data
    instanced_lines_regenerate_all(il);

    // Generate template geometry
    // Max vertices: 4 (rect) + 2*(1 + cap_segments+1) = 4 + 2*(cap_segments+2)
    int max_vertices = 4 + 2 * (INSTANCED_LINES_CAP_SEGMENTS + 2);
    // Max indices: 6 (rect) + 2 * cap_segments * 3
    int max_indices = 6 + 2 * INSTANCED_LINES_CAP_SEGMENTS * 3;

    template_vertex_t* template_vertices = (template_vertex_t*)malloc(max_vertices * sizeof(template_vertex_t));
    uint16_t* template_indices = (uint16_t*)malloc(max_indices * sizeof(uint16_t));

    instanced_lines_generate_template(
        template_vertices, &il->template_vertex_count,
        template_indices, &il->template_index_count,
        INSTANCED_LINES_CAP_SEGMENTS
    );

    // Create template vertex buffer
    il->template_vbuf = sg_make_buffer(&(sg_buffer_desc){
        .usage.vertex_buffer = true,
        .data = {
            .ptr = template_vertices,
            .size = il->template_vertex_count * sizeof(template_vertex_t)
        },
        .label = "instanced-lines-template-vbuf"
    });

    // Create template index buffer
    il->template_ibuf = sg_make_buffer(&(sg_buffer_desc){
        .usage.index_buffer = true,
        .data = {
            .ptr = template_indices,
            .size = il->template_index_count * sizeof(uint16_t)
        },
        .label = "instanced-lines-template-ibuf"
    });

    free(template_vertices);
    free(template_indices);

    // Create instance buffer (stream for per-frame updates)
    il->instance_buf = sg_make_buffer(&(sg_buffer_desc){
        .usage.vertex_buffer = true,
        .usage.stream_update = true,
        .size = INSTANCED_LINES_COUNT * sizeof(line_instance_t),
        .label = "instanced-lines-instance-buf"
    });

    // Create shader
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
            .size = sizeof(instanced_line_params_t),
            .layout = SG_UNIFORMLAYOUT_STD140,
            .glsl_uniforms = {
                [0] = { .type = SG_UNIFORMTYPE_MAT4, .glsl_name = "mvp" },
                [1] = { .type = SG_UNIFORMTYPE_FLOAT, .glsl_name = "line_width" },
                [2] = { .type = SG_UNIFORMTYPE_FLOAT, .glsl_name = "aspect_ratio" },
            }
        },
        .label = "instanced-lines-shader"
    });

    // Create pipeline with instancing
    il->pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = il->shd,
        .layout = {
            .buffers = {
                [0] = { .step_func = SG_VERTEXSTEP_PER_VERTEX },
                [1] = { .step_func = SG_VERTEXSTEP_PER_INSTANCE },
            },
            .attrs = {
                [0] = { .buffer_index = 0, .format = SG_VERTEXFORMAT_FLOAT3 },  // template_pos
                [1] = { .buffer_index = 1, .format = SG_VERTEXFORMAT_FLOAT3, .offset = 0 },   // point_a
                [2] = { .buffer_index = 1, .format = SG_VERTEXFORMAT_FLOAT3, .offset = 12 },  // point_b
                [3] = { .buffer_index = 1, .format = SG_VERTEXFORMAT_FLOAT4, .offset = 24 },  // color
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
        .cull_mode = SG_CULLMODE_NONE,  // Both sides visible for lines
        .label = "instanced-lines-pipeline"
    });
}

static inline void instanced_lines_update(instanced_lines_t* il, float time) {
    for (int i = 0; i < INSTANCED_LINES_COUNT; i++) {
        instanced_line_data_t* line = &il->lines[i];

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

        line_instance_t* inst = &il->instances[i];
        inst->ax = cx - dx;
        inst->ay = cy - dy;
        inst->az = cz - dz;
        inst->bx = cx + dx;
        inst->by = cy + dy;
        inst->bz = cz + dz;
        inst->r = line->r;
        inst->g = line->g;
        inst->b = line->b;
        inst->a = 1.0f;
    }

    sg_update_buffer(il->instance_buf, &(sg_range){
        .ptr = il->instances,
        .size = INSTANCED_LINES_COUNT * sizeof(line_instance_t)
    });
}

static inline void instanced_lines_draw(instanced_lines_t* il, mat4_t mvp, float aspect_ratio) {
    instanced_line_params_t params = {
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
    sg_draw(0, il->template_index_count, INSTANCED_LINES_COUNT);
}

static inline void instanced_lines_shutdown(instanced_lines_t* il) {
    sg_destroy_pipeline(il->pip);
    sg_destroy_shader(il->shd);
    sg_destroy_buffer(il->template_vbuf);
    sg_destroy_buffer(il->template_ibuf);
    sg_destroy_buffer(il->instance_buf);
}

#endif // INSTANCED_LINES_H
