//------------------------------------------------------------------------------
// ui_entity_inspector.h - Entity property inspector panel (header-only)
//
// Displays and allows editing of selected entity properties:
// - Transform (position, rotation, scale)
// - Geometry (type, color, line width, point size)
// - Renderable (visible, layer)
//------------------------------------------------------------------------------
#ifndef UI_ENTITY_INSPECTOR_H
#define UI_ENTITY_INSPECTOR_H

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"
#include "../selection.h"
#include "../ecs/ecs_world.h"
#include "../constraints/constraint_types.h"
#include "../components/geometry_comp.h"
#include "../components/transform_comp.h"
#include "../components/renderable_comp.h"
#include "../components/selectable_comp.h"
#include "../math/math_undo_editor.h"
#include "../undo_redo_exec.h"
#include <ctype.h>
#include <float.h>
#include <stdlib.h>
#include <string.h>

//------------------------------------------------------------------------------
// Types
//------------------------------------------------------------------------------

typedef struct {
    selection_buffer_t *selection;
    ecs_world_state_t *world;
    undo_redo_t *undo_redo;  // Optional, can be NULL

    // Cached values for drag operations (to capture old value at drag start)
    ecs_entity_t editing_entity;
    vec3_t drag_start_position;
    vec3_t drag_start_rotation;
    vec3_t drag_start_scale;
    vec4_t drag_start_color;
    float drag_start_line_width;
    float drag_start_point_size;
    vec3_t drag_start_point_pos;
    vec3_t drag_start_line_a;
    vec3_t drag_start_line_b;
} ui_entity_inspector_state_t;

//------------------------------------------------------------------------------
// Initialization
//------------------------------------------------------------------------------

static inline void ui_entity_inspector_init(ui_entity_inspector_state_t *state,
                                             selection_buffer_t *selection,
                                             ecs_world_state_t *world) {
    memset(state, 0, sizeof(ui_entity_inspector_state_t));
    state->selection = selection;
    state->world = world;
    state->undo_redo = NULL;
}

static inline void ui_entity_inspector_set_undo_redo(ui_entity_inspector_state_t *state,
                                                      undo_redo_t *undo_redo) {
    state->undo_redo = undo_redo;
}

#define UI_GEOMETRY_MANAGER_MAX_ROWS 2048
#define UI_CONSTRAINT_MANAGER_MAX_ROWS 2048

static inline bool ui_geometry_manager_contains(const ecs_entity_t *entities, int count, ecs_entity_t target) {
    for (int i = 0; i < count; i++) {
        if (entities[i] == target) {
            return true;
        }
    }
    return false;
}

static inline int ui_geometry_manager_index_of(const ecs_entity_t *entities, int count, ecs_entity_t target) {
    for (int i = 0; i < count; i++) {
        if (entities[i] == target) {
            return i;
        }
    }
    return -1;
}

static inline void ui_geometry_manager_selection_clear(ecs_entity_t *selection, int *selection_count) {
    (void)selection;
    *selection_count = 0;
}

static inline void ui_geometry_manager_selection_add(ecs_entity_t *selection, int *selection_count, ecs_entity_t e) {
    if (e == 0) return;
    if (ui_geometry_manager_contains(selection, *selection_count, e)) return;
    if (*selection_count >= UI_GEOMETRY_MANAGER_MAX_ROWS) return;
    selection[*selection_count] = e;
    (*selection_count)++;
}

static inline void ui_geometry_manager_selection_remove(ecs_entity_t *selection, int *selection_count, ecs_entity_t e) {
    for (int i = 0; i < *selection_count; i++) {
        if (selection[i] == e) {
            selection[i] = selection[*selection_count - 1];
            (*selection_count)--;
            return;
        }
    }
}

static inline void ui_geometry_manager_handle_click(ecs_entity_t *selection,
                                                    int *selection_count,
                                                    ecs_entity_t clicked_entity,
                                                    bool shift_held,
                                                    bool ctrl_held) {
    if (clicked_entity == 0) {
        if (!shift_held && !ctrl_held) {
            ui_geometry_manager_selection_clear(selection, selection_count);
        }
        return;
    }

    if (ctrl_held) {
        if (ui_geometry_manager_contains(selection, *selection_count, clicked_entity)) {
            ui_geometry_manager_selection_remove(selection, selection_count, clicked_entity);
        } else {
            ui_geometry_manager_selection_add(selection, selection_count, clicked_entity);
        }
    } else if (shift_held) {
        ui_geometry_manager_selection_add(selection, selection_count, clicked_entity);
    } else {
        selection[0] = clicked_entity;
        *selection_count = 1;
    }
}

static inline bool ui_constraint_text_matches(const char *haystack, const char *needle) {
    if (!needle || needle[0] == '\0') return true;
    if (!haystack) return false;

    char lower_hay[512];
    char lower_need[128];

    size_t hlen = strlen(haystack);
    if (hlen >= sizeof(lower_hay)) hlen = sizeof(lower_hay) - 1;
    for (size_t i = 0; i < hlen; i++) {
        lower_hay[i] = (char)tolower((unsigned char)haystack[i]);
    }
    lower_hay[hlen] = '\0';

    size_t nlen = strlen(needle);
    if (nlen >= sizeof(lower_need)) nlen = sizeof(lower_need) - 1;
    for (size_t i = 0; i < nlen; i++) {
        lower_need[i] = (char)tolower((unsigned char)needle[i]);
    }
    lower_need[nlen] = '\0';

    return strstr(lower_hay, lower_need) != NULL;
}

//------------------------------------------------------------------------------
// Internal: Draw single entity inspector
//------------------------------------------------------------------------------

