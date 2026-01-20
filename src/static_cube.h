//------------------------------------------------------------------------------
// static_cube.h - Static cube rendering module (header-only)
//
// Encapsulates cube mesh, shader, pipeline, and rendering.
//------------------------------------------------------------------------------
#ifndef STATIC_CUBE_H
#define STATIC_CUBE_H

#include "platform.h"
#include "sokol_gfx.h"
#include "math3d.h"
#include "primitives.h"
#include "shaders/cube_shaders.h"

//------------------------------------------------------------------------------
// Types
//------------------------------------------------------------------------------

typedef struct {
    sg_pipeline pip;
    sg_bindings bind;
    sg_shader shd;
    sg_buffer vbuf;
    sg_buffer ibuf;
    int index_count;
} static_cube_t;

//------------------------------------------------------------------------------
// Core functions
//------------------------------------------------------------------------------

static inline void static_cube_init(static_cube_t* cube) {
    // Get cube mesh data
    mesh_t mesh = mesh_cube();
    cube->index_count = mesh.index_count;

    // Create vertex buffer
    cube->vbuf = sg_make_buffer(&(sg_buffer_desc){
        .usage.vertex_buffer = true,
        .data = {
            .ptr = mesh.vertices,
            .size = mesh.vertex_count * sizeof(vertex_t)
        },
        .label = "cube-vertices"
    });

    // Create index buffer
    cube->ibuf = sg_make_buffer(&(sg_buffer_desc){
        .usage.index_buffer = true,
        .data = {
            .ptr = mesh.indices,
            .size = mesh.index_count * sizeof(uint16_t)
        },
        .label = "cube-indices"
    });

    // Create shader
    cube->shd = sg_make_shader(&(sg_shader_desc){
        .vertex_func = {
            .source = cube_vs_source,
            .entry = "vs_main",
        },
        .fragment_func = {
            .source = cube_fs_source,
            .entry = "fs_main",
        },
        .uniform_blocks[0] = {
            .stage = SG_SHADERSTAGE_VERTEX,
            .size = sizeof(mat4_t),
            .layout = SG_UNIFORMLAYOUT_STD140,
            .glsl_uniforms[0] = { .type = SG_UNIFORMTYPE_MAT4, .glsl_name = "mvp" }
        },
        .label = "cube-shader"
    });

    // Create pipeline
    cube->pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = cube->shd,
        .layout = {
            .attrs = {
                [0] = { .format = SG_VERTEXFORMAT_FLOAT3 },  // position
                [1] = { .format = SG_VERTEXFORMAT_FLOAT3 }   // normal
            }
        },
        .index_type = SG_INDEXTYPE_UINT16,
        .cull_mode = SG_CULLMODE_BACK,
        .depth = {
            .compare = SG_COMPAREFUNC_LESS_EQUAL,
            .write_enabled = true,
            .pixel_format = SG_PIXELFORMAT_DEPTH
        },
        .colors[0].pixel_format = SG_PIXELFORMAT_RGBA8,
        .label = "cube-pipeline"
    });

    // Setup bindings
    cube->bind = (sg_bindings){
        .vertex_buffers[0] = cube->vbuf,
        .index_buffer = cube->ibuf
    };
}

static inline void static_cube_draw(static_cube_t* cube, mat4_t mvp) {
    sg_apply_pipeline(cube->pip);
    sg_apply_bindings(&cube->bind);
    sg_apply_uniforms(0, &SG_RANGE(mvp));
    sg_draw(0, cube->index_count, 1);
}

static inline void static_cube_shutdown(static_cube_t* cube) {
    sg_destroy_pipeline(cube->pip);
    sg_destroy_shader(cube->shd);
    sg_destroy_buffer(cube->vbuf);
    sg_destroy_buffer(cube->ibuf);
}

#endif // STATIC_CUBE_H
