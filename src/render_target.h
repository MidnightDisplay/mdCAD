//------------------------------------------------------------------------------
// render_target.h - Offscreen render target management (header-only)
//------------------------------------------------------------------------------
#ifndef RENDER_TARGET_H
#define RENDER_TARGET_H

#include "sokol_gfx.h"

//------------------------------------------------------------------------------
// Constants
//------------------------------------------------------------------------------
#define RENDER_TARGET_DEFAULT_WIDTH 512
#define RENDER_TARGET_DEFAULT_HEIGHT 512
#define RENDER_TARGET_MIN_SIZE 64

//------------------------------------------------------------------------------
// Types
//------------------------------------------------------------------------------
typedef struct {
    sg_image color_img;
    sg_image depth_img;
    sg_view color_att_view;
    sg_view depth_att_view;
    sg_view tex_view;
    sg_sampler sampler;
    int width;
    int height;
} render_target_t;

//------------------------------------------------------------------------------
// Functions
//------------------------------------------------------------------------------

// Create render target resources at specified size
static inline void render_target_create(render_target_t* rt, int width, int height) {
    // Ensure minimum size
    if (width < RENDER_TARGET_MIN_SIZE) width = RENDER_TARGET_MIN_SIZE;
    if (height < RENDER_TARGET_MIN_SIZE) height = RENDER_TARGET_MIN_SIZE;

    // Create color render target
    rt->color_img = sg_make_image(&(sg_image_desc){
        .usage.color_attachment = true,
        .width = width,
        .height = height,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .sample_count = 1,
        .label = "offscreen-color"
    });

    // Create depth buffer
    rt->depth_img = sg_make_image(&(sg_image_desc){
        .usage.depth_stencil_attachment = true,
        .width = width,
        .height = height,
        .pixel_format = SG_PIXELFORMAT_DEPTH,
        .sample_count = 1,
        .label = "offscreen-depth"
    });

    // Create color attachment view
    rt->color_att_view = sg_make_view(&(sg_view_desc){
        .color_attachment.image = rt->color_img,
        .label = "color-att-view"
    });

    // Create depth attachment view
    rt->depth_att_view = sg_make_view(&(sg_view_desc){
        .depth_stencil_attachment.image = rt->depth_img,
        .label = "depth-att-view"
    });

    // Create texture view for sampling in ImGui
    rt->tex_view = sg_make_view(&(sg_view_desc){
        .texture.image = rt->color_img,
        .label = "tex-view"
    });

    rt->width = width;
    rt->height = height;
}

// Destroy render target resources
static inline void render_target_destroy(render_target_t* rt) {
    if (rt->color_img.id) {
        sg_destroy_view(rt->color_att_view);
        sg_destroy_view(rt->depth_att_view);
        sg_destroy_view(rt->tex_view);
        sg_destroy_image(rt->color_img);
        sg_destroy_image(rt->depth_img);
        rt->color_img.id = 0;
    }
}

// Initialize render target with default size and create sampler
static inline void render_target_init(render_target_t* rt) {
    rt->color_img.id = 0;  // Mark as uninitialized

    // Create sampler for ImGui to display the render target
    rt->sampler = sg_make_sampler(&(sg_sampler_desc){
        .min_filter = SG_FILTER_LINEAR,
        .mag_filter = SG_FILTER_LINEAR,
        .label = "viewport-sampler"
    });

    render_target_create(rt, RENDER_TARGET_DEFAULT_WIDTH, RENDER_TARGET_DEFAULT_HEIGHT);
}

// Resize render target (destroys and recreates if size changed)
static inline void render_target_resize(render_target_t* rt, int width, int height) {
    // Ensure minimum size
    if (width < RENDER_TARGET_MIN_SIZE) width = RENDER_TARGET_MIN_SIZE;
    if (height < RENDER_TARGET_MIN_SIZE) height = RENDER_TARGET_MIN_SIZE;

    // Only recreate if size actually changed
    if (width == rt->width && height == rt->height) {
        return;
    }

    render_target_destroy(rt);
    render_target_create(rt, width, height);
}

// Shutdown and cleanup all resources including sampler
static inline void render_target_shutdown(render_target_t* rt) {
    render_target_destroy(rt);
    sg_destroy_sampler(rt->sampler);
}

#endif // RENDER_TARGET_H
