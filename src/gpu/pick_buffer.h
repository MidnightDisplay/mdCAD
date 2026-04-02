//------------------------------------------------------------------------------
// pick_buffer.h - GPU picking system with CPU readback (header-only)
//
// Renders a small (20x20) offscreen buffer centered on the mouse cursor,
// with entities rendered using pick IDs encoded as RGB colors. The buffer
// is read back to CPU to determine which entity is under the cursor.
//------------------------------------------------------------------------------
#ifndef PICK_BUFFER_H
#define PICK_BUFFER_H

#include "../platform.h"
#include "sokol_gfx.h"
#include "../math3d.h"
#include "../math/math_interaction.h"
#include "../shaders/pick_shaders.h"
#include "../components/selectable_comp.h"
#include "instance_buffer.h"
#include "geometry_batch.h"
#include "pick_readback.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

//------------------------------------------------------------------------------
// Configuration
//------------------------------------------------------------------------------
#define PICK_BUFFER_SIZE 20           // 20x20 pixels (temporarily reverted for debugging)
#define PICK_BUFFER_LINE_WIDTH 0.005f  // Base line width (same as ECS lines)

//------------------------------------------------------------------------------
// Pick instance data types (similar to geometry_batch but with pick_color)
//------------------------------------------------------------------------------

// Line pick instance: two endpoints + pick color
typedef struct {
    float ax, ay, az;   // Point A position
    float bx, by, bz;   // Point B position
    float r, g, b;      // Pick color (encoded pick ID)
} pick_line_instance_t;

// Point pick instance: center + pick color
typedef struct {
    float x, y, z;      // Center position
    float r, g, b;      // Pick color (encoded pick ID)
} pick_point_instance_t;

// Triangle pick instance: three vertices + pick color
typedef struct {
    float ax, ay, az;   // Vertex A position
    float bx, by, bz;   // Vertex B position
    float cx, cy, cz;   // Vertex C position
    float r, g, b;      // Pick color (encoded pick ID)
} pick_triangle_instance_t;

//------------------------------------------------------------------------------
// Pick uniform block
//------------------------------------------------------------------------------
typedef struct {
    mat4_t mvp;
    float line_width;
    float aspect_ratio;
    float _pad[2];
} pick_params_t;

// Triangle pick uniform block (MVP only - triangles are solid geometry)
typedef struct {
    mat4_t mvp;
} pick_triangle_params_t;

//------------------------------------------------------------------------------
// Pick Buffer State
//------------------------------------------------------------------------------
typedef struct {
    // Render target
    sg_image color_img;
    sg_image depth_img;
    sg_view color_att_view;
    sg_view depth_att_view;
    sg_view tex_view;
    sg_sampler sampler;

    // Pick pipelines
    sg_pipeline line_pip;
    sg_shader line_shd;
    sg_pipeline point_pip;
    sg_shader point_shd;
    sg_pipeline triangle_pip;
    sg_shader triangle_shd;

    // Template geometry (reused from geometry_batch)
    sg_buffer line_template_vbuf;
    sg_buffer line_template_ibuf;
    int line_template_vertex_count;
    int line_template_index_count;

    sg_buffer point_template_vbuf;
    sg_buffer point_template_ibuf;
    int point_template_vertex_count;
    int point_template_index_count;

    sg_buffer triangle_template_vbuf;

    // Instance buffers for pick rendering
    instance_buffer_t line_instances;
    instance_buffer_t point_instances;
    instance_buffer_t triangle_instances;

    // Overlay pick (depth-always, renders on top — for gizmo handles)
    sg_pipeline overlay_line_pip;
    sg_pipeline overlay_point_pip;
    instance_buffer_t overlay_line_instances;
    instance_buffer_t overlay_point_instances;

    // CPU readback buffer
    uint8_t *pixel_data;

    // Pick state
    float center_x;             // Center position in viewport (0-1 normalized)
    float center_y;
    float viewport_width;       // Source viewport dimensions
    float viewport_height;
    float zoom_factor;          // Computed zoom factor for line width scaling
    uint32_t hovered_pick_id;   // Currently hovered entity's pick ID (0 = none)

    // Configurable parameters
    float thickness_multiplier; // Multiplier for line/point thickness (>= 1.0)

    // Debug visualization
    bool debug_enabled;

    // Tracks whether the pick pass has run at least once (image is valid for sampling)
    bool has_rendered;

    // Cursor-gated rebuild: skip pick buffer rebuild when nothing changed
    float prev_center_x;        // Previous cursor position (normalized 0-1)
    float prev_center_y;
    bool needs_rebuild;          // True when pick buffer must be rebuilt this frame
    bool first_frame;            // True until first successful render

    // Camera change detection (view+proj matrix hash)
    uint32_t prev_camera_hash;
} pick_buffer_t;

typedef enum {
    PICK_BUFFER_LAYER_BASE = 0,
    PICK_BUFFER_LAYER_OVERLAY = 1
} pick_buffer_layer_t;

typedef struct {
    uint32_t pick_id;
    pick_buffer_layer_t layer;
} pick_buffer_layer_event_t;

static inline void pick_buffer_layer_record_line(pick_buffer_layer_event_t *events,
                                                 int max_events,
                                                 int *io_count,
                                                 uint32_t pick_id) {
    if (!events || !io_count || max_events <= 0) return;
    if (*io_count < 0 || *io_count >= max_events) return;
    events[*io_count].pick_id = pick_id;
    events[*io_count].layer = PICK_BUFFER_LAYER_BASE;
    (*io_count)++;
}

