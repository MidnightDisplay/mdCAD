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
#include "../constraints/constraint_selection.h"
#include "../components/geometry_comp.h"
#include "../components/transform_comp.h"
#include "../components/renderable_comp.h"
#include "../components/selectable_comp.h"
#include "../math/math_undo_editor.h"
#include "../undo_redo_exec.h"
#include <ctype.h>
#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

//------------------------------------------------------------------------------
// Types
//------------------------------------------------------------------------------

typedef struct {
    selection_buffer_t *selection;
    ecs_world_state_t *world;
    undo_redo_t *undo_redo;  // Optional, can be NULL
    ecs_entity_t active_sketch;
    bool active_sketch_workspace_open;
    ecs_entity_t script_editor_sketch;
    bool script_editor_open_requested;
    ecs_entity_t script_io_sketch;
    bool script_io_open_requested;

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
    ecs_entity_t *selected_constraint_entity;
    void (*on_sketch_geometry_mutated)(void *user_data);
    void *on_sketch_geometry_mutated_user_data;
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
    state->active_sketch = 0;
    state->active_sketch_workspace_open = false;
    state->script_editor_sketch = 0;
    state->script_editor_open_requested = false;
    state->script_io_sketch = 0;
    state->script_io_open_requested = false;
    state->on_sketch_geometry_mutated = NULL;
    state->on_sketch_geometry_mutated_user_data = NULL;
}

static inline void ui_entity_inspector_set_undo_redo(ui_entity_inspector_state_t *state,
                                                      undo_redo_t *undo_redo) {
    state->undo_redo = undo_redo;
}

static inline void ui_entity_inspector_set_selected_constraint_ptr(ui_entity_inspector_state_t *state,
                                                                    ecs_entity_t *selected_constraint_entity) {
    state->selected_constraint_entity = selected_constraint_entity;
}

static inline void ui_entity_inspector_set_sketch_geometry_mutation_callback(
    ui_entity_inspector_state_t *state,
    void (*callback)(void *user_data),
    void *user_data) {
    if (!state) return;
    state->on_sketch_geometry_mutated = callback;
    state->on_sketch_geometry_mutated_user_data = user_data;
}

static inline void ui_entity_inspector_notify_sketch_geometry_mutated(ui_entity_inspector_state_t *state) {
    if (!state || !state->on_sketch_geometry_mutated) return;
    state->on_sketch_geometry_mutated(state->on_sketch_geometry_mutated_user_data);
}

static inline void ui_entity_inspector_request_script_editor(ui_entity_inspector_state_t *state,
                                                             ecs_entity_t sketch) {
    if (!state || sketch == 0) return;
    state->script_editor_sketch = sketch;
    state->script_editor_open_requested = true;
}

static inline bool ui_entity_inspector_consume_script_editor_open_request(ui_entity_inspector_state_t *state,
                                                                           ecs_entity_t *out_sketch) {
    if (!state || !state->script_editor_open_requested || state->script_editor_sketch == 0) return false;
    if (out_sketch) *out_sketch = state->script_editor_sketch;
    state->script_editor_open_requested = false;
    return true;
}

static inline void ui_entity_inspector_request_script_io(ui_entity_inspector_state_t *state,
                                                         ecs_entity_t sketch) {
    if (!state || sketch == 0) return;
    state->script_io_sketch = sketch;
    state->script_io_open_requested = true;
}

