//------------------------------------------------------------------------------
// constraint_glyphs.h - Constraint glyph pick overlay helpers (header-only)
//------------------------------------------------------------------------------
#ifndef CONSTRAINT_GLYPHS_H
#define CONSTRAINT_GLYPHS_H

#include "../ecs/ecs_scene.h"
#include "../gpu/pick_buffer.h"
#include "../math/math_interaction.h"
#include "../components/component_types.h"
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"
#include <math.h>
#include <string.h>
#include <stdio.h>

#define CONSTRAINT_GLYPHS_MAX 1024

static inline float constraint_glyphs_angle_rad_to_deg(float radians) {
    return radians * (180.0f / 3.14159265359f);
}

static inline float constraint_glyphs_user_value_from_scene(const ConstraintComp *constraint, float scene_value) {
    if (!constraint) return scene_value;
    if (constraint->type == CONSTRAINT_ANGLE ||
        constraint->type == CONSTRAINT_ARC_ENDPOINT_ANGLE) {
        return constraint_glyphs_angle_rad_to_deg(scene_value);
    }
    return scene_value;
}

typedef struct {
    uint32_t pick_id;
    ecs_entity_t constraint_entity;
    vec3_t anchor_world;
    vec3_t pick_world;
    float anchor_screen_x;
    float anchor_screen_y;
    bool has_screen_anchor;
} constraint_glyph_entry_t;

typedef struct {
    constraint_glyph_entry_t entries[CONSTRAINT_GLYPHS_MAX];
    int count;
    ecs_entity_t hovered_constraint;
} constraint_glyph_state_t;

static inline int constraint_glyphs_compute_stack_index(constraint_glyph_state_t *state,
                                                        ecs_scene_t *scene,
                                                        int entry_index);

static inline void constraint_glyphs_init(constraint_glyph_state_t *state) {
    if (!state) return;
    memset(state, 0, sizeof(*state));
}

static inline void constraint_glyphs_begin_frame(constraint_glyph_state_t *state) {
    if (!state) return;
    state->count = 0;
    state->hovered_constraint = 0;
}

static inline bool constraint_glyphs_is_pick_id(uint32_t pick_id) {
    return pick_id >= CONSTRAINT_GLYPH_PICK_BASE && pick_id <= CONSTRAINT_GLYPH_PICK_END;
}

static inline const constraint_glyph_entry_t* constraint_glyphs_find_by_pick_id(
    const constraint_glyph_state_t *state, uint32_t pick_id) {
    if (!state || !constraint_glyphs_is_pick_id(pick_id)) return NULL;
    for (int i = 0; i < state->count; i++) {
        if (state->entries[i].pick_id == pick_id) {
            return &state->entries[i];
        }
    }
    return NULL;
}

static inline const constraint_glyph_entry_t* constraint_glyphs_find_by_constraint(
    const constraint_glyph_state_t *state, ecs_entity_t constraint_entity) {
    if (!state || constraint_entity == 0) return NULL;
    for (int i = 0; i < state->count; i++) {
        if (state->entries[i].constraint_entity == constraint_entity) {
            return &state->entries[i];
        }
    }
    return NULL;
}

static inline ecs_entity_t constraint_glyphs_constraint_from_pick_id(
    const constraint_glyph_state_t *state, uint32_t pick_id) {
    const constraint_glyph_entry_t *entry = constraint_glyphs_find_by_pick_id(state, pick_id);
    return entry ? entry->constraint_entity : 0;
}

static inline bool constraint_glyphs_get_screen_anchor(
    const constraint_glyph_state_t *state, ecs_entity_t constraint_entity, float *x, float *y) {
    const constraint_glyph_entry_t *entry = constraint_glyphs_find_by_constraint(state, constraint_entity);
    if (!entry || !entry->has_screen_anchor || !x || !y) return false;
    *x = entry->anchor_screen_x;
    *y = entry->anchor_screen_y;
    return true;
}