static inline void pick_buffer_layer_record_overlay_point(pick_buffer_layer_event_t *events,
                                                          int max_events,
                                                          int *io_count,
                                                          uint32_t pick_id) {
    if (!events || !io_count || max_events <= 0) return;
    if (*io_count < 0 || *io_count >= max_events) return;
    events[*io_count].pick_id = pick_id;
    events[*io_count].layer = PICK_BUFFER_LAYER_OVERLAY;
    (*io_count)++;
}

static inline uint32_t pick_buffer_resolve_preferred_pick(const pick_buffer_layer_event_t *events,
                                                          int event_count) {
    if (!events || event_count <= 0) return 0;

    // Last overlay event wins; if there is no overlay, last base event wins.
    for (int i = event_count - 1; i >= 0; i--) {
        if (events[i].layer == PICK_BUFFER_LAYER_OVERLAY && events[i].pick_id != 0) {
            return events[i].pick_id;
        }
    }
    for (int i = event_count - 1; i >= 0; i--) {
        if (events[i].layer == PICK_BUFFER_LAYER_BASE && events[i].pick_id != 0) {
            return events[i].pick_id;
        }
    }
    return 0;
}

//------------------------------------------------------------------------------
// Internal: Generate template geometry
//------------------------------------------------------------------------------
static inline void pick_buffer_generate_line_template(
    geom_template_vertex_t* vertices, int* vertex_count,
    uint16_t* indices, int* index_count,
    int cap_segments
) {
    int vi = 0;
    int ii = 0;

    // Main rectangle body
    vertices[vi++] = (geom_template_vertex_t){ 0.0f, -0.5f, 0.0f };
    vertices[vi++] = (geom_template_vertex_t){ 0.0f,  0.5f, 0.0f };
    vertices[vi++] = (geom_template_vertex_t){ 0.0f, -0.5f, 1.0f };
    vertices[vi++] = (geom_template_vertex_t){ 0.0f,  0.5f, 1.0f };

    indices[ii++] = 0; indices[ii++] = 2; indices[ii++] = 1;
    indices[ii++] = 1; indices[ii++] = 2; indices[ii++] = 3;

    // Left semicircle cap
    int cap_a_center = vi;
    vertices[vi++] = (geom_template_vertex_t){ 0.0f, 0.0f, 0.0f };
    for (int i = 0; i <= cap_segments; i++) {
        float angle = 3.14159265359f * 0.5f + 3.14159265359f * (float)i / (float)cap_segments;
        vertices[vi++] = (geom_template_vertex_t){ cosf(angle) * 0.5f, sinf(angle) * 0.5f, 0.0f };
    }
    for (int i = 0; i < cap_segments; i++) {
        indices[ii++] = cap_a_center;
        indices[ii++] = cap_a_center + 1 + i;
        indices[ii++] = cap_a_center + 2 + i;
    }

    // Right semicircle cap
    int cap_b_center = vi;
    vertices[vi++] = (geom_template_vertex_t){ 0.0f, 0.0f, 1.0f };
    for (int i = 0; i <= cap_segments; i++) {
        float angle = -3.14159265359f * 0.5f + 3.14159265359f * (float)i / (float)cap_segments;
        vertices[vi++] = (geom_template_vertex_t){ cosf(angle) * 0.5f, sinf(angle) * 0.5f, 1.0f };
    }
    for (int i = 0; i < cap_segments; i++) {
        indices[ii++] = cap_b_center;
        indices[ii++] = cap_b_center + 1 + i;
        indices[ii++] = cap_b_center + 2 + i;
    }

    *vertex_count = vi;
    *index_count = ii;
}

