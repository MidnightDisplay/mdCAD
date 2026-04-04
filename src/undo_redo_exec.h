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
#include "math/math_undo_editor.h"
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
        snap.has_geometry = true;
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
            case GEOM_MESH: {
                geom_mesh_data_t *mesh = &g->data.mesh;
                snap.data.mesh.vertex_count = mesh->vertex_count;
                snap.data.mesh.index_count = mesh->index_count;
                snap.data.mesh.vertices = (vec3_t*)malloc(sizeof(vec3_t) * mesh->vertex_count);
                memcpy(snap.data.mesh.vertices, mesh->vertices, sizeof(vec3_t) * mesh->vertex_count);
                snap.data.mesh.indices = (uint32_t*)malloc(sizeof(uint32_t) * mesh->index_count);
                memcpy(snap.data.mesh.indices, mesh->indices, sizeof(uint32_t) * mesh->index_count);
                if (mesh->normals) {
                    snap.data.mesh.normals = (vec3_t*)malloc(sizeof(vec3_t) * mesh->vertex_count);
                    memcpy(snap.data.mesh.normals, mesh->normals, sizeof(vec3_t) * mesh->vertex_count);
                } else {
                    snap.data.mesh.normals = NULL;
                }
                if (mesh->vertex_colors) {
                    snap.data.mesh.vertex_colors = (vec4_t*)malloc(sizeof(vec4_t) * mesh->vertex_count);
                    memcpy(snap.data.mesh.vertex_colors, mesh->vertex_colors, sizeof(vec4_t) * mesh->vertex_count);
                } else {
                    snap.data.mesh.vertex_colors = NULL;
                }
                break;
            }
            default:
                break;
        }
    }

    // Get renderable
    RenderableComp *r = ecs_world_get_renderable(w, e);
    if (r) {
        snap.has_renderable = true;
        snap.visible = r->visible;
        snap.layer = r->layer;
    }

    // Get optional non-geometry components
    LightComp *light = ecs_world_get_light(w, e);
    if (light) {
        snap.has_light = true;
        snap.light = *light;
    }

    SketchComp *sketch = ecs_world_get_sketch(w, e);
    if (sketch) {
        snap.has_sketch = true;
        snap.sketch = *sketch;
    }

    ConstraintComp *constraint = ecs_world_get_constraint(w, e);
    if (constraint) {
        snap.has_constraint = true;
        snap.constraint = *constraint;
    }

    SketchGeometryStateComp *geom_state = ecs_world_get_sketch_geometry_state(w, e);
    if (geom_state) {
        snap.has_sketch_geometry_state = true;
        snap.sketch_geometry_state = *geom_state;
    }

    // Get parent
    snap.parent_id = (uint64_t)scene_get_parent(scene, e);

    // Get label
    LabelComp *label = ecs_world_get_label(w, e);
    snap.has_label = (label != NULL);
    if (label) {
        snap.label = *label;
    }

    return snap;
}

static inline void undo_collect_linked_constraints_for_geometries(ecs_scene_t *scene,
                                                                   const ecs_entity_t *geometry_entities,
                                                                   int geometry_count,
                                                                   undo_constraint_snapshot_t **out_snapshots,
                                                                   int *out_count) {
    if (!scene || !out_snapshots || !out_count) return;
    *out_snapshots = NULL;
    *out_count = 0;
    if (!geometry_entities || geometry_count <= 0) return;

    undo_constraint_snapshot_t *snapshots = NULL;
    int snapshot_count = 0;

    for (int gi = 0; gi < geometry_count; gi++) {
        ecs_entity_t geometry_entity = geometry_entities[gi];
        if (geometry_entity == 0 || !ecs_is_alive(scene->world->world, geometry_entity)) continue;
        if (!ecs_world_get_geometry(scene->world, geometry_entity)) continue;

        ConstraintParticipantComp *refs = ecs_world_get_constraint_participant(scene->world, geometry_entity);
        if (!refs || refs->constraint_count == 0) continue;

        uint32_t linked_count = refs->constraint_count;
        if (linked_count > CONSTRAINT_PARTICIPANT_MAX_REFS) {
            linked_count = CONSTRAINT_PARTICIPANT_MAX_REFS;
        }
        for (uint32_t i = 0; i < linked_count; i++) {
            ecs_entity_t constraint_entity = (ecs_entity_t)refs->constraints[i];
            if (constraint_entity == 0 || !ecs_is_alive(scene->world->world, constraint_entity)) continue;

            ConstraintComp *constraint = ecs_world_get_constraint(scene->world, constraint_entity);
            if (!constraint) continue;

            bool already_captured = false;
            for (int si = 0; si < snapshot_count; si++) {
                if ((ecs_entity_t)snapshots[si].entity_id == constraint_entity) {
                    already_captured = true;
                    break;
                }
            }
            if (already_captured) continue;

            undo_constraint_snapshot_t *grown = (undo_constraint_snapshot_t*)realloc(
                snapshots, sizeof(undo_constraint_snapshot_t) * (size_t)(snapshot_count + 1));
            if (!grown) {
                if (snapshots) free(snapshots);
                *out_snapshots = NULL;
                *out_count = 0;
                return;
            }
            snapshots = grown;
            undo_constraint_snapshot_t *snap = &snapshots[snapshot_count++];
            memset(snap, 0, sizeof(*snap));
            snap->entity_id = (uint64_t)constraint_entity;
            snap->parent_id = (uint64_t)scene_get_parent(scene, constraint_entity);
            snap->constraint = *constraint;

            LabelComp *label = ecs_world_get_label(scene->world, constraint_entity);
            snap->has_label = (label != NULL);
            if (label) {
                snap->label = *label;
            }
        }
    }

    *out_snapshots = snapshots;
    *out_count = snapshot_count;
}

