//------------------------------------------------------------------------------
// Sokol + cimgui + flecs app with dockspace and 3D viewport
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
#include "math/cglm_entry.h"
#include "math/math_interaction.h"
#include "imgui_storage.h"
#include "render_target.h"
#include "orbit_camera.h"
#include "ui/ui_theme.h"
#include "ui/ui_controls.h"
#include "ui/ui_viewport.h"
#include "ui/ui_camera_debug.h"
#include "ui/ui_visibility.h"

// ECS modules
#include "ecs/ecs_world.h"
#include "ecs/ecs_scene.h"
#include "selection.h"
#include "gpu/pick_buffer.h"
#include "ui/ui_pick_debug.h"
#include "ui/ui_entity_inspector.h"
#include "ui/ui_scene_hierarchy.h"  // Includes undo_redo_exec.h
#include "ui/ui_slot_buffer_debug.h"
#include "ui/ui_fps_debug.h"
#include "constraints/constraint_types.h"
#include "constraints/constraint_glyphs.h"
#include "constraints/constraint_selection.h"
#include "scripting/sketch_script_emit.h"

// Gizmo system
#include "gizmo/gizmo.h"

#include <string.h>

//------------------------------------------------------------------------------
// Application state
//------------------------------------------------------------------------------
static struct {
    // Rendering
    render_target_t viewport_rt;
    sg_pass_action offscreen_pass_action;
    sg_pass_action main_pass_action;

    // Scene state
    orbit_camera_t camera;
    uint64_t last_time;
    float elapsed_time;
    bool ui_visible;

    // UI state
    ui_controls_state_t controls;
    ui_viewport_state_t viewport;
    ui_camera_debug_state_t camera_debug;
    ui_visibility_state_t visibility;

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
    ui_fps_debug_state_t fps_debug;

    // Undo/Redo system
    undo_redo_t undo_redo;

    // Lighting
    bool lighting_enabled;

    // Gizmo system
    gizmo_t gizmo;
    bool gizmo_drag_active;
    // Transform mode drag snapshots
    vec3_t *gizmo_drag_start_positions;
    ecs_entity_t *gizmo_drag_entities;
    int gizmo_drag_entity_count;
    // Geometry mode drag snapshots
    vec3_t *gizmo_drag_start_vertices;
    int *gizmo_drag_vertex_indices;
    int gizmo_drag_vertex_count;

    // Constraint authoring UX
    constraint_glyph_state_t constraint_glyphs;
    bool constraint_menu_open;
    bool constraint_menu_open_request;
    ImVec2 constraint_menu_anchor;
    ecs_entity_t constraint_menu_sketch;
    constraint_participant_descriptor_t constraint_menu_participants[CONSTRAINT_MAX_PARTICIPANTS];
    uint32_t constraint_menu_participant_count;
    constraint_selection_signature_t constraint_menu_signature;
    ecs_entity_t selected_constraint_entity;
    bool constraint_dimension_popup_open_request;
    ecs_entity_t constraint_dimension_popup_constraint;
    float constraint_dimension_popup_value;
    bool constraint_dimension_popup_open;
    bool solver_drag_block_toast_active;
    uint64_t solver_drag_block_toast_expires_ms;
    ecs_entity_t script_editor_sketch;
    bool script_editor_open;
    bool script_editor_initialized;
    bool script_editor_has_unsaved_edits;
    bool script_editor_close_pending;
    bool script_editor_reset_pending;
    bool script_editor_apply_requested;
    bool script_editor_preview_ok;
    bool script_editor_last_apply_failed;
    uint64_t script_editor_last_seen_emit_revision;
    char script_editor_text[16384];
    char script_editor_committed_text[16384];
    sketch_script_error_t script_editor_last_error;
    ecs_entity_t script_io_sketch;
    bool script_io_open;
    uint64_t script_io_last_seen_emit_revision;
    sketch_script_error_t script_io_last_error;
    bool script_io_slider_drag_active;
    ecs_entity_t script_io_slider_drag_sketch;
    char script_io_slider_drag_input_id[SCRIPT_LOCAL_ID_MAX];
    char script_io_slider_drag_before_script[16384];
} state;

static void mdcad_script_editor_clear_error(void) {
    state.script_editor_last_error.line = 0;
    state.script_editor_last_error.column = 0;
    state.script_editor_last_error.message[0] = '\0';
    state.script_editor_last_apply_failed = false;
}

static void mdcad_script_io_clear_error(void) {
    state.script_io_last_error.line = 0;
    state.script_io_last_error.column = 0;
    state.script_io_last_error.message[0] = '\0';
}

static bool mdcad_script_editor_load_emitted_script(ecs_entity_t sketch, bool overwrite_text) {
    if (sketch == 0) return false;
    char emitted[sizeof(state.script_editor_committed_text)] = {0};
    sketch_script_error_t emit_err = {0};
    if (!scene_script_emit_for_sketch(&state.ecs_scene, sketch, emitted, sizeof(emitted), &emit_err)) {
        state.script_editor_last_error = emit_err;
        return false;
    }
    snprintf(state.script_editor_committed_text, sizeof(state.script_editor_committed_text), "%s", emitted);
    state.script_editor_committed_text[sizeof(state.script_editor_committed_text) - 1] = '\0';
    if (overwrite_text) {
        snprintf(state.script_editor_text, sizeof(state.script_editor_text), "%s", emitted);
        state.script_editor_text[sizeof(state.script_editor_text) - 1] = '\0';
        state.script_editor_has_unsaved_edits = false;
    }
    return true;
}

static void mdcad_script_editor_open_for_sketch(ecs_entity_t sketch) {
    if (sketch == 0) return;
    state.script_editor_sketch = sketch;
    state.script_editor_open = true;
    state.script_editor_initialized = true;
    state.script_editor_close_pending = false;
    state.script_editor_reset_pending = false;
    state.script_editor_apply_requested = false;
    state.script_editor_preview_ok = false;
    state.script_editor_last_seen_emit_revision = scene_script_emit_revision(&state.ecs_scene);
    mdcad_script_editor_clear_error();
    state.script_editor_text[0] = '\0';
    state.script_editor_committed_text[0] = '\0';
    mdcad_script_editor_load_emitted_script(sketch, true);
}

static void mdcad_script_io_open_for_sketch(ecs_entity_t sketch) {
    if (sketch == 0) return;
    state.script_io_sketch = sketch;
    state.script_io_open = true;
    state.script_io_last_seen_emit_revision = scene_script_emit_revision(&state.ecs_scene);
    state.script_io_slider_drag_active = false;
    state.script_io_slider_drag_sketch = 0;
    state.script_io_slider_drag_input_id[0] = '\0';
    state.script_io_slider_drag_before_script[0] = '\0';
    mdcad_script_io_clear_error();
}