static inline void pick_buffer_generate_point_template(
    geom_template_vertex_t* vertices, int* vertex_count,
    uint16_t* indices, int* index_count,
    int segments
) {
    int vi = 0;
    int ii = 0;

    vertices[vi++] = (geom_template_vertex_t){ 0.0f, 0.0f, 0.0f };
    for (int i = 0; i <= segments; i++) {
        float angle = 2.0f * 3.14159265359f * (float)i / (float)segments;
        vertices[vi++] = (geom_template_vertex_t){ cosf(angle) * 0.5f, sinf(angle) * 0.5f, 0.0f };
    }
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
static inline void pick_buffer_init(pick_buffer_t *pb) {
    memset(pb, 0, sizeof(*pb));

    pb->debug_enabled = false;
    pb->hovered_pick_id = 0;
    pb->zoom_factor = 1.0f;  // Default zoom factor
    pb->thickness_multiplier = 4.0f;  // Default: same thickness as visual rendering
    pb->needs_rebuild = true;
    pb->first_frame = true;
    pb->prev_center_x = -1.0f;
    pb->prev_center_y = -1.0f;
    pb->prev_camera_hash = 0;

    // Create render target
    pb->color_img = sg_make_image(&(sg_image_desc){
        .usage.color_attachment = true,
        .width = PICK_BUFFER_SIZE,
        .height = PICK_BUFFER_SIZE,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .sample_count = 1,
        .label = "pick-color"
    });

    pb->depth_img = sg_make_image(&(sg_image_desc){
        .usage.depth_stencil_attachment = true,
        .width = PICK_BUFFER_SIZE,
        .height = PICK_BUFFER_SIZE,
        .pixel_format = SG_PIXELFORMAT_DEPTH,
        .sample_count = 1,
        .label = "pick-depth"
    });

    pb->color_att_view = sg_make_view(&(sg_view_desc){
        .color_attachment.image = pb->color_img,
        .label = "pick-color-att-view"
    });

    pb->depth_att_view = sg_make_view(&(sg_view_desc){
        .depth_stencil_attachment.image = pb->depth_img,
        .label = "pick-depth-att-view"
    });

    pb->tex_view = sg_make_view(&(sg_view_desc){
        .texture.image = pb->color_img,
        .label = "pick-tex-view"
    });

    pb->sampler = sg_make_sampler(&(sg_sampler_desc){
        .min_filter = SG_FILTER_NEAREST,
        .mag_filter = SG_FILTER_NEAREST,
        .label = "pick-sampler"
    });

    // Allocate CPU readback buffer
    pb->pixel_data = (uint8_t*)malloc(PICK_BUFFER_SIZE * PICK_BUFFER_SIZE * 4);

    // Create line pick shader
#if defined(SOKOL_VULKAN)
    pb->line_shd = sg_make_shader(&(sg_shader_desc){
        .vertex_func = {
            .bytecode = SG_RANGE(pick_line_vs_spirv),
            .entry = "main",
        },
        .fragment_func = {
            .bytecode = SG_RANGE(pick_line_fs_spirv),
            .entry = "main",
        },
        .attrs = {
            [0] = { .hlsl_sem_name = "POSITION", .hlsl_sem_index = 0 },   // template_pos
            [1] = { .hlsl_sem_name = "TEXCOORD", .hlsl_sem_index = 0 },   // point_a
            [2] = { .hlsl_sem_name = "TEXCOORD", .hlsl_sem_index = 1 },   // point_b
            [3] = { .hlsl_sem_name = "COLOR", .hlsl_sem_index = 0 },      // pick_color
        },
        .uniform_blocks[0] = {
            .stage = SG_SHADERSTAGE_VERTEX,
            .size = sizeof(pick_params_t),
            .layout = SG_UNIFORMLAYOUT_STD140,
        },
        .label = "pick-line-shader"
    });
#else
    pb->line_shd = sg_make_shader(&(sg_shader_desc){
        .vertex_func = {
            .source = pick_line_vs_source,
            .entry = "vs_main",
        },
        .fragment_func = {
            .source = pick_line_fs_source,
            .entry = "fs_main",
        },
        .attrs = {
            [0] = { .hlsl_sem_name = "POSITION", .hlsl_sem_index = 0 },   // template_pos
            [1] = { .hlsl_sem_name = "TEXCOORD", .hlsl_sem_index = 0 },   // point_a
            [2] = { .hlsl_sem_name = "TEXCOORD", .hlsl_sem_index = 1 },   // point_b
            [3] = { .hlsl_sem_name = "COLOR", .hlsl_sem_index = 0 },      // pick_color
        },
        .uniform_blocks[0] = {
            .stage = SG_SHADERSTAGE_VERTEX,
            .size = sizeof(pick_params_t),
            .layout = SG_UNIFORMLAYOUT_STD140,
            .glsl_uniforms = {
                [0] = { .type = SG_UNIFORMTYPE_MAT4, .glsl_name = "mvp" },
                [1] = { .type = SG_UNIFORMTYPE_FLOAT, .glsl_name = "line_width" },
                [2] = { .type = SG_UNIFORMTYPE_FLOAT, .glsl_name = "aspect_ratio" },
            }
        },
        .label = "pick-line-shader"
    });
#endif

    // Create line pick pipeline
    pb->line_pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = pb->line_shd,
        .layout = {
            .buffers = {
                [0] = { .step_func = SG_VERTEXSTEP_PER_VERTEX },
                [1] = { .step_func = SG_VERTEXSTEP_PER_INSTANCE },
            },
            .attrs = {
                [0] = { .buffer_index = 0, .format = SG_VERTEXFORMAT_FLOAT3 },  // template_pos
                [1] = { .buffer_index = 1, .format = SG_VERTEXFORMAT_FLOAT3, .offset = 0 },   // point_a
                [2] = { .buffer_index = 1, .format = SG_VERTEXFORMAT_FLOAT3, .offset = 12 },  // point_b
                [3] = { .buffer_index = 1, .format = SG_VERTEXFORMAT_FLOAT3, .offset = 24 },  // pick_color
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
        .label = "pick-line-pipeline"
    });

    // Create point pick shader
#if defined(SOKOL_VULKAN)
    pb->point_shd = sg_make_shader(&(sg_shader_desc){
        .vertex_func = {
            .bytecode = SG_RANGE(pick_point_vs_spirv),
            .entry = "main",
        },
        .fragment_func = {
            .bytecode = SG_RANGE(pick_point_fs_spirv),
            .entry = "main",
        },
        .attrs = {
            [0] = { .hlsl_sem_name = "POSITION", .hlsl_sem_index = 0 },   // template_pos
            [1] = { .hlsl_sem_name = "TEXCOORD", .hlsl_sem_index = 0 },   // point
            [2] = { .hlsl_sem_name = "COLOR", .hlsl_sem_index = 0 },      // pick_color
        },
        .uniform_blocks[0] = {
            .stage = SG_SHADERSTAGE_VERTEX,
            .size = sizeof(pick_params_t),
            .layout = SG_UNIFORMLAYOUT_STD140,
        },
        .label = "pick-point-shader"
    });
#else
    pb->point_shd = sg_make_shader(&(sg_shader_desc){
        .vertex_func = {
            .source = pick_point_vs_source,
            .entry = "vs_main",
        },
        .fragment_func = {
            .source = pick_point_fs_source,
            .entry = "fs_main",
        },
        .attrs = {
            [0] = { .hlsl_sem_name = "POSITION", .hlsl_sem_index = 0 },   // template_pos
            [1] = { .hlsl_sem_name = "TEXCOORD", .hlsl_sem_index = 0 },   // point
            [2] = { .hlsl_sem_name = "COLOR", .hlsl_sem_index = 0 },      // pick_color
        },
        .uniform_blocks[0] = {
            .stage = SG_SHADERSTAGE_VERTEX,
            .size = sizeof(pick_params_t),
            .layout = SG_UNIFORMLAYOUT_STD140,
            .glsl_uniforms = {
                [0] = { .type = SG_UNIFORMTYPE_MAT4, .glsl_name = "mvp" },
                [1] = { .type = SG_UNIFORMTYPE_FLOAT, .glsl_name = "line_width" },
                [2] = { .type = SG_UNIFORMTYPE_FLOAT, .glsl_name = "aspect_ratio" },
            }
        },
        .label = "pick-point-shader"
    });
#endif

    // Create point pick pipeline
    pb->point_pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = pb->point_shd,
        .layout = {
            .buffers = {
                [0] = { .step_func = SG_VERTEXSTEP_PER_VERTEX },
                [1] = { .step_func = SG_VERTEXSTEP_PER_INSTANCE },
            },
            .attrs = {
                [0] = { .buffer_index = 0, .format = SG_VERTEXFORMAT_FLOAT3 },  // template_pos
                [1] = { .buffer_index = 1, .format = SG_VERTEXFORMAT_FLOAT3, .offset = 0 },   // point
                [2] = { .buffer_index = 1, .format = SG_VERTEXFORMAT_FLOAT3, .offset = 12 },  // pick_color
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
        .label = "pick-point-pipeline"
    });

    // Generate line template geometry
    int cap_segments = GEOM_BATCH_CAP_SEGMENTS;
    int max_line_vertices = 4 + 2 * (cap_segments + 2);
    int max_line_indices = 6 + 2 * cap_segments * 3;

    geom_template_vertex_t* line_vertices = (geom_template_vertex_t*)malloc(max_line_vertices * sizeof(geom_template_vertex_t));
    uint16_t* line_indices = (uint16_t*)malloc(max_line_indices * sizeof(uint16_t));

    pick_buffer_generate_line_template(
        line_vertices, &pb->line_template_vertex_count,
        line_indices, &pb->line_template_index_count,
        cap_segments
    );

    pb->line_template_vbuf = sg_make_buffer(&(sg_buffer_desc){
        .usage.vertex_buffer = true,
        .data = { .ptr = line_vertices, .size = pb->line_template_vertex_count * sizeof(geom_template_vertex_t) },
        .label = "pick-line-template-vbuf"
    });

    pb->line_template_ibuf = sg_make_buffer(&(sg_buffer_desc){
        .usage.index_buffer = true,
        .data = { .ptr = line_indices, .size = pb->line_template_index_count * sizeof(uint16_t) },
        .label = "pick-line-template-ibuf"
    });

    free(line_vertices);
    free(line_indices);

    // Generate point template geometry
    int circle_segments = GEOM_BATCH_CIRCLE_SEGMENTS;
    int max_point_vertices = 2 + circle_segments;
    int max_point_indices = circle_segments * 3;

    geom_template_vertex_t* point_vertices = (geom_template_vertex_t*)malloc(max_point_vertices * sizeof(geom_template_vertex_t));
    uint16_t* point_indices = (uint16_t*)malloc(max_point_indices * sizeof(uint16_t));

    pick_buffer_generate_point_template(
        point_vertices, &pb->point_template_vertex_count,
        point_indices, &pb->point_template_index_count,
        circle_segments
    );

    pb->point_template_vbuf = sg_make_buffer(&(sg_buffer_desc){
        .usage.vertex_buffer = true,
        .data = { .ptr = point_vertices, .size = pb->point_template_vertex_count * sizeof(geom_template_vertex_t) },
        .label = "pick-point-template-vbuf"
    });

    pb->point_template_ibuf = sg_make_buffer(&(sg_buffer_desc){
        .usage.index_buffer = true,
        .data = { .ptr = point_indices, .size = pb->point_template_index_count * sizeof(uint16_t) },
        .label = "pick-point-template-ibuf"
    });

    free(point_vertices);
    free(point_indices);

    // Create triangle pick shader
#if defined(SOKOL_VULKAN)
    pb->triangle_shd = sg_make_shader(&(sg_shader_desc){
        .vertex_func = {
            .bytecode = SG_RANGE(pick_triangle_vs_spirv),
            .entry = "main",
        },
        .fragment_func = {
            .bytecode = SG_RANGE(pick_triangle_fs_spirv),
            .entry = "main",
        },
        .attrs = {
            [0] = { .hlsl_sem_name = "POSITION", .hlsl_sem_index = 0 },   // template_pos
            [1] = { .hlsl_sem_name = "TEXCOORD", .hlsl_sem_index = 0 },   // vertex_a
            [2] = { .hlsl_sem_name = "TEXCOORD", .hlsl_sem_index = 1 },   // vertex_b
            [3] = { .hlsl_sem_name = "TEXCOORD", .hlsl_sem_index = 2 },   // vertex_c
            [4] = { .hlsl_sem_name = "COLOR", .hlsl_sem_index = 0 },      // pick_color
        },
        .uniform_blocks[0] = {
            .stage = SG_SHADERSTAGE_VERTEX,
            .size = sizeof(pick_triangle_params_t),
            .layout = SG_UNIFORMLAYOUT_STD140,
        },
        .label = "pick-triangle-shader"
    });
#else
    pb->triangle_shd = sg_make_shader(&(sg_shader_desc){
        .vertex_func = {
            .source = pick_triangle_vs_source,
            .entry = "vs_main",
        },
        .fragment_func = {
            .source = pick_triangle_fs_source,
            .entry = "fs_main",
        },
        .attrs = {
            [0] = { .hlsl_sem_name = "POSITION", .hlsl_sem_index = 0 },   // template_pos
            [1] = { .hlsl_sem_name = "TEXCOORD", .hlsl_sem_index = 0 },   // vertex_a
            [2] = { .hlsl_sem_name = "TEXCOORD", .hlsl_sem_index = 1 },   // vertex_b
            [3] = { .hlsl_sem_name = "TEXCOORD", .hlsl_sem_index = 2 },   // vertex_c
            [4] = { .hlsl_sem_name = "COLOR", .hlsl_sem_index = 0 },      // pick_color
        },
        .uniform_blocks[0] = {
            .stage = SG_SHADERSTAGE_VERTEX,
            .size = sizeof(pick_triangle_params_t),
            .layout = SG_UNIFORMLAYOUT_STD140,
            .glsl_uniforms = {
                [0] = { .type = SG_UNIFORMTYPE_MAT4, .glsl_name = "mvp" },
            }
        },
        .label = "pick-triangle-shader"
    });
#endif

    // Create triangle pick pipeline (no index buffer — 3 vertices drawn directly)
    pb->triangle_pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = pb->triangle_shd,
        .layout = {
            .buffers = {
                [0] = { .step_func = SG_VERTEXSTEP_PER_VERTEX },
                [1] = { .step_func = SG_VERTEXSTEP_PER_INSTANCE },
            },
            .attrs = {
                [0] = { .buffer_index = 0, .format = SG_VERTEXFORMAT_FLOAT3 },                     // template_pos
                [1] = { .buffer_index = 1, .format = SG_VERTEXFORMAT_FLOAT3, .offset = 0 },        // vertex_a
                [2] = { .buffer_index = 1, .format = SG_VERTEXFORMAT_FLOAT3, .offset = 12 },       // vertex_b
                [3] = { .buffer_index = 1, .format = SG_VERTEXFORMAT_FLOAT3, .offset = 24 },       // vertex_c
                [4] = { .buffer_index = 1, .format = SG_VERTEXFORMAT_FLOAT3, .offset = 36 },       // pick_color
            }
        },
        .primitive_type = SG_PRIMITIVETYPE_TRIANGLES,
        .depth = {
            .compare = SG_COMPAREFUNC_LESS_EQUAL,
            .write_enabled = true,
            .pixel_format = SG_PIXELFORMAT_DEPTH
        },
        .colors[0].pixel_format = SG_PIXELFORMAT_RGBA8,
        .cull_mode = SG_CULLMODE_NONE,
        .label = "pick-triangle-pipeline"
    });

    // Generate triangle template geometry (3 vertices with barycentric selectors, no index buffer)
    geom_template_vertex_t tri_verts[3] = {
        { 1.0f, 0.0f, 0.0f },  // Selector for vertex A
        { 0.0f, 1.0f, 0.0f },  // Selector for vertex B
        { 0.0f, 0.0f, 1.0f },  // Selector for vertex C
    };
    pb->triangle_template_vbuf = sg_make_buffer(&(sg_buffer_desc){
        .usage.vertex_buffer = true,
        .data = SG_RANGE(tri_verts),
        .label = "pick-triangle-template-vbuf"
    });

    // Initialize instance buffers
    instance_buffer_init(&pb->line_instances, sizeof(pick_line_instance_t), 64, "pick-line-instances");
    instance_buffer_init(&pb->point_instances, sizeof(pick_point_instance_t), 64, "pick-point-instances");
    instance_buffer_init(&pb->triangle_instances, sizeof(pick_triangle_instance_t), 64, "pick-triangle-instances");

    // Create overlay pipelines (depth-always, no depth write — for gizmo picks on top)
    pb->overlay_line_pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = pb->line_shd,  // Reuse same shaders
        .layout = {
            .buffers = {
                [0] = { .step_func = SG_VERTEXSTEP_PER_VERTEX },
                [1] = { .step_func = SG_VERTEXSTEP_PER_INSTANCE },
            },
            .attrs = {
                [0] = { .buffer_index = 0, .format = SG_VERTEXFORMAT_FLOAT3 },
                [1] = { .buffer_index = 1, .format = SG_VERTEXFORMAT_FLOAT3, .offset = 0 },
                [2] = { .buffer_index = 1, .format = SG_VERTEXFORMAT_FLOAT3, .offset = 12 },
                [3] = { .buffer_index = 1, .format = SG_VERTEXFORMAT_FLOAT3, .offset = 24 },
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
        .label = "pick-overlay-line-pipeline"
    });

    pb->overlay_point_pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = pb->point_shd,  // Reuse same shaders
        .layout = {
            .buffers = {
                [0] = { .step_func = SG_VERTEXSTEP_PER_VERTEX },
                [1] = { .step_func = SG_VERTEXSTEP_PER_INSTANCE },
            },
            .attrs = {
                [0] = { .buffer_index = 0, .format = SG_VERTEXFORMAT_FLOAT3 },
                [1] = { .buffer_index = 1, .format = SG_VERTEXFORMAT_FLOAT3, .offset = 0 },
                [2] = { .buffer_index = 1, .format = SG_VERTEXFORMAT_FLOAT3, .offset = 12 },
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
        .label = "pick-overlay-point-pipeline"
    });

    // Initialize overlay instance buffers
    instance_buffer_init(&pb->overlay_line_instances, sizeof(pick_line_instance_t), 32, "pick-overlay-line-instances");
    instance_buffer_init(&pb->overlay_point_instances, sizeof(pick_point_instance_t), 32, "pick-overlay-point-instances");
}