static inline uint64_t undo_map_entity_id(uint64_t original_id,
                                          const uint64_t *old_ids,
                                          const uint64_t *new_ids,
                                          int count) {
    if (original_id == 0) return 0;
    if (!old_ids || !new_ids || count <= 0) return original_id;
    for (int i = 0; i < count; i++) {
        if (old_ids[i] == original_id) {
            return new_ids[i];
        }
    }
    return original_id;
}

static inline bool undo_array_contains_entity(const ecs_entity_t *entities, int count, ecs_entity_t entity) {
    if (!entities || count <= 0 || entity == 0) return false;
    for (int i = 0; i < count; i++) {
        if (entities[i] == entity) return true;
    }
    return false;
}

static inline bool undo_array_push_unique_entity(ecs_entity_t **entities,
                                                 int *count,
                                                 int *capacity,
                                                 ecs_entity_t entity) {
    if (!entities || !count || !capacity || entity == 0) return false;
    if (undo_array_contains_entity(*entities, *count, entity)) return true;
    if (*count >= *capacity) {
        int new_capacity = (*capacity == 0) ? 32 : (*capacity * 2);
        ecs_entity_t *grown = (ecs_entity_t*)realloc(*entities, sizeof(ecs_entity_t) * (size_t)new_capacity);
        if (!grown) return false;
        *entities = grown;
        *capacity = new_capacity;
    }
    (*entities)[(*count)++] = entity;
    return true;
}

static inline bool undo_collect_delete_subtree_recursive(ecs_scene_t *scene,
                                                         ecs_entity_t root,
                                                         ecs_entity_t **out_entities,
                                                         int *out_count,
                                                         int *out_capacity) {
    if (!scene || root == 0 || !out_entities || !out_count || !out_capacity) return false;
    if (!ecs_is_alive(scene->world->world, root)) return true;

    if (!undo_array_push_unique_entity(out_entities, out_count, out_capacity, root)) {
        return false;
    }

    ecs_iter_t it = ecs_children(scene->world->world, root);
    while (ecs_children_next(&it)) {
        for (int i = 0; i < it.count; i++) {
            if (!undo_collect_delete_subtree_recursive(scene, it.entities[i],
                                                       out_entities, out_count, out_capacity)) {
                return false;
            }
        }
    }
    return true;
}

static inline bool undo_uint64_array_contains(const uint64_t *values, int count, uint64_t target) {
    if (!values || count <= 0 || target == 0) return false;
    for (int i = 0; i < count; i++) {
        if (values[i] == target) return true;
    }
    return false;
}

static inline bool undo_uint64_array_push_unique(uint64_t **values, int *count, int *capacity, uint64_t value) {
    if (!values || !count || !capacity || value == 0) return false;
    if (undo_uint64_array_contains(*values, *count, value)) return true;
    if (*count >= *capacity) {
        int new_capacity = (*capacity == 0) ? 16 : (*capacity * 2);
        uint64_t *grown = (uint64_t*)realloc(*values, sizeof(uint64_t) * (size_t)new_capacity);
        if (!grown) return false;
        *values = grown;
        *capacity = new_capacity;
    }
    (*values)[(*count)++] = value;
    return true;
}

static inline void undo_filter_linked_constraint_snapshots(undo_constraint_snapshot_t **snapshots,
                                                           int *snapshot_count,
                                                           const uint64_t *deleted_entity_ids,
                                                           int deleted_count) {
    if (!snapshots || !snapshot_count || *snapshot_count <= 0 || !*snapshots) return;

    int write_index = 0;
    for (int i = 0; i < *snapshot_count; i++) {
        undo_constraint_snapshot_t snap = (*snapshots)[i];
        if (undo_uint64_array_contains(deleted_entity_ids, deleted_count, snap.entity_id)) {
            continue;
        }
        (*snapshots)[write_index++] = snap;
    }
    *snapshot_count = write_index;
    if (write_index == 0) {
        free(*snapshots);
        *snapshots = NULL;
    }
}

