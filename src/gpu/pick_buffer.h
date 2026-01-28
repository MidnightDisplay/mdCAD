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
#define PICK_BUFFER_LINE_WIDTH 0.03f  // Base line width (same as ECS lines)

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

//------------------------------------------------------------------------------
// Pick uniform block
//------------------------------------------------------------------------------
typedef struct {
    mat4_t mvp;
    float line_width;
    float aspect_ratio;
    float _pad[2];
} pick_params_t;

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

    // Template geometry (reused from geometry_batch)
    sg_buffer line_template_vbuf;
    sg_buffer line_template_ibuf;
    int line_template_vertex_count;
    int line_template_index_count;

    sg_buffer point_template_vbuf;
    sg_buffer point_template_ibuf;
    int point_template_vertex_count;
    int point_template_index_count;

    // Instance buffers for pick rendering
    instance_buffer_t line_instances;
    instance_buffer_t point_instances;

    // CPU readback buffer
    uint8_t *pixel_data;

    // Pick state
    float center_x;             // Center position in viewport (0-1 normalized)
    float center_y;
    float viewport_width;       // Source viewport dimensions
    float viewport_height;
    float zoom_factor;          // Computed zoom factor for line width scaling
    uint32_t hovered_pick_id;   // Currently hovered entity's pick ID (0 = none)

    // Debug visualization
    bool debug_enabled;
} pick_buffer_t;

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
    pb->line_shd = sg_make_shader(&(sg_shader_desc){
        .vertex_func = {
            .source = pick_line_vs_source,
            .entry = "vs_main",
        },
        .fragment_func = {
            .source = pick_line_fs_source,
            .entry = "fs_main",
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
    pb->point_shd = sg_make_shader(&(sg_shader_desc){
        .vertex_func = {
            .source = pick_point_vs_source,
            .entry = "vs_main",
        },
        .fragment_func = {
            .source = pick_point_fs_source,
            .entry = "fs_main",
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

    // Initialize instance buffers
    instance_buffer_init(&pb->line_instances, sizeof(pick_line_instance_t), 64, "pick-line-instances");
    instance_buffer_init(&pb->point_instances, sizeof(pick_point_instance_t), 64, "pick-point-instances");
}

//------------------------------------------------------------------------------
// Shutdown
//------------------------------------------------------------------------------
static inline void pick_buffer_shutdown(pick_buffer_t *pb) {
    instance_buffer_shutdown(&pb->line_instances);
    instance_buffer_shutdown(&pb->point_instances);

    sg_destroy_pipeline(pb->line_pip);
    sg_destroy_shader(pb->line_shd);
    sg_destroy_pipeline(pb->point_pip);
    sg_destroy_shader(pb->point_shd);

    sg_destroy_buffer(pb->line_template_vbuf);
    sg_destroy_buffer(pb->line_template_ibuf);
    sg_destroy_buffer(pb->point_template_vbuf);
    sg_destroy_buffer(pb->point_template_ibuf);

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
    pb->center_x = x;
    pb->center_y = y;
    pb->viewport_width = viewport_width;
    pb->viewport_height = viewport_height;
}

//------------------------------------------------------------------------------
// Clear instance buffers and prepare for new frame
//------------------------------------------------------------------------------
static inline void pick_buffer_begin_frame(pick_buffer_t *pb) {
    // Reset instance buffers (free all slots)
    instance_buffer_clear(&pb->line_instances);
    instance_buffer_clear(&pb->point_instances);
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
// Compute the modified MVP matrix for pick buffer rendering
// This creates a view that zooms in on the area around the cursor
//------------------------------------------------------------------------------
static inline mat4_t pick_buffer_compute_mvp(pick_buffer_t *pb, mat4_t view, mat4_t proj) {
    // The pick buffer renders a small region around the cursor
    // We need to modify the projection matrix to zoom into that region

    // Convert center from 0-1 to -1 to 1 NDC space
    float ndc_x = pb->center_x * 2.0f - 1.0f;
    float ndc_y = (1.0f - pb->center_y) * 2.0f - 1.0f;  // Flip Y

    // Calculate the zoom factor: use uniform zoom to avoid distortion
    // Use the smaller dimension to ensure we capture enough area
    float zoom_x = pb->viewport_width / (float)PICK_BUFFER_SIZE;
    float zoom_y = pb->viewport_height / (float)PICK_BUFFER_SIZE;

    // Use uniform zoom (minimum of the two to capture a larger area and avoid distortion)
    // Ensure minimum zoom of 1.0 to avoid issues with very small viewports
    float zoom = (zoom_x < zoom_y) ? zoom_x : zoom_y;
    if (zoom < 1.0f) zoom = 1.0f;
    pb->zoom_factor = zoom;  // Store for line width scaling

    // Create a modified projection that:
    // 1. Scales to zoom into the region (uniform)
    // 2. Translates so the cursor is at the center

    // Start with identity
    mat4_t pick_proj = mat4_identity();

    // Scale (zoom in) - column-major: m[col*4 + row]
    // Use uniform zoom to preserve aspect ratio
    pick_proj.m[0] = zoom;    // m[0][0]
    pick_proj.m[5] = zoom;    // m[1][1]

    // Translate to center on cursor (in NDC space)
    pick_proj.m[12] = -ndc_x * zoom;   // m[3][0]
    pick_proj.m[13] = -ndc_y * zoom;   // m[3][1]

    // Combine: pick_proj * proj * view
    mat4_t vp = mat4_mul(proj, view);
    return mat4_mul(pick_proj, vp);
}

//------------------------------------------------------------------------------
// Render pick buffer
//------------------------------------------------------------------------------
static inline void pick_buffer_render(pick_buffer_t *pb, mat4_t view, mat4_t proj) {
    // Upload instance data
    instance_buffer_upload(&pb->line_instances);
    instance_buffer_upload(&pb->point_instances);

    // Compute modified MVP for pick region (also sets pb->zoom_factor)
    mat4_t mvp = pick_buffer_compute_mvp(pb, view, proj);

    // Scale line width by zoom factor so lines appear the same screen-pixel size
    // as in the main viewport. This is critical for accurate picking.
    float scaled_line_width = PICK_BUFFER_LINE_WIDTH * pb->zoom_factor;

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

    sg_end_pass();
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