static void mdcad_draw_script_editor_window(void) {
    ecs_entity_t requested_sketch = 0;
    if (ui_entity_inspector_consume_script_editor_open_request(&state.entity_inspector, &requested_sketch)) {
        mdcad_script_editor_open_for_sketch(requested_sketch);
    }

    if (!state.script_editor_open || state.script_editor_sketch == 0) return;
    if (!ecs_is_alive(state.ecs_world.world, state.script_editor_sketch)) {
        state.script_editor_open = false;
        state.script_editor_sketch = 0;
        return;
    }

    uint64_t current_revision = scene_script_emit_revision(&state.ecs_scene);
    if (current_revision != state.script_editor_last_seen_emit_revision && !state.script_editor_has_unsaved_edits) {
        state.script_editor_last_seen_emit_revision = current_revision;
        mdcad_script_editor_load_emitted_script(state.script_editor_sketch, true);
    }

    bool open = state.script_editor_open;
    igSetNextWindowSize((ImVec2){900.0f, 680.0f}, ImGuiCond_FirstUseEver);
    if (igBegin("Script Editor", &open, ImGuiWindowFlags_None)) {
        LabelComp *label = ecs_world_get_label(&state.ecs_world, state.script_editor_sketch);
        const char *sketch_name = (label && label->name[0]) ? label->name : "Sketch";
        igText("Sketch: %s (#%llu)", sketch_name, (unsigned long long)state.script_editor_sketch);
        igDummy((ImVec2){0.0f, 8.0f});

        bool edited = igInputTextMultiline("##script_editor_text",
                                           state.script_editor_text,
                                           sizeof(state.script_editor_text),
                                           (ImVec2){-1.0f, 420.0f},
                                           ImGuiInputTextFlags_AllowTabInput,
                                           NULL,
                                           NULL);
        if (edited) {
            state.script_editor_has_unsaved_edits = strcmp(state.script_editor_text, state.script_editor_committed_text) != 0;
            state.script_editor_last_apply_failed = false;
        }

        sketch_script_error_t preview_error = {0};
        state.script_editor_preview_ok = scene_script_preview_parse(&state.ecs_scene,
                                                                    state.script_editor_sketch,
                                                                    state.script_editor_text,
                                                                    &preview_error);
        if (!state.script_editor_preview_ok) {
            state.script_editor_last_error = preview_error;
        } else if (!state.script_editor_apply_requested) {
            if (!state.script_editor_last_apply_failed) {
                mdcad_script_editor_clear_error();
            }
        }

        igDummy((ImVec2){0.0f, 8.0f});
        igTextDisabled("Diagnostics");
        if (state.script_editor_text[0] == '\0') {
            igText("No script content yet");
            igTextWrapped("Start by editing sketch geometry or constraints, then generated script will appear here.");
        } else if (!state.script_editor_preview_ok || state.script_editor_last_apply_failed) {
            igTextWrapped("Script validation/apply failed. Review diagnostics, fix highlighted lines, then run Apply Script again.");
            igTextWrapped("%s", state.script_editor_last_error.message[0] ? state.script_editor_last_error.message : "Unknown parse error.");
        } else {
            igTextDisabled("Preview parse: OK");
        }

        igDummy((ImVec2){0.0f, 8.0f});
        if (igButton("Reset to Emitted Script##script_editor_reset", (ImVec2){220.0f, 0.0f})) {
            if (state.script_editor_has_unsaved_edits) {
                state.script_editor_reset_pending = true;
                igOpenPopup_Str("Reset Script Editor##script_editor_reset_popup", 0);
            } else {
                mdcad_script_editor_load_emitted_script(state.script_editor_sketch, true);
                mdcad_script_editor_clear_error();
            }
        }
        igSameLine(0, 8);
        igBeginDisabled(!state.script_editor_preview_ok);
        if (igButton("Apply Script", (ImVec2){180.0f, 0.0f})) {
            state.script_editor_apply_requested = true;
            sketch_script_error_t apply_error = {0};
            bool applied = scene_script_apply_commit(&state.ecs_scene,
                                                     state.script_editor_sketch,
                                                     state.script_editor_text,
                                                     &apply_error);
            if (applied) {
                state.script_editor_last_seen_emit_revision = scene_script_emit_revision(&state.ecs_scene);
                mdcad_script_editor_load_emitted_script(state.script_editor_sketch, true);
                mdcad_script_editor_clear_error();
            } else {
                state.script_editor_last_error = apply_error;
                state.script_editor_last_apply_failed = true;
            }
        }
        igEndDisabled();

        if (igBeginPopupModal("Reset Script Editor##script_editor_reset_popup", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            igTextWrapped("Discard current edits and restore last deterministic script output?");
            igDummy((ImVec2){0.0f, 8.0f});
            if (igButton("Reset##script_editor_reset_confirm", (ImVec2){160.0f, 0.0f})) {
                mdcad_script_editor_load_emitted_script(state.script_editor_sketch, true);
                mdcad_script_editor_clear_error();
                state.script_editor_reset_pending = false;
                igCloseCurrentPopup();
            }
            igSameLine(0, 8);
            if (igButton("Cancel##script_editor_reset_cancel", (ImVec2){120.0f, 0.0f})) {
                state.script_editor_reset_pending = false;
                igCloseCurrentPopup();
            }
            igEndPopup();
        }
    }
    igEnd();

    state.script_editor_apply_requested = false;
    if (!open) {
        if (state.script_editor_has_unsaved_edits) {
            state.script_editor_close_pending = true;
            igOpenPopup_Str("Close Script Editor##script_editor_close_popup", 0);
            state.script_editor_open = true;
        } else {
            state.script_editor_open = false;
            state.script_editor_sketch = 0;
        }
    }

    if (igBeginPopupModal("Close Script Editor##script_editor_close_popup", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        igTextWrapped("Discard unapplied script edits and close the Script Editor?");
        igDummy((ImVec2){0.0f, 8.0f});
        if (igButton("Discard and Close##script_editor_close_confirm", (ImVec2){190.0f, 0.0f})) {
            state.script_editor_open = false;
            state.script_editor_sketch = 0;
            state.script_editor_has_unsaved_edits = false;
            state.script_editor_close_pending = false;
            igCloseCurrentPopup();
        }
        igSameLine(0, 8);
        if (igButton("Cancel##script_editor_close_cancel", (ImVec2){120.0f, 0.0f})) {
            state.script_editor_close_pending = false;
            state.script_editor_open = true;
            igCloseCurrentPopup();
        }
        igEndPopup();
    }
}

static void mdcad_draw_script_io_window(void) {
    ecs_entity_t requested_sketch = 0;
    if (ui_entity_inspector_consume_script_io_open_request(&state.entity_inspector, &requested_sketch)) {
        mdcad_script_io_open_for_sketch(requested_sketch);
    }

    if (!state.script_io_open || state.script_io_sketch == 0) return;
    if (!ecs_is_alive(state.ecs_world.world, state.script_io_sketch)) {
        state.script_io_open = false;
        state.script_io_sketch = 0;
        return;
    }

    bool open = state.script_io_open;
    bool slider_drag_active_this_frame = false;
    igSetNextWindowSize((ImVec2){520.0f, 420.0f}, ImGuiCond_FirstUseEver);
    if (igBegin("Script IO", &open, ImGuiWindowFlags_None)) {
        LabelComp *label = ecs_world_get_label(&state.ecs_world, state.script_io_sketch);
        const char *sketch_name = (label && label->name[0]) ? label->name : "Sketch";
        igText("Sketch: %s (#%llu)", sketch_name, (unsigned long long)state.script_io_sketch);
        igDummy((ImVec2){0.0f, 8.0f});

        int descriptor_count = scene_script_io_descriptor_count(&state.ecs_scene, state.script_io_sketch);
        if (descriptor_count <= 0) {
            igTextDisabled("No script IO inputs/outputs available for this sketch.");
        } else {
            igTextDisabled("Inputs");
            for (int i = 0; i < descriptor_count; i++) {
                scene_script_io_descriptor_t desc = {0};
                if (!scene_script_io_descriptor_at(&state.ecs_scene, state.script_io_sketch, i, &desc)) continue;
                if (!desc.is_input) continue;

                igPushID_Int(i);
                igText("%s", desc.id);

                double edited_value = desc.value;
                bool changed = false;
                bool slider_active = false;
                bool input_active = false;
                if (desc.has_min && desc.has_max) {
                    float slider_value = (float)edited_value;
                    if (igSliderFloat("##script_io_input_slider",
                                      &slider_value,
                                      (float)desc.min_value,
                                      (float)desc.max_value,
                                      "%.6g",
                                      ImGuiSliderFlags_None)) {
                        edited_value = (double)slider_value;
                        changed = true;
                    }
                    slider_active = igIsItemActive();
                }

                double step = desc.has_step ? desc.step_value : 0.0;
                if (igInputDouble("##script_io_input_value",
                                  &edited_value,
                                  step,
                                  step > 0.0 ? step * 10.0 : 0.0,
                                  "%.6f",
                                  ImGuiInputTextFlags_None)) {
                    changed = true;
                }
                input_active = igIsItemActive();

                bool interaction_active = slider_active || input_active;
                bool session_active_for_item = state.script_io_slider_drag_active &&
                                               state.script_io_slider_drag_sketch == state.script_io_sketch &&
                                               strcmp(state.script_io_slider_drag_input_id, desc.id) == 0;
                if (interaction_active && !session_active_for_item) {
                    sketch_script_error_t emit_error = {0};
                    char before_script[16384] = {0};
                    if (scene_script_emit_for_sketch(&state.ecs_scene,
                                                     state.script_io_sketch,
                                                     before_script,
                                                     sizeof(before_script),
                                                     &emit_error)) {
                        state.script_io_slider_drag_active = true;
                        state.script_io_slider_drag_sketch = state.script_io_sketch;
                        snprintf(state.script_io_slider_drag_input_id,
                                 sizeof(state.script_io_slider_drag_input_id),
                                 "%s",
                                 desc.id);
                        state.script_io_slider_drag_input_id[sizeof(state.script_io_slider_drag_input_id) - 1] = '\0';
                        snprintf(state.script_io_slider_drag_before_script,
                                 sizeof(state.script_io_slider_drag_before_script),
                                 "%s",
                                 before_script);
                        state.script_io_slider_drag_before_script[sizeof(state.script_io_slider_drag_before_script) - 1] = '\0';
                        session_active_for_item = true;
                    }
                }
                if (session_active_for_item && interaction_active) {
                    slider_drag_active_this_frame = true;
                }

                if (changed) {
                    sketch_script_error_t apply_error = {0};
                    bool prev_undo_suppressed = state.ecs_scene.script_apply_undo_suppressed;
                    if (session_active_for_item) {
                        state.ecs_scene.script_apply_undo_suppressed = true;
                    }
                    if (scene_script_io_apply_input_value(&state.ecs_scene,
                                                          state.script_io_sketch,
                                                          desc.id,
                                                          edited_value,
                                                          &apply_error)) {
                        state.script_io_last_seen_emit_revision = scene_script_emit_revision(&state.ecs_scene);
                        mdcad_script_io_clear_error();
                    } else {
                        state.script_io_last_error = apply_error;
                        state.script_io_slider_drag_active = false;
                        state.script_io_slider_drag_sketch = 0;
                        state.script_io_slider_drag_input_id[0] = '\0';
                        state.script_io_slider_drag_before_script[0] = '\0';
                    }
                    if (session_active_for_item) {
                        state.ecs_scene.script_apply_undo_suppressed = prev_undo_suppressed;
                    }
                }
                igPopID();
            }

            igDummy((ImVec2){0.0f, 8.0f});
            igSeparator();
            igDummy((ImVec2){0.0f, 8.0f});
            igTextDisabled("Outputs");
            for (int i = 0; i < descriptor_count; i++) {
                scene_script_io_descriptor_t desc = {0};
                if (!scene_script_io_descriptor_at(&state.ecs_scene, state.script_io_sketch, i, &desc)) continue;
                if (desc.is_input) continue;

                igPushID_Int(10000 + i);
                igText("%s", desc.id);
                double output_value = desc.value;
                igBeginDisabled(true);
                igInputDouble("##script_io_output_value",
                              &output_value,
                              0.0,
                              0.0,
                              "%.6f",
                              ImGuiInputTextFlags_ReadOnly);
                igEndDisabled();
                igPopID();
            }
        }

        igDummy((ImVec2){0.0f, 8.0f});
        igTextDisabled("Diagnostics");
        if (state.script_io_last_error.message[0]) {
            igTextWrapped("%s", state.script_io_last_error.message);
        } else {
            igTextDisabled("No errors");
        }
    }
    igEnd();

    if (state.script_io_slider_drag_active) {
        if (!slider_drag_active_this_frame ||
            state.script_io_slider_drag_sketch != state.script_io_sketch) {
            char after_script[16384] = {0};
            sketch_script_error_t emit_error = {0};
            if (scene_script_emit_for_sketch(&state.ecs_scene,
                                             state.script_io_slider_drag_sketch,
                                             after_script,
                                             sizeof(after_script),
                                             &emit_error)) {
                if (strcmp(state.script_io_slider_drag_before_script, after_script) != 0) {
                    undo_cmd_script_apply_transaction(&state.undo_redo,
                                                      state.script_io_slider_drag_sketch,
                                                      state.script_io_slider_drag_before_script,
                                                      after_script);
                }
            }
            state.script_io_slider_drag_active = false;
            state.script_io_slider_drag_sketch = 0;
            state.script_io_slider_drag_input_id[0] = '\0';
            state.script_io_slider_drag_before_script[0] = '\0';
        }
    }

    if (!open) {
        state.script_io_open = false;
        state.script_io_sketch = 0;
        state.script_io_slider_drag_active = false;
        state.script_io_slider_drag_sketch = 0;
        state.script_io_slider_drag_input_id[0] = '\0';
        state.script_io_slider_drag_before_script[0] = '\0';
    }
}

static mat4_t mdcad_mat4_bridge_from_cglm(mat4s matrix) {
    mat4_t bridge;

    memcpy(bridge.m, matrix.raw, sizeof(bridge.m));
    return bridge;
}

static bool mdcad_collect_constraint_context(ecs_scene_t *scene,
                                             selection_buffer_t *selection,
                                             ecs_entity_t *out_sketch,
                                             constraint_participant_descriptor_t *out_participants,
                                             uint32_t *out_participant_count,
                                             constraint_selection_signature_t *out_sig) {
    if (!scene || !selection || !out_sketch || !out_participants || !out_participant_count || !out_sig) {
        return false;
    }

    *out_sketch = 0;
    *out_participant_count = 0;
    memset(out_sig, 0, sizeof(*out_sig));

    for (int i = 0; i < selection->count; i++) {
        ecs_entity_t selected = selection->entities[i];
        if (!ecs_is_alive(scene->world->world, selected)) continue;

        if (scene_is_sketch(scene, selected)) {
            if (*out_sketch == 0) {
                *out_sketch = selected;
            } else if (*out_sketch != selected) {
                return false;
            }
            continue;
        }

        EndPointsComp *endpoint_meta = ecs_world_get_endpoints(scene->world, selected);
        bool endpoint_point_selected = endpoint_meta && endpoint_meta->is_endpoint_point;

        ecs_entity_t participant_entity = selected;
        constraint_participant_role_t participant_role = CONSTRAINT_PARTICIPANT_ROLE_ENTITY;
        uint8_t participant_sub_index = 0;

        if (endpoint_point_selected) {
            if (!endpoints_comp_is_supported_role(endpoint_meta->role)) return false;
            participant_entity = (ecs_entity_t)endpoint_meta->owner_entity;
            participant_role = endpoint_meta->role;
            participant_sub_index = endpoint_meta->sub_index;
        }

        GeometryComp *g = ecs_world_get_geometry(scene->world, participant_entity);
        if (!g) return false;

        ecs_entity_t sketch = scene_get_parent(scene, participant_entity);
        if (!scene_is_sketch(scene, sketch)) return false;
        if (*out_sketch == 0) {
            *out_sketch = sketch;
        } else if (*out_sketch != sketch) {
            return false;
        }

        bool already_added = false;
        for (uint32_t p = 0; p < *out_participant_count; p++) {
            if ((ecs_entity_t)out_participants[p].entity == participant_entity &&
                out_participants[p].role == participant_role &&
                out_participants[p].sub_index == participant_sub_index) {
                already_added = true;
                break;
            }
        }
        if (already_added) continue;
        if (*out_participant_count >= CONSTRAINT_MAX_PARTICIPANTS) return false;

        out_participants[*out_participant_count] =
            constraint_participant_descriptor_make((uint64_t)participant_entity,
                                                   (uint8_t)participant_role,
                                                   participant_sub_index);
        out_sig->geometry_types[*out_participant_count] = g->type;
        out_sig->roles[*out_participant_count] = participant_role;
        (*out_participant_count)++;
        out_sig->count = *out_participant_count;
    }

    return *out_sketch != 0;
}

static bool mdcad_seed_default_sketch_script_io(ecs_entity_t sketch) {
    char emitted[16384] = {0};
    sketch_script_error_t error = {0};
    if (!scene_script_emit_for_sketch(&state.ecs_scene, sketch, emitted, sizeof(emitted), &error)) {
        return false;
    }

    const char *entities_marker = "  entities = {\n";
    char *entities_pos = strstr(emitted, entities_marker);
    if (!entities_pos) {
        return false;
    }

    const char *io_block =
        "  inputs = {\n"
        "    { id = \"front_span\", value = 1.5, min = 0.5, max = 3.0, step = 0.05 },\n"
        "    { id = \"hole_depth\", value = 1.0, min = 0.25, max = 2.0, step = 0.05 },\n"
        "    { id = \"include_cube_connectors\", value = 1.0, min = 0.0, max = 1.0, step = 1.0 },\n"
        "    { id = \"include_hole_connectors\", value = 1.0, min = 0.0, max = 1.0, step = 1.0 }\n"
        "  },\n"
        "  outputs = {\n"
        "    { id = \"body_depth\", value = 1.0 },\n"
        "    { id = \"hole_diameter\", value = 0.7 }\n"
        "  },\n";

    char seeded_script[16384] = {0};
    size_t prefix_len = (size_t)(entities_pos - emitted);
    int written = snprintf(seeded_script,
                           sizeof(seeded_script),
                           "%.*s%s%s",
                           (int)prefix_len,
                           emitted,
                           io_block,
                           entities_pos);
    if (written <= 0 || (size_t)written >= sizeof(seeded_script)) {
        return false;
    }

    bool prev_undo_suppressed = state.ecs_scene.script_apply_undo_suppressed;
    state.ecs_scene.script_apply_undo_suppressed = true;
    bool applied = scene_script_apply_commit(&state.ecs_scene, sketch, seeded_script, &error);
    state.ecs_scene.script_apply_undo_suppressed = prev_undo_suppressed;
    return applied;
}
static void mdcad_seed_default_sketch_scene(void) {
    const vec4_t sketch_color = vec4_make(0.90f, 0.90f, 0.95f, 1.0f);
    ecs_entity_t sketch = scene_add_sketch(&state.ecs_scene,
                                           "StartupSketch",
                                           "Default startup sketch: rounded cube wireframe with through-hole.",
                                           sketch_color);
    if (sketch == 0) return;

    const float z_front = 0.5f;
    const float z_back = -0.5f;
    const float half_extent = 1.0f;
    const float fillet = 0.25f;
    const float top_y = half_extent;
    const float bottom_y = -half_extent;
    const float left_x = -half_extent;
    const float right_x = half_extent;
    const float inner_x = half_extent - fillet;
    const float inner_y = half_extent - fillet;

    ecs_entity_t f_top = scene_add_line_to_sketch(&state.ecs_scene, sketch,
        vec3_make(-inner_x, top_y, z_front), vec3_make(inner_x, top_y, z_front), sketch_color, 0.03f);
    ecs_entity_t f_bottom = scene_add_line_to_sketch(&state.ecs_scene, sketch,
        vec3_make(-inner_x, bottom_y, z_front), vec3_make(inner_x, bottom_y, z_front), sketch_color, 0.03f);
    ecs_entity_t f_left = scene_add_line_to_sketch(&state.ecs_scene, sketch,
        vec3_make(left_x, -inner_y, z_front), vec3_make(left_x, inner_y, z_front), sketch_color, 0.03f);
    ecs_entity_t f_right = scene_add_line_to_sketch(&state.ecs_scene, sketch,
        vec3_make(right_x, -inner_y, z_front), vec3_make(right_x, inner_y, z_front), sketch_color, 0.03f);

    scene_add_arc_to_sketch(&state.ecs_scene, sketch,
        vec3_make(-inner_x, inner_y, z_front), fillet, 3.14159265f, 1.57079633f, vec3_make(0.0f, 0.0f, 1.0f),
        sketch_color, 0.03f);
    scene_add_arc_to_sketch(&state.ecs_scene, sketch,
        vec3_make(inner_x, inner_y, z_front), fillet, 1.57079633f, 0.0f, vec3_make(0.0f, 0.0f, 1.0f),
        sketch_color, 0.03f);
    scene_add_arc_to_sketch(&state.ecs_scene, sketch,
        vec3_make(inner_x, -inner_y, z_front), fillet, 0.0f, -1.57079633f, vec3_make(0.0f, 0.0f, 1.0f),
        sketch_color, 0.03f);
    scene_add_arc_to_sketch(&state.ecs_scene, sketch,
        vec3_make(-inner_x, -inner_y, z_front), fillet, -1.57079633f, -3.14159265f, vec3_make(0.0f, 0.0f, 1.0f),
        sketch_color, 0.03f);

    ecs_entity_t b_top = scene_add_line_to_sketch(&state.ecs_scene, sketch,
        vec3_make(-inner_x, top_y, z_back), vec3_make(inner_x, top_y, z_back), sketch_color, 0.03f);
    ecs_entity_t b_bottom = scene_add_line_to_sketch(&state.ecs_scene, sketch,
        vec3_make(-inner_x, bottom_y, z_back), vec3_make(inner_x, bottom_y, z_back), sketch_color, 0.03f);
    ecs_entity_t b_left = scene_add_line_to_sketch(&state.ecs_scene, sketch,
        vec3_make(left_x, -inner_y, z_back), vec3_make(left_x, inner_y, z_back), sketch_color, 0.03f);
    ecs_entity_t b_right = scene_add_line_to_sketch(&state.ecs_scene, sketch,
        vec3_make(right_x, -inner_y, z_back), vec3_make(right_x, inner_y, z_back), sketch_color, 0.03f);

    scene_add_arc_to_sketch(&state.ecs_scene, sketch,
        vec3_make(-inner_x, inner_y, z_back), fillet, 3.14159265f, 1.57079633f, vec3_make(0.0f, 0.0f, 1.0f),
        sketch_color, 0.03f);
    scene_add_arc_to_sketch(&state.ecs_scene, sketch,
        vec3_make(inner_x, inner_y, z_back), fillet, 1.57079633f, 0.0f, vec3_make(0.0f, 0.0f, 1.0f),
        sketch_color, 0.03f);
    scene_add_arc_to_sketch(&state.ecs_scene, sketch,
        vec3_make(inner_x, -inner_y, z_back), fillet, 0.0f, -1.57079633f, vec3_make(0.0f, 0.0f, 1.0f),
        sketch_color, 0.03f);
    scene_add_arc_to_sketch(&state.ecs_scene, sketch,
        vec3_make(-inner_x, -inner_y, z_back), fillet, -1.57079633f, -3.14159265f, vec3_make(0.0f, 0.0f, 1.0f),
        sketch_color, 0.03f);

    ecs_entity_t cube_conn_1 = scene_add_line_to_sketch(&state.ecs_scene, sketch,
        vec3_make(left_x, inner_y, z_front), vec3_make(left_x, inner_y, z_back), sketch_color, 0.03f);
    ecs_entity_t cube_conn_2 = scene_add_line_to_sketch(&state.ecs_scene, sketch,
        vec3_make(right_x, inner_y, z_front), vec3_make(right_x, inner_y, z_back), sketch_color, 0.03f);
    ecs_entity_t cube_conn_3 = scene_add_line_to_sketch(&state.ecs_scene, sketch,
        vec3_make(right_x, -inner_y, z_front), vec3_make(right_x, -inner_y, z_back), sketch_color, 0.03f);
    ecs_entity_t cube_conn_4 = scene_add_line_to_sketch(&state.ecs_scene, sketch,
        vec3_make(left_x, -inner_y, z_front), vec3_make(left_x, -inner_y, z_back), sketch_color, 0.03f);

    const float hole_r = 0.35f;
    ecs_entity_t hole_front = scene_add_arc_to_sketch(&state.ecs_scene, sketch,
        vec3_make(0.0f, 0.0f, z_front), hole_r, 0.0f, 6.28318531f, vec3_make(0.0f, 0.0f, 1.0f),
        sketch_color, 0.03f);
    ecs_entity_t hole_back = scene_add_arc_to_sketch(&state.ecs_scene, sketch,
        vec3_make(0.0f, 0.0f, z_back), hole_r, 0.0f, 6.28318531f, vec3_make(0.0f, 0.0f, 1.0f),
        sketch_color, 0.03f);

    ecs_entity_t hole_conn_1 = scene_add_line_to_sketch(&state.ecs_scene, sketch,
        vec3_make(hole_r, 0.0f, z_front), vec3_make(hole_r, 0.0f, z_back), sketch_color, 0.03f);
    ecs_entity_t hole_conn_2 = scene_add_line_to_sketch(&state.ecs_scene, sketch,
        vec3_make(-hole_r, 0.0f, z_front), vec3_make(-hole_r, 0.0f, z_back), sketch_color, 0.03f);
    ecs_entity_t hole_conn_3 = scene_add_line_to_sketch(&state.ecs_scene, sketch,
        vec3_make(0.0f, hole_r, z_front), vec3_make(0.0f, hole_r, z_back), sketch_color, 0.03f);
    ecs_entity_t hole_conn_4 = scene_add_line_to_sketch(&state.ecs_scene, sketch,
        vec3_make(0.0f, -hole_r, z_front), vec3_make(0.0f, -hole_r, z_back), sketch_color, 0.03f);

    ecs_entity_t parallel_set_1[] = { cube_conn_1, cube_conn_2, cube_conn_3, cube_conn_4 };
    scene_add_constraint_to_sketch(&state.ecs_scene, sketch, CONSTRAINT_PARALLEL,
                                   parallel_set_1, 4, 0.0f, false);

    ecs_entity_t parallel_set_2[] = { hole_conn_1, hole_conn_2, hole_conn_3, hole_conn_4 };
    scene_add_constraint_to_sketch(&state.ecs_scene, sketch, CONSTRAINT_PARALLEL,
                                   parallel_set_2, 4, 0.0f, false);

    ecs_entity_t side_lines[] = { f_left, f_right, b_left, b_right };
    scene_add_constraint_to_sketch(&state.ecs_scene, sketch, CONSTRAINT_PARALLEL,
                                   side_lines, 4, 0.0f, false);

    if (f_top != 0) {
        scene_add_constraint_to_sketch(&state.ecs_scene, sketch, CONSTRAINT_LENGTH, &f_top, 1, 1.5f, false);
    }
    if (hole_conn_1 != 0) {
        scene_add_constraint_to_sketch(&state.ecs_scene, sketch, CONSTRAINT_LENGTH, &hole_conn_1, 1, 1.0f, false);
        scene_add_constraint_to_sketch(&state.ecs_scene, sketch, CONSTRAINT_ALONG_Z, &hole_conn_1, 1, 0.0f, false);
    }
    if (hole_front != 0 && hole_back != 0) {
        ecs_entity_t hole_pair[] = { hole_front, hole_back };
        scene_add_constraint_to_sketch(&state.ecs_scene, sketch, CONSTRAINT_CORADIAL, hole_pair, 2, 0.0f, false);
        scene_add_constraint_to_sketch(&state.ecs_scene, sketch, CONSTRAINT_CONCENTRIC, hole_pair, 2, 0.0f, false);
    }
    if (f_bottom != 0) {
        scene_add_constraint_to_sketch(&state.ecs_scene, sketch, CONSTRAINT_FIXED, &f_bottom, 1, 0.0f, false);
    }

    scene_refresh_sketch_metadata(&state.ecs_scene, sketch);
    (void)mdcad_seed_default_sketch_script_io(sketch);
}

static void mdcad_draw_constraint_context_menu(void) {
    if (!state.constraint_menu_open_request && !state.constraint_menu_open) return;

    if (state.constraint_menu_open_request) {
        state.constraint_menu_open = true;
        state.constraint_menu_open_request = false;
    }

    if (!state.constraint_menu_open) return;
    if (!ecs_is_alive(state.ecs_world.world, state.constraint_menu_sketch)) {
        state.constraint_menu_open = false;
        return;
    }

    igSetNextWindowPos(state.constraint_menu_anchor, ImGuiCond_Appearing, (ImVec2){0.0f, 0.0f});
    if (!igBegin("Constraint Menu##constraint_context_menu", &state.constraint_menu_open,
                 ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings)) {
        igEnd();
        return;
    }

    bool applied = false;
    int legal_count = 0;
    for (int t = 0; t < CONSTRAINT_TYPE_COUNT; t++) {
        constraint_type_t type = (constraint_type_t)t;
        if (!constraint_type_is_selection_legal(&state.constraint_menu_signature, type)) continue;
        legal_count++;

        if (igMenuItem_Bool(constraint_type_display_name(type), NULL, false, true)) {
            float initial_value = (type == CONSTRAINT_ANGLE) ? 90.0f : 1.0f;
            ecs_entity_t created = scene_add_constraint_to_sketch_with_descriptors(
                &state.ecs_scene,
                state.constraint_menu_sketch,
                type,
                state.constraint_menu_participants,
                state.constraint_menu_participant_count,
                initial_value,
                false);
            if (created != 0) {
                undo_cmd_create_entity(&state.undo_redo, created);
                state.selected_constraint_entity = created;
                if (constraint_type_is_dimensional(type)) {
                    ConstraintComp *created_constraint = ecs_world_get_constraint(&state.ecs_world, created);
                    const constraint_glyph_entry_t *glyph = constraint_glyphs_find_by_constraint(&state.constraint_glyphs, created);
                    if (glyph && glyph->has_screen_anchor) {
                        state.constraint_menu_anchor = (ImVec2){ glyph->anchor_screen_x, glyph->anchor_screen_y };
                    }
                    if (type == CONSTRAINT_LENGTH && created_constraint) {
                        state.constraint_dimension_popup_constraint = created;
                        state.constraint_dimension_popup_value = created_constraint->value;
                        state.constraint_dimension_popup_open_request = true;
                        state.constraint_dimension_popup_open = false;
                    }
                }
                ui_scene_hierarchy_mark_dirty(&state.scene_hierarchy);
                applied = true;
            }
        }
    }

    if (legal_count == 0) {
        if (state.constraint_menu_participant_count == 0) {
            igTextDisabled("Select sketch geometry to apply constraints.");
        } else {
            igTextDisabled("No applicable constraints for current selection.");
        }
    }

    if (igIsKeyPressed_Bool(ImGuiKey_Escape, false)) {
        state.constraint_menu_open = false;
    }
    if (applied) {
        state.constraint_menu_open = false;
    }

    igEnd();
}

static void mdcad_draw_constraint_dimension_popup(void) {
    if (!state.constraint_dimension_popup_open_request && !state.constraint_dimension_popup_open) return;

    if (state.constraint_dimension_popup_open_request) {
        state.constraint_dimension_popup_open = true;
        state.constraint_dimension_popup_open_request = false;
    }
    if (!state.constraint_dimension_popup_open) return;

    if (!ecs_is_alive(state.ecs_world.world, state.constraint_dimension_popup_constraint)) {
        state.constraint_dimension_popup_open = false;
        state.constraint_dimension_popup_constraint = 0;
        return;
    }

    ConstraintComp *constraint = ecs_world_get_constraint(&state.ecs_world, state.constraint_dimension_popup_constraint);
    if (!constraint || !constraint_type_is_dimensional(constraint->type)) {
        state.constraint_dimension_popup_open = false;
        state.constraint_dimension_popup_constraint = 0;
        return;
    }

    ImVec2 popup_anchor = state.constraint_menu_anchor;
    float glyph_x = 0.0f;
    float glyph_y = 0.0f;
    if (constraint_glyphs_get_screen_anchor(&state.constraint_glyphs, state.constraint_dimension_popup_constraint, &glyph_x, &glyph_y)) {
        popup_anchor.x = glyph_x + 16.0f;
        popup_anchor.y = glyph_y + 16.0f;
    }

    igSetNextWindowPos(popup_anchor, ImGuiCond_Appearing, (ImVec2){0.0f, 0.0f});
    if (!igBegin("Edit Dimension##constraint_dimension_popup",
                 &state.constraint_dimension_popup_open,
                 ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings)) {
        igEnd();
        return;
    }

    igText("%s", constraint_type_display_name(constraint->type));
    igSetNextItemWidth(180.0f);
    char value_fmt[16];
    uint8_t decimals = constraint->display_decimals;
    constraint_build_float_format(value_fmt, sizeof(value_fmt), decimals);
    igInputFloat("Value##constraint_dimension_popup_value",
                 &state.constraint_dimension_popup_value, 0.1f, 1.0f, value_fmt,
                 ImGuiInputTextFlags_CharsDecimal);

    bool close_popup = false;
    ImVec2_c popup_pos = igGetWindowPos();
    ImVec2_c popup_size = igGetWindowSize();
    bool mouse_clicked = igIsMouseClicked_Bool(ImGuiMouseButton_Left, false);
    if (state.constraint_dimension_popup_open && mouse_clicked) {
        ImGuiIO *io = igGetIO_Nil();
        float mx = io->MousePos.x;
        float my = io->MousePos.y;
        bool inside_popup =
            mx >= popup_pos.x && mx <= (popup_pos.x + popup_size.x) &&
            my >= popup_pos.y && my <= (popup_pos.y + popup_size.y);
        if (!inside_popup) {
            close_popup = true;
        }
    }

    if (igButton("Accept##constraint_dimension_popup_accept", (ImVec2){100.0f, 0.0f})) {
        scene_constraint_set_dimensional_value(
            &state.ecs_scene,
            state.constraint_dimension_popup_constraint,
            state.constraint_dimension_popup_value,
            constraint->driven);
        close_popup = true;
    }
    if (igIsKeyPressed_Bool(ImGuiKey_Enter, false) || igIsKeyPressed_Bool(ImGuiKey_KeypadEnter, false)) {
        scene_constraint_set_dimensional_value(
            &state.ecs_scene,
            state.constraint_dimension_popup_constraint,
            state.constraint_dimension_popup_value,
            constraint->driven);
        close_popup = true;
    }
    igSameLine(0, 8);
    if (igButton("Cancel##constraint_dimension_popup_cancel", (ImVec2){100.0f, 0.0f})) {
        close_popup = true;
    }
    if (igIsKeyPressed_Bool(ImGuiKey_Escape, false)) {
        close_popup = true;
    }

    if (close_popup) {
        state.constraint_dimension_popup_open = false;
        state.constraint_dimension_popup_constraint = 0;
    }

    igEnd();
}

static uint64_t mdcad_now_ms(void) {
    return (uint64_t)(stm_sec(stm_now()) * 1000.0);
}

static void mdcad_show_solver_drag_block_toast(void) {
    state.solver_drag_block_toast_active = true;
    state.solver_drag_block_toast_expires_ms = mdcad_now_ms() + 1800ULL;
}

static void mdcad_draw_solver_drag_block_toast(void) {
    if (!state.solver_drag_block_toast_active) return;
    uint64_t now_ms = mdcad_now_ms();
    if (now_ms >= state.solver_drag_block_toast_expires_ms) {
        state.solver_drag_block_toast_active = false;
        return;
    }

    ImVec2 viewport_pos = igGetMainViewport()->Pos;
    ImVec2 viewport_size = igGetMainViewport()->Size;
    igSetNextWindowPos((ImVec2){
        viewport_pos.x + viewport_size.x * 0.5f,
        viewport_pos.y + 48.0f
    }, ImGuiCond_Always, (ImVec2){0.5f, 0.0f});
    igSetNextWindowBgAlpha(0.90f);
    if (igBegin("SolverDragToast##solver_drag_blocked", NULL,
                ImGuiWindowFlags_NoDecoration |
                ImGuiWindowFlags_AlwaysAutoResize |
                ImGuiWindowFlags_NoSavedSettings |
                ImGuiWindowFlags_NoNav |
                ImGuiWindowFlags_NoFocusOnAppearing |
                ImGuiWindowFlags_NoMove)) {
        igText("Movement blocked by active constraints.");
    }
    igEnd();
}

static void mdcad_apply_solver_failure_feedback(ecs_entity_t sketch,
                                                const scene_solver_drag_decision_t *decision) {
    if (!decision) return;
    if (!scene_is_sketch(&state.ecs_scene, sketch)) return;
    if (decision->result != SCENE_SOLVER_DRAG_UNSATISFIABLE) return;

    scene_solver_set_failure_implication(&state.ecs_scene,
                                         sketch,
                                         decision->implicated_constraints,
                                         decision->implicated_constraint_count,
                                         decision->block_reason);

    const char *drag_rejected_message = "Drag rejected: active constraints make this move invalid.";
    scene_solver_drag_diagnostic_event_t event;
    if (scene_solver_drag_make_rejected_diagnostic(decision, &event)) {
        snprintf(event.message, sizeof(event.message), "%s", drag_rejected_message);
        scene_solver_add_diagnostic(&state.ecs_scene, sketch, event.severity,
                                    event.timestamp, event.message, event.implicated_constraint);
    }

    ecs_entity_t focus_constraint = decision->first_implicated_constraint;
    if (focus_constraint == 0) {
        const scene_solver_failure_implication_t *imp = scene_solver_failure_implication(&state.ecs_scene);
        if (imp && imp->active) {
            focus_constraint = imp->first_constraint;
        }
    }
    if (focus_constraint != 0) {
        state.selected_constraint_entity = focus_constraint;
        constraint_selection_apply_participants(&state.selection, &state.ecs_world, focus_constraint);
    }
    mdcad_show_solver_drag_block_toast();
}

//------------------------------------------------------------------------------
// Init
//------------------------------------------------------------------------------
static void init(void) {
    stm_setup();
    state.last_time = stm_now();
    mdcad_cglm_compile_anchor();

    sg_setup(&(sg_desc){
        .environment = sglue_environment(),
        .logger.func = slog_func,
#if defined(SOKOL_VULKAN)
        // Vulkan needs larger staging buffer for large point clouds (default 16MB)
        // 1M points * 28 bytes = 28MB, so use 512MB for headroom
        .vulkan.stream_staging_buffer_size = 512 * 1024 * 1024,
#endif
        // Note: Sokol validation is enabled by default in debug builds
        // Errors will be logged via slog_func
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

    // Default startup theme
    ui_theme_apply_visual_studio();

    // Initialize ImGui persistence (must be after simgui_setup and ConfigFlags)
    imgui_storage_init();

    // Initialize render target
    render_target_init(&state.viewport_rt);

    // Initialize camera
    orbit_camera_init(&state.camera);
    state.ui_visible = true;

    // Initialize UI modules
    ui_controls_init(&state.controls, &state.camera, &state.offscreen_pass_action);
    ui_viewport_init(&state.viewport, &state.viewport_rt, &state.camera);
    ui_camera_debug_init(&state.camera_debug, &state.camera);
    ui_visibility_init(&state.visibility);

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
    ui_entity_inspector_set_selected_constraint_ptr(&state.entity_inspector, &state.selected_constraint_entity);

    // Initialize slot buffer debug viewer
    ui_slot_buffer_debug_init(&state.slot_buffer_debug, &state.ecs_scene);

    // Wire up slot buffer debug window toggle to visibility controls
    ui_visibility_set_slot_buffer_debug_ptr(&state.visibility, &state.slot_buffer_debug.window_open);

    // Initialize FPS debug viewer
    ui_fps_debug_init(&state.fps_debug);

    // Wire up FPS debug window toggle to visibility controls
    ui_visibility_set_fps_debug_ptr(&state.visibility, &state.fps_debug.window_open);

    // Initialize undo/redo system
    undo_redo_init(&state.undo_redo, &state.ecs_scene, 100);
    undo_redo_set_selection(&state.undo_redo, &state.selection);
    scene_script_bind_undo_redo(&state.ecs_scene, &state.undo_redo);

    // Wire up undo/redo to scene hierarchy and entity inspector
    ui_scene_hierarchy_set_undo_redo(&state.scene_hierarchy, &state.undo_redo);
    ui_entity_inspector_set_undo_redo(&state.entity_inspector, &state.undo_redo);

    // Initialize gizmo system
    gizmo_init(&state.gizmo);
    constraint_glyphs_init(&state.constraint_glyphs);
    state.constraint_menu_open = false;
    state.constraint_menu_open_request = false;
    state.constraint_menu_sketch = 0;
    state.constraint_menu_participant_count = 0;
    memset(&state.constraint_menu_signature, 0, sizeof(state.constraint_menu_signature));
    state.selected_constraint_entity = 0;
    state.constraint_dimension_popup_open_request = false;
    state.constraint_dimension_popup_open = false;
    state.constraint_dimension_popup_constraint = 0;
    state.constraint_dimension_popup_value = 0.0f;
    state.solver_drag_block_toast_active = false;
    state.solver_drag_block_toast_expires_ms = 0;
    state.script_editor_sketch = 0;
    state.script_editor_open = false;
    state.script_editor_initialized = false;
    state.script_editor_has_unsaved_edits = false;
    state.script_editor_close_pending = false;
    state.script_editor_reset_pending = false;
    state.script_editor_apply_requested = false;
    state.script_editor_preview_ok = false;
    state.script_editor_last_apply_failed = false;
    state.script_editor_last_seen_emit_revision = 0;
    state.script_editor_text[0] = '\0';
    state.script_editor_committed_text[0] = '\0';
    mdcad_script_editor_clear_error();
    state.script_io_sketch = 0;
    state.script_io_open = false;
    state.script_io_last_seen_emit_revision = 0;
    state.script_io_slider_drag_active = false;
    state.script_io_slider_drag_sketch = 0;
    state.script_io_slider_drag_input_id[0] = '\0';
    state.script_io_slider_drag_before_script[0] = '\0';
    mdcad_script_io_clear_error();

    // Create test ECS entities using the scene API
    {
        // ecs_entity_t parent_point = scene_add_point(&state.ecs_scene,
        // vec3_make(0.0f, 0.0f, 0.0f), vec4_make(0.0f, 0.0f, 0.0f, 1.0f), 0.03f);  // White parent (geometry at origin)

        // // RGB axis lines (visible in 3D viewport)
        // ecs_entity_t xAxis = scene_add_line(&state.ecs_scene,
        //     vec3_make(-1.0f, 0.0f, 0.0f), vec3_make(1.0f, 0.0f, 0.0f),
        //     vec4_make(1.0f, 0.2f, 0.2f, 1.0f), 0.03f);  // Red X axis

        // ecs_entity_t yAxis = scene_add_line(&state.ecs_scene,
        //     vec3_make(0.0f, -1.0f, 0.0f), vec3_make(0.0f, 1.0f, 0.0f),
        //     vec4_make(0.2f, 1.0f, 0.2f, 1.0f), 0.03f);  // Green Y axis

        // ecs_entity_t zAxis = scene_add_line(&state.ecs_scene,
        //     vec3_make(0.0f, 0.0f, -1.0f), vec3_make(0.0f, 0.0f, 1.0f),
        //     vec4_make(0.2f, 0.2f, 1.0f, 1.0f), 0.03f);  // Blue Z axis

        // scene_set_parent(&state.ecs_scene, xAxis, parent_point);
        // scene_set_parent(&state.ecs_scene, yAxis, parent_point);
        // scene_set_parent(&state.ecs_scene, zAxis, parent_point);

        // // Sample triangles in the XZ plane
        // scene_add_triangle(&state.ecs_scene,
        //     vec3_make(0.5f, 0.0f, 0.5f),
        //     vec3_make(1.5f, 0.0f, 0.5f),
        //     vec3_make(1.0f, 0.0f, 1.5f),
        //     vec4_make(0.9f, 0.3f, 0.3f, 1.0f));   // Red triangle

        // scene_add_triangle(&state.ecs_scene,
        //     vec3_make(-1.5f, 0.0f, 0.5f),
        //     vec3_make(-0.5f, 0.0f, 0.5f),
        //     vec3_make(-1.0f, 0.0f, 1.5f),
        //     vec4_make(0.3f, 0.9f, 0.3f, 1.0f));   // Green triangle

        // scene_add_triangle(&state.ecs_scene,
        //     vec3_make(-0.5f, 0.0f, -1.5f),
        //     vec3_make(0.5f, 0.0f, -1.5f),
        //     vec3_make(0.0f, 0.8f, -1.0f),
        //     vec4_make(0.3f, 0.3f, 0.9f, 1.0f));   // Blue triangle (tilted up)

        // // Per-vertex colored triangle (RGB gradient)
        // scene_add_triangle_colored(&state.ecs_scene,
        //     vec3_make(-2.0f, 0.0f, -0.5f),
        //     vec3_make(-1.0f, 0.0f, -0.5f),
        //     vec3_make(-1.5f, 0.0f, 0.5f),
        //     vec4_make(1.0f, 0.0f, 0.0f, 1.0f),    // Vertex A: Red
        //     vec4_make(0.0f, 1.0f, 0.0f, 1.0f),    // Vertex B: Green
        //     vec4_make(0.0f, 0.0f, 1.0f, 1.0f));   // Vertex C: Blue

        // // Sample mesh: a quad (2 triangles sharing vertices)
        // scene_add_mesh_quad(&state.ecs_scene,
        //     vec3_make(1.5f, 0.0f, -1.5f),
        //     vec3_make(2.5f, 0.0f, -1.5f),
        //     vec3_make(2.5f, 0.0f, -0.5f),
        //     vec3_make(1.5f, 0.0f, -0.5f),
        //     vec4_make(0.8f, 0.6f, 0.2f, 1.0f));   // Gold quad

        // // Sample mesh: a box (12 triangles, 8 shared vertices)
        // scene_add_mesh_box(&state.ecs_scene,
        //     vec3_make(2.0f, 0.5f, 1.0f),
        //     vec3_make(0.6f, 0.6f, 0.6f),
        //     vec4_make(0.5f, 0.7f, 0.9f, 1.0f));   // Steel blue box

        // // Test points at axis endpoints
        // scene_add_point(&state.ecs_scene,
        //     vec3_make(2.0f, 0.0f, 0.0f), vec4_make(1.0f, 0.4f, 0.4f, 1.0f), 0.08f);  // +X
        // scene_add_point(&state.ecs_scene,
        //     vec3_make(0.0f, 2.0f, 0.0f), vec4_make(0.4f, 1.0f, 0.4f, 1.0f), 0.08f);  // +Y
        // scene_add_point(&state.ecs_scene,
        //     vec3_make(0.0f, 0.0f, 2.0f), vec4_make(0.4f, 0.4f, 1.0f, 1.0f), 0.08f);  // +Z

        // // Test hierarchy: Create a parent point with child lines
        // // Moving the parent's TransformComp.position should move all children with it
        // ecs_entity_t parent_point = scene_add_point(&state.ecs_scene,
        //     vec3_make(0.0f, 0.0f, 0.0f), vec4_make(1.0f, 0.8f, 0.0f, 1.0f), 0.12f);  // Yellow parent (geometry at origin)

        // // Set parent's transform position (this is what children will inherit)
        // scene_set_position(&state.ecs_scene, parent_point, vec3_make(1.5f, 1.5f, 0.0f));

        // // Add child lines forming a small cross (defined relative to parent's origin)
        // ecs_entity_t child_x = scene_add_line(&state.ecs_scene,
        //     vec3_make(-0.5f, 0.0f, 0.0f), vec3_make(0.5f, 0.0f, 0.0f),
        //     vec4_make(1.0f, 0.6f, 0.0f, 1.0f), 0.025f);  // Orange horizontal

        // ecs_entity_t child_y = scene_add_line(&state.ecs_scene,
        //     vec3_make(0.0f, -0.5f, 0.0f), vec3_make(0.0f, 0.5f, 0.0f),
        //     vec4_make(0.8f, 0.4f, 0.0f, 1.0f), 0.025f);  // Dark orange vertical

        // // Set parent-child relationships
        // scene_set_parent(&state.ecs_scene, child_x, parent_point);
        // scene_set_parent(&state.ecs_scene, child_y, parent_point);

        // Children's geometry is relative to parent.
        // With parent at (1.5, 1.5, 0), child_x will render from (1.0, 1.5, 0) to (2.0, 1.5, 0)
        // and child_y from (1.5, 1.0, 0) to (1.5, 2.0, 0)
        // Edit the parent's position in Entity Inspector to see children move together!
    }

    // Seed a default sketch scene for startup UX and quick constraint validation.
    mdcad_seed_default_sketch_scene();
    ui_scene_hierarchy_mark_dirty(&state.scene_hierarchy);

    // Create default 3-point studio lighting
    state.lighting_enabled = true;
    {
        // Key light: warm white, from front-right-above
        scene_add_directional_light(&state.ecs_scene,
            vec3_make(0.5f, -0.7f, -0.5f),
            vec4_make(1.0f, 0.95f, 0.9f, 1.0f), 1.0f);

        // Fill light: cool blue-white, from front-left
        scene_add_directional_light(&state.ecs_scene,
            vec3_make(-0.5f, -0.3f, -0.5f),
            vec4_make(0.8f, 0.85f, 1.0f, 1.0f), 0.4f);

        // Rim light: neutral, from behind
        scene_add_directional_light(&state.ecs_scene,
            vec3_make(0.0f, -0.2f, 0.8f),
            vec4_make(1.0f, 1.0f, 1.0f, 1.0f), 0.3f);
    }

    // Wire up lighting toggle to visibility panel
    ui_visibility_set_lighting_ptr(&state.visibility, &state.lighting_enabled);

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

    // Draw UI panels if visible
    if (state.ui_visible) {
        ui_controls_draw(&state.controls);
        ui_camera_debug_draw(&state.camera_debug);
        ui_visibility_draw(&state.visibility);
        ui_pick_debug_draw(&state.pick_debug);
        ui_entity_inspector_draw(&state.entity_inspector);
        ui_scene_hierarchy_draw(&state.scene_hierarchy);
        ui_slot_buffer_debug_draw(&state.slot_buffer_debug);
        mdcad_draw_script_editor_window();
        mdcad_draw_script_io_window();

        // Update and draw FPS debug (updates every frame, draws if visible)
        ui_fps_debug_update(&state.fps_debug);
        ui_fps_debug_draw(&state.fps_debug);
    }

    mdcad_draw_constraint_context_menu();
    mdcad_draw_constraint_dimension_popup();
    mdcad_draw_solver_drag_block_toast();

    // Handle keyboard shortcuts when no text input has focus
    if (!io->WantCaptureKeyboard) {
        bool shortcut_mod = io->KeyCtrl || io->KeySuper;

        // Undo: Ctrl/Cmd+Z
        if (shortcut_mod && !io->KeyShift && igIsKeyPressed_Bool(ImGuiKey_Z, false)) {
            if (undo_redo_undo(&state.undo_redo)) {
                ui_scene_hierarchy_mark_dirty(&state.scene_hierarchy);
            }
        }

        // Redo: Ctrl/Cmd+Shift+Z or Ctrl/Cmd+Y
        if ((shortcut_mod && io->KeyShift && igIsKeyPressed_Bool(ImGuiKey_Z, false)) ||
            (shortcut_mod && igIsKeyPressed_Bool(ImGuiKey_Y, false))) {
            if (undo_redo_redo(&state.undo_redo)) {
                ui_scene_hierarchy_mark_dirty(&state.scene_hierarchy);
            }
        }

        // Tab: toggle gizmo edit mode (Transform <-> Geometry)
        if (igIsKeyPressed_Bool(ImGuiKey_Tab, false)) {
            gizmo_edit_mode_t new_mode = (state.gizmo.edit_mode == GIZMO_TRANSFORM_MODE)
                ? GIZMO_GEOMETRY_MODE : GIZMO_TRANSFORM_MODE;
            gizmo_set_edit_mode(&state.gizmo, new_mode, &state.ecs_scene, &state.selection);
        }

        // C: open context-aware constraint authoring menu at cursor
        if (igIsKeyPressed_Bool(ImGuiKey_C, false)) {
            ecs_entity_t sketch = 0;
            constraint_participant_descriptor_t participants[CONSTRAINT_MAX_PARTICIPANTS] = {0};
            uint32_t participant_count = 0;
            constraint_selection_signature_t signature;

            if (mdcad_collect_constraint_context(
                    &state.ecs_scene, &state.selection,
                    &sketch, participants, &participant_count, &signature)) {
                state.constraint_menu_sketch = sketch;
                state.constraint_menu_participant_count = participant_count;
                memcpy(state.constraint_menu_participants, participants, sizeof(participants));
                state.constraint_menu_signature = signature;
                state.constraint_menu_anchor = io->MousePos;
                state.constraint_menu_open_request = true;
                state.constraint_menu_open = false;
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

                // Record one atomic delete command so undo can restore coupled side-effects (e.g., constraints).
                undo_cmd_bulk_delete_entities(&state.undo_redo, to_delete, count);

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

    // Suppress camera input when gizmo is active (uses previous-frame hover state)
    state.viewport.suppress_camera_input =
        state.gizmo.hovered_handle != GIZMO_HANDLE_NONE ||
        state.gizmo.hovered_vertex >= 0 ||
        state.gizmo.mode == GIZMO_MODE_DRAGGING;

    // Viewport is always drawn (contains the 3D content)
    ui_viewport_draw(&state.viewport);

    // Update camera (apply inertia after UI has processed input)
    orbit_camera_update(&state.camera, dt);

    // Update ECS world and scene
    ecs_world_progress(&state.ecs_world, dt);
    ecs_scene_update(&state.ecs_scene);

    // Update gizmo (must be after ecs_scene_update for correct world matrices)
    {
        vec3_t cam_eye = orbit_camera_get_eye_position(&state.camera);
        float fov = 0.785398f;  // Must match perspective call below
        float ecs_point_size = state.ecs_scene.batches.points.point_size;
        gizmo_update(&state.gizmo, &state.selection, &state.ecs_scene, cam_eye, fov, ecs_point_size);
    }

    // Update elapsed time for animation
    state.elapsed_time += dt;

    //=== RENDER TO OFFSCREEN TARGET ===

    int vp_width = state.viewport_rt.width;
    int vp_height = state.viewport_rt.height;

    // Calculate aspect ratio for thick line rendering
    float aspect_ratio = (float)vp_width / (float)vp_height;
    mat4s view = orbit_camera_get_view_matrix_cglm(&state.camera);
    mat4s proj = glms_perspective_rh_zo(0.785398f, aspect_ratio, 0.1f, 100.0f);
    mat4s vp = glms_mat4_mul(proj, view);
    mat4s mvp = glms_mat4_mul(vp, GLMS_MAT4_IDENTITY);

    mat4_t view_legacy = mdcad_mat4_bridge_from_cglm(view);
    mat4_t proj_legacy = mdcad_mat4_bridge_from_cglm(proj);
    mat4_t vp_legacy = mdcad_mat4_bridge_from_cglm(vp);
    mat4_t mvp_legacy = mdcad_mat4_bridge_from_cglm(mvp);
    constraint_glyphs_update_screen_anchors(&state.constraint_glyphs, &state.ecs_scene,
        view_legacy, proj_legacy,
        state.viewport.window_pos_x, state.viewport.window_pos_y,
        (float)state.viewport.content_width, (float)state.viewport.content_height);

    // Offscreen pass - render visible objects
    sg_begin_pass(&(sg_pass){
        .action = state.offscreen_pass_action,
        .attachments = {
            .colors[0] = state.viewport_rt.color_att_view,
            .depth_stencil = state.viewport_rt.depth_att_view,
        }
    });

    // Draw ECS scene entities (lines, points, triangles)
    if (state.visibility.show_ecs_entities) {
        // Build triangle shader params with lighting data
        geom_triangle_params_t tri_params = {0};
        tri_params.mvp = mvp_legacy;
        tri_params.lighting_enabled = state.lighting_enabled ? 1.0f : 0.0f;

        // Ambient light
        tri_params.ambient[0] = 0.15f;  // R
        tri_params.ambient[1] = 0.15f;  // G
        tri_params.ambient[2] = 0.15f;  // B
        tri_params.ambient[3] = 1.0f;   // intensity

        // Collect lights from ECS
        scene_light_data_t lights[TRIANGLE_MAX_LIGHTS];
        int light_count = scene_collect_lights(&state.ecs_scene, lights, TRIANGLE_MAX_LIGHTS);
        tri_params.num_lights = (float)light_count;

        for (int i = 0; i < light_count; i++) {
            tri_params.light_dirs[i * 4 + 0] = lights[i].dir_or_pos[0];
            tri_params.light_dirs[i * 4 + 1] = lights[i].dir_or_pos[1];
            tri_params.light_dirs[i * 4 + 2] = lights[i].dir_or_pos[2];
            tri_params.light_dirs[i * 4 + 3] = lights[i].dir_or_pos[3];

            tri_params.light_colors[i * 4 + 0] = lights[i].color[0];
            tri_params.light_colors[i * 4 + 1] = lights[i].color[1];
            tri_params.light_colors[i * 4 + 2] = lights[i].color[2];
            tri_params.light_colors[i * 4 + 3] = lights[i].color[3];
        }

        ecs_scene_draw(&state.ecs_scene, &tri_params, aspect_ratio);
    }

    // Draw gizmo overlay (always on top via depth-always pipeline)
    if (state.gizmo.mode != GIZMO_MODE_HIDDEN) {
        float gizmo_line_width = state.ecs_scene.batches.lines.line_width * 1.5f;
        float gizmo_point_size = state.ecs_scene.batches.points.point_size * 2.0f;
        gizmo_rendering_upload(&state.gizmo.rendering);
        gizmo_rendering_draw(&state.gizmo.rendering, mvp_legacy, aspect_ratio, gizmo_line_width, gizmo_point_size);
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

            // Cursor-gated rebuild: skip expensive pick buffer cycle when nothing changed
            if (pick_buffer_needs_rebuild(&state.pick_buffer, view_legacy, proj_legacy)) {
                // Compute the pick MVP for frustum culling (same matrix used for rendering)
                mat4_t pick_mvp = pick_buffer_compute_mvp(&state.pick_buffer, view_legacy, proj_legacy);

                // Populate pick buffer with ECS entities (only if visible)
                pick_buffer_begin_frame(&state.pick_buffer);
                if (state.visibility.show_ecs_entities) {
                    ecs_scene_populate_pick_buffer(&state.ecs_scene, &state.pick_buffer, pick_mvp);
                }
                // Add gizmo handles + vertex handles to pick buffer
                gizmo_populate_pick_buffer(&state.gizmo, &state.pick_buffer, &state.ecs_scene);
                vec3_t cam_eye = orbit_camera_get_eye_position(&state.camera);
                constraint_glyphs_populate_pick_buffer(&state.constraint_glyphs, &state.ecs_scene, &state.pick_buffer,
                                                       cam_eye, 0.785398f,
                                                       view_legacy, proj_legacy,
                                                       state.viewport.window_pos_x, state.viewport.window_pos_y,
                                                       (float)state.viewport.content_width, (float)state.viewport.content_height);
                constraint_glyphs_update_screen_anchors(&state.constraint_glyphs, &state.ecs_scene,
                    view_legacy, proj_legacy,
                    state.viewport.window_pos_x, state.viewport.window_pos_y,
                    (float)state.viewport.content_width, (float)state.viewport.content_height);

                // Render pick pass
                pick_buffer_render(&state.pick_buffer, view_legacy, proj_legacy);

                // Readback and update hover state
                pick_buffer_readback(&state.pick_buffer);
                pick_buffer_update_hover(&state.pick_buffer);

                pick_buffer_clear_rebuild_flag(&state.pick_buffer);
            }

            uint32_t pick_id = pick_buffer_get_hovered_id(&state.pick_buffer);
            constraint_glyphs_handle_hover(&state.constraint_glyphs, pick_id);

            // Route hover: constraint glyphs, then gizmo handles/vertices, then entities
            if (constraint_glyphs_is_pick_id(pick_id)) {
                gizmo_handle_hover(&state.gizmo, 0);
                ecs_scene_update_hover(&state.ecs_scene, &state.pick_buffer);
            } else if (pick_id >= GIZMO_PICK_RESERVED_START) {
                // Gizmo handle or vertex handle
                gizmo_handle_hover(&state.gizmo, pick_id);
                // Clear ECS hover
                ecs_scene_update_hover(&state.ecs_scene, &state.pick_buffer);
            } else {
                // Normal entity hover
                gizmo_handle_hover(&state.gizmo, 0);  // Clear gizmo hover
                ecs_scene_update_hover(&state.ecs_scene, &state.pick_buffer);
            }

            // Handle mouse interactions
            if (!state.gizmo_drag_active) {
                // Check if we should start a gizmo drag
                if (state.viewport.hovered && io->MouseDown[0] && igIsMouseClicked_Bool(0, false)) {
                    if (state.gizmo.hovered_handle != GIZMO_HANDLE_NONE) {
                        // Start gizmo drag — compute mouse ray
                        ray_t mouse_ray = mdcad_interaction_screen_ray_from_viewport(
                            vp_x, vp_y, (float)vp_width, (float)vp_height, view_legacy, proj_legacy);

                        if (gizmo_begin_drag(&state.gizmo, mouse_ray)) {
                            state.gizmo_drag_active = true;

                            if (state.gizmo.edit_mode == GIZMO_TRANSFORM_MODE) {
                                // Snapshot entity positions
                                int count = state.selection.count;
                                state.gizmo_drag_entity_count = count;
                                state.gizmo_drag_entities = (ecs_entity_t*)malloc(count * sizeof(ecs_entity_t));
                                state.gizmo_drag_start_positions = (vec3_t*)malloc(count * sizeof(vec3_t));
                                for (int i = 0; i < count; i++) {
                                    ecs_entity_t e = state.selection.entities[i];
                                    state.gizmo_drag_entities[i] = e;
                                    const TransformComp *t = ecs_world_get_transform(state.ecs_scene.world, e);
                                    state.gizmo_drag_start_positions[i] = t ? t->position : vec3_make(0, 0, 0);
                                }
                            } else if (state.gizmo.edit_mode == GIZMO_GEOMETRY_MODE &&
                                       state.gizmo.vertex_mode.active) {
                                // Snapshot vertex positions
                                gizmo_vertex_mode_t *vm = &state.gizmo.vertex_mode;
                                ecs_entity_t entity = (ecs_entity_t)vm->target_entity;
                                const GeometryComp *geom = ecs_world_get_geometry(state.ecs_scene.world, entity);
                                if (geom) {
                                    int count = vm->selected_count;
                                    state.gizmo_drag_vertex_count = count;
                                    state.gizmo_drag_vertex_indices = (int*)malloc(count * sizeof(int));
                                    state.gizmo_drag_start_vertices = (vec3_t*)malloc(count * sizeof(vec3_t));
                                    for (int i = 0; i < count; i++) {
                                        int idx = vm->selected_vertices[i];
                                        state.gizmo_drag_vertex_indices[i] = idx;
                                        state.gizmo_drag_start_vertices[i] = gizmo_vertex_mode_get_local_pos(geom, idx);
                                    }
                                }
                            }
                        }
                    } else if (state.gizmo.hovered_vertex >= 0 &&
                               state.gizmo.edit_mode == GIZMO_GEOMETRY_MODE) {
                        // Clicked on a vertex handle — select it
                        gizmo_vertex_mode_select(&state.gizmo.vertex_mode,
                                                  state.gizmo.hovered_vertex,
                                                  state.viewport.shift_held,
                                                  state.viewport.ctrl_held);
                    } else if (state.viewport.clicked) {
                        if (constraint_glyphs_is_pick_id(pick_id)) {
                            ecs_entity_t clicked_constraint =
                                constraint_glyphs_constraint_from_pick_id(&state.constraint_glyphs, pick_id);
                            if (clicked_constraint != 0) {
                                if (ecs_is_alive(state.ecs_world.world, clicked_constraint)) {
                                    state.selected_constraint_entity = clicked_constraint;
                                    constraint_selection_apply_participants(&state.selection,
                                                                            &state.ecs_world,
                                                                            clicked_constraint);

                                    ConstraintComp *constraint = ecs_world_get_constraint(&state.ecs_world, clicked_constraint);
                                    if (constraint && constraint_type_is_dimensional(constraint->type) &&
                                        igIsMouseDoubleClicked_Nil(ImGuiMouseButton_Left)) {
                                        state.constraint_dimension_popup_constraint = clicked_constraint;
                                        state.constraint_dimension_popup_value = constraint->value;
                                        state.constraint_dimension_popup_open_request = true;
                                        state.constraint_dimension_popup_open = false;
                                    }
                                }
                            }
                        } else {
                            // Normal entity click
                            ecs_entity_t clicked_entity = ecs_scene_find_entity_by_pick_id(&state.ecs_scene, pick_id);
                            selection_handle_click(&state.selection, clicked_entity,
                                                    state.viewport.shift_held,
                                                    state.viewport.ctrl_held);
                        }
                        // If selection changed while in geometry mode, update vertex mode
                        if (state.gizmo.edit_mode == GIZMO_GEOMETRY_MODE) {
                            gizmo_set_edit_mode(&state.gizmo, GIZMO_GEOMETRY_MODE,
                                                 &state.ecs_scene, &state.selection);
                        }
                    }
                }
            }

            // Update active drag
            if (state.gizmo_drag_active) {
                ray_t mouse_ray = mdcad_interaction_screen_ray_from_viewport(
                    vp_x, vp_y, (float)vp_width, (float)vp_height, view_legacy, proj_legacy);

                vec3_t requested_delta = gizmo_update_drag(&state.gizmo, mouse_ray);
                scene_solver_drag_decision_t drag_decision;
                bool has_drag_decision = false;
                bool constrained_sketch_drag = false;
                ecs_entity_t drag_sketch = 0;

                if (state.gizmo.edit_mode == GIZMO_TRANSFORM_MODE && state.gizmo_drag_entity_count > 0) {
                    ecs_entity_t first = state.gizmo_drag_entities[0];
                    ecs_entity_t parent = scene_get_parent(&state.ecs_scene, first);
                    if (scene_is_sketch(&state.ecs_scene, parent)) {
                        bool same_sketch = true;
                        for (int i = 1; i < state.gizmo_drag_entity_count; i++) {
                            ecs_entity_t current_parent = scene_get_parent(&state.ecs_scene, state.gizmo_drag_entities[i]);
                            if (current_parent != parent) {
                                same_sketch = false;
                                break;
                            }
                        }
                        if (same_sketch) {
                            constrained_sketch_drag = true;
                            drag_sketch = parent;
                            has_drag_decision = scene_solver_can_apply_drag(
                                &state.ecs_scene,
                                drag_sketch,
                                state.gizmo_drag_entities,
                                state.gizmo_drag_entity_count,
                                requested_delta,
                                &drag_decision);
                        }
                    }
                }

                vec3_t delta = requested_delta;
                if (has_drag_decision && drag_decision.result == SCENE_SOLVER_DRAG_FEASIBLE) {
                    delta = drag_decision.projected_delta;
                } else if (has_drag_decision && drag_decision.result == SCENE_SOLVER_DRAG_UNSATISFIABLE) {
                    delta = vec3_make(0.0f, 0.0f, 0.0f);
                    if (constrained_sketch_drag) {
                        mdcad_apply_solver_failure_feedback(drag_sketch, &drag_decision);
                    }
                }

                float delta_len = vec3_length(delta);

                if (delta_len > 1e-7f) {
                    // Scene changed — invalidate pick buffer
                    pick_buffer_invalidate(&state.pick_buffer);

                    if (state.gizmo.edit_mode == GIZMO_TRANSFORM_MODE) {
                        // Apply delta to all selected entities (endpoint points route through owner geometry sync)
                        scene_apply_transform_delta_for_selection(&state.ecs_scene,
                                                                  state.gizmo_drag_entities,
                                                                  state.gizmo_drag_entity_count,
                                                                  delta);
                    } else if (state.gizmo.edit_mode == GIZMO_GEOMETRY_MODE &&
                               state.gizmo.vertex_mode.active) {
                        // Apply delta to selected vertices
                        ecs_entity_t entity = (ecs_entity_t)state.gizmo.vertex_mode.target_entity;
                        GeometryComp *geom = (GeometryComp*)ecs_world_get_geometry(state.ecs_scene.world, entity);
                        const TransformComp *xform = ecs_world_get_transform(state.ecs_scene.world, entity);
                        if (geom && xform) {
                            gizmo_vertex_mode_apply_delta(&state.gizmo.vertex_mode, geom, xform->world_matrix, delta);
                        }
                        RenderableComp *r = (RenderableComp*)ecs_world_get_renderable(state.ecs_scene.world, entity);
                        if (r) r->instance_dirty = true;
                    }
                }

                // End drag on mouse release
                if (!io->MouseDown[0]) {
                    vec3_t total_delta = gizmo_end_drag(&state.gizmo);
                    state.gizmo_drag_active = false;

                    // Record undo
                    if (vec3_length(total_delta) > 1e-7f) {
                        if (state.gizmo.edit_mode == GIZMO_TRANSFORM_MODE) {
                            for (int i = 0; i < state.gizmo_drag_entity_count; i++) {
                                ecs_entity_t e = state.gizmo_drag_entities[i];
                                const TransformComp *t = ecs_world_get_transform(state.ecs_scene.world, e);
                                vec3_t new_pos = t ? t->position : vec3_make(0, 0, 0);
                                undo_cmd_set_position(&state.undo_redo, e,
                                    state.gizmo_drag_start_positions[i], new_pos);
                            }
                        } else if (state.gizmo.edit_mode == GIZMO_GEOMETRY_MODE &&
                                   state.gizmo.vertex_mode.active) {
                            ecs_entity_t entity = (ecs_entity_t)state.gizmo.vertex_mode.target_entity;
                            const GeometryComp *geom = ecs_world_get_geometry(state.ecs_scene.world, entity);
                            if (geom && state.gizmo_drag_vertex_count > 0) {
                                int count = state.gizmo_drag_vertex_count;
                                vec3_t *new_positions = (vec3_t*)malloc(count * sizeof(vec3_t));
                                for (int i = 0; i < count; i++) {
                                    new_positions[i] = gizmo_vertex_mode_get_local_pos(geom,
                                        state.gizmo_drag_vertex_indices[i]);
                                }
                                undo_cmd_set_geometry_vertices(&state.undo_redo, entity,
                                    state.gizmo_drag_vertex_indices,
                                    state.gizmo_drag_start_vertices,
                                    new_positions, count);
                                free(new_positions);
                            }
                            ecs_entity_t parent = scene_get_parent(&state.ecs_scene, entity);
                            if (parent != 0 && scene_is_sketch(&state.ecs_scene, parent)) {
                                scene_script_reemit_for_sketch(&state.ecs_scene, parent);
                            }
                        }
                        ui_scene_hierarchy_mark_dirty(&state.scene_hierarchy);
                    }

                    // Free drag arrays
                    free(state.gizmo_drag_entities); state.gizmo_drag_entities = NULL;
                    free(state.gizmo_drag_start_positions); state.gizmo_drag_start_positions = NULL;
                    state.gizmo_drag_entity_count = 0;
                    free(state.gizmo_drag_vertex_indices); state.gizmo_drag_vertex_indices = NULL;
                    free(state.gizmo_drag_start_vertices); state.gizmo_drag_start_vertices = NULL;
                    state.gizmo_drag_vertex_count = 0;
                }
            }
        }
    }

    constraint_glyphs_draw_overlay(&state.constraint_glyphs, &state.ecs_scene,
        state.viewport.window_pos_x, state.viewport.window_pos_y,
        (float)state.viewport.content_width, (float)state.viewport.content_height);

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

    // Shutdown gizmo system
    gizmo_shutdown(&state.gizmo);
    free(state.gizmo_drag_entities);
    free(state.gizmo_drag_start_positions);
    free(state.gizmo_drag_vertex_indices);
    free(state.gizmo_drag_start_vertices);

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

    render_target_shutdown(&state.viewport_rt);
    simgui_shutdown();
    sg_shutdown();
}

//------------------------------------------------------------------------------
// Event handling
//------------------------------------------------------------------------------
static void event(const sapp_event* ev) {
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
        .window_title = "mdCAD",
        .width = 1280,
        .height = 720,
        .high_dpi = true,
        .enable_clipboard = true,
        .clipboard_size = 65536,
        .icon.sokol_default = false,
        .logger.func = slog_func,
    };
}
