#ifndef UNDO_REDO_EXEC_H
#define UNDO_REDO_EXEC_H

/*
 * undo_redo_exec.h - Execution logic for undo/redo commands
 *
 * This file implements the actual undo/redo operations by interacting
 * with the ECS scene. Include this file in app.c or wherever the
 * undo/redo system is managed.
 *
 * Usage:
 *   // After setting up undo_redo_t:
 *   undo_redo_undo(&undo_redo);  // Ctrl+Z
 *   undo_redo_redo(&undo_redo);  // Ctrl+Shift+Z
 */

#include "undo_redo.h"
#include "ecs/ecs_scene.h"
#include "selection.h"

// ============================================================================
// Entity Snapshot Creation
// ============================================================================

// Create a snapshot of an entity's complete state (for undo/redo)
static inline undo_entity_snapshot_t undo_snapshot_entity(ecs_scene_t *scene, ecs_entity_t e) {
    undo_entity_snapshot_t snap = {0};

    ecs_world_state_t *w = scene->world;

    // Get transform
    TransformComp *t = ecs_world_get_transform(w, e);
    if (t) {
        snap.position = t->position;
        snap.rotation = t->rotation;
        snap.scale = t->scale;
    }

    // Get geometry
    GeometryComp *g = ecs_world_get_geometry(w, e);
    if (g) {
        snap.geom_type = (undo_geom_type_t)g->type;
        snap.color = g->color;
        snap.line_width = g->line_width;
        snap.point_size = g->point_size;

        switch (g->type) {
            case GEOM_POINT:
                snap.data.point.x = g->data.point.point.x;
                snap.data.point.y = g->data.point.point.y;
                snap.data.point.z = g->data.point.point.z;
                break;
            case GEOM_LINE:
                snap.data.line.a = g->data.line.a;
                snap.data.line.b = g->data.line.b;
                break;
            case GEOM_POLYLINE:
                snap.data.polyline.count = g->data.polyline.count;
                snap.data.polyline.points = (vec3_t*)malloc(sizeof(vec3_t) * g->data.polyline.count);
                if (snap.data.polyline.points) {
                    memcpy(snap.data.polyline.points, g->data.polyline.points,
                           sizeof(vec3_t) * g->data.polyline.count);
                }
                break;
            case GEOM_ARC:
                snap.data.arc.center = g->data.arc.center;
                snap.data.arc.radius = g->data.arc.radius;
                snap.data.arc.start_angle = g->data.arc.start_angle;
                snap.data.arc.end_angle = g->data.arc.end_angle;
                snap.data.arc.normal = g->data.arc.normal;
                break;
            case GEOM_POLYGON:
                snap.data.polygon.count = g->data.polygon.count;
                snap.data.polygon.points = (vec3_t*)malloc(sizeof(vec3_t) * g->data.polygon.count);
                if (snap.data.polygon.points) {
                    memcpy(snap.data.polygon.points, g->data.polygon.points,
                           sizeof(vec3_t) * g->data.polygon.count);
                }
                break;
            case GEOM_BEZIER:
                snap.data.bezier.p0 = g->data.bezier.p0;
                snap.data.bezier.p1 = g->data.bezier.p1;
                snap.data.bezier.p2 = g->data.bezier.p2;
                snap.data.bezier.p3 = g->data.bezier.p3;
                snap.data.bezier.segments = g->data.bezier.segments;
                break;
            case GEOM_HELIX:
                snap.data.helix.axis_start = g->data.helix.axis_start;
                snap.data.helix.axis_end = g->data.helix.axis_end;
                snap.data.helix.radius = g->data.helix.radius;
                snap.data.helix.turns = g->data.helix.turns;
                snap.data.helix.segments = g->data.helix.segments;
                break;
            case GEOM_TRIANGLE:
                snap.data.triangle.a = g->data.triangle.a;
                snap.data.triangle.b = g->data.triangle.b;
                snap.data.triangle.c = g->data.triangle.c;
                snap.data.triangle.has_vertex_colors = g->data.triangle.has_vertex_colors;
                if (g->data.triangle.has_vertex_colors) {
                    snap.data.triangle.color_a = g->data.triangle.color_a;
                    snap.data.triangle.color_b = g->data.triangle.color_b;
                    snap.data.triangle.color_c = g->data.triangle.color_c;
                }
                break;
            default:
                break;
        }
    }

    // Get renderable
    RenderableComp *r = ecs_world_get_renderable(w, e);
    if (r) {
        snap.visible = r->visible;
        snap.layer = r->layer;
    }

    // Get parent
    snap.parent_id = (uint64_t)scene_get_parent(scene, e);

    return snap;
}

