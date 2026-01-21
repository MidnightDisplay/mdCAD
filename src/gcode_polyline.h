//------------------------------------------------------------------------------
// gcode_polyline.h - G-code path renderer with timeline effect (header-only)
//
// Renders G-code toolpath as thick polylines with alpha blending.
// Features timeline scrubbing: highlight a percentage along the path
// with gradual alpha fade to base value.
//------------------------------------------------------------------------------
#ifndef GCODE_POLYLINE_H
#define GCODE_POLYLINE_H

#include "platform.h"
#include "sokol_gfx.h"
#include "math3d.h"
#include "gcode_loader.h"
#include "shaders/instanced_line_shaders.h"
#include <math.h>
#include <stdlib.h>

//------------------------------------------------------------------------------
// Configuration
//------------------------------------------------------------------------------
#define GCODE_POLYLINE_CAP_SEGMENTS 8
#define GCODE_POLYLINE_JOIN_SEGMENTS 12
#define GCODE_POLYLINE_DEFAULT_WIDTH 0.03f
#define GCODE_POLYLINE_DEFAULT_SCALE 0.05f
#define GCODE_POLYLINE_DEFAULT_BASE_ALPHA 0.15f
#define GCODE_POLYLINE_DEFAULT_HIGHLIGHT_WIDTH 0.02f

//------------------------------------------------------------------------------
// Types
//------------------------------------------------------------------------------

typedef struct {
    float x, y, z;
} gcode_template_vertex_t;

typedef struct {
    float ax, ay, az;
    float bx, by, bz;
    float r, g, b, a;
} gcode_segment_instance_t;

typedef struct {
    float px, py, pz;
    float r, g, b, a;
} gcode_join_instance_t;

typedef struct {
    mat4_t mvp;
    float line_width;
    float aspect_ratio;
    float _pad[2];
} gcode_polyline_params_t;

typedef struct {
    // GPU resources for segments
    sg_pipeline segment_pip;
    sg_buffer segment_template_vbuf;
    sg_buffer segment_template_ibuf;
    sg_buffer segment_instance_buf;
    sg_shader segment_shd;
    int segment_template_vertex_count;
    int segment_template_index_count;

    // GPU resources for joins
    sg_pipeline join_pip;
    sg_buffer join_template_vbuf;
    sg_buffer join_template_ibuf;
    sg_buffer join_instance_buf;
    sg_shader join_shd;
    int join_template_vertex_count;
    int join_template_index_count;

    // Path data
    gcode_path_t path;

    // Instance data (dynamically allocated)
    gcode_segment_instance_t* segments;
    gcode_join_instance_t* joins;
    int segment_count;
    int join_count;
    int max_segments;
    int max_joins;

    // Parameters
    float line_width;
    float scale;
    float color_r, color_g, color_b;
    float timeline_position;   // 0.0 - 1.0
    float base_alpha;          // Base transparency (0.0 - 1.0)
    float highlight_width;     // Width of highlight fade (0.0 - 0.5)
} gcode_polyline_t;

//------------------------------------------------------------------------------
// Join shader (same as instanced_polylines.h)
//------------------------------------------------------------------------------
#if defined(SOKOL_GLCORE)
static const char* gcode_join_vs_source =
    "#version 330\n"
    "uniform mat4 mvp;\n"
    "uniform float line_width;\n"
    "uniform float aspect_ratio;\n"
    "layout(location=0) in vec3 template_pos;\n"
    "layout(location=1) in vec3 point;\n"
    "layout(location=2) in vec4 color;\n"
    "out vec4 v_color;\n"
    "void main() {\n"
    "    vec4 clip_p = mvp * vec4(point, 1.0);\n"
    "    vec2 offset = vec2(template_pos.x / aspect_ratio, template_pos.y) * line_width;\n"
    "    gl_Position = clip_p;\n"
    "    gl_Position.xy += offset * clip_p.w;\n"
    "    v_color = color;\n"
    "}\n";
