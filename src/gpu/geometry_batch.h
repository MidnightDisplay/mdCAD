//------------------------------------------------------------------------------
// geometry_batch.h - Geometry batch manager for ECS rendering (header-only)
//
// Manages GPU batches for different geometry types, using instance_buffer.h
// for dynamic slot allocation and the existing instanced rendering infrastructure.
//------------------------------------------------------------------------------
#ifndef GEOMETRY_BATCH_H
#define GEOMETRY_BATCH_H

#include "../platform.h"
#include "sokol_gfx.h"
#include "../math3d.h"
#include "../shaders/instanced_line_shaders.h"
#include "../shaders/join_shaders.h"
#include "instance_buffer.h"
#include <math.h>

//------------------------------------------------------------------------------
// Configuration
//------------------------------------------------------------------------------
#define GEOM_BATCH_CAP_SEGMENTS 8        // Segments for line end caps
#define GEOM_BATCH_CIRCLE_SEGMENTS 16    // Segments for point circles
#define GEOM_BATCH_INITIAL_LINES 64
#define GEOM_BATCH_INITIAL_POINTS 64

//------------------------------------------------------------------------------
// Instance Data Types (matches shader expectations)
//------------------------------------------------------------------------------

// Line instance: two endpoints + color
typedef struct {
    float ax, ay, az;   // Point A position
    float bx, by, bz;   // Point B position
    float r, g, b, a;   // Color
} geom_line_instance_t;

// Point instance: center + color (uses circle join shader)
typedef struct {
    float x, y, z;      // Center position
    float r, g, b, a;   // Color
} geom_point_instance_t;

//------------------------------------------------------------------------------
// Template vertex (shared for lines and points)
//------------------------------------------------------------------------------
typedef struct {
    float x, y, z;
} geom_template_vertex_t;

//------------------------------------------------------------------------------
// Uniform block for line/point shaders
//------------------------------------------------------------------------------
typedef struct {
    mat4_t mvp;
    float line_width;
    float aspect_ratio;
    float _pad[2];
} geom_batch_params_t;

//------------------------------------------------------------------------------
// Line Batch
//------------------------------------------------------------------------------
typedef struct {
    // Instance buffer
    instance_buffer_t instances;

    // Template geometry (rectangle + semicircle caps)
    sg_buffer template_vbuf;
    sg_buffer template_ibuf;
    int template_vertex_count;
    int template_index_count;

    // Pipeline and shader
    sg_pipeline pip;
    sg_shader shd;

    // Rendering parameters
    float line_width;
} geom_line_batch_t;

//------------------------------------------------------------------------------
// Point Batch
//------------------------------------------------------------------------------
typedef struct {
    // Instance buffer
    instance_buffer_t instances;

    // Template geometry (circle)
    sg_buffer template_vbuf;
    sg_buffer template_ibuf;
    int template_vertex_count;
    int template_index_count;

    // Pipeline and shader
    sg_pipeline pip;
    sg_shader shd;

    // Rendering parameters
    float point_size;
} geom_point_batch_t;

//------------------------------------------------------------------------------
// Combined Batch Manager
//------------------------------------------------------------------------------
typedef struct {
    geom_line_batch_t lines;
    geom_point_batch_t points;
    // Future: polyline_batch, arc_batch, etc.
} geometry_batch_manager_t;

