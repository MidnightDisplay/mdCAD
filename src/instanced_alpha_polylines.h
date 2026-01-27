//------------------------------------------------------------------------------
// gcode_polyline.h - G-code path renderer with timeline effect (header-only)
//
// Renders G-code toolpath as thick polylines with alpha blending.
// Features timeline scrubbing: highlight a percentage along the path
// with gradual alpha fade to base value.
//------------------------------------------------------------------------------
#ifndef INSTANCED_ALPHA_POLYLINES_H
#define INSTANCED_ALPHA_POLYLINES_H

#include "platform.h"
#include "sokol_gfx.h"
#include "math3d.h"
#include "gcode_loader.h"
#include "shaders/instanced_line_shaders.h"
#include "shaders/alpha_polyline_shaders.h"
#include <math.h>
#include <stdlib.h>

#ifdef PLATFORM_ANDROID
#include <android/log.h>
#define ALPHA_POLY_LOG(...) __android_log_print(ANDROID_LOG_INFO, "alpha_polylines", __VA_ARGS__)
#else
#define ALPHA_POLY_LOG(...) ((void)0)
#endif

// Embedded G-code data for platforms without filesystem access
#if defined(PLATFORM_WEB) || defined(PLATFORM_IOS) || defined(PLATFORM_ANDROID)
#include "gcode_benchy_embedded.h"
#endif

//------------------------------------------------------------------------------
// Configuration
//------------------------------------------------------------------------------
#define INSTANCED_ALPHA_CAP_SEGMENTS 8
#define INSTANCED_ALPHA_JOIN_SEGMENTS 12
#define INSTANCED_ALPHA_DEFAULT_WIDTH 0.03f
#define INSTANCED_ALPHA_DEFAULT_SCALE 0.05f
#define INSTANCED_ALPHA_DEFAULT_BASE_ALPHA 0.15f
#define INSTANCED_ALPHA_DEFAULT_HIGHLIGHT_WIDTH 0.02f

//------------------------------------------------------------------------------
// Types
//------------------------------------------------------------------------------

typedef struct {
    float x, y, z;
} gcode_template_vertex_t;

// Old segment instance (2 points) - kept for caps
typedef struct {
    float ax, ay, az;
    float bx, by, bz;
    float r, g, b, a;
} gcode_segment_instance_t;

// Intermediate segment instance (4 points: pA, pB, pC, pD)
typedef struct {
    float pAx, pAy, pAz;
    float pBx, pBy, pBz;
    float pCx, pCy, pCz;
    float pDx, pDy, pDz;
    float r, g, b, a;
} gcode_intermediate_segment_t;

// Terminal segment instance (3 points: pA, pB, pC)
typedef struct {
    float pAx, pAy, pAz;
    float pBx, pBy, pBz;
    float pCx, pCy, pCz;
    float r, g, b, a;
} gcode_terminal_segment_t;

// Pie-slice join instance (3 points: pA, pB, pC)
typedef struct {
    float pAx, pAy, pAz;  // previous point
    float pBx, pBy, pBz;  // join center
    float pCx, pCy, pCz;  // next point
    float r, g, b, a;
} gcode_pie_join_instance_t;

// Old join instance (1 point) - kept for reference
typedef struct {
    float px, py, pz;
    float r, g, b, a;
} gcode_join_instance_t;

typedef struct {
    mat4_t mvp;
    float line_width;
    float aspect_ratio;
    float _pad[2];
} instanced_alpha_params_t;

typedef struct {
    mat4_t mvp;
    float line_width;
    float aspect_ratio;
    float join_resolution;
    float miter_angle_limit;  // Angle threshold in radians (above this, use semicircle)
} gcode_pie_join_params_t;

typedef struct {
    // GPU resources for intermediate segments (miter-adjusted at both ends)
    sg_pipeline intermediate_pip;
    sg_buffer intermediate_template_vbuf;
    sg_buffer intermediate_template_ibuf;
    sg_buffer intermediate_instance_buf;
    sg_shader intermediate_shd;
    int intermediate_template_vertex_count;
    int intermediate_template_index_count;

    // GPU resources for terminal segments (miter-adjusted at one end)
    sg_pipeline terminal_pip;
    sg_buffer terminal_template_vbuf;
    sg_buffer terminal_template_ibuf;
    sg_buffer terminal_start_instance_buf;  // First segment
    sg_buffer terminal_end_instance_buf;    // Last segment
    sg_shader terminal_shd;
    int terminal_template_vertex_count;
    int terminal_template_index_count;

    // GPU resources for pie-slice joins
    sg_pipeline pie_join_pip;
    sg_buffer pie_join_template_vbuf;
    sg_buffer pie_join_template_ibuf;
    sg_buffer pie_join_instance_buf;
    sg_shader pie_join_shd;
    int pie_join_template_vertex_count;
    int pie_join_template_index_count;

    // GPU resources for caps (semicircles at start and end)
    sg_pipeline cap_pip;
    sg_buffer cap_template_vbuf;
    sg_buffer cap_template_ibuf;
    sg_buffer cap_instance_buf;
    sg_shader cap_shd;
    int cap_template_vertex_count;
    int cap_template_index_count;

    // Path data
    gcode_path_t path;

    // Instance data (dynamically allocated)
    gcode_intermediate_segment_t* intermediate_segments;
    gcode_terminal_segment_t* terminal_start;  // First segment (single)
    gcode_terminal_segment_t* terminal_end;    // Last segment (single)
    gcode_pie_join_instance_t* pie_joins;
    gcode_segment_instance_t* caps;
    int intermediate_count;
    int pie_join_count;
    int cap_count;
    int max_intermediate;
    int max_pie_joins;

    // Parameters
    float line_width;
    float scale;
    float color_r, color_g, color_b;
    float timeline_position;   // 0.0 - 1.0
    float base_alpha;          // Base transparency (0.0 - 1.0)
    float highlight_width;     // Width of highlight fade (0.0 - 0.5)
    float miter_angle_limit;   // Angle threshold in degrees (above this, use semicircle join)
    bool debug_colors;         // Use different colors for each component
} instanced_alpha_polylines_t;