static const char* gcode_join_fs_source =
    "#version 330\n"
    "in vec4 v_color;\n"
    "out vec4 frag_color;\n"
    "void main() {\n"
    "    frag_color = v_color;\n"
    "}\n";
#elif defined(SOKOL_METAL)
static const char* gcode_join_vs_source =
    "#include <metal_stdlib>\n"
    "using namespace metal;\n"
    "struct vs_in {\n"
    "    float3 template_pos [[attribute(0)]];\n"
    "    float3 point [[attribute(1)]];\n"
    "    float4 color [[attribute(2)]];\n"
    "};\n"
    "struct vs_out {\n"
    "    float4 pos [[position]];\n"
    "    float4 color;\n"
    "};\n"
    "struct vs_params {\n"
    "    float4x4 mvp;\n"
    "    float line_width;\n"
    "    float aspect_ratio;\n"
    "};\n"
    "vertex vs_out vs_main(vs_in in [[stage_in]], constant vs_params& params [[buffer(0)]]) {\n"
    "    vs_out out;\n"
    "    float4 clip_p = params.mvp * float4(in.point, 1.0);\n"
    "    float2 offset = float2(in.template_pos.x / params.aspect_ratio, in.template_pos.y) * params.line_width;\n"
    "    out.pos = clip_p;\n"
    "    out.pos.xy += offset * clip_p.w;\n"
    "    out.color = in.color;\n"
    "    return out;\n"
    "};\n";
static const char* gcode_join_fs_source =
    "#include <metal_stdlib>\n"
    "using namespace metal;\n"
    "struct fs_in {\n"
    "    float4 color;\n"
    "};\n"
    "fragment float4 fs_main(fs_in in [[stage_in]]) {\n"
    "    return in.color;\n"
    "}\n";
#elif defined(SOKOL_WGPU)
static const char* gcode_join_vs_source =
    "struct vs_params {\n"
    "    mvp: mat4x4<f32>,\n"
    "    line_width: f32,\n"
    "    aspect_ratio: f32,\n"
    "};\n"
    "@group(0) @binding(0) var<uniform> params: vs_params;\n"
    "struct vs_out {\n"
    "    @builtin(position) pos: vec4<f32>,\n"
    "    @location(0) color: vec4<f32>,\n"
    "};\n"
    "@vertex\n"
    "fn vs_main(\n"
    "    @location(0) template_pos: vec3<f32>,\n"
    "    @location(1) point: vec3<f32>,\n"
    "    @location(2) color: vec4<f32>\n"
    ") -> vs_out {\n"
    "    var out: vs_out;\n"
    "    let clip_p = params.mvp * vec4<f32>(point, 1.0);\n"
    "    let offset = vec2<f32>(template_pos.x / params.aspect_ratio, template_pos.y) * params.line_width;\n"
    "    out.pos = clip_p;\n"
    "    out.pos.x = out.pos.x + offset.x * clip_p.w;\n"
    "    out.pos.y = out.pos.y + offset.y * clip_p.w;\n"
    "    out.color = color;\n"
    "    return out;\n"
    "}\n";
static const char* gcode_join_fs_source =
    "@fragment\n"
    "fn fs_main(@location(0) color: vec4<f32>) -> @location(0) vec4<f32> {\n"
    "    return color;\n"
    "}\n";
#elif defined(SOKOL_D3D11)
static const char* gcode_join_vs_source =
    "cbuffer vs_params : register(b0) {\n"
    "    float4x4 mvp;\n"
    "    float line_width;\n"
    "    float aspect_ratio;\n"
    "};\n"
    "struct vs_in {\n"
    "    float3 template_pos : POSITION;\n"
    "    float3 point : TEXCOORD0;\n"
    "    float4 color : COLOR;\n"
    "};\n"
    "struct vs_out {\n"
    "    float4 color : COLOR;\n"
    "    float4 pos : SV_Position;\n"
    "};\n"
    "vs_out main(vs_in inp) {\n"
    "    vs_out outp;\n"
    "    float4 clip_p = mul(mvp, float4(inp.point, 1.0));\n"
    "    float2 offset = float2(inp.template_pos.x / aspect_ratio, inp.template_pos.y) * line_width;\n"
    "    outp.pos = clip_p;\n"
    "    outp.pos.xy += offset * clip_p.w;\n"
    "    outp.color = inp.color;\n"
    "    return outp;\n"
    "}\n";
