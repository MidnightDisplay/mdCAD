//------------------------------------------------------------------------------
// Sokol + cimgui demo with dockspace and 3D viewport rendering a cube
//------------------------------------------------------------------------------

// Platform detection must be first (before Sokol includes)
#include "platform.h"

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
#include "imgui_storage.h"
#include "render_target.h"
#include "ui/ui_controls.h"
#include "ui/ui_viewport.h"

#include <math.h>

//------------------------------------------------------------------------------
// Application state
//------------------------------------------------------------------------------
typedef struct {
    mat4_t mvp;
} vs_params_t;

static struct {
    // Rendering
    render_target_t viewport_rt;
    sg_pass_action offscreen_pass_action;
    sg_pass_action main_pass_action;
    sg_pipeline pip;
    sg_bindings bind;

    // Scene state
    float rotation;
    uint64_t last_time;

    // UI state
    ui_controls_state_t controls;
    ui_viewport_state_t viewport;
} state;

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
#ifndef PLATFORM_WEB
        .ini_filename = "imgui.ini",
#endif
        .logger.func = slog_func,
    });

    // Enable docking
    ImGuiIO* io = igGetIO_Nil();
    io->ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // Initialize ImGui persistence (must be after simgui_setup and ConfigFlags)
    imgui_storage_init();

    // Initialize render target
    render_target_init(&state.viewport_rt);

    // Initialize UI modules
    ui_controls_init(&state.controls, &state.rotation, &state.offscreen_pass_action);
    ui_viewport_init(&state.viewport, &state.viewport_rt);

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
    // Handle ImGui settings persistence
    imgui_storage_frame();

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

    //=== UI ===
    igDockSpaceOverViewport(0, NULL, ImGuiDockNodeFlags_None, NULL);
    ui_controls_draw(&state.controls);
    ui_viewport_draw(&state.viewport);

    //=== RENDER CUBE TO OFFSCREEN TARGET ===

    // Calculate MVP matrix for isometric-like view
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

    int vp_width = state.viewport_rt.width;
    int vp_height = state.viewport_rt.height;

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
            .colors[0] = state.viewport_rt.color_att_view,
            .depth_stencil = state.viewport_rt.depth_att_view,
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
    imgui_storage_shutdown();
    render_target_shutdown(&state.viewport_rt);
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