//------------------------------------------------------------------------------
// Template geometry generation
//------------------------------------------------------------------------------

// Segment template: rectangle body ONLY (no caps) to prevent overlap at joins
static inline void gcode_generate_segment_template(
    gcode_template_vertex_t* vertices, int* vertex_count,
    uint16_t* indices, int* index_count
) {
    int vi = 0;
    int ii = 0;

    // Main rectangle body only - no caps!
    // This prevents alpha overlap at joins where segments meet
    vertices[vi++] = (gcode_template_vertex_t){ 0.0f, -0.5f, 0.0f };
    vertices[vi++] = (gcode_template_vertex_t){ 0.0f,  0.5f, 0.0f };
    vertices[vi++] = (gcode_template_vertex_t){ 0.0f, -0.5f, 1.0f };
    vertices[vi++] = (gcode_template_vertex_t){ 0.0f,  0.5f, 1.0f };

    indices[ii++] = 0; indices[ii++] = 2; indices[ii++] = 1;
    indices[ii++] = 1; indices[ii++] = 2; indices[ii++] = 3;

    *vertex_count = vi;
    *index_count = ii;
}

// Cap template: semicircle at z=0 (point A side) for terminal vertices
// Used to cap the start and end of the polyline
static inline void gcode_generate_cap_template(
    gcode_template_vertex_t* vertices, int* vertex_count,
    uint16_t* indices, int* index_count,
    int cap_segments
) {
    int vi = 0;
    int ii = 0;

    // Semicircle at z=0 (extends in negative direction from point A)
    // Center vertex
    vertices[vi++] = (gcode_template_vertex_t){ 0.0f, 0.0f, 0.0f };

    // Semicircle edge vertices (from 90° to 270°, i.e., the left half)
    // Radius 0.5: after shader multiplies by width, cap radius = 0.5*width = halfWidth
    // This matches terminal segment edges which are at +-halfWidth from center
    for (int i = 0; i <= cap_segments; i++) {
        float angle = 3.14159265359f * 0.5f + 3.14159265359f * (float)i / (float)cap_segments;
        float x = cosf(angle) * 0.5f;
        float y = sinf(angle) * 0.5f;
        vertices[vi++] = (gcode_template_vertex_t){ x, y, 0.0f };
    }

    // Triangle fan
    for (int i = 0; i < cap_segments; i++) {
        indices[ii++] = 0;
        indices[ii++] = 1 + i;
        indices[ii++] = 2 + i;
    }

    *vertex_count = vi;
    *index_count = ii;
}

// Pie-slice join template: triangle fan for angular fill
// The template_pos.x stores the vertex ID (0=center, 1..n=arc vertices)
// The shader computes the actual angle based on the neighboring segments
static inline void gcode_generate_pie_join_template(
    gcode_template_vertex_t* vertices, int* vertex_count,
    uint16_t* indices, int* index_count,
    int resolution
) {
    int vi = 0;
    int ii = 0;

    // Vertex 0: center (id=0)
    vertices[vi++] = (gcode_template_vertex_t){ 0.0f, 0.0f, 0.0f };

    // Vertices 1..resolution+1: arc points (id=1..resolution+1)
    for (int i = 0; i <= resolution; i++) {
        vertices[vi++] = (gcode_template_vertex_t){ (float)(i + 1), 0.0f, 0.0f };
    }

    // Triangle fan from center
    for (int i = 0; i < resolution; i++) {
        indices[ii++] = 0;           // center
        indices[ii++] = 1 + i;       // current arc vertex
        indices[ii++] = 2 + i;       // next arc vertex
    }

    *vertex_count = vi;
    *index_count = ii;
}