static const char* gcode_join_fs_source =
    "struct fs_in {\n"
    "    float4 color : COLOR;\n"
    "};\n"
    "float4 main(fs_in inp) : SV_Target0 {\n"
    "    return inp.color;\n"
    "}\n";
#endif

//------------------------------------------------------------------------------
// Template geometry generation
//------------------------------------------------------------------------------
static inline void gcode_generate_segment_template(
    gcode_template_vertex_t* vertices, int* vertex_count,
    uint16_t* indices, int* index_count,
    int cap_segments
) {
    int vi = 0;
    int ii = 0;

    // Main rectangle
    vertices[vi++] = (gcode_template_vertex_t){ 0.0f, -0.5f, 0.0f };
    vertices[vi++] = (gcode_template_vertex_t){ 0.0f,  0.5f, 0.0f };
    vertices[vi++] = (gcode_template_vertex_t){ 0.0f, -0.5f, 1.0f };
    vertices[vi++] = (gcode_template_vertex_t){ 0.0f,  0.5f, 1.0f };

    indices[ii++] = 0; indices[ii++] = 2; indices[ii++] = 1;
    indices[ii++] = 1; indices[ii++] = 2; indices[ii++] = 3;

    // Left semicircle cap
    int cap_a_center = vi;
    vertices[vi++] = (gcode_template_vertex_t){ 0.0f, 0.0f, 0.0f };
    for (int i = 0; i <= cap_segments; i++) {
        float angle = 3.14159265359f * 0.5f + 3.14159265359f * (float)i / (float)cap_segments;
        float x = cosf(angle) * 0.5f;
        float y = sinf(angle) * 0.5f;
        vertices[vi++] = (gcode_template_vertex_t){ x, y, 0.0f };
    }
    for (int i = 0; i < cap_segments; i++) {
        indices[ii++] = cap_a_center;
        indices[ii++] = cap_a_center + 1 + i;
        indices[ii++] = cap_a_center + 2 + i;
    }

    // Right semicircle cap
    int cap_b_center = vi;
    vertices[vi++] = (gcode_template_vertex_t){ 0.0f, 0.0f, 1.0f };
    for (int i = 0; i <= cap_segments; i++) {
        float angle = -3.14159265359f * 0.5f + 3.14159265359f * (float)i / (float)cap_segments;
        float x = cosf(angle) * 0.5f;
        float y = sinf(angle) * 0.5f;
        vertices[vi++] = (gcode_template_vertex_t){ x, y, 1.0f };
    }
    for (int i = 0; i < cap_segments; i++) {
        indices[ii++] = cap_b_center;
        indices[ii++] = cap_b_center + 1 + i;
        indices[ii++] = cap_b_center + 2 + i;
    }

    *vertex_count = vi;
    *index_count = ii;
}

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

