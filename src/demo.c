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
#include "gamepad_input.h"
#include "ui/ui_theme.h"
#include "ui/ui_controls.h"
#include "ui/ui_viewport.h"
#include "ui/ui_camera_debug.h"
#include "ui/ui_lines_controls.h"
#include "ui/ui_thick_lines_controls.h"
#include "ui/ui_visibility.h"
#include "ui/ui_gcode_controls.h"

// ECS modules
#include "ecs/ecs_world.h"
#include "ecs/ecs_scene.h"
#include "selection.h"
#include "gpu/pick_buffer.h"
#include "ui/ui_pick_debug.h"
#include "ui/ui_entity_inspector.h"
#include "ui/ui_scene_hierarchy.h"  // Includes undo_redo_exec.h
#include "ui/ui_slot_buffer_debug.h"

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
    gamepad_state_t gamepad;
    uint64_t last_time;
    float elapsed_time;
    bool ui_visible;

    // UI state
    ui_controls_state_t controls;
    ui_viewport_state_t viewport;
    ui_camera_debug_state_t camera_debug;
    ui_lines_controls_state_t lines_controls;
    ui_thick_lines_controls_state_t thick_lines_controls;
    ui_visibility_state_t visibility;
    ui_gcode_controls_state_t gcode_controls;

    // ECS state
    ecs_world_state_t ecs_world;
    ecs_scene_t ecs_scene;

    // Selection state
    selection_buffer_t selection;

    // GPU Picking state
    pick_buffer_t pick_buffer;
    ui_pick_debug_state_t pick_debug;

    // Entity management UI
    ui_entity_inspector_state_t entity_inspector;
    ui_scene_hierarchy_state_t scene_hierarchy;

    // Debug UI
    ui_slot_buffer_debug_state_t slot_buffer_debug;

    // Undo/Redo system
    undo_redo_t undo_redo;
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

    // Initialize camera and gamepad
    orbit_camera_init(&state.camera);
    gamepad_init(&state.gamepad);
    state.ui_visible = true;

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

    // Initialize ECS world and scene
    ecs_world_init(&state.ecs_world);
    ecs_scene_init(&state.ecs_scene, &state.ecs_world);

    // Initialize selection buffer
    selection_init(&state.selection, &state.ecs_world);

    // Initialize GPU picking
    pick_buffer_init(&state.pick_buffer);
    ui_pick_debug_init(&state.pick_debug, &state.pick_buffer);

    // Wire up pick debug window toggle to visibility controls
    ui_visibility_set_pick_debug_ptr(&state.visibility, &state.pick_debug.window_open);

    // Wire up ECS thickness controls to visibility panel
    ui_visibility_set_ecs_thickness_ptrs(&state.visibility,
        &state.ecs_scene.batches.lines.line_width,
        &state.ecs_scene.batches.points.point_size,
        &state.pick_buffer.thickness_multiplier);

    // Wire up selection count to visibility panel
    ui_visibility_set_selection_count_ptr(&state.visibility, &state.selection.count);

    // Initialize entity management UI
    ui_entity_inspector_init(&state.entity_inspector, &state.selection, &state.ecs_world);
    ui_scene_hierarchy_init(&state.scene_hierarchy, &state.selection, &state.ecs_scene);

    // Initialize slot buffer debug viewer
    ui_slot_buffer_debug_init(&state.slot_buffer_debug, &state.ecs_scene);

    // Wire up slot buffer debug window toggle to visibility controls
    ui_visibility_set_slot_buffer_debug_ptr(&state.visibility, &state.slot_buffer_debug.window_open);

    // Initialize undo/redo system
    undo_redo_init(&state.undo_redo, &state.ecs_scene, 100);
    undo_redo_set_selection(&state.undo_redo, &state.selection);

    // Wire up undo/redo to scene hierarchy and entity inspector
    ui_scene_hierarchy_set_undo_redo(&state.scene_hierarchy, &state.undo_redo);
    ui_entity_inspector_set_undo_redo(&state.entity_inspector, &state.undo_redo);

    // Create test ECS entities using the scene API
    {
        // RGB axis lines (visible in 3D viewport)
        scene_add_line(&state.ecs_scene,
            vec3_make(-2.0f, 0.0f, 0.0f), vec3_make(2.0f, 0.0f, 0.0f),
            vec4_make(1.0f, 0.2f, 0.2f, 1.0f), 0.03f);  // Red X axis

        scene_add_line(&state.ecs_scene,
            vec3_make(0.0f, -2.0f, 0.0f), vec3_make(0.0f, 2.0f, 0.0f),
            vec4_make(0.2f, 1.0f, 0.2f, 1.0f), 0.03f);  // Green Y axis

        scene_add_line(&state.ecs_scene,
            vec3_make(0.0f, 0.0f, -2.0f), vec3_make(0.0f, 0.0f, 2.0f),
            vec4_make(0.2f, 0.2f, 1.0f, 1.0f), 0.03f);  // Blue Z axis

        // Test points at axis endpoints
        scene_add_point(&state.ecs_scene,
            vec3_make(2.0f, 0.0f, 0.0f), vec4_make(1.0f, 0.4f, 0.4f, 1.0f), 0.08f);  // +X
        scene_add_point(&state.ecs_scene,
            vec3_make(0.0f, 2.0f, 0.0f), vec4_make(0.4f, 1.0f, 0.4f, 1.0f), 0.08f);  // +Y
        scene_add_point(&state.ecs_scene,
            vec3_make(0.0f, 0.0f, 2.0f), vec4_make(0.4f, 0.4f, 1.0f, 1.0f), 0.08f);  // +Z

        // Test hierarchy: Create a parent point with child lines
        // Moving the parent's TransformComp.position should move all children with it
        ecs_entity_t parent_point = scene_add_point(&state.ecs_scene,
            vec3_make(0.0f, 0.0f, 0.0f), vec4_make(1.0f, 0.8f, 0.0f, 1.0f), 0.12f);  // Yellow parent (geometry at origin)

        // Set parent's transform position (this is what children will inherit)
        scene_set_position(&state.ecs_scene, parent_point, vec3_make(1.5f, 1.5f, 0.0f));

        // Add child lines forming a small cross (defined relative to parent's origin)
        ecs_entity_t child_x = scene_add_line(&state.ecs_scene,
            vec3_make(-0.5f, 0.0f, 0.0f), vec3_make(0.5f, 0.0f, 0.0f),
            vec4_make(1.0f, 0.6f, 0.0f, 1.0f), 0.025f);  // Orange horizontal

        ecs_entity_t child_y = scene_add_line(&state.ecs_scene,
            vec3_make(0.0f, -0.5f, 0.0f), vec3_make(0.0f, 0.5f, 0.0f),
            vec4_make(0.8f, 0.4f, 0.0f, 1.0f), 0.025f);  // Dark orange vertical

        // Set parent-child relationships
        scene_set_parent(&state.ecs_scene, child_x, parent_point);
        scene_set_parent(&state.ecs_scene, child_y, parent_point);

        // Children's geometry is relative to parent.
        // With parent at (1.5, 1.5, 0), child_x will render from (1.0, 1.5, 0) to (2.0, 1.5, 0)
        // and child_y from (1.5, 1.0, 0) to (1.5, 2.0, 0)
        // Edit the parent's position in Entity Inspector to see children move together!
    }

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

    // Update gamepad state
    gamepad_begin_frame(&state.gamepad);
    gamepad_update(&state.gamepad, dt);

    // Apply gamepad input to camera
    if (state.gamepad.connected) {
        float orbit_x, orbit_y, pan_x, pan_y, zoom;
        gamepad_get_orbit_input(&state.gamepad, &orbit_x, &orbit_y);
        gamepad_get_pan_input(&state.gamepad, &pan_x, &pan_y);
        zoom = gamepad_get_zoom_input(&state.gamepad);

        orbit_camera_apply_gamepad(&state.camera, orbit_x, orbit_y, pan_x, pan_y, zoom, dt);

        // Camera reset (L1 + R1 held)
        if (gamepad_should_reset(&state.gamepad)) {
            orbit_camera_reset(&state.camera);
        }

        // A button: Toggle UI visibility
        if (gamepad_button_pressed(&state.gamepad, GAMEPAD_BUTTON_A)) {
            state.ui_visible = !state.ui_visible;
        }

        // Y button: Cycle themes
        if (gamepad_button_pressed(&state.gamepad, GAMEPAD_BUTTON_Y)) {
            static int current_theme = 0;
            current_theme = (current_theme + 1) % 3;
            switch (current_theme) {
                case 0: ui_theme_apply_catppuccin_frappe(); break;
                case 1: ui_theme_apply_visual_studio(); break;
                case 2: ui_theme_apply_ios_light(); break;
            }
            // Sync clear color with theme
            float r, g, b;
            ui_theme_get_frame_bg(&r, &g, &b);
            state.offscreen_pass_action.colors[0].clear_value = (sg_color){r, g, b, 1.0f};
        }

        // Start button: Toggle all UI panels
        if (gamepad_button_pressed(&state.gamepad, GAMEPAD_BUTTON_START)) {
            state.ui_visible = !state.ui_visible;
        }

        // D-pad up/down: Timeline scrub (if G-code loaded)
        if (state.gcode_loaded) {
            if (gamepad_button_held(&state.gamepad, GAMEPAD_BUTTON_DPAD_UP)) {
                state.gcode_polyline.timeline_position += dt * 0.2f;
                if (state.gcode_polyline.timeline_position > 1.0f)
                    state.gcode_polyline.timeline_position = 1.0f;
            }
            if (gamepad_button_held(&state.gamepad, GAMEPAD_BUTTON_DPAD_DOWN)) {
                state.gcode_polyline.timeline_position -= dt * 0.2f;
                if (state.gcode_polyline.timeline_position < 0.0f)
                    state.gcode_polyline.timeline_position = 0.0f;
            }
        }
    }

    simgui_new_frame(&(simgui_frame_desc_t){
        .width = width,
        .height = height,
        .delta_time = sapp_frame_duration(),
        .dpi_scale = sapp_dpi_scale(),
    });

    //=== UI ===
    igDockSpaceOverViewport(0, NULL, ImGuiDockNodeFlags_None, NULL);

    // Draw UI panels if visible
    if (state.ui_visible) {
        ui_controls_draw(&state.controls);
        ui_camera_debug_draw(&state.camera_debug);
        ui_lines_controls_draw(&state.lines_controls);
        ui_thick_lines_controls_draw(&state.thick_lines_controls);
        ui_visibility_draw(&state.visibility);
        ui_gcode_controls_draw(&state.gcode_controls);
        ui_pick_debug_draw(&state.pick_debug);
        ui_entity_inspector_draw(&state.entity_inspector);
        ui_scene_hierarchy_draw(&state.scene_hierarchy);
        ui_slot_buffer_debug_draw(&state.slot_buffer_debug);
    }

    // Handle keyboard shortcuts when no text input has focus
    if (!io->WantCaptureKeyboard) {
        // Undo: Ctrl+Z
        if (io->KeyCtrl && !io->KeyShift && igIsKeyPressed_Bool(ImGuiKey_Z, false)) {
            if (undo_redo_undo(&state.undo_redo)) {
                ui_scene_hierarchy_mark_dirty(&state.scene_hierarchy);
            }
        }

        // Redo: Ctrl+Shift+Z or Ctrl+Y
        if ((io->KeyCtrl && io->KeyShift && igIsKeyPressed_Bool(ImGuiKey_Z, false)) ||
            (io->KeyCtrl && igIsKeyPressed_Bool(ImGuiKey_Y, false))) {
            if (undo_redo_redo(&state.undo_redo)) {
                ui_scene_hierarchy_mark_dirty(&state.scene_hierarchy);
            }
        }

        // Delete: Delete or Backspace (macOS) deletes selected entities
        if (igIsKeyPressed_Bool(ImGuiKey_Delete, false) ||
            igIsKeyPressed_Bool(ImGuiKey_Backspace, false)) {
            // Copy selection to temp array since we'll be modifying it
            int count = state.selection.count;
            if (count > 0) {
                ecs_entity_t *to_delete = (ecs_entity_t*)malloc(count * sizeof(ecs_entity_t));
                selection_copy_entities(&state.selection, to_delete, count);

                // Record undo commands BEFORE deleting (in reverse order so undo restores in correct order)
                for (int i = count - 1; i >= 0; i--) {
                    undo_cmd_delete_entity(&state.undo_redo, to_delete[i]);
                }

                // Clear selection first (before deleting entities)
                selection_clear(&state.selection);

                // Delete all entities
                for (int i = 0; i < count; i++) {
                    scene_remove_entity(&state.ecs_scene, to_delete[i]);
                }
                free(to_delete);

                // Mark scene hierarchy cache dirty
                ui_scene_hierarchy_mark_dirty(&state.scene_hierarchy);
            }
        }
    }

    // Viewport is always drawn (contains the 3D content)
    ui_viewport_draw(&state.viewport);

    // Update camera (apply inertia after UI has processed input)
    orbit_camera_update(&state.camera, dt);

    // Update ECS world and scene
    ecs_world_progress(&state.ecs_world, dt);
    ecs_scene_update(&state.ecs_scene);

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

    // Draw ECS scene entities (lines, points, etc.)
    if (state.visibility.show_ecs_entities) {
        ecs_scene_draw(&state.ecs_scene, mvp, aspect_ratio);
    }

    sg_end_pass();

    //=== GPU PICKING PASS ===
    // Handle clicks when viewport is hovered but outside the valid picking area
    // or when ECS entities are not visible
    if (state.viewport.clicked && !state.visibility.show_ecs_entities) {
        // Clicked on viewport with ECS hidden - clear selection unless modifiers held
        selection_handle_click(&state.selection, 0,
                               state.viewport.shift_held,
                               state.viewport.ctrl_held);
    }

    // Update pick buffer center from mouse position (relative to viewport)
    if (state.viewport.hovered) {
        float mouse_x = io->MousePos.x;
        float mouse_y = io->MousePos.y;

        // Use logical viewport size (not DPI-scaled render target size) for coordinate conversion
        // Mouse coordinates and window_pos are in logical screen coordinates
        float logical_vp_width = (float)state.viewport.content_width;
        float logical_vp_height = (float)state.viewport.content_height;

        // Convert to normalized viewport coordinates (0-1)
        float vp_x = (mouse_x - state.viewport.window_pos_x) / logical_vp_width;
        float vp_y = (mouse_y - state.viewport.window_pos_y) / logical_vp_height;

        // Clamp to viewport bounds
        if (vp_x >= 0.0f && vp_x <= 1.0f && vp_y >= 0.0f && vp_y <= 1.0f) {
            // Pass the actual render target dimensions for proper zoom calculation
            pick_buffer_set_center(&state.pick_buffer, vp_x, vp_y, (float)vp_width, (float)vp_height);

            // Populate pick buffer with ECS entities (only if visible)
            pick_buffer_begin_frame(&state.pick_buffer);
            if (state.visibility.show_ecs_entities) {
                ecs_scene_populate_pick_buffer(&state.ecs_scene, &state.pick_buffer);
            }

            // Render pick pass
            pick_buffer_render(&state.pick_buffer, view, proj);

            // Readback and update hover state (Note: readback not yet implemented)
            pick_buffer_readback(&state.pick_buffer);
            pick_buffer_update_hover(&state.pick_buffer);

            // Update ECS hover state based on pick result
            ecs_scene_update_hover(&state.ecs_scene, &state.pick_buffer);

            // Handle click selection
            if (state.viewport.clicked) {
                uint32_t pick_id = pick_buffer_get_hovered_id(&state.pick_buffer);
                ecs_entity_t clicked_entity = ecs_scene_find_entity_by_pick_id(&state.ecs_scene, pick_id);
                selection_handle_click(&state.selection, clicked_entity,
                                       state.viewport.shift_held,
                                       state.viewport.ctrl_held);
            }
        }
    }

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

    // Shutdown undo/redo system
    undo_redo_shutdown(&state.undo_redo);

    // Shutdown GPU picking
    pick_buffer_shutdown(&state.pick_buffer);

    // Shutdown selection buffer
    selection_shutdown(&state.selection);

    // Shutdown scene hierarchy (frees cache)
    ui_scene_hierarchy_shutdown(&state.scene_hierarchy);

    // Shutdown ECS scene and world
    ecs_scene_shutdown(&state.ecs_scene);
    ecs_world_shutdown(&state.ecs_world);

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
    // Handle gamepad input first
    gamepad_handle_event(&state.gamepad, ev);

    // Handle app suspend (Android/iOS) - save ImGui settings
    if (ev->type == SAPP_EVENTTYPE_SUSPENDED) {
        imgui_storage_mark_should_save();
    }

    // Forward to ImGui
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