// Legacy full circle join template (kept for reference)
static inline void gcode_generate_join_template(
    gcode_template_vertex_t* vertices, int* vertex_count,
    uint16_t* indices, int* index_count,
    int segments
) {
    int vi = 0;
    int ii = 0;

    // Center vertex
    vertices[vi++] = (gcode_template_vertex_t){ 0.0f, 0.0f, 0.0f };

    // Circle edge vertices
    for (int i = 0; i <= segments; i++) {
        float angle = 2.0f * 3.14159265359f * (float)i / (float)segments;
        float x = cosf(angle) * 0.5f;
        float y = sinf(angle) * 0.5f;
        vertices[vi++] = (gcode_template_vertex_t){ x, y, 0.0f };
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
// Smoothstep helper
//------------------------------------------------------------------------------
static inline float gcode_smoothstep(float edge0, float edge1, float x) {
    float t = (x - edge0) / (edge1 - edge0);
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    return t * t * (3.0f - 2.0f * t);
}

//------------------------------------------------------------------------------
// Core functions
//------------------------------------------------------------------------------

static inline bool instanced_alpha_polylines_init(instanced_alpha_polylines_t* gp, const char* gcode_filename) {
    // Initialize path
    gcode_path_init(&gp->path);

    // Load G-code - use embedded data on platforms without filesystem, file on desktop
#if defined(PLATFORM_WEB) || defined(PLATFORM_IOS) || defined(PLATFORM_ANDROID)
    (void)gcode_filename;  // Unused on these platforms
    ALPHA_POLY_LOG("Loading embedded G-code, size=%u", gcode_benchy_size);
    if (!gcode_path_load_from_memory(&gp->path, gcode_benchy_data, gcode_benchy_size)) {
        ALPHA_POLY_LOG("Failed to load G-code from memory!");
        return false;
    }
#else
    if (!gcode_path_load(&gp->path, gcode_filename)) {
        return false;
    }
#endif

    ALPHA_POLY_LOG("G-code loaded: %d points, total_length=%.2f", gp->path.count, gp->path.total_length);

    // Center the path
    gcode_path_center(&gp->path);

    // Initialize parameters
    gp->line_width = INSTANCED_ALPHA_DEFAULT_WIDTH;
    gp->scale = INSTANCED_ALPHA_DEFAULT_SCALE;

    // Default color: Catppuccin Frappé lavender (#babbf1)
    gp->color_r = 0.729f;
    gp->color_g = 0.733f;
    gp->color_b = 0.945f;

    gp->timeline_position = 0.5f;
    gp->base_alpha = INSTANCED_ALPHA_DEFAULT_BASE_ALPHA;
    gp->highlight_width = INSTANCED_ALPHA_DEFAULT_HIGHLIGHT_WIDTH;
    gp->miter_angle_limit = 150.0f;  // Default: use semicircle for angles > 150 degrees
    gp->debug_colors = true;

    // Calculate counts
    // For a polyline with n points: n-1 segments, n-2 joins
    // Segments: first (terminal), intermediate (n-3), last (terminal)
    // If n < 4, there are no intermediate segments
    int n = gp->path.count;
    gp->max_intermediate = (n >= 4) ? (n - 3) : 0;  // Segments 1 to n-3 (indices)
    gp->max_pie_joins = (n >= 3) ? (n - 2) : 0;     // Joins at vertices 1 to n-2

    // Allocate instance arrays
    gp->intermediate_segments = NULL;
    if (gp->max_intermediate > 0) {
        gp->intermediate_segments = (gcode_intermediate_segment_t*)malloc(
            gp->max_intermediate * sizeof(gcode_intermediate_segment_t));
    }

    gp->terminal_start = (gcode_terminal_segment_t*)malloc(sizeof(gcode_terminal_segment_t));
    gp->terminal_end = (gcode_terminal_segment_t*)malloc(sizeof(gcode_terminal_segment_t));

    gp->pie_joins = NULL;
    if (gp->max_pie_joins > 0) {
        gp->pie_joins = (gcode_pie_join_instance_t*)malloc(
            gp->max_pie_joins * sizeof(gcode_pie_join_instance_t));
    }

    gp->caps = (gcode_segment_instance_t*)malloc(2 * sizeof(gcode_segment_instance_t));

    gp->intermediate_count = 0;
    gp->pie_join_count = 0;
    gp->cap_count = 0;

    // Generate segment template (rectangle only)
    int max_seg_vertices = 4;
    int max_seg_indices = 6;
    gcode_template_vertex_t* seg_verts = (gcode_template_vertex_t*)malloc(max_seg_vertices * sizeof(gcode_template_vertex_t));
    uint16_t* seg_indices = (uint16_t*)malloc(max_seg_indices * sizeof(uint16_t));

    gcode_generate_segment_template(seg_verts, &gp->intermediate_template_vertex_count,
                                     seg_indices, &gp->intermediate_template_index_count);

    gp->intermediate_template_vbuf = sg_make_buffer(&(sg_buffer_desc){
        .usage.vertex_buffer = true,
        .data = { .ptr = seg_verts, .size = gp->intermediate_template_vertex_count * sizeof(gcode_template_vertex_t) },
        .label = "gcode-intermediate-template-vbuf"
    });
    gp->intermediate_template_ibuf = sg_make_buffer(&(sg_buffer_desc){
        .usage.index_buffer = true,
        .data = { .ptr = seg_indices, .size = gp->intermediate_template_index_count * sizeof(uint16_t) },
        .label = "gcode-intermediate-template-ibuf"
    });

    // Terminal segments use same template
    gp->terminal_template_vertex_count = gp->intermediate_template_vertex_count;
    gp->terminal_template_index_count = gp->intermediate_template_index_count;
    gp->terminal_template_vbuf = sg_make_buffer(&(sg_buffer_desc){
        .usage.vertex_buffer = true,
        .data = { .ptr = seg_verts, .size = gp->terminal_template_vertex_count * sizeof(gcode_template_vertex_t) },
        .label = "gcode-terminal-template-vbuf"
    });
    gp->terminal_template_ibuf = sg_make_buffer(&(sg_buffer_desc){
        .usage.index_buffer = true,
        .data = { .ptr = seg_indices, .size = gp->terminal_template_index_count * sizeof(uint16_t) },
        .label = "gcode-terminal-template-ibuf"
    });

    free(seg_verts);
    free(seg_indices);

    // Generate pie-slice join template
    int pie_resolution = INSTANCED_ALPHA_JOIN_SEGMENTS;
    int max_pie_vertices = pie_resolution + 2;
    int max_pie_indices = pie_resolution * 3;
    gcode_template_vertex_t* pie_verts = (gcode_template_vertex_t*)malloc(max_pie_vertices * sizeof(gcode_template_vertex_t));
    uint16_t* pie_indices = (uint16_t*)malloc(max_pie_indices * sizeof(uint16_t));

    gcode_generate_pie_join_template(pie_verts, &gp->pie_join_template_vertex_count,
                                      pie_indices, &gp->pie_join_template_index_count,
                                      pie_resolution);

    gp->pie_join_template_vbuf = sg_make_buffer(&(sg_buffer_desc){
        .usage.vertex_buffer = true,
        .data = { .ptr = pie_verts, .size = gp->pie_join_template_vertex_count * sizeof(gcode_template_vertex_t) },
        .label = "gcode-pie-join-template-vbuf"
    });
    gp->pie_join_template_ibuf = sg_make_buffer(&(sg_buffer_desc){
        .usage.index_buffer = true,
        .data = { .ptr = pie_indices, .size = gp->pie_join_template_index_count * sizeof(uint16_t) },
        .label = "gcode-pie-join-template-ibuf"
    });

    free(pie_verts);
    free(pie_indices);

    // Generate cap template
    int max_cap_vertices = INSTANCED_ALPHA_CAP_SEGMENTS + 2;
    int max_cap_indices = INSTANCED_ALPHA_CAP_SEGMENTS * 3;
    gcode_template_vertex_t* cap_verts = (gcode_template_vertex_t*)malloc(max_cap_vertices * sizeof(gcode_template_vertex_t));
    uint16_t* cap_indices = (uint16_t*)malloc(max_cap_indices * sizeof(uint16_t));

    gcode_generate_cap_template(cap_verts, &gp->cap_template_vertex_count,
                                 cap_indices, &gp->cap_template_index_count,
                                 INSTANCED_ALPHA_CAP_SEGMENTS);

    gp->cap_template_vbuf = sg_make_buffer(&(sg_buffer_desc){
        .usage.vertex_buffer = true,
        .data = { .ptr = cap_verts, .size = gp->cap_template_vertex_count * sizeof(gcode_template_vertex_t) },
        .label = "gcode-cap-template-vbuf"
    });
    gp->cap_template_ibuf = sg_make_buffer(&(sg_buffer_desc){
        .usage.index_buffer = true,
        .data = { .ptr = cap_indices, .size = gp->cap_template_index_count * sizeof(uint16_t) },
        .label = "gcode-cap-template-ibuf"
    });

    free(cap_verts);
    free(cap_indices);

    // Create instance buffers
    size_t intermediate_buf_size = (gp->max_intermediate > 0) ?
        gp->max_intermediate * sizeof(gcode_intermediate_segment_t) : 64;
    gp->intermediate_instance_buf = sg_make_buffer(&(sg_buffer_desc){
        .usage.vertex_buffer = true,
        .usage.stream_update = true,
        .size = intermediate_buf_size,
        .label = "gcode-intermediate-instance-buf"
    });

    gp->terminal_start_instance_buf = sg_make_buffer(&(sg_buffer_desc){
        .usage.vertex_buffer = true,
        .usage.stream_update = true,
        .size = sizeof(gcode_terminal_segment_t),
        .label = "gcode-terminal-start-instance-buf"
    });

    gp->terminal_end_instance_buf = sg_make_buffer(&(sg_buffer_desc){
        .usage.vertex_buffer = true,
        .usage.stream_update = true,
        .size = sizeof(gcode_terminal_segment_t),
        .label = "gcode-terminal-end-instance-buf"
    });

    size_t pie_join_buf_size = (gp->max_pie_joins > 0) ?
        gp->max_pie_joins * sizeof(gcode_pie_join_instance_t) : 64;
    gp->pie_join_instance_buf = sg_make_buffer(&(sg_buffer_desc){
        .usage.vertex_buffer = true,
        .usage.stream_update = true,
        .size = pie_join_buf_size,
        .label = "gcode-pie-join-instance-buf"
    });

    gp->cap_instance_buf = sg_make_buffer(&(sg_buffer_desc){
        .usage.vertex_buffer = true,
        .usage.stream_update = true,
        .size = 2 * sizeof(gcode_segment_instance_t),
        .label = "gcode-cap-instance-buf"
    });

    // Create shaders
    gp->intermediate_shd = sg_make_shader(&(sg_shader_desc){
        .vertex_func = { .source = alpha_intermediate_segment_vs_source, .entry = "vs_main" },
        .fragment_func = { .source = alpha_intermediate_segment_fs_source, .entry = "fs_main" },
        .uniform_blocks[0] = {
            .stage = SG_SHADERSTAGE_VERTEX,
            .size = sizeof(instanced_alpha_params_t),
            .layout = SG_UNIFORMLAYOUT_STD140,
        },
        .label = "gcode-intermediate-shader"
    });

    gp->terminal_shd = sg_make_shader(&(sg_shader_desc){
        .vertex_func = { .source = alpha_terminal_segment_vs_source, .entry = "vs_main" },
        .fragment_func = { .source = alpha_terminal_segment_fs_source, .entry = "fs_main" },
        .uniform_blocks[0] = {
            .stage = SG_SHADERSTAGE_VERTEX,
            .size = sizeof(instanced_alpha_params_t),
            .layout = SG_UNIFORMLAYOUT_STD140,
        },
        .label = "gcode-terminal-shader"
    });

    gp->pie_join_shd = sg_make_shader(&(sg_shader_desc){
        .vertex_func = { .source = alpha_pie_join_vs_source, .entry = "vs_main" },
        .fragment_func = { .source = alpha_pie_join_fs_source, .entry = "fs_main" },
        .uniform_blocks[0] = {
            .stage = SG_SHADERSTAGE_VERTEX,
            .size = sizeof(gcode_pie_join_params_t),
            .layout = SG_UNIFORMLAYOUT_STD140,
        },
        .label = "gcode-pie-join-shader"
    });

    gp->cap_shd = sg_make_shader(&(sg_shader_desc){
        .vertex_func = { .source = instanced_line_vs_source, .entry = "vs_main" },
        .fragment_func = { .source = instanced_line_fs_source, .entry = "fs_main" },
        .uniform_blocks[0] = {
            .stage = SG_SHADERSTAGE_VERTEX,
            .size = sizeof(instanced_alpha_params_t),
            .layout = SG_UNIFORMLAYOUT_STD140,
        },
        .label = "gcode-cap-shader"
    });

    // Check shader compilation status
    ALPHA_POLY_LOG("Shader states: intermediate=%d, terminal=%d, pie_join=%d, cap=%d",
        sg_query_shader_state(gp->intermediate_shd),
        sg_query_shader_state(gp->terminal_shd),
        sg_query_shader_state(gp->pie_join_shd),
        sg_query_shader_state(gp->cap_shd));

    // Alpha blending configuration (reused by all pipelines)
    sg_blend_state alpha_blend = {
        .enabled = true,
        .src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA,
        .dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
        .op_rgb = SG_BLENDOP_ADD,
        .src_factor_alpha = SG_BLENDFACTOR_ONE,
        .dst_factor_alpha = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
        .op_alpha = SG_BLENDOP_ADD,
    };

    // Intermediate segment pipeline (4 points: pA, pB, pC, pD + color)
    gp->intermediate_pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = gp->intermediate_shd,
        .layout = {
            .buffers = {
                [0] = { .step_func = SG_VERTEXSTEP_PER_VERTEX },
                [1] = { .step_func = SG_VERTEXSTEP_PER_INSTANCE },
            },
            .attrs = {
                [0] = { .buffer_index = 0, .format = SG_VERTEXFORMAT_FLOAT3 },  // template_pos
                [1] = { .buffer_index = 1, .format = SG_VERTEXFORMAT_FLOAT3, .offset = 0 },   // pA
                [2] = { .buffer_index = 1, .format = SG_VERTEXFORMAT_FLOAT3, .offset = 12 },  // pB
                [3] = { .buffer_index = 1, .format = SG_VERTEXFORMAT_FLOAT3, .offset = 24 },  // pC
                [4] = { .buffer_index = 1, .format = SG_VERTEXFORMAT_FLOAT3, .offset = 36 },  // pD
                [5] = { .buffer_index = 1, .format = SG_VERTEXFORMAT_FLOAT4, .offset = 48 },  // color
            }
        },
        .index_type = SG_INDEXTYPE_UINT16,
        .primitive_type = SG_PRIMITIVETYPE_TRIANGLES,
        .depth = { .compare = SG_COMPAREFUNC_LESS_EQUAL, .write_enabled = false, .pixel_format = SG_PIXELFORMAT_DEPTH },
        .colors[0] = { .pixel_format = SG_PIXELFORMAT_RGBA8, .blend = alpha_blend },
        .cull_mode = SG_CULLMODE_NONE,
        .label = "gcode-intermediate-pipeline"
    });

    // Terminal segment pipeline (3 points: pA, pB, pC + color)
    gp->terminal_pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = gp->terminal_shd,
        .layout = {
            .buffers = {
                [0] = { .step_func = SG_VERTEXSTEP_PER_VERTEX },
                [1] = { .step_func = SG_VERTEXSTEP_PER_INSTANCE },
            },
            .attrs = {
                [0] = { .buffer_index = 0, .format = SG_VERTEXFORMAT_FLOAT3 },  // template_pos
                [1] = { .buffer_index = 1, .format = SG_VERTEXFORMAT_FLOAT3, .offset = 0 },   // pA
                [2] = { .buffer_index = 1, .format = SG_VERTEXFORMAT_FLOAT3, .offset = 12 },  // pB
                [3] = { .buffer_index = 1, .format = SG_VERTEXFORMAT_FLOAT3, .offset = 24 },  // pC
                [4] = { .buffer_index = 1, .format = SG_VERTEXFORMAT_FLOAT4, .offset = 36 },  // color
            }
        },
        .index_type = SG_INDEXTYPE_UINT16,
        .primitive_type = SG_PRIMITIVETYPE_TRIANGLES,
        .depth = { .compare = SG_COMPAREFUNC_LESS_EQUAL, .write_enabled = false, .pixel_format = SG_PIXELFORMAT_DEPTH },
        .colors[0] = { .pixel_format = SG_PIXELFORMAT_RGBA8, .blend = alpha_blend },
        .cull_mode = SG_CULLMODE_NONE,
        .label = "gcode-terminal-pipeline"
    });

    // Pie-slice join pipeline (3 points: pA, pB, pC + color)
    gp->pie_join_pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = gp->pie_join_shd,
        .layout = {
            .buffers = {
                [0] = { .step_func = SG_VERTEXSTEP_PER_VERTEX },
                [1] = { .step_func = SG_VERTEXSTEP_PER_INSTANCE },
            },
            .attrs = {
                [0] = { .buffer_index = 0, .format = SG_VERTEXFORMAT_FLOAT3 },  // template_pos (id)
                [1] = { .buffer_index = 1, .format = SG_VERTEXFORMAT_FLOAT3, .offset = 0 },   // pA
                [2] = { .buffer_index = 1, .format = SG_VERTEXFORMAT_FLOAT3, .offset = 12 },  // pB
                [3] = { .buffer_index = 1, .format = SG_VERTEXFORMAT_FLOAT3, .offset = 24 },  // pC
                [4] = { .buffer_index = 1, .format = SG_VERTEXFORMAT_FLOAT4, .offset = 36 },  // color
            }
        },
        .index_type = SG_INDEXTYPE_UINT16,
        .primitive_type = SG_PRIMITIVETYPE_TRIANGLES,
        .depth = { .compare = SG_COMPAREFUNC_LESS_EQUAL, .write_enabled = false, .pixel_format = SG_PIXELFORMAT_DEPTH },
        .colors[0] = { .pixel_format = SG_PIXELFORMAT_RGBA8, .blend = alpha_blend },
        .cull_mode = SG_CULLMODE_NONE,
        .label = "gcode-pie-join-pipeline"
    });

    // Cap pipeline (2 points: pA, pB + color) - uses standard instanced line shader
    gp->cap_pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = gp->cap_shd,
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
        .depth = { .compare = SG_COMPAREFUNC_LESS_EQUAL, .write_enabled = false, .pixel_format = SG_PIXELFORMAT_DEPTH },
        .colors[0] = { .pixel_format = SG_PIXELFORMAT_RGBA8, .blend = alpha_blend },
        .cull_mode = SG_CULLMODE_NONE,
        .label = "gcode-cap-pipeline"
    });

    // Check pipeline creation status
    ALPHA_POLY_LOG("Pipeline states: intermediate=%d, terminal=%d, pie_join=%d, cap=%d",
        sg_query_pipeline_state(gp->intermediate_pip),
        sg_query_pipeline_state(gp->terminal_pip),
        sg_query_pipeline_state(gp->pie_join_pip),
        sg_query_pipeline_state(gp->cap_pip));

    ALPHA_POLY_LOG("Init complete: max_intermediate=%d, max_pie_joins=%d",
        gp->max_intermediate, gp->max_pie_joins);

    return true;
}