static inline bool gcode_polyline_init(gcode_polyline_t* gp, const char* gcode_filename) {
    // Initialize path
    gcode_path_init(&gp->path);

    // Load G-code file
    if (!gcode_path_load(&gp->path, gcode_filename)) {
        return false;
    }

    // Center the path
    gcode_path_center(&gp->path);

    // Initialize parameters
    gp->line_width = GCODE_POLYLINE_DEFAULT_WIDTH;
    gp->scale = GCODE_POLYLINE_DEFAULT_SCALE;

    // Default color: Catppuccin Frappé lavender (#babbf1)
    gp->color_r = 0.729f;
    gp->color_g = 0.733f;
    gp->color_b = 0.945f;

    gp->timeline_position = 0.5f;
    gp->base_alpha = GCODE_POLYLINE_DEFAULT_BASE_ALPHA;
    gp->highlight_width = GCODE_POLYLINE_DEFAULT_HIGHLIGHT_WIDTH;

    // Allocate instance arrays
    gp->max_segments = gp->path.count - 1;
    gp->max_joins = gp->path.count - 2;
    gp->segments = (gcode_segment_instance_t*)malloc(gp->max_segments * sizeof(gcode_segment_instance_t));
    gp->joins = (gcode_join_instance_t*)malloc(gp->max_joins * sizeof(gcode_join_instance_t));
    gp->segment_count = 0;
    gp->join_count = 0;

    // Generate segment template
    int max_seg_vertices = 4 + 2 * (GCODE_POLYLINE_CAP_SEGMENTS + 2);
    int max_seg_indices = 6 + 2 * GCODE_POLYLINE_CAP_SEGMENTS * 3;

    gcode_template_vertex_t* seg_verts = (gcode_template_vertex_t*)malloc(max_seg_vertices * sizeof(gcode_template_vertex_t));
    uint16_t* seg_indices = (uint16_t*)malloc(max_seg_indices * sizeof(uint16_t));

    gcode_generate_segment_template(
        seg_verts, &gp->segment_template_vertex_count,
        seg_indices, &gp->segment_template_index_count,
        GCODE_POLYLINE_CAP_SEGMENTS
    );

    gp->segment_template_vbuf = sg_make_buffer(&(sg_buffer_desc){
        .usage.vertex_buffer = true,
        .data = { .ptr = seg_verts, .size = gp->segment_template_vertex_count * sizeof(gcode_template_vertex_t) },
        .label = "gcode-segment-template-vbuf"
    });

    gp->segment_template_ibuf = sg_make_buffer(&(sg_buffer_desc){
        .usage.index_buffer = true,
        .data = { .ptr = seg_indices, .size = gp->segment_template_index_count * sizeof(uint16_t) },
        .label = "gcode-segment-template-ibuf"
    });

    free(seg_verts);
    free(seg_indices);

    // Generate join template
    int max_join_vertices = GCODE_POLYLINE_JOIN_SEGMENTS + 2;
    int max_join_indices = GCODE_POLYLINE_JOIN_SEGMENTS * 3;

    gcode_template_vertex_t* join_verts = (gcode_template_vertex_t*)malloc(max_join_vertices * sizeof(gcode_template_vertex_t));
    uint16_t* join_indices = (uint16_t*)malloc(max_join_indices * sizeof(uint16_t));

    gcode_generate_join_template(
        join_verts, &gp->join_template_vertex_count,
        join_indices, &gp->join_template_index_count,
        GCODE_POLYLINE_JOIN_SEGMENTS
    );

    gp->join_template_vbuf = sg_make_buffer(&(sg_buffer_desc){
        .usage.vertex_buffer = true,
        .data = { .ptr = join_verts, .size = gp->join_template_vertex_count * sizeof(gcode_template_vertex_t) },
        .label = "gcode-join-template-vbuf"
    });

    gp->join_template_ibuf = sg_make_buffer(&(sg_buffer_desc){
        .usage.index_buffer = true,
        .data = { .ptr = join_indices, .size = gp->join_template_index_count * sizeof(uint16_t) },
        .label = "gcode-join-template-ibuf"
    });

    free(join_verts);
    free(join_indices);

    // Create instance buffers (stream for per-frame updates when timeline changes)
    gp->segment_instance_buf = sg_make_buffer(&(sg_buffer_desc){
        .usage.vertex_buffer = true,
        .usage.stream_update = true,
        .size = gp->max_segments * sizeof(gcode_segment_instance_t),
        .label = "gcode-segment-instance-buf"
    });

    gp->join_instance_buf = sg_make_buffer(&(sg_buffer_desc){
        .usage.vertex_buffer = true,
        .usage.stream_update = true,
        .size = gp->max_joins * sizeof(gcode_join_instance_t),
        .label = "gcode-join-instance-buf"
    });

    // Segment shader (reuse instanced line shader)
    gp->segment_shd = sg_make_shader(&(sg_shader_desc){
        .vertex_func = { .source = instanced_line_vs_source, .entry = "vs_main" },
        .fragment_func = { .source = instanced_line_fs_source, .entry = "fs_main" },
        .uniform_blocks[0] = {
            .stage = SG_SHADERSTAGE_VERTEX,
            .size = sizeof(gcode_polyline_params_t),
            .layout = SG_UNIFORMLAYOUT_STD140,
            .glsl_uniforms = {
                [0] = { .type = SG_UNIFORMTYPE_MAT4, .glsl_name = "mvp" },
                [1] = { .type = SG_UNIFORMTYPE_FLOAT, .glsl_name = "line_width" },
                [2] = { .type = SG_UNIFORMTYPE_FLOAT, .glsl_name = "aspect_ratio" },
            }
        },
        .label = "gcode-segment-shader"
    });

    // Segment pipeline with alpha blending
    gp->segment_pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = gp->segment_shd,
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
            .write_enabled = false,  // Disable for transparency
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
        .label = "gcode-segment-pipeline"
    });

    // Join shader
    gp->join_shd = sg_make_shader(&(sg_shader_desc){
        .vertex_func = { .source = gcode_join_vs_source, .entry = "vs_main" },
        .fragment_func = { .source = gcode_join_fs_source, .entry = "fs_main" },
        .uniform_blocks[0] = {
            .stage = SG_SHADERSTAGE_VERTEX,
            .size = sizeof(gcode_polyline_params_t),
            .layout = SG_UNIFORMLAYOUT_STD140,
            .glsl_uniforms = {
                [0] = { .type = SG_UNIFORMTYPE_MAT4, .glsl_name = "mvp" },
                [1] = { .type = SG_UNIFORMTYPE_FLOAT, .glsl_name = "line_width" },
                [2] = { .type = SG_UNIFORMTYPE_FLOAT, .glsl_name = "aspect_ratio" },
            }
        },
        .label = "gcode-join-shader"
    });

    // Join pipeline with alpha blending
    gp->join_pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = gp->join_shd,
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
            .write_enabled = false,  // Disable for transparency
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
        .label = "gcode-join-pipeline"
    });

    return true;
}