//------------------------------------------------------------------------------
// Shutdown
//------------------------------------------------------------------------------
static inline void pick_buffer_shutdown(pick_buffer_t *pb) {
    instance_buffer_shutdown(&pb->line_instances);
    instance_buffer_shutdown(&pb->point_instances);
    instance_buffer_shutdown(&pb->triangle_instances);
    instance_buffer_shutdown(&pb->overlay_line_instances);
    instance_buffer_shutdown(&pb->overlay_point_instances);

    sg_destroy_pipeline(pb->line_pip);
    sg_destroy_shader(pb->line_shd);
    sg_destroy_pipeline(pb->point_pip);
    sg_destroy_shader(pb->point_shd);
    sg_destroy_pipeline(pb->triangle_pip);
    sg_destroy_shader(pb->triangle_shd);
    sg_destroy_pipeline(pb->overlay_line_pip);
    sg_destroy_pipeline(pb->overlay_point_pip);

    sg_destroy_buffer(pb->line_template_vbuf);
    sg_destroy_buffer(pb->line_template_ibuf);
    sg_destroy_buffer(pb->point_template_vbuf);
    sg_destroy_buffer(pb->point_template_ibuf);
    sg_destroy_buffer(pb->triangle_template_vbuf);

    sg_destroy_view(pb->color_att_view);
    sg_destroy_view(pb->depth_att_view);
    sg_destroy_view(pb->tex_view);
    sg_destroy_sampler(pb->sampler);
    sg_destroy_image(pb->color_img);
    sg_destroy_image(pb->depth_img);

    free(pb->pixel_data);
}