static inline void undo_restore_bulk_entity_relationships(ecs_scene_t *scene,
                                                          undo_entity_snapshot_t *snapshots,
                                                          const uint64_t *old_ids,
                                                          const uint64_t *new_ids,
                                                          int count) {
    if (!scene || !snapshots || !old_ids || !new_ids || count <= 0) return;

    uint64_t *sketch_ids = NULL;
    int sketch_count = 0;
    int sketch_capacity = 0;

    // First pass: restore parent links using ID remap (old -> new IDs)
    for (int i = 0; i < count; i++) {
        ecs_entity_t child = (ecs_entity_t)new_ids[i];
        if (child == 0 || !ecs_is_alive(scene->world->world, child)) continue;

        uint64_t old_parent = snapshots[i].parent_id;
        if (old_parent == 0) continue;
        ecs_entity_t remapped_parent = (ecs_entity_t)undo_map_entity_id(old_parent, old_ids, new_ids, count);
        if (remapped_parent != 0 && ecs_is_alive(scene->world->world, remapped_parent)) {
            scene_set_parent(scene, child, remapped_parent);
            if (scene_is_sketch(scene, remapped_parent)) {
                undo_uint64_array_push_unique(&sketch_ids, &sketch_count, &sketch_capacity, (uint64_t)remapped_parent);
            }
        }
    }

    // Second pass: remap direct constraint participants and rebuild participant refs
    for (int i = 0; i < count; i++) {
        ecs_entity_t constraint_entity = (ecs_entity_t)new_ids[i];
        if (constraint_entity == 0 || !ecs_is_alive(scene->world->world, constraint_entity)) continue;
        if (!snapshots[i].has_constraint) continue;

        ConstraintComp *constraint = ecs_world_get_constraint(scene->world, constraint_entity);
        if (!constraint) continue;

        uint64_t remapped_participants[CONSTRAINT_MAX_PARTICIPANTS] = {0};
        uint32_t remapped_count = 0;
        for (uint32_t pi = 0; pi < constraint->participant_count; pi++) {
            uint64_t mapped = undo_map_entity_id(constraint->participants[pi], old_ids, new_ids, count);
            if (mapped == 0) continue;
            if (!ecs_is_alive(scene->world->world, (ecs_entity_t)mapped)) continue;

            bool duplicate = false;
            for (uint32_t existing = 0; existing < remapped_count; existing++) {
                if (remapped_participants[existing] == mapped) {
                    duplicate = true;
                    break;
                }
            }
            if (duplicate) continue;
            if (remapped_count >= CONSTRAINT_MAX_PARTICIPANTS) break;
            remapped_participants[remapped_count++] = mapped;
        }

        if (remapped_count < constraint_type_min_participants(constraint->type)) {
            scene_remove_entity(scene, constraint_entity);
            continue;
        }

        constraint->participant_count = remapped_count;
        memset(constraint->participants, 0, sizeof(constraint->participants));
        for (uint32_t pi = 0; pi < remapped_count; pi++) {
            constraint->participants[pi] = remapped_participants[pi];
        }

        for (uint32_t pi = 0; pi < remapped_count; pi++) {
            ecs_entity_t participant = (ecs_entity_t)remapped_participants[pi];
            ConstraintParticipantComp *refs = ecs_world_get_constraint_participant(scene->world, participant);
            if (!refs) {
                ConstraintParticipantComp init_refs = constraint_participant_comp_default();
                ecs_world_set_constraint_participant(scene->world, participant, &init_refs);
                refs = ecs_world_get_constraint_participant(scene->world, participant);
            }
            if (refs) {
                constraint_participant_add(refs, (uint64_t)constraint_entity);
            }
        }

        ecs_entity_t parent = scene_get_parent(scene, constraint_entity);
        if (parent != 0 && scene_is_sketch(scene, parent)) {
            undo_uint64_array_push_unique(&sketch_ids, &sketch_count, &sketch_capacity, (uint64_t)parent);
        }
    }

    // Third pass: refresh sketch metadata for touched sketches and restored sketch roots
    for (int i = 0; i < count; i++) {
        ecs_entity_t e = (ecs_entity_t)new_ids[i];
        if (e != 0 && ecs_is_alive(scene->world->world, e) && scene_is_sketch(scene, e)) {
            undo_uint64_array_push_unique(&sketch_ids, &sketch_count, &sketch_capacity, (uint64_t)e);
        }
    }
    for (int i = 0; i < sketch_count; i++) {
        ecs_entity_t sketch = (ecs_entity_t)sketch_ids[i];
        if (sketch != 0 && ecs_is_alive(scene->world->world, sketch) && scene_is_sketch(scene, sketch)) {
            scene_refresh_sketch_metadata(scene, sketch);
        }
    }
    if (sketch_ids) free(sketch_ids);
}