static inline void gcode_polyline_update(gcode_polyline_t* gp) {
    if (gp->path.count < 2) return;

    gp->segment_count = 0;
    gp->join_count = 0;

    float total_len = gp->path.total_length;
    if (total_len < 0.001f) total_len = 1.0f;

    // Build segments with timeline-based alpha
    for (int i = 0; i < gp->path.count - 1; i++) {
        gcode_point_t* pa = &gp->path.points[i];
        gcode_point_t* pb = &gp->path.points[i + 1];

        // Calculate normalized position along path (midpoint of segment)
        float len_at_a = gp->path.cumulative_lengths[i];
        float len_at_b = gp->path.cumulative_lengths[i + 1];
        float segment_pos = (len_at_a + len_at_b) * 0.5f / total_len;

        // Calculate alpha based on distance from timeline position
        float distance = fabsf(segment_pos - gp->timeline_position);
        // Use highlight_width as the fade zone
        float fade_zone = gp->highlight_width;
        if (fade_zone < 0.001f) fade_zone = 0.001f;

        float highlight = 1.0f - gcode_smoothstep(0.0f, fade_zone, distance);
        float alpha = gp->base_alpha + (1.0f - gp->base_alpha) * highlight;

        gcode_segment_instance_t* seg = &gp->segments[gp->segment_count++];
        seg->ax = pa->x * gp->scale;
        seg->ay = pa->z * gp->scale;  // Swap Y and Z for proper orientation
        seg->az = pa->y * gp->scale;
        seg->bx = pb->x * gp->scale;
        seg->by = pb->z * gp->scale;
        seg->bz = pb->y * gp->scale;
        seg->r = gp->color_r;
        seg->g = gp->color_g;
        seg->b = gp->color_b;
        seg->a = alpha;
    }

    // Build joins at interior vertices
    for (int i = 1; i < gp->path.count - 1; i++) {
        gcode_point_t* p = &gp->path.points[i];

        // Calculate normalized position
        float pos = gp->path.cumulative_lengths[i] / total_len;

        // Calculate alpha
        float distance = fabsf(pos - gp->timeline_position);
        float fade_zone = gp->highlight_width;
        if (fade_zone < 0.001f) fade_zone = 0.001f;

        float highlight = 1.0f - gcode_smoothstep(0.0f, fade_zone, distance);
        float alpha = gp->base_alpha + (1.0f - gp->base_alpha) * highlight;

        gcode_join_instance_t* join = &gp->joins[gp->join_count++];
        join->px = p->x * gp->scale;
        join->py = p->z * gp->scale;
        join->pz = p->y * gp->scale;
        join->r = gp->color_r;
        join->g = gp->color_g;
        join->b = gp->color_b;
        join->a = alpha;
    }

    // Upload instance data
    if (gp->segment_count > 0) {
        sg_update_buffer(gp->segment_instance_buf, &(sg_range){
            .ptr = gp->segments,
            .size = gp->segment_count * sizeof(gcode_segment_instance_t)
        });
    }

    if (gp->join_count > 0) {
        sg_update_buffer(gp->join_instance_buf, &(sg_range){
            .ptr = gp->joins,
            .size = gp->join_count * sizeof(gcode_join_instance_t)
        });
    }
}