//------------------------------------------------------------------------------
// Set pick buffer center (in viewport coordinates 0-1)
//------------------------------------------------------------------------------
static inline void pick_buffer_set_center(pick_buffer_t *pb, float x, float y,
                                           float viewport_width, float viewport_height) {
    // Detect cursor movement > 1 pixel (in viewport pixel coordinates)
    float dx = (x - pb->prev_center_x) * viewport_width;
    float dy = (y - pb->prev_center_y) * viewport_height;
    if (dx * dx + dy * dy > 1.0f) {
        pb->needs_rebuild = true;
    }

    pb->prev_center_x = x;
    pb->prev_center_y = y;
    pb->center_x = x;
    pb->center_y = y;
    pb->viewport_width = viewport_width;
    pb->viewport_height = viewport_height;
}

// Force pick buffer rebuild (call when scene changes: entity add/remove/move, visibility toggle)
static inline void pick_buffer_invalidate(pick_buffer_t *pb) {
    pb->needs_rebuild = true;
}

// Check if pick buffer needs rebuild this frame.
// Also detects camera changes via view+proj matrix hash.
static inline bool pick_buffer_needs_rebuild(pick_buffer_t *pb, mat4_t view, mat4_t proj) {
    if (pb->first_frame) {
        pb->first_frame = false;
        pb->needs_rebuild = true;
    }

    // Simple camera change detection: hash the first few floats of view+proj
    // Using a fast FNV-1a-style hash on the raw float bits
    uint32_t h = 2166136261u;
    const uint32_t *vf = (const uint32_t *)view.m;
    const uint32_t *pf = (const uint32_t *)proj.m;
    for (int i = 0; i < 16; i++) {
        h ^= vf[i]; h *= 16777619u;
        h ^= pf[i]; h *= 16777619u;
    }
    if (h != pb->prev_camera_hash) {
        pb->prev_camera_hash = h;
        pb->needs_rebuild = true;
    }

    return pb->needs_rebuild;
}

