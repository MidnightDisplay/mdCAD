//------------------------------------------------------------------------------
// ui_viewport.h - 3D Viewport window UI (header-only)
//------------------------------------------------------------------------------
#ifndef UI_VIEWPORT_H
#define UI_VIEWPORT_H

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"
#include "sokol_imgui.h"
#include "sokol_app.h"
#include "../platform.h"
#include "../render_target.h"
#include "../orbit_camera.h"

//------------------------------------------------------------------------------
// Types
//------------------------------------------------------------------------------
typedef struct {
    render_target_t* render_target;  // Pointer to the render target to display
    orbit_camera_t* camera;          // Pointer to the orbital camera
    int content_width;
    int content_height;

    // For GPU picking
    float window_pos_x;              // Window content position (screen coords)
    float window_pos_y;
    bool hovered;                    // Is viewport image hovered this frame

    // For selection
    bool clicked;                    // Was viewport clicked this frame (left button)
    bool shift_held;                 // Is shift key held during click
    bool ctrl_held;                  // Is ctrl/cmd key held during click
} ui_viewport_state_t;

//------------------------------------------------------------------------------
// Functions
//------------------------------------------------------------------------------

// Initialize viewport state
static inline void ui_viewport_init(ui_viewport_state_t* vp, render_target_t* rt, orbit_camera_t* cam) {
    vp->render_target = rt;
    vp->camera = cam;
    vp->content_width = 0;
    vp->content_height = 0;
    vp->window_pos_x = 0;
    vp->window_pos_y = 0;
    vp->hovered = false;
    vp->clicked = false;
    vp->shift_held = false;
    vp->ctrl_held = false;
}

// Draw the 3D Viewport window
// Returns true if viewport size changed (caller may need to re-render at new size)
static inline bool ui_viewport_draw(ui_viewport_state_t* vp) {
    bool size_changed = false;

    igPushStyleVar_Vec2(ImGuiStyleVar_WindowPadding, (ImVec2){0, 0});
    igBegin("3D Viewport", NULL, ImGuiWindowFlags_None);

    // Get available content region size (in logical/point units)
    ImVec2 content_size = igGetContentRegionAvail();
    int logical_width = (int)content_size.x;
    int logical_height = (int)content_size.y;

    // Ensure minimum size
    if (logical_width < RENDER_TARGET_MIN_SIZE) logical_width = RENDER_TARGET_MIN_SIZE;
    if (logical_height < RENDER_TARGET_MIN_SIZE) logical_height = RENDER_TARGET_MIN_SIZE;

    // Scale by DPI for sharp rendering on HiDPI displays
    float dpi_scale = sapp_dpi_scale();
    int rt_width = (int)(logical_width * dpi_scale);
    int rt_height = (int)(logical_height * dpi_scale);

    // Check if size changed
    if (logical_width != vp->content_width || logical_height != vp->content_height) {
        vp->content_width = logical_width;
        vp->content_height = logical_height;
        render_target_resize(vp->render_target, rt_width, rt_height);
        size_changed = true;
    }

    // Get the content region screen position (for pick buffer coordinate conversion)
    ImVec2_c cursor_screen_pos = igGetCursorScreenPos();
    vp->window_pos_x = cursor_screen_pos.x;
    vp->window_pos_y = cursor_screen_pos.y;

    // Display the rendered image using ImGui
    uint64_t tex_id = simgui_imtextureid_with_sampler(
        vp->render_target->tex_view,
        vp->render_target->sampler
    );
    ImTextureRef_c tex_ref = { ._TexID = tex_id };

    // OpenGL framebuffers are Y-flipped relative to screen coordinates
    // Metal/D3D: (0,0) is top-left, OpenGL: (0,0) is bottom-left
#if defined(SOKOL_GLCORE) || defined(SOKOL_GLES3)
    ImVec2_c uv0 = {0, 1};  // Flip V coordinates for OpenGL
    ImVec2_c uv1 = {1, 0};
#else
    ImVec2_c uv0 = {0, 0};
    ImVec2_c uv1 = {1, 1};
#endif
    igImage(tex_ref, (ImVec2_c){(float)logical_width, (float)logical_height}, uv0, uv1);

    // Handle camera input when mouse is over the viewport image
    vp->hovered = igIsItemHovered(ImGuiHoveredFlags_None);

    // Get IO state
    ImGuiIO* io = igGetIO_Nil();
    bool shift = (io->KeyMods & ImGuiMod_Shift) != 0;
    bool ctrl = (io->KeyMods & ImGuiMod_Ctrl) != 0;

    // Detect click on viewport (mouse button just pressed while hovering)
    // Only register click if not dragging (mouse hasn't moved significantly)
    vp->clicked = false;
    vp->shift_held = false;
    vp->ctrl_held = false;

    if (vp->hovered && igIsMouseClicked_Bool(0, false)) {
        // Check if this is a simple click (not the start of a drag)
        // We'll confirm the click when mouse is released in the same spot
        // For now, register click on mouse down
        vp->clicked = true;
        vp->shift_held = shift;
        vp->ctrl_held = ctrl;
    }

    if (vp->camera) {
        bool hovered = vp->hovered;

        float dx = io->MouseDelta.x;
        float dy = io->MouseDelta.y;
        float wheel = io->MouseWheel;
        bool left = io->MouseDown[0];
        bool middle = io->MouseDown[2];

        orbit_camera_handle_input(vp->camera, hovered, dx, dy, wheel, left, middle, shift);
    }

    igEnd();
    igPopStyleVar(1);

    return size_changed;
}

// Get current viewport dimensions
static inline void ui_viewport_get_size(ui_viewport_state_t* vp, int* width, int* height) {
    *width = vp->content_width;
    *height = vp->content_height;
}

#endif // UI_VIEWPORT_H
