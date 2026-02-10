//------------------------------------------------------------------------------
// ui_about.h - Help -> About window (header-only)
//
// Shows application info, license, and third-party attributions.
//------------------------------------------------------------------------------
#ifndef UI_ABOUT_H
#define UI_ABOUT_H

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"

#include <stdbool.h>

//------------------------------------------------------------------------------
// Types
//------------------------------------------------------------------------------

typedef struct {
    bool window_open;
} ui_about_state_t;

//------------------------------------------------------------------------------
// Initialization
//------------------------------------------------------------------------------

static inline void ui_about_init(ui_about_state_t *state) {
    state->window_open = false;
}

//------------------------------------------------------------------------------
// Help menu (call from menu bar)
//------------------------------------------------------------------------------

static inline void ui_about_draw_menu(ui_about_state_t *state) {
    if (igBeginMenu("Help", true)) {
        if (igMenuItem_Bool("About mdCAD", NULL, false, true)) {
            state->window_open = true;
        }
        igEndMenu();
    }
}

//------------------------------------------------------------------------------
// About window
//------------------------------------------------------------------------------

static inline void ui_about_draw(ui_about_state_t *state) {
    if (!state->window_open) return;

    igSetNextWindowSize((ImVec2){480, 0}, ImGuiCond_FirstUseEver);
    if (!igBegin("About mdCAD", &state->window_open, ImGuiWindowFlags_NoCollapse)) {
        igEnd();
        return;
    }

    // App info
    igText("mdCAD");
    igSameLine(0, -1);
    igTextDisabled("v0.1.0");
    igSeparator();

    igText("Licensed under the MIT License");
    igText("Copyright (c) 2026 Rodion Radchenko and mdCAD contributors");
    igSpacing();
    igSeparator();
    igSpacing();

    // Third-party libraries
    igText("Third-Party Libraries");
    igSpacing();

    // Sokol
    if (igCollapsingHeader_TreeNodeFlags("Sokol  (zlib/libpng)", ImGuiTreeNodeFlags_None)) {
        igText("Copyright (c) 2018 Andre Weissflog");
        igTextDisabled("https://github.com/floooh/sokol");
        igSpacing();
        igTextWrapped("zlib/libpng license. This software is provided 'as-is', without "
                      "any express or implied warranty. Permission is granted to anyone to "
                      "use this software for any purpose, including commercial applications, "
                      "and to alter it and redistribute it freely, subject to the restrictions "
                      "in the full license text.");
        igSpacing();
    }

    // Dear ImGui
    if (igCollapsingHeader_TreeNodeFlags("Dear ImGui  (MIT)", ImGuiTreeNodeFlags_None)) {
        igText("Copyright (c) 2014-2024 Omar Cornut");
        igTextDisabled("https://github.com/ocornut/imgui");
        igSpacing();
    }

    // cimgui
    if (igCollapsingHeader_TreeNodeFlags("cimgui  (MIT)", ImGuiTreeNodeFlags_None)) {
        igText("Copyright (c) 2015 Stephan Dilly");
        igTextDisabled("https://github.com/cimgui/cimgui");
        igSpacing();
    }

    // Flecs
    if (igCollapsingHeader_TreeNodeFlags("Flecs  (MIT)", ImGuiTreeNodeFlags_None)) {
        igText("Copyright (c) 2019 Sander Mertens");
        igTextDisabled("https://github.com/SanderMertens/flecs");
        igSpacing();
    }

    // cJSON
    if (igCollapsingHeader_TreeNodeFlags("cJSON  (MIT)", ImGuiTreeNodeFlags_None)) {
        igText("Copyright (c) 2009-2017 Dave Gamble and cJSON contributors");
        igTextDisabled("https://github.com/DaveGamble/cJSON");
        igSpacing();
    }

    igSeparator();
    igSpacing();
    igTextDisabled("See THIRD_PARTY_LICENSES.md for full license texts.");
    igSpacing();

    // Close button
    float button_width = 120.0f;
    float avail_width = igGetContentRegionAvail().x;
    igSetCursorPosX(igGetCursorPosX() + (avail_width - button_width) * 0.5f);
    if (igButton("Close", (ImVec2){button_width, 0})) {
        state->window_open = false;
    }

    igEnd();
}

#endif // UI_ABOUT_H
