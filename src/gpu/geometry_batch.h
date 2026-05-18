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
#include "../shaders/instanced_triangle_shaders.h"
#include "instance_buffer.h"
#include "../components/geometry_comp.h"
#include <math.h>
#include <stdio.h>  // For debug printf

//------------------------------------------------------------------------------
// Configuration
//------------------------------------------------------------------------------
#define GEOM_BATCH_CAP_SEGMENTS 8        // Segments for line end caps
#define GEOM_BATCH_CIRCLE_SEGMENTS 16    // Segments for point circles
#define GEOM_BATCH_INITIAL_LINES 64
#define GEOM_BATCH_INITIAL_POINTS 64
#define GEOM_BATCH_INITIAL_TRIANGLES 64

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

// Triangle instance: three vertices + normal + three colors
typedef struct {
    float ax, ay, az;    // Vertex A position
    float bx, by, bz;    // Vertex B position
    float cx, cy, cz;    // Vertex C position
    float nx, ny, nz;    // Face normal
    float ra, ga, ba, aa; // Color at vertex A
    float rb, gb, bb, ab; // Color at vertex B
    float rc, gc, bc, ac; // Color at vertex C
} geom_triangle_instance_t;  // 24 floats = 96 bytes

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

// Maximum lights supported by triangle shader
#define TRIANGLE_MAX_LIGHTS 4

// Uniform block for triangle shaders (std140 layout, VS block 0)
// Contains MVP + lighting data. Lighting is computed in vertex shader
// (valid for flat shading since face normal is constant per triangle).
typedef struct {
    mat4_t mvp;                                    // 64 bytes, offset 0
    float light_dirs[TRIANGLE_MAX_LIGHTS * 4];     // 64 bytes, offset 64  (4 x vec4: xyz=dir/pos, w=type)
    float light_colors[TRIANGLE_MAX_LIGHTS * 4];   // 64 bytes, offset 128 (4 x vec4: rgb=color, w=intensity)
    float ambient[4];                              // 16 bytes, offset 192 (vec4: rgb=color, w=intensity)
    float num_lights;                              // 4 bytes,  offset 208
    float lighting_enabled;                        // 4 bytes,  offset 212
    float _pad[2];                                 // 8 bytes,  offset 216 (pad to 224 = 14*16)
} geom_triangle_params_t;                          // 224 bytes total

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
// Triangle Batch
//------------------------------------------------------------------------------
typedef struct {
    // Instance buffer
    instance_buffer_t instances;

    // Template geometry (3 vertices with barycentric selectors)
    sg_buffer template_vbuf;
    sg_buffer template_ibuf;
    int template_vertex_count;
    int template_index_count;

    // Pipeline and shader
    sg_pipeline pip;
    sg_shader shd;
} geom_triangle_batch_t;