// Helper to scale a point
static inline void gcode_scale_point(gcode_point_t* p, float scale, float* ox, float* oy, float* oz) {
    *ox = p->x * scale;
    *oy = p->z * scale;  // Swap Y and Z for proper orientation
    *oz = p->y * scale;
}

static inline void instanced_alpha_polylines_update(instanced_alpha_polylines_t* gp) {
    if (gp->path.count < 2) return;

    gp->intermediate_count = 0;
    gp->pie_join_count = 0;
    gp->cap_count = 0;

    float total_len = gp->path.total_length;
    if (total_len < 0.001f) total_len = 1.0f;

    int n = gp->path.count;

    // Helper to compute alpha at a position
    #define COMPUTE_ALPHA(pos) ({ \
        float distance = fabsf((pos) - gp->timeline_position); \
        float fade_zone = gp->highlight_width; \
        if (fade_zone < 0.001f) fade_zone = 0.001f; \
        float highlight = 1.0f - gcode_smoothstep(0.0f, fade_zone, distance); \
        gp->base_alpha + (1.0f - gp->base_alpha) * highlight; \
    })

    // First segment (terminal start): P0-P1 with neighbor P2
    // For terminal shader: pA=P0, pB=P1, pC=P2
    if (n >= 2) {
        gcode_terminal_segment_t* ts = gp->terminal_start;
        float seg_pos = (gp->path.cumulative_lengths[0] + gp->path.cumulative_lengths[1]) * 0.5f / total_len;
        float alpha = COMPUTE_ALPHA(seg_pos);

        gcode_scale_point(&gp->path.points[0], gp->scale, &ts->pAx, &ts->pAy, &ts->pAz);
        gcode_scale_point(&gp->path.points[1], gp->scale, &ts->pBx, &ts->pBy, &ts->pBz);
        if (n >= 3) {
            gcode_scale_point(&gp->path.points[2], gp->scale, &ts->pCx, &ts->pCy, &ts->pCz);
        } else {
            // No neighbor, use P1 as dummy
            ts->pCx = ts->pBx; ts->pCy = ts->pBy; ts->pCz = ts->pBz;
        }

        // Debug: CYAN for start terminal
        if (gp->debug_colors) {
            ts->r = 0.0f; ts->g = 1.0f; ts->b = 1.0f;
        } else {
            ts->r = gp->color_r; ts->g = gp->color_g; ts->b = gp->color_b;
        }
        ts->a = alpha;
    }

    // Last segment (terminal end): P(n-2)-P(n-1) with neighbor P(n-3)
    // For terminal shader: pA = cap end (z=0), pB = miter end (z=1), pC = neighbor
    // We want the cap at P(n-1) and miter at P(n-2), so we REVERSE the point order:
    //   pA = P(n-1) (cap end, flat)
    //   pB = P(n-2) (miter end, joins with intermediate segments)
    //   pC = P(n-3) (neighbor for miter calculation)
    // This draws the segment "backwards" but that's intentional for correct miter placement
    if (n >= 2) {
        gcode_terminal_segment_t* te = gp->terminal_end;
        int last = n - 1;
        float seg_pos = (gp->path.cumulative_lengths[last-1] + gp->path.cumulative_lengths[last]) * 0.5f / total_len;
        float alpha = COMPUTE_ALPHA(seg_pos);

        // pA = P(n-1) - cap end (where the end cap goes)
        gcode_scale_point(&gp->path.points[last], gp->scale, &te->pAx, &te->pAy, &te->pAz);
        // pB = P(n-2) - miter end (joins with previous segment)
        gcode_scale_point(&gp->path.points[last-1], gp->scale, &te->pBx, &te->pBy, &te->pBz);
        // pC = P(n-3) - neighbor for miter calculation (or P(n-2) if n < 3)
        if (n >= 3) {
            gcode_scale_point(&gp->path.points[last-2], gp->scale, &te->pCx, &te->pCy, &te->pCz);
        } else {
            // Only 2 points, no neighbor - use pB as dummy
            te->pCx = te->pBx; te->pCy = te->pBy; te->pCz = te->pBz;
        }

        // Debug: MAGENTA for end terminal
        if (gp->debug_colors) {
            te->r = 1.0f; te->g = 0.0f; te->b = 1.0f;
        } else {
            te->r = gp->color_r; te->g = gp->color_g; te->b = gp->color_b;
        }
        te->a = alpha;
    }

    // Intermediate segments: P(i)-P(i+1) for i = 1 to n-3
    // Each needs: pA=P(i-1), pB=P(i), pC=P(i+1), pD=P(i+2)
    for (int i = 1; i <= n - 3; i++) {
        gcode_intermediate_segment_t* seg = &gp->intermediate_segments[gp->intermediate_count++];
        float seg_pos = (gp->path.cumulative_lengths[i] + gp->path.cumulative_lengths[i+1]) * 0.5f / total_len;
        float alpha = COMPUTE_ALPHA(seg_pos);

        gcode_scale_point(&gp->path.points[i-1], gp->scale, &seg->pAx, &seg->pAy, &seg->pAz);
        gcode_scale_point(&gp->path.points[i], gp->scale, &seg->pBx, &seg->pBy, &seg->pBz);
        gcode_scale_point(&gp->path.points[i+1], gp->scale, &seg->pCx, &seg->pCy, &seg->pCz);
        gcode_scale_point(&gp->path.points[i+2], gp->scale, &seg->pDx, &seg->pDy, &seg->pDz);

        // Debug: RED for intermediate segments
        if (gp->debug_colors) {
            seg->r = 1.0f; seg->g = 0.3f; seg->b = 0.3f;
        } else {
            seg->r = gp->color_r; seg->g = gp->color_g; seg->b = gp->color_b;
        }
        seg->a = alpha;
    }

    // Pie-slice joins at interior vertices: P(i) for i = 1 to n-2
    // Each needs: pA=P(i-1), pB=P(i), pC=P(i+1)
    for (int i = 1; i <= n - 2; i++) {
        gcode_pie_join_instance_t* join = &gp->pie_joins[gp->pie_join_count++];
        float pos = gp->path.cumulative_lengths[i] / total_len;
        float alpha = COMPUTE_ALPHA(pos);

        gcode_scale_point(&gp->path.points[i-1], gp->scale, &join->pAx, &join->pAy, &join->pAz);
        gcode_scale_point(&gp->path.points[i], gp->scale, &join->pBx, &join->pBy, &join->pBz);
        gcode_scale_point(&gp->path.points[i+1], gp->scale, &join->pCx, &join->pCy, &join->pCz);

        // Debug: YELLOW for joins
        if (gp->debug_colors) {
            join->r = 1.0f; join->g = 1.0f; join->b = 0.3f;
        } else {
            join->r = gp->color_r; join->g = gp->color_g; join->b = gp->color_b;
        }
        join->a = alpha;
    }

    // Caps at start (P0) and end (P(n-1))
    // Start cap
    {
        gcode_segment_instance_t* cap = &gp->caps[gp->cap_count++];
        float alpha = COMPUTE_ALPHA(0.0f);

        gcode_scale_point(&gp->path.points[0], gp->scale, &cap->ax, &cap->ay, &cap->az);
        gcode_scale_point(&gp->path.points[1], gp->scale, &cap->bx, &cap->by, &cap->bz);

        // Debug: BLUE for start cap
        if (gp->debug_colors) {
            cap->r = 0.3f; cap->g = 0.5f; cap->b = 1.0f;
        } else {
            cap->r = gp->color_r; cap->g = gp->color_g; cap->b = gp->color_b;
        }
        cap->a = alpha;
    }

    // End cap
    {
        gcode_segment_instance_t* cap = &gp->caps[gp->cap_count++];
        float alpha = COMPUTE_ALPHA(1.0f);

        gcode_scale_point(&gp->path.points[n-1], gp->scale, &cap->ax, &cap->ay, &cap->az);
        gcode_scale_point(&gp->path.points[n-2], gp->scale, &cap->bx, &cap->by, &cap->bz);

        // Debug: GREEN for end cap
        if (gp->debug_colors) {
            cap->r = 0.3f; cap->g = 1.0f; cap->b = 0.3f;
        } else {
            cap->r = gp->color_r; cap->g = gp->color_g; cap->b = gp->color_b;
        }
        cap->a = alpha;
    }

    #undef COMPUTE_ALPHA

    // Upload instance data
    if (gp->intermediate_count > 0) {
        sg_update_buffer(gp->intermediate_instance_buf, &(sg_range){
            .ptr = gp->intermediate_segments,
            .size = gp->intermediate_count * sizeof(gcode_intermediate_segment_t)
        });
    }

    sg_update_buffer(gp->terminal_start_instance_buf, &(sg_range){
        .ptr = gp->terminal_start,
        .size = sizeof(gcode_terminal_segment_t)
    });

    sg_update_buffer(gp->terminal_end_instance_buf, &(sg_range){
        .ptr = gp->terminal_end,
        .size = sizeof(gcode_terminal_segment_t)
    });

    if (gp->pie_join_count > 0) {
        sg_update_buffer(gp->pie_join_instance_buf, &(sg_range){
            .ptr = gp->pie_joins,
            .size = gp->pie_join_count * sizeof(gcode_pie_join_instance_t)
        });
    }

    sg_update_buffer(gp->cap_instance_buf, &(sg_range){
        .ptr = gp->caps,
        .size = gp->cap_count * sizeof(gcode_segment_instance_t)
    });
}