// Create an entity from a snapshot (returns the new entity ID)
static inline ecs_entity_t undo_create_from_snapshot(ecs_scene_t *scene, undo_entity_snapshot_t *snap) {
    ecs_entity_t e = 0;

    // Create entity based on geometry type
    switch (snap->geom_type) {
        case UNDO_GEOM_POINT:
            e = scene_add_point(scene,
                vec3_make(snap->data.point.x, snap->data.point.y, snap->data.point.z),
                snap->color, snap->point_size);
            break;
        case UNDO_GEOM_LINE:
            e = scene_add_line(scene, snap->data.line.a, snap->data.line.b,
                               snap->color, snap->line_width);
            break;
        case UNDO_GEOM_POLYLINE:
            if (snap->data.polyline.points && snap->data.polyline.count >= 2) {
                e = scene_add_polyline(scene, snap->data.polyline.points,
                                       snap->data.polyline.count,
                                       snap->color, snap->line_width);
            }
            break;
        case UNDO_GEOM_ARC:
            e = scene_add_arc(scene, snap->data.arc.center, snap->data.arc.radius,
                              snap->data.arc.start_angle, snap->data.arc.end_angle,
                              snap->data.arc.normal, snap->color, snap->line_width);
            break;
        case UNDO_GEOM_POLYGON:
            if (snap->data.polygon.points && snap->data.polygon.count >= 3) {
                e = scene_add_polygon(scene, snap->data.polygon.points,
                                      snap->data.polygon.count,
                                      snap->color, snap->line_width);
            }
            break;
        case UNDO_GEOM_BEZIER:
            e = scene_add_bezier(scene, snap->data.bezier.p0, snap->data.bezier.p1,
                                 snap->data.bezier.p2, snap->data.bezier.p3,
                                 snap->data.bezier.segments, snap->color, snap->line_width);
            break;
        case UNDO_GEOM_HELIX:
            e = scene_add_helix(scene, snap->data.helix.axis_start, snap->data.helix.axis_end,
                                snap->data.helix.radius, snap->data.helix.turns,
                                snap->data.helix.segments, snap->color, snap->line_width);
            break;
        case UNDO_GEOM_TRIANGLE:
            if (snap->data.triangle.has_vertex_colors) {
                e = scene_add_triangle_colored(scene,
                    snap->data.triangle.a, snap->data.triangle.b, snap->data.triangle.c,
                    snap->data.triangle.color_a, snap->data.triangle.color_b,
                    snap->data.triangle.color_c);
            } else {
                e = scene_add_triangle(scene, snap->data.triangle.a, snap->data.triangle.b,
                                       snap->data.triangle.c, snap->color);
            }
            break;
        default:
            break;
    }

    if (e != 0) {
        // Apply transform
        TransformComp *t = ecs_world_get_transform(scene->world, e);
        if (t) {
            t->position = snap->position;
            t->rotation = snap->rotation;
            t->scale = snap->scale;
            t->dirty = true;
        }

        // Apply renderable properties
        RenderableComp *r = ecs_world_get_renderable(scene->world, e);
        if (r) {
            r->visible = snap->visible;
            r->layer = snap->layer;
            r->instance_dirty = true;
        }
    }

    return e;
}

// ============================================================================
// Command Recording Functions
// ============================================================================

// Record entity creation (call AFTER creating the entity)
static inline void undo_cmd_create_entity(undo_redo_t *ur, ecs_entity_t e) {
    if (!ur || !ur->scene) return;
    ecs_scene_t *scene = (ecs_scene_t*)ur->scene;

    undo_command_t cmd = {0};
    cmd.type = CMD_CREATE_ENTITY;
    cmd.data.create.entity_id = (uint64_t)e;
    cmd.data.create.snapshot = undo_snapshot_entity(scene, e);

    undo_redo_push(ur, &cmd);
}