//------------------------------------------------------------------------------
// Combined Batch Manager
//------------------------------------------------------------------------------
typedef struct {
    geom_line_batch_t lines;
    geom_point_batch_t points;
    geom_triangle_batch_t triangles;
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
    batch->line_width = 0.003f;

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
#if defined(SOKOL_VULKAN)
    batch->shd = sg_make_shader(&(sg_shader_desc){
        .vertex_func = {
            .bytecode = SG_RANGE(instanced_line_vs_spirv),
            .entry = "main",
        },
        .fragment_func = {
            .bytecode = SG_RANGE(instanced_line_fs_spirv),
            .entry = "main",
        },
        .attrs = {
            [0] = { .hlsl_sem_name = "POSITION", .hlsl_sem_index = 0 },   // template_pos
            [1] = { .hlsl_sem_name = "TEXCOORD", .hlsl_sem_index = 0 },   // point_a
            [2] = { .hlsl_sem_name = "TEXCOORD", .hlsl_sem_index = 1 },   // point_b
            [3] = { .hlsl_sem_name = "COLOR", .hlsl_sem_index = 0 },      // color
        },
        .uniform_blocks[0] = {
            .stage = SG_SHADERSTAGE_VERTEX,
            .size = sizeof(geom_batch_params_t),
            .layout = SG_UNIFORMLAYOUT_STD140,
        },
        .label = "ecs-line-shader"
    });
#else
    batch->shd = sg_make_shader(&(sg_shader_desc){
        .vertex_func = {
            .source = instanced_line_vs_source,
            .entry = "vs_main",
        },
        .fragment_func = {
            .source = instanced_line_fs_source,
            .entry = "fs_main",
        },
        .attrs = {
            [0] = { .hlsl_sem_name = "POSITION", .hlsl_sem_index = 0 },   // template_pos
            [1] = { .hlsl_sem_name = "TEXCOORD", .hlsl_sem_index = 0 },   // point_a
            [2] = { .hlsl_sem_name = "TEXCOORD", .hlsl_sem_index = 1 },   // point_b
            [3] = { .hlsl_sem_name = "COLOR", .hlsl_sem_index = 0 },      // color
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
#endif

#if defined(_DEBUG) || defined(DEBUG)
    printf("[DEBUG] Line shader state: %d (2=valid, 4=failed)\n", sg_query_shader_state(batch->shd));
#endif

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
        .sample_count = MDCAD_VIEWPORT_MSAA_SAMPLES,
        .colors[0].pixel_format = SG_PIXELFORMAT_RGBA8,
        .cull_mode = SG_CULLMODE_NONE,
        .label = "ecs-line-pipeline"
    });

#if defined(_DEBUG) || defined(DEBUG)
    printf("[D3D11 DEBUG] Line pipeline state: %d (2=valid, 4=failed)\n", sg_query_pipeline_state(batch->pip));
#endif
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
    batch->point_size = 0.001f;

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
#if defined(SOKOL_VULKAN)
    batch->shd = sg_make_shader(&(sg_shader_desc){
        .vertex_func = {
            .bytecode = SG_RANGE(join_vs_spirv),
            .entry = "main",
        },
        .fragment_func = {
            .bytecode = SG_RANGE(join_fs_spirv),
            .entry = "main",
        },
        .attrs = {
            [0] = { .hlsl_sem_name = "POSITION", .hlsl_sem_index = 0 },   // template_pos
            [1] = { .hlsl_sem_name = "TEXCOORD", .hlsl_sem_index = 0 },   // center
            [2] = { .hlsl_sem_name = "COLOR", .hlsl_sem_index = 0 },      // color
        },
        .uniform_blocks[0] = {
            .stage = SG_SHADERSTAGE_VERTEX,
            .size = sizeof(geom_batch_params_t),
            .layout = SG_UNIFORMLAYOUT_STD140,
        },
        .label = "ecs-point-shader"
    });
#else
    batch->shd = sg_make_shader(&(sg_shader_desc){
        .vertex_func = {
            .source = join_vs_source,
            .entry = "vs_main",
        },
        .fragment_func = {
            .source = join_fs_source,
            .entry = "fs_main",
        },
        .attrs = {
            [0] = { .hlsl_sem_name = "POSITION", .hlsl_sem_index = 0 },   // template_pos
            [1] = { .hlsl_sem_name = "TEXCOORD", .hlsl_sem_index = 0 },   // center
            [2] = { .hlsl_sem_name = "COLOR", .hlsl_sem_index = 0 },      // color
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
#endif

#if defined(_DEBUG) || defined(DEBUG)
    printf("[D3D11 DEBUG] Point shader state: %d (2=valid, 4=failed)\n", sg_query_shader_state(batch->shd));
#endif

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
        .sample_count = MDCAD_VIEWPORT_MSAA_SAMPLES,
        .colors[0].pixel_format = SG_PIXELFORMAT_RGBA8,
        .cull_mode = SG_CULLMODE_NONE,
        .label = "ecs-point-pipeline"
    });

#if defined(_DEBUG) || defined(DEBUG)
    printf("[D3D11 DEBUG] Point pipeline state: %d (2=valid, 4=failed)\n", sg_query_pipeline_state(batch->pip));
#endif
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
// Triangle Batch Functions
//------------------------------------------------------------------------------

// Compute face normal from triangle vertices: normalize(cross(b-a, c-a))
static inline vec3_t geom_triangle_compute_normal(vec3_t a, vec3_t b, vec3_t c) {
    vec3_t ab = vec3_sub(b, a);
    vec3_t ac = vec3_sub(c, a);
    vec3_t n = vec3_cross(ab, ac);
    float len = sqrtf(n.x * n.x + n.y * n.y + n.z * n.z);
    if (len < 1e-10f) return vec3_make(0.0f, 1.0f, 0.0f);
    return vec3_scale(n, 1.0f / len);
}

// Triangle template: 3 vertices with barycentric selectors
static inline void geom_batch_generate_triangle_template(
    geom_template_vertex_t* vertices, int* vertex_count,
    uint16_t* indices, int* index_count
) {
    // Vertex 0: selector (1,0,0) -> maps to instance vertex A
    vertices[0] = (geom_template_vertex_t){ 1.0f, 0.0f, 0.0f };
    // Vertex 1: selector (0,1,0) -> maps to instance vertex B
    vertices[1] = (geom_template_vertex_t){ 0.0f, 1.0f, 0.0f };
    // Vertex 2: selector (0,0,1) -> maps to instance vertex C
    vertices[2] = (geom_template_vertex_t){ 0.0f, 0.0f, 1.0f };

    indices[0] = 0;
    indices[1] = 1;
    indices[2] = 2;

    *vertex_count = 3;
    *index_count = 3;
}

static inline void geom_triangle_batch_init(geom_triangle_batch_t* batch) {
    // Initialize instance buffer
    instance_buffer_init(&batch->instances,
                         sizeof(geom_triangle_instance_t),
                         GEOM_BATCH_INITIAL_TRIANGLES,
                         "ecs-triangle-instances");

    // Generate template geometry (3 barycentric selector vertices)
    geom_template_vertex_t template_vertices[3];
    uint16_t template_indices[3];

    geom_batch_generate_triangle_template(
        template_vertices, &batch->template_vertex_count,
        template_indices, &batch->template_index_count
    );

    // Create template buffers
    batch->template_vbuf = sg_make_buffer(&(sg_buffer_desc){
        .usage.vertex_buffer = true,
        .data = {
            .ptr = template_vertices,
            .size = batch->template_vertex_count * sizeof(geom_template_vertex_t)
        },
        .label = "ecs-triangle-template-vbuf"
    });

    batch->template_ibuf = sg_make_buffer(&(sg_buffer_desc){
        .usage.index_buffer = true,
        .data = {
            .ptr = template_indices,
            .size = batch->template_index_count * sizeof(uint16_t)
        },
        .label = "ecs-triangle-template-ibuf"
    });

    // Create shader
#if defined(SOKOL_VULKAN)
    batch->shd = sg_make_shader(&(sg_shader_desc){
        .vertex_func = {
            .bytecode = SG_RANGE(instanced_triangle_vs_spirv),
            .entry = "main",
        },
        .fragment_func = {
            .bytecode = SG_RANGE(instanced_triangle_fs_spirv),
            .entry = "main",
        },
        .attrs = {
            [0] = { .hlsl_sem_name = "POSITION", .hlsl_sem_index = 0 },   // template_pos
            [1] = { .hlsl_sem_name = "TEXCOORD", .hlsl_sem_index = 0 },   // vertex_a
            [2] = { .hlsl_sem_name = "TEXCOORD", .hlsl_sem_index = 1 },   // vertex_b
            [3] = { .hlsl_sem_name = "TEXCOORD", .hlsl_sem_index = 2 },   // vertex_c
            [4] = { .hlsl_sem_name = "TEXCOORD", .hlsl_sem_index = 3 },   // normal
            [5] = { .hlsl_sem_name = "TEXCOORD", .hlsl_sem_index = 4 },   // color_a
            [6] = { .hlsl_sem_name = "TEXCOORD", .hlsl_sem_index = 5 },   // color_b
            [7] = { .hlsl_sem_name = "TEXCOORD", .hlsl_sem_index = 6 },   // color_c
        },
        .uniform_blocks[0] = {
            .stage = SG_SHADERSTAGE_VERTEX,
            .size = sizeof(geom_triangle_params_t),
            .layout = SG_UNIFORMLAYOUT_STD140,
        },
        .label = "ecs-triangle-shader"
    });
#else
    batch->shd = sg_make_shader(&(sg_shader_desc){
        .vertex_func = {
            .source = instanced_triangle_vs_source,
            .entry = "vs_main",
        },
        .fragment_func = {
            .source = instanced_triangle_fs_source,
            .entry = "fs_main",
        },
        .attrs = {
            [0] = { .hlsl_sem_name = "POSITION", .hlsl_sem_index = 0 },   // template_pos
            [1] = { .hlsl_sem_name = "TEXCOORD", .hlsl_sem_index = 0 },   // vertex_a
            [2] = { .hlsl_sem_name = "TEXCOORD", .hlsl_sem_index = 1 },   // vertex_b
            [3] = { .hlsl_sem_name = "TEXCOORD", .hlsl_sem_index = 2 },   // vertex_c
            [4] = { .hlsl_sem_name = "TEXCOORD", .hlsl_sem_index = 3 },   // normal
            [5] = { .hlsl_sem_name = "TEXCOORD", .hlsl_sem_index = 4 },   // color_a
            [6] = { .hlsl_sem_name = "TEXCOORD", .hlsl_sem_index = 5 },   // color_b
            [7] = { .hlsl_sem_name = "TEXCOORD", .hlsl_sem_index = 6 },   // color_c
        },
        .uniform_blocks[0] = {
            .stage = SG_SHADERSTAGE_VERTEX,
            .size = sizeof(geom_triangle_params_t),
            .layout = SG_UNIFORMLAYOUT_STD140,
            .glsl_uniforms = {
                [0] = { .type = SG_UNIFORMTYPE_MAT4, .glsl_name = "mvp" },
                [1] = { .type = SG_UNIFORMTYPE_FLOAT4, .array_count = TRIANGLE_MAX_LIGHTS, .glsl_name = "light_dirs" },
                [2] = { .type = SG_UNIFORMTYPE_FLOAT4, .array_count = TRIANGLE_MAX_LIGHTS, .glsl_name = "light_colors" },
                [3] = { .type = SG_UNIFORMTYPE_FLOAT4, .glsl_name = "ambient_color" },
                [4] = { .type = SG_UNIFORMTYPE_FLOAT, .glsl_name = "num_lights" },
                [5] = { .type = SG_UNIFORMTYPE_FLOAT, .glsl_name = "lighting_enabled" },
            }
        },
        .label = "ecs-triangle-shader"
    });
#endif

#if defined(_DEBUG) || defined(DEBUG)
    printf("[DEBUG] Triangle shader state: %d (2=valid, 4=failed)\n", sg_query_shader_state(batch->shd));
#endif

    // Create pipeline with instancing
    batch->pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = batch->shd,
        .layout = {
            .buffers = {
                [0] = { .step_func = SG_VERTEXSTEP_PER_VERTEX },
                [1] = { .step_func = SG_VERTEXSTEP_PER_INSTANCE },
            },
            .attrs = {
                [0] = { .buffer_index = 0, .format = SG_VERTEXFORMAT_FLOAT3 },                  // template_pos
                [1] = { .buffer_index = 1, .format = SG_VERTEXFORMAT_FLOAT3, .offset = 0 },     // vertex_a
                [2] = { .buffer_index = 1, .format = SG_VERTEXFORMAT_FLOAT3, .offset = 12 },    // vertex_b
                [3] = { .buffer_index = 1, .format = SG_VERTEXFORMAT_FLOAT3, .offset = 24 },    // vertex_c
                [4] = { .buffer_index = 1, .format = SG_VERTEXFORMAT_FLOAT3, .offset = 36 },    // normal
                [5] = { .buffer_index = 1, .format = SG_VERTEXFORMAT_FLOAT4, .offset = 48 },    // color_a
                [6] = { .buffer_index = 1, .format = SG_VERTEXFORMAT_FLOAT4, .offset = 64 },    // color_b
                [7] = { .buffer_index = 1, .format = SG_VERTEXFORMAT_FLOAT4, .offset = 80 },    // color_c
            }
        },
        .index_type = SG_INDEXTYPE_UINT16,
        .primitive_type = SG_PRIMITIVETYPE_TRIANGLES,
        .depth = {
            .compare = SG_COMPAREFUNC_LESS_EQUAL,
            .write_enabled = true,
            .pixel_format = SG_PIXELFORMAT_DEPTH
        },
        .sample_count = MDCAD_VIEWPORT_MSAA_SAMPLES,
        .colors[0].pixel_format = SG_PIXELFORMAT_RGBA8,
        .cull_mode = SG_CULLMODE_NONE,
        .label = "ecs-triangle-pipeline"
    });

#if defined(_DEBUG) || defined(DEBUG)
    printf("[DEBUG] Triangle pipeline state: %d (2=valid, 4=failed)\n", sg_query_pipeline_state(batch->pip));
#endif
}

static inline int geom_triangle_batch_alloc(geom_triangle_batch_t* batch) {
    return instance_buffer_alloc_slot(&batch->instances);
}

static inline int geom_triangle_batch_alloc_contiguous(geom_triangle_batch_t* batch, int n) {
    return instance_buffer_alloc_contiguous(&batch->instances, n);
}

static inline void geom_triangle_batch_free(geom_triangle_batch_t* batch, int slot) {
    instance_buffer_free_slot(&batch->instances, slot);
}

static inline void geom_triangle_batch_set(geom_triangle_batch_t* batch, int slot,
                                            vec3_t a, vec3_t b, vec3_t c,
                                            vec3_t normal, vec4_t color) {
    geom_triangle_instance_t inst = {
        .ax = a.x, .ay = a.y, .az = a.z,
        .bx = b.x, .by = b.y, .bz = b.z,
        .cx = c.x, .cy = c.y, .cz = c.z,
        .nx = normal.x, .ny = normal.y, .nz = normal.z,
        .ra = color.x, .ga = color.y, .ba = color.z, .aa = color.w,
        .rb = color.x, .gb = color.y, .bb = color.z, .ab = color.w,
        .rc = color.x, .gc = color.y, .bc = color.z, .ac = color.w,
    };
    instance_buffer_set(&batch->instances, slot, &inst);
}

static inline void geom_triangle_batch_set_colored(geom_triangle_batch_t* batch, int slot,
                                                     vec3_t a, vec3_t b, vec3_t c,
                                                     vec3_t normal,
                                                     vec4_t color_a, vec4_t color_b, vec4_t color_c) {
    geom_triangle_instance_t inst = {
        .ax = a.x, .ay = a.y, .az = a.z,
        .bx = b.x, .by = b.y, .bz = b.z,
        .cx = c.x, .cy = c.y, .cz = c.z,
        .nx = normal.x, .ny = normal.y, .nz = normal.z,
        .ra = color_a.x, .ga = color_a.y, .ba = color_a.z, .aa = color_a.w,
        .rb = color_b.x, .gb = color_b.y, .bb = color_b.z, .ab = color_b.w,
        .rc = color_c.x, .gc = color_c.y, .bc = color_c.z, .ac = color_c.w,
    };
    instance_buffer_set(&batch->instances, slot, &inst);
}

static inline void geom_triangle_batch_set_entity(geom_triangle_batch_t* batch, int slot,
                                                    uint64_t entity_id, uint8_t geom_type) {
    instance_buffer_set_entity(&batch->instances, slot, entity_id, geom_type);
}

static inline void geom_triangle_batch_upload(geom_triangle_batch_t* batch) {
    instance_buffer_upload(&batch->instances);
}

static inline void geom_triangle_batch_draw(geom_triangle_batch_t* batch,
                                              const geom_triangle_params_t *params) {
    int count = instance_buffer_count(&batch->instances);
    if (count == 0) return;

    sg_apply_pipeline(batch->pip);
    sg_apply_bindings(&(sg_bindings){
        .vertex_buffers = {
            [0] = batch->template_vbuf,
            [1] = instance_buffer_gpu_buffer(&batch->instances),
        },
        .index_buffer = batch->template_ibuf,
    });
    sg_apply_uniforms(0, &(sg_range){ .ptr = params, .size = sizeof(geom_triangle_params_t) });
    sg_draw(0, batch->template_index_count, count);
}

static inline void geom_triangle_batch_shutdown(geom_triangle_batch_t* batch) {
    instance_buffer_shutdown(&batch->instances);
    sg_destroy_pipeline(batch->pip);
    sg_destroy_shader(batch->shd);
    sg_destroy_buffer(batch->template_vbuf);
    sg_destroy_buffer(batch->template_ibuf);
}

//------------------------------------------------------------------------------
// Point Cloud Batch Helper Functions
// Point clouds use the point batch with contiguous slot allocation
//------------------------------------------------------------------------------

// Allocate contiguous slots for a point cloud
// Returns first slot index, or -1 if failed
static inline int geom_point_cloud_batch_alloc(geom_point_batch_t* batch, int point_count) {
    return geom_point_batch_alloc_contiguous(batch, point_count);
}

// Set point cloud data into contiguous slots
// first_slot: first slot from geom_point_cloud_batch_alloc
// points: array of positions
// colors: array of colors (can be NULL for uniform color)
// uniform_color: used when colors is NULL
// count: number of points
static inline void geom_point_cloud_batch_set(geom_point_batch_t* batch, int first_slot,
                                               const vec3_t* points, const vec4_t* colors,
                                               vec4_t uniform_color, int count) {
    for (int i = 0; i < count; i++) {
        vec4_t color = colors ? colors[i] : uniform_color;
        geom_point_batch_set(batch, first_slot + i, points[i], color);
    }
}

// Set entity mapping for all point cloud slots
static inline void geom_point_cloud_batch_set_entity(geom_point_batch_t* batch, int first_slot,
                                                      int count, uint64_t entity_id) {
    for (int i = 0; i < count; i++) {
        geom_point_batch_set_entity(batch, first_slot + i, entity_id, (uint8_t)GEOM_POINT_CLOUD);
    }
}

// Free all slots used by a point cloud
static inline void geom_point_cloud_batch_free(geom_point_batch_t* batch, int first_slot, int count) {
    for (int i = 0; i < count; i++) {
        geom_point_batch_free(batch, first_slot + i);
    }
}

//------------------------------------------------------------------------------
// Batch Manager Functions
//------------------------------------------------------------------------------

static inline void geometry_batch_manager_init(geometry_batch_manager_t* mgr) {
    geom_line_batch_init(&mgr->lines);
    geom_point_batch_init(&mgr->points);
    geom_triangle_batch_init(&mgr->triangles);
}

static inline void geometry_batch_manager_upload(geometry_batch_manager_t* mgr) {
    geom_line_batch_upload(&mgr->lines);
    geom_point_batch_upload(&mgr->points);
    geom_triangle_batch_upload(&mgr->triangles);
}

static inline void geometry_batch_manager_draw(geometry_batch_manager_t* mgr,
                                                const geom_triangle_params_t *tri_params,
                                                float aspect_ratio) {
    geom_triangle_batch_draw(&mgr->triangles, tri_params);
    geom_line_batch_draw(&mgr->lines, tri_params->mvp, aspect_ratio);
    geom_point_batch_draw(&mgr->points, tri_params->mvp, aspect_ratio);
}

static inline void geometry_batch_manager_shutdown(geometry_batch_manager_t* mgr) {
    geom_line_batch_shutdown(&mgr->lines);
    geom_point_batch_shutdown(&mgr->points);
    geom_triangle_batch_shutdown(&mgr->triangles);
}

#endif // GEOMETRY_BATCH_H
