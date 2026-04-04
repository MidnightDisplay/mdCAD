#ifndef UNDO_REDO_H
#define UNDO_REDO_H

/*
 * undo_redo.h - Command pattern undo/redo system for ECS scene editor
 *
 * Supports:
 * - Entity creation/deletion
 * - Property modifications (transform, geometry, visibility)
 * - Reparenting operations
 * - Bulk operations (multi-select color/visibility)
 *
 * Usage:
 *   undo_redo_t undo_redo;
 *   undo_redo_init(&undo_redo, &ecs_scene, 100);  // 100 command capacity
 *
 *   // Record commands before/after operations:
 *   undo_cmd_set_position(&undo_redo, entity, old_pos, new_pos);
 *
 *   // Undo/Redo:
 *   undo_redo_undo(&undo_redo);  // Ctrl+Z
 *   undo_redo_redo(&undo_redo);  // Ctrl+Shift+Z
 *
 *   undo_redo_shutdown(&undo_redo);
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "math3d.h"
#include "components/component_types.h"
#include "components/label_comp.h"
#include "components/light_comp.h"
#include "components/sketch_comp.h"
#include "components/sketch_geometry_state_comp.h"
#include "components/constraint_comp.h"

// Forward declarations - we use void* to avoid coupling to ECS headers
// Actual types: ecs_scene_t* and selection_buffer_t*

static inline char* undo_strdup(const char *src) {
    if (!src) return NULL;
    size_t len = strlen(src) + 1;
    char *out = (char*)malloc(len);
    if (!out) return NULL;
    memcpy(out, src, len);
    return out;
}

// ============================================================================
// Command Types
// ============================================================================

typedef enum {
    CMD_NONE = 0,

    // Entity lifecycle
    CMD_CREATE_ENTITY,
    CMD_DELETE_ENTITY,

    // Transform properties
    CMD_SET_POSITION,
    CMD_SET_ROTATION,
    CMD_SET_SCALE,

    // Geometry properties
    CMD_SET_COLOR,
    CMD_SET_LINE_WIDTH,
    CMD_SET_POINT_SIZE,
    CMD_SET_LINE_ENDPOINTS,   // For line A/B points
    CMD_SET_POINT_POSITION,   // For point geometry position

    // Renderable properties
    CMD_SET_VISIBLE,

    // Hierarchy
    CMD_SET_PARENT,

    // Bulk operations
    CMD_BULK_SET_COLOR,
    CMD_BULK_SET_VISIBLE,
    CMD_BULK_SET_SKETCH_FIXED,
    CMD_BULK_DELETE_ENTITIES,

    // Geometry vertex editing (gizmo geometry mode)
    CMD_SET_GEOMETRY_VERTICES,
    CMD_SCRIPT_APPLY_TRANSACTION,
    CMD_MOVE_ENDPOINT_PARTICIPANT,

    CMD_TYPE_COUNT
} undo_cmd_type_t;

// ============================================================================
// Geometry Type Enum (matches geometry_comp.h)
// ============================================================================

typedef enum {
    UNDO_GEOM_POINT = 0,
    UNDO_GEOM_LINE,
    UNDO_GEOM_POLYLINE,
    UNDO_GEOM_ARC,
    UNDO_GEOM_POLYGON,
    UNDO_GEOM_HELIX,
    UNDO_GEOM_BEZIER,
    UNDO_GEOM_POINT_CLOUD,
    UNDO_GEOM_TRIANGLE,
    UNDO_GEOM_MESH,
    UNDO_GEOM_TYPE_COUNT
} undo_geom_type_t;

// ============================================================================
// Entity Data Snapshot (for create/delete)
// ============================================================================

typedef struct {
    // Component presence
    bool has_geometry;
    bool has_light;
    bool has_sketch;
    bool has_constraint;
    bool has_sketch_geometry_state;
    bool has_renderable;

    // Geometry type
    undo_geom_type_t geom_type;

    // Common properties
    vec4_t color;
    float line_width;
    float point_size;

    // Transform
    vec3_t position;
    vec3_t rotation;
    vec3_t scale;

    // Renderable
    bool visible;
    int layer;

    // Parent (0 = no parent)
    uint64_t parent_id;

    // Label
    bool has_label;
    LabelComp label;

    // Non-geometry components
    LightComp light;
    SketchComp sketch;
    ConstraintComp constraint;
    SketchGeometryStateComp sketch_geometry_state;

    // Geometry-specific data
    union {
        struct { float x, y, z; } point;
        struct { vec3_t a, b; } line;
        struct {
            vec3_t *points;
            int count;
        } polyline;
        struct {
            vec3_t center;
            float radius, start_angle, end_angle;
            vec3_t normal;
            int segments;
        } arc;
        struct {
            vec3_t *points;
            int count;
        } polygon;
        struct {
            vec3_t p0, p1, p2, p3;
            int segments;
        } bezier;
        struct {
            vec3_t axis_start, axis_end;
            float radius, turns;
            int segments;
        } helix;
        struct {
            vec3_t a, b, c;
            vec4_t color_a, color_b, color_c;
            bool has_vertex_colors;
        } triangle;
        struct {
            vec3_t *vertices;
            vec3_t *normals;
            vec4_t *vertex_colors;
            int vertex_count;
            uint32_t *indices;
            int index_count;
        } mesh;
    } data;
} undo_entity_snapshot_t;

// ============================================================================
// Command Data Structures
// ============================================================================

typedef struct {
    uint64_t entity_id;
    undo_entity_snapshot_t snapshot;
} cmd_create_entity_t;

typedef struct {
    uint64_t entity_id;
    undo_entity_snapshot_t snapshot;
    uint64_t parent_id;
    ConstraintComp constraint;
    LabelComp label;
    bool has_label;
} undo_constraint_snapshot_t;

typedef struct {
    uint64_t entity_id;
    undo_entity_snapshot_t snapshot;
    // For hierarchies, we store child data too
    uint64_t *child_ids;
    undo_entity_snapshot_t *child_snapshots;
    int child_count;

    // Constraints deleted as side effects of deleting geometry entities
    undo_constraint_snapshot_t *linked_constraint_snapshots;
    int linked_constraint_count;
} cmd_delete_entity_t;

typedef struct {
    uint64_t entity_id;
    vec3_t old_value;
    vec3_t new_value;
} cmd_set_vec3_t;

typedef struct {
    uint64_t entity_id;
    vec4_t old_value;
    vec4_t new_value;
} cmd_set_color_t;

typedef struct {
    uint64_t entity_id;
    float old_value;
    float new_value;
} cmd_set_float_t;

typedef struct {
    uint64_t entity_id;
    vec3_t old_a, old_b;
    vec3_t new_a, new_b;
} cmd_set_line_endpoints_t;

typedef struct {
    uint64_t entity_id;
    bool old_value;
    bool new_value;
} cmd_set_visible_t;

typedef struct {
    uint64_t entity_id;
    uint64_t old_parent;
    uint64_t new_parent;
} cmd_set_parent_t;

typedef struct {
    uint64_t *entity_ids;
    vec4_t *old_colors;
    int count;
    vec4_t new_color;
} cmd_bulk_set_color_t;

typedef struct {
    uint64_t *entity_ids;
    bool *old_visible;
    int count;
    bool new_visible;
} cmd_bulk_set_visible_t;

typedef struct {
    uint64_t *entity_ids;
    bool *old_fixed;
    int count;
    bool new_fixed;
} cmd_bulk_set_sketch_fixed_t;

typedef struct {
    uint64_t *entity_ids;
    undo_entity_snapshot_t *snapshots;
    int count;

    // Constraints deleted as side effects of deleting geometry entities
    undo_constraint_snapshot_t *linked_constraint_snapshots;
    int linked_constraint_count;
} cmd_bulk_delete_entities_t;

typedef struct {
    uint64_t entity_id;
    int *vertex_indices;      // Which vertices changed
    vec3_t *old_positions;    // Original positions (local space)
    vec3_t *new_positions;    // New positions (local space)
    int count;
} cmd_set_geometry_vertices_t;

typedef struct {
    uint64_t sketch_id;
    char *before_script;
    char *after_script;
} cmd_script_apply_transaction_t;

typedef struct {
    uint64_t owner_entity_id;
    constraint_participant_role_t role;
    uint8_t sub_index;
    vec3_t old_local_point;
    vec3_t new_local_point;
} cmd_move_endpoint_participant_t;

// ============================================================================
// Command Union
// ============================================================================

typedef struct {
    undo_cmd_type_t type;

    union {
        cmd_create_entity_t create;
        cmd_delete_entity_t delete_;
        cmd_set_vec3_t set_vec3;     // position, rotation, scale, point_position
        cmd_set_color_t set_color;
        cmd_set_float_t set_float;   // line_width, point_size
        cmd_set_line_endpoints_t set_endpoints;
        cmd_set_visible_t set_visible;
        cmd_set_parent_t set_parent;
        cmd_bulk_set_color_t bulk_color;
        cmd_bulk_set_visible_t bulk_visible;
        cmd_bulk_set_sketch_fixed_t bulk_sketch_fixed;
        cmd_bulk_delete_entities_t bulk_delete;
        cmd_set_geometry_vertices_t set_vertices;
        cmd_script_apply_transaction_t script_apply_transaction;
        cmd_move_endpoint_participant_t move_endpoint_participant;
    } data;
} undo_command_t;

// ============================================================================
// Undo/Redo Stack
// ============================================================================

typedef struct {
    undo_command_t *commands;
    int capacity;
    int count;
    int current;  // Index of next command to execute (for undo: current-1, for redo: current)

    void *scene;      // ecs_scene_t* - the ECS scene
    void *selection;  // selection_buffer_t* - the selection buffer

    // Callback for UI refresh after undo/redo
    void (*on_change)(void *user_data);
    void *user_data;
} undo_redo_t;

// ============================================================================
// Helper: Free command resources
// ============================================================================

static inline void undo_command_free(undo_command_t *cmd) {
    switch (cmd->type) {
        case CMD_CREATE_ENTITY:
            // Free polyline/polygon points if present
            if (cmd->data.create.snapshot.geom_type == UNDO_GEOM_POLYLINE &&
                cmd->data.create.snapshot.data.polyline.points) {
                free(cmd->data.create.snapshot.data.polyline.points);
            }
            if (cmd->data.create.snapshot.geom_type == UNDO_GEOM_POLYGON &&
                cmd->data.create.snapshot.data.polygon.points) {
                free(cmd->data.create.snapshot.data.polygon.points);
            }
            if (cmd->data.create.snapshot.geom_type == UNDO_GEOM_MESH) {
                if (cmd->data.create.snapshot.data.mesh.vertices) free(cmd->data.create.snapshot.data.mesh.vertices);
                if (cmd->data.create.snapshot.data.mesh.normals) free(cmd->data.create.snapshot.data.mesh.normals);
                if (cmd->data.create.snapshot.data.mesh.vertex_colors) free(cmd->data.create.snapshot.data.mesh.vertex_colors);
                if (cmd->data.create.snapshot.data.mesh.indices) free(cmd->data.create.snapshot.data.mesh.indices);
            }
            break;

        case CMD_DELETE_ENTITY:
            // Free snapshot points
            if (cmd->data.delete_.snapshot.geom_type == UNDO_GEOM_POLYLINE &&
                cmd->data.delete_.snapshot.data.polyline.points) {
                free(cmd->data.delete_.snapshot.data.polyline.points);
            }
            if (cmd->data.delete_.snapshot.geom_type == UNDO_GEOM_POLYGON &&
                cmd->data.delete_.snapshot.data.polygon.points) {
                free(cmd->data.delete_.snapshot.data.polygon.points);
            }
            if (cmd->data.delete_.snapshot.geom_type == UNDO_GEOM_MESH) {
                if (cmd->data.delete_.snapshot.data.mesh.vertices) free(cmd->data.delete_.snapshot.data.mesh.vertices);
                if (cmd->data.delete_.snapshot.data.mesh.normals) free(cmd->data.delete_.snapshot.data.mesh.normals);
                if (cmd->data.delete_.snapshot.data.mesh.vertex_colors) free(cmd->data.delete_.snapshot.data.mesh.vertex_colors);
                if (cmd->data.delete_.snapshot.data.mesh.indices) free(cmd->data.delete_.snapshot.data.mesh.indices);
            }
            // Free child snapshots
            if (cmd->data.delete_.child_snapshots) {
                for (int i = 0; i < cmd->data.delete_.child_count; i++) {
                    undo_entity_snapshot_t *snap = &cmd->data.delete_.child_snapshots[i];
                    if (snap->geom_type == UNDO_GEOM_POLYLINE && snap->data.polyline.points) {
                        free(snap->data.polyline.points);
                    }
                    if (snap->geom_type == UNDO_GEOM_POLYGON && snap->data.polygon.points) {
                        free(snap->data.polygon.points);
                    }
                    if (snap->geom_type == UNDO_GEOM_MESH) {
                        if (snap->data.mesh.vertices) free(snap->data.mesh.vertices);
                        if (snap->data.mesh.normals) free(snap->data.mesh.normals);
                        if (snap->data.mesh.vertex_colors) free(snap->data.mesh.vertex_colors);
                        if (snap->data.mesh.indices) free(snap->data.mesh.indices);
                    }
                }
                free(cmd->data.delete_.child_snapshots);
            }
            if (cmd->data.delete_.child_ids) {
                free(cmd->data.delete_.child_ids);
            }
            if (cmd->data.delete_.linked_constraint_snapshots) {
                free(cmd->data.delete_.linked_constraint_snapshots);
            }
            break;

        case CMD_BULK_SET_COLOR:
            if (cmd->data.bulk_color.entity_ids) free(cmd->data.bulk_color.entity_ids);
            if (cmd->data.bulk_color.old_colors) free(cmd->data.bulk_color.old_colors);
            break;

        case CMD_BULK_SET_VISIBLE:
            if (cmd->data.bulk_visible.entity_ids) free(cmd->data.bulk_visible.entity_ids);
            if (cmd->data.bulk_visible.old_visible) free(cmd->data.bulk_visible.old_visible);
            break;

        case CMD_BULK_SET_SKETCH_FIXED:
            if (cmd->data.bulk_sketch_fixed.entity_ids) free(cmd->data.bulk_sketch_fixed.entity_ids);
            if (cmd->data.bulk_sketch_fixed.old_fixed) free(cmd->data.bulk_sketch_fixed.old_fixed);
            break;

        case CMD_BULK_DELETE_ENTITIES:
            if (cmd->data.bulk_delete.snapshots) {
                for (int i = 0; i < cmd->data.bulk_delete.count; i++) {
                    undo_entity_snapshot_t *snap = &cmd->data.bulk_delete.snapshots[i];
                    if (snap->geom_type == UNDO_GEOM_POLYLINE && snap->data.polyline.points) {
                        free(snap->data.polyline.points);
                    }
                    if (snap->geom_type == UNDO_GEOM_POLYGON && snap->data.polygon.points) {
                        free(snap->data.polygon.points);
                    }
                    if (snap->geom_type == UNDO_GEOM_MESH) {
                        if (snap->data.mesh.vertices) free(snap->data.mesh.vertices);
                        if (snap->data.mesh.normals) free(snap->data.mesh.normals);
                        if (snap->data.mesh.vertex_colors) free(snap->data.mesh.vertex_colors);
                        if (snap->data.mesh.indices) free(snap->data.mesh.indices);
                    }
                }
                free(cmd->data.bulk_delete.snapshots);
            }
            if (cmd->data.bulk_delete.entity_ids) free(cmd->data.bulk_delete.entity_ids);
            if (cmd->data.bulk_delete.linked_constraint_snapshots) {
                free(cmd->data.bulk_delete.linked_constraint_snapshots);
            }
            break;

        case CMD_SET_GEOMETRY_VERTICES:
            if (cmd->data.set_vertices.vertex_indices) free(cmd->data.set_vertices.vertex_indices);
            if (cmd->data.set_vertices.old_positions) free(cmd->data.set_vertices.old_positions);
            if (cmd->data.set_vertices.new_positions) free(cmd->data.set_vertices.new_positions);
            break;

        case CMD_SCRIPT_APPLY_TRANSACTION:
            if (cmd->data.script_apply_transaction.before_script) {
                free(cmd->data.script_apply_transaction.before_script);
            }
            if (cmd->data.script_apply_transaction.after_script) {
                free(cmd->data.script_apply_transaction.after_script);
            }
            break;

        default:
            break;
    }
    cmd->type = CMD_NONE;
}

// ============================================================================
// Initialization / Shutdown
// ============================================================================

static inline void undo_redo_init(undo_redo_t *ur, void *scene, int capacity) {
    memset(ur, 0, sizeof(undo_redo_t));
    ur->scene = scene;
    ur->capacity = capacity > 0 ? capacity : 100;
    ur->commands = (undo_command_t *)calloc(ur->capacity, sizeof(undo_command_t));
    ur->count = 0;
    ur->current = 0;
}

static inline void undo_redo_set_selection(undo_redo_t *ur, void *selection) {
    ur->selection = (struct selection_buffer *)selection;
}

static inline void undo_redo_set_callback(undo_redo_t *ur, void (*callback)(void*), void *user_data) {
    ur->on_change = callback;
    ur->user_data = user_data;
}

static inline void undo_redo_shutdown(undo_redo_t *ur) {
    if (ur->commands) {
        for (int i = 0; i < ur->count; i++) {
            undo_command_free(&ur->commands[i]);
        }
        free(ur->commands);
        ur->commands = NULL;
    }
    ur->count = 0;
    ur->current = 0;
}

// ============================================================================
// Stack Operations
// ============================================================================

// Clear commands from current to end (called when recording new command after undo)
static inline void undo_redo_clear_redo(undo_redo_t *ur) {
    for (int i = ur->current; i < ur->count; i++) {
        undo_command_free(&ur->commands[i]);
    }
    ur->count = ur->current;
}

// Push a command onto the stack
static inline void undo_redo_push(undo_redo_t *ur, undo_command_t *cmd) {
    // Clear any redo history
    undo_redo_clear_redo(ur);

    // If at capacity, shift everything down (remove oldest)
    if (ur->count >= ur->capacity) {
        undo_command_free(&ur->commands[0]);
        memmove(&ur->commands[0], &ur->commands[1], (ur->capacity - 1) * sizeof(undo_command_t));
        ur->count = ur->capacity - 1;
        ur->current = ur->count;
    }

    // Add new command
    ur->commands[ur->count] = *cmd;
    ur->count++;
    ur->current = ur->count;
}

static inline bool undo_redo_can_undo(undo_redo_t *ur) {
    return ur->current > 0;
}

static inline bool undo_redo_can_redo(undo_redo_t *ur) {
    return ur->current < ur->count;
}

static inline int undo_redo_get_undo_count(undo_redo_t *ur) {
    return ur->current;
}

static inline int undo_redo_get_redo_count(undo_redo_t *ur) {
    return ur->count - ur->current;
}

// ============================================================================
// Command Names (for UI display)
// ============================================================================

static inline const char* undo_cmd_name(undo_cmd_type_t type) {
    switch (type) {
        case CMD_CREATE_ENTITY: return "Create Entity";
        case CMD_DELETE_ENTITY: return "Delete Entity";
        case CMD_SET_POSITION: return "Move";
        case CMD_SET_ROTATION: return "Rotate";
        case CMD_SET_SCALE: return "Scale";
        case CMD_SET_COLOR: return "Change Color";
        case CMD_SET_LINE_WIDTH: return "Change Line Width";
        case CMD_SET_POINT_SIZE: return "Change Point Size";
        case CMD_SET_LINE_ENDPOINTS: return "Edit Line";
        case CMD_SET_POINT_POSITION: return "Move Point";
        case CMD_SET_VISIBLE: return "Toggle Visibility";
        case CMD_SET_PARENT: return "Reparent";
        case CMD_BULK_SET_COLOR: return "Bulk Color";
        case CMD_BULK_SET_VISIBLE: return "Bulk Visibility";
        case CMD_BULK_SET_SKETCH_FIXED: return "Bulk Fix/Unfix";
        case CMD_BULK_DELETE_ENTITIES: return "Bulk Delete";
        case CMD_SET_GEOMETRY_VERTICES: return "Edit Vertices";
        case CMD_SCRIPT_APPLY_TRANSACTION: return "Apply Script";
        case CMD_MOVE_ENDPOINT_PARTICIPANT: return "Move Endpoint";
        default: return "Unknown";
    }
}

// ============================================================================
// Get undo/redo action names
// ============================================================================

static inline const char* undo_redo_get_undo_name(undo_redo_t *ur) {
    if (!undo_redo_can_undo(ur)) return NULL;
    return undo_cmd_name(ur->commands[ur->current - 1].type);
}

static inline const char* undo_redo_get_redo_name(undo_redo_t *ur) {
    if (!undo_redo_can_redo(ur)) return NULL;
    return undo_cmd_name(ur->commands[ur->current].type);
}

// ============================================================================
// Clear all history
// ============================================================================

static inline void undo_redo_clear(undo_redo_t *ur) {
    for (int i = 0; i < ur->count; i++) {
        undo_command_free(&ur->commands[i]);
    }
    ur->count = 0;
    ur->current = 0;
}

#endif // UNDO_REDO_H
