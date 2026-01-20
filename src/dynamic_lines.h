//------------------------------------------------------------------------------
// dynamic_lines.h - Dynamic line rendering system (header-only)
//
// Renders many animated lines within a spherical volume.
// Uses Sokol stream buffers for efficient per-frame updates.
//------------------------------------------------------------------------------
#ifndef DYNAMIC_LINES_H
#define DYNAMIC_LINES_H

#include "platform.h"
#include "sokol_gfx.h"
#include "math3d.h"
#include "shaders/line_shaders.h"
#include <math.h>

//------------------------------------------------------------------------------
// Configuration
//------------------------------------------------------------------------------
#define LINES_COUNT 10000
#define LINES_DEFAULT_SPHERE_RADIUS 5.0f
#define LINES_DEFAULT_WIGGLE_AMPLITUDE 0.3f
#define LINES_DEFAULT_WIGGLE_FREQUENCY 2.0f
#define LINES_DEFAULT_LINE_LENGTH 0.15f

//------------------------------------------------------------------------------
// Types
//------------------------------------------------------------------------------

// Vertex data sent to GPU (position + color interleaved)
typedef struct {
    float px, py, pz;      // position
    float r, g, b, a;      // color
} line_vertex_t;

// Per-line CPU data (persistent, used to compute vertex positions each frame)
typedef struct {
    vec3_t base_pos;       // Center point of the line
    vec3_t direction;      // Line orientation (normalized)
    vec3_t motion_dir;     // Direction of sin-wave wiggle (normalized)
    float phase;           // Phase offset for sin wave [0, 2π]
    float r, g, b;         // RGB color
} line_data_t;

// Main dynamic lines state
typedef struct {
    // GPU resources
    sg_pipeline pip;
    sg_buffer vbuf;
    sg_shader shd;

    // CPU data arrays
    line_data_t lines[LINES_COUNT];
    line_vertex_t vertices[LINES_COUNT * 2];

    // Configurable parameters
    float sphere_radius;
    float wiggle_amplitude;
    float wiggle_frequency;
    float line_length;

    // RNG state (for reproducible regeneration)
    uint32_t rng_state;
} dynamic_lines_t;

//------------------------------------------------------------------------------
// Random number generation (simple LCG)
//------------------------------------------------------------------------------
static inline float lines_randf(dynamic_lines_t* dl) {
    dl->rng_state = dl->rng_state * 1103515245u + 12345u;
    return (float)(dl->rng_state & 0x7FFFFFFFu) / (float)0x7FFFFFFFu;
}

static inline vec3_t lines_random_point_in_sphere(dynamic_lines_t* dl, float radius) {
    vec3_t p;
    // Rejection sampling for uniform distribution in sphere
    do {
        p.x = lines_randf(dl) * 2.0f - 1.0f;
        p.y = lines_randf(dl) * 2.0f - 1.0f;
        p.z = lines_randf(dl) * 2.0f - 1.0f;
    } while (p.x * p.x + p.y * p.y + p.z * p.z > 1.0f);
    p.x *= radius;
    p.y *= radius;
    p.z *= radius;
    return p;
}