// Record entity deletion (call BEFORE deleting the entity)
static inline void undo_cmd_delete_entity(undo_redo_t *ur, ecs_entity_t e) {
    if (!ur || !ur->scene) return;
    ecs_scene_t *scene = (ecs_scene_t*)ur->scene;

    undo_command_t cmd = {0};
    cmd.type = CMD_DELETE_ENTITY;
    cmd.data.delete_.entity_id = (uint64_t)e;
    cmd.data.delete_.snapshot = undo_snapshot_entity(scene, e);

    // Collect children - count first, then allocate dynamically
    int child_count = scene_count_children(scene, e);

    if (child_count > 0) {
        // Allocate array for children
        ecs_entity_t *children = (ecs_entity_t*)malloc(sizeof(ecs_entity_t) * child_count);
        int actual_count = scene_get_children(scene, e, children, child_count);

        cmd.data.delete_.child_ids = (uint64_t*)malloc(sizeof(uint64_t) * actual_count);
        cmd.data.delete_.child_snapshots = (undo_entity_snapshot_t*)malloc(sizeof(undo_entity_snapshot_t) * actual_count);
        cmd.data.delete_.child_count = actual_count;

        for (int i = 0; i < actual_count; i++) {
            cmd.data.delete_.child_ids[i] = (uint64_t)children[i];
            cmd.data.delete_.child_snapshots[i] = undo_snapshot_entity(scene, children[i]);
        }

        free(children);
    }

    undo_redo_push(ur, &cmd);
}

// Record position change
static inline void undo_cmd_set_position(undo_redo_t *ur, ecs_entity_t e, vec3_t old_pos, vec3_t new_pos) {
    if (!ur) return;

    undo_command_t cmd = {0};
    cmd.type = CMD_SET_POSITION;
    cmd.data.set_vec3.entity_id = (uint64_t)e;
    cmd.data.set_vec3.old_value = old_pos;
    cmd.data.set_vec3.new_value = new_pos;

    undo_redo_push(ur, &cmd);
}

// Record rotation change
static inline void undo_cmd_set_rotation(undo_redo_t *ur, ecs_entity_t e, vec3_t old_rot, vec3_t new_rot) {
    if (!ur) return;

    undo_command_t cmd = {0};
    cmd.type = CMD_SET_ROTATION;
    cmd.data.set_vec3.entity_id = (uint64_t)e;
    cmd.data.set_vec3.old_value = old_rot;
    cmd.data.set_vec3.new_value = new_rot;

    undo_redo_push(ur, &cmd);
}

// Record scale change
static inline void undo_cmd_set_scale(undo_redo_t *ur, ecs_entity_t e, vec3_t old_scale, vec3_t new_scale) {
    if (!ur) return;

    undo_command_t cmd = {0};
    cmd.type = CMD_SET_SCALE;
    cmd.data.set_vec3.entity_id = (uint64_t)e;
    cmd.data.set_vec3.old_value = old_scale;
    cmd.data.set_vec3.new_value = new_scale;

    undo_redo_push(ur, &cmd);
}

// Record color change
static inline void undo_cmd_set_color(undo_redo_t *ur, ecs_entity_t e, vec4_t old_color, vec4_t new_color) {
    if (!ur) return;

    undo_command_t cmd = {0};
    cmd.type = CMD_SET_COLOR;
    cmd.data.set_color.entity_id = (uint64_t)e;
    cmd.data.set_color.old_value = old_color;
    cmd.data.set_color.new_value = new_color;

    undo_redo_push(ur, &cmd);
}

// Record line width change
static inline void undo_cmd_set_line_width(undo_redo_t *ur, ecs_entity_t e, float old_width, float new_width) {
    if (!ur) return;

    undo_command_t cmd = {0};
    cmd.type = CMD_SET_LINE_WIDTH;
    cmd.data.set_float.entity_id = (uint64_t)e;
    cmd.data.set_float.old_value = old_width;
    cmd.data.set_float.new_value = new_width;

    undo_redo_push(ur, &cmd);
}

// Record point size change
static inline void undo_cmd_set_point_size(undo_redo_t *ur, ecs_entity_t e, float old_size, float new_size) {
    if (!ur) return;

    undo_command_t cmd = {0};
    cmd.type = CMD_SET_POINT_SIZE;
    cmd.data.set_float.entity_id = (uint64_t)e;
    cmd.data.set_float.old_value = old_size;
    cmd.data.set_float.new_value = new_size;

    undo_redo_push(ur, &cmd);
}