static inline void undo_restore_linked_constraints(ecs_scene_t *scene,
                                                   undo_constraint_snapshot_t *snapshots,
                                                   int snapshot_count,
                                                   const uint64_t *old_ids,
                                                   const uint64_t *new_ids,
                                                   int id_map_count) {
    if (!scene || !snapshots || snapshot_count <= 0) return;

    for (int si = 0; si < snapshot_count; si++) {
        undo_constraint_snapshot_t *snap = &snapshots[si];
        ConstraintComp restored = snap->constraint;
        uint64_t remapped_participants[CONSTRAINT_MAX_PARTICIPANTS] = {0};
        uint32_t remapped_count = 0;

        for (uint32_t pi = 0; pi < restored.participant_count; pi++) {
            uint64_t original_participant = restored.participants[pi];
            uint64_t remapped = undo_map_entity_id(original_participant, old_ids, new_ids, id_map_count);
            if (remapped == 0) continue;
            if (!ecs_is_alive(scene->world->world, (ecs_entity_t)remapped)) continue;

            bool duplicate = false;
            for (uint32_t existing = 0; existing < remapped_count; existing++) {
                if (remapped_participants[existing] == remapped) {
                    duplicate = true;
                    break;
                }
            }
            if (duplicate) continue;
            if (remapped_count >= CONSTRAINT_MAX_PARTICIPANTS) break;
            remapped_participants[remapped_count++] = remapped;
        }

        if (remapped_count < constraint_type_min_participants(restored.type)) {
            continue;
        }

        restored.participant_count = remapped_count;
        memset(restored.participants, 0, sizeof(restored.participants));
        for (uint32_t pi = 0; pi < remapped_count; pi++) {
            restored.participants[pi] = remapped_participants[pi];
        }
        restored.display_decimals = constraint_clamp_decimals((int)restored.display_decimals);

        ecs_entity_t parent = (ecs_entity_t)undo_map_entity_id(snap->parent_id, old_ids, new_ids, id_map_count);
        if (parent == 0 || !ecs_is_alive(scene->world->world, parent)) {
            ecs_entity_t fallback_parent = scene_get_parent(scene, (ecs_entity_t)remapped_participants[0]);
            if (scene_is_sketch(scene, fallback_parent)) {
                parent = fallback_parent;
            }
        }

        ecs_entity_t restored_constraint = (ecs_entity_t)snap->entity_id;
        ConstraintComp *existing_constraint = NULL;
        if (restored_constraint != 0 && ecs_is_alive(scene->world->world, restored_constraint)) {
            existing_constraint = ecs_world_get_constraint(scene->world, restored_constraint);
        }

        if (existing_constraint) {
            scene_constraint_unlink_participants(scene, existing_constraint, restored_constraint);
        } else {
            restored_constraint = scene_add_anchor(scene, "", "");
            if (restored_constraint == 0) continue;
        }

        ecs_world_set_constraint(scene->world, restored_constraint, &restored);
        if (parent != 0) {
            scene_set_parent(scene, restored_constraint, parent);
        }

        if (snap->has_label) {
            ecs_world_set_label(scene->world, restored_constraint, &snap->label);
        } else if (scene_is_sketch(scene, parent)) {
            scene_set_constraint_default_label(scene, parent, restored_constraint, restored.type);
        }

        for (uint32_t pi = 0; pi < restored.participant_count; pi++) {
            ecs_entity_t participant = (ecs_entity_t)restored.participants[pi];
            ConstraintParticipantComp *refs = ecs_world_get_constraint_participant(scene->world, participant);
            if (!refs) {
                ConstraintParticipantComp init_refs = constraint_participant_comp_default();
                ecs_world_set_constraint_participant(scene->world, participant, &init_refs);
                refs = ecs_world_get_constraint_participant(scene->world, participant);
            }
            if (refs) {
                constraint_participant_add(refs, (uint64_t)restored_constraint);
            }
        }

        if (scene_is_sketch(scene, parent)) {
            scene_refresh_sketch_metadata(scene, parent);
        }
        snap->entity_id = (uint64_t)restored_constraint;
    }
}

