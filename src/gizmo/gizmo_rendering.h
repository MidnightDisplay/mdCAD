//------------------------------------------------------------------------------
// gizmo_rendering.h - GPU resources for gizmo overlay (header-only)
//
// Separate pipelines with depth-always + no depth write for always-on-top
// rendering. Uses same shaders as geometry_batch.h but different pipelines.
//------------------------------------------------------------------------------
#ifndef GIZMO_RENDERING_H
#define GIZMO_RENDERING_H

#include "../platform.h"
#include "sokol_gfx.h"
#include "../math3d.h"
#include "../shaders/instanced_line_shaders.h"
#include "../shaders/join_shaders.h"
#include "../gpu/geometry_batch.h"  // For instance types + template generation + params

#include <stdlib.h>
#include <string.h>

//------------------------------------------------------------------------------
// Configuration
//------------------------------------------------------------------------------
#define GIZMO_MAX_LINES   32
#define GIZMO_MAX_POINTS  10016  // 16 gizmo tips + up to 10000 vertex handles

//------------------------------------------------------------------------------
// Types
//------------------------------------------------------------------------------
typedef struct {
    // Line instances (CPU staging)
    geom_line_instance_t *line_data;
    int line_count;
    int line_capacity;
    sg_buffer line_instance_buf;

    // Point instances (CPU staging)
    geom_point_instance_t *point_data;
    int point_count;
    int point_capacity;
    sg_buffer point_instance_buf;

    // Template geometry (shared for lines)
    sg_buffer line_template_vbuf;
    sg_buffer line_template_ibuf;
    int line_template_vertex_count;
    int line_template_index_count;

    // Template geometry (shared for points)
    sg_buffer point_template_vbuf;
    sg_buffer point_template_ibuf;
    int point_template_vertex_count;
    int point_template_index_count;

    // Pipelines (depth-always, no depth write)
    sg_pipeline line_pip;
    sg_shader line_shd;
    sg_pipeline point_pip;
    sg_shader point_shd;

    bool initialized;
} gizmo_rendering_t;

