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
#include "imgui_storage.h"
#include "render_target.h"
#include "orbit_camera.h"
#include "static_cube.h"
#include "dynamic_lines.h"
#include "instanced_lines.h"
#include "instanced_polylines.h"
#include "instanced_lines_alpha.h"
#include "instanced_alpha_polylines.h"
#include "ui/ui_theme.h"
#include "ui/ui_controls.h"
#include "ui/ui_viewport.h"
#include "ui/ui_camera_debug.h"
#include "ui/ui_lines_controls.h"
#include "ui/ui_thick_lines_controls.h"
#include "ui/ui_visibility.h"
#include "ui/ui_gcode_controls.h"

//------------------------------------------------------------------------------
// Application state
//------------------------------------------------------------------------------
static struct {
    // Rendering
    render_target_t viewport_rt;
    sg_pass_action offscreen_pass_action;
    sg_pass_action main_pass_action;

    // Renderables
    static_cube_t cube;
    dynamic_lines_t lines;
    instanced_lines_t instanced_lines;
    instanced_polylines_t instanced_polylines;
    instanced_lines_alpha_t alpha_lines;
    instanced_alpha_polylines_t gcode_polyline;
    bool gcode_loaded;

    // Scene state
    orbit_camera_t camera;
    uint64_t last_time;
    float elapsed_time;

    // UI state
    ui_controls_state_t controls;
    ui_viewport_state_t viewport;
    ui_camera_debug_state_t camera_debug;
    ui_lines_controls_state_t lines_controls;
    ui_thick_lines_controls_state_t thick_lines_controls;
    ui_visibility_state_t visibility;
    ui_gcode_controls_state_t gcode_controls;
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

    // Apply Visual Studio theme
    ui_theme_apply_catppuccin_frappe();

    // Initialize ImGui persistence (must be after simgui_setup and ConfigFlags)
    imgui_storage_init();

    // Initialize render target
    render_target_init(&state.viewport_rt);

    // Initialize camera
    orbit_camera_init(&state.camera);

    // Initialize renderables
    static_cube_init(&state.cube);
    dynamic_lines_init(&state.lines);
    instanced_lines_init(&state.instanced_lines);
    instanced_polylines_init(&state.instanced_polylines);
    instanced_lines_alpha_init(&state.alpha_lines);

    // Load G-code file
    //state.gcode_loaded = instanced_alpha_polylines_init(&state.gcode_polyline, "models/gcode/3DBenchy.gcode");
    state.gcode_loaded = instanced_alpha_polylines_init(&state.gcode_polyline, "models/gcode/Triceratops.gcode");

    // Initialize UI modules
    ui_controls_init(&state.controls, &state.camera, &state.offscreen_pass_action);
    ui_viewport_init(&state.viewport, &state.viewport_rt, &state.camera);
    ui_camera_debug_init(&state.camera_debug, &state.camera);
    ui_lines_controls_init(&state.lines_controls, &state.lines);
    ui_thick_lines_controls_init(&state.thick_lines_controls, &state.instanced_lines, &state.instanced_polylines);
    ui_visibility_init(&state.visibility);
    ui_gcode_controls_init(&state.gcode_controls,
        state.gcode_loaded ? &state.gcode_polyline : NULL,
        &state.alpha_lines);

    // Main pass action (just clear to dark gray)
    state.main_pass_action = (sg_pass_action){
        .colors[0] = { .load_action = SG_LOADACTION_CLEAR, .clear_value = { 0.1f, 0.1f, 0.1f, 1.0f } }
    };
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

    const int width = sapp_width();
    const int height = sapp_height();

    // Update camera begin frame (check if mouse buttons released)
    ImGuiIO* io = igGetIO_Nil();
    bool any_mouse_down = io->MouseDown[0] || io->MouseDown[1] || io->MouseDown[2];
    orbit_camera_begin_frame(&state.camera, any_mouse_down);

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
    ui_camera_debug_draw(&state.camera_debug);
    ui_lines_controls_draw(&state.lines_controls);
    ui_thick_lines_controls_draw(&state.thick_lines_controls);
    ui_visibility_draw(&state.visibility);
    ui_gcode_controls_draw(&state.gcode_controls);

    // Update camera (apply inertia after UI has processed input)
    orbit_camera_update(&state.camera, dt);

    // Update elapsed time for animation
    state.elapsed_time += dt;

    //=== RENDER TO OFFSCREEN TARGET ===

    int vp_width = state.viewport_rt.width;
    int vp_height = state.viewport_rt.height;

    // Get view matrix from orbital camera
    mat4_t view = orbit_camera_get_view_matrix(&state.camera);
    mat4_t proj = mat4_perspective(0.785398f, (float)vp_width / (float)vp_height, 0.1f, 100.0f);
    mat4_t vp_mat = mat4_mul(proj, view);
    mat4_t mvp = mat4_mul(vp_mat, mat4_identity());

    // Update dynamic lines vertex buffer (always update even if hidden, for smooth animation)
    dynamic_lines_update(&state.lines, state.elapsed_time);
    instanced_lines_update(&state.instanced_lines, state.elapsed_time);
    instanced_polylines_update(&state.instanced_polylines, state.elapsed_time);
    instanced_lines_alpha_update(&state.alpha_lines, state.elapsed_time);
    if (state.gcode_loaded) {
        instanced_alpha_polylines_update(&state.gcode_polyline);
    }

    // Calculate aspect ratio for thick line rendering
    float aspect_ratio = (float)vp_width / (float)vp_height;

    // Offscreen pass - render visible objects
    sg_begin_pass(&(sg_pass){
        .action = state.offscreen_pass_action,
        .attachments = {
            .colors[0] = state.viewport_rt.color_att_view,
            .depth_stencil = state.viewport_rt.depth_att_view,
        }
    });

    // Draw cube if visible
    if (state.visibility.show_cube) {
        static_cube_draw(&state.cube, mvp);
    }

    // Draw dynamic lines if visible
    if (state.visibility.show_lines) {
        dynamic_lines_draw(&state.lines, mvp);
    }

    // Draw instanced thick lines if visible
    if (state.visibility.show_instanced_lines) {
        instanced_lines_draw(&state.instanced_lines, mvp, aspect_ratio);
    }

    // Draw instanced polylines if visible
    if (state.visibility.show_instanced_polylines) {
        instanced_polylines_draw(&state.instanced_polylines, mvp, aspect_ratio);
    }

    // Draw alpha-blended objects (after opaque objects)
    // G-code path
    if (state.visibility.show_gcode_path && state.gcode_loaded) {
        instanced_alpha_polylines_draw(&state.gcode_polyline, mvp, aspect_ratio);
    }

    // Alpha-blended animated lines
    if (state.visibility.show_alpha_lines) {
        instanced_lines_alpha_draw(&state.alpha_lines, mvp, aspect_ratio);
    }

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
    static_cube_shutdown(&state.cube);
    dynamic_lines_shutdown(&state.lines);
    instanced_lines_shutdown(&state.instanced_lines);
    instanced_polylines_shutdown(&state.instanced_polylines);
    instanced_lines_alpha_shutdown(&state.alpha_lines);
    if (state.gcode_loaded) {
        instanced_alpha_polylines_shutdown(&state.gcode_polyline);
    }
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
        .high_dpi = true,
        .icon.sokol_default = true,
        .logger.func = slog_func,
    };
}