//------------------------------------------------------------------------------
// Line template geometry generation
//------------------------------------------------------------------------------
static inline void geom_batch_generate_line_template(
    geom_template_vertex_t* vertices, int* vertex_count,
    uint16_t* indices, int* index_count,
    int cap_segments
) {
    int vi = 0;
    int ii = 0;

    // Main rectangle body (4 vertices, 2 triangles)
    vertices[vi++] = (geom_template_vertex_t){ 0.0f, -0.5f, 0.0f };  // Bottom-left (A side)
    vertices[vi++] = (geom_template_vertex_t){ 0.0f,  0.5f, 0.0f };  // Top-left (A side)
    vertices[vi++] = (geom_template_vertex_t){ 0.0f, -0.5f, 1.0f };  // Bottom-right (B side)
    vertices[vi++] = (geom_template_vertex_t){ 0.0f,  0.5f, 1.0f };  // Top-right (B side)

    // Rectangle triangles
    indices[ii++] = 0; indices[ii++] = 2; indices[ii++] = 1;
    indices[ii++] = 1; indices[ii++] = 2; indices[ii++] = 3;

    // Left semicircle cap (at point A, z=0)
    int cap_a_center = vi;
    vertices[vi++] = (geom_template_vertex_t){ 0.0f, 0.0f, 0.0f };

    for (int i = 0; i <= cap_segments; i++) {
        float angle = 3.14159265359f * 0.5f + 3.14159265359f * (float)i / (float)cap_segments;
        float x = cosf(angle) * 0.5f;
        float y = sinf(angle) * 0.5f;
        vertices[vi++] = (geom_template_vertex_t){ x, y, 0.0f };
    }

    for (int i = 0; i < cap_segments; i++) {
        indices[ii++] = cap_a_center;
        indices[ii++] = cap_a_center + 1 + i;
        indices[ii++] = cap_a_center + 2 + i;
    }

    // Right semicircle cap (at point B, z=1)
    int cap_b_center = vi;
    vertices[vi++] = (geom_template_vertex_t){ 0.0f, 0.0f, 1.0f };

    for (int i = 0; i <= cap_segments; i++) {
        float angle = -3.14159265359f * 0.5f + 3.14159265359f * (float)i / (float)cap_segments;
        float x = cosf(angle) * 0.5f;
        float y = sinf(angle) * 0.5f;
        vertices[vi++] = (geom_template_vertex_t){ x, y, 1.0f };
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
// Point template geometry generation (full circle)
//------------------------------------------------------------------------------
static inline void geom_batch_generate_point_template(
    geom_template_vertex_t* vertices, int* vertex_count,
    uint16_t* indices, int* index_count,
    int segments
) {
    int vi = 0;
    int ii = 0;

    // Center vertex
    vertices[vi++] = (geom_template_vertex_t){ 0.0f, 0.0f, 0.0f };

    // Circle vertices
    for (int i = 0; i <= segments; i++) {
        float angle = 2.0f * 3.14159265359f * (float)i / (float)segments;
        float x = cosf(angle) * 0.5f;
        float y = sinf(angle) * 0.5f;
        vertices[vi++] = (geom_template_vertex_t){ x, y, 0.0f };
    }

    // Circle triangles (fan from center)
    for (int i = 0; i < segments; i++) {
        indices[ii++] = 0;          // Center
        indices[ii++] = 1 + i;      // Current
        indices[ii++] = 2 + i;      // Next
    }

    *vertex_count = vi;
    *index_count = ii;
}

//------------------------------------------------------------------------------
// Line Batch Functions
//------------------------------------------------------------------------------

static inline void geom_line_batch_init(geom_line_batch_t* batch) {
    batch->line_width = 0.03f;

    // Initialize instance buffer
    instance_buffer_init(&batch->instances,
                         sizeof(geom_line_instance_t),
                         GEOM_BATCH_INITIAL_LINES,
                         "ecs-line-instances");

    // Generate template geometry
    int max_vertices = 4 + 2 * (GEOM_BATCH_CAP_SEGMENTS + 2);
    int max_indices = 6 + 2 * GEOM_BATCH_CAP_SEGMENTS * 3;

    geom_template_vertex_t* template_vertices = (geom_template_vertex_t*)malloc(max_vertices * sizeof(geom_template_vertex_t));
    uint16_t* template_indices = (uint16_t*)malloc(max_indices * sizeof(uint16_t));

    geom_batch_generate_line_template(
        template_vertices, &batch->template_vertex_count,
        template_indices, &batch->template_index_count,
        GEOM_BATCH_CAP_SEGMENTS
    );

    // Create template buffers
    batch->template_vbuf = sg_make_buffer(&(sg_buffer_desc){
        .usage.vertex_buffer = true,
        .data = {
            .ptr = template_vertices,
            .size = batch->template_vertex_count * sizeof(geom_template_vertex_t)
        },
        .label = "ecs-line-template-vbuf"
    });

    batch->template_ibuf = sg_make_buffer(&(sg_buffer_desc){
        .usage.index_buffer = true,
        .data = {
            .ptr = template_indices,
            .size = batch->template_index_count * sizeof(uint16_t)
        },
        .label = "ecs-line-template-ibuf"
    });

    free(template_vertices);
    free(template_indices);

    // Create shader
    batch->shd = sg_make_shader(&(sg_shader_desc){
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
            .size = sizeof(geom_batch_params_t),
            .layout = SG_UNIFORMLAYOUT_STD140,
            .glsl_uniforms = {
                [0] = { .type = SG_UNIFORMTYPE_MAT4, .glsl_name = "mvp" },
                [1] = { .type = SG_UNIFORMTYPE_FLOAT, .glsl_name = "line_width" },
                [2] = { .type = SG_UNIFORMTYPE_FLOAT, .glsl_name = "aspect_ratio" },
            }
        },
        .label = "ecs-line-shader"
    });

    // Create pipeline with instancing
    batch->pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = batch->shd,
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
        .cull_mode = SG_CULLMODE_NONE,
        .label = "ecs-line-pipeline"
    });
}

static inline int geom_line_batch_alloc(geom_line_batch_t* batch) {
    return instance_buffer_alloc_slot(&batch->instances);
}