//------------------------------------------------------------------------------
// Init
//------------------------------------------------------------------------------
static inline void gizmo_rendering_init(gizmo_rendering_t *gr) {
    memset(gr, 0, sizeof(*gr));
    gr->line_capacity = GIZMO_MAX_LINES;
    gr->point_capacity = GIZMO_MAX_POINTS;
    gr->line_data = (geom_line_instance_t*)calloc(gr->line_capacity, sizeof(geom_line_instance_t));
    gr->point_data = (geom_point_instance_t*)calloc(gr->point_capacity, sizeof(geom_point_instance_t));

    // --- Line template geometry ---
    {
        int max_v = 4 + 2 * (GEOM_BATCH_CAP_SEGMENTS + 2);
        int max_i = 6 + 2 * GEOM_BATCH_CAP_SEGMENTS * 3;
        geom_template_vertex_t *verts = (geom_template_vertex_t*)malloc(max_v * sizeof(geom_template_vertex_t));
        uint16_t *idxs = (uint16_t*)malloc(max_i * sizeof(uint16_t));
        geom_batch_generate_line_template(verts, &gr->line_template_vertex_count,
                                          idxs, &gr->line_template_index_count,
                                          GEOM_BATCH_CAP_SEGMENTS);
        gr->line_template_vbuf = sg_make_buffer(&(sg_buffer_desc){
            .usage.vertex_buffer = true,
            .data = { .ptr = verts, .size = gr->line_template_vertex_count * sizeof(geom_template_vertex_t) },
            .label = "gizmo-line-template-vbuf"
        });
        gr->line_template_ibuf = sg_make_buffer(&(sg_buffer_desc){
            .usage.index_buffer = true,
            .data = { .ptr = idxs, .size = gr->line_template_index_count * sizeof(uint16_t) },
            .label = "gizmo-line-template-ibuf"
        });
        free(verts);
        free(idxs);
    }

    // --- Point template geometry ---
    {
        int max_v = 2 + GEOM_BATCH_CIRCLE_SEGMENTS;
        int max_i = GEOM_BATCH_CIRCLE_SEGMENTS * 3;
        geom_template_vertex_t *verts = (geom_template_vertex_t*)malloc(max_v * sizeof(geom_template_vertex_t));
        uint16_t *idxs = (uint16_t*)malloc(max_i * sizeof(uint16_t));
        geom_batch_generate_point_template(verts, &gr->point_template_vertex_count,
                                           idxs, &gr->point_template_index_count,
                                           GEOM_BATCH_CIRCLE_SEGMENTS);
        gr->point_template_vbuf = sg_make_buffer(&(sg_buffer_desc){
            .usage.vertex_buffer = true,
            .data = { .ptr = verts, .size = gr->point_template_vertex_count * sizeof(geom_template_vertex_t) },
            .label = "gizmo-point-template-vbuf"
        });
        gr->point_template_ibuf = sg_make_buffer(&(sg_buffer_desc){
            .usage.index_buffer = true,
            .data = { .ptr = idxs, .size = gr->point_template_index_count * sizeof(uint16_t) },
            .label = "gizmo-point-template-ibuf"
        });
        free(verts);
        free(idxs);
    }

    // --- Line shader ---
#if defined(SOKOL_VULKAN)
    gr->line_shd = sg_make_shader(&(sg_shader_desc){
        .vertex_func = { .bytecode = SG_RANGE(instanced_line_vs_spirv), .entry = "main" },
        .fragment_func = { .bytecode = SG_RANGE(instanced_line_fs_spirv), .entry = "main" },
        .attrs = {
            [0] = { .hlsl_sem_name = "POSITION", .hlsl_sem_index = 0 },
            [1] = { .hlsl_sem_name = "TEXCOORD", .hlsl_sem_index = 0 },
            [2] = { .hlsl_sem_name = "TEXCOORD", .hlsl_sem_index = 1 },
            [3] = { .hlsl_sem_name = "COLOR",    .hlsl_sem_index = 0 },
        },
        .uniform_blocks[0] = {
            .stage = SG_SHADERSTAGE_VERTEX,
            .size = sizeof(geom_batch_params_t),
            .layout = SG_UNIFORMLAYOUT_STD140,
        },
        .label = "gizmo-line-shader"
    });
#else
    gr->line_shd = sg_make_shader(&(sg_shader_desc){
        .vertex_func = { .source = instanced_line_vs_source, .entry = "vs_main" },
        .fragment_func = { .source = instanced_line_fs_source, .entry = "fs_main" },
        .attrs = {
            [0] = { .hlsl_sem_name = "POSITION", .hlsl_sem_index = 0 },
            [1] = { .hlsl_sem_name = "TEXCOORD", .hlsl_sem_index = 0 },
            [2] = { .hlsl_sem_name = "TEXCOORD", .hlsl_sem_index = 1 },
            [3] = { .hlsl_sem_name = "COLOR",    .hlsl_sem_index = 0 },
        },
        .uniform_blocks[0] = {
            .stage = SG_SHADERSTAGE_VERTEX,
            .size = sizeof(geom_batch_params_t),
            .layout = SG_UNIFORMLAYOUT_STD140,
            .glsl_uniforms = {
                [0] = { .type = SG_UNIFORMTYPE_MAT4,  .glsl_name = "mvp" },
                [1] = { .type = SG_UNIFORMTYPE_FLOAT, .glsl_name = "line_width" },
                [2] = { .type = SG_UNIFORMTYPE_FLOAT, .glsl_name = "aspect_ratio" },
            }
        },
        .label = "gizmo-line-shader"
    });
#endif

    // --- Line pipeline (ALWAYS on top, no depth write) ---
    gr->line_pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = gr->line_shd,
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
            .compare = SG_COMPAREFUNC_ALWAYS,
            .write_enabled = false,
            .pixel_format = SG_PIXELFORMAT_DEPTH
        },
        .colors[0].pixel_format = SG_PIXELFORMAT_RGBA8,
        .cull_mode = SG_CULLMODE_NONE,
        .label = "gizmo-line-pipeline"
    });

    // --- Point shader ---