static inline void instanced_alpha_polylines_draw(instanced_alpha_polylines_t* gp, mat4_t mvp, float aspect_ratio) {
    if (gp->path.count < 2) return;

    instanced_alpha_params_t params = {
        .mvp = mvp,
        .line_width = gp->line_width,
        .aspect_ratio = aspect_ratio,
    };

    gcode_pie_join_params_t pie_params = {
        .mvp = mvp,
        .line_width = gp->line_width,
        .aspect_ratio = aspect_ratio,
        .join_resolution = (float)INSTANCED_ALPHA_JOIN_SEGMENTS,
        .miter_angle_limit = gp->miter_angle_limit * 3.14159265359f / 180.0f,  // Convert degrees to radians
    };

    // Draw terminal start segment
    sg_apply_pipeline(gp->terminal_pip);
    sg_apply_bindings(&(sg_bindings){
        .vertex_buffers = { [0] = gp->terminal_template_vbuf, [1] = gp->terminal_start_instance_buf },
        .index_buffer = gp->terminal_template_ibuf,
    });
    sg_apply_uniforms(0, &SG_RANGE(params));
    sg_draw(0, gp->terminal_template_index_count, 1);

    // Draw terminal end segment
    sg_apply_bindings(&(sg_bindings){
        .vertex_buffers = { [0] = gp->terminal_template_vbuf, [1] = gp->terminal_end_instance_buf },
        .index_buffer = gp->terminal_template_ibuf,
    });
    sg_apply_uniforms(0, &SG_RANGE(params));
    sg_draw(0, gp->terminal_template_index_count, 1);

    // Draw intermediate segments
    if (gp->intermediate_count > 0) {
        sg_apply_pipeline(gp->intermediate_pip);
        sg_apply_bindings(&(sg_bindings){
            .vertex_buffers = { [0] = gp->intermediate_template_vbuf, [1] = gp->intermediate_instance_buf },
            .index_buffer = gp->intermediate_template_ibuf,
        });
        sg_apply_uniforms(0, &SG_RANGE(params));
        sg_draw(0, gp->intermediate_template_index_count, gp->intermediate_count);
    }

    // Draw pie-slice joins
    if (gp->pie_join_count > 0) {
        sg_apply_pipeline(gp->pie_join_pip);
        sg_apply_bindings(&(sg_bindings){
            .vertex_buffers = { [0] = gp->pie_join_template_vbuf, [1] = gp->pie_join_instance_buf },
            .index_buffer = gp->pie_join_template_ibuf,
        });
        sg_apply_uniforms(0, &SG_RANGE(pie_params));
        sg_draw(0, gp->pie_join_template_index_count, gp->pie_join_count);
    }

    // Draw caps
    if (gp->cap_count > 0) {
        sg_apply_pipeline(gp->cap_pip);
        sg_apply_bindings(&(sg_bindings){
            .vertex_buffers = { [0] = gp->cap_template_vbuf, [1] = gp->cap_instance_buf },
            .index_buffer = gp->cap_template_ibuf,
        });
        sg_apply_uniforms(0, &SG_RANGE(params));
        sg_draw(0, gp->cap_template_index_count, gp->cap_count);
    }
}