static inline void ui_entity_inspector_draw_single(ui_entity_inspector_state_t *state,
                                                    ecs_entity_t e) {
    ecs_world_state_t *w = state->world;

    // Entity header
    igText("Entity ID: %llu", (unsigned long long)e);

    // Get selectable for pick ID display
    SelectableComp *sel = ecs_world_get_selectable(w, e);
    if (sel) {
        igTextDisabled("Pick ID: %u", sel->pick_id);
    }

    igSeparator();

    static ecs_entity_t gm_selected_entities[UI_GEOMETRY_MANAGER_MAX_ROWS] = {0};
    static int gm_selected_count = 0;
    static ecs_entity_t gm_bound_sketch = 0;
    static int gm_delete_pending_count = 0;
    static int cm_type_filter = 0;
    static char cm_search_filter[128] = "";
    static ecs_entity_t cm_delete_pending_constraint = 0;

    SketchComp *sketch = ecs_world_get_sketch(w, e);
    ecs_scene_t *scene = NULL;
    if (state->undo_redo && state->undo_redo->scene) {
        scene = (ecs_scene_t*)state->undo_redo->scene;
    }

    // Label section (optional component - only shown if entity has one)
    LabelComp *label = ecs_world_get_label(w, e);
    if (label && igCollapsingHeader_TreeNodeFlags("Label", ImGuiTreeNodeFlags_DefaultOpen)) {
        igInputText("Name", label->name, LABEL_NAME_MAX, 0, NULL, NULL);
        igInputText("Description", label->description, LABEL_DESC_MAX, 0, NULL, NULL);
    }

    // Transform section
    TransformComp *t = ecs_world_get_transform(w, e);
    if (t && igCollapsingHeader_TreeNodeFlags("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
        bool changed = false;

        // Position
        float pos[3] = { t->position.x, t->position.y, t->position.z };
        if (igDragFloat3("Position", pos, 0.1f, -100.0f, 100.0f, "%.2f", 0)) {
            t->position.x = pos[0];
            t->position.y = pos[1];
            t->position.z = pos[2];
            changed = true;
        }
        // Capture value at drag start
        if (igIsItemActivated()) {
            state->editing_entity = e;
            state->drag_start_position = t->position;
        }
        // Record undo command at drag end
        if (igIsItemDeactivatedAfterEdit() && state->undo_redo && state->editing_entity == e) {
            vec3_t new_pos = t->position;
            if (state->drag_start_position.x != new_pos.x ||
                state->drag_start_position.y != new_pos.y ||
                state->drag_start_position.z != new_pos.z) {
                undo_cmd_set_position(state->undo_redo, e, state->drag_start_position, new_pos);
            }
        }

        // Rotation (in degrees for user friendliness)
        float rot_deg[3] = {
            mdcad_undo_editor_rad_to_deg(t->rotation.x),  // rad to deg
            mdcad_undo_editor_rad_to_deg(t->rotation.y),
            mdcad_undo_editor_rad_to_deg(t->rotation.z)
        };
        if (igDragFloat3("Rotation", rot_deg, 1.0f, -360.0f, 360.0f, "%.1f", 0)) {
            t->rotation.x = mdcad_undo_editor_deg_to_rad(rot_deg[0]);  // deg to rad
            t->rotation.y = mdcad_undo_editor_deg_to_rad(rot_deg[1]);
            t->rotation.z = mdcad_undo_editor_deg_to_rad(rot_deg[2]);
            changed = true;
        }
        if (igIsItemActivated()) {
            state->editing_entity = e;
            state->drag_start_rotation = t->rotation;
        }
        if (igIsItemDeactivatedAfterEdit() && state->undo_redo && state->editing_entity == e) {
            vec3_t new_rot = t->rotation;
            if (state->drag_start_rotation.x != new_rot.x ||
                state->drag_start_rotation.y != new_rot.y ||
                state->drag_start_rotation.z != new_rot.z) {
                undo_cmd_set_rotation(state->undo_redo, e, state->drag_start_rotation, new_rot);
            }
        }

        // Scale
        float scale[3] = { t->scale.x, t->scale.y, t->scale.z };
        if (igDragFloat3("Scale", scale, 0.01f, 0.01f, 10.0f, "%.2f", 0)) {
            t->scale.x = scale[0];
            t->scale.y = scale[1];
            t->scale.z = scale[2];
            changed = true;
        }
        if (igIsItemActivated()) {
            state->editing_entity = e;
            state->drag_start_scale = t->scale;
        }
        if (igIsItemDeactivatedAfterEdit() && state->undo_redo && state->editing_entity == e) {
            vec3_t new_scale = t->scale;
            if (state->drag_start_scale.x != new_scale.x ||
                state->drag_start_scale.y != new_scale.y ||
                state->drag_start_scale.z != new_scale.z) {
                undo_cmd_set_scale(state->undo_redo, e, state->drag_start_scale, new_scale);
            }
        }

        if (changed) {
            t->dirty = true;
            RenderableComp *r = ecs_world_get_renderable(w, e);
            if (r) r->instance_dirty = true;

            // Mark all descendants dirty so they update their world matrices
            ecs_world_mark_descendants_dirty(w, e);
        }
    }

    if (sketch && igCollapsingHeader_TreeNodeFlags("SketchManager", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (scene) {
            scene_refresh_sketch_metadata(scene, e);
            sketch = ecs_world_get_sketch(w, e);
        }

        igText("Status: %s", sketch_status_name(sketch->status));
        igTextDisabled("Fixed-state: %d/%d geometry fixed",
            sketch->fixed_geometry_count, sketch->geometry_count);

        igDummy((ImVec2){0.0f, 8.0f});
        igText("Geometry count: %d", sketch->geometry_count);
        igText("Constraint count: %d", sketch->constraint_count);

        igDummy((ImVec2){0.0f, 8.0f});
        igTextDisabled("Color policy");
        igTextWrapped("Sketch color applies by default; geometry with explicit non-black RGB color overrides inherited sketch color.");
    }

    if (sketch && igCollapsingHeader_TreeNodeFlags("GeometryManager", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (gm_bound_sketch != e) {
            gm_bound_sketch = e;
            gm_selected_count = 0;
        }

        ecs_entity_t geometry_rows[UI_GEOMETRY_MANAGER_MAX_ROWS];
        int geometry_row_count = 0;
        ecs_iter_t child_it = ecs_children(w->world, e);
        while (ecs_children_next(&child_it)) {
            for (int i = 0; i < child_it.count; i++) {
                ecs_entity_t child = child_it.entities[i];
                if (!ecs_world_get_geometry(w, child)) {
                    continue;
                }
                if (geometry_row_count < UI_GEOMETRY_MANAGER_MAX_ROWS) {
                    geometry_rows[geometry_row_count++] = child;
                }
            }
        }

        for (int i = gm_selected_count - 1; i >= 0; i--) {
            if (!ui_geometry_manager_contains(geometry_rows, geometry_row_count, gm_selected_entities[i])) {
                gm_selected_entities[i] = gm_selected_entities[gm_selected_count - 1];
                gm_selected_count--;
            }
        }

        igTextDisabled("Add geometry");
        igBeginDisabled(scene == NULL);
        if (igButton("Add Point##geometry_manager_add_point", (ImVec2){0, 0})) {
            ecs_entity_t created = scene_add_point_to_sketch(scene, e,
                vec3_make(0.0f, 0.0f, 0.0f), sketch->color, 0.06f);
            if (created != 0) {
                if (state->undo_redo) {
                    undo_cmd_create_entity(state->undo_redo, created);
                }
                scene_refresh_sketch_metadata(scene, e);
                sketch = ecs_world_get_sketch(w, e);
                gm_selected_entities[0] = created;
                gm_selected_count = 1;
            }
        }
        igSameLine(0, 8);
        if (igButton("Add Line##geometry_manager_add_line", (ImVec2){0, 0})) {
            ecs_entity_t created = scene_add_line_to_sketch(scene, e,
                vec3_make(-1.0f, 0.0f, 0.0f),
                vec3_make(1.0f, 0.0f, 0.0f),
                sketch->color, 0.03f);
            if (created != 0) {
                if (state->undo_redo) {
                    undo_cmd_create_entity(state->undo_redo, created);
                }
                scene_refresh_sketch_metadata(scene, e);
                sketch = ecs_world_get_sketch(w, e);
                gm_selected_entities[0] = created;
                gm_selected_count = 1;
            }
        }
        igSameLine(0, 8);
        if (igButton("Add Arc##geometry_manager_add_arc", (ImVec2){0, 0})) {
            ecs_entity_t created = scene_add_arc_to_sketch(scene, e,
                vec3_make(0.0f, 0.0f, 0.0f), 1.0f, 0.0f, 1.5707963f,
                vec3_make(0.0f, 0.0f, 1.0f), sketch->color, 0.03f);
            if (created != 0) {
                if (state->undo_redo) {
                    undo_cmd_create_entity(state->undo_redo, created);
                }
                scene_refresh_sketch_metadata(scene, e);
                sketch = ecs_world_get_sketch(w, e);
                gm_selected_entities[0] = created;
                gm_selected_count = 1;
            }
        }
        igSameLine(0, 8);
        if (igButton("Add Circle##geometry_manager_add_circle", (ImVec2){0, 0})) {
            ecs_entity_t created = scene_add_arc_to_sketch(scene, e,
                vec3_make(0.0f, 0.0f, 0.0f), 1.0f, 0.0f, 6.2831853f,
                vec3_make(0.0f, 0.0f, 1.0f), sketch->color, 0.03f);
            if (created != 0) {
                if (state->undo_redo) {
                    undo_cmd_create_entity(state->undo_redo, created);
                }
                scene_refresh_sketch_metadata(scene, e);
                sketch = ecs_world_get_sketch(w, e);
                gm_selected_entities[0] = created;
                gm_selected_count = 1;
            }
        }
        igEndDisabled();
        if (scene == NULL) {
            igTextDisabled("GeometryManager add controls require scene context.");
        }
        igDummy((ImVec2){0.0f, 8.0f});

        if (geometry_row_count == 0) {
            igTextDisabled("No geometry in this sketch");
            igTextWrapped("Add Point, Line, or Arc/Circle from Add Entity or GeometryManager controls to start defining this sketch.");
        } else {
            for (int row = 0; row < geometry_row_count; row++) {
                ecs_entity_t child = geometry_rows[row];
                GeometryComp *child_geom = ecs_world_get_geometry(w, child);
                if (!child_geom) continue;

                LabelComp *child_label = ecs_world_get_label(w, child);
                SketchGeometryStateComp *child_state = ecs_world_get_sketch_geometry_state(w, child);
                SketchGeometryStateComp effective_state = child_state ? *child_state : sketch_geometry_state_comp_default();
                const char *type_name = geometry_type_name(child_geom->type);
                const char *row_name = (child_label && child_label->name[0] != '\0') ? child_label->name : NULL;
                const char *fixed_label = sketch_geometry_state_label(effective_state);

                char row_text[256];
                if (row_name) {
                    snprintf(row_text, sizeof(row_text), "%s | %s | %s##geom_row_%llu",
                        type_name, row_name, fixed_label, (unsigned long long)child);
                } else {
                    snprintf(row_text, sizeof(row_text), "%s | #%llu | %s##geom_row_%llu",
                        type_name, (unsigned long long)child, fixed_label, (unsigned long long)child);
                }

                bool selected = ui_geometry_manager_contains(gm_selected_entities, gm_selected_count, child);
                if (igSelectable_Bool(row_text, selected, 0, (ImVec2){0.0f, 0.0f})) {
                    ImGuiIO *io = igGetIO_Nil();
                    ui_geometry_manager_handle_click(
                        gm_selected_entities,
                        &gm_selected_count,
                        child,
                        io->KeyShift,
                        io->KeyCtrl
                    );
                }
            }
        }

        igDummy((ImVec2){0.0f, 8.0f});
        igTextDisabled("Selected rows: %d", gm_selected_count);

        igBeginDisabled(gm_selected_count <= 0);
        if (igButton("Fix##geometry_manager_bulk_fix", (ImVec2){0, 0})) {
            if (gm_selected_count > 0) {
                bool *old_fixed = NULL;
                if (state->undo_redo) {
                    old_fixed = (bool*)malloc((size_t)gm_selected_count * sizeof(bool));
                }

                ecs_entity_t action_entities[UI_GEOMETRY_MANAGER_MAX_ROWS];
                int action_count = 0;
                for (int i = 0; i < gm_selected_count; i++) {
                    ecs_entity_t target = gm_selected_entities[i];
                    if (!ecs_is_alive(w->world, target) || !ecs_world_get_geometry(w, target)) continue;

                    SketchGeometryStateComp *row_state = ecs_world_get_sketch_geometry_state(w, target);
                    if (!row_state) {
                        SketchGeometryStateComp init_state = sketch_geometry_state_comp_default();
                        ecs_world_set_sketch_geometry_state(w, target, &init_state);
                        row_state = ecs_world_get_sketch_geometry_state(w, target);
                    }
                    if (!row_state) continue;

                    if (old_fixed) old_fixed[action_count] = row_state->fixed;
                    row_state->fixed = true;
                    action_entities[action_count++] = target;
                }

                if (state->undo_redo && action_count > 0 && old_fixed) {
                    undo_cmd_bulk_set_sketch_fixed(state->undo_redo, action_entities, old_fixed, action_count, true);
                }
                if (old_fixed) free(old_fixed);

                if (scene) {
                    scene_refresh_sketch_metadata(scene, e);
                    sketch = ecs_world_get_sketch(w, e);
                }
            }
        }
        igEndDisabled();
        igSameLine(0, 8);
        igBeginDisabled(gm_selected_count <= 0);
        if (igButton("Unfix##geometry_manager_bulk_unfix", (ImVec2){0, 0})) {
            if (gm_selected_count > 0) {
                bool *old_fixed = NULL;
                if (state->undo_redo) {
                    old_fixed = (bool*)malloc((size_t)gm_selected_count * sizeof(bool));
                }

                ecs_entity_t action_entities[UI_GEOMETRY_MANAGER_MAX_ROWS];
                int action_count = 0;
                for (int i = 0; i < gm_selected_count; i++) {
                    ecs_entity_t target = gm_selected_entities[i];
                    if (!ecs_is_alive(w->world, target) || !ecs_world_get_geometry(w, target)) continue;

                    SketchGeometryStateComp *row_state = ecs_world_get_sketch_geometry_state(w, target);
                    if (!row_state) {
                        SketchGeometryStateComp init_state = sketch_geometry_state_comp_default();
                        ecs_world_set_sketch_geometry_state(w, target, &init_state);
                        row_state = ecs_world_get_sketch_geometry_state(w, target);
                    }
                    if (!row_state) continue;

                    if (old_fixed) old_fixed[action_count] = row_state->fixed;
                    row_state->fixed = false;
                    action_entities[action_count++] = target;
                }

                if (state->undo_redo && action_count > 0 && old_fixed) {
                    undo_cmd_bulk_set_sketch_fixed(state->undo_redo, action_entities, old_fixed, action_count, false);
                }
                if (old_fixed) free(old_fixed);

                if (scene) {
                    scene_refresh_sketch_metadata(scene, e);
                    sketch = ecs_world_get_sketch(w, e);
                }
            }
        }
        igEndDisabled();
        igSameLine(0, 8);
        igBeginDisabled(gm_selected_count <= 0);
        if (igButton("Delete##geometry_manager_bulk_delete", (ImVec2){0, 0})) {
            if (gm_selected_count > 0) {
                gm_delete_pending_count = gm_selected_count;
                igOpenPopup_Str("Delete Geometry##geometry_manager_delete_popup", 0);
            }
        }
        igEndDisabled();

        if (igBeginPopupModal("Delete Geometry##geometry_manager_delete_popup", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            if (gm_delete_pending_count > 1) {
                igTextWrapped("Delete %d selected geometry items from this sketch? This will be one undo step.", gm_delete_pending_count);
            } else {
                igTextWrapped("Delete selected geometry from this sketch? This action can be undone in one step.");
            }

            igDummy((ImVec2){0.0f, 8.0f});
            if (igButton("Delete##geometry_manager_confirm_delete", (ImVec2){120.0f, 0.0f})) {
                ecs_entity_t delete_entities[UI_GEOMETRY_MANAGER_MAX_ROWS];
                int delete_count = 0;
                for (int i = 0; i < gm_selected_count; i++) {
                    ecs_entity_t target = gm_selected_entities[i];
                    if (!ecs_is_alive(w->world, target) || !ecs_world_get_geometry(w, target)) continue;
                    delete_entities[delete_count++] = target;
                }

                if (state->undo_redo && delete_count > 0) {
                    undo_cmd_bulk_delete_entities(state->undo_redo, delete_entities, delete_count);
                }

                for (int i = 0; i < delete_count; i++) {
                    if (scene) {
                        scene_remove_entity(scene, delete_entities[i]);
                    } else {
                        ecs_delete(w->world, delete_entities[i]);
                    }
                }

                gm_selected_count = 0;
                gm_delete_pending_count = 0;

                if (scene) {
                    scene_refresh_sketch_metadata(scene, e);
                    sketch = ecs_world_get_sketch(w, e);
                }
                igCloseCurrentPopup();
            }
            igSameLine(0, 8);
            if (igButton("Cancel##geometry_manager_cancel_delete", (ImVec2){120.0f, 0.0f})) {
                gm_delete_pending_count = 0;
                igCloseCurrentPopup();
            }
            igEndPopup();
        }
    }

    if (sketch && igCollapsingHeader_TreeNodeFlags("ConstraintManager", ImGuiTreeNodeFlags_DefaultOpen)) {
        ecs_entity_t constraint_rows[UI_CONSTRAINT_MANAGER_MAX_ROWS];
        int constraint_row_count = 0;
        ecs_iter_t child_it = ecs_children(w->world, e);
        while (ecs_children_next(&child_it)) {
            for (int i = 0; i < child_it.count; i++) {
                ecs_entity_t child = child_it.entities[i];
                if (!ecs_world_get_constraint(w, child)) continue;
                if (constraint_row_count < UI_CONSTRAINT_MANAGER_MAX_ROWS) {
                    constraint_rows[constraint_row_count++] = child;
                }
            }
        }

        static const char *constraint_type_filter_items[] = {
            "All",
            "Fixed",
            "Coincident",
            "Collinear",
            "Parallel",
            "Perpendicular",
            "Along X",
            "Along Y",
            "Along Z",
            "Coradial",
            "Concentric",
            "Length",
            "Angle",
            "Tangential"
        };

        igTextDisabled("Filter");
        igSetNextItemWidth(160.0f);
        igCombo_Str_arr("Type##constraint_manager_type_filter",
                        &cm_type_filter,
                        constraint_type_filter_items,
                        (int)(sizeof(constraint_type_filter_items) / sizeof(constraint_type_filter_items[0])),
                        -1);
        igSameLine(0, 8);
        igSetNextItemWidth(-1.0f);
        igInputTextWithHint("##constraint_manager_search_filter",
                            "Search name or description",
                            cm_search_filter,
                            sizeof(cm_search_filter),
                            0,
                            NULL,
                            NULL);
        igDummy((ImVec2){0.0f, 8.0f});

        if (constraint_row_count == 0) {
            igTextDisabled("No constraints yet");
            igTextWrapped("Select sketch geometry, press C to open applicable constraints, then apply one to define sketch behavior.");
        } else {
            for (int row = 0; row < constraint_row_count; row++) {
                ecs_entity_t c_e = constraint_rows[row];
                ConstraintComp *constraint = ecs_world_get_constraint(w, c_e);
                if (!constraint) continue;

                if (cm_type_filter > 0) {
                    constraint_type_t selected_type = (constraint_type_t)(cm_type_filter - 1);
                    if (constraint->type != selected_type) {
                        continue;
                    }
                }

                LabelComp *c_label = ecs_world_get_label(w, c_e);
                const char *row_name = (c_label && c_label->name[0] != '\0') ? c_label->name : NULL;
                const char *row_desc = (c_label && c_label->description[0] != '\0') ? c_label->description : "";
                if (!ui_constraint_text_matches(row_name, cm_search_filter) &&
                    !ui_constraint_text_matches(row_desc, cm_search_filter)) {
                    continue;
                }

                char participants_text[256];
                participants_text[0] = '\0';
                for (uint32_t pi = 0; pi < constraint->participant_count; pi++) {
                    ecs_entity_t p = (ecs_entity_t)constraint->participants[pi];
                    LabelComp *p_label = ecs_world_get_label(w, p);
                    char entry[80];
                    if (p_label && p_label->name[0] != '\0') {
                        snprintf(entry, sizeof(entry), "%s", p_label->name);
                    } else {
                        snprintf(entry, sizeof(entry), "#%llu", (unsigned long long)p);
                    }
                    if (participants_text[0] != '\0') {
                        strncat(participants_text, ", ", sizeof(participants_text) - strlen(participants_text) - 1);
                    }
                    strncat(participants_text, entry, sizeof(participants_text) - strlen(participants_text) - 1);
                }

                char value_text[64];
                if (constraint_type_is_dimensional(constraint->type) && constraint->has_value) {
                    snprintf(value_text, sizeof(value_text), "%.3f%s",
                             constraint->value,
                             constraint->driven ? " (Driven)" : "");
                } else {
                    snprintf(value_text, sizeof(value_text), "%s",
                             constraint_type_is_dimensional(constraint->type) ? "Unset" : "-");
                }

                const char *type_name = constraint_type_display_name(constraint->type);
                char row_text[640];
                if (row_name) {
                    snprintf(row_text, sizeof(row_text), "%s | %s | %s | %s##constraint_row_%llu",
                             type_name, row_name, value_text, participants_text, (unsigned long long)c_e);
                } else {
                    snprintf(row_text, sizeof(row_text), "%s | #%llu | %s | %s##constraint_row_%llu",
                             type_name, (unsigned long long)c_e, value_text, participants_text, (unsigned long long)c_e);
                }

                bool row_selected = false;
                if (constraint->participant_count > 0) {
                    row_selected = selection_contains(state->selection, (ecs_entity_t)constraint->participants[0]);
                }
                if (igSelectable_Bool(row_text, row_selected, 0, (ImVec2){0.0f, 0.0f})) {
                    selection_clear(state->selection);
                    for (uint32_t pi = 0; pi < constraint->participant_count; pi++) {
                        ecs_entity_t p = (ecs_entity_t)constraint->participants[pi];
                        if (ecs_is_alive(w->world, p)) {
                            selection_add(state->selection, p);
                        }
                    }
                }

                if (constraint_type_is_dimensional(constraint->type)) {
                    char value_id[96];
                    snprintf(value_id, sizeof(value_id), "Value##constraint_value_%llu", (unsigned long long)c_e);
                    float edit_value = constraint->value;
                    igSetNextItemWidth(140.0f);
                    if (igInputFloat(value_id, &edit_value, 0.1f, 1.0f, "%.4f", 0)) {
                        if (scene) {
                            scene_constraint_set_dimensional_value(scene, c_e, edit_value, constraint->driven);
                        }
                    }
                    igSameLine(0, 8);
                    char driven_id[96];
                    snprintf(driven_id, sizeof(driven_id), "Driven##constraint_driven_%llu", (unsigned long long)c_e);
                    bool driven = constraint->driven;
                    if (igCheckbox(driven_id, &driven)) {
                        if (scene) {
                            scene_constraint_set_dimensional_value(scene, c_e, constraint->value, driven);
                        }
                    }
                    if (igIsItemHovered(ImGuiHoveredFlags_None)) {
                        igSetTooltip("Driven constraints remain visible/readable but do not drive solve equations.");
                    }
                    igSameLine(0, 8);
                }

                char delete_id[96];
                snprintf(delete_id, sizeof(delete_id), "Delete##constraint_delete_%llu", (unsigned long long)c_e);
                igPushStyleColor_Vec4(ImGuiCol_Button, (ImVec4){0.906f, 0.510f, 0.518f, 1.000f});
                igPushStyleColor_Vec4(ImGuiCol_ButtonHovered, (ImVec4){0.906f, 0.510f, 0.518f, 0.860f});
                igPushStyleColor_Vec4(ImGuiCol_ButtonActive, (ImVec4){0.906f, 0.510f, 0.518f, 0.780f});
                if (igButton(delete_id, (ImVec2){80.0f, 0.0f})) {
                    cm_delete_pending_constraint = c_e;
                    igOpenPopup_Str("Delete Constraint##constraint_manager_delete_popup", 0);
                }
                igPopStyleColor(3);
            }
        }

        if (igBeginPopupModal("Delete Constraint##constraint_manager_delete_popup", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            igTextWrapped("Delete selected constraint? Participating geometry will remain; this only removes the constraint.");
            igDummy((ImVec2){0.0f, 8.0f});
            if (igButton("Delete##constraint_manager_confirm_delete", (ImVec2){120.0f, 0.0f})) {
                if (scene && cm_delete_pending_constraint != 0) {
                    scene_remove_constraint(scene, cm_delete_pending_constraint);
                    scene_refresh_sketch_metadata(scene, e);
                    sketch = ecs_world_get_sketch(w, e);
                }
                cm_delete_pending_constraint = 0;
                igCloseCurrentPopup();
            }
            igSameLine(0, 8);
            if (igButton("Cancel##constraint_manager_cancel_delete", (ImVec2){120.0f, 0.0f})) {
                cm_delete_pending_constraint = 0;
                igCloseCurrentPopup();
            }
            igEndPopup();
        }
    }

    // Geometry section
    GeometryComp *g = ecs_world_get_geometry(w, e);
    if (g && igCollapsingHeader_TreeNodeFlags("Geometry", ImGuiTreeNodeFlags_DefaultOpen)) {
        bool changed = false;

        // Type (read-only)
        igText("Type: %s", geometry_type_name(g->type));

        // Color
        float color[4] = { g->color.x, g->color.y, g->color.z, g->color.w };
        if (igColorEdit4("Color", color, ImGuiColorEditFlags_AlphaBar)) {
            g->color.x = color[0];
            g->color.y = color[1];
            g->color.z = color[2];
            g->color.w = color[3];
            changed = true;
        }
        if (igIsItemActivated()) {
            state->editing_entity = e;
            state->drag_start_color = g->color;
        }
        if (igIsItemDeactivatedAfterEdit() && state->undo_redo && state->editing_entity == e) {
            vec4_t new_color = g->color;
            if (state->drag_start_color.x != new_color.x ||
                state->drag_start_color.y != new_color.y ||
                state->drag_start_color.z != new_color.z ||
                state->drag_start_color.w != new_color.w) {
                undo_cmd_set_color(state->undo_redo, e, state->drag_start_color, new_color);
            }
        }

        // Type-specific properties
        switch (g->type) {
            case GEOM_POINT: {
                if (igDragFloat("Point Size", &g->point_size, 0.005f, 0.01f, 0.5f, "%.3f", 0)) {
                    changed = true;
                }
                if (igIsItemActivated()) {
                    state->editing_entity = e;
                    state->drag_start_point_size = g->point_size;
                }
                if (igIsItemDeactivatedAfterEdit() && state->undo_redo && state->editing_entity == e) {
                    if (state->drag_start_point_size != g->point_size) {
                        undo_cmd_set_point_size(state->undo_redo, e, state->drag_start_point_size, g->point_size);
                    }
                }

                // Point position
                float pt[3] = { g->data.point.point.x, g->data.point.point.y, g->data.point.point.z };
                if (igDragFloat3("Point", pt, 0.1f, -100.0f, 100.0f, "%.2f", 0)) {
                    g->data.point.point.x = pt[0];
                    g->data.point.point.y = pt[1];
                    g->data.point.point.z = pt[2];
                    changed = true;
                }
                if (igIsItemActivated()) {
                    state->editing_entity = e;
                    state->drag_start_point_pos = g->data.point.point;
                }
                if (igIsItemDeactivatedAfterEdit() && state->undo_redo && state->editing_entity == e) {
                    vec3_t new_pt = g->data.point.point;
                    if (state->drag_start_point_pos.x != new_pt.x ||
                        state->drag_start_point_pos.y != new_pt.y ||
                        state->drag_start_point_pos.z != new_pt.z) {
                        undo_cmd_set_point_position(state->undo_redo, e, state->drag_start_point_pos, new_pt);
                    }
                }
                break;
            }
            case GEOM_LINE: {
                if (igDragFloat("Line Width", &g->line_width, 0.005f, 0.005f, 0.2f, "%.3f", 0)) {
                    changed = true;
                }
                if (igIsItemActivated()) {
                    state->editing_entity = e;
                    state->drag_start_line_width = g->line_width;
                }
                if (igIsItemDeactivatedAfterEdit() && state->undo_redo && state->editing_entity == e) {
                    if (state->drag_start_line_width != g->line_width) {
                        undo_cmd_set_line_width(state->undo_redo, e, state->drag_start_line_width, g->line_width);
                    }
                }

                // Line endpoints
                float a[3] = { g->data.line.a.x, g->data.line.a.y, g->data.line.a.z };
                float b[3] = { g->data.line.b.x, g->data.line.b.y, g->data.line.b.z };
                if (igDragFloat3("Point A", a, 0.1f, -100.0f, 100.0f, "%.2f", 0)) {
                    g->data.line.a.x = a[0];
                    g->data.line.a.y = a[1];
                    g->data.line.a.z = a[2];
                    changed = true;
                }
                if (igIsItemActivated()) {
                    state->editing_entity = e;
                    state->drag_start_line_a = g->data.line.a;
                    state->drag_start_line_b = g->data.line.b;  // Capture both at same time
                }
                if (igIsItemDeactivatedAfterEdit() && state->undo_redo && state->editing_entity == e) {
                    vec3_t new_a = g->data.line.a;
                    vec3_t new_b = g->data.line.b;
                    if (state->drag_start_line_a.x != new_a.x ||
                        state->drag_start_line_a.y != new_a.y ||
                        state->drag_start_line_a.z != new_a.z) {
                        undo_cmd_set_line_endpoints(state->undo_redo, e,
                            state->drag_start_line_a, state->drag_start_line_b, new_a, new_b);
                    }
                }

                if (igDragFloat3("Point B", b, 0.1f, -100.0f, 100.0f, "%.2f", 0)) {
                    g->data.line.b.x = b[0];
                    g->data.line.b.y = b[1];
                    g->data.line.b.z = b[2];
                    changed = true;
                }
                if (igIsItemActivated()) {
                    state->editing_entity = e;
                    state->drag_start_line_a = g->data.line.a;
                    state->drag_start_line_b = g->data.line.b;
                }
                if (igIsItemDeactivatedAfterEdit() && state->undo_redo && state->editing_entity == e) {
                    vec3_t new_a = g->data.line.a;
                    vec3_t new_b = g->data.line.b;
                    if (state->drag_start_line_b.x != new_b.x ||
                        state->drag_start_line_b.y != new_b.y ||
                        state->drag_start_line_b.z != new_b.z) {
                        undo_cmd_set_line_endpoints(state->undo_redo, e,
                            state->drag_start_line_a, state->drag_start_line_b, new_a, new_b);
                    }
                }
                break;
            }
            case GEOM_ARC: {
                if (igDragFloat("Line Width", &g->line_width, 0.005f, 0.005f, 0.2f, "%.3f", 0)) {
                    changed = true;
                }
                if (igIsItemActivated()) {
                    state->editing_entity = e;
                    state->drag_start_line_width = g->line_width;
                }
                if (igIsItemDeactivatedAfterEdit() && state->undo_redo && state->editing_entity == e) {
                    if (state->drag_start_line_width != g->line_width) {
                        undo_cmd_set_line_width(state->undo_redo, e, state->drag_start_line_width, g->line_width);
                    }
                }
                // Arc parameters (read-only for now)
                igTextDisabled("Radius: %.2f", g->data.arc.radius);
                igTextDisabled("Angles: %.1f - %.1f deg",
                    mdcad_undo_editor_rad_to_deg(g->data.arc.start_angle),
                    mdcad_undo_editor_rad_to_deg(g->data.arc.end_angle));
                break;
            }
            case GEOM_BEZIER: {
                if (igDragFloat("Line Width", &g->line_width, 0.005f, 0.005f, 0.2f, "%.3f", 0)) {
                    changed = true;
                }
                if (igIsItemActivated()) {
                    state->editing_entity = e;
                    state->drag_start_line_width = g->line_width;
                }
                if (igIsItemDeactivatedAfterEdit() && state->undo_redo && state->editing_entity == e) {
                    if (state->drag_start_line_width != g->line_width) {
                        undo_cmd_set_line_width(state->undo_redo, e, state->drag_start_line_width, g->line_width);
                    }
                }
                igTextDisabled("Segments: %d", g->data.bezier.segments);
                break;
            }
            case GEOM_HELIX: {
                if (igDragFloat("Line Width", &g->line_width, 0.005f, 0.005f, 0.2f, "%.3f", 0)) {
                    changed = true;
                }
                if (igIsItemActivated()) {
                    state->editing_entity = e;
                    state->drag_start_line_width = g->line_width;
                }
                if (igIsItemDeactivatedAfterEdit() && state->undo_redo && state->editing_entity == e) {
                    if (state->drag_start_line_width != g->line_width) {
                        undo_cmd_set_line_width(state->undo_redo, e, state->drag_start_line_width, g->line_width);
                    }
                }
                igTextDisabled("Radius: %.2f", g->data.helix.radius);
                igTextDisabled("Turns: %.1f", g->data.helix.turns);
                break;
            }
            case GEOM_POLYLINE:
            case GEOM_POLYGON: {
                if (igDragFloat("Line Width", &g->line_width, 0.005f, 0.005f, 0.2f, "%.3f", 0)) {
                    changed = true;
                }
                if (igIsItemActivated()) {
                    state->editing_entity = e;
                    state->drag_start_line_width = g->line_width;
                }
                if (igIsItemDeactivatedAfterEdit() && state->undo_redo && state->editing_entity == e) {
                    if (state->drag_start_line_width != g->line_width) {
                        undo_cmd_set_line_width(state->undo_redo, e, state->drag_start_line_width, g->line_width);
                    }
                }
                int count = (g->type == GEOM_POLYLINE) ? g->data.polyline.count : g->data.polygon.count;
                igTextDisabled("Point count: %d", count);
                break;
            }
            case GEOM_TRIANGLE: {
                // Per-vertex color toggle
                if (igCheckbox("Per-Vertex Colors", &g->data.triangle.has_vertex_colors)) {
                    if (g->data.triangle.has_vertex_colors) {
                        // Initialize vertex colors from uniform color
                        g->data.triangle.color_a = g->color;
                        g->data.triangle.color_b = g->color;
                        g->data.triangle.color_c = g->color;
                    }
                    changed = true;
                }
                if (g->data.triangle.has_vertex_colors) {
                    float ca[4] = { g->data.triangle.color_a.x, g->data.triangle.color_a.y,
                                    g->data.triangle.color_a.z, g->data.triangle.color_a.w };
                    if (igColorEdit4("Color A", ca, ImGuiColorEditFlags_AlphaBar)) {
                        g->data.triangle.color_a = (vec4_t){ ca[0], ca[1], ca[2], ca[3] };
                        changed = true;
                    }
                    float cb[4] = { g->data.triangle.color_b.x, g->data.triangle.color_b.y,
                                    g->data.triangle.color_b.z, g->data.triangle.color_b.w };
                    if (igColorEdit4("Color B", cb, ImGuiColorEditFlags_AlphaBar)) {
                        g->data.triangle.color_b = (vec4_t){ cb[0], cb[1], cb[2], cb[3] };
                        changed = true;
                    }
                    float cc[4] = { g->data.triangle.color_c.x, g->data.triangle.color_c.y,
                                    g->data.triangle.color_c.z, g->data.triangle.color_c.w };
                    if (igColorEdit4("Color C", cc, ImGuiColorEditFlags_AlphaBar)) {
                        g->data.triangle.color_c = (vec4_t){ cc[0], cc[1], cc[2], cc[3] };
                        changed = true;
                    }
                }
                break;
            }
            case GEOM_MESH: {
                igTextDisabled("Vertices: %d", g->data.mesh.vertex_count);
                igTextDisabled("Faces: %d", geom_mesh_face_count(&g->data.mesh));
                igTextDisabled("Indices: %d", g->data.mesh.index_count);
                if (g->data.mesh.vertex_colors) {
                    igTextDisabled("Per-vertex colors: Yes");
                }
                break;
            }
            default:
                break;
        }

        if (changed) {
            RenderableComp *r = ecs_world_get_renderable(w, e);
            if (r) r->instance_dirty = true;
        }
    }

    // Renderable section
    RenderableComp *r = ecs_world_get_renderable(w, e);
    if (r && igCollapsingHeader_TreeNodeFlags("Rendering", ImGuiTreeNodeFlags_DefaultOpen)) {
        bool old_visible = r->visible;
        if (igCheckbox("Visible", &r->visible)) {
            r->instance_dirty = true;
            // Record visibility change (checkbox is immediate, no drag tracking needed)
            if (state->undo_redo && old_visible != r->visible) {
                undo_cmd_set_visible(state->undo_redo, e, old_visible, r->visible);
            }
        }
        igInputInt("Layer", &r->layer, 1, 10, 0);

        // Show/Hide all children buttons (only if entity has children)
        int child_count = ecs_world_count_children(w, e);
        if (child_count > 0) {
            if (igButton("Show Children", (ImVec2){0, 0})) {
                ecs_world_set_descendants_visible(w, e, true);
            }
            igSameLine(0, 4);
            if (igButton("Hide Children", (ImVec2){0, 0})) {
                ecs_world_set_descendants_visible(w, e, false);
            }
        }
    }

    // Light section (for light entities)
    LightComp *light = ecs_world_get_light(w, e);
    if (light && igCollapsingHeader_TreeNodeFlags("Light", ImGuiTreeNodeFlags_DefaultOpen)) {
        // Type (read-only)
        igText("Type: %s", light->type == LIGHT_DIRECTIONAL ? "Directional" : "Point");

        // Color (editable)
        float lcolor[3] = { light->color.x, light->color.y, light->color.z };
        if (igColorEdit3("Light Color", lcolor, ImGuiColorEditFlags_None)) {
            light->color.x = lcolor[0];
            light->color.y = lcolor[1];
            light->color.z = lcolor[2];
        }

        // Intensity (editable)
        igDragFloat("Intensity", &light->intensity, 0.05f, 0.0f, 10.0f, "%.2f", 0);

        // Direction / position info from transform
        if (t) {
            igSeparator();
            if (light->type == LIGHT_DIRECTIONAL) {
                igText("Direction: (%.2f, %.2f, %.2f)",
                       t->position.x, t->position.y, t->position.z);
                igTextDisabled("Edit via Transform > Position");
            } else {
                igText("Position: (%.2f, %.2f, %.2f)",
                       t->position.x, t->position.y, t->position.z);
                igTextDisabled("Edit via Transform > Position");
            }
        }
    }
}

//------------------------------------------------------------------------------
// Internal: Draw multi-selection inspector
//------------------------------------------------------------------------------

static inline void ui_entity_inspector_draw_multi(ui_entity_inspector_state_t *state) {
    selection_buffer_t *sel = state->selection;
    ecs_world_state_t *w = state->world;

    igText("%d entities selected", sel->count);
    igSeparator();

    // Show entity list
    if (igCollapsingHeader_TreeNodeFlags("Selected Entities", ImGuiTreeNodeFlags_DefaultOpen)) {
        for (int i = 0; i < sel->count; i++) {
            ecs_entity_t e = sel->entities[i];
            if (!ecs_is_alive(w->world, e)) continue;

            GeometryComp *g = ecs_world_get_geometry(w, e);
            const char *type_name = g ? geometry_type_name(g->type) : "Unknown";

            char label[64];
            snprintf(label, sizeof(label), "%s #%llu", type_name, (unsigned long long)e);
            igBulletText("%s", label);
        }
    }

    igSeparator();

    // Common property editing (color)
    if (igCollapsingHeader_TreeNodeFlags("Common Properties", ImGuiTreeNodeFlags_DefaultOpen)) {
        igTextDisabled("Edit color for all selected:");

        static float common_color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
        static vec4_t *bulk_old_colors = NULL;
        static uint64_t *bulk_entity_ids = NULL;
        static int bulk_count = 0;

        if (igColorEdit4("Set Color", common_color, ImGuiColorEditFlags_AlphaBar)) {
            // Apply to all selected entities
            for (int i = 0; i < sel->count; i++) {
                ecs_entity_t e = sel->entities[i];
                if (!ecs_is_alive(w->world, e)) continue;

                GeometryComp *g = ecs_world_get_geometry(w, e);
                RenderableComp *r = ecs_world_get_renderable(w, e);
                if (g) {
                    g->color.x = common_color[0];
                    g->color.y = common_color[1];
                    g->color.z = common_color[2];
                    g->color.w = common_color[3];
                }
                if (r) r->instance_dirty = true;
            }
        }
        // Capture old colors at edit start for bulk undo
        if (igIsItemActivated() && state->undo_redo) {
            // Allocate and store old colors
            if (bulk_old_colors) free(bulk_old_colors);
            if (bulk_entity_ids) free(bulk_entity_ids);
            bulk_count = sel->count;
            bulk_old_colors = (vec4_t*)malloc(bulk_count * sizeof(vec4_t));
            bulk_entity_ids = (uint64_t*)malloc(bulk_count * sizeof(uint64_t));
            for (int i = 0; i < bulk_count; i++) {
                ecs_entity_t e = sel->entities[i];
                bulk_entity_ids[i] = e;
                GeometryComp *g = ecs_world_get_geometry(w, e);
                if (g) {
                    bulk_old_colors[i] = g->color;
                } else {
                    bulk_old_colors[i] = (vec4_t){1,1,1,1};
                }
            }
        }
        // Record bulk undo at edit end
        if (igIsItemDeactivatedAfterEdit() && state->undo_redo && bulk_count > 0) {
            vec4_t new_color = { common_color[0], common_color[1], common_color[2], common_color[3] };
            undo_cmd_bulk_set_color(state->undo_redo, bulk_entity_ids, bulk_old_colors, bulk_count, new_color);
            // Transfer ownership to undo system
            bulk_entity_ids = NULL;
            bulk_old_colors = NULL;
            bulk_count = 0;
        }

        // Visibility toggle for all
        if (igButton("Show All", (ImVec2){0, 0})) {
            // Record bulk visibility undo
            if (state->undo_redo && sel->count > 0) {
                uint64_t *ids = (uint64_t*)malloc(sel->count * sizeof(uint64_t));
                bool *old_vis = (bool*)malloc(sel->count * sizeof(bool));
                for (int i = 0; i < sel->count; i++) {
                    ecs_entity_t e = sel->entities[i];
                    ids[i] = e;
                    RenderableComp *r = ecs_world_get_renderable(w, e);
                    old_vis[i] = r ? r->visible : true;
                }
                undo_cmd_bulk_set_visible(state->undo_redo, ids, old_vis, sel->count, true);
            }
            // Apply visibility
            for (int i = 0; i < sel->count; i++) {
                ecs_entity_t e = sel->entities[i];
                if (!ecs_is_alive(w->world, e)) continue;

                RenderableComp *r = ecs_world_get_renderable(w, e);
                if (r) {
                    r->visible = true;
                    r->instance_dirty = true;
                }
            }
        }
        igSameLine(0, 5);
        if (igButton("Hide All", (ImVec2){0, 0})) {
            // Record bulk visibility undo
            if (state->undo_redo && sel->count > 0) {
                uint64_t *ids = (uint64_t*)malloc(sel->count * sizeof(uint64_t));
                bool *old_vis = (bool*)malloc(sel->count * sizeof(bool));
                for (int i = 0; i < sel->count; i++) {
                    ecs_entity_t e = sel->entities[i];
                    ids[i] = e;
                    RenderableComp *r = ecs_world_get_renderable(w, e);
                    old_vis[i] = r ? r->visible : false;
                }
                undo_cmd_bulk_set_visible(state->undo_redo, ids, old_vis, sel->count, false);
            }
            // Apply visibility
            for (int i = 0; i < sel->count; i++) {
                ecs_entity_t e = sel->entities[i];
                if (!ecs_is_alive(w->world, e)) continue;

                RenderableComp *r = ecs_world_get_renderable(w, e);
                if (r) {
                    r->visible = false;
                    r->instance_dirty = true;
                }
            }
        }
    }
}

//------------------------------------------------------------------------------
// Main Draw Function
//------------------------------------------------------------------------------

static inline void ui_entity_inspector_draw(ui_entity_inspector_state_t *state) {
    if (!igBegin("Entity Inspector", NULL, 0)) {
        igEnd();
        return;
    }

    selection_buffer_t *sel = state->selection;
    int count = selection_count(sel);

    if (count == 0) {
        igTextDisabled("No entity selected");
        igTextDisabled("Click on an entity to select it");
    } else if (count == 1) {
        // Single entity inspector
        ecs_entity_t e = selection_get(sel, 0);
        if (e != 0 && ecs_is_alive(state->world->world, e)) {
            ui_entity_inspector_draw_single(state, e);
        } else {
            igTextDisabled("Selected entity no longer exists");
        }
    } else {
        // Multi-selection inspector
        ui_entity_inspector_draw_multi(state);
    }

    igEnd();
}

#endif // UI_ENTITY_INSPECTOR_H