// Record line endpoints change
static inline void undo_cmd_set_line_endpoints(undo_redo_t *ur, ecs_entity_t e,
                                                vec3_t old_a, vec3_t old_b,
                                                vec3_t new_a, vec3_t new_b) {
    if (!ur) return;

    undo_command_t cmd = {0};
    cmd.type = CMD_SET_LINE_ENDPOINTS;
    cmd.data.set_endpoints.entity_id = (uint64_t)e;
    cmd.data.set_endpoints.old_a = old_a;
    cmd.data.set_endpoints.old_b = old_b;
    cmd.data.set_endpoints.new_a = new_a;
    cmd.data.set_endpoints.new_b = new_b;

    undo_redo_push(ur, &cmd);
}

// Record point position change (geometry point, not transform)
static inline void undo_cmd_set_point_position(undo_redo_t *ur, ecs_entity_t e, vec3_t old_pos, vec3_t new_pos) {
    if (!ur) return;

    undo_command_t cmd = {0};
    cmd.type = CMD_SET_POINT_POSITION;
    cmd.data.set_vec3.entity_id = (uint64_t)e;
    cmd.data.set_vec3.old_value = old_pos;
    cmd.data.set_vec3.new_value = new_pos;

    undo_redo_push(ur, &cmd);
}

// Record visibility change
static inline void undo_cmd_set_visible(undo_redo_t *ur, ecs_entity_t e, bool old_visible, bool new_visible) {
    if (!ur) return;

    undo_command_t cmd = {0};
    cmd.type = CMD_SET_VISIBLE;
    cmd.data.set_visible.entity_id = (uint64_t)e;
    cmd.data.set_visible.old_value = old_visible;
    cmd.data.set_visible.new_value = new_visible;

    undo_redo_push(ur, &cmd);
}

// Record parent change
static inline void undo_cmd_set_parent(undo_redo_t *ur, ecs_entity_t e, ecs_entity_t old_parent, ecs_entity_t new_parent) {
    if (!ur) return;

    undo_command_t cmd = {0};
    cmd.type = CMD_SET_PARENT;
    cmd.data.set_parent.entity_id = (uint64_t)e;
    cmd.data.set_parent.old_parent = (uint64_t)old_parent;
    cmd.data.set_parent.new_parent = (uint64_t)new_parent;

    undo_redo_push(ur, &cmd);
}

// Record bulk color change
static inline void undo_cmd_bulk_set_color(undo_redo_t *ur, ecs_entity_t *entities, vec4_t *old_colors, int count, vec4_t new_color) {
    if (!ur || count <= 0) return;

    undo_command_t cmd = {0};
    cmd.type = CMD_BULK_SET_COLOR;
    cmd.data.bulk_color.entity_ids = (uint64_t*)malloc(sizeof(uint64_t) * count);
    cmd.data.bulk_color.old_colors = (vec4_t*)malloc(sizeof(vec4_t) * count);
    cmd.data.bulk_color.count = count;
    cmd.data.bulk_color.new_color = new_color;

    for (int i = 0; i < count; i++) {
        cmd.data.bulk_color.entity_ids[i] = (uint64_t)entities[i];
        cmd.data.bulk_color.old_colors[i] = old_colors[i];
    }

    undo_redo_push(ur, &cmd);
}

// Record bulk visibility change
static inline void undo_cmd_bulk_set_visible(undo_redo_t *ur, ecs_entity_t *entities, bool *old_visible, int count, bool new_visible) {
    if (!ur || count <= 0) return;

    undo_command_t cmd = {0};
    cmd.type = CMD_BULK_SET_VISIBLE;
    cmd.data.bulk_visible.entity_ids = (uint64_t*)malloc(sizeof(uint64_t) * count);
    cmd.data.bulk_visible.old_visible = (bool*)malloc(sizeof(bool) * count);
    cmd.data.bulk_visible.count = count;
    cmd.data.bulk_visible.new_visible = new_visible;

    for (int i = 0; i < count; i++) {
        cmd.data.bulk_visible.entity_ids[i] = (uint64_t)entities[i];
        cmd.data.bulk_visible.old_visible[i] = old_visible[i];
    }

    undo_redo_push(ur, &cmd);
}