static inline void instanced_alpha_polylines_shutdown(instanced_alpha_polylines_t* gp) {
    sg_destroy_pipeline(gp->intermediate_pip);
    sg_destroy_pipeline(gp->terminal_pip);
    sg_destroy_pipeline(gp->pie_join_pip);
    sg_destroy_pipeline(gp->cap_pip);

    sg_destroy_shader(gp->intermediate_shd);
    sg_destroy_shader(gp->terminal_shd);
    sg_destroy_shader(gp->pie_join_shd);
    sg_destroy_shader(gp->cap_shd);

    sg_destroy_buffer(gp->intermediate_template_vbuf);
    sg_destroy_buffer(gp->intermediate_template_ibuf);
    sg_destroy_buffer(gp->intermediate_instance_buf);

    sg_destroy_buffer(gp->terminal_template_vbuf);
    sg_destroy_buffer(gp->terminal_template_ibuf);
    sg_destroy_buffer(gp->terminal_start_instance_buf);
    sg_destroy_buffer(gp->terminal_end_instance_buf);

    sg_destroy_buffer(gp->pie_join_template_vbuf);
    sg_destroy_buffer(gp->pie_join_template_ibuf);
    sg_destroy_buffer(gp->pie_join_instance_buf);

    sg_destroy_buffer(gp->cap_template_vbuf);
    sg_destroy_buffer(gp->cap_template_ibuf);
    sg_destroy_buffer(gp->cap_instance_buf);

    if (gp->intermediate_segments) {
        free(gp->intermediate_segments);
        gp->intermediate_segments = NULL;
    }
    if (gp->terminal_start) {
        free(gp->terminal_start);
        gp->terminal_start = NULL;
    }
    if (gp->terminal_end) {
        free(gp->terminal_end);
        gp->terminal_end = NULL;
    }
    if (gp->pie_joins) {
        free(gp->pie_joins);
        gp->pie_joins = NULL;
    }
    if (gp->caps) {
        free(gp->caps);
        gp->caps = NULL;
    }

    gcode_path_shutdown(&gp->path);
}

#endif // INSTANCED_ALPHA_POLYLINES_H