// Allocate n contiguous slots (for polylines, arcs, etc.)
static inline int geom_line_batch_alloc_contiguous(geom_line_batch_t* batch, int n) {
    return instance_buffer_alloc_contiguous(&batch->instances, n);
}

static inline void geom_line_batch_free(geom_line_batch_t* batch, int slot) {
    instance_buffer_free_slot(&batch->instances, slot);
}

static inline void geom_line_batch_set(geom_line_batch_t* batch, int slot,
                                        vec3_t a, vec3_t b, vec4_t color) {
    geom_line_instance_t inst = {
        .ax = a.x, .ay = a.y, .az = a.z,
        .bx = b.x, .by = b.y, .bz = b.z,
        .r = color.x, .g = color.y, .b = color.z, .a = color.w
    };
    instance_buffer_set(&batch->instances, slot, &inst);
}

// Set entity mapping for a line slot (for debug viewer)
static inline void geom_line_batch_set_entity(geom_line_batch_t* batch, int slot,
                                               uint64_t entity_id, uint8_t geom_type) {
    instance_buffer_set_entity(&batch->instances, slot, entity_id, geom_type);
}

static inline void geom_line_batch_upload(geom_line_batch_t* batch) {
    instance_buffer_upload(&batch->instances);
}

static inline void geom_line_batch_draw(geom_line_batch_t* batch, mat4_t mvp, float aspect_ratio) {
    int count = instance_buffer_count(&batch->instances);
    if (count == 0) return;

    geom_batch_params_t params = {
        .mvp = mvp,
        .line_width = batch->line_width,
        .aspect_ratio = aspect_ratio,
    };

    sg_apply_pipeline(batch->pip);
    sg_apply_bindings(&(sg_bindings){
        .vertex_buffers = {
            [0] = batch->template_vbuf,
            [1] = instance_buffer_gpu_buffer(&batch->instances),
        },
        .index_buffer = batch->template_ibuf,
    });
    sg_apply_uniforms(0, &SG_RANGE(params));
    sg_draw(0, batch->template_index_count, count);
}

static inline void geom_line_batch_shutdown(geom_line_batch_t* batch) {
    instance_buffer_shutdown(&batch->instances);
    sg_destroy_pipeline(batch->pip);
    sg_destroy_shader(batch->shd);
    sg_destroy_buffer(batch->template_vbuf);
    sg_destroy_buffer(batch->template_ibuf);
}

//------------------------------------------------------------------------------
// Point Batch Functions
//------------------------------------------------------------------------------

static inline void geom_point_batch_init(geom_point_batch_t* batch) {
    batch->point_size = 0.05f;

    // Initialize instance buffer
    instance_buffer_init(&batch->instances,
                         sizeof(geom_point_instance_t),
                         GEOM_BATCH_INITIAL_POINTS,
                         "ecs-point-instances");

    // Generate template geometry (circle)
    int max_vertices = 2 + GEOM_BATCH_CIRCLE_SEGMENTS;
    int max_indices = GEOM_BATCH_CIRCLE_SEGMENTS * 3;

    geom_template_vertex_t* template_vertices = (geom_template_vertex_t*)malloc(max_vertices * sizeof(geom_template_vertex_t));
    uint16_t* template_indices = (uint16_t*)malloc(max_indices * sizeof(uint16_t));

    geom_batch_generate_point_template(
        template_vertices, &batch->template_vertex_count,
        template_indices, &batch->template_index_count,
        GEOM_BATCH_CIRCLE_SEGMENTS
    );

    // Create template buffers
    batch->template_vbuf = sg_make_buffer(&(sg_buffer_desc){
        .usage.vertex_buffer = true,
        .data = {
            .ptr = template_vertices,
            .size = batch->template_vertex_count * sizeof(geom_template_vertex_t)
        },
        .label = "ecs-point-template-vbuf"
    });

    batch->template_ibuf = sg_make_buffer(&(sg_buffer_desc){
        .usage.index_buffer = true,
        .data = {
            .ptr = template_indices,
            .size = batch->template_index_count * sizeof(uint16_t)
        },
        .label = "ecs-point-template-ibuf"
    });

    free(template_vertices);
    free(template_indices);

    // Create shader (uses join shader which works for circles)
    batch->shd = sg_make_shader(&(sg_shader_desc){
        .vertex_func = {
            .source = join_vs_source,
            .entry = "vs_main",
        },
        .fragment_func = {
            .source = join_fs_source,
            .entry = "fs_main",
        },
        .uniform_blocks[0] = {
            .stage = SG_SHADERSTAGE_VERTEX,
            .size = sizeof(geom_batch_params_t),
            .layout = SG_UNIFORMLAYOUT_STD140,
            .glsl_uniforms = {
                [0] = { .type = SG_UNIFORMTYPE_MAT4, .glsl_name = "mvp" },
                [1] = { .type = SG_UNIFORMTYPE_FLOAT, .glsl_name = "line_width" },
                [2] = { .type = SG_UNIFORMTYPE_FLOAT, .glsl_name = "aspect_ratio" },
            }
        },
        .label = "ecs-point-shader"
    });

    // Create pipeline with instancing
    batch->pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = batch->shd,
        .layout = {
            .buffers = {
                [0] = { .step_func = SG_VERTEXSTEP_PER_VERTEX },
                [1] = { .step_func = SG_VERTEXSTEP_PER_INSTANCE },
            },
            .attrs = {
                [0] = { .buffer_index = 0, .format = SG_VERTEXFORMAT_FLOAT3 },  // template_pos
                [1] = { .buffer_index = 1, .format = SG_VERTEXFORMAT_FLOAT3, .offset = 0 },  // center
                [2] = { .buffer_index = 1, .format = SG_VERTEXFORMAT_FLOAT4, .offset = 12 }, // color
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
        .label = "ecs-point-pipeline"
    });
}

