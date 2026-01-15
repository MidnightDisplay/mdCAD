//------------------------------------------------------------------------------
// Sokol + cimgui demo with dockspace and 3D viewport rendering a cube
//------------------------------------------------------------------------------

// Define backend before includes (same as sokol.c)
#if defined(_WIN32)
#define SOKOL_D3D11
#elif defined(__EMSCRIPTEN__)
#define SOKOL_WGPU
#elif defined(__APPLE__)
#define SOKOL_METAL
#else
#define SOKOL_GLCORE
#endif

#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_log.h"
#include "sokol_glue.h"
#include "sokol_time.h"
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"
#include "sokol_imgui.h"

// Project modules
#include "math3d.h"
#include "primitives.h"
#include "shaders/cube_shaders.h"

#include <string.h>

//------------------------------------------------------------------------------
// Application state
//------------------------------------------------------------------------------
#define OFFSCREEN_WIDTH 512
#define OFFSCREEN_HEIGHT 512

typedef struct {
    mat4_t mvp;
} vs_params_t;

static struct {
    // Offscreen rendering
    sg_image color_img;
    sg_image depth_img;
    sg_view color_att_view;
    sg_view depth_att_view;
    sg_view tex_view;
    sg_pass_action offscreen_pass_action;
    sg_pipeline pip;
    sg_bindings bind;
    sg_sampler sampler;

    // Main pass
    sg_pass_action main_pass_action;

    // State
    float clear_color[3];
    float rotation;
    uint64_t last_time;

    // Viewport size tracking
    int viewport_width;
    int viewport_height;
} state;

//------------------------------------------------------------------------------
// Recreate offscreen resources when viewport size changes
//------------------------------------------------------------------------------
static void create_offscreen_resources(int width, int height) {
    // Destroy old resources if they exist
    if (state.color_img.id) {
        sg_destroy_view(state.color_att_view);
        sg_destroy_view(state.depth_att_view);
        sg_destroy_view(state.tex_view);
        sg_destroy_image(state.color_img);
        sg_destroy_image(state.depth_img);
    }

    // Create color render target
    state.color_img = sg_make_image(&(sg_image_desc){
        .usage.color_attachment = true,
        .width = width,
        .height = height,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .sample_count = 1,
        .label = "offscreen-color"
    });

    // Create depth buffer
    state.depth_img = sg_make_image(&(sg_image_desc){
        .usage.depth_stencil_attachment = true,
        .width = width,
        .height = height,
        .pixel_format = SG_PIXELFORMAT_DEPTH,
        .sample_count = 1,
        .label = "offscreen-depth"
    });

    // Create color attachment view
    state.color_att_view = sg_make_view(&(sg_view_desc){
        .color_attachment.image = state.color_img,
        .label = "color-att-view"
    });

    // Create depth attachment view
    state.depth_att_view = sg_make_view(&(sg_view_desc){
        .depth_stencil_attachment.image = state.depth_img,
        .label = "depth-att-view"
    });

    // Create texture view for sampling in ImGui
    state.tex_view = sg_make_view(&(sg_view_desc){
        .texture.image = state.color_img,
        .label = "tex-view"
    });

    state.viewport_width = width;
    state.viewport_height = height;
}