#if defined(SOKOL_VULKAN)
    gr->point_shd = sg_make_shader(&(sg_shader_desc){
        .vertex_func = { .bytecode = SG_RANGE(join_vs_spirv), .entry = "main" },
        .fragment_func = { .bytecode = SG_RANGE(join_fs_spirv), .entry = "main" },
        .attrs = {
            [0] = { .hlsl_sem_name = "POSITION", .hlsl_sem_index = 0 },
            [1] = { .hlsl_sem_name = "TEXCOORD", .hlsl_sem_index = 0 },
            [2] = { .hlsl_sem_name = "COLOR",    .hlsl_sem_index = 0 },
        },
        .uniform_blocks[0] = {
            .stage = SG_SHADERSTAGE_VERTEX,
            .size = sizeof(geom_batch_params_t),
            .layout = SG_UNIFORMLAYOUT_STD140,
        },
        .label = "gizmo-point-shader"
    });
#else
    gr->point_shd = sg_make_shader(&(sg_shader_desc){
        .vertex_func = { .source = join_vs_source, .entry = "vs_main" },
        .fragment_func = { .source = join_fs_source, .entry = "fs_main" },
        .attrs = {
            [0] = { .hlsl_sem_name = "POSITION", .hlsl_sem_index = 0 },
            [1] = { .hlsl_sem_name = "TEXCOORD", .hlsl_sem_index = 0 },
            [2] = { .hlsl_sem_name = "COLOR",    .hlsl_sem_index = 0 },
        },
        .uniform_blocks[0] = {
            .stage = SG_SHADERSTAGE_VERTEX,
            .size = sizeof(geom_batch_params_t),
            .layout = SG_UNIFORMLAYOUT_STD140,
            .glsl_uniforms = {
                [0] = { .type = SG_UNIFORMTYPE_MAT4,  .glsl_name = "mvp" },
                [1] = { .type = SG_UNIFORMTYPE_FLOAT, .glsl_name = "line_width" },
                [2] = { .type = SG_UNIFORMTYPE_FLOAT, .glsl_name = "aspect_ratio" },
            }
        },
        .label = "gizmo-point-shader"
    });
#endif

    // --- Point pipeline (ALWAYS on top, no depth write) ---
    gr->point_pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = gr->point_shd,
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
            .compare = SG_COMPAREFUNC_ALWAYS,
            .write_enabled = false,
            .pixel_format = SG_PIXELFORMAT_DEPTH
        },
        .colors[0].pixel_format = SG_PIXELFORMAT_RGBA8,
        .cull_mode = SG_CULLMODE_NONE,
        .label = "gizmo-point-pipeline"
    });

    // --- GPU instance buffers (stream usage for per-frame upload) ---
    gr->line_instance_buf = sg_make_buffer(&(sg_buffer_desc){
        .usage = { .vertex_buffer = true, .stream_update = true },
        .size = gr->line_capacity * sizeof(geom_line_instance_t),
        .label = "gizmo-line-instances"
    });

    gr->point_instance_buf = sg_make_buffer(&(sg_buffer_desc){
        .usage = { .vertex_buffer = true, .stream_update = true },
        .size = gr->point_capacity * sizeof(geom_point_instance_t),
        .label = "gizmo-point-instances"
    });

    gr->initialized = true;
}

//------------------------------------------------------------------------------
// Clear instance data (call at start of gizmo update)
//------------------------------------------------------------------------------
static inline void gizmo_rendering_clear(gizmo_rendering_t *gr) {
    gr->line_count = 0;
    gr->point_count = 0;
}

//------------------------------------------------------------------------------
// Add instances (returns slot index)
//------------------------------------------------------------------------------
static inline int gizmo_rendering_add_line(gizmo_rendering_t *gr,
                                            vec3_t a, vec3_t b,
                                            float r, float g, float b_col, float alpha) {
    if (gr->line_count >= gr->line_capacity) return -1;
    int slot = gr->line_count++;
    gr->line_data[slot] = (geom_line_instance_t){
        .ax = a.x, .ay = a.y, .az = a.z,
        .bx = b.x, .by = b.y, .bz = b.z,
        .r = r, .g = g, .b = b_col, .a = alpha
    };
    return slot;
}

static inline int gizmo_rendering_add_point(gizmo_rendering_t *gr,
                                             vec3_t center,
                                             float r, float g, float b, float alpha) {
    if (gr->point_count >= gr->point_capacity) return -1;
    int slot = gr->point_count++;
    gr->point_data[slot] = (geom_point_instance_t){
        .x = center.x, .y = center.y, .z = center.z,
        .r = r, .g = g, .b = b, .a = alpha
    };
    return slot;
}

