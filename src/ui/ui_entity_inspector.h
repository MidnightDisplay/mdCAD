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
#include "../components/geometry_comp.h"
#include "../components/transform_comp.h"
#include "../components/renderable_comp.h"
#include "../components/selectable_comp.h"
#include "../undo_redo_exec.h"
#include <float.h>

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

    // Label section (optional component - only shown if entity has one)
    LabelComp *label = ecs_world_get_label(w, e);
    if (label && igCollapsingHeader_TreeNodeFlags("Label", ImGuiTreeNodeFlags_DefaultOpen)) {
        igInputText("Name", label->name, LABEL_NAME_MAX, 0, NULL, NULL);
        igInputTextMultiline("Description", label->description, LABEL_DESC_MAX,
                             (ImVec2){-FLT_MIN, igGetTextLineHeight() * 3}, 0, NULL, NULL);
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
            t->rotation.x * 57.29578f,  // rad to deg
            t->rotation.y * 57.29578f,
            t->rotation.z * 57.29578f
        };
        if (igDragFloat3("Rotation", rot_deg, 1.0f, -360.0f, 360.0f, "%.1f", 0)) {
            t->rotation.x = rot_deg[0] * 0.01745329f;  // deg to rad
            t->rotation.y = rot_deg[1] * 0.01745329f;
            t->rotation.z = rot_deg[2] * 0.01745329f;
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
                    g->data.arc.start_angle * 57.29578f,
                    g->data.arc.end_angle * 57.29578f);
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