// Record geometry vertex position changes
static inline void undo_cmd_set_geometry_vertices(undo_redo_t *ur, ecs_entity_t e,
                                                    int *vertex_indices, vec3_t *old_positions,
                                                    vec3_t *new_positions, int count) {
    if (!ur || count <= 0) return;

    undo_command_t cmd = {0};
    cmd.type = CMD_SET_GEOMETRY_VERTICES;
    cmd.data.set_vertices.entity_id = (uint64_t)e;
    cmd.data.set_vertices.count = count;
    cmd.data.set_vertices.vertex_indices = (int*)malloc(sizeof(int) * count);
    cmd.data.set_vertices.old_positions = (vec3_t*)malloc(sizeof(vec3_t) * count);
    cmd.data.set_vertices.new_positions = (vec3_t*)malloc(sizeof(vec3_t) * count);

    memcpy(cmd.data.set_vertices.vertex_indices, vertex_indices, sizeof(int) * count);
    memcpy(cmd.data.set_vertices.old_positions, old_positions, sizeof(vec3_t) * count);
    memcpy(cmd.data.set_vertices.new_positions, new_positions, sizeof(vec3_t) * count);

    undo_redo_push(ur, &cmd);
}

// Helper: set vertex position in GeometryComp by index
static inline void undo_set_vertex_pos(GeometryComp *g, int idx, vec3_t pos) {
    switch (g->type) {
        case GEOM_POINT:    g->data.point.point = pos; break;
        case GEOM_LINE:     if (idx == 0) g->data.line.a = pos; else g->data.line.b = pos; break;
        case GEOM_POLYLINE: if (idx < g->data.polyline.count) g->data.polyline.points[idx] = pos; break;
        case GEOM_POLYGON:  if (idx < g->data.polygon.count) g->data.polygon.points[idx] = pos; break;
        case GEOM_TRIANGLE:
            if (idx == 0) g->data.triangle.a = pos;
            else if (idx == 1) g->data.triangle.b = pos;
            else g->data.triangle.c = pos;
            break;
        default: break;
    }
}

// ============================================================================
// Apply Command (for redo)
// ============================================================================