static inline int geom_point_batch_alloc(geom_point_batch_t* batch) {
    return instance_buffer_alloc_slot(&batch->instances);
}

// Allocate n contiguous slots (for polyline joins, polygon vertices, etc.)
static inline int geom_point_batch_alloc_contiguous(geom_point_batch_t* batch, int n) {
    return instance_buffer_alloc_contiguous(&batch->instances, n);
}

static inline void geom_point_batch_free(geom_point_batch_t* batch, int slot) {
    instance_buffer_free_slot(&batch->instances, slot);
}

static inline void geom_point_batch_set(geom_point_batch_t* batch, int slot,
                                         vec3_t center, vec4_t color) {
    geom_point_instance_t inst = {
        .x = center.x, .y = center.y, .z = center.z,
        .r = color.x, .g = color.y, .b = color.z, .a = color.w
    };
    instance_buffer_set(&batch->instances, slot, &inst);
}

// Set entity mapping for a point slot (for debug viewer)
static inline void geom_point_batch_set_entity(geom_point_batch_t* batch, int slot,
                                                uint64_t entity_id, uint8_t geom_type) {
    instance_buffer_set_entity(&batch->instances, slot, entity_id, geom_type);
}

static inline void geom_point_batch_upload(geom_point_batch_t* batch) {
    instance_buffer_upload(&batch->instances);
}

static inline void geom_point_batch_draw(geom_point_batch_t* batch, mat4_t mvp, float aspect_ratio) {
    int count = instance_buffer_count(&batch->instances);
    if (count == 0) return;

    geom_batch_params_t params = {
        .mvp = mvp,
        .line_width = batch->point_size,  // Reuse line_width uniform for point size
        .aspect_ratio = aspect_ratio,
    };

    sg_apply_pipeline(batch->pip);
    sg_apply_bindings(&(sg_bindings){
        .vertex_buffers = {
            [0] = batch->template_vbuf,
            [1] = instance_buffer_gpu_buffer(&batch->instances),
        },
        .index_buffer = batch->template_ibuf,
    });
    sg_apply_uniforms(0, &SG_RANGE(params));
    sg_draw(0, batch->template_index_count, count);
}

static inline void geom_point_batch_shutdown(geom_point_batch_t* batch) {
    instance_buffer_shutdown(&batch->instances);
    sg_destroy_pipeline(batch->pip);
    sg_destroy_shader(batch->shd);
    sg_destroy_buffer(batch->template_vbuf);
    sg_destroy_buffer(batch->template_ibuf);
}

//------------------------------------------------------------------------------
// Batch Manager Functions
//------------------------------------------------------------------------------

static inline void geometry_batch_manager_init(geometry_batch_manager_t* mgr) {
    geom_line_batch_init(&mgr->lines);
    geom_point_batch_init(&mgr->points);
}

static inline void geometry_batch_manager_upload(geometry_batch_manager_t* mgr) {
    geom_line_batch_upload(&mgr->lines);
    geom_point_batch_upload(&mgr->points);
}

static inline void geometry_batch_manager_draw(geometry_batch_manager_t* mgr,
                                                mat4_t mvp, float aspect_ratio) {
    geom_line_batch_draw(&mgr->lines, mvp, aspect_ratio);
    geom_point_batch_draw(&mgr->points, mvp, aspect_ratio);
}

static inline void geometry_batch_manager_shutdown(geometry_batch_manager_t* mgr) {
    geom_line_batch_shutdown(&mgr->lines);
    geom_point_batch_shutdown(&mgr->points);
}

#endif // GEOMETRY_BATCH_H