static inline bool ui_entity_inspector_consume_script_io_open_request(ui_entity_inspector_state_t *state,
                                                                       ecs_entity_t *out_sketch) {
    if (!state || !state->script_io_open_requested || state->script_io_sketch == 0) return false;
    if (out_sketch) *out_sketch = state->script_io_sketch;
    state->script_io_open_requested = false;
    return true;
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

static inline int ui_geometry_manager_name_casecmp(const char *a, const char *b) {
    if (!a) a = "";
    if (!b) b = "";
    while (*a && *b) {
        int ca = tolower((unsigned char)*a);
        int cb = tolower((unsigned char)*b);
        if (ca != cb) return (ca < cb) ? -1 : 1;
        a++;
        b++;
    }
    if (*a == '\0' && *b == '\0') return 0;
    return (*a == '\0') ? -1 : 1;
}

static inline const char* ui_geometry_manager_last_underscore(const char *s) {
    if (!s) return NULL;
    const char *last = NULL;
    while (*s) {
        if (*s == '_') last = s;
        s++;
    }
    return last;
}

static inline bool ui_geometry_manager_parse_suffix_number(const char *name,
                                                           int *out_number,
                                                           const char **out_prefix_end) {
    if (!name || !out_number || !out_prefix_end) return false;
    const char *sep = ui_geometry_manager_last_underscore(name);
    if (!sep || *(sep + 1) == '\0') return false;
    const char *p = sep + 1;
    int value = 0;
    while (*p) {
        if (!isdigit((unsigned char)*p)) return false;
        value = value * 10 + (*p - '0');
        p++;
    }
    *out_number = value;
    *out_prefix_end = sep;
    return true;
}

static inline int ui_geometry_manager_compare_entities(ecs_world_state_t *w, ecs_entity_t a, ecs_entity_t b) {
    LabelComp *la = ecs_world_get_label(w, a);
    LabelComp *lb = ecs_world_get_label(w, b);
    const char *name_a = (la && la->name[0] != '\0') ? la->name : NULL;
    const char *name_b = (lb && lb->name[0] != '\0') ? lb->name : NULL;

    if (name_a && name_b) {
        int suffix_num_a = 0;
        int suffix_num_b = 0;
        const char *prefix_end_a = NULL;
        const char *prefix_end_b = NULL;
        bool has_suffix_a = ui_geometry_manager_parse_suffix_number(name_a, &suffix_num_a, &prefix_end_a);
        bool has_suffix_b = ui_geometry_manager_parse_suffix_number(name_b, &suffix_num_b, &prefix_end_b);

        if (has_suffix_a && has_suffix_b) {
            int prefix_len_a = (int)(prefix_end_a - name_a);
            int prefix_len_b = (int)(prefix_end_b - name_b);
            if (prefix_len_a == prefix_len_b) {
                bool same_prefix = true;
                for (int i = 0; i < prefix_len_a; i++) {
                    int ca = tolower((unsigned char)name_a[i]);
                    int cb = tolower((unsigned char)name_b[i]);
                    if (ca != cb) {
                        same_prefix = false;
                        break;
                    }
                }
                if (same_prefix && suffix_num_a != suffix_num_b) {
                    return (suffix_num_a < suffix_num_b) ? -1 : 1;
                }
            }
        }

        int by_name = ui_geometry_manager_name_casecmp(name_a, name_b);
        if (by_name != 0) return by_name;
    } else if (name_a && !name_b) {
        return -1;
    } else if (!name_a && name_b) {
        return 1;
    }

    if (a < b) return -1;
    if (a > b) return 1;
    return 0;
}

static inline void ui_geometry_manager_sort_rows(ecs_world_state_t *w,
                                                 ecs_entity_t *rows,
                                                 int row_count) {
    if (!rows || row_count <= 1) return;
    for (int i = 1; i < row_count; i++) {
        ecs_entity_t key = rows[i];
        int j = i - 1;
        while (j >= 0 && ui_geometry_manager_compare_entities(w, key, rows[j]) < 0) {
            rows[j + 1] = rows[j];
            j--;
        }
        rows[j + 1] = key;
    }
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

static inline void ui_geometry_manager_sync_selection_from_scene(ecs_world_state_t *w,
                                                                 selection_buffer_t *source_selection,
                                                                 ecs_entity_t sketch_entity,
                                                                 ecs_entity_t *row_selection,
                                                                 int *row_selection_count) {
    if (!w || !source_selection || !row_selection || !row_selection_count) return;
    *row_selection_count = 0;

    int selected_total = selection_count(source_selection);
    for (int i = 0; i < selected_total; i++) {
        ecs_entity_t selected = selection_get(source_selection, i);
        if (selected == 0 || !ecs_is_alive(w->world, selected)) continue;

        ecs_entity_t row_entity = selected;
        EndPointsComp *endpoint_meta = ecs_world_get_endpoints(w, selected);
        if (endpoint_meta && endpoint_meta->is_endpoint_point) {
            row_entity = (ecs_entity_t)endpoint_meta->owner_entity;
        }
        if (row_entity == 0 || !ecs_is_alive(w->world, row_entity)) continue;
        if (ecs_world_get_parent(w, row_entity) != sketch_entity) continue;
        if (!ecs_world_get_geometry(w, row_entity)) continue;
        ui_geometry_manager_selection_add(row_selection, row_selection_count, row_entity);
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

static inline ecs_entity_t ui_entity_inspector_geometry_manager_add_point(ui_entity_inspector_state_t *state,
                                                                           ecs_scene_t *scene,
                                                                           ecs_entity_t sketch_entity,
                                                                           vec4_t sketch_color) {
    if (!state || !scene || sketch_entity == 0) return 0;
    ecs_entity_t created = scene_add_point_to_sketch(scene, sketch_entity,
        vec3_make(0.0f, 0.0f, 0.0f), sketch_color, 0.06f);
    if (created == 0) return 0;
    if (state->undo_redo) {
        undo_cmd_create_entity(state->undo_redo, created);
    }
    scene_refresh_sketch_metadata(scene, sketch_entity);
    ui_entity_inspector_notify_sketch_geometry_mutated(state);
    return created;
}

static inline ecs_entity_t ui_entity_inspector_geometry_manager_add_line(ui_entity_inspector_state_t *state,
                                                                          ecs_scene_t *scene,
                                                                          ecs_entity_t sketch_entity,
                                                                          vec4_t sketch_color) {
    if (!state || !scene || sketch_entity == 0) return 0;
    ecs_entity_t created = scene_add_line_to_sketch(scene, sketch_entity,
        vec3_make(-1.0f, 0.0f, 0.0f),
        vec3_make(1.0f, 0.0f, 0.0f),
        sketch_color, 0.03f);
    if (created == 0) return 0;
    if (state->undo_redo) {
        undo_cmd_create_entity(state->undo_redo, created);
    }
    scene_refresh_sketch_metadata(scene, sketch_entity);
    ui_entity_inspector_notify_sketch_geometry_mutated(state);
    return created;
}

static inline ecs_entity_t ui_entity_inspector_geometry_manager_add_arc(ui_entity_inspector_state_t *state,
                                                                         ecs_scene_t *scene,
                                                                         ecs_entity_t sketch_entity,
                                                                         vec4_t sketch_color) {
    if (!state || !scene || sketch_entity == 0) return 0;
    ecs_entity_t created = scene_add_arc_to_sketch(scene, sketch_entity,
        vec3_make(0.0f, 0.0f, 0.0f), 1.0f, 0.0f, 1.5707963f,
        vec3_make(0.0f, 0.0f, 1.0f), sketch_color, 0.03f);
    if (created == 0) return 0;
    if (state->undo_redo) {
        undo_cmd_create_entity(state->undo_redo, created);
    }
    scene_refresh_sketch_metadata(scene, sketch_entity);
    ui_entity_inspector_notify_sketch_geometry_mutated(state);
    return created;
}

static inline ecs_entity_t ui_entity_inspector_geometry_manager_add_circle(ui_entity_inspector_state_t *state,
                                                                            ecs_scene_t *scene,
                                                                            ecs_entity_t sketch_entity,
                                                                            vec4_t sketch_color) {
    if (!state || !scene || sketch_entity == 0) return 0;
    ecs_entity_t created = scene_add_arc_to_sketch(scene, sketch_entity,
        vec3_make(0.0f, 0.0f, 0.0f), 1.0f, 0.0f, 6.2831853f,
        vec3_make(0.0f, 0.0f, 1.0f), sketch_color, 0.03f);
    if (created == 0) return 0;
    if (state->undo_redo) {
        undo_cmd_create_entity(state->undo_redo, created);
    }
    scene_refresh_sketch_metadata(scene, sketch_entity);
    ui_entity_inspector_notify_sketch_geometry_mutated(state);
    return created;
}

static inline int ui_entity_inspector_geometry_manager_bulk_set_fixed(ui_entity_inspector_state_t *state,
                                                                       ecs_world_state_t *w,
                                                                       ecs_scene_t *scene,
                                                                       ecs_entity_t sketch_entity,
                                                                       const ecs_entity_t *targets,
                                                                       int target_count,
                                                                       bool fixed_value) {
    if (!state || !w || !scene || sketch_entity == 0 || !targets || target_count <= 0) return 0;

    bool *old_fixed = NULL;
    if (state->undo_redo) {
        old_fixed = (bool*)malloc((size_t)target_count * sizeof(bool));
    }

    ecs_entity_t action_entities[UI_GEOMETRY_MANAGER_MAX_ROWS];
    int action_count = 0;
    for (int i = 0; i < target_count; i++) {
        ecs_entity_t target = targets[i];
        if (!ecs_is_alive(w->world, target) || !ecs_world_get_geometry(w, target)) continue;

        SketchGeometryStateComp *row_state = ecs_world_get_sketch_geometry_state(w, target);
        if (!row_state) {
            SketchGeometryStateComp init_state = sketch_geometry_state_comp_default();
            ecs_world_set_sketch_geometry_state(w, target, &init_state);
            row_state = ecs_world_get_sketch_geometry_state(w, target);
        }
        if (!row_state) continue;

        if (old_fixed) old_fixed[action_count] = row_state->fixed;
        row_state->fixed = fixed_value;
        action_entities[action_count++] = target;
    }

    if (state->undo_redo && action_count > 0 && old_fixed) {
        undo_cmd_bulk_set_sketch_fixed(state->undo_redo, action_entities, old_fixed, action_count, fixed_value);
    }
    if (old_fixed) free(old_fixed);

    if (action_count <= 0) return 0;
    scene_refresh_sketch_metadata(scene, sketch_entity);
    ui_entity_inspector_notify_sketch_geometry_mutated(state);
    return action_count;
}

static inline int ui_entity_inspector_geometry_manager_bulk_delete(ui_entity_inspector_state_t *state,
                                                                    ecs_world_state_t *w,
                                                                    ecs_scene_t *scene,
                                                                    selection_buffer_t *selection,
                                                                    const ecs_entity_t *targets,
                                                                    int target_count) {
    if (!state || !w || !scene || !targets || target_count <= 0) return 0;

    ecs_entity_t delete_entities[UI_GEOMETRY_MANAGER_MAX_ROWS];
    int delete_count = 0;
    for (int i = 0; i < target_count; i++) {
        ecs_entity_t target = targets[i];
        if (!ecs_is_alive(w->world, target) || !ecs_world_get_geometry(w, target)) continue;
        delete_entities[delete_count++] = target;
    }
    if (delete_count <= 0) return 0;

    if (state->undo_redo) {
        undo_cmd_bulk_delete_entities(state->undo_redo, delete_entities, delete_count);
    }

    if (selection) {
        for (int i = 0; i < delete_count; i++) {
            selection_remove(selection, delete_entities[i]);
        }
    }
    for (int i = 0; i < delete_count; i++) {
        scene_remove_entity(scene, delete_entities[i]);
    }
    if (selection) {
        selection_prune_dead(selection);
    }

    ui_entity_inspector_notify_sketch_geometry_mutated(state);
    return delete_count;
}

static inline uint8_t ui_constraint_display_decimals(const ConstraintComp *constraint) {
    if (!constraint) return 0;
    return constraint->display_decimals;
}

static inline float ui_constraint_angle_rad_to_deg(float radians) {
    return radians * (180.0f / 3.14159265359f);
}

static inline float ui_constraint_angle_deg_to_rad(float degrees) {
    return degrees * (3.14159265359f / 180.0f);
}

static inline float ui_constraint_user_value_from_scene(const ConstraintComp *constraint, float scene_value) {
    if (!constraint) return scene_value;
    if (constraint->type == CONSTRAINT_ANGLE) {
        return ui_constraint_angle_rad_to_deg(scene_value);
    }
    return scene_value;
}

static inline float ui_constraint_scene_value_from_user(const ConstraintComp *constraint, float user_value) {
    if (!constraint) return user_value;
    if (constraint->type == CONSTRAINT_ANGLE) {
        return ui_constraint_angle_deg_to_rad(user_value);
    }
    return user_value;
}

static inline void ui_constraint_value_format(const ConstraintComp *constraint,
                                              char *out_fmt, size_t out_fmt_size) {
    constraint_build_float_format(out_fmt, out_fmt_size, ui_constraint_display_decimals(constraint));
}

static inline void ui_constraint_value_text(const ConstraintComp *constraint,
                                            char *out_text, size_t out_text_size) {
    if (!constraint || !out_text || out_text_size == 0) return;
    constraint_format_value(out_text, out_text_size,
                            ui_constraint_user_value_from_scene(constraint, constraint->value),
                            ui_constraint_display_decimals(constraint));
}

static inline bool ui_entity_inspector_draw_sketch_geometry_manager(ui_entity_inspector_state_t *state,
                                                                     ecs_entity_t e,
                                                                     SketchComp **sketch_ref,
                                                                     ecs_scene_t *scene,
                                                                     bool as_workspace_window) {
    ecs_world_state_t *w = state->world;
    SketchComp *sketch = sketch_ref ? *sketch_ref : NULL;
    bool section_changed = false;
    static ecs_entity_t gm_selected_entities[UI_GEOMETRY_MANAGER_MAX_ROWS] = {0};
    static int gm_selected_count = 0;
    static ecs_entity_t gm_bound_sketch = 0;
    static int gm_delete_pending_count = 0;

    if (!sketch) return false;
    if (!as_workspace_window && !igCollapsingHeader_TreeNodeFlags("GeometryManager", ImGuiTreeNodeFlags_DefaultOpen)) {
        return false;
    }
    if (as_workspace_window) {
        igTextDisabled("GeometryManager");
        igSeparator();
    }

    if (gm_bound_sketch != e) {
        gm_bound_sketch = e;
        gm_selected_count = 0;
    }

    if (state->selection) {
        ui_geometry_manager_sync_selection_from_scene(
            w, state->selection, e, gm_selected_entities, &gm_selected_count);
    } else {
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
    ui_geometry_manager_sort_rows(w, geometry_rows, geometry_row_count);

    for (int i = gm_selected_count - 1; i >= 0; i--) {
        if (!ui_geometry_manager_contains(geometry_rows, geometry_row_count, gm_selected_entities[i])) {
            gm_selected_entities[i] = gm_selected_entities[gm_selected_count - 1];
            gm_selected_count--;
        }
    }

    igTextDisabled("Add geometry");
    igBeginDisabled(scene == NULL);
    if (igButton("Add Point##geometry_manager_add_point", (ImVec2){0, 0})) {
        ecs_entity_t created = ui_entity_inspector_geometry_manager_add_point(state, scene, e, sketch->color);
        if (created != 0) {
            sketch = ecs_world_get_sketch(w, e);
            gm_selected_entities[0] = created;
            gm_selected_count = 1;
            section_changed = true;
        }
    }
    igSameLine(0, 8);
    if (igButton("Add Line##geometry_manager_add_line", (ImVec2){0, 0})) {
        ecs_entity_t created = ui_entity_inspector_geometry_manager_add_line(state, scene, e, sketch->color);
        if (created != 0) {
            sketch = ecs_world_get_sketch(w, e);
            gm_selected_entities[0] = created;
            gm_selected_count = 1;
            section_changed = true;
        }
    }
    igSameLine(0, 8);
    if (igButton("Add Arc##geometry_manager_add_arc", (ImVec2){0, 0})) {
        ecs_entity_t created = ui_entity_inspector_geometry_manager_add_arc(state, scene, e, sketch->color);
        if (created != 0) {
            sketch = ecs_world_get_sketch(w, e);
            gm_selected_entities[0] = created;
            gm_selected_count = 1;
            section_changed = true;
        }
    }
    igSameLine(0, 8);
    if (igButton("Add Circle##geometry_manager_add_circle", (ImVec2){0, 0})) {
        ecs_entity_t created = ui_entity_inspector_geometry_manager_add_circle(state, scene, e, sketch->color);
        if (created != 0) {
            sketch = ecs_world_get_sketch(w, e);
            gm_selected_entities[0] = created;
            gm_selected_count = 1;
            section_changed = true;
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
                selection_handle_click(state->selection, child, io->KeyShift, io->KeyCtrl);
            }
        }
    }

    igDummy((ImVec2){0.0f, 8.0f});
    igTextDisabled("Selected rows: %d", gm_selected_count);

    igBeginDisabled(gm_selected_count <= 0);
    if (igButton("Fix##geometry_manager_bulk_fix", (ImVec2){0, 0})) {
        int action_count = ui_entity_inspector_geometry_manager_bulk_set_fixed(
            state, w, scene, e, gm_selected_entities, gm_selected_count, true);
        if (action_count > 0) {
            sketch = ecs_world_get_sketch(w, e);
            section_changed = true;
        }
    }
    igEndDisabled();
    igSameLine(0, 8);
    igBeginDisabled(gm_selected_count <= 0);
    if (igButton("Unfix##geometry_manager_bulk_unfix", (ImVec2){0, 0})) {
        int action_count = ui_entity_inspector_geometry_manager_bulk_set_fixed(
            state, w, scene, e, gm_selected_entities, gm_selected_count, false);
        if (action_count > 0) {
            sketch = ecs_world_get_sketch(w, e);
            section_changed = true;
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
            int delete_count = ui_entity_inspector_geometry_manager_bulk_delete(
                state, w, scene, state->selection, gm_selected_entities, gm_selected_count);

            gm_selected_count = 0;
            gm_delete_pending_count = 0;

            if (delete_count > 0) {
                sketch = ecs_world_get_sketch(w, e);
                section_changed = true;
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

    if (sketch_ref) *sketch_ref = sketch;
    return section_changed;
}

static inline bool ui_entity_inspector_draw_sketch_constraint_manager(ui_entity_inspector_state_t *state,
                                                                       ecs_entity_t e,
                                                                       SketchComp **sketch_ref,
                                                                       ecs_scene_t *scene,
                                                                       bool as_workspace_window) {
    ecs_world_state_t *w = state->world;
    SketchComp *sketch = sketch_ref ? *sketch_ref : NULL;
    bool section_changed = false;
    static int cm_type_filter = 0;
    static char cm_search_filter[128] = "";
    static ecs_entity_t cm_delete_pending_constraint = 0;

    if (!sketch) return false;
    if (!as_workspace_window && !igCollapsingHeader_TreeNodeFlags("ConstraintManager", ImGuiTreeNodeFlags_DefaultOpen)) {
        return false;
    }
    if (as_workspace_window) {
        igTextDisabled("ConstraintManager");
        igSeparator();
    }

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
                char numeric[64];
                ui_constraint_value_text(constraint, numeric, sizeof(numeric));
                snprintf(value_text, sizeof(value_text), "%s%s",
                         numeric, constraint->driven ? " (Driven)" : "");
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
                    constraint_selection_apply_participants(state->selection, w, c_e);
                    if (state->selected_constraint_entity) {
                        *state->selected_constraint_entity = c_e;
                    }
                }

                if (constraint_type_is_dimensional(constraint->type)) {
                    char value_id[96];
                    snprintf(value_id, sizeof(value_id), "Value##constraint_value_%llu", (unsigned long long)c_e);
                    float edit_value = ui_constraint_user_value_from_scene(constraint, constraint->value);
                    char value_fmt[16];
                    ui_constraint_value_format(constraint, value_fmt, sizeof(value_fmt));
                    igSetNextItemWidth(140.0f);
                    float step = (constraint->type == CONSTRAINT_ANGLE) ? 1.0f : 0.1f;
                    float step_fast = (constraint->type == CONSTRAINT_ANGLE) ? 5.0f : 1.0f;
                    if (igInputFloat(value_id, &edit_value, step, step_fast, value_fmt,
                                     ImGuiInputTextFlags_CharsDecimal)) {
                        if (scene) {
                            scene_constraint_set_dimensional_value(
                                scene, c_e, ui_constraint_scene_value_from_user(constraint, edit_value), constraint->driven);
                            section_changed = true;
                        }
                    }
                igSameLine(0, 8);
                char driven_id[96];
                snprintf(driven_id, sizeof(driven_id), "Driven##constraint_driven_%llu", (unsigned long long)c_e);
                bool driven = constraint->driven;
                if (igCheckbox(driven_id, &driven)) {
                    if (scene) {
                        scene_constraint_set_dimensional_value(scene, c_e, constraint->value, driven);
                        section_changed = true;
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
                if (scene_remove_constraint(scene, cm_delete_pending_constraint)) {
                    selection_clear(state->selection);
                    section_changed = true;
                }
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

    if (sketch_ref) *sketch_ref = sketch;
    return section_changed;
}

static inline void ui_entity_inspector_draw_solver_section(ui_entity_inspector_state_t *state,
                                                           ecs_world_state_t *w,
                                                           ecs_entity_t e,
                                                           SketchComp **sketch_ref,
                                                           ecs_scene_t *scene) {
    if (!state || !w || !sketch_ref || !(*sketch_ref)) return;
    SketchComp *sketch = *sketch_ref;
    static ecs_entity_t solver_clear_pending_sketch = 0;

    igTextDisabled("Solver");
    igText("Backend: %s", scene ? scene_solver_backend_name(scene) : "ConstraintSketchSolverV1");
    igTextDisabled("Backend ID: %u", scene ? scene_solver_backend_id(scene) : 1u);
    igText("Status: %s", sketch_status_name(sketch->status));
    if (sketch->last_solve_timestamp_ms > 0) {
        igTextDisabled("Last solve (epoch ms): %llu",
                       (unsigned long long)sketch->last_solve_timestamp_ms);
    } else {
        igTextDisabled("Last solve: never");
    }

    bool auto_solve_enabled = sketch->auto_solve_enabled;
    if (igCheckbox("Auto-solve##sketch_solver_auto", &auto_solve_enabled)) {
        if (scene) {
            scene_solver_set_auto_solve(scene, e, auto_solve_enabled);
            if (auto_solve_enabled) {
                scene_solver_request_auto(scene, e);
            }
            sketch = ecs_world_get_sketch(w, e);
        } else {
            sketch->auto_solve_enabled = auto_solve_enabled;
        }
    }

    float position_tolerance = scene
        ? scene_solver_position_tolerance(scene, e)
        : ((sketch->solver_position_tolerance > 0.0f)
               ? sketch->solver_position_tolerance
               : SKETCH_SOLVER_DEFAULT_POSITION_TOLERANCE);
    igSetNextItemWidth(180.0f);
    if (igInputFloat("Position Tolerance##sketch_solver_position_tol", &position_tolerance, 0.0001f, 0.001f, "%.6f",
                     ImGuiInputTextFlags_CharsDecimal)) {
        if (scene) {
            scene_solver_set_position_tolerance(scene, e, position_tolerance);
            sketch = ecs_world_get_sketch(w, e);
        } else {
            sketch->solver_position_tolerance =
                (position_tolerance > 0.0f) ? position_tolerance : SKETCH_SOLVER_DEFAULT_POSITION_TOLERANCE;
        }
    }

    float angle_tolerance = scene
        ? scene_solver_angle_tolerance(scene, e)
        : ((sketch->solver_angle_tolerance > 0.0f)
               ? sketch->solver_angle_tolerance
               : SKETCH_SOLVER_DEFAULT_ANGLE_TOLERANCE);
    igSetNextItemWidth(180.0f);
    if (igInputFloat("Angle Tolerance##sketch_solver_angle_tol", &angle_tolerance, 0.0001f, 0.001f, "%.6f",
                     ImGuiInputTextFlags_CharsDecimal)) {
        if (scene) {
            scene_solver_set_angle_tolerance(scene, e, angle_tolerance);
            sketch = ecs_world_get_sketch(w, e);
        } else {
            sketch->solver_angle_tolerance =
                (angle_tolerance > 0.0f) ? angle_tolerance : SKETCH_SOLVER_DEFAULT_ANGLE_TOLERANCE;
        }
    }

    uint32_t max_passes = scene
        ? scene_solver_max_passes(scene, e)
        : ((sketch->solver_max_passes > 0) ? sketch->solver_max_passes : SKETCH_SOLVER_DEFAULT_MAX_PASSES);
    int max_passes_edit = (int)max_passes;
    igSetNextItemWidth(180.0f);
    if (igInputInt("Max Passes##sketch_solver_max_passes", &max_passes_edit, 1, 10, 0)) {
        if (max_passes_edit < 1) max_passes_edit = 1;
        if (scene) {
            scene_solver_set_max_passes(scene, e, (uint32_t)max_passes_edit);
            sketch = ecs_world_get_sketch(w, e);
        } else {
            sketch->solver_max_passes = (uint32_t)max_passes_edit;
        }
    }

    igBeginDisabled(scene == NULL);
    if (igButton("Recalculate Sketch", (ImVec2){0, 0})) {
        scene_solver_request_recalculate(scene, e);
        scene_solver_add_diagnostic(scene, e, SKETCH_SOLVER_DIAG_INFO,
                                    "manual", "Manual solve requested via Recalculate Sketch.", 0);
        sketch = ecs_world_get_sketch(w, e);
    }
    igEndDisabled();

    igDummy((ImVec2){0.0f, 8.0f});
    igTextDisabled("Diagnostics");
    int diag_count = scene ? scene_solver_diagnostic_count(scene, e) : 0;
    if (diag_count <= 0) {
        igTextDisabled("No solver diagnostics yet");
        igTextWrapped("Run Recalculate Sketch or edit sketch geometry/constraints to generate diagnostics for this sketch.");
    } else {
        igTextDisabled("Identical consecutive diagnostics are suppressed.");
        for (int i = 0; i < diag_count; i++) {
            const sketch_solver_diagnostic_t *diag = scene_solver_diagnostic_at(scene, e, i);
            if (!diag) continue;
            const char *level = scene_solver_diagnostic_level_name(diag->severity);
            const char *ts = diag->timestamp[0] ? diag->timestamp : "n/a";
            igText("[%s] %s", level, ts);
            igSameLine(0, 8.0f);
            igTextWrapped("%s", diag->message[0] ? diag->message : "(no message)");
        }
    }

    igBeginDisabled(scene == NULL);
    if (igButton("Clear Diagnostics History##sketch_solver_clear", (ImVec2){0, 0})) {
        solver_clear_pending_sketch = e;
        igOpenPopup_Str("Clear Diagnostics History##sketch_solver_clear_popup", 0);
    }
    igEndDisabled();

    if (igBeginPopupModal("Clear Diagnostics History##sketch_solver_clear_popup", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        igTextWrapped("Clear Diagnostics History: Clear all saved solver diagnostics for this sketch? This action cannot be undone.");
        igDummy((ImVec2){0.0f, 8.0f});
        if (igButton("Clear Diagnostics History##sketch_solver_confirm_clear", (ImVec2){220.0f, 0.0f})) {
            if (scene && solver_clear_pending_sketch != 0) {
                scene_solver_clear_diagnostics(scene, solver_clear_pending_sketch);
                sketch = ecs_world_get_sketch(w, solver_clear_pending_sketch);
            }
            solver_clear_pending_sketch = 0;
            igCloseCurrentPopup();
        }
        igSameLine(0, 8);
        if (igButton("Cancel##sketch_solver_cancel_clear", (ImVec2){120.0f, 0.0f})) {
            solver_clear_pending_sketch = 0;
            igCloseCurrentPopup();
        }
        igEndPopup();
    }

    *sketch_ref = sketch;
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

        bool is_active_sketch = (state->active_sketch == e);
        igDummy((ImVec2){0.0f, 8.0f});
        igTextDisabled("Workspace");
        igText("Sketch workspace: %s", is_active_sketch ? "Active" : "Inactive");
        if (igButton(is_active_sketch ? "Deactivate Sketch##sketch_workspace_toggle"
                                      : "Activate Sketch##sketch_workspace_toggle",
                     (ImVec2){0, 0})) {
            if (is_active_sketch) {
                state->active_sketch = 0;
                state->active_sketch_workspace_open = false;
            } else {
                state->active_sketch = e;
                state->active_sketch_workspace_open = true;
            }
        }

        igSameLine(0, 8);
        if (igButton("Open Script Editor##sketch_script_editor_open", (ImVec2){0, 0})) {
            ui_entity_inspector_request_script_editor(state, e);
        }
        igSameLine(0, 8);
        if (igButton("Open Script IO##sketch_script_io_open", (ImVec2){0, 0})) {
            ui_entity_inspector_request_script_io(state, e);
        }

        igDummy((ImVec2){0.0f, 8.0f});
        igSeparator();
        igDummy((ImVec2){0.0f, 8.0f});

        bool suppress_solver_in_inspector = (state->active_sketch_workspace_open && state->active_sketch == e);
        if (!suppress_solver_in_inspector) {
            ui_entity_inspector_draw_solver_section(state, w, e, &sketch, scene);
        } else {
            igTextDisabled("Solver controls moved to Active Sketch Workspace.");
        }
    }

    if (sketch &&
        (state->active_sketch == 0 || state->active_sketch != e) &&
        igCollapsingHeader_TreeNodeFlags("GeometryManager", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (gm_bound_sketch != e) {
            gm_bound_sketch = e;
            gm_selected_count = 0;
        }

        if (state->selection) {
            ui_geometry_manager_sync_selection_from_scene(
                w, state->selection, e, gm_selected_entities, &gm_selected_count);
        } else {
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
        ui_geometry_manager_sort_rows(w, geometry_rows, geometry_row_count);

        for (int i = gm_selected_count - 1; i >= 0; i--) {
            if (!ui_geometry_manager_contains(geometry_rows, geometry_row_count, gm_selected_entities[i])) {
                gm_selected_entities[i] = gm_selected_entities[gm_selected_count - 1];
                gm_selected_count--;
            }
        }

        igTextDisabled("Add geometry");
        igBeginDisabled(scene == NULL);
        if (igButton("Add Point##geometry_manager_add_point", (ImVec2){0, 0})) {
            ecs_entity_t created = ui_entity_inspector_geometry_manager_add_point(state, scene, e, sketch->color);
            if (created != 0) {
                sketch = ecs_world_get_sketch(w, e);
                gm_selected_entities[0] = created;
                gm_selected_count = 1;
            }
        }
        igSameLine(0, 8);
        if (igButton("Add Line##geometry_manager_add_line", (ImVec2){0, 0})) {
            ecs_entity_t created = ui_entity_inspector_geometry_manager_add_line(state, scene, e, sketch->color);
            if (created != 0) {
                sketch = ecs_world_get_sketch(w, e);
                gm_selected_entities[0] = created;
                gm_selected_count = 1;
            }
        }
        igSameLine(0, 8);
        if (igButton("Add Arc##geometry_manager_add_arc", (ImVec2){0, 0})) {
            ecs_entity_t created = ui_entity_inspector_geometry_manager_add_arc(state, scene, e, sketch->color);
            if (created != 0) {
                sketch = ecs_world_get_sketch(w, e);
                gm_selected_entities[0] = created;
                gm_selected_count = 1;
            }
        }
        igSameLine(0, 8);
        if (igButton("Add Circle##geometry_manager_add_circle", (ImVec2){0, 0})) {
            ecs_entity_t created = ui_entity_inspector_geometry_manager_add_circle(state, scene, e, sketch->color);
            if (created != 0) {
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
                    selection_handle_click(state->selection, child, io->KeyShift, io->KeyCtrl);
                }
            }
        }

        igDummy((ImVec2){0.0f, 8.0f});
        igTextDisabled("Selected rows: %d", gm_selected_count);

        igBeginDisabled(gm_selected_count <= 0);
        if (igButton("Fix##geometry_manager_bulk_fix", (ImVec2){0, 0})) {
            int action_count = ui_entity_inspector_geometry_manager_bulk_set_fixed(
                state, w, scene, e, gm_selected_entities, gm_selected_count, true);
            if (action_count > 0) {
                sketch = ecs_world_get_sketch(w, e);
            }
        }
        igEndDisabled();
        igSameLine(0, 8);
        igBeginDisabled(gm_selected_count <= 0);
        if (igButton("Unfix##geometry_manager_bulk_unfix", (ImVec2){0, 0})) {
            int action_count = ui_entity_inspector_geometry_manager_bulk_set_fixed(
                state, w, scene, e, gm_selected_entities, gm_selected_count, false);
            if (action_count > 0) {
                sketch = ecs_world_get_sketch(w, e);
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
                int delete_count = ui_entity_inspector_geometry_manager_bulk_delete(
                    state, w, scene, state->selection, gm_selected_entities, gm_selected_count);

                gm_selected_count = 0;
                gm_delete_pending_count = 0;

                if (delete_count > 0) {
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

    if (sketch &&
        (state->active_sketch == 0 || state->active_sketch != e) &&
        igCollapsingHeader_TreeNodeFlags("ConstraintManager", ImGuiTreeNodeFlags_DefaultOpen)) {
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
                    char numeric[64];
                    ui_constraint_value_text(constraint, numeric, sizeof(numeric));
                    snprintf(value_text, sizeof(value_text), "%s%s",
                             numeric, constraint->driven ? " (Driven)" : "");
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
                constraint_selection_apply_participants(state->selection, w, c_e);
                if (state->selected_constraint_entity) {
                    *state->selected_constraint_entity = c_e;
                }
            }

                if (constraint_type_is_dimensional(constraint->type)) {
                    char value_id[96];
                    snprintf(value_id, sizeof(value_id), "Value##constraint_value_%llu", (unsigned long long)c_e);
                    float edit_value = constraint->value;
                    char value_fmt[16];
                    ui_constraint_value_format(constraint, value_fmt, sizeof(value_fmt));
                    igSetNextItemWidth(140.0f);
                    if (igInputFloat(value_id, &edit_value, 0.1f, 1.0f, value_fmt,
                                     ImGuiInputTextFlags_CharsDecimal)) {
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
                    if (scene_remove_constraint(scene, cm_delete_pending_constraint)) {
                        selection_clear(state->selection);
                    }
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

    // SketchGeometry section (geometry-side linked constraint management)
    GeometryComp *g = ecs_world_get_geometry(w, e);
    if (g && igCollapsingHeader_TreeNodeFlags("SketchGeometry", ImGuiTreeNodeFlags_DefaultOpen)) {
        ConstraintParticipantComp *linked_refs = ecs_world_get_constraint_participant(w, e);
        uint32_t linked_count = linked_refs ? linked_refs->constraint_count : 0;
        igText("Linked constraints: %u", linked_count);

        ecs_entity_t delete_constraint_entity = 0;
        if (!linked_refs || linked_refs->constraint_count == 0) {
            igTextDisabled("No linked constraints");
        } else {
            for (uint32_t ci = 0; ci < linked_refs->constraint_count; ci++) {
                ecs_entity_t linked_constraint = (ecs_entity_t)linked_refs->constraints[ci];
                if (linked_constraint == 0 || !ecs_is_alive(w->world, linked_constraint)) continue;

                ConstraintComp *linked = ecs_world_get_constraint(w, linked_constraint);
                if (!linked) continue;

                LabelComp *linked_label = ecs_world_get_label(w, linked_constraint);
                char linked_name[160];
                if (linked_label && linked_label->name[0] != '\0') {
                    snprintf(linked_name, sizeof(linked_name), "%s", linked_label->name);
                } else {
                    snprintf(linked_name, sizeof(linked_name), "#%llu", (unsigned long long)linked_constraint);
                }

                igPushID_Int((int)ci);
                igText("%s (%s)", linked_name, constraint_type_display_name(linked->type));
                igSameLine(0, 8);
                if (igSmallButton("Select")) {
                    selection_clear(state->selection);
                    selection_add(state->selection, linked_constraint);
                }

                if (constraint_type_is_dimensional(linked->type)) {
                    float edit_value = ui_constraint_user_value_from_scene(linked, linked->value);
                    char value_fmt[16];
                    ui_constraint_value_format(linked, value_fmt, sizeof(value_fmt));
                    igSameLine(0, 8);
                    igSetNextItemWidth(110.0f);
                    float step = (linked->type == CONSTRAINT_ANGLE) ? 1.0f : 0.1f;
                    float step_fast = (linked->type == CONSTRAINT_ANGLE) ? 5.0f : 1.0f;
                    if (igInputFloat("##linked_constraint_value", &edit_value, step, step_fast, value_fmt,
                                     ImGuiInputTextFlags_CharsDecimal)) {
                        if (scene) {
                            scene_constraint_set_dimensional_value(
                                scene, linked_constraint, ui_constraint_scene_value_from_user(linked, edit_value), linked->driven);
                        }
                    }
                }

                igSameLine(0, 8);
                igBeginDisabled(scene == NULL);
                if (igSmallButton("Delete")) {
                    delete_constraint_entity = linked_constraint;
                }
                igEndDisabled();
                igPopID();

                if (delete_constraint_entity != 0) {
                    break;
                }
            }
        }

        if (delete_constraint_entity != 0 && scene) {
            scene_remove_constraint(scene, delete_constraint_entity);
        }
    }

    // SketchConstraint section (constraint-side participant management)
    ConstraintComp *constraint_entity = ecs_world_get_constraint(w, e);
    if (constraint_entity &&
        igCollapsingHeader_TreeNodeFlags("SketchConstraint", ImGuiTreeNodeFlags_DefaultOpen)) {
        igText("Type: %s", constraint_type_display_name(constraint_entity->type));
        igText("Participants: %u", constraint_entity->participant_count);

        if (constraint_type_is_dimensional(constraint_entity->type)) {
            float edit_value = ui_constraint_user_value_from_scene(constraint_entity, constraint_entity->value);
            char value_fmt[16];
            ui_constraint_value_format(constraint_entity, value_fmt, sizeof(value_fmt));
            igSetNextItemWidth(140.0f);
            float step = (constraint_entity->type == CONSTRAINT_ANGLE) ? 1.0f : 0.1f;
            float step_fast = (constraint_entity->type == CONSTRAINT_ANGLE) ? 5.0f : 1.0f;
            if (igInputFloat("Value##selected_constraint_value", &edit_value, step, step_fast, value_fmt,
                             ImGuiInputTextFlags_CharsDecimal)) {
                if (scene) {
                    scene_constraint_set_dimensional_value(
                        scene, e, ui_constraint_scene_value_from_user(constraint_entity, edit_value), constraint_entity->driven);
                }
            }
            igSameLine(0, 8);
            bool driven = constraint_entity->driven;
            if (igCheckbox("Driven##selected_constraint_driven", &driven)) {
                if (scene) {
                    scene_constraint_set_dimensional_value(scene, e, constraint_entity->value, driven);
                }
            }
        }

        uint32_t min_participants = constraint_type_min_participants(constraint_entity->type);
        ecs_entity_t remove_participant_entity = 0;
        for (uint32_t pi = 0; pi < constraint_entity->participant_count; pi++) {
            ecs_entity_t participant = (ecs_entity_t)constraint_entity->participants[pi];
            if (participant == 0 || !ecs_is_alive(w->world, participant)) continue;

            LabelComp *p_label = ecs_world_get_label(w, participant);
            char participant_name[160];
            if (p_label && p_label->name[0] != '\0') {
                snprintf(participant_name, sizeof(participant_name), "%s", p_label->name);
            } else {
                snprintf(participant_name, sizeof(participant_name), "#%llu", (unsigned long long)participant);
            }

            igPushID_Int((int)pi + 10000);
            igText("%s", participant_name);
            igSameLine(0, 8);
            if (igSmallButton("Select")) {
                selection_clear(state->selection);
                selection_add(state->selection, participant);
            }

            bool can_remove_participant = scene != NULL && constraint_entity->participant_count > min_participants;
            igSameLine(0, 8);
            igBeginDisabled(!can_remove_participant);
            if (igSmallButton("Remove")) {
                remove_participant_entity = participant;
            }
            igEndDisabled();
            igPopID();

            if (remove_participant_entity != 0) {
                break;
            }
        }

        if (remove_participant_entity != 0 && scene) {
            scene_constraint_remove_participant(scene, e, remove_participant_entity);
            if (!ecs_is_alive(w->world, e)) {
                selection_clear(state->selection);
                return;
            }
        }

        igBeginDisabled(scene == NULL);
        if (igButton("Delete Constraint##selected_constraint_delete", (ImVec2){0, 0})) {
            scene_remove_constraint(scene, e);
            selection_clear(state->selection);
            igEndDisabled();
            return;
        }
        igEndDisabled();
    }

    // Geometry section
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
                        EndPointsComp *endpoint_meta = ecs_world_get_endpoints(w, e);
                        if (endpoint_meta && endpoint_meta->is_endpoint_point && scene) {
                            undo_cmd_record_endpoint_participant_move_if_changed(
                                state->undo_redo, scene, e, state->drag_start_point_pos, new_pt);
                        } else {
                            undo_cmd_set_point_position(state->undo_redo, e, state->drag_start_point_pos, new_pt);
                        }
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
            if (scene) {
                EndPointsComp *endpoint_meta = ecs_world_get_endpoints(w, e);
                if (endpoint_meta && endpoint_meta->is_endpoint_point) {
                    scene_sync_owner_geometry_from_endpoint_entity(scene, e);
                } else {
                    scene_sync_endpoint_entities_for_owner(scene, e);
                    ecs_entity_t parent = scene_get_parent(scene, e);
                    if (parent != 0 && scene_is_sketch(scene, parent)) {
                        scene_solver_request_auto(scene, parent);
                        scene_script_reemit_for_sketch(scene, parent);
                    }
                }
            }
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
    if (state->active_sketch != 0 && !ecs_is_alive(state->world->world, state->active_sketch)) {
        state->active_sketch = 0;
        state->active_sketch_workspace_open = false;
    }
    if (state->script_editor_sketch != 0 && !ecs_is_alive(state->world->world, state->script_editor_sketch)) {
        state->script_editor_sketch = 0;
        state->script_editor_open_requested = false;
    }

    if (state->active_sketch != 0 && state->active_sketch_workspace_open) {
        bool open = true;
        igSetNextWindowSize((ImVec2){760.0f, 560.0f}, ImGuiCond_FirstUseEver);
        if (igBegin("Active Sketch Workspace", &open, ImGuiWindowFlags_None)) {
            if (!ecs_is_alive(state->world->world, state->active_sketch)) {
                igTextDisabled("Active sketch no longer exists.");
                state->active_sketch = 0;
                state->active_sketch_workspace_open = false;
            } else {
                ecs_entity_t sketch_entity = state->active_sketch;
                ecs_scene_t *scene = NULL;
                if (state->undo_redo && state->undo_redo->scene) {
                    scene = (ecs_scene_t*)state->undo_redo->scene;
                }

                SketchComp *active_sketch = ecs_world_get_sketch(state->world, sketch_entity);
                LabelComp *active_label = ecs_world_get_label(state->world, sketch_entity);
                const char *active_name = (active_label && active_label->name[0] != '\0')
                    ? active_label->name : "Sketch";
                igText("Active sketch: %s (#%llu)", active_name, (unsigned long long)sketch_entity);

                if (scene) {
                    scene_refresh_sketch_metadata(scene, sketch_entity);
                    active_sketch = ecs_world_get_sketch(state->world, sketch_entity);
                }

                if (active_sketch) {
                    ui_entity_inspector_draw_solver_section(state, state->world, sketch_entity, &active_sketch, scene);
                    igDummy((ImVec2){0.0f, 10.0f});
                    igSeparator();
                    igDummy((ImVec2){0.0f, 10.0f});
                    ui_entity_inspector_draw_sketch_geometry_manager(state, sketch_entity, &active_sketch, scene, true);
                    igDummy((ImVec2){0.0f, 10.0f});
                    ui_entity_inspector_draw_sketch_constraint_manager(state, sketch_entity, &active_sketch, scene, true);
                } else {
                    igTextDisabled("Active entity is not a sketch.");
                }
            }
        }
        igEnd();
        if (!open) {
            state->active_sketch_workspace_open = false;
            state->active_sketch = 0;
        }
    }

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
