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

//------------------------------------------------------------------------------
// Types
//------------------------------------------------------------------------------

typedef struct {
    selection_buffer_t *selection;
    ecs_world_state_t *world;
} ui_entity_inspector_state_t;

//------------------------------------------------------------------------------
// Initialization
//------------------------------------------------------------------------------

static inline void ui_entity_inspector_init(ui_entity_inspector_state_t *state,
                                             selection_buffer_t *selection,
                                             ecs_world_state_t *world) {
    state->selection = selection;
    state->world = world;
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

        // Scale
        float scale[3] = { t->scale.x, t->scale.y, t->scale.z };
        if (igDragFloat3("Scale", scale, 0.01f, 0.01f, 10.0f, "%.2f", 0)) {
            t->scale.x = scale[0];
            t->scale.y = scale[1];
            t->scale.z = scale[2];
            changed = true;
        }

        if (changed) {
            t->dirty = true;
            RenderableComp *r = ecs_world_get_renderable(w, e);
            if (r) r->instance_dirty = true;
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

        // Type-specific properties
        switch (g->type) {
            case GEOM_POINT: {
                if (igDragFloat("Point Size", &g->point_size, 0.005f, 0.01f, 0.5f, "%.3f", 0)) {
                    changed = true;
                }
                // Point position
                float pt[3] = { g->data.point.point.x, g->data.point.point.y, g->data.point.point.z };
                if (igDragFloat3("Point", pt, 0.1f, -100.0f, 100.0f, "%.2f", 0)) {
                    g->data.point.point.x = pt[0];
                    g->data.point.point.y = pt[1];
                    g->data.point.point.z = pt[2];
                    changed = true;
                }
                break;
            }
            case GEOM_LINE: {
                if (igDragFloat("Line Width", &g->line_width, 0.005f, 0.005f, 0.2f, "%.3f", 0)) {
                    changed = true;
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
                if (igDragFloat3("Point B", b, 0.1f, -100.0f, 100.0f, "%.2f", 0)) {
                    g->data.line.b.x = b[0];
                    g->data.line.b.y = b[1];
                    g->data.line.b.z = b[2];
                    changed = true;
                }
                break;
            }
            case GEOM_ARC: {
                if (igDragFloat("Line Width", &g->line_width, 0.005f, 0.005f, 0.2f, "%.3f", 0)) {
                    changed = true;
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
                igTextDisabled("Segments: %d", g->data.bezier.segments);
                break;
            }
            case GEOM_HELIX: {
                if (igDragFloat("Line Width", &g->line_width, 0.005f, 0.005f, 0.2f, "%.3f", 0)) {
                    changed = true;
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
                int count = (g->type == GEOM_POLYLINE) ? g->data.polyline.count : g->data.polygon.count;
                igTextDisabled("Point count: %d", count);
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
    if (r && igCollapsingHeader_TreeNodeFlags("Rendering", 0)) {
        if (igCheckbox("Visible", &r->visible)) {
            r->instance_dirty = true;
        }
        igInputInt("Layer", &r->layer, 1, 10, 0);
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

        // Visibility toggle for all
        if (igButton("Show All", (ImVec2){0, 0})) {
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
