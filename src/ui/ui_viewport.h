//------------------------------------------------------------------------------
// ui_viewport.h - 3D Viewport window UI (header-only)
//------------------------------------------------------------------------------
#ifndef UI_VIEWPORT_H
#define UI_VIEWPORT_H

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"
#include "sokol_imgui.h"
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
}

// Draw the 3D Viewport window
// Returns true if viewport size changed (caller may need to re-render at new size)
static inline bool ui_viewport_draw(ui_viewport_state_t* vp) {
    bool size_changed = false;

    igPushStyleVar_Vec2(ImGuiStyleVar_WindowPadding, (ImVec2){0, 0});
    igBegin("3D Viewport", NULL, ImGuiWindowFlags_None);

    // Get available content region size
    ImVec2 content_size = igGetContentRegionAvail();
    int vp_width = (int)content_size.x;
    int vp_height = (int)content_size.y;

    // Ensure minimum size
    if (vp_width < RENDER_TARGET_MIN_SIZE) vp_width = RENDER_TARGET_MIN_SIZE;
    if (vp_height < RENDER_TARGET_MIN_SIZE) vp_height = RENDER_TARGET_MIN_SIZE;

    // Check if size changed
    if (vp_width != vp->content_width || vp_height != vp->content_height) {
        vp->content_width = vp_width;
        vp->content_height = vp_height;
        render_target_resize(vp->render_target, vp_width, vp_height);
        size_changed = true;
    }

    // Display the rendered image using ImGui
    uint64_t tex_id = simgui_imtextureid_with_sampler(
        vp->render_target->tex_view,
        vp->render_target->sampler
    );
    ImTextureRef_c tex_ref = { ._TexID = tex_id };
    igImage(tex_ref, (ImVec2_c){(float)vp_width, (float)vp_height},
            (ImVec2_c){0, 0}, (ImVec2_c){1, 1});

    // Handle camera input when mouse is over the viewport image
    if (vp->camera) {
        bool hovered = igIsItemHovered(ImGuiHoveredFlags_None);

        ImGuiIO* io = igGetIO_Nil();
        float dx = io->MouseDelta.x;
        float dy = io->MouseDelta.y;
        float wheel = io->MouseWheel;
        bool left = io->MouseDown[0];
        bool middle = io->MouseDown[2];
        bool shift = (io->KeyMods & ImGuiMod_Shift) != 0;

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