static inline void gcode_polyline_draw(gcode_polyline_t* gp, mat4_t mvp, float aspect_ratio) {
    if (gp->segment_count == 0) return;

    gcode_polyline_params_t params = {
        .mvp = mvp,
        .line_width = gp->line_width,
        .aspect_ratio = aspect_ratio,
    };

    // Draw segments
    sg_apply_pipeline(gp->segment_pip);
    sg_apply_bindings(&(sg_bindings){
        .vertex_buffers = {
            [0] = gp->segment_template_vbuf,
            [1] = gp->segment_instance_buf,
        },
        .index_buffer = gp->segment_template_ibuf,
    });
    sg_apply_uniforms(0, &SG_RANGE(params));
    sg_draw(0, gp->segment_template_index_count, gp->segment_count);

    // Draw joins
    if (gp->join_count > 0) {
        sg_apply_pipeline(gp->join_pip);
        sg_apply_bindings(&(sg_bindings){
            .vertex_buffers = {
                [0] = gp->join_template_vbuf,
                [1] = gp->join_instance_buf,
            },
            .index_buffer = gp->join_template_ibuf,
        });
        sg_apply_uniforms(0, &SG_RANGE(params));
        sg_draw(0, gp->join_template_index_count, gp->join_count);
    }
}

static inline void gcode_polyline_shutdown(gcode_polyline_t* gp) {
    sg_destroy_pipeline(gp->segment_pip);
    sg_destroy_pipeline(gp->join_pip);
    sg_destroy_shader(gp->segment_shd);
    sg_destroy_shader(gp->join_shd);
    sg_destroy_buffer(gp->segment_template_vbuf);
    sg_destroy_buffer(gp->segment_template_ibuf);
    sg_destroy_buffer(gp->segment_instance_buf);
    sg_destroy_buffer(gp->join_template_vbuf);
    sg_destroy_buffer(gp->join_template_ibuf);
    sg_destroy_buffer(gp->join_instance_buf);

    if (gp->segments) {
        free(gp->segments);
        gp->segments = NULL;
    }
    if (gp->joins) {
        free(gp->joins);
        gp->joins = NULL;
    }

    gcode_path_shutdown(&gp->path);
}

#endif // GCODE_POLYLINE_H