//------------------------------------------------------------------------------
// Init
//------------------------------------------------------------------------------
static void init(void) {
    stm_setup();
    state.last_time = stm_now();

    sg_setup(&(sg_desc){
        .environment = sglue_environment(),
        .logger.func = slog_func,
    });

    // Setup ImGui with docking enabled
    simgui_setup(&(simgui_desc_t){
        .logger.func = slog_func,
    });

    // Enable docking
    ImGuiIO* io = igGetIO_Nil();
    io->ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // Initial clear color (cornflower blue)
    state.clear_color[0] = 0.39f;
    state.clear_color[1] = 0.58f;
    state.clear_color[2] = 0.93f;

    // Create initial offscreen resources
    create_offscreen_resources(OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);

    // Offscreen pass action
    state.offscreen_pass_action = (sg_pass_action){
        .colors[0] = {
            .load_action = SG_LOADACTION_CLEAR,
            .clear_value = { state.clear_color[0], state.clear_color[1], state.clear_color[2], 1.0f }
        },
        .depth = { .load_action = SG_LOADACTION_CLEAR, .clear_value = 1.0f }
    };

    // Main pass action (just clear to dark gray)
    state.main_pass_action = (sg_pass_action){
        .colors[0] = { .load_action = SG_LOADACTION_CLEAR, .clear_value = { 0.1f, 0.1f, 0.1f, 1.0f } }
    };

    // Get cube mesh data
    mesh_t cube = mesh_cube();

    // Create vertex buffer
    sg_buffer vbuf = sg_make_buffer(&(sg_buffer_desc){
        .usage.vertex_buffer = true,
        .data = {
            .ptr = cube.vertices,
            .size = cube.vertex_count * sizeof(vertex_t)
        },
        .label = "cube-vertices"
    });

    // Create index buffer
    sg_buffer ibuf = sg_make_buffer(&(sg_buffer_desc){
        .usage.index_buffer = true,
        .data = {
            .ptr = cube.indices,
            .size = cube.index_count * sizeof(uint16_t)
        },
        .label = "cube-indices"
    });

    // Create sampler for ImGui to display the render target
    state.sampler = sg_make_sampler(&(sg_sampler_desc){
        .min_filter = SG_FILTER_LINEAR,
        .mag_filter = SG_FILTER_LINEAR,
        .label = "viewport-sampler"
    });

    // Create shader
    sg_shader shd = sg_make_shader(&(sg_shader_desc){
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
            .size = sizeof(vs_params_t),
            .layout = SG_UNIFORMLAYOUT_STD140,
            .glsl_uniforms[0] = { .type = SG_UNIFORMTYPE_MAT4, .glsl_name = "mvp" }
        },
        .label = "cube-shader"
    });

    // Create pipeline
    state.pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
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
    state.bind = (sg_bindings){
        .vertex_buffers[0] = vbuf,
        .index_buffer = ibuf
    };

    state.rotation = 0.0f;
}