static inline void undo_apply_command(undo_redo_t *ur, undo_command_t *cmd) {
    if (!ur || !ur->scene) return;
    ecs_scene_t *scene = (ecs_scene_t*)ur->scene;
    ecs_world_state_t *w = scene->world;

    switch (cmd->type) {
        case CMD_CREATE_ENTITY: {
            // Redo create: recreate the entity
            ecs_entity_t e = undo_create_from_snapshot(scene, &cmd->data.create.snapshot);
            // Update the stored entity ID for future undo/redo
            cmd->data.create.entity_id = (uint64_t)e;
            // Restore parent if needed
            if (cmd->data.create.snapshot.parent_id != 0) {
                scene_set_parent(scene, e, (ecs_entity_t)cmd->data.create.snapshot.parent_id);
            }
            break;
        }

        case CMD_DELETE_ENTITY: {
            // Redo delete: remove the entity
            ecs_entity_t e = (ecs_entity_t)cmd->data.delete_.entity_id;
            if (ecs_is_alive(w->world, e)) {
                // Remove from selection if selected
                if (ur->selection) {
                    selection_buffer_t *sel = (selection_buffer_t*)ur->selection;
                    selection_remove(sel, e);
                }
                scene_remove_entity(scene, e);
            }
            break;
        }

        case CMD_SET_POSITION: {
            ecs_entity_t e = (ecs_entity_t)cmd->data.set_vec3.entity_id;
            TransformComp *t = ecs_world_get_transform(w, e);
            if (t) {
                t->position = cmd->data.set_vec3.new_value;
                t->dirty = true;
                ecs_world_mark_descendants_dirty(w, e);
            }
            RenderableComp *r = ecs_world_get_renderable(w, e);
            if (r) r->instance_dirty = true;
            break;
        }

        case CMD_SET_ROTATION: {
            ecs_entity_t e = (ecs_entity_t)cmd->data.set_vec3.entity_id;
            TransformComp *t = ecs_world_get_transform(w, e);
            if (t) {
                t->rotation = cmd->data.set_vec3.new_value;
                t->dirty = true;
                ecs_world_mark_descendants_dirty(w, e);
            }
            RenderableComp *r = ecs_world_get_renderable(w, e);
            if (r) r->instance_dirty = true;
            break;
        }

        case CMD_SET_SCALE: {
            ecs_entity_t e = (ecs_entity_t)cmd->data.set_vec3.entity_id;
            TransformComp *t = ecs_world_get_transform(w, e);
            if (t) {
                t->scale = cmd->data.set_vec3.new_value;
                t->dirty = true;
                ecs_world_mark_descendants_dirty(w, e);
            }
            RenderableComp *r = ecs_world_get_renderable(w, e);
            if (r) r->instance_dirty = true;
            break;
        }

        case CMD_SET_COLOR: {
            ecs_entity_t e = (ecs_entity_t)cmd->data.set_color.entity_id;
            GeometryComp *g = ecs_world_get_geometry(w, e);
            if (g) g->color = cmd->data.set_color.new_value;
            RenderableComp *r = ecs_world_get_renderable(w, e);
            if (r) r->instance_dirty = true;
            break;
        }

        case CMD_SET_LINE_WIDTH: {
            ecs_entity_t e = (ecs_entity_t)cmd->data.set_float.entity_id;
            GeometryComp *g = ecs_world_get_geometry(w, e);
            if (g) g->line_width = cmd->data.set_float.new_value;
            RenderableComp *r = ecs_world_get_renderable(w, e);
            if (r) r->instance_dirty = true;
            break;
        }

        case CMD_SET_POINT_SIZE: {
            ecs_entity_t e = (ecs_entity_t)cmd->data.set_float.entity_id;
            GeometryComp *g = ecs_world_get_geometry(w, e);
            if (g) g->point_size = cmd->data.set_float.new_value;
            RenderableComp *r = ecs_world_get_renderable(w, e);
            if (r) r->instance_dirty = true;
            break;
        }

        case CMD_SET_LINE_ENDPOINTS: {
            ecs_entity_t e = (ecs_entity_t)cmd->data.set_endpoints.entity_id;
            GeometryComp *g = ecs_world_get_geometry(w, e);
            if (g && g->type == GEOM_LINE) {
                g->data.line.a = cmd->data.set_endpoints.new_a;
                g->data.line.b = cmd->data.set_endpoints.new_b;
            }
            RenderableComp *r = ecs_world_get_renderable(w, e);
            if (r) r->instance_dirty = true;
            break;
        }

        case CMD_SET_POINT_POSITION: {
            ecs_entity_t e = (ecs_entity_t)cmd->data.set_vec3.entity_id;
            GeometryComp *g = ecs_world_get_geometry(w, e);
            if (g && g->type == GEOM_POINT) {
                g->data.point.point = cmd->data.set_vec3.new_value;
            }
            RenderableComp *r = ecs_world_get_renderable(w, e);
            if (r) r->instance_dirty = true;
            break;
        }

        case CMD_SET_VISIBLE: {
            ecs_entity_t e = (ecs_entity_t)cmd->data.set_visible.entity_id;
            RenderableComp *r = ecs_world_get_renderable(w, e);
            if (r) {
                r->visible = cmd->data.set_visible.new_value;
                r->instance_dirty = true;
            }
            break;
        }

        case CMD_SET_PARENT: {
            ecs_entity_t e = (ecs_entity_t)cmd->data.set_parent.entity_id;
            ecs_entity_t new_parent = (ecs_entity_t)cmd->data.set_parent.new_parent;
            scene_set_parent(scene, e, new_parent);
            break;
        }

        case CMD_BULK_SET_COLOR: {
            for (int i = 0; i < cmd->data.bulk_color.count; i++) {
                ecs_entity_t e = (ecs_entity_t)cmd->data.bulk_color.entity_ids[i];
                GeometryComp *g = ecs_world_get_geometry(w, e);
                if (g) g->color = cmd->data.bulk_color.new_color;
                RenderableComp *r = ecs_world_get_renderable(w, e);
                if (r) r->instance_dirty = true;
            }
            break;
        }

        case CMD_BULK_SET_VISIBLE: {
            for (int i = 0; i < cmd->data.bulk_visible.count; i++) {
                ecs_entity_t e = (ecs_entity_t)cmd->data.bulk_visible.entity_ids[i];
                RenderableComp *r = ecs_world_get_renderable(w, e);
                if (r) {
                    r->visible = cmd->data.bulk_visible.new_visible;
                    r->instance_dirty = true;
                }
            }
            break;
        }

        case CMD_SET_GEOMETRY_VERTICES: {
            ecs_entity_t e = (ecs_entity_t)cmd->data.set_vertices.entity_id;
            GeometryComp *g = ecs_world_get_geometry(w, e);
            if (g) {
                for (int i = 0; i < cmd->data.set_vertices.count; i++) {
                    undo_set_vertex_pos(g, cmd->data.set_vertices.vertex_indices[i],
                                        cmd->data.set_vertices.new_positions[i]);
                }
            }
            RenderableComp *r = ecs_world_get_renderable(w, e);
            if (r) r->instance_dirty = true;
            break;
        }

        default:
            break;
    }
}