static inline bool constraint_glyphs_geometry_anchor_world(ecs_scene_t *scene,
                                                           ecs_entity_t geometry_entity,
                                                           vec3_t *out_anchor) {
    if (!scene || !out_anchor || geometry_entity == 0) return false;

    GeometryComp *g = ecs_world_get_geometry(scene->world, geometry_entity);
    TransformComp *t = ecs_world_get_transform(scene->world, geometry_entity);
    if (!g || !t) return false;

    vec3_t local_anchor = vec3_make(0.0f, 0.0f, 0.0f);
    switch (g->type) {
        case GEOM_POINT:
            local_anchor = g->data.point.point;
            break;
        case GEOM_LINE:
            local_anchor = vec3_scale(vec3_add(g->data.line.a, g->data.line.b), 0.5f);
            break;
        case GEOM_ARC:
            local_anchor = g->data.arc.center;
            break;
        default:
            return false;
    }

    *out_anchor = ecs_scene_transform_point_world(&t->world_matrix, local_anchor);
    return true;
}

static inline bool constraint_glyphs_constraint_anchor_world(ecs_scene_t *scene,
                                                             ecs_entity_t constraint_entity,
                                                             const ConstraintComp *constraint,
                                                             vec3_t *out_anchor) {
    if (!scene || !constraint || !out_anchor || constraint_entity == 0) return false;
    if (constraint->participant_count == 0) return false;

    vec3_t sum = vec3_make(0.0f, 0.0f, 0.0f);
    int valid_count = 0;
    for (uint32_t i = 0; i < constraint->participant_count; i++) {
        ecs_entity_t participant = (ecs_entity_t)constraint->participants[i];
        vec3_t participant_anchor;
        if (constraint_glyphs_geometry_anchor_world(scene, participant, &participant_anchor)) {
            sum = vec3_add(sum, participant_anchor);
            valid_count++;
        }
    }

    if (valid_count == 0) return false;
    *out_anchor = vec3_scale(sum, 1.0f / (float)valid_count);
    return true;
}