// Reset the rebuild flag after a successful rebuild cycle
static inline void pick_buffer_clear_rebuild_flag(pick_buffer_t *pb) {
    pb->needs_rebuild = false;
}

//------------------------------------------------------------------------------
// Clear instance buffers and prepare for new frame
//------------------------------------------------------------------------------
static inline void pick_buffer_begin_frame(pick_buffer_t *pb) {
    // Reset instance buffers (free all slots)
    instance_buffer_clear(&pb->line_instances);
    instance_buffer_clear(&pb->point_instances);
    instance_buffer_clear(&pb->triangle_instances);
    instance_buffer_clear(&pb->overlay_line_instances);
    instance_buffer_clear(&pb->overlay_point_instances);
}

//------------------------------------------------------------------------------
// Add a line for pick rendering
//------------------------------------------------------------------------------
static inline void pick_buffer_add_line(pick_buffer_t *pb,
                                         vec3_t a, vec3_t b,
                                         uint32_t pick_id) {
    int slot = instance_buffer_alloc_slot(&pb->line_instances);
    if (slot >= 0) {
        float r, g, b_color;
        pick_id_to_rgb_float(pick_id, &r, &g, &b_color);

        pick_line_instance_t inst = {
            .ax = a.x, .ay = a.y, .az = a.z,
            .bx = b.x, .by = b.y, .bz = b.z,
            .r = r, .g = g, .b = b_color
        };
        instance_buffer_set(&pb->line_instances, slot, &inst);
    }
}