//------------------------------------------------------------------------------
// Frame
//------------------------------------------------------------------------------
static void frame(void) {
    // Calculate delta time
    uint64_t now = stm_now();
    float dt = (float)stm_sec(stm_diff(now, state.last_time));
    state.last_time = now;

    // Rotate cube slowly
    state.rotation += dt * 0.5f;

    const int width = sapp_width();
    const int height = sapp_height();

    simgui_new_frame(&(simgui_frame_desc_t){
        .width = width,
        .height = height,
        .delta_time = sapp_frame_duration(),
        .dpi_scale = sapp_dpi_scale(),
    });

    //=== UI CODE ===

    // Create dockspace over the entire viewport
    igDockSpaceOverViewport(0, NULL, ImGuiDockNodeFlags_None, NULL);

    // Control window
    igBegin("Controls", NULL, ImGuiWindowFlags_None);
    igText("3D Viewport Settings");
    igSeparator();
    if (igColorEdit3("Clear Color", state.clear_color, ImGuiColorEditFlags_None)) {
        // Update pass action when color changes
        state.offscreen_pass_action.colors[0].clear_value.r = state.clear_color[0];
        state.offscreen_pass_action.colors[0].clear_value.g = state.clear_color[1];
        state.offscreen_pass_action.colors[0].clear_value.b = state.clear_color[2];
    }
    igText("Rotation: %.2f rad", state.rotation);
    if (igButton("Reset Rotation", (ImVec2){0, 0})) {
        state.rotation = 0.0f;
    }
    igEnd();

    // 3D Viewport window
    igPushStyleVar_Vec2(ImGuiStyleVar_WindowPadding, (ImVec2){0, 0});
    igBegin("3D Viewport", NULL, ImGuiWindowFlags_None);

    // Get available content region size
    ImVec2 content_size = igGetContentRegionAvail();
    int vp_width = (int)content_size.x;
    int vp_height = (int)content_size.y;

    // Ensure minimum size
    if (vp_width < 64) vp_width = 64;
    if (vp_height < 64) vp_height = 64;

    // Recreate offscreen resources if size changed
    if (vp_width != state.viewport_width || vp_height != state.viewport_height) {
        create_offscreen_resources(vp_width, vp_height);
    }

    // Display the rendered image using ImGui
    uint64_t tex_id = simgui_imtextureid_with_sampler(state.tex_view, state.sampler);
    ImTextureRef_c tex_ref = { ._TexID = tex_id };
    igImage(tex_ref, (ImVec2_c){(float)vp_width, (float)vp_height},
            (ImVec2_c){0, 0}, (ImVec2_c){1, 1});

    igEnd();
    igPopStyleVar(1);

    //=== RENDER CUBE TO OFFSCREEN TARGET ===

    // Calculate MVP matrix for isometric-like view
    // Camera positioned at 45-degree angle to see 3 faces of the cube
    float cam_dist = 3.0f;
    float cam_height = 2.0f;
    float cam_angle = 0.785398f; // 45 degrees
    vec3_t eye = {
        cam_dist * sinf(cam_angle),
        cam_height,
        cam_dist * cosf(cam_angle)
    };
    vec3_t target = { 0.0f, 0.0f, 0.0f };
    vec3_t up = { 0.0f, 1.0f, 0.0f };

    mat4_t view = mat4_lookat(eye, target, up);
    mat4_t proj = mat4_perspective(0.785398f, (float)vp_width / (float)vp_height, 0.1f, 100.0f);
    mat4_t model = mat4_rotate_y(state.rotation);
    mat4_t vp_mat = mat4_mul(proj, view);
    mat4_t mvp = mat4_mul(vp_mat, model);

    vs_params_t vs_params = { .mvp = mvp };

    // Offscreen pass - render the cube
    sg_begin_pass(&(sg_pass){
        .action = state.offscreen_pass_action,
        .attachments = {
            .colors[0] = state.color_att_view,
            .depth_stencil = state.depth_att_view,
        }
    });
    sg_apply_pipeline(state.pip);
    sg_apply_bindings(&state.bind);
    sg_apply_uniforms(0, &SG_RANGE(vs_params));
    sg_draw(0, 36, 1);
    sg_end_pass();

    //=== MAIN PASS - RENDER IMGUI ===
    sg_begin_pass(&(sg_pass){
        .swapchain = sglue_swapchain(),
        .action = state.main_pass_action
    });
    simgui_render();
    sg_end_pass();

    sg_commit();
}

//------------------------------------------------------------------------------
// Cleanup
//------------------------------------------------------------------------------
static void cleanup(void) {
    sg_destroy_view(state.color_att_view);
    sg_destroy_view(state.depth_att_view);
    sg_destroy_view(state.tex_view);
    sg_destroy_image(state.color_img);
    sg_destroy_image(state.depth_img);
    simgui_shutdown();
    sg_shutdown();
}

//------------------------------------------------------------------------------
// Event handling
//------------------------------------------------------------------------------
static void event(const sapp_event* ev) {
    simgui_handle_event(ev);
}

//------------------------------------------------------------------------------
// Main entry point
//------------------------------------------------------------------------------
sapp_desc sokol_main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    return (sapp_desc){
        .init_cb = init,
        .frame_cb = frame,
        .cleanup_cb = cleanup,
        .event_cb = event,
        .window_title = "Sokol Cube Viewport",
        .width = 1280,
        .height = 720,
        .icon.sokol_default = true,
        .logger.func = slog_func,
    };
}