static inline void constraint_glyphs_populate_pick_buffer(constraint_glyph_state_t *state,
                                                          ecs_scene_t *scene,
                                                          pick_buffer_t *pick_buffer,
                                                          vec3_t camera_position,
                                                          float camera_fov_radians,
                                                          mat4_t view,
                                                          mat4_t proj,
                                                          float viewport_x,
                                                          float viewport_y,
                                                          float viewport_width,
                                                          float viewport_height) {
    if (!state || !scene || !pick_buffer) return;

    constraint_glyphs_begin_frame(state);
    ecs_world_state_t *world = scene->world;

    ecs_query_t *query = ecs_query(world->world, {
        .terms = {
            { .id = world->ConstraintComp_id }
        }
    });

    ecs_iter_t it = ecs_query_iter(world->world, query);
    while (ecs_query_next(&it)) {
        ConstraintComp *constraints = ecs_field(&it, ConstraintComp, 0);
        for (int i = 0; i < it.count; i++) {
            if (state->count >= CONSTRAINT_GLYPHS_MAX) {
                ecs_iter_fini(&it);
                break;
            }

            ecs_entity_t constraint_entity = it.entities[i];
            ConstraintComp *constraint = &constraints[i];
            vec3_t anchor_world;
            if (!constraint_glyphs_constraint_anchor_world(scene, constraint_entity, constraint, &anchor_world)) {
                continue;
            }

            uint32_t pick_id = CONSTRAINT_GLYPH_PICK_BASE + (uint32_t)state->count;
            if (!constraint_glyphs_is_pick_id(pick_id)) {
                ecs_iter_fini(&it);
                break;
            }

            constraint_glyph_entry_t *entry = &state->entries[state->count++];
            entry->pick_id = pick_id;
            entry->constraint_entity = constraint_entity;
            entry->anchor_world = anchor_world;
            entry->pick_world = anchor_world;
            entry->has_screen_anchor = false;
        }
    }

    for (int i = 0; i < state->count; i++) {
        constraint_glyph_entry_t *entry = &state->entries[i];
        if (!ecs_is_alive(scene->world->world, entry->constraint_entity)) continue;
        ConstraintComp *constraint = ecs_world_get_constraint(scene->world, entry->constraint_entity);
        if (!constraint) continue;

        float ndc_x = 0.0f;
        float ndc_y = 0.0f;
        mat4_t vp = mat4_mul(proj, view);
        if (!clip_space_project(vp, entry->anchor_world, &ndc_x, &ndc_y)) {
            continue;
        }
        float base_screen_x = viewport_x + (ndc_x * 0.5f + 0.5f) * viewport_width;
        float base_screen_y = viewport_y + (-ndc_y * 0.5f + 0.5f) * viewport_height;

        int stack_index = constraint_glyphs_compute_stack_index(state, scene, i);
        const float stack_spacing_px = 18.0f;
        const float singular_offset_px = (constraint->participant_count <= 1) ? -12.0f : 0.0f;
        float target_screen_x = base_screen_x + (float)stack_index * stack_spacing_px;
        float target_screen_y = base_screen_y + singular_offset_px;

        float rel_x = (target_screen_x - viewport_x) / viewport_width;
        float rel_y = (target_screen_y - viewport_y) / viewport_height;
        if (rel_x < 0.0f) rel_x = 0.0f;
        if (rel_x > 1.0f) rel_x = 1.0f;
        if (rel_y < 0.0f) rel_y = 0.0f;
        if (rel_y > 1.0f) rel_y = 1.0f;
        ray_t screen_ray = mdcad_interaction_screen_ray_from_viewport(
            rel_x, rel_y, viewport_width, viewport_height, view, proj);
        vec3_t to_anchor = vec3_sub(entry->anchor_world, camera_position);
        float depth_along_ray = vec3_dot(to_anchor, screen_ray.direction);
        if (depth_along_ray < 0.01f) depth_along_ray = vec3_length(to_anchor);
        if (depth_along_ray < 0.01f) depth_along_ray = 0.01f;
        vec3_t pick_anchor = vec3_add(camera_position, vec3_scale(screen_ray.direction, depth_along_ray));
        entry->pick_world = pick_anchor;

        // Keep glyph hit target approximately constant on screen across zoom.
        float pick_dist = vec3_length(vec3_sub(camera_position, pick_anchor));
        if (pick_dist < 0.001f) pick_dist = 0.001f;
        float glyph_world_size = pick_dist * tanf(camera_fov_radians * 0.5f) * 0.02f;
        if (glyph_world_size < 0.005f) glyph_world_size = 0.005f;
        pick_buffer_add_overlay_point(pick_buffer, pick_anchor, entry->pick_id);
        pick_buffer_add_overlay_line(pick_buffer,
            vec3_add(pick_anchor, vec3_make(-glyph_world_size, 0.0f, 0.0f)),
            vec3_add(pick_anchor, vec3_make( glyph_world_size, 0.0f, 0.0f)),
            entry->pick_id);
        pick_buffer_add_overlay_line(pick_buffer,
            vec3_add(pick_anchor, vec3_make(0.0f, -glyph_world_size, 0.0f)),
            vec3_add(pick_anchor, vec3_make(0.0f,  glyph_world_size, 0.0f)),
            entry->pick_id);
    }
    ecs_query_fini(query);
}