//------------------------------------------------------------------------------
// Add a point for pick rendering
//------------------------------------------------------------------------------
static inline void pick_buffer_add_point(pick_buffer_t *pb,
                                          vec3_t center,
                                          uint32_t pick_id) {
    int slot = instance_buffer_alloc_slot(&pb->point_instances);
    if (slot >= 0) {
        float r, g, b;
        pick_id_to_rgb_float(pick_id, &r, &g, &b);

        pick_point_instance_t inst = {
            .x = center.x, .y = center.y, .z = center.z,
            .r = r, .g = g, .b = b
        };
        instance_buffer_set(&pb->point_instances, slot, &inst);
    }
}

//------------------------------------------------------------------------------
// Add a triangle for pick rendering
//------------------------------------------------------------------------------
static inline void pick_buffer_add_triangle(pick_buffer_t *pb,
                                             vec3_t a, vec3_t b, vec3_t c,
                                             uint32_t pick_id) {
    int slot = instance_buffer_alloc_slot(&pb->triangle_instances);
    if (slot >= 0) {
        float r, g, bl;
        pick_id_to_rgb_float(pick_id, &r, &g, &bl);

        pick_triangle_instance_t inst = {
            .ax = a.x, .ay = a.y, .az = a.z,
            .bx = b.x, .by = b.y, .bz = b.z,
            .cx = c.x, .cy = c.y, .cz = c.z,
            .r = r, .g = g, .b = bl
        };
        instance_buffer_set(&pb->triangle_instances, slot, &inst);
    }
}

//------------------------------------------------------------------------------
// Add an overlay line for pick rendering (depth-always, renders on top)
//------------------------------------------------------------------------------
static inline void pick_buffer_add_overlay_line(pick_buffer_t *pb,
                                                 vec3_t a, vec3_t b,
                                                 uint32_t pick_id) {
    int slot = instance_buffer_alloc_slot(&pb->overlay_line_instances);
    if (slot >= 0) {
        float r, g, b_color;
        pick_id_to_rgb_float(pick_id, &r, &g, &b_color);

        pick_line_instance_t inst = {
            .ax = a.x, .ay = a.y, .az = a.z,
            .bx = b.x, .by = b.y, .bz = b.z,
            .r = r, .g = g, .b = b_color
        };
        instance_buffer_set(&pb->overlay_line_instances, slot, &inst);
    }
}

//------------------------------------------------------------------------------
// Add an overlay point for pick rendering (depth-always, renders on top)
//------------------------------------------------------------------------------
static inline void pick_buffer_add_overlay_point(pick_buffer_t *pb,
                                                  vec3_t center,
                                                  uint32_t pick_id) {
    int slot = instance_buffer_alloc_slot(&pb->overlay_point_instances);
    if (slot >= 0) {
        float r, g, b;
        pick_id_to_rgb_float(pick_id, &r, &g, &b);

        pick_point_instance_t inst = {
            .x = center.x, .y = center.y, .z = center.z,
            .r = r, .g = g, .b = b
        };
        instance_buffer_set(&pb->overlay_point_instances, slot, &inst);
    }
}

//------------------------------------------------------------------------------
// Compute the modified MVP matrix for pick buffer rendering
// This creates a view that zooms in on the area around the cursor
//------------------------------------------------------------------------------
static inline mat4_t pick_buffer_compute_mvp(pick_buffer_t *pb, mat4_t view, mat4_t proj) {
    return mdcad_interaction_compute_pick_mvp(pb->center_x,
                                               pb->center_y,
                                               pb->viewport_width,
                                               pb->viewport_height,
                                               (float)PICK_BUFFER_SIZE,
                                               &pb->zoom_factor,
                                               view,
                                               proj);
}