// ============================================================================
// Unapply Command (for undo - reverse the operation)
// ============================================================================

static inline void undo_unapply_command(undo_redo_t *ur, undo_command_t *cmd) {
    if (!ur || !ur->scene) return;
    ecs_scene_t *scene = (ecs_scene_t*)ur->scene;
    ecs_world_state_t *w = scene->world;

    switch (cmd->type) {
        case CMD_CREATE_ENTITY: {
            // Undo create: delete the entity
            ecs_entity_t e = (ecs_entity_t)cmd->data.create.entity_id;
            if (ecs_is_alive(w->world, e)) {
                // Remove from selection if selected
                if (ur->selection) {
                    selection_buffer_t *sel = (selection_buffer_t*)ur->selection;
                    selection_remove(sel, e);
                }
                scene_remove_entity(scene, e);
            }
            break;
        }

        case CMD_DELETE_ENTITY: {
            // Undo delete: recreate the entity and its children
            ecs_entity_t e = undo_create_from_snapshot(scene, &cmd->data.delete_.snapshot);
            // Update stored entity ID
            cmd->data.delete_.entity_id = (uint64_t)e;

            // Restore parent
            if (cmd->data.delete_.snapshot.parent_id != 0) {
                scene_set_parent(scene, e, (ecs_entity_t)cmd->data.delete_.snapshot.parent_id);
            }

            // Recreate children
            for (int i = 0; i < cmd->data.delete_.child_count; i++) {
                ecs_entity_t child = undo_create_from_snapshot(scene, &cmd->data.delete_.child_snapshots[i]);
                cmd->data.delete_.child_ids[i] = (uint64_t)child;
                scene_set_parent(scene, child, e);
            }
            break;
        }

        case CMD_SET_POSITION: {
            ecs_entity_t e = (ecs_entity_t)cmd->data.set_vec3.entity_id;
            TransformComp *t = ecs_world_get_transform(w, e);
            if (t) {
                t->position = cmd->data.set_vec3.old_value;
                t->dirty = true;
                ecs_world_mark_descendants_dirty(w, e);
            }
            RenderableComp *r = ecs_world_get_renderable(w, e);
            if (r) r->instance_dirty = true;
            break;
        }

        case CMD_SET_ROTATION: {
            ecs_entity_t e = (ecs_entity_t)cmd->data.set_vec3.entity_id;
            TransformComp *t = ecs_world_get_transform(w, e);
            if (t) {
                t->rotation = cmd->data.set_vec3.old_value;
                t->dirty = true;
                ecs_world_mark_descendants_dirty(w, e);
            }
            RenderableComp *r = ecs_world_get_renderable(w, e);
            if (r) r->instance_dirty = true;
            break;
        }

        case CMD_SET_SCALE: {
            ecs_entity_t e = (ecs_entity_t)cmd->data.set_vec3.entity_id;
            TransformComp *t = ecs_world_get_transform(w, e);
            if (t) {
                t->scale = cmd->data.set_vec3.old_value;
                t->dirty = true;
                ecs_world_mark_descendants_dirty(w, e);
            }
            RenderableComp *r = ecs_world_get_renderable(w, e);
            if (r) r->instance_dirty = true;
            break;
        }

        case CMD_SET_COLOR: {
            ecs_entity_t e = (ecs_entity_t)cmd->data.set_color.entity_id;
            GeometryComp *g = ecs_world_get_geometry(w, e);
            if (g) g->color = cmd->data.set_color.old_value;
            RenderableComp *r = ecs_world_get_renderable(w, e);
            if (r) r->instance_dirty = true;
            break;
        }

        case CMD_SET_LINE_WIDTH: {
            ecs_entity_t e = (ecs_entity_t)cmd->data.set_float.entity_id;
            GeometryComp *g = ecs_world_get_geometry(w, e);
            if (g) g->line_width = cmd->data.set_float.old_value;
            RenderableComp *r = ecs_world_get_renderable(w, e);
            if (r) r->instance_dirty = true;
            break;
        }

        case CMD_SET_POINT_SIZE: {
            ecs_entity_t e = (ecs_entity_t)cmd->data.set_float.entity_id;
            GeometryComp *g = ecs_world_get_geometry(w, e);
            if (g) g->point_size = cmd->data.set_float.old_value;
            RenderableComp *r = ecs_world_get_renderable(w, e);
            if (r) r->instance_dirty = true;
            break;
        }

        case CMD_SET_LINE_ENDPOINTS: {
            ecs_entity_t e = (ecs_entity_t)cmd->data.set_endpoints.entity_id;
            GeometryComp *g = ecs_world_get_geometry(w, e);
            if (g && g->type == GEOM_LINE) {
                g->data.line.a = cmd->data.set_endpoints.old_a;
                g->data.line.b = cmd->data.set_endpoints.old_b;
            }
            RenderableComp *r = ecs_world_get_renderable(w, e);
            if (r) r->instance_dirty = true;
            break;
        }

        case CMD_SET_POINT_POSITION: {
            ecs_entity_t e = (ecs_entity_t)cmd->data.set_vec3.entity_id;
            GeometryComp *g = ecs_world_get_geometry(w, e);
            if (g && g->type == GEOM_POINT) {
                g->data.point.point = cmd->data.set_vec3.old_value;
            }
            RenderableComp *r = ecs_world_get_renderable(w, e);
            if (r) r->instance_dirty = true;
            break;
        }

        case CMD_SET_VISIBLE: {
            ecs_entity_t e = (ecs_entity_t)cmd->data.set_visible.entity_id;
            RenderableComp *r = ecs_world_get_renderable(w, e);
            if (r) {
                r->visible = cmd->data.set_visible.old_value;
                r->instance_dirty = true;
            }
            break;
        }

        case CMD_SET_PARENT: {
            ecs_entity_t e = (ecs_entity_t)cmd->data.set_parent.entity_id;
            ecs_entity_t old_parent = (ecs_entity_t)cmd->data.set_parent.old_parent;
            scene_set_parent(scene, e, old_parent);
            break;
        }

        case CMD_BULK_SET_COLOR: {
            for (int i = 0; i < cmd->data.bulk_color.count; i++) {
                ecs_entity_t e = (ecs_entity_t)cmd->data.bulk_color.entity_ids[i];
                GeometryComp *g = ecs_world_get_geometry(w, e);
                if (g) g->color = cmd->data.bulk_color.old_colors[i];
                RenderableComp *r = ecs_world_get_renderable(w, e);
                if (r) r->instance_dirty = true;
            }
            break;
        }

        case CMD_BULK_SET_VISIBLE: {
            for (int i = 0; i < cmd->data.bulk_visible.count; i++) {
                ecs_entity_t e = (ecs_entity_t)cmd->data.bulk_visible.entity_ids[i];
                RenderableComp *r = ecs_world_get_renderable(w, e);
                if (r) {
                    r->visible = cmd->data.bulk_visible.old_visible[i];
                    r->instance_dirty = true;
                }
            }
            break;
        }

        case CMD_SET_GEOMETRY_VERTICES: {
            ecs_entity_t e = (ecs_entity_t)cmd->data.set_vertices.entity_id;
            GeometryComp *g = ecs_world_get_geometry(w, e);
            if (g) {
                for (int i = 0; i < cmd->data.set_vertices.count; i++) {
                    undo_set_vertex_pos(g, cmd->data.set_vertices.vertex_indices[i],
                                        cmd->data.set_vertices.old_positions[i]);
                }
            }
            RenderableComp *r = ecs_world_get_renderable(w, e);
            if (r) r->instance_dirty = true;
            break;
        }

        default:
            break;
    }
}

// ============================================================================
// Public Undo/Redo Functions
// ============================================================================

static inline bool undo_redo_undo(undo_redo_t *ur) {
    if (!undo_redo_can_undo(ur)) return false;

    ur->current--;
    undo_unapply_command(ur, &ur->commands[ur->current]);

    if (ur->on_change) {
        ur->on_change(ur->user_data);
    }

    return true;
}

static inline bool undo_redo_redo(undo_redo_t *ur) {
    if (!undo_redo_can_redo(ur)) return false;

    undo_apply_command(ur, &ur->commands[ur->current]);
    ur->current++;

    if (ur->on_change) {
        ur->on_change(ur->user_data);
    }

    return true;
}

#endif // UNDO_REDO_EXEC_H