static inline void constraint_glyphs_update_screen_anchors(constraint_glyph_state_t *state,
                                                           ecs_scene_t *scene,
                                                           mat4_t view,
                                                           mat4_t proj,
                                                           float viewport_x,
                                                           float viewport_y,
                                                           float viewport_width,
                                                           float viewport_height) {
    if (!state || !scene || viewport_width <= 0.0f || viewport_height <= 0.0f) return;

    mat4_t vp = mat4_mul(proj, view);
    for (int i = 0; i < state->count; i++) {
        if (!ecs_is_alive(scene->world->world, state->entries[i].constraint_entity)) {
            state->entries[i].has_screen_anchor = false;
            continue;
        }
        float ndc_x = 0.0f;
        float ndc_y = 0.0f;
        if (!clip_space_project(vp, state->entries[i].anchor_world, &ndc_x, &ndc_y)) {
            state->entries[i].has_screen_anchor = false;
            continue;
        }
        state->entries[i].anchor_screen_x = viewport_x + (ndc_x * 0.5f + 0.5f) * viewport_width;
        state->entries[i].anchor_screen_y = viewport_y + (-ndc_y * 0.5f + 0.5f) * viewport_height;
        state->entries[i].has_screen_anchor = true;
    }
}

static inline void constraint_glyphs_handle_hover(constraint_glyph_state_t *state, uint32_t pick_id) {
    if (!state) return;
    state->hovered_constraint = constraint_glyphs_constraint_from_pick_id(state, pick_id);
}

static inline bool constraint_glyphs_constraints_share_participant(ecs_scene_t *scene,
                                                                    ecs_entity_t a_entity,
                                                                    ecs_entity_t b_entity) {
    if (!scene || a_entity == 0 || b_entity == 0 || a_entity == b_entity) return false;
    ConstraintComp *a = ecs_world_get_constraint(scene->world, a_entity);
    ConstraintComp *b = ecs_world_get_constraint(scene->world, b_entity);
    if (!a || !b) return false;
    for (uint32_t i = 0; i < a->participant_count; i++) {
        uint64_t p = a->participants[i];
        if (p == 0) continue;
        for (uint32_t j = 0; j < b->participant_count; j++) {
            if (p == b->participants[j]) {
                return true;
            }
        }
    }
    return false;
}

static inline int constraint_glyphs_type_priority(const ConstraintComp *constraint) {
    if (!constraint) return 0;
    if (constraint->type == CONSTRAINT_LENGTH ||
        constraint->type == CONSTRAINT_ANGLE ||
        constraint->type == CONSTRAINT_ARC_ENDPOINT_ANGLE) {
        return 1;
    }
    return 0;
}

static inline int constraint_glyphs_compute_stack_index(constraint_glyph_state_t *state,
                                                        ecs_scene_t *scene,
                                                        int entry_index) {
    if (!state || !scene || entry_index < 0 || entry_index >= state->count) return 0;
    const constraint_glyph_entry_t *entry = &state->entries[entry_index];
    if (!ecs_is_alive(scene->world->world, entry->constraint_entity)) return 0;
    ConstraintComp *entry_constraint = ecs_world_get_constraint(scene->world, entry->constraint_entity);
    if (!entry_constraint) return 0;

    int index = 0;
    int entry_priority = constraint_glyphs_type_priority(entry_constraint);
    uint64_t entry_id = (uint64_t)entry->constraint_entity;

    for (int j = 0; j < state->count; j++) {
        if (j == entry_index) continue;
        const constraint_glyph_entry_t *other = &state->entries[j];
        if (!ecs_is_alive(scene->world->world, other->constraint_entity)) continue;
        if (!constraint_glyphs_constraints_share_participant(scene,
                                                             entry->constraint_entity,
                                                             other->constraint_entity)) {
            continue;
        }

        ConstraintComp *other_constraint = ecs_world_get_constraint(scene->world, other->constraint_entity);
        if (!other_constraint) continue;
        int other_priority = constraint_glyphs_type_priority(other_constraint);
        uint64_t other_id = (uint64_t)other->constraint_entity;
        if (other_priority < entry_priority ||
            (other_priority == entry_priority && other_id < entry_id)) {
            index++;
        }
    }
    return index;
}