//------------------------------------------------------------------------------
// Render pick buffer
//------------------------------------------------------------------------------
static inline void pick_buffer_render(pick_buffer_t *pb, mat4_t view, mat4_t proj) {
    // Upload instance data
    instance_buffer_upload(&pb->line_instances);
    instance_buffer_upload(&pb->point_instances);
    instance_buffer_upload(&pb->triangle_instances);

    // Compute modified MVP for pick region (also sets pb->zoom_factor)
    mat4_t mvp = pick_buffer_compute_mvp(pb, view, proj);

    // Scale line width by zoom factor so lines appear the same screen-pixel size
    // as in the main viewport. Apply thickness_multiplier for easier picking.
    float scaled_line_width = PICK_BUFFER_LINE_WIDTH * pb->zoom_factor * pb->thickness_multiplier;

    // Begin pick pass
    sg_begin_pass(&(sg_pass){
        .action = {
            .colors[0] = { .load_action = SG_LOADACTION_CLEAR, .clear_value = { 0, 0, 0, 0 } },
            .depth = { .load_action = SG_LOADACTION_CLEAR, .clear_value = 1.0f }
        },
        .attachments = {
            .colors[0] = pb->color_att_view,
            .depth_stencil = pb->depth_att_view
        },
        .label = "pick-pass"
    });

    // Compute aspect ratio for the pick view
    // Since we use uniform zoom, the captured region maintains the original viewport's aspect ratio
    float pick_aspect = pb->viewport_width / pb->viewport_height;

    pick_params_t params = {
        .mvp = mvp,
        .line_width = scaled_line_width,
        .aspect_ratio = pick_aspect  // Use viewport aspect ratio for correct line rendering
    };

    // Draw triangles first (solid geometry, drawn before lines/points for correct depth)
    int triangle_count = instance_buffer_count(&pb->triangle_instances);
    if (triangle_count > 0) {
        pick_triangle_params_t tri_params = { .mvp = mvp };
        sg_apply_pipeline(pb->triangle_pip);
        sg_apply_bindings(&(sg_bindings){
            .vertex_buffers = {
                [0] = pb->triangle_template_vbuf,
                [1] = instance_buffer_gpu_buffer(&pb->triangle_instances)
            }
        });
        sg_apply_uniforms(0, &SG_RANGE(tri_params));
        sg_draw(0, 3, triangle_count);
    }

    // Draw lines
    int line_count = instance_buffer_count(&pb->line_instances);
    if (line_count > 0) {
        sg_apply_pipeline(pb->line_pip);
        sg_apply_bindings(&(sg_bindings){
            .vertex_buffers = {
                [0] = pb->line_template_vbuf,
                [1] = instance_buffer_gpu_buffer(&pb->line_instances)
            },
            .index_buffer = pb->line_template_ibuf
        });
        sg_apply_uniforms(0, &SG_RANGE(params));
        sg_draw(0, pb->line_template_index_count, line_count);
    }

    // Draw points (use slightly larger size for easier picking)
    int point_count = instance_buffer_count(&pb->point_instances);
    if (point_count > 0) {
        params.line_width = scaled_line_width * 1.5f;  // Points slightly larger than lines

        sg_apply_pipeline(pb->point_pip);
        sg_apply_bindings(&(sg_bindings){
            .vertex_buffers = {
                [0] = pb->point_template_vbuf,
                [1] = instance_buffer_gpu_buffer(&pb->point_instances)
            },
            .index_buffer = pb->point_template_ibuf
        });
        sg_apply_uniforms(0, &SG_RANGE(params));
        sg_draw(0, pb->point_template_index_count, point_count);
    }

    // Draw overlay picks (depth-always, on top of everything)
    instance_buffer_upload(&pb->overlay_line_instances);
    instance_buffer_upload(&pb->overlay_point_instances);

    int overlay_line_count = instance_buffer_count(&pb->overlay_line_instances);
    if (overlay_line_count > 0) {
        params.line_width = scaled_line_width;
        sg_apply_pipeline(pb->overlay_line_pip);
        sg_apply_bindings(&(sg_bindings){
            .vertex_buffers = {
                [0] = pb->line_template_vbuf,
                [1] = instance_buffer_gpu_buffer(&pb->overlay_line_instances)
            },
            .index_buffer = pb->line_template_ibuf
        });
        sg_apply_uniforms(0, &SG_RANGE(params));
        sg_draw(0, pb->line_template_index_count, overlay_line_count);
    }

    int overlay_point_count = instance_buffer_count(&pb->overlay_point_instances);
    if (overlay_point_count > 0) {
        params.line_width = scaled_line_width * 1.5f;
        sg_apply_pipeline(pb->overlay_point_pip);
        sg_apply_bindings(&(sg_bindings){
            .vertex_buffers = {
                [0] = pb->point_template_vbuf,
                [1] = instance_buffer_gpu_buffer(&pb->overlay_point_instances)
            },
            .index_buffer = pb->point_template_ibuf
        });
        sg_apply_uniforms(0, &SG_RANGE(params));
        sg_draw(0, pb->point_template_index_count, overlay_point_count);
    }

    sg_end_pass();
    pb->has_rendered = true;
}

//------------------------------------------------------------------------------
// CPU Readback
// Note: Sokol doesn't have built-in async readback. This uses sg_query_image_pixels
// which may not be available on all backends. For now, we'll use a fallback.
//------------------------------------------------------------------------------
static inline void pick_buffer_readback(pick_buffer_t *pb) {
    // Use platform-specific readback implementation
    bool success = pick_readback_pixels(
        pb->color_img,
        PICK_BUFFER_SIZE,
        PICK_BUFFER_SIZE,
        pb->pixel_data
    );

    if (!success) {
        // Readback failed or not implemented - clear to indicate no hover
        memset(pb->pixel_data, 0, PICK_BUFFER_SIZE * PICK_BUFFER_SIZE * 4);
        pb->hovered_pick_id = 0;
    }
}

//------------------------------------------------------------------------------
// Sample a pixel from the readback buffer
//------------------------------------------------------------------------------
static inline uint32_t pick_buffer_sample(pick_buffer_t *pb, int x, int y) {
    if (x < 0 || x >= PICK_BUFFER_SIZE || y < 0 || y >= PICK_BUFFER_SIZE) {
        return 0;
    }

    int idx = (y * PICK_BUFFER_SIZE + x) * 4;
    uint8_t r = pb->pixel_data[idx + 0];
    uint8_t g = pb->pixel_data[idx + 1];
    uint8_t b = pb->pixel_data[idx + 2];

    return rgb_to_pick_id(r, g, b);
}

//------------------------------------------------------------------------------
// Sample the center pixel (most common case for hover detection)
//------------------------------------------------------------------------------
static inline uint32_t pick_buffer_sample_center(pick_buffer_t *pb) {
    return pick_buffer_sample(pb, PICK_BUFFER_SIZE / 2, PICK_BUFFER_SIZE / 2);
}

//------------------------------------------------------------------------------
// Update hover state (call after readback)
//------------------------------------------------------------------------------
static inline void pick_buffer_update_hover(pick_buffer_t *pb) {
    pb->hovered_pick_id = pick_buffer_sample_center(pb);
}

//------------------------------------------------------------------------------
// Get currently hovered pick ID
//------------------------------------------------------------------------------
static inline uint32_t pick_buffer_get_hovered_id(pick_buffer_t *pb) {
    return pb->hovered_pick_id;
}

#endif // PICK_BUFFER_H