// Create an entity from a snapshot (returns the new entity ID)
static inline ecs_entity_t undo_create_from_snapshot(ecs_scene_t *scene, undo_entity_snapshot_t *snap) {
    ecs_entity_t e = 0;

    if (snap->has_geometry) {
        // Create entity based on geometry type
        switch (snap->geom_type) {
            case UNDO_GEOM_POINT:
                e = scene_add_point(scene,
                    mdcad_undo_editor_vec3_make(snap->data.point.x, snap->data.point.y, snap->data.point.z),
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
            case UNDO_GEOM_MESH:
                if (snap->data.mesh.vertices && snap->data.mesh.indices &&
                    snap->data.mesh.vertex_count >= 3 && snap->data.mesh.index_count >= 3) {
                    if (snap->data.mesh.vertex_colors) {
                        e = scene_add_mesh_colored(scene,
                            snap->data.mesh.vertices, snap->data.mesh.vertex_count,
                            snap->data.mesh.indices, snap->data.mesh.index_count,
                            snap->data.mesh.vertex_colors);
                    } else {
                        e = scene_add_mesh(scene,
                            snap->data.mesh.vertices, snap->data.mesh.vertex_count,
                            snap->data.mesh.indices, snap->data.mesh.index_count,
                            snap->color);
                    }
                }
                break;
            default:
                break;
        }
    } else {
        e = scene_add_anchor(scene, "", "");
        if (e == 0) return 0;
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

        if (snap->has_label) {
            ecs_world_set_label(scene->world, e, &snap->label);
        }
        if (snap->has_light) {
            ecs_set_id(scene->world->world, e, scene->world->LightComp_id, sizeof(LightComp), &snap->light);
        }
        if (snap->has_sketch) {
            ecs_world_set_sketch(scene->world, e, &snap->sketch);
        }
        if (snap->has_constraint) {
            ecs_world_set_constraint(scene->world, e, &snap->constraint);
        }
        if (snap->has_sketch_geometry_state) {
            ecs_world_set_sketch_geometry_state(scene->world, e, &snap->sketch_geometry_state);
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
    int actual_count = 0;
    ecs_entity_t *children = NULL;

    if (child_count > 0) {
        // Allocate array for children
        children = (ecs_entity_t*)malloc(sizeof(ecs_entity_t) * child_count);
        actual_count = scene_get_children(scene, e, children, child_count);

        cmd.data.delete_.child_ids = (uint64_t*)malloc(sizeof(uint64_t) * actual_count);
        cmd.data.delete_.child_snapshots = (undo_entity_snapshot_t*)malloc(sizeof(undo_entity_snapshot_t) * actual_count);
        cmd.data.delete_.child_count = actual_count;

        for (int i = 0; i < actual_count; i++) {
            cmd.data.delete_.child_ids[i] = (uint64_t)children[i];
            cmd.data.delete_.child_snapshots[i] = undo_snapshot_entity(scene, children[i]);
        }
    }

    // Capture constraints that will be removed indirectly when geometry entities are deleted.
    int geometry_capacity = 1 + actual_count;
    ecs_entity_t *geometry_targets = (ecs_entity_t*)malloc(sizeof(ecs_entity_t) * geometry_capacity);
    int geometry_count = 0;
    if (ecs_world_get_geometry(scene->world, e)) {
        geometry_targets[geometry_count++] = e;
    }
    for (int i = 0; i < actual_count; i++) {
        if (ecs_world_get_geometry(scene->world, children[i])) {
            geometry_targets[geometry_count++] = children[i];
        }
    }
    if (geometry_count > 0) {
        undo_collect_linked_constraints_for_geometries(scene,
                                                       geometry_targets,
                                                       geometry_count,
                                                       &cmd.data.delete_.linked_constraint_snapshots,
                                                       &cmd.data.delete_.linked_constraint_count);
    }
    if (geometry_targets) free(geometry_targets);
    if (children) free(children);

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

// Record bulk sketch fixed-state change
static inline void undo_cmd_bulk_set_sketch_fixed(undo_redo_t *ur, ecs_entity_t *entities,
                                                   bool *old_fixed, int count, bool new_fixed) {
    if (!ur || count <= 0) return;

    undo_command_t cmd = {0};
    cmd.type = CMD_BULK_SET_SKETCH_FIXED;
    cmd.data.bulk_sketch_fixed.entity_ids = (uint64_t*)malloc(sizeof(uint64_t) * count);
    cmd.data.bulk_sketch_fixed.old_fixed = (bool*)malloc(sizeof(bool) * count);
    cmd.data.bulk_sketch_fixed.count = count;
    cmd.data.bulk_sketch_fixed.new_fixed = new_fixed;

    for (int i = 0; i < count; i++) {
        cmd.data.bulk_sketch_fixed.entity_ids[i] = (uint64_t)entities[i];
        cmd.data.bulk_sketch_fixed.old_fixed[i] = old_fixed[i];
    }

    undo_redo_push(ur, &cmd);
}

// Record bulk entity delete as one atomic undo command
static inline void undo_cmd_bulk_delete_entities(undo_redo_t *ur, ecs_entity_t *entities, int count) {
    if (!ur || !ur->scene || !entities || count <= 0) return;
    ecs_scene_t *scene = (ecs_scene_t*)ur->scene;

    ecs_entity_t *expanded = NULL;
    int expanded_count = 0;
    int expanded_capacity = 0;
    for (int i = 0; i < count; i++) {
        ecs_entity_t root = entities[i];
        if (root == 0 || !ecs_is_alive(scene->world->world, root)) continue;
        if (!undo_collect_delete_subtree_recursive(scene, root, &expanded, &expanded_count, &expanded_capacity)) {
            if (expanded) free(expanded);
            return;
        }
    }
    if (expanded_count <= 0) {
        if (expanded) free(expanded);
        return;
    }

    undo_command_t cmd = {0};
    cmd.type = CMD_BULK_DELETE_ENTITIES;
    cmd.data.bulk_delete.entity_ids = (uint64_t*)malloc(sizeof(uint64_t) * (size_t)expanded_count);
    cmd.data.bulk_delete.snapshots = (undo_entity_snapshot_t*)calloc((size_t)expanded_count, sizeof(undo_entity_snapshot_t));
    cmd.data.bulk_delete.count = expanded_count;

    if (!cmd.data.bulk_delete.entity_ids || !cmd.data.bulk_delete.snapshots) {
        if (cmd.data.bulk_delete.entity_ids) free(cmd.data.bulk_delete.entity_ids);
        if (cmd.data.bulk_delete.snapshots) free(cmd.data.bulk_delete.snapshots);
        free(expanded);
        return;
    }

    for (int i = 0; i < expanded_count; i++) {
        cmd.data.bulk_delete.entity_ids[i] = (uint64_t)expanded[i];
        cmd.data.bulk_delete.snapshots[i] = undo_snapshot_entity(scene, expanded[i]);
    }

    undo_collect_linked_constraints_for_geometries(scene,
                                                   expanded,
                                                   expanded_count,
                                                   &cmd.data.bulk_delete.linked_constraint_snapshots,
                                                   &cmd.data.bulk_delete.linked_constraint_count);
    undo_filter_linked_constraint_snapshots(&cmd.data.bulk_delete.linked_constraint_snapshots,
                                            &cmd.data.bulk_delete.linked_constraint_count,
                                            cmd.data.bulk_delete.entity_ids,
                                            cmd.data.bulk_delete.count);
    free(expanded);

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

static inline void undo_cmd_script_apply_transaction(undo_redo_t *ur,
                                                     ecs_entity_t sketch,
                                                     const char *before_script,
                                                     const char *after_script) {
    if (!ur || sketch == 0 || !before_script || !after_script) return;

    undo_command_t cmd = {0};
    cmd.type = CMD_SCRIPT_APPLY_TRANSACTION;
    cmd.data.script_apply_transaction.sketch_id = (uint64_t)sketch;
    cmd.data.script_apply_transaction.before_script = undo_strdup(before_script);
    cmd.data.script_apply_transaction.after_script = undo_strdup(after_script);
    if (!cmd.data.script_apply_transaction.before_script ||
        !cmd.data.script_apply_transaction.after_script) {
        if (cmd.data.script_apply_transaction.before_script) free(cmd.data.script_apply_transaction.before_script);
        if (cmd.data.script_apply_transaction.after_script) free(cmd.data.script_apply_transaction.after_script);
        return;
    }

    undo_redo_push(ur, &cmd);
}

static inline void undo_cmd_move_endpoint_participant(undo_redo_t *ur,
                                                      ecs_entity_t owner_entity,
                                                      constraint_participant_role_t role,
                                                      uint8_t sub_index,
                                                      vec3_t old_local_point,
                                                      vec3_t new_local_point) {
    if (!ur) return;
    if (!endpoints_comp_is_supported_role(role)) return;

    undo_command_t cmd = {0};
    cmd.type = CMD_MOVE_ENDPOINT_PARTICIPANT;
    cmd.data.move_endpoint_participant.owner_entity_id = (uint64_t)owner_entity;
    cmd.data.move_endpoint_participant.role = role;
    cmd.data.move_endpoint_participant.sub_index = sub_index;
    cmd.data.move_endpoint_participant.old_local_point = old_local_point;
    cmd.data.move_endpoint_participant.new_local_point = new_local_point;

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
        case GEOM_MESH:
            if (idx >= 0 && idx < g->data.mesh.vertex_count)
                g->data.mesh.vertices[idx] = pos;
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
                t->position = mdcad_undo_editor_vec3_make(cmd->data.set_vec3.new_value.x,
                                                          cmd->data.set_vec3.new_value.y,
                                                          cmd->data.set_vec3.new_value.z);
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
                t->rotation = mdcad_undo_editor_vec3_make(cmd->data.set_vec3.new_value.x,
                                                          cmd->data.set_vec3.new_value.y,
                                                          cmd->data.set_vec3.new_value.z);
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
                t->scale = mdcad_undo_editor_vec3_make(cmd->data.set_vec3.new_value.x,
                                                       cmd->data.set_vec3.new_value.y,
                                                       cmd->data.set_vec3.new_value.z);
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

        case CMD_BULK_SET_SKETCH_FIXED: {
            for (int i = 0; i < cmd->data.bulk_sketch_fixed.count; i++) {
                ecs_entity_t e = (ecs_entity_t)cmd->data.bulk_sketch_fixed.entity_ids[i];
                SketchGeometryStateComp *state = ecs_world_get_sketch_geometry_state(w, e);
                if (!state) {
                    SketchGeometryStateComp init_state = sketch_geometry_state_comp_default();
                    ecs_world_set_sketch_geometry_state(w, e, &init_state);
                    state = ecs_world_get_sketch_geometry_state(w, e);
                }
                if (state) {
                    state->fixed = cmd->data.bulk_sketch_fixed.new_fixed;
                }
                ecs_entity_t parent = scene_get_parent(scene, e);
                if (parent != 0 && scene_is_sketch(scene, parent)) {
                    scene_refresh_sketch_metadata(scene, parent);
                }
            }
            break;
        }

        case CMD_BULK_DELETE_ENTITIES: {
            for (int i = 0; i < cmd->data.bulk_delete.count; i++) {
                ecs_entity_t e = (ecs_entity_t)cmd->data.bulk_delete.entity_ids[i];
                if (!ecs_is_alive(w->world, e)) continue;

                ecs_entity_t parent = scene_get_parent(scene, e);
                if (ur->selection) {
                    selection_buffer_t *sel = (selection_buffer_t*)ur->selection;
                    selection_remove(sel, e);
                }
                scene_remove_entity(scene, e);
                if (parent != 0 && scene_is_sketch(scene, parent)) {
                    scene_refresh_sketch_metadata(scene, parent);
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

        case CMD_SCRIPT_APPLY_TRANSACTION: {
            ecs_entity_t sketch = (ecs_entity_t)cmd->data.script_apply_transaction.sketch_id;
            if (!scene_is_sketch(scene, sketch) || !cmd->data.script_apply_transaction.after_script) break;
            if (ur->selection) {
                selection_clear((selection_buffer_t*)ur->selection);
            }
            sketch_script_error_t error = {0};
            bool prev_suppressed = scene->script_apply_undo_suppressed;
            scene->script_apply_undo_suppressed = true;
            scene_script_apply_commit(scene, sketch, cmd->data.script_apply_transaction.after_script, &error);
            scene->script_apply_undo_suppressed = prev_suppressed;
            break;
        }

        case CMD_MOVE_ENDPOINT_PARTICIPANT: {
            ecs_entity_t owner = (ecs_entity_t)cmd->data.move_endpoint_participant.owner_entity_id;
            GeometryComp *g = ecs_world_get_geometry(w, owner);
            if (!g) break;
            if (!scene_apply_local_point_to_participant(g,
                                                        cmd->data.move_endpoint_participant.role,
                                                        cmd->data.move_endpoint_participant.new_local_point)) {
                break;
            }
            RenderableComp *r = ecs_world_get_renderable(w, owner);
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
            uint64_t *old_ids = (uint64_t*)malloc(sizeof(uint64_t) * (size_t)(1 + cmd->data.delete_.child_count));
            uint64_t *new_ids = (uint64_t*)calloc((size_t)(1 + cmd->data.delete_.child_count), sizeof(uint64_t));
            old_ids[0] = cmd->data.delete_.entity_id;

            ecs_entity_t e = undo_create_from_snapshot(scene, &cmd->data.delete_.snapshot);
            // Update stored entity ID
            cmd->data.delete_.entity_id = (uint64_t)e;
            new_ids[0] = (uint64_t)e;

            // Restore parent
            if (cmd->data.delete_.snapshot.parent_id != 0) {
                scene_set_parent(scene, e, (ecs_entity_t)cmd->data.delete_.snapshot.parent_id);
            }

            // Recreate children
            for (int i = 0; i < cmd->data.delete_.child_count; i++) {
                old_ids[i + 1] = cmd->data.delete_.child_ids[i];
                ecs_entity_t child = undo_create_from_snapshot(scene, &cmd->data.delete_.child_snapshots[i]);
                cmd->data.delete_.child_ids[i] = (uint64_t)child;
                new_ids[i + 1] = (uint64_t)child;
                scene_set_parent(scene, child, e);
            }

            undo_restore_linked_constraints(scene,
                                            cmd->data.delete_.linked_constraint_snapshots,
                                            cmd->data.delete_.linked_constraint_count,
                                            old_ids,
                                            new_ids,
                                            1 + cmd->data.delete_.child_count);
            free(old_ids);
            free(new_ids);
            break;
        }

        case CMD_SET_POSITION: {
            ecs_entity_t e = (ecs_entity_t)cmd->data.set_vec3.entity_id;
            TransformComp *t = ecs_world_get_transform(w, e);
            if (t) {
                t->position = mdcad_undo_editor_vec3_make(cmd->data.set_vec3.old_value.x,
                                                          cmd->data.set_vec3.old_value.y,
                                                          cmd->data.set_vec3.old_value.z);
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
                t->rotation = mdcad_undo_editor_vec3_make(cmd->data.set_vec3.old_value.x,
                                                          cmd->data.set_vec3.old_value.y,
                                                          cmd->data.set_vec3.old_value.z);
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
                t->scale = mdcad_undo_editor_vec3_make(cmd->data.set_vec3.old_value.x,
                                                       cmd->data.set_vec3.old_value.y,
                                                       cmd->data.set_vec3.old_value.z);
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

        case CMD_BULK_SET_SKETCH_FIXED: {
            for (int i = 0; i < cmd->data.bulk_sketch_fixed.count; i++) {
                ecs_entity_t e = (ecs_entity_t)cmd->data.bulk_sketch_fixed.entity_ids[i];
                SketchGeometryStateComp *state = ecs_world_get_sketch_geometry_state(w, e);
                if (!state) {
                    SketchGeometryStateComp init_state = sketch_geometry_state_comp_default();
                    ecs_world_set_sketch_geometry_state(w, e, &init_state);
                    state = ecs_world_get_sketch_geometry_state(w, e);
                }
                if (state) {
                    state->fixed = cmd->data.bulk_sketch_fixed.old_fixed[i];
                }
                ecs_entity_t parent = scene_get_parent(scene, e);
                if (parent != 0 && scene_is_sketch(scene, parent)) {
                    scene_refresh_sketch_metadata(scene, parent);
                }
            }
            break;
        }

        case CMD_BULK_DELETE_ENTITIES: {
            uint64_t *old_ids = (uint64_t*)malloc(sizeof(uint64_t) * (size_t)cmd->data.bulk_delete.count);
            uint64_t *new_ids = (uint64_t*)calloc((size_t)cmd->data.bulk_delete.count, sizeof(uint64_t));
            memcpy(old_ids, cmd->data.bulk_delete.entity_ids, sizeof(uint64_t) * (size_t)cmd->data.bulk_delete.count);

            for (int i = 0; i < cmd->data.bulk_delete.count; i++) {
                ecs_entity_t recreated = undo_create_from_snapshot(scene, &cmd->data.bulk_delete.snapshots[i]);
                cmd->data.bulk_delete.entity_ids[i] = (uint64_t)recreated;
                new_ids[i] = (uint64_t)recreated;
            }

            undo_restore_bulk_entity_relationships(scene,
                                                   cmd->data.bulk_delete.snapshots,
                                                   old_ids,
                                                   new_ids,
                                                   cmd->data.bulk_delete.count);

            undo_restore_linked_constraints(scene,
                                            cmd->data.bulk_delete.linked_constraint_snapshots,
                                            cmd->data.bulk_delete.linked_constraint_count,
                                            old_ids,
                                            new_ids,
                                            cmd->data.bulk_delete.count);
            free(old_ids);
            free(new_ids);
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

        case CMD_SCRIPT_APPLY_TRANSACTION: {
            ecs_entity_t sketch = (ecs_entity_t)cmd->data.script_apply_transaction.sketch_id;
            if (!scene_is_sketch(scene, sketch) || !cmd->data.script_apply_transaction.before_script) break;
            if (ur->selection) {
                selection_clear((selection_buffer_t*)ur->selection);
            }
            sketch_script_error_t error = {0};
            bool prev_suppressed = scene->script_apply_undo_suppressed;
            scene->script_apply_undo_suppressed = true;
            scene_script_apply_commit(scene, sketch, cmd->data.script_apply_transaction.before_script, &error);
            scene->script_apply_undo_suppressed = prev_suppressed;
            break;
        }

        case CMD_MOVE_ENDPOINT_PARTICIPANT: {
            ecs_entity_t owner = (ecs_entity_t)cmd->data.move_endpoint_participant.owner_entity_id;
            GeometryComp *g = ecs_world_get_geometry(w, owner);
            if (!g) break;
            if (!scene_apply_local_point_to_participant(g,
                                                        cmd->data.move_endpoint_participant.role,
                                                        cmd->data.move_endpoint_participant.old_local_point)) {
                break;
            }
            RenderableComp *r = ecs_world_get_renderable(w, owner);
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