static inline vec3_t lines_random_direction(dynamic_lines_t* dl) {
    vec3_t d = lines_random_point_in_sphere(dl, 1.0f);
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
// Regeneration functions
//------------------------------------------------------------------------------
static inline void dynamic_lines_regenerate_positions(dynamic_lines_t* dl) {
    dl->rng_state = 12345u;  // Reset for reproducible positions
    for (int i = 0; i < LINES_COUNT; i++) {
        dl->lines[i].base_pos = lines_random_point_in_sphere(dl, dl->sphere_radius);
    }
}

static inline void dynamic_lines_regenerate_directions(dynamic_lines_t* dl) {
    dl->rng_state = 67890u;  // Different seed for directions
    for (int i = 0; i < LINES_COUNT; i++) {
        dl->lines[i].direction = lines_random_direction(dl);
        dl->lines[i].motion_dir = lines_random_direction(dl);
        dl->lines[i].phase = lines_randf(dl) * 6.28318530718f;  // 2π
    }
}

static inline void dynamic_lines_regenerate_colors(dynamic_lines_t* dl) {
    dl->rng_state = 13579u;  // Different seed for colors
    for (int i = 0; i < LINES_COUNT; i++) {
        // Generate vibrant colors by using HSV-like approach
        float hue = lines_randf(dl);
        float sat = 0.7f + lines_randf(dl) * 0.3f;
        float val = 0.8f + lines_randf(dl) * 0.2f;

        // Simple HSV to RGB conversion
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

        dl->lines[i].r = r;
        dl->lines[i].g = g;
        dl->lines[i].b = b;
    }
}

static inline void dynamic_lines_regenerate_all(dynamic_lines_t* dl) {
    dynamic_lines_regenerate_positions(dl);
    dynamic_lines_regenerate_directions(dl);
    dynamic_lines_regenerate_colors(dl);
}

//------------------------------------------------------------------------------
// Core functions
//------------------------------------------------------------------------------
static inline void dynamic_lines_init(dynamic_lines_t* dl) {
    // Initialize parameters to defaults
    dl->sphere_radius = LINES_DEFAULT_SPHERE_RADIUS;
    dl->wiggle_amplitude = LINES_DEFAULT_WIGGLE_AMPLITUDE;
    dl->wiggle_frequency = LINES_DEFAULT_WIGGLE_FREQUENCY;
    dl->line_length = LINES_DEFAULT_LINE_LENGTH;
    dl->rng_state = 12345u;

    // Generate initial line data
    dynamic_lines_regenerate_all(dl);

    // Create vertex buffer with stream_update for frequent updates
    dl->vbuf = sg_make_buffer(&(sg_buffer_desc){
        .usage.vertex_buffer = true,
        .usage.stream_update = true,
        .size = LINES_COUNT * 2 * sizeof(line_vertex_t),
        .label = "lines-vertices"
    });

    // Create shader
    dl->shd = sg_make_shader(&(sg_shader_desc){
        .vertex_func = {
            .source = line_vs_source,
            .entry = "vs_main",
        },
        .fragment_func = {
            .source = line_fs_source,
            .entry = "fs_main",
        },
        .uniform_blocks[0] = {
            .stage = SG_SHADERSTAGE_VERTEX,
            .size = sizeof(mat4_t),
            .layout = SG_UNIFORMLAYOUT_STD140,
            .glsl_uniforms[0] = { .type = SG_UNIFORMTYPE_MAT4, .glsl_name = "mvp" }
        },
        .label = "lines-shader"
    });

    // Create pipeline for line rendering
    dl->pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = dl->shd,
        .layout = {
            .attrs = {
                [0] = { .format = SG_VERTEXFORMAT_FLOAT3 },  // position
                [1] = { .format = SG_VERTEXFORMAT_FLOAT4 }   // color (RGBA)
            }
        },
        .primitive_type = SG_PRIMITIVETYPE_LINES,
        .depth = {
            .compare = SG_COMPAREFUNC_LESS_EQUAL,
            .write_enabled = true,
            .pixel_format = SG_PIXELFORMAT_DEPTH
        },
        .colors[0].pixel_format = SG_PIXELFORMAT_RGBA8,
        .label = "lines-pipeline"
    });
}

static inline void dynamic_lines_update(dynamic_lines_t* dl, float time) {
    // Compute vertex positions for all lines based on current time
    for (int i = 0; i < LINES_COUNT; i++) {
        line_data_t* line = &dl->lines[i];

        // Calculate sin-wave wiggle offset
        float wave = sinf(time * dl->wiggle_frequency + line->phase);
        float offset_x = line->motion_dir.x * wave * dl->wiggle_amplitude;
        float offset_y = line->motion_dir.y * wave * dl->wiggle_amplitude;
        float offset_z = line->motion_dir.z * wave * dl->wiggle_amplitude;

        // Compute center position with wiggle
        float cx = line->base_pos.x + offset_x;
        float cy = line->base_pos.y + offset_y;
        float cz = line->base_pos.z + offset_z;

        // Compute line endpoints
        float dx = line->direction.x * dl->line_length;
        float dy = line->direction.y * dl->line_length;
        float dz = line->direction.z * dl->line_length;

        // Endpoint 0
        line_vertex_t* v0 = &dl->vertices[i * 2];
        v0->px = cx - dx;
        v0->py = cy - dy;
        v0->pz = cz - dz;
        v0->r = line->r;
        v0->g = line->g;
        v0->b = line->b;
        v0->a = 1.0f;

        // Endpoint 1
        line_vertex_t* v1 = &dl->vertices[i * 2 + 1];
        v1->px = cx + dx;
        v1->py = cy + dy;
        v1->pz = cz + dz;
        v1->r = line->r;
        v1->g = line->g;
        v1->b = line->b;
        v1->a = 1.0f;
    }

    // Upload vertex data to GPU
    sg_update_buffer(dl->vbuf, &(sg_range){
        .ptr = dl->vertices,
        .size = LINES_COUNT * 2 * sizeof(line_vertex_t)
    });
}

static inline void dynamic_lines_draw(dynamic_lines_t* dl, mat4_t mvp) {
    sg_apply_pipeline(dl->pip);
    sg_apply_bindings(&(sg_bindings){
        .vertex_buffers[0] = dl->vbuf
    });
    sg_apply_uniforms(0, &SG_RANGE(mvp));
    sg_draw(0, LINES_COUNT * 2, 1);
}

static inline void dynamic_lines_shutdown(dynamic_lines_t* dl) {
    sg_destroy_pipeline(dl->pip);
    sg_destroy_shader(dl->shd);
    sg_destroy_buffer(dl->vbuf);
}

#endif // DYNAMIC_LINES_H