//------------------------------------------------------------------------------
// Update existing instances in-place
//------------------------------------------------------------------------------
static inline void gizmo_rendering_set_line(gizmo_rendering_t *gr, int slot,
                                             vec3_t a, vec3_t b,
                                             float r, float g, float b_col, float alpha) {
    if (slot < 0 || slot >= gr->line_count) return;
    gr->line_data[slot] = (geom_line_instance_t){
        .ax = a.x, .ay = a.y, .az = a.z,
        .bx = b.x, .by = b.y, .bz = b.z,
        .r = r, .g = g, .b = b_col, .a = alpha
    };
}

static inline void gizmo_rendering_set_point(gizmo_rendering_t *gr, int slot,
                                              vec3_t center,
                                              float r, float g, float b, float alpha) {
    if (slot < 0 || slot >= gr->point_count) return;
    gr->point_data[slot] = (geom_point_instance_t){
        .x = center.x, .y = center.y, .z = center.z,
        .r = r, .g = g, .b = b, .a = alpha
    };
}

//------------------------------------------------------------------------------
// Upload + Draw
//------------------------------------------------------------------------------
static inline void gizmo_rendering_upload(gizmo_rendering_t *gr) {
    if (gr->line_count > 0) {
        sg_update_buffer(gr->line_instance_buf, &(sg_range){
            .ptr = gr->line_data,
            .size = gr->line_count * sizeof(geom_line_instance_t)
        });
    }
    if (gr->point_count > 0) {
        sg_update_buffer(gr->point_instance_buf, &(sg_range){
            .ptr = gr->point_data,
            .size = gr->point_count * sizeof(geom_point_instance_t)
        });
    }
}

static inline void gizmo_rendering_draw(gizmo_rendering_t *gr,
                                          mat4_t mvp, float aspect_ratio,
                                          float line_width, float point_size) {
    // Draw lines
    if (gr->line_count > 0) {
        geom_batch_params_t params = {
            .mvp = mvp,
            .line_width = line_width,
            .aspect_ratio = aspect_ratio,
        };
        sg_apply_pipeline(gr->line_pip);
        sg_apply_bindings(&(sg_bindings){
            .vertex_buffers = {
                [0] = gr->line_template_vbuf,
                [1] = gr->line_instance_buf,
            },
            .index_buffer = gr->line_template_ibuf,
        });
        sg_apply_uniforms(0, &SG_RANGE(params));
        sg_draw(0, gr->line_template_index_count, gr->line_count);
    }

    // Draw points
    if (gr->point_count > 0) {
        geom_batch_params_t params = {
            .mvp = mvp,
            .line_width = point_size,
            .aspect_ratio = aspect_ratio,
        };
        sg_apply_pipeline(gr->point_pip);
        sg_apply_bindings(&(sg_bindings){
            .vertex_buffers = {
                [0] = gr->point_template_vbuf,
                [1] = gr->point_instance_buf,
            },
            .index_buffer = gr->point_template_ibuf,
        });
        sg_apply_uniforms(0, &SG_RANGE(params));
        sg_draw(0, gr->point_template_index_count, gr->point_count);
    }
}

//------------------------------------------------------------------------------
// Shutdown
//------------------------------------------------------------------------------
static inline void gizmo_rendering_shutdown(gizmo_rendering_t *gr) {
    if (!gr->initialized) return;
    sg_destroy_buffer(gr->line_instance_buf);
    sg_destroy_buffer(gr->point_instance_buf);
    sg_destroy_pipeline(gr->line_pip);
    sg_destroy_shader(gr->line_shd);
    sg_destroy_pipeline(gr->point_pip);
    sg_destroy_shader(gr->point_shd);
    sg_destroy_buffer(gr->line_template_vbuf);
    sg_destroy_buffer(gr->line_template_ibuf);
    sg_destroy_buffer(gr->point_template_vbuf);
    sg_destroy_buffer(gr->point_template_ibuf);
    free(gr->line_data);
    free(gr->point_data);
    gr->initialized = false;
}

#endif // GIZMO_RENDERING_H