static inline void constraint_glyphs_draw_overlay(constraint_glyph_state_t *state,
                                                  ecs_scene_t *scene,
                                                  float viewport_x,
                                                  float viewport_y,
                                                  float viewport_width,
                                                  float viewport_height) {
    if (!state || !scene || viewport_width <= 0.0f || viewport_height <= 0.0f) return;
    ImGuiViewport *main_viewport = igGetMainViewport();
    if (!main_viewport) return;
    ImDrawList *draw_list = igGetForegroundDrawList_ViewportPtr(main_viewport);
    if (!draw_list) return;

    ImVec2_c clip_min = { viewport_x, viewport_y };
    ImVec2_c clip_max = { viewport_x + viewport_width, viewport_y + viewport_height };
    ImDrawList_PushClipRect(draw_list, clip_min, clip_max, true);

    const ImU32 glyph_color = igGetColorU32_Vec4((ImVec4_c){0.95f, 0.95f, 0.95f, 0.90f});
    const ImU32 hover_color = igGetColorU32_Vec4((ImVec4_c){1.00f, 0.85f, 0.15f, 1.00f});
    const ImU32 dim_bg_color = igGetColorU32_Vec4((ImVec4_c){0.08f, 0.08f, 0.10f, 0.90f});
    const ImU32 dim_text_color = igGetColorU32_Vec4((ImVec4_c){0.95f, 0.95f, 0.95f, 1.00f});

    for (int i = 0; i < state->count; i++) {
        const constraint_glyph_entry_t *entry = &state->entries[i];
        if (!entry->has_screen_anchor) continue;
        if (!ecs_is_alive(scene->world->world, entry->constraint_entity)) continue;

        int stack_index = constraint_glyphs_compute_stack_index(state, scene, i);
        const float stack_spacing_px = 18.0f;
        float x = entry->anchor_screen_x + (float)stack_index * stack_spacing_px;
        float y = entry->anchor_screen_y;
        if (x < viewport_x || x > (viewport_x + viewport_width) ||
            y < viewport_y || y > (viewport_y + viewport_height)) {
            continue;
        }

        ConstraintComp *constraint = ecs_world_get_constraint(scene->world, entry->constraint_entity);
        if (!constraint) continue;

        const bool hovered = (state->hovered_constraint == entry->constraint_entity);
        const ImU32 stroke = hovered ? hover_color : glyph_color;
        const float radius = hovered ? 7.5f : 6.0f;
        const float thickness = hovered ? 2.5f : 2.0f;
        const bool singular = (constraint->participant_count <= 1);
        const float y_offset = singular ? -(radius * 2.0f) : 0.0f;
        const float cx = x;
        const float cy = y + y_offset;

        ImDrawList_AddCircle(draw_list, (ImVec2_c){cx, cy}, radius, stroke, 20, thickness);
        ImDrawList_AddLine(draw_list, (ImVec2_c){cx - 4.0f, cy}, (ImVec2_c){cx + 4.0f, cy}, stroke, thickness);
        ImDrawList_AddLine(draw_list, (ImVec2_c){cx, cy - 4.0f}, (ImVec2_c){cx, cy + 4.0f}, stroke, thickness);

        if (constraint_comp_is_dimensional(constraint)) {
            char value_text[64];
            uint8_t decimals = constraint->display_decimals;
            float display_value = constraint_glyphs_user_value_from_scene(constraint, constraint->value);
            constraint_format_value(value_text, sizeof(value_text), display_value, decimals);

            ImVec2_c text_size = igCalcTextSize(value_text, NULL, false, -1.0f);
            float bg_w = text_size.x + 16.0f;
            if (bg_w < 42.0f) bg_w = 42.0f;
            ImVec2_c bg_min = { cx + 10.0f, cy - 9.0f };
            ImVec2_c bg_max = { bg_min.x + bg_w, cy + 9.0f };
            ImDrawList_AddRectFilled(draw_list, bg_min, bg_max, dim_bg_color, 4.0f, 0);
            ImDrawList_AddText_Vec2(draw_list, (ImVec2_c){bg_min.x + 8.0f, bg_min.y + 2.5f}, dim_text_color, value_text, NULL);
        }
    }

    ImDrawList_PopClipRect(draw_list);
}

#endif // CONSTRAINT_GLYPHS_H
