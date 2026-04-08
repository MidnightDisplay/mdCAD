//------------------------------------------------------------------------------
// ecs_scene.h - High-level scene API for ECS entity management (header-only)
//
// Provides a simple API for creating and managing ECS entities with automatic
// instance buffer slot allocation and GPU rendering integration.
//------------------------------------------------------------------------------
#ifndef ECS_SCENE_H
#define ECS_SCENE_H

#include "ecs_world.h"
#include "../math/cglm_entry.h"
#include "../gpu/geometry_batch.h"
#include "../gpu/pick_buffer.h"
#include "../components/geometry_comp.h"
#include "../components/transform_comp.h"
#include "../components/renderable_comp.h"
#include "../components/selectable_comp.h"
#include "../components/constraint_comp.h"
#include "../components/constraint_participant_comp.h"
#include "../components/endpoints_comp.h"
#include "../constraints/constraint_types.h"
#include "../scripting/sketch_script_contract.h"
#include "../components/script_identity_comp.h"
#include "../undo_redo.h"
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <assert.h>
#include <stdlib.h>

// For theme-aware hover colors (cimgui already defined in app.c before this include)
#ifndef CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#endif
#include "cimgui.h"

//------------------------------------------------------------------------------
// Configuration
//------------------------------------------------------------------------------

#define ECS_SCENE_ARC_SEGMENTS_PER_RAD 8   // Arc tessellation density
#define ECS_SCENE_BEZIER_DEFAULT_SEGMENTS 16
#define ECS_SCENE_HELIX_DEFAULT_SEGMENTS 32
#define ECS_SCENE_SOLVER_MAX_IMPLICATED_CONSTRAINTS 32
#define ECS_SCENE_SOLVER_MAX_IMPLICATED_PARTICIPANTS 128
#define ECS_SCENE_SOLVER_DRAG_MAX_DELTA_PER_FRAME 0.25f
#define ECS_SCENE_SCRIPT_IO_MAX_SKETCHES 64
#define ECS_SCENE_SCRIPT_IO_MAX_INPUTS 32
#define ECS_SCENE_SCRIPT_IO_MAX_OUTPUTS 32
#define ECS_SCENE_SCRIPT_TEXT_BUFFER_SIZE 16384

//------------------------------------------------------------------------------
// Tessellation Helpers
//------------------------------------------------------------------------------

// Calculate number of segments needed for an arc
static inline int ecs_scene_arc_segment_count(float start_angle, float end_angle) {
    float angle_span = fabsf(end_angle - start_angle);
    int segments = (int)(angle_span * ECS_SCENE_ARC_SEGMENTS_PER_RAD) + 1;
    if (segments < 2) segments = 2;
    if (segments > 128) segments = 128;
    return segments;
}

// Tessellate arc to points array (caller must free)
// Returns allocated points array and sets point_count
static inline vec3_t* ecs_scene_tessellate_arc(
    vec3_t center, float radius, float start_angle, float end_angle, vec3_t normal,
    int *point_count
) {
    int segments = ecs_scene_arc_segment_count(start_angle, end_angle);
    *point_count = segments + 1;

    vec3_t *points = (vec3_t*)malloc((*point_count) * sizeof(vec3_t));
    if (!points) return NULL;

    // Compute local coordinate system on the arc plane
    // normal is the Z axis, we need X and Y axes in the plane
    vec3_t up = normal;
    vec3_t arbitrary = (fabsf(up.y) < 0.9f) ? vec3_make(0, 1, 0) : vec3_make(1, 0, 0);
    vec3_t x_axis = vec3_normalize(vec3_cross(arbitrary, up));
    vec3_t y_axis = vec3_cross(up, x_axis);

    for (int i = 0; i <= segments; i++) {
        float t = (float)i / (float)segments;
        float angle = start_angle + t * (end_angle - start_angle);

        float cx = cosf(angle) * radius;
        float cy = sinf(angle) * radius;

        points[i] = vec3_add(center,
                             vec3_add(vec3_scale(x_axis, cx),
                                      vec3_scale(y_axis, cy)));
    }

    return points;
}

// Tessellate cubic bezier to points array (caller must free)
static inline vec3_t* ecs_scene_tessellate_bezier(
    vec3_t p0, vec3_t p1, vec3_t p2, vec3_t p3,
    int segments, int *point_count
) {
    if (segments < 2) segments = 2;
    *point_count = segments + 1;

    vec3_t *points = (vec3_t*)malloc((*point_count) * sizeof(vec3_t));
    if (!points) return NULL;

    for (int i = 0; i <= segments; i++) {
        float t = (float)i / (float)segments;
        float t2 = t * t;
        float t3 = t2 * t;
        float mt = 1.0f - t;
        float mt2 = mt * mt;
        float mt3 = mt2 * mt;

        // Cubic bezier: B(t) = (1-t)³P0 + 3(1-t)²tP1 + 3(1-t)t²P2 + t³P3
        vec3_t pt;
        pt.x = mt3 * p0.x + 3.0f * mt2 * t * p1.x + 3.0f * mt * t2 * p2.x + t3 * p3.x;
        pt.y = mt3 * p0.y + 3.0f * mt2 * t * p1.y + 3.0f * mt * t2 * p2.y + t3 * p3.y;
        pt.z = mt3 * p0.z + 3.0f * mt2 * t * p1.z + 3.0f * mt * t2 * p2.z + t3 * p3.z;
        points[i] = pt;
    }

    return points;
}

// Tessellate helix to points array (caller must free)
static inline vec3_t* ecs_scene_tessellate_helix(
    vec3_t axis_start, vec3_t axis_end,
    float radius, float turns, int segments,
    int *point_count
) {
    if (segments < 4) segments = 4;
    *point_count = segments + 1;

    vec3_t *points = (vec3_t*)malloc((*point_count) * sizeof(vec3_t));
    if (!points) return NULL;

    // Axis direction
    vec3_t axis = vec3_sub(axis_end, axis_start);
    float axis_length = vec3_length(axis);
    vec3_t axis_dir = (axis_length > 0.0001f) ? vec3_scale(axis, 1.0f / axis_length) : vec3_make(0, 1, 0);

    // Create perpendicular vectors
    vec3_t arbitrary = (fabsf(axis_dir.y) < 0.9f) ? vec3_make(0, 1, 0) : vec3_make(1, 0, 0);
    vec3_t x_axis = vec3_normalize(vec3_cross(arbitrary, axis_dir));
    vec3_t y_axis = vec3_cross(axis_dir, x_axis);

    for (int i = 0; i <= segments; i++) {
        float t = (float)i / (float)segments;
        float angle = t * turns * 2.0f * 3.14159265359f;

        // Position along axis
        vec3_t axis_pos = vec3_add(axis_start, vec3_scale(axis, t));

        // Radial offset
        float cx = cosf(angle) * radius;
        float cy = sinf(angle) * radius;
        vec3_t radial = vec3_add(vec3_scale(x_axis, cx), vec3_scale(y_axis, cy));

        points[i] = vec3_add(axis_pos, radial);
    }

    return points;
}

//------------------------------------------------------------------------------
// Types
//------------------------------------------------------------------------------

// Complete ECS scene state
typedef struct {
    bool active;
    ecs_entity_t sketch;
    ecs_entity_t first_constraint;
    int implicated_constraint_count;
    ecs_entity_t implicated_constraints[ECS_SCENE_SOLVER_MAX_IMPLICATED_CONSTRAINTS];
    int participant_count;
    ecs_entity_t participants[ECS_SCENE_SOLVER_MAX_IMPLICATED_PARTICIPANTS];
    char reason[192];
} scene_solver_failure_implication_t;

typedef enum {
    SCENE_SOLVER_DRAG_FEASIBLE = 0,
    SCENE_SOLVER_DRAG_UNSATISFIABLE = 1,
    SCENE_SOLVER_DRAG_INVALID = 2
} scene_solver_drag_result_t;

typedef struct {
    scene_solver_drag_result_t result;
    vec3_t projected_delta;
    ecs_entity_t first_implicated_constraint;
    int implicated_constraint_count;
    ecs_entity_t implicated_constraints[ECS_SCENE_SOLVER_MAX_IMPLICATED_CONSTRAINTS];
    char block_reason[192];
} scene_solver_drag_decision_t;

typedef struct {
    sketch_solver_diagnostic_severity_t severity;
    char timestamp[SKETCH_SOLVER_DIAGNOSTIC_TIMESTAMP_MAX];
    char message[SKETCH_SOLVER_DIAGNOSTIC_MESSAGE_MAX];
    ecs_entity_t implicated_constraint;
} scene_solver_drag_diagnostic_event_t;

typedef struct {
    char id[SCRIPT_LOCAL_ID_MAX];
    double value;
    bool has_min;
    double min_value;
    bool has_max;
    double max_value;
    bool has_step;
    double step_value;
} scene_script_io_entry_t;

typedef struct {
    bool occupied;
    ecs_entity_t sketch;
    uint32_t input_count;
    uint32_t output_count;
    scene_script_io_entry_t inputs[ECS_SCENE_SCRIPT_IO_MAX_INPUTS];
    scene_script_io_entry_t outputs[ECS_SCENE_SCRIPT_IO_MAX_OUTPUTS];
} scene_script_io_state_t;

typedef struct {
    char id[SCRIPT_LOCAL_ID_MAX];
    bool is_input;
    double value;
    bool has_min;
    double min_value;
    bool has_max;
    double max_value;
    bool has_step;
    double step_value;
} scene_script_io_descriptor_t;

typedef struct {
    ecs_world_state_t *world;           // Pointer to ECS world
    geometry_batch_manager_t batches;   // GPU batch manager

    // Visibility flag for ECS rendering
    bool visible;
    uint32_t next_sketch_name_index;
    uint32_t solver_backend_id;
    const char *solver_backend_name;
    uint64_t script_emit_revision;
    undo_redo_t *script_undo_redo;
    bool script_apply_undo_suppressed;
    scene_script_io_state_t script_io_states[ECS_SCENE_SCRIPT_IO_MAX_SKETCHES];
    scene_solver_failure_implication_t solver_failure_implication;
} ecs_scene_t;

//------------------------------------------------------------------------------
// Forward declarations
//------------------------------------------------------------------------------

static inline bool scene_is_sketch(ecs_scene_t *scene, ecs_entity_t e);
static inline void scene_refresh_sketch_metadata(ecs_scene_t *scene, ecs_entity_t sketch);
static inline void scene_normalize_sketch_script_local_ids(ecs_scene_t *scene, ecs_entity_t sketch);
static inline bool scene_script_emit_for_sketch(ecs_scene_t *scene,
                                                 ecs_entity_t sketch,
                                                 char *out_script,
                                                 size_t out_script_size,
                                                 sketch_script_error_t *out_error);
static inline bool scene_script_apply_commit(ecs_scene_t *scene,
                                             ecs_entity_t sketch,
                                             const char *script_text,
                                             sketch_script_error_t *out_error);
static inline bool scene_script_reemit_for_sketch(ecs_scene_t *scene, ecs_entity_t sketch);
static inline uint64_t scene_script_emit_revision(const ecs_scene_t *scene);
static inline void scene_remove_entity(ecs_scene_t *scene, ecs_entity_t e);
static inline void scene_script_bind_undo_redo(ecs_scene_t *scene, undo_redo_t *undo_redo);
static inline int scene_script_io_descriptor_count(const ecs_scene_t *scene, ecs_entity_t sketch);
static inline bool scene_script_io_descriptor_at(const ecs_scene_t *scene,
                                                 ecs_entity_t sketch,
                                                 int index,
                                                 scene_script_io_descriptor_t *out_desc);
static inline bool scene_script_io_read_value(const ecs_scene_t *scene,
                                              ecs_entity_t sketch,
                                              const char *id,
                                              double *out_value,
                                              bool *out_is_input);
static inline bool scene_script_io_apply_input_value(ecs_scene_t *scene,
                                                     ecs_entity_t sketch,
                                                     const char *id,
                                                     double value,
                                                     sketch_script_error_t *out_error);
static inline ecs_entity_t ecs_scene_find_entity_by_pick_id(ecs_scene_t *scene, uint32_t pick_id);
static inline ecs_entity_t scene_add_constraint_to_sketch_with_descriptors(
    ecs_scene_t *scene,
    ecs_entity_t sketch,
    constraint_type_t type,
    const constraint_participant_descriptor_t *participants,
    uint32_t participant_count,
    float value,
    bool driven);
static inline const char* scene_solver_backend_name(const ecs_scene_t *scene);
static inline uint32_t scene_solver_backend_id(const ecs_scene_t *scene);
static inline uint64_t scene_solver_now_ms(void);
static inline float scene_solver_position_tolerance(const ecs_scene_t *scene, ecs_entity_t sketch);
static inline bool scene_solver_set_position_tolerance(ecs_scene_t *scene, ecs_entity_t sketch, float tolerance);
static inline float scene_solver_angle_tolerance(const ecs_scene_t *scene, ecs_entity_t sketch);
static inline bool scene_solver_set_angle_tolerance(ecs_scene_t *scene, ecs_entity_t sketch, float tolerance);
static inline uint32_t scene_solver_auto_solve_debounce_ms(const ecs_scene_t *scene, ecs_entity_t sketch);
static inline bool scene_solver_set_auto_solve_debounce_ms(ecs_scene_t *scene, ecs_entity_t sketch, uint32_t debounce_ms);
static inline uint32_t scene_solver_max_passes(const ecs_scene_t *scene, ecs_entity_t sketch);
static inline bool scene_solver_set_max_passes(ecs_scene_t *scene, ecs_entity_t sketch, uint32_t max_passes);
static inline bool scene_solver_set_auto_solve(ecs_scene_t *scene, ecs_entity_t sketch, bool enabled);
static inline bool scene_solver_request_auto(ecs_scene_t *scene, ecs_entity_t sketch);
static inline void scene_solver_process_auto_queue(ecs_scene_t *scene);
static inline bool scene_solver_set_drag_anchor(ecs_scene_t *scene,
                                                ecs_entity_t sketch,
                                                ecs_entity_t owner_entity,
                                                constraint_participant_role_t role,
                                                uint8_t sub_index);
static inline bool scene_solver_clear_drag_anchor(ecs_scene_t *scene, ecs_entity_t sketch);
static inline bool scene_solver_request_recalculate(ecs_scene_t *scene, ecs_entity_t sketch);
static inline bool scene_solver_add_diagnostic(ecs_scene_t *scene, ecs_entity_t sketch,
                                               sketch_solver_diagnostic_severity_t severity,
                                               const char *timestamp,
                                               const char *message,
                                               ecs_entity_t implicated_constraint);
static inline bool scene_solver_clear_diagnostics(ecs_scene_t *scene, ecs_entity_t sketch);
static inline int scene_solver_diagnostic_count(ecs_scene_t *scene, ecs_entity_t sketch);
static inline const sketch_solver_diagnostic_t* scene_solver_diagnostic_at(ecs_scene_t *scene,
                                                                           ecs_entity_t sketch,
                                                                           int index);
static inline bool scene_solver_apply_status(ecs_scene_t *scene, ecs_entity_t sketch, sketch_status_t status);
static inline const char* scene_solver_diagnostic_level_name(sketch_solver_diagnostic_severity_t severity);
static inline bool scene_solver_set_failure_implication(ecs_scene_t *scene,
                                                        ecs_entity_t sketch,
                                                        const ecs_entity_t *implicated_constraints,
                                                        int implicated_constraint_count,
                                                        const char *reason);
static inline void scene_solver_clear_failure_implication(ecs_scene_t *scene,
                                                          ecs_entity_t sketch,
                                                          bool clear_on_success);
static inline const scene_solver_failure_implication_t* scene_solver_failure_implication(const ecs_scene_t *scene);
static inline bool scene_solver_can_apply_drag(ecs_scene_t *scene,
                                              ecs_entity_t sketch,
                                              const ecs_entity_t *drag_entities,
                                              int drag_entity_count,
                                              vec3_t requested_delta,
                                              scene_solver_drag_decision_t *out_decision);
static inline bool scene_solver_drag_make_rejected_diagnostic(const scene_solver_drag_decision_t *decision,
                                                             scene_solver_drag_diagnostic_event_t *out_event);
static inline bool scene_entity_participant_subpoint(const GeometryComp *g,
                                                     constraint_participant_role_t role,
                                                     vec3_t *out_local_point,
                                                     uint8_t *out_sub_index);
static inline bool scene_apply_local_point_to_participant(GeometryComp *g,
                                                          constraint_participant_role_t role,
                                                          vec3_t local_point);
static inline vec3_t scene_world_delta_to_local(const mat4_t *world_matrix, vec3_t world_delta);
static inline bool scene_apply_endpoint_point_world_delta(ecs_scene_t *scene,
                                                          ecs_entity_t endpoint_entity,
                                                          vec3_t world_delta);
static inline bool scene_sync_owner_geometry_from_endpoint_entity(ecs_scene_t *scene,
                                                                  ecs_entity_t endpoint_entity);
static inline bool scene_apply_standalone_sketch_point_world_delta(ecs_scene_t *scene,
                                                                    ecs_entity_t point_entity,
                                                                    vec3_t world_delta);
static inline bool scene_update_arc_renderable_slots(ecs_scene_t *scene, ecs_entity_t entity, RenderableComp *r, const GeometryComp *g);
static inline bool scene_apply_transform_delta_for_selection(ecs_scene_t *scene,
                                                             const ecs_entity_t *entities,
                                                             int entity_count,
                                                             vec3_t world_delta);
static inline ecs_entity_t scene_find_parent_sketch(ecs_scene_t *scene, ecs_entity_t entity);
static inline bool scene_endpoint_owner_entity_for_pick(ecs_scene_t *scene,
                                                         uint32_t pick_id,
                                                         ecs_entity_t *out_owner_entity,
                                                         constraint_participant_role_t *out_role,
                                                         uint8_t *out_sub_index);
static inline void scene_sync_endpoint_entities_for_owner(ecs_scene_t *scene, ecs_entity_t owner_entity);

typedef struct {
    ecs_entity_t entity;
    geometry_type_t geometry_type;
    constraint_participant_role_t role;
    uint8_t sub_index;
    vec3_t point;
    bool has_candidate;
    bool fixed;
} scene_solver_point_candidate_t;

typedef struct {
    ecs_entity_t entity;
    vec3_t normal;
    bool fixed;
} scene_solver_arc_normal_candidate_t;

static inline int scene_solver_compare_entity_asc(const void *lhs, const void *rhs) {
    const ecs_entity_t a = *(const ecs_entity_t *)lhs;
    const ecs_entity_t b = *(const ecs_entity_t *)rhs;
    if (a < b) return -1;
    if (a > b) return 1;
    return 0;
}

static inline int scene_solver_find_point_candidate(scene_solver_point_candidate_t *entries,
                                                    int entry_count,
                                                    ecs_entity_t entity,
                                                    constraint_participant_role_t role) {
    if (!entries || entry_count <= 0 || entity == 0) return -1;
    for (int i = 0; i < entry_count; i++) {
        if (entries[i].entity == entity && entries[i].role == role) return i;
    }
    return -1;
}

static inline int scene_solver_ensure_point_candidate(ecs_scene_t *scene,
                                                      scene_solver_point_candidate_t *entries,
                                                      int max_entries,
                                                      int *io_entry_count,
                                                      ecs_entity_t entity,
                                                      constraint_participant_role_t role) {
    if (!scene || !entries || !io_entry_count || entity == 0) return -1;
    int existing = scene_solver_find_point_candidate(entries, *io_entry_count, entity, role);
    if (existing >= 0) return existing;
    if (*io_entry_count >= max_entries) return -1;

    GeometryComp *g = ecs_world_get_geometry(scene->world, entity);
    if (!g) return -1;

    if (g->type == GEOM_POINT) {
        role = CONSTRAINT_PARTICIPANT_ROLE_ENTITY;
    } else if (g->type == GEOM_LINE) {
        if (role != CONSTRAINT_PARTICIPANT_ROLE_POINT_A && role != CONSTRAINT_PARTICIPANT_ROLE_POINT_B) {
            return -1;
        }
    } else if (g->type == GEOM_ARC) {
        if (role != CONSTRAINT_PARTICIPANT_ROLE_POINT_A &&
            role != CONSTRAINT_PARTICIPANT_ROLE_POINT_B &&
            role != CONSTRAINT_PARTICIPANT_ROLE_CENTER) {
            return -1;
        }
    } else {
        return -1;
    }

    SketchGeometryStateComp *state = ecs_world_get_sketch_geometry_state(scene->world, entity);

    int index = (*io_entry_count)++;
    entries[index].entity = entity;
    entries[index].geometry_type = g->type;
    entries[index].role = role;
    if (role == CONSTRAINT_PARTICIPANT_ROLE_POINT_B) {
        entries[index].sub_index = 1;
    } else if (role == CONSTRAINT_PARTICIPANT_ROLE_CENTER) {
        entries[index].sub_index = 2;
    } else {
        entries[index].sub_index = 0;
    }
    if (g->type == GEOM_POINT) {
        entries[index].point = g->data.point.point;
    } else if (g->type == GEOM_ARC && role == CONSTRAINT_PARTICIPANT_ROLE_CENTER) {
        entries[index].point = g->data.arc.center;
    } else {
        if (!scene_entity_participant_subpoint(g, role, &entries[index].point, &entries[index].sub_index)) {
            (*io_entry_count)--;
            return -1;
        }
    }
    entries[index].has_candidate = true;
    entries[index].fixed = state ? state->fixed : false;
    return index;
}

static inline int scene_solver_find_arc_normal_candidate(scene_solver_arc_normal_candidate_t *entries,
                                                         int entry_count,
                                                         ecs_entity_t entity) {
    if (!entries || entry_count <= 0 || entity == 0) return -1;
    for (int i = 0; i < entry_count; i++) {
        if (entries[i].entity == entity) return i;
    }
    return -1;
}

static inline int scene_solver_ensure_arc_normal_candidate(ecs_scene_t *scene,
                                                           scene_solver_arc_normal_candidate_t *entries,
                                                           int max_entries,
                                                           int *io_entry_count,
                                                           ecs_entity_t entity) {
    if (!scene || !entries || !io_entry_count || entity == 0) return -1;
    int existing = scene_solver_find_arc_normal_candidate(entries, *io_entry_count, entity);
    if (existing >= 0) return existing;
    if (*io_entry_count >= max_entries) return -1;

    GeometryComp *g = ecs_world_get_geometry(scene->world, entity);
    if (!g || g->type != GEOM_ARC) return -1;
    SketchGeometryStateComp *state = ecs_world_get_sketch_geometry_state(scene->world, entity);

    int index = (*io_entry_count)++;
    entries[index].entity = entity;
    entries[index].normal = vec3_normalize(g->data.arc.normal);
    entries[index].fixed = state ? state->fixed : false;
    return index;
}

static inline bool scene_solver_extract_line_direction_from_constraint(ecs_scene_t *scene,
                                                                       scene_solver_point_candidate_t *candidates,
                                                                       int max_candidates,
                                                                       int *io_candidate_count,
                                                                       const constraint_participant_descriptor_t *descriptor,
                                                                       vec3_t *out_direction,
                                                                       bool *out_line_fixed) {
    if (!scene || !candidates || !io_candidate_count || !descriptor || !out_direction || !out_line_fixed) {
        return false;
    }
    ecs_entity_t line_entity = (ecs_entity_t)descriptor->entity;
    GeometryComp *line_geom = ecs_world_get_geometry(scene->world, line_entity);
    if (!line_geom || line_geom->type != GEOM_LINE) return false;

    int ia = scene_solver_ensure_point_candidate(scene, candidates,
                                                 max_candidates, io_candidate_count,
                                                 line_entity, CONSTRAINT_PARTICIPANT_ROLE_POINT_A);
    int ib = scene_solver_ensure_point_candidate(scene, candidates,
                                                 max_candidates, io_candidate_count,
                                                 line_entity, CONSTRAINT_PARTICIPANT_ROLE_POINT_B);
    if (ia < 0 || ib < 0) return false;

    vec3_t delta = vec3_sub(candidates[ib].point, candidates[ia].point);
    float len = vec3_length(delta);
    if (len <= 1e-6f || !isfinite(len)) return false;
    *out_direction = vec3_scale(delta, 1.0f / len);
    *out_line_fixed = candidates[ia].fixed && candidates[ib].fixed;
    return true;
}

static inline bool scene_solver_try_set_arc_normal(scene_solver_arc_normal_candidate_t *entry,
                                                   vec3_t target_normal,
                                                   float angle_tolerance,
                                                   float *io_pass_max_angle_delta,
                                                   float *out_residual) {
    if (!entry) return false;
    vec3_t normalized = vec3_normalize(target_normal);
    float len = vec3_length(normalized);
    if (len <= 1e-6f || !isfinite(len)) return false;

    float dot = vec3_dot(entry->normal, normalized);
    if (dot > 1.0f) dot = 1.0f;
    if (dot < -1.0f) dot = -1.0f;
    float residual = acosf(dot);
    if (out_residual) *out_residual = residual;
    if (io_pass_max_angle_delta && residual > *io_pass_max_angle_delta) {
        *io_pass_max_angle_delta = residual;
    }
    if (residual <= angle_tolerance) return true;
    entry->normal = normalized;
    return true;
}

static inline bool scene_solver_arc_tangent_direction(const scene_solver_point_candidate_t *arc_center,
                                                      const scene_solver_point_candidate_t *arc_endpoint,
                                                      const scene_solver_arc_normal_candidate_t *arc_normal,
                                                      vec3_t *out_tangent) {
    if (!arc_center || !arc_endpoint || !arc_normal || !out_tangent) return false;
    vec3_t radial = vec3_sub(arc_endpoint->point, arc_center->point);
    float radial_len = vec3_length(radial);
    if (radial_len <= 1e-6f || !isfinite(radial_len)) return false;
    radial = vec3_scale(radial, 1.0f / radial_len);
    vec3_t tangent = vec3_cross(arc_normal->normal, radial);
    float tangent_len = vec3_length(tangent);
    if (tangent_len <= 1e-6f || !isfinite(tangent_len)) return false;
    *out_tangent = vec3_scale(tangent, 1.0f / tangent_len);
    return true;
}

static inline bool endpoint_pick_is_encoded(uint32_t pick_id) {
    return pick_id >= ENDPOINT_PICK_BASE && pick_id <= ENDPOINT_PICK_END;
}

static inline bool endpoint_pick_encode(uint32_t entity_pick_id,
                                        constraint_participant_role_t role,
                                        uint32_t *out_pick_id) {
    if (!out_pick_id) return false;
    if (entity_pick_id == 0 || entity_pick_id >= CONSTRAINT_GLYPH_PICK_BASE) return false;
    if (role != CONSTRAINT_PARTICIPANT_ROLE_POINT_A && role != CONSTRAINT_PARTICIPANT_ROLE_POINT_B) return false;

    uint32_t slot = entity_pick_id - 1u;
    if (slot >= ENDPOINT_PICK_MAX_ENTITIES) return false;

    uint32_t role_index = (role == CONSTRAINT_PARTICIPANT_ROLE_POINT_A) ? 0u : 1u;
    *out_pick_id = ENDPOINT_PICK_BASE + (slot * ENDPOINT_PICK_ROLE_COUNT) + role_index;
    return true;
}

static inline bool endpoint_pick_decode(uint32_t pick_id,
                                        uint32_t *out_entity_pick_id,
                                        constraint_participant_role_t *out_role) {
    if (!endpoint_pick_is_encoded(pick_id)) return false;
    uint32_t encoded = pick_id - ENDPOINT_PICK_BASE;
    uint32_t slot = encoded / ENDPOINT_PICK_ROLE_COUNT;
    uint32_t role_index = encoded % ENDPOINT_PICK_ROLE_COUNT;
    if (slot >= ENDPOINT_PICK_MAX_ENTITIES) return false;

    if (out_entity_pick_id) {
        *out_entity_pick_id = slot + 1u;
    }
    if (out_role) {
        *out_role = (role_index == 0u)
            ? CONSTRAINT_PARTICIPANT_ROLE_POINT_A
            : CONSTRAINT_PARTICIPANT_ROLE_POINT_B;
    }
    return true;
}

static inline ecs_entity_t scene_find_parent_sketch(ecs_scene_t *scene, ecs_entity_t entity) {
    if (!scene || entity == 0 || !ecs_is_alive(scene->world->world, entity)) return 0;
    ecs_entity_t cursor = entity;
    while (cursor != 0 && ecs_is_alive(scene->world->world, cursor)) {
        if (scene_is_sketch(scene, cursor)) return cursor;
        cursor = ecs_world_get_parent(scene->world, cursor);
    }
    return 0;
}

static inline bool scene_endpoint_owner_entity_for_pick(ecs_scene_t *scene,
                                                         uint32_t pick_id,
                                                         ecs_entity_t *out_owner_entity,
                                                         constraint_participant_role_t *out_role,
                                                         uint8_t *out_sub_index) {
    if (!scene || pick_id == 0) return false;
    ecs_entity_t endpoint_entity = ecs_scene_find_entity_by_pick_id(scene, pick_id);
    if (endpoint_entity == 0 || !ecs_is_alive(scene->world->world, endpoint_entity)) return false;

    EndPointsComp *endpoint_meta = ecs_world_get_endpoints(scene->world, endpoint_entity);
    GeometryComp *endpoint_geom = ecs_world_get_geometry(scene->world, endpoint_entity);
    if (!endpoint_meta || !endpoint_meta->is_endpoint_point || !endpoint_geom || endpoint_geom->type != GEOM_POINT) {
        return false;
    }
    if (!endpoints_comp_is_supported_role(endpoint_meta->role)) return false;

    ecs_entity_t owner = (ecs_entity_t)endpoint_meta->owner_entity;
    if (owner == 0 || !ecs_is_alive(scene->world->world, owner)) return false;

    GeometryComp *owner_geom = ecs_world_get_geometry(scene->world, owner);
    if (!owner_geom || (owner_geom->type != GEOM_LINE && owner_geom->type != GEOM_ARC)) return false;

    ecs_entity_t owner_sketch = scene_find_parent_sketch(scene, owner);
    if (!scene_is_sketch(scene, owner_sketch)) return false;

    if (out_owner_entity) *out_owner_entity = owner;
    if (out_role) *out_role = endpoint_meta->role;
    if (out_sub_index) *out_sub_index = endpoint_meta->sub_index;
    return true;
}

static inline bool scene_entity_participant_subpoint(const GeometryComp *g,
                                                     constraint_participant_role_t role,
                                                     vec3_t *out_local_point,
                                                     uint8_t *out_sub_index) {
    if (!g || !out_local_point) return false;
    if (role != CONSTRAINT_PARTICIPANT_ROLE_POINT_A && role != CONSTRAINT_PARTICIPANT_ROLE_POINT_B) return false;

    switch (g->type) {
        case GEOM_LINE:
            if (role == CONSTRAINT_PARTICIPANT_ROLE_POINT_A) {
                *out_local_point = g->data.line.a;
                if (out_sub_index) *out_sub_index = 0;
            } else {
                *out_local_point = g->data.line.b;
                if (out_sub_index) *out_sub_index = 1;
            }
            return true;
        case GEOM_ARC: {
            float angle = (role == CONSTRAINT_PARTICIPANT_ROLE_POINT_A)
                ? g->data.arc.start_angle
                : g->data.arc.end_angle;
            vec3_t center = g->data.arc.center;
            float radius = g->data.arc.radius;
            vec3_t normal = g->data.arc.normal;
            vec3_t arbitrary = (fabsf(normal.y) < 0.9f) ? vec3_make(0, 1, 0) : vec3_make(1, 0, 0);
            vec3_t x_axis = vec3_normalize(vec3_cross(arbitrary, normal));
            vec3_t y_axis = vec3_cross(normal, x_axis);
            float cx = cosf(angle) * radius;
            float cy = sinf(angle) * radius;
            *out_local_point = vec3_add(center, vec3_add(vec3_scale(x_axis, cx), vec3_scale(y_axis, cy)));
            if (out_sub_index) *out_sub_index = (role == CONSTRAINT_PARTICIPANT_ROLE_POINT_A) ? 0 : 1;
            return true;
        }
        default:
            return false;
    }
}

static inline bool scene_apply_local_point_to_participant(GeometryComp *g,
                                                          constraint_participant_role_t role,
                                                          vec3_t local_point) {
    if (!g) return false;
    if (role != CONSTRAINT_PARTICIPANT_ROLE_ENTITY &&
        role != CONSTRAINT_PARTICIPANT_ROLE_CENTER &&
        role != CONSTRAINT_PARTICIPANT_ROLE_POINT_A &&
        role != CONSTRAINT_PARTICIPANT_ROLE_POINT_B) {
        return false;
    }

    switch (g->type) {
        case GEOM_POINT:
            g->data.point.point = local_point;
            return role == CONSTRAINT_PARTICIPANT_ROLE_ENTITY;
        case GEOM_LINE:
            if (role == CONSTRAINT_PARTICIPANT_ROLE_POINT_A) {
                g->data.line.a = local_point;
                return true;
            }
            if (role == CONSTRAINT_PARTICIPANT_ROLE_POINT_B) {
                g->data.line.b = local_point;
                return true;
            }
            return false;
        case GEOM_ARC: {
            if (role == CONSTRAINT_PARTICIPANT_ROLE_CENTER) {
                g->data.arc.center = local_point;
                return true;
            }
            if (role != CONSTRAINT_PARTICIPANT_ROLE_POINT_A &&
                role != CONSTRAINT_PARTICIPANT_ROLE_POINT_B) {
                return false;
            }

            vec3_t normal = g->data.arc.normal;
            vec3_t arbitrary = (fabsf(normal.y) < 0.9f) ? vec3_make(0, 1, 0) : vec3_make(1, 0, 0);
            vec3_t x_axis = vec3_normalize(vec3_cross(arbitrary, normal));
            vec3_t y_axis = vec3_cross(normal, x_axis);
            vec3_t radial = vec3_sub(local_point, g->data.arc.center);
            float radius = vec3_length(radial);
            if (!isfinite(radius) || radius <= 1e-6f) {
                return false;
            }

            float projected_x = vec3_dot(radial, x_axis);
            float projected_y = vec3_dot(radial, y_axis);
            float angle = atan2f(projected_y, projected_x);
            if (!isfinite(angle)) return false;

            // Preserve angular branch continuity to avoid endpoint replay flipping/inverting
            // arc span when an equivalent point crosses [-pi, +pi] wrap boundaries.
            const float two_pi = 6.28318530718f;
            float reference = (role == CONSTRAINT_PARTICIPANT_ROLE_POINT_A)
                ? g->data.arc.start_angle
                : g->data.arc.end_angle;
            if (isfinite(reference)) {
                while ((angle - reference) > 3.14159265359f) angle -= two_pi;
                while ((angle - reference) < -3.14159265359f) angle += two_pi;
            }

            g->data.arc.radius = radius;
            if (role == CONSTRAINT_PARTICIPANT_ROLE_POINT_A) {
                g->data.arc.start_angle = angle;
            } else {
                g->data.arc.end_angle = angle;
            }
            return true;
        }
        default:
            return false;
    }
}

static inline vec3_t scene_world_delta_to_local(const mat4_t *world_matrix, vec3_t world_delta) {
    if (!world_matrix) return vec3_make(0.0f, 0.0f, 0.0f);
    if (!isfinite(world_delta.x) || !isfinite(world_delta.y) || !isfinite(world_delta.z)) {
        return vec3_make(0.0f, 0.0f, 0.0f);
    }

    mat4 world_cglm;
    memcpy(world_cglm, world_matrix->m, sizeof(*world_matrix));
    float det = glm_mat4_det(world_cglm);
    if (!isfinite(det) || fabsf(det) <= 1e-8f) {
        return vec3_make(0.0f, 0.0f, 0.0f);
    }

    mat4 inv_world;
    glm_mat4_inv(world_cglm, inv_world);
    vec4 delta4 = { world_delta.x, world_delta.y, world_delta.z, 0.0f };
    vec4 local4;
    glm_mat4_mulv(inv_world, delta4, local4);

    if (!isfinite(local4[0]) || !isfinite(local4[1]) || !isfinite(local4[2])) {
        return vec3_make(0.0f, 0.0f, 0.0f);
    }
    return vec3_make(local4[0], local4[1], local4[2]);
}

static inline bool scene_apply_endpoint_point_world_delta(ecs_scene_t *scene,
                                                          ecs_entity_t endpoint_entity,
                                                          vec3_t world_delta) {
    if (!scene || endpoint_entity == 0) return false;
    if (!ecs_is_alive(scene->world->world, endpoint_entity)) return false;

    EndPointsComp *endpoint_meta = ecs_world_get_endpoints(scene->world, endpoint_entity);
    GeometryComp *endpoint_geom = ecs_world_get_geometry(scene->world, endpoint_entity);
    if (!endpoint_meta || !endpoint_meta->is_endpoint_point || !endpoint_geom || endpoint_geom->type != GEOM_POINT) {
        return false;
    }

    if (!endpoints_comp_is_supported_role(endpoint_meta->role) &&
        endpoint_meta->role != CONSTRAINT_PARTICIPANT_ROLE_CENTER) {
        return false;
    }

    ecs_entity_t owner = (ecs_entity_t)endpoint_meta->owner_entity;
    if (owner == 0 || !ecs_is_alive(scene->world->world, owner)) return false;
    GeometryComp *owner_geom = ecs_world_get_geometry(scene->world, owner);
    TransformComp *owner_xform = ecs_world_get_transform(scene->world, owner);
    if (!owner_geom || !owner_xform) return false;

    ecs_entity_t sketch = scene_find_parent_sketch(scene, owner);
    if (!scene_is_sketch(scene, sketch)) return false;

    vec3_t local_delta = scene_world_delta_to_local(&owner_xform->world_matrix, world_delta);
    vec3_t source = vec3_make(0.0f, 0.0f, 0.0f);
    bool has_source = false;
    if (owner_geom->type == GEOM_ARC && endpoint_meta->role == CONSTRAINT_PARTICIPANT_ROLE_CENTER) {
        source = owner_geom->data.arc.center;
        has_source = true;
    } else {
        has_source = scene_entity_participant_subpoint(owner_geom, endpoint_meta->role, &source, NULL);
    }
    if (!has_source) return false;

    vec3_t target = vec3_add(source, local_delta);
    if (!scene_apply_local_point_to_participant(owner_geom, endpoint_meta->role, target)) return false;

    RenderableComp *owner_renderable = ecs_world_get_renderable(scene->world, owner);
    if (owner_renderable) owner_renderable->instance_dirty = true;

    scene_sync_endpoint_entities_for_owner(scene, owner);
    if (endpoint_meta->role == CONSTRAINT_PARTICIPANT_ROLE_POINT_A ||
        endpoint_meta->role == CONSTRAINT_PARTICIPANT_ROLE_POINT_B) {
        (void)scene_solver_set_drag_anchor(scene, sketch, owner, endpoint_meta->role, endpoint_meta->sub_index);
    }
    scene_solver_request_auto(scene, sketch);
    scene_script_reemit_for_sketch(scene, sketch);
    return true;
}

static inline bool scene_sync_owner_geometry_from_endpoint_entity(ecs_scene_t *scene,
                                                                  ecs_entity_t endpoint_entity) {
    if (!scene || endpoint_entity == 0) return false;
    if (!ecs_is_alive(scene->world->world, endpoint_entity)) return false;

    EndPointsComp *endpoint_meta = ecs_world_get_endpoints(scene->world, endpoint_entity);
    GeometryComp *endpoint_geom = ecs_world_get_geometry(scene->world, endpoint_entity);
    if (!endpoint_meta || !endpoint_meta->is_endpoint_point || !endpoint_geom || endpoint_geom->type != GEOM_POINT) {
        return false;
    }

    if (!endpoints_comp_is_supported_role(endpoint_meta->role)) return false;

    ecs_entity_t owner = (ecs_entity_t)endpoint_meta->owner_entity;
    if (owner == 0 || !ecs_is_alive(scene->world->world, owner)) return false;

    GeometryComp *owner_geom = ecs_world_get_geometry(scene->world, owner);
    if (!owner_geom) return false;

    ecs_entity_t sketch = scene_find_parent_sketch(scene, owner);
    if (!scene_is_sketch(scene, sketch)) return false;

    vec3_t local_point = endpoint_geom->data.point.point;
    if (!scene_apply_local_point_to_participant(owner_geom, endpoint_meta->role, local_point)) return false;

    RenderableComp *owner_renderable = ecs_world_get_renderable(scene->world, owner);
    if (owner_renderable) owner_renderable->instance_dirty = true;

    scene_sync_endpoint_entities_for_owner(scene, owner);
    if (endpoint_meta->role == CONSTRAINT_PARTICIPANT_ROLE_POINT_A ||
        endpoint_meta->role == CONSTRAINT_PARTICIPANT_ROLE_POINT_B) {
        (void)scene_solver_set_drag_anchor(scene, sketch, owner, endpoint_meta->role, endpoint_meta->sub_index);
    }
    scene_solver_request_auto(scene, sketch);
    scene_script_reemit_for_sketch(scene, sketch);
    return true;
}

static inline bool scene_apply_standalone_sketch_point_world_delta(ecs_scene_t *scene,
                                                                    ecs_entity_t point_entity,
                                                                    vec3_t world_delta) {
    if (!scene || point_entity == 0) return false;
    if (!ecs_is_alive(scene->world->world, point_entity)) return false;

    EndPointsComp *endpoint_meta = ecs_world_get_endpoints(scene->world, point_entity);
    if (endpoint_meta && endpoint_meta->is_endpoint_point) return false;

    GeometryComp *point_geom = ecs_world_get_geometry(scene->world, point_entity);
    TransformComp *point_xform = ecs_world_get_transform(scene->world, point_entity);
    if (!point_geom || point_geom->type != GEOM_POINT || !point_xform) return false;

    ecs_entity_t sketch = scene_find_parent_sketch(scene, point_entity);
    if (!scene_is_sketch(scene, sketch)) return false;

    vec3_t local_delta = scene_world_delta_to_local(&point_xform->world_matrix, world_delta);
    vec3_t next_local = vec3_add(point_geom->data.point.point, local_delta);
    if (!isfinite(next_local.x) || !isfinite(next_local.y) || !isfinite(next_local.z)) return false;

    point_geom->data.point.point = next_local;
    RenderableComp *point_renderable = ecs_world_get_renderable(scene->world, point_entity);
    if (point_renderable) point_renderable->instance_dirty = true;

    (void)scene_solver_clear_drag_anchor(scene, sketch);
    scene_solver_request_auto(scene, sketch);
    scene_script_reemit_for_sketch(scene, sketch);
    return true;
}

static inline bool scene_update_arc_renderable_slots(ecs_scene_t *scene,
                                                     ecs_entity_t entity,
                                                     RenderableComp *r,
                                                     const GeometryComp *g) {
    if (!scene || !r || !g) return false;
    if (g->type != GEOM_ARC) return false;
    if (r->instance_slot == 0xFFFFFFFF) return false;

    int point_count = 0;
    vec3_t *points = ecs_scene_tessellate_arc(g->data.arc.center, g->data.arc.radius,
                                              g->data.arc.start_angle, g->data.arc.end_angle,
                                              g->data.arc.normal, &point_count);
    if (!points || point_count < 2) {
        if (points) free(points);
        return false;
    }

    int required_segments = point_count - 1;
    int required_joins = (point_count > 2) ? (point_count - 2) : 0;
    bool needs_realloc = ((int)r->segment_count != required_segments) || ((int)r->join_count != required_joins);

    if (needs_realloc) {
        for (uint32_t i = 0; i < r->segment_count; i++) {
            geom_line_batch_free(&scene->batches.lines, (int)(r->instance_slot + i));
        }
        if (r->join_slot_start != 0xFFFFFFFF) {
            for (uint32_t i = 0; i < r->join_count; i++) {
                geom_point_batch_free(&scene->batches.points, (int)(r->join_slot_start + i));
            }
        }

        int new_segment_slot = geom_line_batch_alloc_contiguous(&scene->batches.lines, required_segments);
        if (new_segment_slot < 0) {
            free(points);
            return false;
        }

        int new_join_slot = -1;
        if (required_joins > 0) {
            new_join_slot = geom_point_batch_alloc_contiguous(&scene->batches.points, required_joins);
            if (new_join_slot < 0) {
                for (int i = 0; i < required_segments; i++) {
                    geom_line_batch_free(&scene->batches.lines, new_segment_slot + i);
                }
                free(points);
                return false;
            }
        }

        r->instance_slot = (uint32_t)new_segment_slot;
        r->segment_count = (uint32_t)required_segments;
        r->join_slot_start = (new_join_slot >= 0) ? (uint32_t)new_join_slot : 0xFFFFFFFF;
        r->join_count = (uint32_t)required_joins;
    }

    for (int i = 0; i < required_segments; i++) {
        geom_line_batch_set_entity(&scene->batches.lines, (int)(r->instance_slot + (uint32_t)i),
                                   (uint64_t)entity, (uint8_t)GEOM_ARC);
    }
    if (r->join_slot_start != 0xFFFFFFFF) {
        for (int i = 0; i < required_joins; i++) {
            geom_point_batch_set_entity(&scene->batches.points, (int)(r->join_slot_start + (uint32_t)i),
                                        (uint64_t)entity, (uint8_t)GEOM_ARC);
        }
    }

    free(points);
    r->instance_dirty = true;
    return true;
}

static inline bool scene_apply_transform_delta_for_selection(ecs_scene_t *scene,
                                                             const ecs_entity_t *entities,
                                                             int entity_count,
                                                             vec3_t world_delta) {
    if (!scene || !entities || entity_count <= 0) return false;
    bool applied_any = false;
    for (int i = 0; i < entity_count; i++) {
        ecs_entity_t e = entities[i];
        if (e == 0 || !ecs_is_alive(scene->world->world, e)) continue;

        EndPointsComp *endpoint_meta = ecs_world_get_endpoints(scene->world, e);
        bool is_endpoint_point = endpoint_meta && endpoint_meta->is_endpoint_point;
        if (is_endpoint_point) {
            if (scene_apply_endpoint_point_world_delta(scene, e, world_delta)) {
                applied_any = true;
            }
            continue;
        }

        TransformComp *t = ecs_world_get_transform(scene->world, e);
        if (t) {
            t->position = vec3_add(t->position, world_delta);
            t->dirty = true;
            ecs_world_mark_descendants_dirty(scene->world, e);
            applied_any = true;
        }
        RenderableComp *r = (RenderableComp*)ecs_world_get_renderable(scene->world, e);
        if (r) r->instance_dirty = true;
    }
    return applied_any;
}

static inline ecs_entity_t scene_constraint_participant_entity_for_pick(ecs_scene_t *scene,
                                                                         uint32_t pick_id,
                                                                         constraint_participant_role_t *out_role,
                                                                         uint8_t *out_sub_index) {
    if (!scene || pick_id == 0) return 0;
    ecs_entity_t owner_entity = 0;
    constraint_participant_role_t role = CONSTRAINT_PARTICIPANT_ROLE_UNSPECIFIED;
    uint8_t sub_index = 0;
    if (!scene_endpoint_owner_entity_for_pick(scene, pick_id, &owner_entity, &role, &sub_index)) return 0;
    if (out_role) *out_role = role;
    if (out_sub_index) *out_sub_index = sub_index;
    return owner_entity;
}

static inline void scene_solver_sort_entities_unique(ecs_entity_t *values, int *io_count) {
    if (!values || !io_count || *io_count <= 1) return;
    qsort(values, (size_t)*io_count, sizeof(ecs_entity_t), scene_solver_compare_entity_asc);
    int write_index = 0;
    for (int i = 0; i < *io_count; i++) {
        if (values[i] == 0) continue;
        if (write_index > 0 && values[write_index - 1] == values[i]) continue;
        values[write_index++] = values[i];
    }
    *io_count = write_index;
}

static inline vec3_t scene_solver_project_drag_delta_with_budget(vec3_t requested_delta) {
    const float max_delta = ECS_SCENE_SOLVER_DRAG_MAX_DELTA_PER_FRAME;
    float len = vec3_length(requested_delta);
    if (len <= max_delta || len <= 1e-7f) {
        return requested_delta;
    }
    float scale = max_delta / len;
    return vec3_scale(requested_delta, scale);
}

//------------------------------------------------------------------------------
// Scene Initialization
//------------------------------------------------------------------------------

static inline void ecs_scene_init(ecs_scene_t *scene, ecs_world_state_t *world) {
    scene->world = world;
    scene->visible = true;
    scene->next_sketch_name_index = 1;
    scene->solver_backend_id = 1;
    scene->solver_backend_name = "ConstraintSketchSolverV1";
    scene->script_emit_revision = 0;
    scene->script_undo_redo = NULL;
    scene->script_apply_undo_suppressed = false;
    memset(scene->script_io_states, 0, sizeof(scene->script_io_states));
    memset(&scene->solver_failure_implication, 0, sizeof(scene->solver_failure_implication));
    geometry_batch_manager_init(&scene->batches);
}

static inline void ecs_scene_shutdown(ecs_scene_t *scene) {
    geometry_batch_manager_shutdown(&scene->batches);
}

//------------------------------------------------------------------------------
// Transform Helpers
//------------------------------------------------------------------------------

static inline vec3_t ecs_scene_transform_point_world(const mat4_t *world_matrix, vec3_t local_point) {
    mat4 world_cglm;
    vec3 local = { local_point.x, local_point.y, local_point.z };
    vec3 world;

    memcpy(world_cglm, world_matrix->m, sizeof(*world_matrix));
    glm_mat4_mulv3(world_cglm, local, 1.0f, world);
    return (vec3_t){ world[0], world[1], world[2] };
}

//------------------------------------------------------------------------------
// Entity Creation API
//------------------------------------------------------------------------------

// Create a line entity
static inline ecs_entity_t scene_add_line(ecs_scene_t *scene,
                                           vec3_t a, vec3_t b,
                                           vec4_t color, float width) {
    // Create base entity with transform, renderable, selectable
    ecs_entity_t e = ecs_world_create_entity(scene->world);

    // Set geometry
    GeometryComp g = geometry_comp_line(a, b, color, width);
    ecs_world_set_geometry(scene->world, e, &g);

    // Allocate instance buffer slot
    int slot = geom_line_batch_alloc(&scene->batches.lines);
    if (slot >= 0) {
        // Get transform for position (identity for now)
        TransformComp *t = ecs_world_get_transform(scene->world, e);
        vec3_t world_a = ecs_scene_transform_point_world(&t->world_matrix, a);
        vec3_t world_b = ecs_scene_transform_point_world(&t->world_matrix, b);

        // Set instance data
        geom_line_batch_set(&scene->batches.lines, slot, world_a, world_b, color);

        // Set entity mapping for debug viewer
        geom_line_batch_set_entity(&scene->batches.lines, slot, (uint64_t)e, (uint8_t)GEOM_LINE);

        // Update renderable with slot info
        RenderableComp *r = ecs_world_get_renderable(scene->world, e);
        if (r) {
            r->batch_id = GEOM_LINE;
            r->instance_slot = (uint32_t)slot;
            r->instance_dirty = false;  // Already set
        }
    }

    return e;
}

// Create a polyline entity (allocates N-1 segment slots and N-2 join slots)
static inline ecs_entity_t scene_add_polyline(ecs_scene_t *scene,
                                               vec3_t *points, int point_count,
                                               vec4_t color, float width) {
    if (point_count < 2) return 0;  // Need at least 2 points

    ecs_entity_t e = ecs_world_create_entity(scene->world);

    // Create geometry component (copies points)
    GeometryComp g = geometry_comp_polyline(points, point_count, color, width);
    ecs_world_set_geometry(scene->world, e, &g);

    // Allocate CONTIGUOUS segment slots (N-1 for N points)
    int num_segments = point_count - 1;
    int first_segment_slot = geom_line_batch_alloc_contiguous(&scene->batches.lines, num_segments);

    if (first_segment_slot >= 0) {
        GeometryComp *geom = ecs_world_get_geometry(scene->world, e);
        TransformComp *t = ecs_world_get_transform(scene->world, e);
        if (geom && t) {
            for (int i = 0; i < num_segments; i++) {
                vec3_t world_a = ecs_scene_transform_point_world(&t->world_matrix, geom->data.polyline.points[i]);
                vec3_t world_b = ecs_scene_transform_point_world(&t->world_matrix, geom->data.polyline.points[i + 1]);
                geom_line_batch_set(&scene->batches.lines, first_segment_slot + i, world_a, world_b, color);
                // Set entity mapping for debug viewer
                geom_line_batch_set_entity(&scene->batches.lines, first_segment_slot + i, (uint64_t)e, (uint8_t)GEOM_POLYLINE);
            }
        }
    }

    // Allocate CONTIGUOUS join slots (N-2 for N points, at interior vertices)
    int num_joins = (point_count > 2) ? (point_count - 2) : 0;
    int first_join_slot = -1;

    if (num_joins > 0) {
        first_join_slot = geom_point_batch_alloc_contiguous(&scene->batches.points, num_joins);
        if (first_join_slot >= 0) {
            GeometryComp *geom = ecs_world_get_geometry(scene->world, e);
            TransformComp *t = ecs_world_get_transform(scene->world, e);
            if (geom && t) {
                for (int i = 0; i < num_joins; i++) {
                    // Join is at interior vertex i+1 (vertices 1 to N-2)
                    vec3_t world_pos = ecs_scene_transform_point_world(&t->world_matrix, geom->data.polyline.points[i + 1]);
                    geom_point_batch_set(&scene->batches.points, first_join_slot + i, world_pos, color);
                    // Set entity mapping for debug viewer
                    geom_point_batch_set_entity(&scene->batches.points, first_join_slot + i, (uint64_t)e, (uint8_t)GEOM_POLYLINE);
                }
            }
        }
    }

    // Update renderable with slot info
    RenderableComp *r = ecs_world_get_renderable(scene->world, e);
    if (r) {
        r->batch_id = GEOM_POLYLINE;
        r->instance_slot = (first_segment_slot >= 0) ? (uint32_t)first_segment_slot : 0xFFFFFFFF;
        r->segment_count = (uint32_t)num_segments;
        r->join_slot_start = (first_join_slot >= 0) ? (uint32_t)first_join_slot : 0xFFFFFFFF;
        r->join_count = (uint32_t)num_joins;
        r->instance_dirty = false;  // Already set
    }

    return e;
}

// Create a polygon entity (closed polyline: N segments and N joins for N points)
static inline ecs_entity_t scene_add_polygon(ecs_scene_t *scene,
                                              vec3_t *points, int point_count,
                                              vec4_t color, float width) {
    if (point_count < 3) return 0;  // Need at least 3 points for a polygon

    ecs_entity_t e = ecs_world_create_entity(scene->world);

    // Create geometry component (copies points)
    GeometryComp g = geometry_comp_polygon(points, point_count, color, width);
    ecs_world_set_geometry(scene->world, e, &g);

    // Allocate CONTIGUOUS segment slots (N for N points - includes closing segment)
    int num_segments = point_count;
    int first_segment_slot = geom_line_batch_alloc_contiguous(&scene->batches.lines, num_segments);

    if (first_segment_slot >= 0) {
        GeometryComp *geom = ecs_world_get_geometry(scene->world, e);
        TransformComp *t = ecs_world_get_transform(scene->world, e);
        if (geom && t) {
            for (int i = 0; i < num_segments; i++) {
                int next = (i + 1) % point_count;
                vec3_t world_a = ecs_scene_transform_point_world(&t->world_matrix, geom->data.polygon.points[i]);
                vec3_t world_b = ecs_scene_transform_point_world(&t->world_matrix, geom->data.polygon.points[next]);
                geom_line_batch_set(&scene->batches.lines, first_segment_slot + i, world_a, world_b, color);
                // Set entity mapping for debug viewer
                geom_line_batch_set_entity(&scene->batches.lines, first_segment_slot + i, (uint64_t)e, (uint8_t)GEOM_POLYGON);
            }
        }
    }

    // Allocate CONTIGUOUS join slots (N for N points - all vertices get joins in closed polygon)
    int num_joins = point_count;
    int first_join_slot = geom_point_batch_alloc_contiguous(&scene->batches.points, num_joins);

    if (first_join_slot >= 0) {
        GeometryComp *geom = ecs_world_get_geometry(scene->world, e);
        TransformComp *t = ecs_world_get_transform(scene->world, e);
        if (geom && t) {
            for (int i = 0; i < num_joins; i++) {
                vec3_t world_pos = ecs_scene_transform_point_world(&t->world_matrix, geom->data.polygon.points[i]);
                geom_point_batch_set(&scene->batches.points, first_join_slot + i, world_pos, color);
                // Set entity mapping for debug viewer
                geom_point_batch_set_entity(&scene->batches.points, first_join_slot + i, (uint64_t)e, (uint8_t)GEOM_POLYGON);
            }
        }
    }

    // Update renderable with slot info
    RenderableComp *r = ecs_world_get_renderable(scene->world, e);
    if (r) {
        r->batch_id = GEOM_POLYGON;
        r->instance_slot = (first_segment_slot >= 0) ? (uint32_t)first_segment_slot : 0xFFFFFFFF;
        r->segment_count = (uint32_t)num_segments;
        r->join_slot_start = (first_join_slot >= 0) ? (uint32_t)first_join_slot : 0xFFFFFFFF;
        r->join_count = (uint32_t)num_joins;
        r->instance_dirty = false;
    }

    return e;
}

// Create a point entity
// Returns 0 if allocation fails (slot buffer at capacity)
static inline ecs_entity_t scene_add_point(ecs_scene_t *scene,
                                            vec3_t pos,
                                            vec4_t color, float size) {
    // Allocate slot BEFORE creating entity so we can fail cleanly
    int slot = geom_point_batch_alloc(&scene->batches.points);
    if (slot < 0) {
        // Slot buffer at capacity - cannot create point
        return 0;
    }

    ecs_entity_t e = ecs_world_create_entity(scene->world);

    GeometryComp g = geometry_comp_point(pos, color, size);
    ecs_world_set_geometry(scene->world, e, &g);

    TransformComp *t = ecs_world_get_transform(scene->world, e);
    vec3_t world_pos = ecs_scene_transform_point_world(&t->world_matrix, pos);

    geom_point_batch_set(&scene->batches.points, slot, world_pos, color);

    // Set entity mapping for debug viewer
    geom_point_batch_set_entity(&scene->batches.points, slot, (uint64_t)e, (uint8_t)GEOM_POINT);

    RenderableComp *r = ecs_world_get_renderable(scene->world, e);
    if (r) {
        r->batch_id = GEOM_POINT;
        r->instance_slot = (uint32_t)slot;
        r->instance_dirty = false;
    }

    return e;
}

// Create an arc entity (tessellated to polyline segments)
static inline ecs_entity_t scene_add_arc(ecs_scene_t *scene,
                                          vec3_t center, float radius,
                                          float start_angle, float end_angle,
                                          vec3_t normal,
                                          vec4_t color, float width) {
    // Tessellate arc to points
    int point_count = 0;
    vec3_t *points = ecs_scene_tessellate_arc(center, radius, start_angle, end_angle, normal, &point_count);
    if (!points || point_count < 2) {
        if (points) free(points);
        return 0;
    }

    ecs_entity_t e = ecs_world_create_entity(scene->world);

    GeometryComp g = geometry_comp_arc(center, radius, start_angle, end_angle, normal, color, width);
    ecs_world_set_geometry(scene->world, e, &g);

    // Allocate CONTIGUOUS segment slots (N-1 for N points)
    int num_segments = point_count - 1;
    int first_segment_slot = geom_line_batch_alloc_contiguous(&scene->batches.lines, num_segments);

    if (first_segment_slot >= 0) {
        TransformComp *t = ecs_world_get_transform(scene->world, e);
        if (t) {
            for (int i = 0; i < num_segments; i++) {
                vec3_t world_a = ecs_scene_transform_point_world(&t->world_matrix, points[i]);
                vec3_t world_b = ecs_scene_transform_point_world(&t->world_matrix, points[i + 1]);
                geom_line_batch_set(&scene->batches.lines, first_segment_slot + i, world_a, world_b, color);
                // Set entity mapping for debug viewer
                geom_line_batch_set_entity(&scene->batches.lines, first_segment_slot + i, (uint64_t)e, (uint8_t)GEOM_ARC);
            }
        }
    }

    // Allocate CONTIGUOUS join slots (N-2 for N points)
    int num_joins = (point_count > 2) ? (point_count - 2) : 0;
    int first_join_slot = -1;

    if (num_joins > 0) {
        first_join_slot = geom_point_batch_alloc_contiguous(&scene->batches.points, num_joins);
        if (first_join_slot >= 0) {
            TransformComp *t = ecs_world_get_transform(scene->world, e);
            if (t) {
                for (int i = 0; i < num_joins; i++) {
                    vec3_t world_pos = ecs_scene_transform_point_world(&t->world_matrix, points[i + 1]);
                    geom_point_batch_set(&scene->batches.points, first_join_slot + i, world_pos, color);
                    // Set entity mapping for debug viewer
                    geom_point_batch_set_entity(&scene->batches.points, first_join_slot + i, (uint64_t)e, (uint8_t)GEOM_ARC);
                }
            }
        }
    }

    // Update renderable with slot info
    RenderableComp *r = ecs_world_get_renderable(scene->world, e);
    if (r) {
        r->batch_id = GEOM_ARC;
        r->instance_slot = (first_segment_slot >= 0) ? (uint32_t)first_segment_slot : 0xFFFFFFFF;
        r->segment_count = (uint32_t)num_segments;
        r->join_slot_start = (first_join_slot >= 0) ? (uint32_t)first_join_slot : 0xFFFFFFFF;
        r->join_count = (uint32_t)num_joins;
        r->instance_dirty = false;
    }

    free(points);
    return e;
}

// Create a bezier curve entity (tessellated to polyline segments)
static inline ecs_entity_t scene_add_bezier(ecs_scene_t *scene,
                                             vec3_t p0, vec3_t p1, vec3_t p2, vec3_t p3,
                                             int segments,
                                             vec4_t color, float width) {
    if (segments < 2) segments = ECS_SCENE_BEZIER_DEFAULT_SEGMENTS;

    // Tessellate bezier to points
    int point_count = 0;
    vec3_t *points = ecs_scene_tessellate_bezier(p0, p1, p2, p3, segments, &point_count);
    if (!points || point_count < 2) {
        if (points) free(points);
        return 0;
    }

    ecs_entity_t e = ecs_world_create_entity(scene->world);

    GeometryComp g = geometry_comp_bezier(p0, p1, p2, p3, segments, color, width);
    ecs_world_set_geometry(scene->world, e, &g);

    // Allocate CONTIGUOUS segment slots (N-1 for N points)
    int num_segments = point_count - 1;
    int first_segment_slot = geom_line_batch_alloc_contiguous(&scene->batches.lines, num_segments);

    if (first_segment_slot >= 0) {
        TransformComp *t = ecs_world_get_transform(scene->world, e);
        if (t) {
            for (int i = 0; i < num_segments; i++) {
                vec3_t world_a = ecs_scene_transform_point_world(&t->world_matrix, points[i]);
                vec3_t world_b = ecs_scene_transform_point_world(&t->world_matrix, points[i + 1]);
                geom_line_batch_set(&scene->batches.lines, first_segment_slot + i, world_a, world_b, color);
                // Set entity mapping for debug viewer
                geom_line_batch_set_entity(&scene->batches.lines, first_segment_slot + i, (uint64_t)e, (uint8_t)GEOM_BEZIER);
            }
        }
    }

    // Allocate CONTIGUOUS join slots (N-2 for N points)
    int num_joins = (point_count > 2) ? (point_count - 2) : 0;
    int first_join_slot = -1;

    if (num_joins > 0) {
        first_join_slot = geom_point_batch_alloc_contiguous(&scene->batches.points, num_joins);
        if (first_join_slot >= 0) {
            TransformComp *t = ecs_world_get_transform(scene->world, e);
            if (t) {
                for (int i = 0; i < num_joins; i++) {
                    vec3_t world_pos = ecs_scene_transform_point_world(&t->world_matrix, points[i + 1]);
                    geom_point_batch_set(&scene->batches.points, first_join_slot + i, world_pos, color);
                    // Set entity mapping for debug viewer
                    geom_point_batch_set_entity(&scene->batches.points, first_join_slot + i, (uint64_t)e, (uint8_t)GEOM_BEZIER);
                }
            }
        }
    }

    // Update renderable with slot info
    RenderableComp *r = ecs_world_get_renderable(scene->world, e);
    if (r) {
        r->batch_id = GEOM_BEZIER;
        r->instance_slot = (first_segment_slot >= 0) ? (uint32_t)first_segment_slot : 0xFFFFFFFF;
        r->segment_count = (uint32_t)num_segments;
        r->join_slot_start = (first_join_slot >= 0) ? (uint32_t)first_join_slot : 0xFFFFFFFF;
        r->join_count = (uint32_t)num_joins;
        r->instance_dirty = false;
    }

    free(points);
    return e;
}

// Create a helix entity (tessellated to polyline segments)
static inline ecs_entity_t scene_add_helix(ecs_scene_t *scene,
                                            vec3_t axis_start, vec3_t axis_end,
                                            float radius, float turns, int segments,
                                            vec4_t color, float width) {
    if (segments < 4) segments = ECS_SCENE_HELIX_DEFAULT_SEGMENTS;

    // Tessellate helix to points
    int point_count = 0;
    vec3_t *points = ecs_scene_tessellate_helix(axis_start, axis_end, radius, turns, segments, &point_count);
    if (!points || point_count < 2) {
        if (points) free(points);
        return 0;
    }

    ecs_entity_t e = ecs_world_create_entity(scene->world);

    GeometryComp g = geometry_comp_helix(axis_start, axis_end, radius, turns, segments, color, width);
    ecs_world_set_geometry(scene->world, e, &g);

    // Allocate CONTIGUOUS segment slots (N-1 for N points)
    int num_segments = point_count - 1;
    int first_segment_slot = geom_line_batch_alloc_contiguous(&scene->batches.lines, num_segments);

    if (first_segment_slot >= 0) {
        TransformComp *t = ecs_world_get_transform(scene->world, e);
        if (t) {
            for (int i = 0; i < num_segments; i++) {
                vec3_t world_a = ecs_scene_transform_point_world(&t->world_matrix, points[i]);
                vec3_t world_b = ecs_scene_transform_point_world(&t->world_matrix, points[i + 1]);
                geom_line_batch_set(&scene->batches.lines, first_segment_slot + i, world_a, world_b, color);
                // Set entity mapping for debug viewer
                geom_line_batch_set_entity(&scene->batches.lines, first_segment_slot + i, (uint64_t)e, (uint8_t)GEOM_HELIX);
            }
        }
    }

    // Allocate CONTIGUOUS join slots (N-2 for N points)
    int num_joins = (point_count > 2) ? (point_count - 2) : 0;
    int first_join_slot = -1;

    if (num_joins > 0) {
        first_join_slot = geom_point_batch_alloc_contiguous(&scene->batches.points, num_joins);
        if (first_join_slot >= 0) {
            TransformComp *t = ecs_world_get_transform(scene->world, e);
            if (t) {
                for (int i = 0; i < num_joins; i++) {
                    vec3_t world_pos = ecs_scene_transform_point_world(&t->world_matrix, points[i + 1]);
                    geom_point_batch_set(&scene->batches.points, first_join_slot + i, world_pos, color);
                    // Set entity mapping for debug viewer
                    geom_point_batch_set_entity(&scene->batches.points, first_join_slot + i, (uint64_t)e, (uint8_t)GEOM_HELIX);
                }
            }
        }
    }

    // Update renderable with slot info
    RenderableComp *r = ecs_world_get_renderable(scene->world, e);
    if (r) {
        r->batch_id = GEOM_HELIX;
        r->instance_slot = (first_segment_slot >= 0) ? (uint32_t)first_segment_slot : 0xFFFFFFFF;
        r->segment_count = (uint32_t)num_segments;
        r->join_slot_start = (first_join_slot >= 0) ? (uint32_t)first_join_slot : 0xFFFFFFFF;
        r->join_count = (uint32_t)num_joins;
        r->instance_dirty = false;
    }

    free(points);
    return e;
}

// Create a point cloud entity (single entity with many points)
// Returns 0 if allocation fails (slot buffer at capacity)
static inline ecs_entity_t scene_add_point_cloud(ecs_scene_t *scene,
                                                  vec3_t *points, vec4_t *colors, int count,
                                                  vec4_t uniform_color, float point_size) {
    if (count < 1) return 0;  // Need at least 1 point

    // Allocate CONTIGUOUS point slots for all points in the cloud
    // Do this BEFORE creating entity so we can fail cleanly
    int first_slot = geom_point_cloud_batch_alloc(&scene->batches.points, count);
    if (first_slot < 0) {
        // Slot buffer at capacity - cannot create point cloud
        return 0;
    }

    ecs_entity_t e = ecs_world_create_entity(scene->world);

    // Create geometry component (copies points and colors)
    GeometryComp g = geometry_comp_point_cloud(points, colors, count, uniform_color, point_size);
    ecs_world_set_geometry(scene->world, e, &g);

    TransformComp *t = ecs_world_get_transform(scene->world, e);
    GeometryComp *geom = ecs_world_get_geometry(scene->world, e);
    if (t && geom) {
        // Set instance data for each point
        for (int i = 0; i < count; i++) {
            vec3_t world_pos = ecs_scene_transform_point_world(&t->world_matrix, geom->data.point_cloud.points[i]);
            vec4_t color = geom->data.point_cloud.colors ? geom->data.point_cloud.colors[i] : uniform_color;
            geom_point_batch_set(&scene->batches.points, first_slot + i, world_pos, color);
        }
        // Set entity mapping for debug viewer
        geom_point_cloud_batch_set_entity(&scene->batches.points, first_slot, count, (uint64_t)e);
    }

    // Update renderable with slot info
    RenderableComp *r = ecs_world_get_renderable(scene->world, e);
    if (r) {
        r->batch_id = GEOM_POINT_CLOUD;
        r->instance_slot = (uint32_t)first_slot;
        r->segment_count = (uint32_t)count;  // Reuse segment_count for point count
        r->join_slot_start = 0xFFFFFFFF;     // Not used for point clouds
        r->join_count = 0;
        r->instance_dirty = false;
    }

    return e;
}

// Create a triangle entity
static inline ecs_entity_t scene_add_triangle(ecs_scene_t *scene,
                                                vec3_t a, vec3_t b, vec3_t c,
                                                vec4_t color) {
    // Allocate slot BEFORE creating entity so we can fail cleanly
    int slot = geom_triangle_batch_alloc(&scene->batches.triangles);
    if (slot < 0) return 0;

    ecs_entity_t e = ecs_world_create_entity(scene->world);

    GeometryComp g = geometry_comp_triangle(a, b, c, color);
    ecs_world_set_geometry(scene->world, e, &g);

    TransformComp *t = ecs_world_get_transform(scene->world, e);
    vec3_t world_a = ecs_scene_transform_point_world(&t->world_matrix, a);
    vec3_t world_b = ecs_scene_transform_point_world(&t->world_matrix, b);
    vec3_t world_c = ecs_scene_transform_point_world(&t->world_matrix, c);
    vec3_t normal = geom_triangle_compute_normal(world_a, world_b, world_c);

    geom_triangle_batch_set(&scene->batches.triangles, slot,
                            world_a, world_b, world_c, normal, color);
    geom_triangle_batch_set_entity(&scene->batches.triangles, slot,
                                   (uint64_t)e, (uint8_t)GEOM_TRIANGLE);

    RenderableComp *r = ecs_world_get_renderable(scene->world, e);
    if (r) {
        r->batch_id = GEOM_TRIANGLE;
        r->instance_slot = (uint32_t)slot;
        r->instance_dirty = false;
    }

    return e;
}

// Create a triangle entity with per-vertex colors
static inline ecs_entity_t scene_add_triangle_colored(ecs_scene_t *scene,
                                                        vec3_t a, vec3_t b, vec3_t c,
                                                        vec4_t color_a, vec4_t color_b, vec4_t color_c) {
    // Allocate slot BEFORE creating entity so we can fail cleanly
    int slot = geom_triangle_batch_alloc(&scene->batches.triangles);
    if (slot < 0) return 0;

    ecs_entity_t e = ecs_world_create_entity(scene->world);

    GeometryComp g = geometry_comp_triangle_colored(a, b, c, color_a, color_b, color_c);
    ecs_world_set_geometry(scene->world, e, &g);

    TransformComp *t = ecs_world_get_transform(scene->world, e);
    vec3_t world_a = ecs_scene_transform_point_world(&t->world_matrix, a);
    vec3_t world_b = ecs_scene_transform_point_world(&t->world_matrix, b);
    vec3_t world_c = ecs_scene_transform_point_world(&t->world_matrix, c);
    vec3_t normal = geom_triangle_compute_normal(world_a, world_b, world_c);

    geom_triangle_batch_set_colored(&scene->batches.triangles, slot,
                                     world_a, world_b, world_c, normal,
                                     color_a, color_b, color_c);
    geom_triangle_batch_set_entity(&scene->batches.triangles, slot,
                                   (uint64_t)e, (uint8_t)GEOM_TRIANGLE);

    RenderableComp *r = ecs_world_get_renderable(scene->world, e);
    if (r) {
        r->batch_id = GEOM_TRIANGLE;
        r->instance_slot = (uint32_t)slot;
        r->instance_dirty = false;
    }

    return e;
}

// Create an indexed mesh entity (multiple triangles sharing vertices)
static inline ecs_entity_t scene_add_mesh(ecs_scene_t *scene,
                                            vec3_t *vertices, int vertex_count,
                                            uint32_t *indices, int index_count,
                                            vec4_t color) {
    if (vertex_count < 3 || index_count < 3) return 0;
    int face_count = index_count / 3;

    // Allocate contiguous triangle batch slots BEFORE creating entity
    int first_slot = geom_triangle_batch_alloc_contiguous(&scene->batches.triangles, face_count);
    if (first_slot < 0) return 0;

    ecs_entity_t e = ecs_world_create_entity(scene->world);

    // Create geometry component (copies data, computes normals)
    GeometryComp g = geometry_comp_mesh(vertices, vertex_count, indices, index_count, NULL, color);
    ecs_world_set_geometry(scene->world, e, &g);

    // Fill triangle batch slots from indexed mesh
    TransformComp *t = ecs_world_get_transform(scene->world, e);
    GeometryComp *geom = ecs_world_get_geometry(scene->world, e);
    if (t && geom) {
        for (int f = 0; f < face_count; f++) {
            uint32_t i0 = geom->data.mesh.indices[f * 3 + 0];
            uint32_t i1 = geom->data.mesh.indices[f * 3 + 1];
            uint32_t i2 = geom->data.mesh.indices[f * 3 + 2];

            vec3_t wa = ecs_scene_transform_point_world(&t->world_matrix, geom->data.mesh.vertices[i0]);
            vec3_t wb = ecs_scene_transform_point_world(&t->world_matrix, geom->data.mesh.vertices[i1]);
            vec3_t wc = ecs_scene_transform_point_world(&t->world_matrix, geom->data.mesh.vertices[i2]);
            vec3_t normal = geom_triangle_compute_normal(wa, wb, wc);

            geom_triangle_batch_set(&scene->batches.triangles, first_slot + f,
                                    wa, wb, wc, normal, color);
            geom_triangle_batch_set_entity(&scene->batches.triangles, first_slot + f,
                                           (uint64_t)e, (uint8_t)GEOM_MESH);
        }
    }

    RenderableComp *r = ecs_world_get_renderable(scene->world, e);
    if (r) {
        r->batch_id = GEOM_MESH;
        r->instance_slot = (uint32_t)first_slot;
        r->segment_count = (uint32_t)face_count;
        r->join_slot_start = 0xFFFFFFFF;
        r->join_count = 0;
        r->instance_dirty = false;
    }

    return e;
}

// Create an indexed mesh entity with per-vertex colors
static inline ecs_entity_t scene_add_mesh_colored(ecs_scene_t *scene,
                                                     vec3_t *vertices, int vertex_count,
                                                     uint32_t *indices, int index_count,
                                                     vec4_t *vertex_colors) {
    if (vertex_count < 3 || index_count < 3) return 0;
    int face_count = index_count / 3;

    int first_slot = geom_triangle_batch_alloc_contiguous(&scene->batches.triangles, face_count);
    if (first_slot < 0) return 0;

    ecs_entity_t e = ecs_world_create_entity(scene->world);

    GeometryComp g = geometry_comp_mesh(vertices, vertex_count, indices, index_count,
                                        vertex_colors, vertex_colors[0]);
    ecs_world_set_geometry(scene->world, e, &g);

    TransformComp *t = ecs_world_get_transform(scene->world, e);
    GeometryComp *geom = ecs_world_get_geometry(scene->world, e);
    if (t && geom) {
        for (int f = 0; f < face_count; f++) {
            uint32_t i0 = geom->data.mesh.indices[f * 3 + 0];
            uint32_t i1 = geom->data.mesh.indices[f * 3 + 1];
            uint32_t i2 = geom->data.mesh.indices[f * 3 + 2];

            vec3_t wa = ecs_scene_transform_point_world(&t->world_matrix, geom->data.mesh.vertices[i0]);
            vec3_t wb = ecs_scene_transform_point_world(&t->world_matrix, geom->data.mesh.vertices[i1]);
            vec3_t wc = ecs_scene_transform_point_world(&t->world_matrix, geom->data.mesh.vertices[i2]);
            vec3_t normal = geom_triangle_compute_normal(wa, wb, wc);

            geom_triangle_batch_set_colored(&scene->batches.triangles, first_slot + f,
                                            wa, wb, wc, normal,
                                            geom->data.mesh.vertex_colors[i0],
                                            geom->data.mesh.vertex_colors[i1],
                                            geom->data.mesh.vertex_colors[i2]);
            geom_triangle_batch_set_entity(&scene->batches.triangles, first_slot + f,
                                           (uint64_t)e, (uint8_t)GEOM_MESH);
        }
    }

    RenderableComp *r = ecs_world_get_renderable(scene->world, e);
    if (r) {
        r->batch_id = GEOM_MESH;
        r->instance_slot = (uint32_t)first_slot;
        r->segment_count = (uint32_t)face_count;
        r->join_slot_start = 0xFFFFFFFF;
        r->join_count = 0;
        r->instance_dirty = false;
    }

    return e;
}

// Create a quad mesh (2 triangles) from 4 corners
static inline ecs_entity_t scene_add_mesh_quad(ecs_scene_t *scene,
                                                 vec3_t a, vec3_t b, vec3_t c, vec3_t d,
                                                 vec4_t color) {
    vec3_t verts[4] = { a, b, c, d };
    uint32_t indices[6] = { 0, 1, 2,  0, 2, 3 };
    return scene_add_mesh(scene, verts, 4, indices, 6, color);
}

// Create a box mesh (12 triangles, 8 vertices) centered at `center` with half-extents `size`
static inline ecs_entity_t scene_add_mesh_box(ecs_scene_t *scene,
                                                vec3_t center, vec3_t size,
                                                vec4_t color) {
    float hx = size.x * 0.5f, hy = size.y * 0.5f, hz = size.z * 0.5f;
    float cx = center.x, cy = center.y, cz = center.z;

    vec3_t verts[8] = {
        { cx - hx, cy - hy, cz - hz },  // 0: left  bottom back
        { cx + hx, cy - hy, cz - hz },  // 1: right bottom back
        { cx + hx, cy + hy, cz - hz },  // 2: right top    back
        { cx - hx, cy + hy, cz - hz },  // 3: left  top    back
        { cx - hx, cy - hy, cz + hz },  // 4: left  bottom front
        { cx + hx, cy - hy, cz + hz },  // 5: right bottom front
        { cx + hx, cy + hy, cz + hz },  // 6: right top    front
        { cx - hx, cy + hy, cz + hz },  // 7: left  top    front
    };

    uint32_t indices[36] = {
        // Front face
        4, 5, 6,  4, 6, 7,
        // Back face
        1, 0, 3,  1, 3, 2,
        // Top face
        7, 6, 2,  7, 2, 3,
        // Bottom face
        0, 1, 5,  0, 5, 4,
        // Right face
        5, 1, 2,  5, 2, 6,
        // Left face
        0, 4, 7,  0, 7, 3,
    };

    return scene_add_mesh(scene, verts, 8, indices, 36, color);
}

//------------------------------------------------------------------------------
// Light Entity Creation
//------------------------------------------------------------------------------

// Create a directional light entity (direction stored as transform position)
static inline ecs_entity_t scene_add_directional_light(ecs_scene_t *scene,
                                                         vec3_t direction, vec4_t color,
                                                         float intensity) {
    ecs_world_state_t *w = scene->world;
    ecs_entity_t e = ecs_new(w->world);

    // Add transform - position holds the light direction
    TransformComp t = transform_comp_default();
    t.position = direction;
    ecs_set_id(w->world, e, w->TransformComp_id, sizeof(TransformComp), &t);

    // Add light component
    LightComp l = light_comp_directional(color, intensity);
    ecs_set_id(w->world, e, w->LightComp_id, sizeof(LightComp), &l);

    return e;
}

// Create a point light entity
static inline ecs_entity_t scene_add_point_light(ecs_scene_t *scene,
                                                    vec3_t position, vec4_t color,
                                                    float intensity) {
    ecs_world_state_t *w = scene->world;
    ecs_entity_t e = ecs_new(w->world);

    TransformComp t = transform_comp_default();
    t.position = position;
    ecs_set_id(w->world, e, w->TransformComp_id, sizeof(TransformComp), &t);

    LightComp l = light_comp_point(color, intensity);
    ecs_set_id(w->world, e, w->LightComp_id, sizeof(LightComp), &l);

    return e;
}

// Packed light data for shader uniforms
typedef struct {
    float dir_or_pos[4];  // xyz = direction (directional) or position (point), w = type (0=dir, 1=point)
    float color[4];       // rgb = color, a = intensity
} scene_light_data_t;

// Collect all lights in the scene into a flat array for shader use
// Returns the number of lights collected (up to max_lights)
static inline int scene_collect_lights(ecs_scene_t *scene,
                                         scene_light_data_t *out_lights, int max_lights) {
    ecs_world_state_t *w = scene->world;
    int count = 0;

    // Query all entities that have both TransformComp and LightComp
    ecs_query_t *q = ecs_query(w->world, {
        .terms = {
            { .id = w->TransformComp_id },
            { .id = w->LightComp_id }
        }
    });

    ecs_iter_t it = ecs_query_iter(w->world, q);
    while (ecs_query_next(&it) && count < max_lights) {
        TransformComp *transforms = ecs_field(&it, TransformComp, 0);
        LightComp *lights = ecs_field(&it, LightComp, 1);

        for (int i = 0; i < it.count && count < max_lights; i++) {
            TransformComp *t = &transforms[i];
            LightComp *l = &lights[i];

            out_lights[count].dir_or_pos[0] = t->position.x;
            out_lights[count].dir_or_pos[1] = t->position.y;
            out_lights[count].dir_or_pos[2] = t->position.z;
            out_lights[count].dir_or_pos[3] = (l->type == LIGHT_POINT) ? 1.0f : 0.0f;

            out_lights[count].color[0] = l->color.x;
            out_lights[count].color[1] = l->color.y;
            out_lights[count].color[2] = l->color.z;
            out_lights[count].color[3] = l->intensity;

            count++;
        }

        if (count >= max_lights) {
            ecs_iter_fini(&it);
            break;
        }
    }

    ecs_query_fini(q);
    return count;
}

//------------------------------------------------------------------------------
// Entity Deletion
//------------------------------------------------------------------------------

static inline void scene_constraint_unlink_participants(ecs_scene_t *scene,
                                                        const ConstraintComp *constraint,
                                                        ecs_entity_t constraint_entity) {
    if (!constraint || constraint_entity == 0) return;
    for (uint32_t i = 0; i < constraint->participant_count; i++) {
        ecs_entity_t participant = (ecs_entity_t)constraint->participants[i];
        if (!ecs_is_alive(scene->world->world, participant)) continue;
        ConstraintParticipantComp *refs =
            ecs_world_get_constraint_participant(scene->world, participant);
        if (!refs) continue;
        constraint_participant_remove(refs, (uint64_t)constraint_entity);
    }
}

static inline bool scene_constraint_remove_participant(ecs_scene_t *scene,
                                                       ecs_entity_t constraint_entity,
                                                       ecs_entity_t participant_entity) {
    if (!scene || constraint_entity == 0 || participant_entity == 0) return false;
    if (!ecs_is_alive(scene->world->world, constraint_entity)) return false;
    if (!ecs_is_alive(scene->world->world, participant_entity)) return false;

    ConstraintComp *constraint = ecs_world_get_constraint(scene->world, constraint_entity);
    if (!constraint) return false;

    int remove_index = -1;
    for (uint32_t i = 0; i < constraint->participant_count; i++) {
        if ((ecs_entity_t)constraint->participants[i] == participant_entity) {
            remove_index = (int)i;
            break;
        }
    }
    if (remove_index < 0) return false;

    ConstraintParticipantComp *participant_refs =
        ecs_world_get_constraint_participant(scene->world, participant_entity);
    if (participant_refs) {
        constraint_participant_remove(participant_refs, (uint64_t)constraint_entity);
    }

    for (uint32_t i = (uint32_t)remove_index; i + 1 < constraint->participant_count; i++) {
        constraint->participants[i] = constraint->participants[i + 1];
    }
    if (constraint->participant_count > 0) {
        constraint->participants[constraint->participant_count - 1] = 0;
        constraint->participant_count--;
    }

    if (constraint->participant_count < constraint_type_min_participants(constraint->type)) {
        scene_remove_entity(scene, constraint_entity);
        return true;
    }

    ecs_entity_t sketch = ecs_world_get_parent(scene->world, constraint_entity);
    if (scene_is_sketch(scene, sketch)) {
        scene_refresh_sketch_metadata(scene, sketch);
    }
    return true;
}

static inline void scene_geometry_unlink_constraints(ecs_scene_t *scene, ecs_entity_t geometry_entity) {
    if (!scene || geometry_entity == 0) return;
    if (!ecs_is_alive(scene->world->world, geometry_entity)) return;

    ConstraintParticipantComp *refs = ecs_world_get_constraint_participant(scene->world, geometry_entity);
    if (!refs || refs->constraint_count == 0) return;

    uint64_t linked_constraints[CONSTRAINT_PARTICIPANT_MAX_REFS];
    uint32_t linked_count = refs->constraint_count;
    if (linked_count > CONSTRAINT_PARTICIPANT_MAX_REFS) {
        linked_count = CONSTRAINT_PARTICIPANT_MAX_REFS;
    }

    for (uint32_t i = 0; i < linked_count; i++) {
        linked_constraints[i] = refs->constraints[i];
    }

    for (uint32_t i = 0; i < linked_count; i++) {
        ecs_entity_t constraint_entity = (ecs_entity_t)linked_constraints[i];
        if (constraint_entity != 0 && ecs_is_alive(scene->world->world, constraint_entity)) {
            scene_constraint_remove_participant(scene, constraint_entity, geometry_entity);
        }
    }
}

// Internal helper to free instance slots for an entity (without deleting from ECS)
static inline void scene_free_entity_slots(ecs_scene_t *scene, ecs_entity_t e) {
    RenderableComp *r = ecs_world_get_renderable(scene->world, e);
    if (r && r->instance_slot != 0xFFFFFFFF) {
        // Free the instance slot(s) based on batch type
        switch (r->batch_id) {
            case GEOM_LINE:
                geom_line_batch_free(&scene->batches.lines, (int)r->instance_slot);
                break;
            case GEOM_POINT:
                geom_point_batch_free(&scene->batches.points, (int)r->instance_slot);
                break;
            case GEOM_POINT_CLOUD:
                // Free all point slots (segment_count holds point count)
                geom_point_cloud_batch_free(&scene->batches.points, (int)r->instance_slot, (int)r->segment_count);
                break;
            case GEOM_TRIANGLE:
                geom_triangle_batch_free(&scene->batches.triangles, (int)r->instance_slot);
                break;
            case GEOM_MESH:
                // Free all contiguous triangle batch slots (segment_count holds face count)
                for (uint32_t i = 0; i < r->segment_count; i++) {
                    geom_triangle_batch_free(&scene->batches.triangles, (int)(r->instance_slot + i));
                }
                break;
            case GEOM_POLYLINE:
            case GEOM_ARC:
            case GEOM_POLYGON:
            case GEOM_HELIX:
            case GEOM_BEZIER:
                // Free all segment slots
                for (uint32_t i = 0; i < r->segment_count; i++) {
                    geom_line_batch_free(&scene->batches.lines, (int)(r->instance_slot + i));
                }
                // Free all join slots
                if (r->join_slot_start != 0xFFFFFFFF) {
                    for (uint32_t i = 0; i < r->join_count; i++) {
                        geom_point_batch_free(&scene->batches.points, (int)(r->join_slot_start + i));
                    }
                }
                break;
            default:
                break;
        }
    }
}

// Recursively remove an entity and all its children
static inline void scene_remove_entity(ecs_scene_t *scene, ecs_entity_t e) {
    if (!ecs_is_alive(scene->world->world, e)) return;
    ecs_entity_t parent = ecs_world_get_parent(scene->world, e);

    // First, recursively delete all children
    // We need to collect children first because deleting modifies the hierarchy
    // Process in batches to handle unlimited children
    ecs_entity_t children[64];
    int child_count;

    // Keep deleting children until none remain
    while ((child_count = ecs_world_get_children(scene->world, e, children, 64)) > 0) {
        for (int i = 0; i < child_count; i++) {
            scene_remove_entity(scene, children[i]);
        }
    }

    // Deleting a geometry entity must also delete constraints referencing it.
    // This avoids dangling constraint entities and stale glyph references.
    if (ecs_world_get_geometry(scene->world, e)) {
        scene_geometry_unlink_constraints(scene, e);
    }

    // Keep constraint participant links coherent when deleting constraint entities.
    ConstraintComp *constraint = ecs_world_get_constraint(scene->world, e);
    if (constraint) {
        scene_constraint_unlink_participants(scene, constraint, e);
    }

    // Free this entity's instance buffer slots
    scene_free_entity_slots(scene, e);

    // Delete entity (also frees pick ID and geometry allocations)
    ecs_world_delete_entity(scene->world, e);

    if (parent != 0 && ecs_is_alive(scene->world->world, parent) && scene_is_sketch(scene, parent)) {
        scene_refresh_sketch_metadata(scene, parent);
    }
}

//------------------------------------------------------------------------------
// Parent-Child Relationship API
//------------------------------------------------------------------------------

// Set parent of an entity (pass 0 to unparent)
static inline void scene_set_parent(ecs_scene_t *scene, ecs_entity_t child, ecs_entity_t parent) {
    ecs_world_set_parent(scene->world, child, parent);
}

// Create a transform-only anchor entity (no geometry, no GPU slot)
static inline ecs_entity_t scene_add_anchor(ecs_scene_t *scene,
                                               const char *name, const char *desc) {
    ecs_entity_t e = ecs_world_create_anchor_entity(scene->world);
    if (name || desc) {
        LabelComp label = label_comp_make(name ? name : "", desc ? desc : "");
        ecs_world_set_label(scene->world, e, &label);
    }
    return e;
}

// Create a sketch container entity
static inline bool scene_should_autoname_sketch(const char *name) {
    return !name || name[0] == '\0' || strcmp(name, "Sketch") == 0;
}

static inline const char* scene_geometry_name_prefix(geometry_type_t type) {
    switch (type) {
        case GEOM_POINT: return "Point";
        case GEOM_LINE: return "Line";
        case GEOM_POLYLINE: return "Polyline";
        case GEOM_ARC: return "Arc";
        case GEOM_POLYGON: return "Polygon";
        case GEOM_HELIX: return "Helix";
        case GEOM_BEZIER: return "Bezier";
        case GEOM_POINT_CLOUD: return "PointCloud";
        case GEOM_TRIANGLE: return "Triangle";
        case GEOM_MESH: return "Mesh";
        default: return "Geometry";
    }
}

static inline const char* scene_constraint_name_prefix(constraint_type_t type) {
    switch (type) {
        case CONSTRAINT_FIXED: return "Fixed";
        case CONSTRAINT_COINCIDENT: return "Coincident";
        case CONSTRAINT_COLLINEAR: return "Collinear";
        case CONSTRAINT_PARALLEL: return "Parallel";
        case CONSTRAINT_PERPENDICULAR: return "Perpendicular";
        case CONSTRAINT_ALONG_X: return "AlongX";
        case CONSTRAINT_ALONG_Y: return "AlongY";
        case CONSTRAINT_ALONG_Z: return "AlongZ";
        case CONSTRAINT_CORADIAL: return "Coradial";
        case CONSTRAINT_CONCENTRIC: return "Concentric";
        case CONSTRAINT_LENGTH: return "Length";
        case CONSTRAINT_ANGLE: return "Angle";
        case CONSTRAINT_TANGENTIAL: return "Tangential";
        case CONSTRAINT_ARC_AXIS_LINE: return "ArcAxisLine";
        case CONSTRAINT_LINE_ARC_ENDPOINT_TANGENCY: return "LineArcEndpointTangency";
        case CONSTRAINT_ARC_ENDPOINT_ANGLE: return "ArcEndpointAngle";
        default: return "Constraint";
    }
}

static inline void scene_sketch_name_fallback(ecs_scene_t *scene, char *out_name, size_t out_size) {
    snprintf(out_name, out_size, "Sketch_%u", scene->next_sketch_name_index++);
}

static inline void scene_get_sketch_label_name(ecs_scene_t *scene, ecs_entity_t sketch,
                                               char *out_name, size_t out_size) {
    if (!out_name || out_size == 0) return;
    out_name[0] = '\0';

    LabelComp *label = ecs_world_get_label(scene->world, sketch);
    if (label && label->name[0] != '\0') {
        strncpy(out_name, label->name, out_size - 1);
        out_name[out_size - 1] = '\0';
        return;
    }

    SketchComp *sk = ecs_world_get_sketch(scene->world, sketch);
    if (sk && sk->next_geometry_name_index[GEOM_POINT] > 0) {
        strncpy(out_name, "Sketch", out_size - 1);
        out_name[out_size - 1] = '\0';
        return;
    }

    strncpy(out_name, "Sketch", out_size - 1);
    out_name[out_size - 1] = '\0';
}

static inline void scene_set_geometry_default_label(ecs_scene_t *scene, ecs_entity_t sketch,
                                                    ecs_entity_t geometry_entity, geometry_type_t type) {
    SketchComp *sk = ecs_world_get_sketch(scene->world, sketch);
    if (!sk) return;
    if (type < 0 || type >= GEOM_TYPE_COUNT) return;

    LabelComp *existing = ecs_world_get_label(scene->world, geometry_entity);
    if (existing && existing->name[0] != '\0') {
        return;
    }

    uint32_t index = ++sk->next_geometry_name_index[type];
    const char *prefix = scene_geometry_name_prefix(type);
    char sketch_name[LABEL_NAME_MAX];
    char name[LABEL_NAME_MAX];
    scene_get_sketch_label_name(scene, sketch, sketch_name, sizeof(sketch_name));
    snprintf(name, sizeof(name), "%s_%u", prefix, index);

    LabelComp label = label_comp_make(name, sketch_name);
    ecs_world_set_label(scene->world, geometry_entity, &label);
}

static inline void scene_set_constraint_default_label(ecs_scene_t *scene, ecs_entity_t sketch,
                                                      ecs_entity_t constraint_entity, constraint_type_t type) {
    SketchComp *sk = ecs_world_get_sketch(scene->world, sketch);
    if (!sk) return;
    if (type < 0 || type >= CONSTRAINT_TYPE_COUNT) return;

    LabelComp *existing = ecs_world_get_label(scene->world, constraint_entity);
    if (existing && existing->name[0] != '\0') {
        return;
    }

    uint32_t index = ++sk->next_constraint_name_index[type];
    const char *prefix = scene_constraint_name_prefix(type);
    char sketch_name[LABEL_NAME_MAX];
    char name[LABEL_NAME_MAX];
    scene_get_sketch_label_name(scene, sketch, sketch_name, sizeof(sketch_name));
    snprintf(name, sizeof(name), "%s_%u", prefix, index);

    LabelComp label = label_comp_make(name, sketch_name);
    ecs_world_set_label(scene->world, constraint_entity, &label);
}

static inline ecs_entity_t scene_add_sketch(ecs_scene_t *scene,
                                            const char *name, const char *desc,
                                            vec4_t sketch_color) {
    char auto_name[LABEL_NAME_MAX];
    const char *resolved_name = name;
    if (scene_should_autoname_sketch(name)) {
        scene_sketch_name_fallback(scene, auto_name, sizeof(auto_name));
        resolved_name = auto_name;
    }

    ecs_entity_t sketch = scene_add_anchor(scene, resolved_name ? resolved_name : "Sketch", desc ? desc : "");
    if (sketch == 0) return 0;

    SketchComp sketch_comp = sketch_comp_default();
    sketch_comp.color = sketch_color;
    sketch_comp.solver_backend_id = scene_solver_backend_id(scene);
    ecs_world_set_sketch(scene->world, sketch, &sketch_comp);

    return sketch;
}

static inline bool scene_is_sketch(ecs_scene_t *scene, ecs_entity_t e) {
    if (!scene || e == 0) return false;
    if (!ecs_is_alive(scene->world->world, e)) return false;
    return ecs_world_get_sketch(scene->world, e) != NULL;
}

// Attach an existing geometry entity to a sketch container
static inline bool scene_attach_geometry_to_sketch(ecs_scene_t *scene,
                                                   ecs_entity_t sketch,
                                                   ecs_entity_t geometry_entity) {
    if (!scene_is_sketch(scene, sketch)) return false;
    if (!ecs_is_alive(scene->world->world, geometry_entity)) return false;
    if (!ecs_world_get_geometry(scene->world, geometry_entity)) return false;

    scene_set_parent(scene, geometry_entity, sketch);

    SketchGeometryStateComp *state = ecs_world_get_sketch_geometry_state(scene->world, geometry_entity);
    if (!state) {
        SketchGeometryStateComp init_state = sketch_geometry_state_comp_default();
        ecs_world_set_sketch_geometry_state(scene->world, geometry_entity, &init_state);
    }

    GeometryComp *g = ecs_world_get_geometry(scene->world, geometry_entity);
    if (g) {
        scene_set_geometry_default_label(scene, sketch, geometry_entity, g->type);
    }

    scene_sync_endpoint_entities_for_owner(scene, geometry_entity);

    scene_solver_request_auto(scene, sketch);
    scene_script_reemit_for_sketch(scene, sketch);

    return true;
}

// Create geometry and attach it to a sketch in one helper call
static inline ecs_entity_t scene_add_point_to_sketch(ecs_scene_t *scene, ecs_entity_t sketch,
                                                     vec3_t pos, vec4_t color, float size) {
    ecs_entity_t e = scene_add_point(scene, pos, color, size);
    if (e == 0) return 0;
    if (!scene_attach_geometry_to_sketch(scene, sketch, e)) {
        scene_remove_entity(scene, e);
        return 0;
    }
    return e;
}

static inline ecs_entity_t scene_add_line_to_sketch(ecs_scene_t *scene, ecs_entity_t sketch,
                                                    vec3_t a, vec3_t b, vec4_t color, float width) {
    ecs_entity_t e = scene_add_line(scene, a, b, color, width);
    if (e == 0) return 0;
    if (!scene_attach_geometry_to_sketch(scene, sketch, e)) {
        scene_remove_entity(scene, e);
        return 0;
    }
    return e;
}

static inline ecs_entity_t scene_add_arc_to_sketch(ecs_scene_t *scene, ecs_entity_t sketch,
                                                    vec3_t center, float radius,
                                                    float start_angle, float end_angle,
                                                    vec3_t normal, vec4_t color, float width) {
    ecs_entity_t e = scene_add_arc(scene, center, radius, start_angle, end_angle, normal, color, width);
    if (e == 0) return 0;
    if (!scene_attach_geometry_to_sketch(scene, sketch, e)) {
        scene_remove_entity(scene, e);
        return 0;
    }
    return e;
}

static inline void scene_sync_endpoint_entities_for_owner(ecs_scene_t *scene, ecs_entity_t owner_entity) {
    if (!scene || owner_entity == 0 || !ecs_is_alive(scene->world->world, owner_entity)) return;

    GeometryComp *owner_geom = ecs_world_get_geometry(scene->world, owner_entity);
    if (!owner_geom) return;
    if (owner_geom->type != GEOM_LINE && owner_geom->type != GEOM_ARC) return;

    ecs_entity_t sketch = scene_find_parent_sketch(scene, owner_entity);
    if (!scene_is_sketch(scene, sketch)) return; // sketch scope guard

    EndPointsComp *owner_endpoints = ecs_world_get_endpoints(scene->world, owner_entity);
    if (!owner_endpoints || owner_endpoints->is_endpoint_point) {
        EndPointsComp init_owner = endpoints_comp_owner_default();
        ecs_world_set_endpoints(scene->world, owner_entity, &init_owner);
        owner_endpoints = ecs_world_get_endpoints(scene->world, owner_entity);
        if (!owner_endpoints) return;
    }

    constraint_participant_role_t roles[3] = {
        CONSTRAINT_PARTICIPANT_ROLE_POINT_A,
        CONSTRAINT_PARTICIPANT_ROLE_POINT_B,
        CONSTRAINT_PARTICIPANT_ROLE_CENTER
    };
    uint8_t role_count = (owner_geom->type == GEOM_ARC) ? 3u : 2u;

    for (uint8_t i = 0; i < role_count; i++) {
        constraint_participant_role_t role = roles[i];
        vec3_t owner_point = vec3_make(0.0f, 0.0f, 0.0f);
        uint8_t sub_index = 0;
        if (role == CONSTRAINT_PARTICIPANT_ROLE_CENTER) {
            owner_point = owner_geom->data.arc.center;
            sub_index = 2u;
        } else {
            if (!scene_entity_participant_subpoint(owner_geom, role, &owner_point, &sub_index)) {
                continue;
            }
        }

        endpoint_binding_t binding = {0};
        ecs_entity_t endpoint_entity = 0;
        if (endpoints_comp_find_binding(owner_endpoints, role, &binding)) {
            endpoint_entity = (ecs_entity_t)binding.endpoint_entity;
            if (endpoint_entity != 0 && !ecs_is_alive(scene->world->world, endpoint_entity)) {
                endpoint_entity = 0;
            }
        }

        if (endpoint_entity == 0) {
            endpoint_entity = scene_add_point(scene, owner_point, owner_geom->color, 8.0f);
            if (endpoint_entity == 0) continue;
            scene_set_parent(scene, endpoint_entity, owner_entity);
        }

        EndPointsComp endpoint_meta = endpoints_comp_point((uint64_t)owner_entity, role, sub_index);
        ecs_world_set_endpoints(scene->world, endpoint_entity, &endpoint_meta);
        endpoints_comp_set_binding(owner_endpoints, role, sub_index, (uint64_t)endpoint_entity);

        GeometryComp *endpoint_geom = ecs_world_get_geometry(scene->world, endpoint_entity);
        if (endpoint_geom && endpoint_geom->type == GEOM_POINT) {
            endpoint_geom->data.point.point = owner_point;
            endpoint_geom->color = owner_geom->color;
            endpoint_geom->point_size = 8.0f;
        }
        TransformComp *endpoint_xform = ecs_world_get_transform(scene->world, endpoint_entity);
        if (endpoint_xform) {
            endpoint_xform->position = vec3_make(0.0f, 0.0f, 0.0f);
            endpoint_xform->rotation = vec3_make(0.0f, 0.0f, 0.0f);
            endpoint_xform->scale = vec3_make(1.0f, 1.0f, 1.0f);
            endpoint_xform->dirty = true;
        }
        RenderableComp *endpoint_renderable = ecs_world_get_renderable(scene->world, endpoint_entity);
        if (endpoint_renderable) {
            endpoint_renderable->visible = true;
            endpoint_renderable->instance_dirty = true;
        }
        SelectableComp *endpoint_selectable = ecs_world_get_selectable(scene->world, endpoint_entity);
        if (endpoint_selectable) endpoint_selectable->pickable = true;
    }

    owner_endpoints->is_endpoint_point = false;
}

static inline bool scene_is_constraint_entity(ecs_scene_t *scene, ecs_entity_t e) {
    if (!scene || e == 0) return false;
    if (!ecs_is_alive(scene->world->world, e)) return false;
    return ecs_world_get_constraint(scene->world, e) != NULL;
}

static inline int scene_count_sketch_constraints(ecs_scene_t *scene, ecs_entity_t sketch) {
    if (!scene_is_sketch(scene, sketch)) return 0;

    int count = 0;
    ecs_iter_t it = ecs_children(scene->world->world, sketch);
    while (ecs_children_next(&it)) {
        for (int i = 0; i < it.count; i++) {
            if (ecs_world_get_constraint(scene->world, it.entities[i])) {
                count++;
            }
        }
    }
    return count;
}

static inline bool scene_script_entity_needs_id(ecs_scene_t *scene, ecs_entity_t e, ecs_entity_t sketch) {
    if (!scene || e == 0 || sketch == 0) return false;
    if (!ecs_is_alive(scene->world->world, e)) return false;
    if (ecs_world_get_parent(scene->world, e) != sketch) return false;
    if (ecs_world_get_geometry(scene->world, e)) return true;
    if (ecs_world_get_constraint(scene->world, e)) return true;
    return false;
}

static inline uint32_t scene_script_local_id_sequence(const ScriptIdentityComp *identity,
                                                      const char *prefix) {
    if (!identity || !prefix || prefix[0] == '\0') return 0;
    size_t prefix_len = strlen(prefix);
    if (strncmp(identity->script_local_id, prefix, prefix_len) != 0) return 0;
    if (identity->script_local_id[prefix_len] != '_') return 0;
    const char *digits = identity->script_local_id + prefix_len + 1;
    if (*digits == '\0') return 0;
    uint32_t value = 0;
    for (const char *c = digits; *c; c++) {
        if (*c < '0' || *c > '9') return 0;
        value = value * 10u + (uint32_t)(*c - '0');
    }
    return value;
}

static inline bool scene_script_local_id_duplicate(ecs_scene_t *scene, ecs_entity_t sketch,
                                                   ecs_entity_t candidate, const char *id) {
    if (!scene || !id || id[0] == '\0') return false;
    ecs_iter_t it = ecs_children(scene->world->world, sketch);
    while (ecs_children_next(&it)) {
        for (int i = 0; i < it.count; i++) {
            ecs_entity_t child = it.entities[i];
            if (child == candidate) continue;
            if (!scene_script_entity_needs_id(scene, child, sketch)) continue;
            ScriptIdentityComp *other = ecs_world_get_script_identity(scene->world, child);
            if (!other || other->script_local_id[0] == '\0') continue;
            if (strcmp(other->script_local_id, id) == 0) return true;
        }
    }
    return false;
}

static inline void scene_script_local_id_assign_unique(ecs_scene_t *scene, ecs_entity_t sketch,
                                                       ecs_entity_t entity, const char *prefix,
                                                       uint32_t *next_index) {
    if (!scene || !prefix || !next_index || entity == 0) return;
    ScriptIdentityComp *id = ecs_world_get_script_identity(scene->world, entity);
    if (!id) {
        ScriptIdentityComp init = script_identity_comp_default();
        ecs_world_set_script_identity(scene->world, entity, &init);
        id = ecs_world_get_script_identity(scene->world, entity);
        if (!id) return;
    }

    char candidate[SCRIPT_LOCAL_ID_MAX];
    while (true) {
        uint32_t current = (*next_index)++;
        snprintf(candidate, sizeof(candidate), "%s_%u", prefix, current);
        candidate[sizeof(candidate) - 1] = '\0';
        if (!scene_script_local_id_duplicate(scene, sketch, entity, candidate)) {
            strncpy(id->script_local_id, candidate, SCRIPT_LOCAL_ID_MAX - 1);
            id->script_local_id[SCRIPT_LOCAL_ID_MAX - 1] = '\0';
            return;
        }
    }
}

static inline void scene_normalize_sketch_script_local_ids(ecs_scene_t *scene, ecs_entity_t sketch) {
    if (!scene_is_sketch(scene, sketch)) return;

    uint32_t next_geom = 1;
    uint32_t next_constraint = 1;

    ecs_iter_t first = ecs_children(scene->world->world, sketch);
    while (ecs_children_next(&first)) {
        for (int i = 0; i < first.count; i++) {
            ecs_entity_t child = first.entities[i];
            if (!scene_script_entity_needs_id(scene, child, sketch)) continue;
            ScriptIdentityComp *id = ecs_world_get_script_identity(scene->world, child);
            if (!id || id->script_local_id[0] == '\0') continue;
            const char *prefix = ecs_world_get_constraint(scene->world, child) ? "constraint" : "geometry";
            uint32_t seq = scene_script_local_id_sequence(id, prefix);
            if (seq == 0) continue;
            if (ecs_world_get_constraint(scene->world, child)) {
                if (seq >= next_constraint) next_constraint = seq + 1;
            } else {
                if (seq >= next_geom) next_geom = seq + 1;
            }
        }
    }

    ecs_iter_t second = ecs_children(scene->world->world, sketch);
    while (ecs_children_next(&second)) {
        for (int i = 0; i < second.count; i++) {
            ecs_entity_t child = second.entities[i];
            if (!scene_script_entity_needs_id(scene, child, sketch)) continue;
            ScriptIdentityComp *id = ecs_world_get_script_identity(scene->world, child);
            const bool is_constraint = ecs_world_get_constraint(scene->world, child) != NULL;
            const char *prefix = is_constraint ? "constraint" : "geometry";
            bool valid = false;
            if (id && id->script_local_id[0] != '\0') {
                valid = scene_script_local_id_sequence(id, prefix) > 0 &&
                        !scene_script_local_id_duplicate(scene, sketch, child, id->script_local_id);
            }
            if (!valid) {
                if (is_constraint) {
                    scene_script_local_id_assign_unique(scene, sketch, child, prefix, &next_constraint);
                } else {
                    scene_script_local_id_assign_unique(scene, sketch, child, prefix, &next_geom);
                }
            }
        }
    }
}

static inline ecs_entity_t scene_add_constraint_to_sketch(ecs_scene_t *scene,
                                                          ecs_entity_t sketch,
                                                          constraint_type_t type,
                                                          const ecs_entity_t *participants,
                                                          uint32_t participant_count,
                                                          float value,
                                                          bool driven) {
    constraint_participant_descriptor_t descriptors[CONSTRAINT_MAX_PARTICIPANTS] = {0};
    if (!participants || participant_count == 0 || participant_count > CONSTRAINT_MAX_PARTICIPANTS) return 0;
    for (uint32_t i = 0; i < participant_count; i++) {
        descriptors[i] = constraint_participant_descriptor_make((uint64_t)participants[i],
                                                                CONSTRAINT_PARTICIPANT_ROLE_ENTITY,
                                                                0);
    }
    return scene_add_constraint_to_sketch_with_descriptors(scene, sketch, type,
                                                           descriptors, participant_count,
                                                           value, driven);
}

static inline ecs_entity_t scene_add_constraint_to_sketch_with_descriptors(
                                                          ecs_scene_t *scene,
                                                          ecs_entity_t sketch,
                                                          constraint_type_t type,
                                                          const constraint_participant_descriptor_t *participants,
                                                          uint32_t participant_count,
                                                          float value,
                                                          bool driven) {
    if (!scene_is_sketch(scene, sketch) || !participants || participant_count == 0) return 0;
    if (participant_count > CONSTRAINT_MAX_PARTICIPANTS) return 0;
    if (participant_count < constraint_type_min_participants(type)) return 0;

    constraint_selection_signature_t signature = {0};
    signature.count = participant_count;
    uint64_t participant_ids[CONSTRAINT_MAX_PARTICIPANTS];
    constraint_participant_descriptor_t participant_descriptors[CONSTRAINT_MAX_PARTICIPANTS] = {0};
    for (uint32_t i = 0; i < participant_count; i++) {
        ecs_entity_t p = (ecs_entity_t)participants[i].entity;
        constraint_participant_role_t role = (constraint_participant_role_t)participants[i].role;
        if (!ecs_is_alive(scene->world->world, p)) return 0;
        GeometryComp *g = ecs_world_get_geometry(scene->world, p);
        if (!g) return 0;
        if (ecs_world_get_parent(scene->world, p) != sketch) return 0;
        if (role == CONSTRAINT_PARTICIPANT_ROLE_UNSPECIFIED) role = CONSTRAINT_PARTICIPANT_ROLE_ENTITY;
        if (g->type == GEOM_LINE || g->type == GEOM_ARC) {
            if (g->type == GEOM_LINE) {
                if (role != CONSTRAINT_PARTICIPANT_ROLE_ENTITY &&
                    role != CONSTRAINT_PARTICIPANT_ROLE_POINT_A &&
                    role != CONSTRAINT_PARTICIPANT_ROLE_POINT_B) {
                    return 0;
                }
            } else {
                if (role != CONSTRAINT_PARTICIPANT_ROLE_ENTITY &&
                    role != CONSTRAINT_PARTICIPANT_ROLE_POINT_A &&
                    role != CONSTRAINT_PARTICIPANT_ROLE_POINT_B &&
                    role != CONSTRAINT_PARTICIPANT_ROLE_CENTER) {
                    return 0;
                }
            }
        } else {
            role = CONSTRAINT_PARTICIPANT_ROLE_ENTITY;
        }
        participant_ids[i] = (uint64_t)p;
        signature.entities[i] = (uint64_t)p;
        signature.geometry_types[i] = g->type;
        signature.roles[i] = role;
        participant_descriptors[i] =
            constraint_participant_descriptor_make((uint64_t)p, (uint8_t)role, participants[i].sub_index);
    }
    if (!constraint_type_is_selection_legal(&signature, type)) return 0;

    ecs_entity_t constraint_e = scene_add_anchor(scene, "", "");
    if (constraint_e == 0) return 0;

    ConstraintComp constraint = constraint_comp_make(type, participant_ids, participant_count, value, driven);
    if (constraint_comp_is_dimensional(&constraint)) {
        constraint.display_decimals = constraint_value_infer_decimals(value, 0);
        constraint.value = constraint_round_to_decimals(value, constraint.display_decimals);
    }
    memcpy(constraint.participant_descriptors,
           participant_descriptors,
           sizeof(constraint_participant_descriptor_t) * participant_count);
    ecs_world_set_constraint(scene->world, constraint_e, &constraint);
    scene_set_parent(scene, constraint_e, sketch);
    scene_set_constraint_default_label(scene, sketch, constraint_e, type);

    for (uint32_t i = 0; i < participant_count; i++) {
        ecs_entity_t p = (ecs_entity_t)participant_descriptors[i].entity;
        ConstraintParticipantComp *refs = ecs_world_get_constraint_participant(scene->world, p);
        if (!refs) {
            ConstraintParticipantComp init_refs = constraint_participant_comp_default();
            ecs_world_set_constraint_participant(scene->world, p, &init_refs);
            refs = ecs_world_get_constraint_participant(scene->world, p);
        }
        if (!refs || !constraint_participant_add(refs, (uint64_t)constraint_e)) {
            scene_remove_entity(scene, constraint_e);
            return 0;
        }
    }

    scene_refresh_sketch_metadata(scene, sketch);
    scene_solver_request_auto(scene, sketch);
    scene_script_reemit_for_sketch(scene, sketch);
    return constraint_e;
}

static inline bool scene_constraint_set_dimensional_value(ecs_scene_t *scene,
                                                          ecs_entity_t constraint_entity,
                                                          float value,
                                                          bool driven) {
    ConstraintComp *constraint = ecs_world_get_constraint(scene->world, constraint_entity);
    if (!constraint || !constraint_comp_is_dimensional(constraint)) return false;
    uint8_t fallback_decimals = constraint->display_decimals;
    if (fallback_decimals == 0) fallback_decimals = 4;
    constraint->has_value = true;
    uint8_t inferred_decimals = constraint_value_infer_decimals(value, fallback_decimals);
    constraint->value = constraint_round_to_decimals(value, inferred_decimals);
    constraint->driven = driven;
    constraint->display_decimals = inferred_decimals;

    ecs_entity_t sketch = ecs_world_get_parent(scene->world, constraint_entity);
    if (scene_is_sketch(scene, sketch)) {
        scene_solver_request_auto(scene, sketch);
        scene_script_reemit_for_sketch(scene, sketch);
    }
    return true;
}

static inline bool scene_remove_constraint(ecs_scene_t *scene, ecs_entity_t constraint_entity) {
    if (!scene || constraint_entity == 0) return false;
    if (!ecs_is_alive(scene->world->world, constraint_entity)) return false;
    if (!scene_is_constraint_entity(scene, constraint_entity)) return false;
    ecs_entity_t sketch = ecs_world_get_parent(scene->world, constraint_entity);
    scene_remove_entity(scene, constraint_entity);
    if (scene_is_sketch(scene, sketch)) {
        scene_solver_request_auto(scene, sketch);
        scene_script_reemit_for_sketch(scene, sketch);
    }
    return true;
}

static inline const char* scene_solver_backend_name(const ecs_scene_t *scene) {
    if (!scene || !scene->solver_backend_name || scene->solver_backend_name[0] == '\0') {
        return "ConstraintSketchSolverV1";
    }
    return scene->solver_backend_name;
}

static inline uint32_t scene_solver_backend_id(const ecs_scene_t *scene) {
    if (!scene || scene->solver_backend_id == 0) {
        return 1;
    }
    return scene->solver_backend_id;
}

static inline float scene_solver_position_tolerance(const ecs_scene_t *scene, ecs_entity_t sketch) {
    if (!scene || !scene_is_sketch((ecs_scene_t*)scene, sketch)) return SKETCH_SOLVER_DEFAULT_POSITION_TOLERANCE;
    const SketchComp *sk = ecs_world_get_sketch(scene->world, sketch);
    if (!sk || sk->solver_position_tolerance <= 0.0f) return SKETCH_SOLVER_DEFAULT_POSITION_TOLERANCE;
    return sk->solver_position_tolerance;
}

static inline bool scene_solver_set_position_tolerance(ecs_scene_t *scene, ecs_entity_t sketch, float tolerance) {
    if (!scene || !scene_is_sketch(scene, sketch)) return false;
    SketchComp *sk = ecs_world_get_sketch(scene->world, sketch);
    if (!sk) return false;
    sk->solver_position_tolerance = (tolerance > 0.0f) ? tolerance : SKETCH_SOLVER_DEFAULT_POSITION_TOLERANCE;
    return true;
}

static inline float scene_solver_angle_tolerance(const ecs_scene_t *scene, ecs_entity_t sketch) {
    if (!scene || !scene_is_sketch((ecs_scene_t*)scene, sketch)) return SKETCH_SOLVER_DEFAULT_ANGLE_TOLERANCE;
    const SketchComp *sk = ecs_world_get_sketch(scene->world, sketch);
    if (!sk || sk->solver_angle_tolerance <= 0.0f) return SKETCH_SOLVER_DEFAULT_ANGLE_TOLERANCE;
    return sk->solver_angle_tolerance;
}

static inline bool scene_solver_set_angle_tolerance(ecs_scene_t *scene, ecs_entity_t sketch, float tolerance) {
    if (!scene || !scene_is_sketch(scene, sketch)) return false;
    SketchComp *sk = ecs_world_get_sketch(scene->world, sketch);
    if (!sk) return false;
    sk->solver_angle_tolerance = (tolerance > 0.0f) ? tolerance : SKETCH_SOLVER_DEFAULT_ANGLE_TOLERANCE;
    return true;
}

static inline uint32_t scene_solver_auto_solve_debounce_ms(const ecs_scene_t *scene, ecs_entity_t sketch) {
    if (!scene || !scene_is_sketch((ecs_scene_t*)scene, sketch)) return SKETCH_SOLVER_DEFAULT_DEBOUNCE_MS;
    const SketchComp *sk = ecs_world_get_sketch(scene->world, sketch);
    if (!sk) return SKETCH_SOLVER_DEFAULT_DEBOUNCE_MS;
    if (sk->auto_solve_debounce_ms > 100u) return 100u;
    return sk->auto_solve_debounce_ms;
}

static inline bool scene_solver_set_auto_solve_debounce_ms(ecs_scene_t *scene, ecs_entity_t sketch, uint32_t debounce_ms) {
    if (!scene || !scene_is_sketch(scene, sketch)) return false;
    SketchComp *sk = ecs_world_get_sketch(scene->world, sketch);
    if (!sk) return false;
    if (debounce_ms > 100u) debounce_ms = 100u;
    sk->auto_solve_debounce_ms = debounce_ms;
    return true;
}

static inline uint32_t scene_solver_max_passes(const ecs_scene_t *scene, ecs_entity_t sketch) {
    if (!scene || !scene_is_sketch((ecs_scene_t*)scene, sketch)) return SKETCH_SOLVER_DEFAULT_MAX_PASSES;
    const SketchComp *sk = ecs_world_get_sketch(scene->world, sketch);
    if (!sk || sk->solver_max_passes == 0) return SKETCH_SOLVER_DEFAULT_MAX_PASSES;
    return sk->solver_max_passes;
}

static inline bool scene_solver_set_max_passes(ecs_scene_t *scene, ecs_entity_t sketch, uint32_t max_passes) {
    if (!scene || !scene_is_sketch(scene, sketch)) return false;
    SketchComp *sk = ecs_world_get_sketch(scene->world, sketch);
    if (!sk) return false;
    sk->solver_max_passes = (max_passes > 0) ? max_passes : SKETCH_SOLVER_DEFAULT_MAX_PASSES;
    return true;
}

static inline bool scene_solver_set_auto_solve(ecs_scene_t *scene, ecs_entity_t sketch, bool enabled) {
    if (!scene || !scene_is_sketch(scene, sketch)) return false;
    SketchComp *sk = ecs_world_get_sketch(scene->world, sketch);
    if (!sk) return false;
    sk->auto_solve_enabled = enabled;
    if (!enabled) {
        sk->auto_solve_pending = false;
        sk->auto_solve_queued_at_ms = 0;
        sk->auto_solve_queue_token = 0;
    }
    return true;
}

static inline bool scene_solver_request_auto(ecs_scene_t *scene, ecs_entity_t sketch) {
    if (!scene || !scene_is_sketch(scene, sketch)) return false;
    SketchComp *sk = ecs_world_get_sketch(scene->world, sketch);
    if (!sk) return false;
    if (!sk->auto_solve_enabled) return false;

    if (!sk->auto_solve_pending) {
        sk->auto_solve_pending = true;
        sk->solve_request_serial++;
        sk->auto_solve_queued_at_ms = scene_solver_now_ms();
        sk->auto_solve_queue_token = sk->solve_request_serial;
    }
    return true;
}

static inline bool scene_solver_set_drag_anchor(ecs_scene_t *scene,
                                                ecs_entity_t sketch,
                                                ecs_entity_t owner_entity,
                                                constraint_participant_role_t role,
                                                uint8_t sub_index) {
    if (!scene || !scene_is_sketch(scene, sketch)) return false;
    SketchComp *sk = ecs_world_get_sketch(scene->world, sketch);
    if (!sk) return false;
    if (owner_entity == 0 || !ecs_is_alive(scene->world->world, owner_entity)) {
        sk->solver_drag_anchor_valid = false;
        sk->solver_drag_anchor_owner_entity = 0;
        sk->solver_drag_anchor_role = (uint8_t)CONSTRAINT_PARTICIPANT_ROLE_UNSPECIFIED;
        sk->solver_drag_anchor_sub_index = 0;
        return false;
    }
    sk->solver_drag_anchor_valid = true;
    sk->solver_drag_anchor_owner_entity = (uint64_t)owner_entity;
    sk->solver_drag_anchor_role = (uint8_t)role;
    sk->solver_drag_anchor_sub_index = sub_index;
    return true;
}

static inline bool scene_solver_clear_drag_anchor(ecs_scene_t *scene, ecs_entity_t sketch) {
    if (!scene || !scene_is_sketch(scene, sketch)) return false;
    SketchComp *sk = ecs_world_get_sketch(scene->world, sketch);
    if (!sk) return false;
    sk->solver_drag_anchor_valid = false;
    sk->solver_drag_anchor_owner_entity = 0;
    sk->solver_drag_anchor_role = (uint8_t)CONSTRAINT_PARTICIPANT_ROLE_UNSPECIFIED;
    sk->solver_drag_anchor_sub_index = 0;
    return true;
}

static inline uint64_t scene_solver_now_ms(void) {
    struct timespec ts;
    if (timespec_get(&ts, TIME_UTC) == TIME_UTC) {
        return ((uint64_t)ts.tv_sec * 1000ULL) + ((uint64_t)ts.tv_nsec / 1000000ULL);
    }
    return (uint64_t)time(NULL) * 1000ULL;
}

static inline void scene_solver_process_auto_queue(ecs_scene_t *scene) {
    if (!scene || !scene->world) return;
    ecs_world_state_t *w = scene->world;
    uint64_t now_ms = scene_solver_now_ms();
    ecs_query_t *q = ecs_query(w->world, {
        .terms = {
            { .id = w->SketchComp_id }
        }
    });
    ecs_iter_t it = ecs_query_iter(w->world, q);
    while (ecs_query_next(&it)) {
        SketchComp *sketches = ecs_field(&it, SketchComp, 0);
        for (int i = 0; i < it.count; i++) {
            ecs_entity_t sketch_entity = it.entities[i];
            SketchComp *sk = &sketches[i];
            if (!sk->auto_solve_enabled || !sk->auto_solve_pending) continue;
            uint64_t queued_at = sk->auto_solve_queued_at_ms;
            uint64_t elapsed_ms = (now_ms >= queued_at) ? (now_ms - queued_at) : 0ULL;
            uint32_t debounce_ms = sk->auto_solve_debounce_ms;
            if (elapsed_ms < debounce_ms) continue;
            scene_solver_request_recalculate(scene, sketch_entity);
        }
    }
    ecs_query_fini(q);
}

static inline bool scene_solver_request_recalculate(ecs_scene_t *scene, ecs_entity_t sketch) {
    if (!scene || !scene_is_sketch(scene, sketch)) return false;
    SketchComp *sk = ecs_world_get_sketch(scene->world, sketch);
    if (!sk) return false;
    bool preserve_drag_anchor = sk->solver_drag_anchor_valid;
    sk->auto_solve_pending = false;
    sk->auto_solve_queued_at_ms = 0;
    sk->auto_solve_queue_token = 0;

    ecs_entity_t sketch_points[ECS_SCENE_SOLVER_MAX_IMPLICATED_PARTICIPANTS] = {0};
    int point_count = 0;
    ecs_entity_t implicated_constraints[ECS_SCENE_SOLVER_MAX_IMPLICATED_CONSTRAINTS] = {0};
    int implicated_constraint_count = 0;
    bool solve_failed = false;
    bool converged = false;
    const char *failure_reason = "Unsupported constraint participant combination.";
    float position_tolerance = (sk->solver_position_tolerance > 0.0f)
        ? sk->solver_position_tolerance
        : SKETCH_SOLVER_DEFAULT_POSITION_TOLERANCE;
    float angle_tolerance = (sk->solver_angle_tolerance > 0.0f)
        ? sk->solver_angle_tolerance
        : SKETCH_SOLVER_DEFAULT_ANGLE_TOLERANCE;
    uint32_t max_passes = (sk->solver_max_passes > 0)
        ? sk->solver_max_passes
        : SKETCH_SOLVER_DEFAULT_MAX_PASSES;

    ecs_iter_t children = ecs_children(scene->world->world, sketch);
    while (ecs_children_next(&children)) {
        for (int i = 0; i < children.count; i++) {
            ecs_entity_t child = children.entities[i];
            GeometryComp *g = ecs_world_get_geometry(scene->world, child);
            if (!g || g->type != GEOM_POINT) continue;
            if (point_count < ECS_SCENE_SOLVER_MAX_IMPLICATED_PARTICIPANTS) {
                sketch_points[point_count++] = child;
            }
        }
    }

    scene_solver_point_candidate_t candidates[ECS_SCENE_SOLVER_MAX_IMPLICATED_PARTICIPANTS] = {0};
    int candidate_count = 0;
    scene_solver_arc_normal_candidate_t arc_normal_candidates[ECS_SCENE_SOLVER_MAX_IMPLICATED_PARTICIPANTS] = {0};
    int arc_normal_count = 0;
    for (int i = 0; i < point_count; i++) {
        scene_solver_ensure_point_candidate(scene, candidates, ECS_SCENE_SOLVER_MAX_IMPLICATED_PARTICIPANTS,
                                           &candidate_count, sketch_points[i], CONSTRAINT_PARTICIPANT_ROLE_ENTITY);
    }

    for (uint32_t pass = 0; pass < max_passes && !solve_failed; pass++) {
        float pass_max_position_delta = 0.0f;
        float pass_max_angle_delta = 0.0f;
        float pass_max_residual = 0.0f;
        children = ecs_children(scene->world->world, sketch);
        while (ecs_children_next(&children) && !solve_failed) {
            for (int i = 0; i < children.count; i++) {
                ecs_entity_t child = children.entities[i];
                ConstraintComp *constraint = ecs_world_get_constraint(scene->world, child);
                if (!constraint) continue;

                uint32_t participant_count = constraint->participant_count;
                if (participant_count > CONSTRAINT_MAX_PARTICIPANTS) {
                    participant_count = CONSTRAINT_MAX_PARTICIPANTS;
                }

                if (constraint->type == CONSTRAINT_COINCIDENT && participant_count == 2) {
                    ecs_entity_t pa = (ecs_entity_t)constraint->participant_descriptors[0].entity;
                    ecs_entity_t pb = (ecs_entity_t)constraint->participant_descriptors[1].entity;
                    constraint_participant_role_t role_a =
                        (constraint_participant_role_t)constraint->participant_descriptors[0].role;
                    constraint_participant_role_t role_b =
                        (constraint_participant_role_t)constraint->participant_descriptors[1].role;
                    int ia = scene_solver_ensure_point_candidate(scene, candidates,
                                                                 ECS_SCENE_SOLVER_MAX_IMPLICATED_PARTICIPANTS,
                                                                 &candidate_count, pa, role_a);
                    int ib = scene_solver_ensure_point_candidate(scene, candidates,
                                                                 ECS_SCENE_SOLVER_MAX_IMPLICATED_PARTICIPANTS,
                                                                 &candidate_count, pb, role_b);
                    if (ia < 0 || ib < 0) {
                        solve_failed = true;
                        failure_reason = "Unsupported coincident participants: only point-point is solved in this phase.";
                        implicated_constraints[implicated_constraint_count++] = child;
                        break;
                    }

                    vec3_t delta = vec3_sub(candidates[ia].point, candidates[ib].point);
                    float distance = vec3_length(delta);
                    if (distance > pass_max_residual) pass_max_residual = distance;

                    if (candidates[ia].fixed && candidates[ib].fixed) {
                        if (distance > position_tolerance) {
                            solve_failed = true;
                            failure_reason = "Unsatisfied coincident constraint.";
                            implicated_constraints[implicated_constraint_count++] = child;
                            break;
                        }
                        continue;
                    }

                    vec3_t before_a = candidates[ia].point;
                    vec3_t before_b = candidates[ib].point;
                    if (candidates[ia].fixed && !candidates[ib].fixed) {
                        candidates[ib].point = candidates[ia].point;
                    } else if (!candidates[ia].fixed && candidates[ib].fixed) {
                        candidates[ia].point = candidates[ib].point;
                    } else {
                        vec3_t midpoint = vec3_scale(vec3_add(candidates[ia].point, candidates[ib].point), 0.5f);
                        candidates[ia].point = midpoint;
                        candidates[ib].point = midpoint;
                    }
                    float delta_a = vec3_length(vec3_sub(candidates[ia].point, before_a));
                    float delta_b = vec3_length(vec3_sub(candidates[ib].point, before_b));
                    if (delta_a > pass_max_position_delta) pass_max_position_delta = delta_a;
                    if (delta_b > pass_max_position_delta) pass_max_position_delta = delta_b;
                    continue;
                }

                if (constraint->type == CONSTRAINT_LENGTH) {
                    if (constraint->driven) continue;
                    if (participant_count < 1) {
                        solve_failed = true;
                        failure_reason = "Unsatisfied driving LENGTH constraint.";
                        implicated_constraints[implicated_constraint_count++] = child;
                        break;
                    }
                    ecs_entity_t line_entity = (ecs_entity_t)constraint->participant_descriptors[0].entity;
                    int ia = scene_solver_ensure_point_candidate(scene, candidates,
                                                                 ECS_SCENE_SOLVER_MAX_IMPLICATED_PARTICIPANTS,
                                                                 &candidate_count, line_entity,
                                                                 CONSTRAINT_PARTICIPANT_ROLE_POINT_A);
                    int ib = scene_solver_ensure_point_candidate(scene, candidates,
                                                                 ECS_SCENE_SOLVER_MAX_IMPLICATED_PARTICIPANTS,
                                                                 &candidate_count, line_entity,
                                                                 CONSTRAINT_PARTICIPANT_ROLE_POINT_B);
                    if (ia < 0 || ib < 0) {
                        solve_failed = true;
                        failure_reason = "Unsatisfied driving LENGTH constraint.";
                        implicated_constraints[implicated_constraint_count++] = child;
                        break;
                    }
                    float desired_length = constraint->value;
                    if (!isfinite(desired_length) || desired_length <= 0.0f) {
                        solve_failed = true;
                        failure_reason = "Unsatisfied driving LENGTH constraint.";
                        implicated_constraints[implicated_constraint_count++] = child;
                        break;
                    }

                    vec3_t line_delta = vec3_sub(candidates[ib].point, candidates[ia].point);
                    float current_length = vec3_length(line_delta);
                    float residual = fabsf(current_length - desired_length);
                    if (residual > pass_max_residual) pass_max_residual = residual;

                    if (candidates[ia].fixed && candidates[ib].fixed) {
                        implicated_constraints[implicated_constraint_count++] = child;
                        continue;
                    }

                    vec3_t direction = (current_length > 1e-6f)
                        ? vec3_scale(line_delta, 1.0f / current_length)
                        : vec3_make(1.0f, 0.0f, 0.0f);
                    vec3_t before_a = candidates[ia].point;
                    vec3_t before_b = candidates[ib].point;
                    if (candidates[ia].fixed && !candidates[ib].fixed) {
                        candidates[ib].point = vec3_add(candidates[ia].point, vec3_scale(direction, desired_length));
                    } else if (!candidates[ia].fixed && candidates[ib].fixed) {
                        candidates[ia].point = vec3_sub(candidates[ib].point, vec3_scale(direction, desired_length));
                    } else {
                        candidates[ib].point = vec3_add(candidates[ia].point, vec3_scale(direction, desired_length));
                    }
                    float delta_a = vec3_length(vec3_sub(candidates[ia].point, before_a));
                    float delta_b = vec3_length(vec3_sub(candidates[ib].point, before_b));
                    if (delta_a > pass_max_position_delta) pass_max_position_delta = delta_a;
                    if (delta_b > pass_max_position_delta) pass_max_position_delta = delta_b;
                    continue;
                }

                if (constraint->type == CONSTRAINT_ALONG_X ||
                    constraint->type == CONSTRAINT_ALONG_Y ||
                    constraint->type == CONSTRAINT_ALONG_Z) {
                    if (constraint->driven) continue;
                    int free_axis = 0;
                    const char *along_unsat_reason = "Unsatisfied driving ALONG X constraint.";
                    if (constraint->type == CONSTRAINT_ALONG_Y) {
                        free_axis = 1;
                        along_unsat_reason = "Unsatisfied driving ALONG Y constraint.";
                    } else if (constraint->type == CONSTRAINT_ALONG_Z) {
                        free_axis = 2;
                        along_unsat_reason = "Unsatisfied driving ALONG Z constraint.";
                    }
                    int locked_axis_a = (free_axis == 0) ? 1 : 0;
                    int locked_axis_b = (free_axis == 2) ? 1 : 2;

                    if (participant_count < 2) {
                        solve_failed = true;
                        failure_reason = along_unsat_reason;
                        implicated_constraints[implicated_constraint_count++] = child;
                        break;
                    }

                    int along_indices[CONSTRAINT_MAX_PARTICIPANTS] = {0};
                    int along_count = 0;
                    for (uint32_t p = 0; p < participant_count; p++) {
                        ecs_entity_t participant_entity =
                            (ecs_entity_t)constraint->participant_descriptors[p].entity;
                        constraint_participant_role_t participant_role =
                            (constraint_participant_role_t)constraint->participant_descriptors[p].role;
                        int idx = scene_solver_ensure_point_candidate(scene, candidates,
                                                                      ECS_SCENE_SOLVER_MAX_IMPLICATED_PARTICIPANTS,
                                                                      &candidate_count,
                                                                      participant_entity,
                                                                      participant_role);
                        if (idx < 0) {
                            solve_failed = true;
                            failure_reason = along_unsat_reason;
                            implicated_constraints[implicated_constraint_count++] = child;
                            break;
                        }
                        bool seen = false;
                        for (int ai = 0; ai < along_count; ai++) {
                            if (along_indices[ai] == idx) {
                                seen = true;
                                break;
                            }
                        }
                        if (!seen && along_count < (int)CONSTRAINT_MAX_PARTICIPANTS) {
                            along_indices[along_count++] = idx;
                        }
                    }
                    if (solve_failed) break;
                    if (along_count < 2) {
                        solve_failed = true;
                        failure_reason = along_unsat_reason;
                        implicated_constraints[implicated_constraint_count++] = child;
                        break;
                    }

                    float target_coord_a = 0.0f;
                    float target_coord_b = 0.0f;
                    for (int ai = 0; ai < along_count; ai++) {
                        vec3_t pt = candidates[along_indices[ai]].point;
                        float coord_a = (locked_axis_a == 0) ? pt.x : ((locked_axis_a == 1) ? pt.y : pt.z);
                        float coord_b = (locked_axis_b == 0) ? pt.x : ((locked_axis_b == 1) ? pt.y : pt.z);
                        target_coord_a += coord_a;
                        target_coord_b += coord_b;
                    }
                    target_coord_a /= (float)along_count;
                    target_coord_b /= (float)along_count;

                    float residual = 0.0f;
                    bool all_fixed = true;
                    for (int ai = 0; ai < along_count; ai++) {
                        scene_solver_point_candidate_t *entry = &candidates[along_indices[ai]];
                        float coord_a = (locked_axis_a == 0)
                            ? entry->point.x
                            : ((locked_axis_a == 1) ? entry->point.y : entry->point.z);
                        float coord_b = (locked_axis_b == 0)
                            ? entry->point.x
                            : ((locked_axis_b == 1) ? entry->point.y : entry->point.z);
                        float abs_residual_a = fabsf(coord_a - target_coord_a);
                        float abs_residual_b = fabsf(coord_b - target_coord_b);
                        if (abs_residual_a > residual) residual = abs_residual_a;
                        if (abs_residual_b > residual) residual = abs_residual_b;
                        if (!entry->fixed) all_fixed = false;
                    }
                    if (residual > pass_max_residual) pass_max_residual = residual;

                    if (all_fixed) {
                        if (residual > position_tolerance) {
                            solve_failed = true;
                            failure_reason = along_unsat_reason;
                            implicated_constraints[implicated_constraint_count++] = child;
                            break;
                        }
                        continue;
                    }

                    for (int ai = 0; ai < along_count; ai++) {
                        scene_solver_point_candidate_t *entry = &candidates[along_indices[ai]];
                        if (entry->fixed) continue;
                        vec3_t before = entry->point;
                        if (locked_axis_a == 0) entry->point.x = target_coord_a;
                        else if (locked_axis_a == 1) entry->point.y = target_coord_a;
                        else entry->point.z = target_coord_a;
                        if (locked_axis_b == 0) entry->point.x = target_coord_b;
                        else if (locked_axis_b == 1) entry->point.y = target_coord_b;
                        else entry->point.z = target_coord_b;
                        float delta = vec3_length(vec3_sub(entry->point, before));
                        if (delta > pass_max_position_delta) pass_max_position_delta = delta;
                    }
                    continue;
                }

                if ((constraint->type == CONSTRAINT_PARALLEL ||
                     constraint->type == CONSTRAINT_PERPENDICULAR) &&
                    participant_count == 2) {
                    const char *line_line_unsat_reason =
                        (constraint->type == CONSTRAINT_PARALLEL)
                            ? "Unsatisfied parallel constraint."
                            : "Unsatisfied perpendicular constraint.";
                    ecs_entity_t line_a_entity = (ecs_entity_t)constraint->participant_descriptors[0].entity;
                    ecs_entity_t line_b_entity = (ecs_entity_t)constraint->participant_descriptors[1].entity;
                    int ia0 = scene_solver_ensure_point_candidate(scene, candidates,
                                                                  ECS_SCENE_SOLVER_MAX_IMPLICATED_PARTICIPANTS,
                                                                  &candidate_count, line_a_entity,
                                                                  CONSTRAINT_PARTICIPANT_ROLE_POINT_A);
                    int ia1 = scene_solver_ensure_point_candidate(scene, candidates,
                                                                  ECS_SCENE_SOLVER_MAX_IMPLICATED_PARTICIPANTS,
                                                                  &candidate_count, line_a_entity,
                                                                  CONSTRAINT_PARTICIPANT_ROLE_POINT_B);
                    int ib0 = scene_solver_ensure_point_candidate(scene, candidates,
                                                                  ECS_SCENE_SOLVER_MAX_IMPLICATED_PARTICIPANTS,
                                                                  &candidate_count, line_b_entity,
                                                                  CONSTRAINT_PARTICIPANT_ROLE_POINT_A);
                    int ib1 = scene_solver_ensure_point_candidate(scene, candidates,
                                                                  ECS_SCENE_SOLVER_MAX_IMPLICATED_PARTICIPANTS,
                                                                  &candidate_count, line_b_entity,
                                                                  CONSTRAINT_PARTICIPANT_ROLE_POINT_B);
                    if (ia0 < 0 || ia1 < 0 || ib0 < 0 || ib1 < 0) {
                        solve_failed = true;
                        failure_reason = line_line_unsat_reason;
                        implicated_constraints[implicated_constraint_count++] = child;
                        break;
                    }

                    vec3_t va = vec3_sub(candidates[ia1].point, candidates[ia0].point);
                    vec3_t vb = vec3_sub(candidates[ib1].point, candidates[ib0].point);
                    float len_a = vec3_length(va);
                    float len_b = vec3_length(vb);
                    if (len_a <= 1e-6f || len_b <= 1e-6f || !isfinite(len_a) || !isfinite(len_b)) {
                        solve_failed = true;
                        failure_reason = line_line_unsat_reason;
                        implicated_constraints[implicated_constraint_count++] = child;
                        break;
                    }

                    vec3_t na = vec3_scale(va, 1.0f / len_a);
                    vec3_t nb = vec3_scale(vb, 1.0f / len_b);
                    float dot = vec3_dot(na, nb);
                    if (dot > 1.0f) dot = 1.0f;
                    if (dot < -1.0f) dot = -1.0f;
                    float residual = 0.0f;
                    if (constraint->type == CONSTRAINT_PARALLEL) {
                        residual = acosf(fabsf(dot));
                    } else {
                        residual = fabsf(dot);
                    }
                    if (residual > pass_max_residual) pass_max_residual = residual;
                    if (residual > pass_max_angle_delta) pass_max_angle_delta = residual;
                    if (residual <= angle_tolerance) {
                        continue;
                    }

                    bool line_a_fixed = candidates[ia0].fixed && candidates[ia1].fixed;
                    bool line_b_fixed = candidates[ib0].fixed && candidates[ib1].fixed;
                    if (line_a_fixed && line_b_fixed) {
                        solve_failed = true;
                        failure_reason = line_line_unsat_reason;
                        implicated_constraints[implicated_constraint_count++] = child;
                        break;
                    }

                    vec3_t before_ia0 = candidates[ia0].point;
                    vec3_t before_ia1 = candidates[ia1].point;
                    vec3_t before_ib0 = candidates[ib0].point;
                    vec3_t before_ib1 = candidates[ib1].point;
                    bool updated = false;
                    if (!line_b_fixed) {
                        vec3_t target_dir = na;
                        if (constraint->type == CONSTRAINT_PARALLEL) {
                            vec3_t target_opposite = vec3_scale(target_dir, -1.0f);
                            target_dir = (vec3_dot(target_dir, nb) >= vec3_dot(target_opposite, nb))
                                             ? target_dir
                                             : target_opposite;
                        } else {
                            vec3_t perp_axis = vec3_cross(na, nb);
                            float perp_len = vec3_length(perp_axis);
                            if (perp_len <= 1e-6f || !isfinite(perp_len)) {
                                vec3_t fallback = (fabsf(na.z) < 0.9f)
                                                      ? vec3_make(0.0f, 0.0f, 1.0f)
                                                      : vec3_make(0.0f, 1.0f, 0.0f);
                                perp_axis = vec3_cross(na, fallback);
                                perp_len = vec3_length(perp_axis);
                            }
                            if (perp_len <= 1e-6f || !isfinite(perp_len)) {
                                solve_failed = true;
                                failure_reason = line_line_unsat_reason;
                                implicated_constraints[implicated_constraint_count++] = child;
                                break;
                            }
                            perp_axis = vec3_scale(perp_axis, 1.0f / perp_len);
                            vec3_t perp_dir = vec3_normalize(vec3_cross(perp_axis, na));
                            vec3_t perp_dir_opposite = vec3_scale(perp_dir, -1.0f);
                            target_dir = (vec3_dot(perp_dir, nb) >= vec3_dot(perp_dir_opposite, nb))
                                             ? perp_dir
                                             : perp_dir_opposite;
                        }
                        if (!candidates[ib1].fixed) {
                            candidates[ib1].point = vec3_add(candidates[ib0].point, vec3_scale(target_dir, len_b));
                            updated = true;
                        } else if (!candidates[ib0].fixed) {
                            candidates[ib0].point = vec3_sub(candidates[ib1].point, vec3_scale(target_dir, len_b));
                            updated = true;
                        }
                    } else if (!line_a_fixed) {
                        vec3_t target_dir = nb;
                        if (constraint->type == CONSTRAINT_PARALLEL) {
                            vec3_t target_opposite = vec3_scale(target_dir, -1.0f);
                            target_dir = (vec3_dot(target_dir, na) >= vec3_dot(target_opposite, na))
                                             ? target_dir
                                             : target_opposite;
                        } else {
                            vec3_t perp_axis = vec3_cross(nb, na);
                            float perp_len = vec3_length(perp_axis);
                            if (perp_len <= 1e-6f || !isfinite(perp_len)) {
                                vec3_t fallback = (fabsf(nb.z) < 0.9f)
                                                      ? vec3_make(0.0f, 0.0f, 1.0f)
                                                      : vec3_make(0.0f, 1.0f, 0.0f);
                                perp_axis = vec3_cross(nb, fallback);
                                perp_len = vec3_length(perp_axis);
                            }
                            if (perp_len <= 1e-6f || !isfinite(perp_len)) {
                                solve_failed = true;
                                failure_reason = line_line_unsat_reason;
                                implicated_constraints[implicated_constraint_count++] = child;
                                break;
                            }
                            perp_axis = vec3_scale(perp_axis, 1.0f / perp_len);
                            vec3_t perp_dir = vec3_normalize(vec3_cross(perp_axis, nb));
                            vec3_t perp_dir_opposite = vec3_scale(perp_dir, -1.0f);
                            target_dir = (vec3_dot(perp_dir, na) >= vec3_dot(perp_dir_opposite, na))
                                             ? perp_dir
                                             : perp_dir_opposite;
                        }
                        if (!candidates[ia1].fixed) {
                            candidates[ia1].point = vec3_add(candidates[ia0].point, vec3_scale(target_dir, len_a));
                            updated = true;
                        } else if (!candidates[ia0].fixed) {
                            candidates[ia0].point = vec3_sub(candidates[ia1].point, vec3_scale(target_dir, len_a));
                            updated = true;
                        }
                    }

                    if (!updated) {
                        solve_failed = true;
                        failure_reason = line_line_unsat_reason;
                        implicated_constraints[implicated_constraint_count++] = child;
                        break;
                    }

                    float delta_ia0 = vec3_length(vec3_sub(candidates[ia0].point, before_ia0));
                    float delta_ia1 = vec3_length(vec3_sub(candidates[ia1].point, before_ia1));
                    float delta_ib0 = vec3_length(vec3_sub(candidates[ib0].point, before_ib0));
                    float delta_ib1 = vec3_length(vec3_sub(candidates[ib1].point, before_ib1));
                    if (delta_ia0 > pass_max_position_delta) pass_max_position_delta = delta_ia0;
                    if (delta_ia1 > pass_max_position_delta) pass_max_position_delta = delta_ia1;
                    if (delta_ib0 > pass_max_position_delta) pass_max_position_delta = delta_ib0;
                    if (delta_ib1 > pass_max_position_delta) pass_max_position_delta = delta_ib1;
                    continue;
                }

                if (constraint->type == CONSTRAINT_ANGLE) {
                    if (constraint->driven) continue;
                    if (participant_count < 2) {
                        solve_failed = true;
                        failure_reason = "Unsatisfied driving ANGLE constraint.";
                        implicated_constraints[implicated_constraint_count++] = child;
                        break;
                    }

                    ecs_entity_t line_a_entity = (ecs_entity_t)constraint->participant_descriptors[0].entity;
                    ecs_entity_t line_b_entity = (ecs_entity_t)constraint->participant_descriptors[1].entity;
                    int ia0 = scene_solver_ensure_point_candidate(scene, candidates,
                                                                  ECS_SCENE_SOLVER_MAX_IMPLICATED_PARTICIPANTS,
                                                                  &candidate_count, line_a_entity,
                                                                  CONSTRAINT_PARTICIPANT_ROLE_POINT_A);
                    int ia1 = scene_solver_ensure_point_candidate(scene, candidates,
                                                                  ECS_SCENE_SOLVER_MAX_IMPLICATED_PARTICIPANTS,
                                                                  &candidate_count, line_a_entity,
                                                                  CONSTRAINT_PARTICIPANT_ROLE_POINT_B);
                    int ib0 = scene_solver_ensure_point_candidate(scene, candidates,
                                                                  ECS_SCENE_SOLVER_MAX_IMPLICATED_PARTICIPANTS,
                                                                  &candidate_count, line_b_entity,
                                                                  CONSTRAINT_PARTICIPANT_ROLE_POINT_A);
                    int ib1 = scene_solver_ensure_point_candidate(scene, candidates,
                                                                  ECS_SCENE_SOLVER_MAX_IMPLICATED_PARTICIPANTS,
                                                                  &candidate_count, line_b_entity,
                                                                  CONSTRAINT_PARTICIPANT_ROLE_POINT_B);
                    if (ia0 < 0 || ia1 < 0 || ib0 < 0 || ib1 < 0) {
                        solve_failed = true;
                        failure_reason = "Unsatisfied driving ANGLE constraint.";
                        implicated_constraints[implicated_constraint_count++] = child;
                        break;
                    }

                    float desired_angle = constraint->value;
                    if (!isfinite(desired_angle) || desired_angle < 0.0f || desired_angle > 3.14159265359f) {
                        solve_failed = true;
                        failure_reason = "Unsatisfied driving ANGLE constraint.";
                        implicated_constraints[implicated_constraint_count++] = child;
                        break;
                    }

                    vec3_t va = vec3_sub(candidates[ia1].point, candidates[ia0].point);
                    vec3_t vb = vec3_sub(candidates[ib1].point, candidates[ib0].point);
                    float len_a = vec3_length(va);
                    float len_b = vec3_length(vb);
                    if (len_a <= 1e-6f || len_b <= 1e-6f) {
                        solve_failed = true;
                        failure_reason = "Unsatisfied driving ANGLE constraint.";
                        implicated_constraints[implicated_constraint_count++] = child;
                        break;
                    }

                    vec3_t na = vec3_scale(va, 1.0f / len_a);
                    vec3_t nb = vec3_scale(vb, 1.0f / len_b);
                    float cos_current = vec3_dot(na, nb);
                    if (cos_current > 1.0f) cos_current = 1.0f;
                    if (cos_current < -1.0f) cos_current = -1.0f;
                    float current_angle = acosf(cos_current);
                    float residual = fabsf(current_angle - desired_angle);
                    if (residual > pass_max_residual) pass_max_residual = residual;
                    if (residual > pass_max_angle_delta) pass_max_angle_delta = residual;

                    bool b0_fixed = candidates[ib0].fixed;
                    bool b1_fixed = candidates[ib1].fixed;
                    bool a0_fixed = candidates[ia0].fixed;
                    bool a1_fixed = candidates[ia1].fixed;
                    if ((b0_fixed && b1_fixed && a0_fixed && a1_fixed) && residual > angle_tolerance) {
                        solve_failed = true;
                        failure_reason = "Unsatisfied driving ANGLE constraint.";
                        implicated_constraints[implicated_constraint_count++] = child;
                        break;
                    }
                    if (residual <= angle_tolerance) {
                        continue;
                    }

                    vec3_t plane_n = vec3_cross(na, nb);
                    float plane_n_len = vec3_length(plane_n);
                    if (plane_n_len <= 1e-6f) {
                        vec3_t fallback = (fabsf(na.z) < 0.9f) ? vec3_make(0.0f, 0.0f, 1.0f) : vec3_make(0.0f, 1.0f, 0.0f);
                        plane_n = vec3_cross(na, fallback);
                        plane_n_len = vec3_length(plane_n);
                        if (plane_n_len <= 1e-6f) {
                            fallback = vec3_make(1.0f, 0.0f, 0.0f);
                            plane_n = vec3_cross(na, fallback);
                            plane_n_len = vec3_length(plane_n);
                        }
                    }
                    if (plane_n_len <= 1e-6f) {
                        solve_failed = true;
                        failure_reason = "Unsatisfied driving ANGLE constraint.";
                        implicated_constraints[implicated_constraint_count++] = child;
                        break;
                    }
                    plane_n = vec3_scale(plane_n, 1.0f / plane_n_len);
                    vec3_t perp = vec3_cross(plane_n, na);
                    float perp_len = vec3_length(perp);
                    if (perp_len <= 1e-6f) {
                        solve_failed = true;
                        failure_reason = "Unsatisfied driving ANGLE constraint.";
                        implicated_constraints[implicated_constraint_count++] = child;
                        break;
                    }
                    perp = vec3_scale(perp, 1.0f / perp_len);

                    float c = cosf(desired_angle);
                    float s = sinf(desired_angle);
                    vec3_t dir_plus = vec3_add(vec3_scale(na, c), vec3_scale(perp, s));
                    vec3_t dir_minus = vec3_add(vec3_scale(na, c), vec3_scale(perp, -s));
                    float plus_len = vec3_length(dir_plus);
                    float minus_len = vec3_length(dir_minus);
                    if (plus_len <= 1e-6f || minus_len <= 1e-6f) {
                        solve_failed = true;
                        failure_reason = "Unsatisfied driving ANGLE constraint.";
                        implicated_constraints[implicated_constraint_count++] = child;
                        break;
                    }
                    dir_plus = vec3_scale(dir_plus, 1.0f / plus_len);
                    dir_minus = vec3_scale(dir_minus, 1.0f / minus_len);
                    vec3_t target_dir = (vec3_dot(dir_plus, nb) >= vec3_dot(dir_minus, nb)) ? dir_plus : dir_minus;

                    vec3_t before_a0 = candidates[ia0].point;
                    vec3_t before_a1 = candidates[ia1].point;
                    vec3_t before_b0 = candidates[ib0].point;
                    vec3_t before_b1 = candidates[ib1].point;
                    bool updated = false;
                    if (!b1_fixed) {
                        candidates[ib1].point = vec3_add(candidates[ib0].point, vec3_scale(target_dir, len_b));
                        updated = true;
                    } else if (!b0_fixed) {
                        candidates[ib0].point = vec3_sub(candidates[ib1].point, vec3_scale(target_dir, len_b));
                        updated = true;
                    } else if (!a1_fixed) {
                        vec3_t target_na = vec3_sub(vec3_scale(target_dir, cosf(desired_angle)),
                                                    vec3_scale(perp, sinf(desired_angle)));
                        float target_na_len = vec3_length(target_na);
                        if (target_na_len <= 1e-6f) {
                            solve_failed = true;
                            failure_reason = "Unsatisfied driving ANGLE constraint.";
                            implicated_constraints[implicated_constraint_count++] = child;
                            break;
                        }
                        target_na = vec3_scale(target_na, 1.0f / target_na_len);
                        candidates[ia1].point = vec3_add(candidates[ia0].point, vec3_scale(target_na, len_a));
                        updated = true;
                    } else if (!a0_fixed) {
                        vec3_t target_na = vec3_sub(vec3_scale(target_dir, cosf(desired_angle)),
                                                    vec3_scale(perp, sinf(desired_angle)));
                        float target_na_len = vec3_length(target_na);
                        if (target_na_len <= 1e-6f) {
                            solve_failed = true;
                            failure_reason = "Unsatisfied driving ANGLE constraint.";
                            implicated_constraints[implicated_constraint_count++] = child;
                            break;
                        }
                        target_na = vec3_scale(target_na, 1.0f / target_na_len);
                        candidates[ia0].point = vec3_sub(candidates[ia1].point, vec3_scale(target_na, len_a));
                        updated = true;
                    }

                    if (!updated) {
                        solve_failed = true;
                        failure_reason = "Unsatisfied driving ANGLE constraint.";
                        implicated_constraints[implicated_constraint_count++] = child;
                        break;
                    }

                    float delta_a0 = vec3_length(vec3_sub(candidates[ia0].point, before_a0));
                    float delta_a1 = vec3_length(vec3_sub(candidates[ia1].point, before_a1));
                    float delta_b0 = vec3_length(vec3_sub(candidates[ib0].point, before_b0));
                    float delta_b1 = vec3_length(vec3_sub(candidates[ib1].point, before_b1));
                    if (delta_a0 > pass_max_position_delta) pass_max_position_delta = delta_a0;
                    if (delta_a1 > pass_max_position_delta) pass_max_position_delta = delta_a1;
                    if (delta_b0 > pass_max_position_delta) pass_max_position_delta = delta_b0;
                    if (delta_b1 > pass_max_position_delta) pass_max_position_delta = delta_b1;
                    continue;
                }

                if (constraint->type == CONSTRAINT_ARC_AXIS_LINE) {
                    if (participant_count < 2) {
                        solve_failed = true;
                        failure_reason = "Unsatisfied arc-axis-vs-line constraint.";
                        implicated_constraints[implicated_constraint_count++] = child;
                        break;
                    }

                    int line_participant = -1;
                    int arc_participant = -1;
                    for (uint32_t p = 0; p < participant_count; p++) {
                        ecs_entity_t participant_entity =
                            (ecs_entity_t)constraint->participant_descriptors[p].entity;
                        constraint_participant_role_t participant_role =
                            (constraint_participant_role_t)constraint->participant_descriptors[p].role;
                        GeometryComp *participant_geom = ecs_world_get_geometry(scene->world, participant_entity);
                        if (!participant_geom) continue;
                        if (participant_geom->type == GEOM_LINE &&
                            participant_role == CONSTRAINT_PARTICIPANT_ROLE_ENTITY &&
                            line_participant < 0) {
                            line_participant = (int)p;
                        } else if (participant_geom->type == GEOM_ARC &&
                                   participant_role == CONSTRAINT_PARTICIPANT_ROLE_ENTITY &&
                                   arc_participant < 0) {
                            arc_participant = (int)p;
                        }
                    }
                    if (line_participant < 0 || arc_participant < 0) {
                        solve_failed = true;
                        failure_reason = "Unsatisfied arc-axis-vs-line constraint.";
                        implicated_constraints[implicated_constraint_count++] = child;
                        break;
                    }

                    vec3_t line_dir = vec3_make(0.0f, 0.0f, 0.0f);
                    bool line_fixed = false;
                    if (!scene_solver_extract_line_direction_from_constraint(
                            scene,
                            candidates,
                            ECS_SCENE_SOLVER_MAX_IMPLICATED_PARTICIPANTS,
                            &candidate_count,
                            &constraint->participant_descriptors[line_participant],
                            &line_dir,
                            &line_fixed)) {
                        solve_failed = true;
                        failure_reason = "Unsatisfied arc-axis-vs-line constraint.";
                        implicated_constraints[implicated_constraint_count++] = child;
                        break;
                    }

                    ecs_entity_t arc_entity = (ecs_entity_t)constraint->participant_descriptors[arc_participant].entity;
                    int arc_normal_index = scene_solver_ensure_arc_normal_candidate(
                        scene,
                        arc_normal_candidates,
                        ECS_SCENE_SOLVER_MAX_IMPLICATED_PARTICIPANTS,
                        &arc_normal_count,
                        arc_entity);
                    if (arc_normal_index < 0) {
                        solve_failed = true;
                        failure_reason = "Unsatisfied arc-axis-vs-line constraint.";
                        implicated_constraints[implicated_constraint_count++] = child;
                        break;
                    }
                    scene_solver_arc_normal_candidate_t *arc_normal = &arc_normal_candidates[arc_normal_index];

                    float dot = vec3_dot(arc_normal->normal, line_dir);
                    if (dot > 1.0f) dot = 1.0f;
                    if (dot < -1.0f) dot = -1.0f;
                    float abs_dot = fabsf(dot);
                    if (abs_dot > 1.0f) abs_dot = 1.0f;
                    float residual = acosf(abs_dot);
                    if (residual > pass_max_residual) pass_max_residual = residual;
                    if (residual > pass_max_angle_delta) pass_max_angle_delta = residual;
                    if (residual <= angle_tolerance) {
                        continue;
                    }

                    if (!arc_normal->fixed) {
                        vec3_t target_normal = (dot >= 0.0f) ? line_dir : vec3_scale(line_dir, -1.0f);
                        float normal_residual = 0.0f;
                        if (!scene_solver_try_set_arc_normal(arc_normal, target_normal,
                                                             angle_tolerance,
                                                             &pass_max_angle_delta,
                                                             &normal_residual)) {
                            solve_failed = true;
                            failure_reason = "Unsatisfied arc-axis-vs-line constraint.";
                            implicated_constraints[implicated_constraint_count++] = child;
                            break;
                        }
                        if (normal_residual > pass_max_residual) pass_max_residual = normal_residual;
                        continue;
                    }

                    if (line_fixed) {
                        solve_failed = true;
                        failure_reason = "Unsatisfied arc-axis-vs-line constraint.";
                        implicated_constraints[implicated_constraint_count++] = child;
                        break;
                    }

                    ecs_entity_t line_entity = (ecs_entity_t)constraint->participant_descriptors[line_participant].entity;
                    int line_a_index = scene_solver_ensure_point_candidate(
                        scene, candidates, ECS_SCENE_SOLVER_MAX_IMPLICATED_PARTICIPANTS,
                        &candidate_count, line_entity, CONSTRAINT_PARTICIPANT_ROLE_POINT_A);
                    int line_b_index = scene_solver_ensure_point_candidate(
                        scene, candidates, ECS_SCENE_SOLVER_MAX_IMPLICATED_PARTICIPANTS,
                        &candidate_count, line_entity, CONSTRAINT_PARTICIPANT_ROLE_POINT_B);
                    if (line_a_index < 0 || line_b_index < 0) {
                        solve_failed = true;
                        failure_reason = "Unsatisfied arc-axis-vs-line constraint.";
                        implicated_constraints[implicated_constraint_count++] = child;
                        break;
                    }

                    bool line_a_fixed = candidates[line_a_index].fixed;
                    bool line_b_fixed = candidates[line_b_index].fixed;
                    if (line_a_fixed && line_b_fixed) {
                        solve_failed = true;
                        failure_reason = "Unsatisfied arc-axis-vs-line constraint.";
                        implicated_constraints[implicated_constraint_count++] = child;
                        break;
                    }

                    vec3_t line_delta = vec3_sub(candidates[line_b_index].point, candidates[line_a_index].point);
                    float line_len = vec3_length(line_delta);
                    if (line_len <= 1e-6f || !isfinite(line_len)) {
                        solve_failed = true;
                        failure_reason = "Unsatisfied arc-axis-vs-line constraint.";
                        implicated_constraints[implicated_constraint_count++] = child;
                        break;
                    }
                    vec3_t current_line_dir = vec3_scale(line_delta, 1.0f / line_len);
                    vec3_t line_target = (vec3_dot(current_line_dir, arc_normal->normal) >= 0.0f)
                        ? arc_normal->normal
                        : vec3_scale(arc_normal->normal, -1.0f);
                    float line_target_len = vec3_length(line_target);
                    if (line_target_len <= 1e-6f || !isfinite(line_target_len)) {
                        solve_failed = true;
                        failure_reason = "Unsatisfied arc-axis-vs-line constraint.";
                        implicated_constraints[implicated_constraint_count++] = child;
                        break;
                    }
                    line_target = vec3_scale(line_target, 1.0f / line_target_len);

                    vec3_t before_a = candidates[line_a_index].point;
                    vec3_t before_b = candidates[line_b_index].point;
                    if (!line_b_fixed) {
                        candidates[line_b_index].point =
                            vec3_add(candidates[line_a_index].point, vec3_scale(line_target, line_len));
                    } else {
                        candidates[line_a_index].point =
                            vec3_sub(candidates[line_b_index].point, vec3_scale(line_target, line_len));
                    }
                    float delta_a = vec3_length(vec3_sub(candidates[line_a_index].point, before_a));
                    float delta_b = vec3_length(vec3_sub(candidates[line_b_index].point, before_b));
                    if (delta_a > pass_max_position_delta) pass_max_position_delta = delta_a;
                    if (delta_b > pass_max_position_delta) pass_max_position_delta = delta_b;
                    continue;
                }

                if (constraint->type == CONSTRAINT_LINE_ARC_ENDPOINT_TANGENCY) {
                    if (participant_count < 2) {
                        solve_failed = true;
                        failure_reason = "Unsatisfied line-arc endpoint tangency constraint.";
                        implicated_constraints[implicated_constraint_count++] = child;
                        break;
                    }

                    int line_participant = -1;
                    int arc_participant = -1;
                    for (uint32_t p = 0; p < participant_count; p++) {
                        ecs_entity_t participant_entity =
                            (ecs_entity_t)constraint->participant_descriptors[p].entity;
                        constraint_participant_role_t participant_role =
                            (constraint_participant_role_t)constraint->participant_descriptors[p].role;
                        GeometryComp *participant_geom = ecs_world_get_geometry(scene->world, participant_entity);
                        if (!participant_geom) continue;
                        bool endpoint_role = participant_role == CONSTRAINT_PARTICIPANT_ROLE_POINT_A ||
                                             participant_role == CONSTRAINT_PARTICIPANT_ROLE_POINT_B;
                        if (participant_geom->type == GEOM_LINE && endpoint_role && line_participant < 0) {
                            line_participant = (int)p;
                        } else if (participant_geom->type == GEOM_ARC && endpoint_role && arc_participant < 0) {
                            arc_participant = (int)p;
                        }
                    }
                    if (line_participant < 0 || arc_participant < 0) {
                        solve_failed = true;
                        failure_reason = "Unsatisfied line-arc endpoint tangency constraint.";
                        implicated_constraints[implicated_constraint_count++] = child;
                        break;
                    }

                    ecs_entity_t line_entity = (ecs_entity_t)constraint->participant_descriptors[line_participant].entity;
                    constraint_participant_role_t line_role =
                        (constraint_participant_role_t)constraint->participant_descriptors[line_participant].role;
                    ecs_entity_t arc_entity = (ecs_entity_t)constraint->participant_descriptors[arc_participant].entity;
                    constraint_participant_role_t arc_role =
                        (constraint_participant_role_t)constraint->participant_descriptors[arc_participant].role;

                    constraint_participant_role_t other_line_role =
                        (line_role == CONSTRAINT_PARTICIPANT_ROLE_POINT_A)
                            ? CONSTRAINT_PARTICIPANT_ROLE_POINT_B
                            : CONSTRAINT_PARTICIPANT_ROLE_POINT_A;
                    int line_endpoint_index = scene_solver_ensure_point_candidate(
                        scene, candidates, ECS_SCENE_SOLVER_MAX_IMPLICATED_PARTICIPANTS,
                        &candidate_count, line_entity, line_role);
                    int line_other_index = scene_solver_ensure_point_candidate(
                        scene, candidates, ECS_SCENE_SOLVER_MAX_IMPLICATED_PARTICIPANTS,
                        &candidate_count, line_entity, other_line_role);
                    int arc_endpoint_index = scene_solver_ensure_point_candidate(
                        scene, candidates, ECS_SCENE_SOLVER_MAX_IMPLICATED_PARTICIPANTS,
                        &candidate_count, arc_entity, arc_role);
                    int arc_center_index = scene_solver_ensure_point_candidate(
                        scene, candidates, ECS_SCENE_SOLVER_MAX_IMPLICATED_PARTICIPANTS,
                        &candidate_count, arc_entity, CONSTRAINT_PARTICIPANT_ROLE_CENTER);
                    int arc_normal_index = scene_solver_ensure_arc_normal_candidate(
                        scene, arc_normal_candidates, ECS_SCENE_SOLVER_MAX_IMPLICATED_PARTICIPANTS,
                        &arc_normal_count, arc_entity);
                    if (line_endpoint_index < 0 || line_other_index < 0 ||
                        arc_endpoint_index < 0 || arc_center_index < 0 || arc_normal_index < 0) {
                        solve_failed = true;
                        failure_reason = "Unsatisfied line-arc endpoint tangency constraint.";
                        implicated_constraints[implicated_constraint_count++] = child;
                        break;
                    }

                    bool line_endpoint_fixed = candidates[line_endpoint_index].fixed;
                    bool arc_endpoint_fixed = candidates[arc_endpoint_index].fixed;

                    bool drag_anchor_is_line_endpoint = false;
                    bool drag_anchor_is_arc_endpoint = false;
                    if (sk->solver_drag_anchor_valid &&
                        sk->solver_drag_anchor_owner_entity != 0 &&
                        (sk->solver_drag_anchor_role == CONSTRAINT_PARTICIPANT_ROLE_POINT_A ||
                         sk->solver_drag_anchor_role == CONSTRAINT_PARTICIPANT_ROLE_POINT_B)) {
                        if ((ecs_entity_t)sk->solver_drag_anchor_owner_entity == line_entity &&
                            sk->solver_drag_anchor_role == line_role) {
                            drag_anchor_is_line_endpoint = true;
                        } else if ((ecs_entity_t)sk->solver_drag_anchor_owner_entity == arc_entity &&
                                   sk->solver_drag_anchor_role == arc_role) {
                            drag_anchor_is_arc_endpoint = true;
                        }
                    }

                    vec3_t before_line_endpoint = candidates[line_endpoint_index].point;
                    vec3_t before_arc_endpoint = candidates[arc_endpoint_index].point;
                    vec3_t coincidence_delta =
                        vec3_sub(candidates[line_endpoint_index].point, candidates[arc_endpoint_index].point);
                    float coincidence_distance = vec3_length(coincidence_delta);
                    if (coincidence_distance > pass_max_residual) pass_max_residual = coincidence_distance;
                    if (line_endpoint_fixed && arc_endpoint_fixed && coincidence_distance > position_tolerance) {
                        solve_failed = true;
                        failure_reason = "Unsatisfied line-arc endpoint tangency constraint.";
                        implicated_constraints[implicated_constraint_count++] = child;
                        break;
                    }
                    if (coincidence_distance > position_tolerance) {
                        if (drag_anchor_is_line_endpoint && !arc_endpoint_fixed) {
                            candidates[arc_endpoint_index].point = candidates[line_endpoint_index].point;
                        } else if (drag_anchor_is_arc_endpoint && !line_endpoint_fixed) {
                            candidates[line_endpoint_index].point = candidates[arc_endpoint_index].point;
                        } else if (line_endpoint_fixed && !arc_endpoint_fixed) {
                            candidates[arc_endpoint_index].point = candidates[line_endpoint_index].point;
                        } else if (!line_endpoint_fixed && arc_endpoint_fixed) {
                            candidates[line_endpoint_index].point = candidates[arc_endpoint_index].point;
                        } else if (!line_endpoint_fixed && !arc_endpoint_fixed) {
                            // Deterministic policy: line endpoint is anchor when both are editable.
                            candidates[arc_endpoint_index].point = candidates[line_endpoint_index].point;
                        }
                        float delta_line_endpoint = vec3_length(
                            vec3_sub(candidates[line_endpoint_index].point, before_line_endpoint));
                        float delta_arc_endpoint = vec3_length(
                            vec3_sub(candidates[arc_endpoint_index].point, before_arc_endpoint));
                        if (delta_line_endpoint > pass_max_position_delta) pass_max_position_delta = delta_line_endpoint;
                        if (delta_arc_endpoint > pass_max_position_delta) pass_max_position_delta = delta_arc_endpoint;
                    }

                    vec3_t shared_point = candidates[line_endpoint_index].point;
                    vec3_t radial = vec3_sub(shared_point, candidates[arc_center_index].point);
                    float radial_len = vec3_length(radial);
                    vec3_t line_vec = vec3_sub(candidates[line_other_index].point, shared_point);
                    float line_len = vec3_length(line_vec);
                    if (radial_len <= 1e-6f || !isfinite(radial_len) ||
                        line_len <= 1e-6f || !isfinite(line_len)) {
                        solve_failed = true;
                        failure_reason = "Unsatisfied line-arc endpoint tangency constraint.";
                        implicated_constraints[implicated_constraint_count++] = child;
                        break;
                    }

                    scene_solver_arc_normal_candidate_t *arc_normal = &arc_normal_candidates[arc_normal_index];
                    vec3_t tangent_dir = vec3_make(0.0f, 0.0f, 0.0f);
                    if (!scene_solver_arc_tangent_direction(&candidates[arc_center_index],
                                                            &candidates[arc_endpoint_index],
                                                            arc_normal,
                                                            &tangent_dir)) {
                        solve_failed = true;
                        failure_reason = "Unsatisfied line-arc endpoint tangency constraint.";
                        implicated_constraints[implicated_constraint_count++] = child;
                        break;
                    }

                    vec3_t line_dir = vec3_scale(line_vec, 1.0f / line_len);
                    vec3_t tangent_opposite = vec3_scale(tangent_dir, -1.0f);
                    vec3_t target_line_dir =
                        (vec3_dot(line_dir, tangent_dir) >= vec3_dot(line_dir, tangent_opposite))
                            ? tangent_dir
                            : tangent_opposite;
                    float tangent_dot = vec3_dot(line_dir, tangent_dir);
                    if (tangent_dot > 1.0f) tangent_dot = 1.0f;
                    if (tangent_dot < -1.0f) tangent_dot = -1.0f;
                    float tangent_residual = acosf(fabsf(tangent_dot));
                    if (tangent_residual > pass_max_residual) pass_max_residual = tangent_residual;
                    if (tangent_residual > pass_max_angle_delta) pass_max_angle_delta = tangent_residual;
                    if (tangent_residual <= angle_tolerance) {
                        continue;
                    }

                    bool updated = false;
                    if (drag_anchor_is_line_endpoint) {
                        if (!candidates[line_other_index].fixed) {
                            vec3_t before_other = candidates[line_other_index].point;
                            candidates[line_other_index].point =
                                vec3_add(shared_point, vec3_scale(target_line_dir, line_len));
                            float delta_other = vec3_length(vec3_sub(candidates[line_other_index].point, before_other));
                            if (delta_other > pass_max_position_delta) pass_max_position_delta = delta_other;
                            updated = true;
                        } else if (!arc_normal->fixed) {
                            vec3_t radial_normalized = vec3_scale(radial, 1.0f / radial_len);
                            vec3_t target_normal = vec3_cross(radial_normalized, target_line_dir);
                            float target_normal_len = vec3_length(target_normal);
                            if (target_normal_len <= 1e-6f || !isfinite(target_normal_len)) {
                                solve_failed = true;
                                failure_reason = "Unsatisfied line-arc endpoint tangency constraint.";
                                implicated_constraints[implicated_constraint_count++] = child;
                                break;
                            }
                            target_normal = vec3_scale(target_normal, 1.0f / target_normal_len);
                            if (vec3_dot(target_normal, arc_normal->normal) < 0.0f) {
                                target_normal = vec3_scale(target_normal, -1.0f);
                            }
                            float normal_residual = 0.0f;
                            if (!scene_solver_try_set_arc_normal(arc_normal, target_normal,
                                                                 angle_tolerance,
                                                                 &pass_max_angle_delta,
                                                                 &normal_residual)) {
                                solve_failed = true;
                                failure_reason = "Unsatisfied line-arc endpoint tangency constraint.";
                                implicated_constraints[implicated_constraint_count++] = child;
                                break;
                            }
                            if (normal_residual > pass_max_residual) pass_max_residual = normal_residual;
                            updated = true;
                        }
                    } else if (drag_anchor_is_arc_endpoint) {
                        if (!candidates[line_other_index].fixed) {
                            vec3_t before_other = candidates[line_other_index].point;
                            candidates[line_other_index].point =
                                vec3_add(shared_point, vec3_scale(target_line_dir, line_len));
                            float delta_other = vec3_length(vec3_sub(candidates[line_other_index].point, before_other));
                            if (delta_other > pass_max_position_delta) pass_max_position_delta = delta_other;
                            updated = true;
                        } else if (!candidates[arc_center_index].fixed) {
                            vec3_t before_center = candidates[arc_center_index].point;
                            vec3_t target_center = vec3_sub(shared_point, vec3_scale(radial, 1.0f));
                            candidates[arc_center_index].point = target_center;
                            float delta_center = vec3_length(vec3_sub(candidates[arc_center_index].point, before_center));
                            if (delta_center > pass_max_position_delta) pass_max_position_delta = delta_center;
                            updated = true;
                        }
                    } else if (!candidates[line_other_index].fixed) {
                        vec3_t before_other = candidates[line_other_index].point;
                        candidates[line_other_index].point =
                            vec3_add(shared_point, vec3_scale(target_line_dir, line_len));
                        float delta_other = vec3_length(vec3_sub(candidates[line_other_index].point, before_other));
                        if (delta_other > pass_max_position_delta) pass_max_position_delta = delta_other;
                        updated = true;
                    } else if (!arc_normal->fixed) {
                        vec3_t radial_normalized = vec3_scale(radial, 1.0f / radial_len);
                        vec3_t target_normal = vec3_cross(radial_normalized, target_line_dir);
                        float target_normal_len = vec3_length(target_normal);
                        if (target_normal_len <= 1e-6f || !isfinite(target_normal_len)) {
                            solve_failed = true;
                            failure_reason = "Unsatisfied line-arc endpoint tangency constraint.";
                            implicated_constraints[implicated_constraint_count++] = child;
                            break;
                        }
                        target_normal = vec3_scale(target_normal, 1.0f / target_normal_len);
                        if (vec3_dot(target_normal, arc_normal->normal) < 0.0f) {
                            target_normal = vec3_scale(target_normal, -1.0f);
                        }
                        float normal_residual = 0.0f;
                        if (!scene_solver_try_set_arc_normal(arc_normal, target_normal,
                                                             angle_tolerance,
                                                             &pass_max_angle_delta,
                                                             &normal_residual)) {
                            solve_failed = true;
                            failure_reason = "Unsatisfied line-arc endpoint tangency constraint.";
                            implicated_constraints[implicated_constraint_count++] = child;
                            break;
                        }
                        if (normal_residual > pass_max_residual) pass_max_residual = normal_residual;
                        updated = true;
                    }

                    if (!updated) {
                        solve_failed = true;
                        failure_reason = "Unsatisfied line-arc endpoint tangency constraint.";
                        implicated_constraints[implicated_constraint_count++] = child;
                        break;
                    }
                    continue;
                }

                if (constraint->type == CONSTRAINT_ARC_ENDPOINT_ANGLE) {
                    if (participant_count < 2) {
                        solve_failed = true;
                        failure_reason = "Unsatisfied arc endpoint-angle constraint.";
                        implicated_constraints[implicated_constraint_count++] = child;
                        break;
                    }

                    ecs_entity_t arc_entity_a = (ecs_entity_t)constraint->participant_descriptors[0].entity;
                    ecs_entity_t arc_entity_b = (ecs_entity_t)constraint->participant_descriptors[1].entity;
                    constraint_participant_role_t role_a =
                        (constraint_participant_role_t)constraint->participant_descriptors[0].role;
                    constraint_participant_role_t role_b =
                        (constraint_participant_role_t)constraint->participant_descriptors[1].role;
                    bool role_a_is_endpoint = role_a == CONSTRAINT_PARTICIPANT_ROLE_POINT_A ||
                                              role_a == CONSTRAINT_PARTICIPANT_ROLE_POINT_B;
                    bool role_b_is_endpoint = role_b == CONSTRAINT_PARTICIPANT_ROLE_POINT_A ||
                                              role_b == CONSTRAINT_PARTICIPANT_ROLE_POINT_B;
                    if (!role_a_is_endpoint || !role_b_is_endpoint ||
                        arc_entity_a == 0 || arc_entity_b == 0 || arc_entity_a != arc_entity_b) {
                        solve_failed = true;
                        failure_reason = "Unsatisfied arc endpoint-angle constraint.";
                        implicated_constraints[implicated_constraint_count++] = child;
                        break;
                    }

                    int anchor_endpoint_index = scene_solver_ensure_point_candidate(
                        scene, candidates, ECS_SCENE_SOLVER_MAX_IMPLICATED_PARTICIPANTS,
                        &candidate_count, arc_entity_a, role_a);
                    int moved_endpoint_index = scene_solver_ensure_point_candidate(
                        scene, candidates, ECS_SCENE_SOLVER_MAX_IMPLICATED_PARTICIPANTS,
                        &candidate_count, arc_entity_b, role_b);
                    int center_index = scene_solver_ensure_point_candidate(
                        scene, candidates, ECS_SCENE_SOLVER_MAX_IMPLICATED_PARTICIPANTS,
                        &candidate_count, arc_entity_a, CONSTRAINT_PARTICIPANT_ROLE_CENTER);
                    int arc_normal_index = scene_solver_ensure_arc_normal_candidate(
                        scene, arc_normal_candidates, ECS_SCENE_SOLVER_MAX_IMPLICATED_PARTICIPANTS,
                        &arc_normal_count, arc_entity_a);
                    if (anchor_endpoint_index < 0 || moved_endpoint_index < 0 ||
                        center_index < 0 || arc_normal_index < 0) {
                        solve_failed = true;
                        failure_reason = "Unsatisfied arc endpoint-angle constraint.";
                        implicated_constraints[implicated_constraint_count++] = child;
                        break;
                    }

                    const float pi = 3.14159265359f;
                    float desired_angle = constraint->value;
                    if (!isfinite(desired_angle)) {
                        solve_failed = true;
                        failure_reason = "Unsatisfied arc endpoint-angle constraint.";
                        implicated_constraints[implicated_constraint_count++] = child;
                        break;
                    }
                    if (desired_angle < 0.0f) desired_angle = 0.0f;
                    if (desired_angle > pi) desired_angle = pi;

                    vec3_t anchor_radial = vec3_sub(candidates[anchor_endpoint_index].point,
                                                    candidates[center_index].point);
                    vec3_t moved_radial = vec3_sub(candidates[moved_endpoint_index].point,
                                                   candidates[center_index].point);
                    float anchor_len = vec3_length(anchor_radial);
                    float moved_len = vec3_length(moved_radial);
                    if (anchor_len <= 1e-6f || moved_len <= 1e-6f ||
                        !isfinite(anchor_len) || !isfinite(moved_len)) {
                        solve_failed = true;
                        failure_reason = "Unsatisfied arc endpoint-angle constraint.";
                        implicated_constraints[implicated_constraint_count++] = child;
                        break;
                    }

                    vec3_t anchor_dir = vec3_scale(anchor_radial, 1.0f / anchor_len);
                    vec3_t moved_dir = vec3_scale(moved_radial, 1.0f / moved_len);
                    float current_dot = vec3_dot(anchor_dir, moved_dir);
                    if (current_dot > 1.0f) current_dot = 1.0f;
                    if (current_dot < -1.0f) current_dot = -1.0f;
                    float current_angle = acosf(current_dot);
                    float residual = fabsf(current_angle - desired_angle);
                    if (residual > pass_max_residual) pass_max_residual = residual;
                    if (residual > pass_max_angle_delta) pass_max_angle_delta = residual;
                    if (residual <= angle_tolerance) {
                        continue;
                    }

                    if (candidates[moved_endpoint_index].fixed) {
                        solve_failed = true;
                        failure_reason = "Unsatisfied arc endpoint-angle constraint.";
                        implicated_constraints[implicated_constraint_count++] = child;
                        break;
                    }

                    vec3_t arc_normal = arc_normal_candidates[arc_normal_index].normal;
                    float arc_normal_len = vec3_length(arc_normal);
                    if (arc_normal_len <= 1e-6f || !isfinite(arc_normal_len)) {
                        solve_failed = true;
                        failure_reason = "Unsatisfied arc endpoint-angle constraint.";
                        implicated_constraints[implicated_constraint_count++] = child;
                        break;
                    }
                    arc_normal = vec3_scale(arc_normal, 1.0f / arc_normal_len);

                    vec3_t anchor_plane = vec3_sub(anchor_dir, vec3_scale(arc_normal, vec3_dot(anchor_dir, arc_normal)));
                    float anchor_plane_len = vec3_length(anchor_plane);
                    if (anchor_plane_len <= 1e-6f || !isfinite(anchor_plane_len)) {
                        solve_failed = true;
                        failure_reason = "Unsatisfied arc endpoint-angle constraint.";
                        implicated_constraints[implicated_constraint_count++] = child;
                        break;
                    }
                    anchor_plane = vec3_scale(anchor_plane, 1.0f / anchor_plane_len);
                    vec3_t perp = vec3_cross(arc_normal, anchor_plane);
                    float perp_len = vec3_length(perp);
                    if (perp_len <= 1e-6f || !isfinite(perp_len)) {
                        solve_failed = true;
                        failure_reason = "Unsatisfied arc endpoint-angle constraint.";
                        implicated_constraints[implicated_constraint_count++] = child;
                        break;
                    }
                    perp = vec3_scale(perp, 1.0f / perp_len);

                    vec3_t target_plus = vec3_add(vec3_scale(anchor_plane, cosf(desired_angle)),
                                                  vec3_scale(perp, sinf(desired_angle)));
                    vec3_t target_minus = vec3_add(vec3_scale(anchor_plane, cosf(desired_angle)),
                                                   vec3_scale(perp, -sinf(desired_angle)));
                    float target_plus_len = vec3_length(target_plus);
                    float target_minus_len = vec3_length(target_minus);
                    if (target_plus_len <= 1e-6f || target_minus_len <= 1e-6f ||
                        !isfinite(target_plus_len) || !isfinite(target_minus_len)) {
                        solve_failed = true;
                        failure_reason = "Unsatisfied arc endpoint-angle constraint.";
                        implicated_constraints[implicated_constraint_count++] = child;
                        break;
                    }
                    target_plus = vec3_scale(target_plus, 1.0f / target_plus_len);
                    target_minus = vec3_scale(target_minus, 1.0f / target_minus_len);
                    vec3_t target_dir =
                        (vec3_dot(target_plus, moved_dir) >= vec3_dot(target_minus, moved_dir))
                            ? target_plus
                            : target_minus;

                    vec3_t before_moved = candidates[moved_endpoint_index].point;
                    candidates[moved_endpoint_index].point =
                        vec3_add(candidates[center_index].point, vec3_scale(target_dir, moved_len));
                    float moved_delta = vec3_length(vec3_sub(candidates[moved_endpoint_index].point, before_moved));
                    if (moved_delta > pass_max_position_delta) pass_max_position_delta = moved_delta;
                    continue;
                }

                if (constraint->type == CONSTRAINT_FIXED) {
                    continue;
                }

                solve_failed = true;
                failure_reason = "Constraint type not yet solved in transactional recalc.";
                implicated_constraints[implicated_constraint_count++] = child;
                break;
            }
        }

        if (solve_failed) break;
        if (pass_max_position_delta <= position_tolerance &&
            pass_max_angle_delta <= angle_tolerance &&
            pass_max_residual <= position_tolerance) {
            converged = true;
            break;
        }
    }

    sk->solve_request_serial++;
    sk->last_solve_timestamp_ms = scene_solver_now_ms();
    sk->solver_backend_id = scene_solver_backend_id(scene);

    if (solve_failed) {
        scene_solver_sort_entities_unique(implicated_constraints, &implicated_constraint_count);
        scene_solver_set_failure_implication(scene, sketch,
                                             implicated_constraints,
                                             implicated_constraint_count,
                                             failure_reason);
        scene_solver_add_diagnostic(scene, sketch, SKETCH_SOLVER_DIAG_ERROR,
                                    "solve", failure_reason,
                                    implicated_constraint_count > 0 ? implicated_constraints[0] : 0);
        sk->solve_completed_serial = sk->solve_request_serial;
        scene_solver_apply_status(scene, sketch, SKETCH_STATUS_ERROR);
        if (!preserve_drag_anchor) {
            sk->solver_drag_anchor_valid = false;
            sk->solver_drag_anchor_owner_entity = 0;
            sk->solver_drag_anchor_role = (uint8_t)CONSTRAINT_PARTICIPANT_ROLE_UNSPECIFIED;
            sk->solver_drag_anchor_sub_index = 0;
        }
        return false;
    }

    if (!converged) {
        implicated_constraint_count = 0;
        children = ecs_children(scene->world->world, sketch);
        while (ecs_children_next(&children)) {
            for (int i = 0; i < children.count; i++) {
                ecs_entity_t child = children.entities[i];
                if (!ecs_world_get_constraint(scene->world, child)) continue;
                if (implicated_constraint_count >= ECS_SCENE_SOLVER_MAX_IMPLICATED_CONSTRAINTS) break;
                implicated_constraints[implicated_constraint_count++] = child;
            }
            if (implicated_constraint_count >= ECS_SCENE_SOLVER_MAX_IMPLICATED_CONSTRAINTS) break;
        }
        scene_solver_sort_entities_unique(implicated_constraints, &implicated_constraint_count);
        scene_solver_set_failure_implication(scene, sketch,
                                             implicated_constraints,
                                             implicated_constraint_count,
                                             "max passes reached");
        scene_solver_add_diagnostic(scene, sketch, SKETCH_SOLVER_DIAG_ERROR,
                                    "solve", "max passes reached",
                                    implicated_constraint_count > 0 ? implicated_constraints[0] : 0);
        sk->solve_completed_serial = sk->solve_request_serial;
        scene_solver_apply_status(scene, sketch, SKETCH_STATUS_ERROR);
        if (!preserve_drag_anchor) {
            sk->solver_drag_anchor_valid = false;
            sk->solver_drag_anchor_owner_entity = 0;
            sk->solver_drag_anchor_role = (uint8_t)CONSTRAINT_PARTICIPANT_ROLE_UNSPECIFIED;
            sk->solver_drag_anchor_sub_index = 0;
        }
        return false;
    }

    for (int i = 0; i < arc_normal_count; i++) {
        GeometryComp *g = ecs_world_get_geometry(scene->world, arc_normal_candidates[i].entity);
        RenderableComp *r = ecs_world_get_renderable(scene->world, arc_normal_candidates[i].entity);
        if (!g || g->type != GEOM_ARC) continue;
        g->data.arc.normal = vec3_normalize(arc_normal_candidates[i].normal);
        if (r) r->instance_dirty = true;
        scene_sync_endpoint_entities_for_owner(scene, arc_normal_candidates[i].entity);
    }
    for (int i = 0; i < candidate_count; i++) {
        GeometryComp *g = ecs_world_get_geometry(scene->world, candidates[i].entity);
        RenderableComp *r = ecs_world_get_renderable(scene->world, candidates[i].entity);
        if (!g) continue;
        scene_apply_local_point_to_participant(g, candidates[i].role, candidates[i].point);
        if (r) r->instance_dirty = true;
        scene_sync_endpoint_entities_for_owner(scene, candidates[i].entity);
    }

    sk->solve_completed_serial = sk->solve_request_serial;
    sketch_status_t derived_status = scene_derive_sketch_status(scene, sketch);
    bool status_applied = scene_solver_apply_status(scene, sketch, derived_status);
    if (!preserve_drag_anchor) {
        sk->solver_drag_anchor_valid = false;
        sk->solver_drag_anchor_owner_entity = 0;
        sk->solver_drag_anchor_role = (uint8_t)CONSTRAINT_PARTICIPANT_ROLE_UNSPECIFIED;
        sk->solver_drag_anchor_sub_index = 0;
    }
    return status_applied;
}

static inline bool scene_solver_add_diagnostic(ecs_scene_t *scene, ecs_entity_t sketch,
                                               sketch_solver_diagnostic_severity_t severity,
                                               const char *timestamp,
                                               const char *message,
                                               ecs_entity_t implicated_constraint) {
    if (!scene || !scene_is_sketch(scene, sketch)) return false;
    SketchComp *sk = ecs_world_get_sketch(scene->world, sketch);
    if (!sk) return false;

    if (sk->diagnostics_count > 0) {
        const sketch_solver_diagnostic_t *last =
            scene_solver_diagnostic_at(scene, sketch, (int)sk->diagnostics_count - 1);
        if (last &&
            last->severity == severity &&
            last->implicated_constraint == (uint64_t)implicated_constraint &&
            ((last->message[0] == '\0' && (!message || message[0] == '\0')) ||
             (message && strcmp(last->message, message) == 0))) {
            return true;
        }
    }

    uint32_t write_index = sk->diagnostics_head;
    sketch_solver_diagnostic_t *diag = &sk->diagnostics[write_index];
    memset(diag, 0, sizeof(*diag));
    diag->severity = severity;
    diag->implicated_constraint = (uint64_t)implicated_constraint;
    if (timestamp) {
        snprintf(diag->timestamp, sizeof(diag->timestamp), "%s", timestamp);
    }
    if (message) {
        snprintf(diag->message, sizeof(diag->message), "%s", message);
    }

    sk->diagnostics_head = (sk->diagnostics_head + 1U) % SKETCH_SOLVER_DIAGNOSTICS_MAX;
    if (sk->diagnostics_count < SKETCH_SOLVER_DIAGNOSTICS_MAX) {
        sk->diagnostics_count++;
    }
    return true;
}

static inline bool scene_solver_clear_diagnostics(ecs_scene_t *scene, ecs_entity_t sketch) {
    if (!scene || !scene_is_sketch(scene, sketch)) return false;
    SketchComp *sk = ecs_world_get_sketch(scene->world, sketch);
    if (!sk) return false;
    sk->diagnostics_count = 0;
    sk->diagnostics_head = 0;
    return true;
}

static inline int scene_solver_diagnostic_count(ecs_scene_t *scene, ecs_entity_t sketch) {
    if (!scene || !scene_is_sketch(scene, sketch)) return 0;
    SketchComp *sk = ecs_world_get_sketch(scene->world, sketch);
    if (!sk) return 0;
    return (int)sk->diagnostics_count;
}

static inline const sketch_solver_diagnostic_t* scene_solver_diagnostic_at(ecs_scene_t *scene,
                                                                           ecs_entity_t sketch,
                                                                           int index) {
    if (!scene || !scene_is_sketch(scene, sketch)) return NULL;
    SketchComp *sk = ecs_world_get_sketch(scene->world, sketch);
    if (!sk) return NULL;
    if (index < 0 || index >= (int)sk->diagnostics_count) return NULL;

    uint32_t oldest = (sk->diagnostics_count < SKETCH_SOLVER_DIAGNOSTICS_MAX)
        ? 0
        : sk->diagnostics_head;
    uint32_t slot = (oldest + (uint32_t)index) % SKETCH_SOLVER_DIAGNOSTICS_MAX;
    return &sk->diagnostics[slot];
}

static inline bool scene_solver_apply_status(ecs_scene_t *scene, ecs_entity_t sketch, sketch_status_t status) {
    if (!scene || !scene_is_sketch(scene, sketch)) return false;
    SketchComp *sk = ecs_world_get_sketch(scene->world, sketch);
    if (!sk) return false;
    sk->status = status;
    if (status == SKETCH_STATUS_SOLVED) {
        scene_solver_clear_failure_implication(scene, sketch, true);
        const scene_solver_failure_implication_t *imp = scene_solver_failure_implication(scene);
        assert(!imp || !imp->active || (imp->sketch != 0 && imp->sketch != sketch));
    }
    return true;
}

static inline const char* scene_solver_diagnostic_level_name(sketch_solver_diagnostic_severity_t severity) {
    switch (severity) {
        case SKETCH_SOLVER_DIAG_INFO: return "INFO";
        case SKETCH_SOLVER_DIAG_WARNING: return "WARNING";
        case SKETCH_SOLVER_DIAG_ERROR: return "ERROR";
        default: return "ERROR";
    }
}

static inline bool scene_solver_set_failure_implication(ecs_scene_t *scene,
                                                       ecs_entity_t sketch,
                                                       const ecs_entity_t *implicated_constraints,
                                                       int implicated_constraint_count,
                                                       const char *reason) {
    if (!scene || !scene_is_sketch(scene, sketch)) return false;
    scene_solver_failure_implication_t *imp = &scene->solver_failure_implication;
    memset(imp, 0, sizeof(*imp));
    imp->active = true;
    imp->sketch = sketch;

    if (reason) {
        snprintf(imp->reason, sizeof(imp->reason), "%s", reason);
    }

    if (!implicated_constraints || implicated_constraint_count <= 0) {
        return true;
    }

    int capped_constraints = implicated_constraint_count;
    if (capped_constraints > ECS_SCENE_SOLVER_MAX_IMPLICATED_CONSTRAINTS) {
        capped_constraints = ECS_SCENE_SOLVER_MAX_IMPLICATED_CONSTRAINTS;
    }

    ecs_entity_t sorted_constraints[ECS_SCENE_SOLVER_MAX_IMPLICATED_CONSTRAINTS] = {0};
    int sorted_constraint_count = 0;
    for (int i = 0; i < capped_constraints; i++) {
        ecs_entity_t constraint_e = implicated_constraints[i];
        if (constraint_e == 0) continue;
        if (!ecs_is_alive(scene->world->world, constraint_e)) continue;
        sorted_constraints[sorted_constraint_count++] = constraint_e;
    }
    scene_solver_sort_entities_unique(sorted_constraints, &sorted_constraint_count);

    for (int i = 0; i < sorted_constraint_count; i++) {
        ecs_entity_t constraint_e = sorted_constraints[i];
        if (constraint_e == 0) continue;

        imp->implicated_constraints[imp->implicated_constraint_count++] = constraint_e;
        if (imp->first_constraint == 0) {
            imp->first_constraint = constraint_e;
        }

        ConstraintComp *constraint = ecs_world_get_constraint(scene->world, constraint_e);
        if (!constraint) continue;

        uint32_t participant_count = constraint->participant_count;
        if (participant_count > CONSTRAINT_MAX_PARTICIPANTS) {
            participant_count = CONSTRAINT_MAX_PARTICIPANTS;
        }

        for (uint32_t p = 0; p < participant_count; p++) {
            ecs_entity_t participant = (ecs_entity_t)constraint->participants[p];
            if (participant == 0) continue;
            if (!ecs_is_alive(scene->world->world, participant)) continue;

            bool duplicate = false;
            for (int k = 0; k < imp->participant_count; k++) {
                if (imp->participants[k] == participant) {
                    duplicate = true;
                    break;
                }
            }
            if (duplicate) continue;
            if (imp->participant_count >= ECS_SCENE_SOLVER_MAX_IMPLICATED_PARTICIPANTS) break;

            imp->participants[imp->participant_count++] = participant;
        }
    }

    scene_solver_sort_entities_unique(imp->participants, &imp->participant_count);

    return true;
}

static inline void scene_solver_clear_failure_implication(ecs_scene_t *scene,
                                                          ecs_entity_t sketch,
                                                          bool clear_on_success) {
    if (!scene) return;
    if (!clear_on_success) return;
    scene_solver_failure_implication_t *imp = &scene->solver_failure_implication;
    if (!imp->active) return;
    if (sketch != 0 && imp->sketch != 0 && imp->sketch != sketch) return;
    memset(imp, 0, sizeof(*imp));
}

static inline const scene_solver_failure_implication_t* scene_solver_failure_implication(const ecs_scene_t *scene) {
    if (!scene) return NULL;
    return &scene->solver_failure_implication;
}

static inline bool scene_solver_can_apply_drag(ecs_scene_t *scene,
                                              ecs_entity_t sketch,
                                              const ecs_entity_t *drag_entities,
                                              int drag_entity_count,
                                              vec3_t requested_delta,
                                              scene_solver_drag_decision_t *out_decision) {
    if (!out_decision) return false;
    memset(out_decision, 0, sizeof(*out_decision));
    out_decision->projected_delta = scene_solver_project_drag_delta_with_budget(requested_delta);
    out_decision->result = SCENE_SOLVER_DRAG_INVALID;

    if (!scene || !scene_is_sketch(scene, sketch)) {
        snprintf(out_decision->block_reason, sizeof(out_decision->block_reason), "Invalid sketch for constrained drag.");
        return false;
    }
    if (!drag_entities || drag_entity_count <= 0) {
        snprintf(out_decision->block_reason, sizeof(out_decision->block_reason), "No drag entities selected.");
        return false;
    }

    const float eps = 1e-7f;
    float max_component = fmaxf(fabsf(requested_delta.x), fmaxf(fabsf(requested_delta.y), fabsf(requested_delta.z)));
    if (max_component <= eps) {
        out_decision->result = SCENE_SOLVER_DRAG_FEASIBLE;
        out_decision->projected_delta = vec3_make(0.0f, 0.0f, 0.0f);
        return true;
    }

    bool blocked_by_fixed = false;
    for (int i = 0; i < drag_entity_count; i++) {
        ecs_entity_t e = drag_entities[i];
        if (!ecs_is_alive(scene->world->world, e)) continue;
        if (ecs_world_get_geometry(scene->world, e) == NULL) continue;
        ecs_entity_t parent = ecs_world_get_parent(scene->world, e);
        if (parent != sketch) continue;

        SketchGeometryStateComp *geom_state = ecs_world_get_sketch_geometry_state(scene->world, e);
        if (geom_state && geom_state->fixed) {
            blocked_by_fixed = true;
            break;
        }
    }

    if (blocked_by_fixed) {
        out_decision->result = SCENE_SOLVER_DRAG_UNSATISFIABLE;
        out_decision->projected_delta = vec3_make(0.0f, 0.0f, 0.0f);
        snprintf(out_decision->block_reason, sizeof(out_decision->block_reason),
                 "Unsatisfiable movement: fixed sketch geometry cannot be moved.");

        int count = 0;
        ecs_iter_t it = ecs_children(scene->world->world, sketch);
        while (ecs_children_next(&it)) {
            for (int i = 0; i < it.count; i++) {
                ecs_entity_t child = it.entities[i];
                if (!ecs_world_get_constraint(scene->world, child)) continue;
                if (count >= ECS_SCENE_SOLVER_MAX_IMPLICATED_CONSTRAINTS) break;
                out_decision->implicated_constraints[count++] = child;
                if (out_decision->first_implicated_constraint == 0) {
                    out_decision->first_implicated_constraint = child;
                }
            }
            if (count >= ECS_SCENE_SOLVER_MAX_IMPLICATED_CONSTRAINTS) break;
        }
        out_decision->implicated_constraint_count = count;
        return true;
    }

    out_decision->result = SCENE_SOLVER_DRAG_FEASIBLE;
    out_decision->projected_delta = scene_solver_project_drag_delta_with_budget(requested_delta);
    out_decision->first_implicated_constraint = 0;
    out_decision->implicated_constraint_count = 0;
    out_decision->block_reason[0] = '\0';
    return true;
}

static inline bool scene_solver_drag_make_rejected_diagnostic(const scene_solver_drag_decision_t *decision,
                                                             scene_solver_drag_diagnostic_event_t *out_event) {
    if (!decision || !out_event) return false;
    if (decision->result != SCENE_SOLVER_DRAG_UNSATISFIABLE) return false;

    memset(out_event, 0, sizeof(*out_event));
    out_event->severity = SKETCH_SOLVER_DIAG_WARNING;
    snprintf(out_event->timestamp, sizeof(out_event->timestamp), "%llu",
             (unsigned long long)((uint64_t)time(NULL) * 1000ULL));
    snprintf(out_event->message, sizeof(out_event->message),
             "Drag rejected: active constraints make this move invalid.");
    out_event->implicated_constraint = decision->first_implicated_constraint;
    return true;
}

// Batch-parent all children to a single parent
static inline void scene_set_parent_batch(ecs_scene_t *scene,
    ecs_entity_t *children, int count, ecs_entity_t parent) {
    ecs_world_set_parent_batch(scene->world, children, count, parent);
}

// Batch-parent each child to its own parent
static inline void scene_set_parents_batch(ecs_scene_t *scene,
    ecs_entity_t *children, ecs_entity_t *parents, int count) {
    ecs_world_set_parents_batch(scene->world, children, parents, count);
}

// Get parent of an entity (returns 0 if no parent)
static inline ecs_entity_t scene_get_parent(ecs_scene_t *scene, ecs_entity_t child) {
    return ecs_world_get_parent(scene->world, child);
}

// Check if entity has a parent
static inline bool scene_has_parent(ecs_scene_t *scene, ecs_entity_t e) {
    return ecs_world_has_parent(scene->world, e);
}

// Get children of an entity (returns count, fills out_children up to max_count)
static inline int scene_get_children(ecs_scene_t *scene, ecs_entity_t parent,
                                      ecs_entity_t *out_children, int max_count) {
    return ecs_world_get_children(scene->world, parent, out_children, max_count);
}

// Count children of an entity (no limit)
static inline int scene_count_children(ecs_scene_t *scene, ecs_entity_t parent) {
    return ecs_world_count_children(scene->world, parent);
}

// Check if entity has any children
static inline bool scene_has_children(ecs_scene_t *scene, ecs_entity_t e) {
    return ecs_world_has_children(scene->world, e);
}

static inline int scene_count_sketch_geometry(ecs_scene_t *scene, ecs_entity_t sketch) {
    if (!scene_is_sketch(scene, sketch)) return 0;

    int count = 0;
    ecs_iter_t it = ecs_children(scene->world->world, sketch);
    while (ecs_children_next(&it)) {
        for (int i = 0; i < it.count; i++) {
            if (ecs_world_get_geometry(scene->world, it.entities[i])) {
                count++;
            }
        }
    }
    return count;
}

static inline int scene_count_sketch_fixed_geometry(ecs_scene_t *scene, ecs_entity_t sketch) {
    if (!scene_is_sketch(scene, sketch)) return 0;

    int fixed_count = 0;
    ecs_iter_t it = ecs_children(scene->world->world, sketch);
    while (ecs_children_next(&it)) {
        for (int i = 0; i < it.count; i++) {
            ecs_entity_t child = it.entities[i];
            if (!ecs_world_get_geometry(scene->world, child)) {
                continue;
            }
            SketchGeometryStateComp *state = ecs_world_get_sketch_geometry_state(scene->world, child);
            if (state && state->fixed) {
                fixed_count++;
            }
        }
    }
    return fixed_count;
}

static inline int scene_get_sketch_constraint_count(ecs_scene_t *scene, ecs_entity_t sketch) {
    SketchComp *sk = ecs_world_get_sketch(scene->world, sketch);
    if (!sk) return 0;
    sk->constraint_count = scene_count_sketch_constraints(scene, sketch);
    return sk->constraint_count;
}

// Placeholder derivation for Phase 10 (D-06/D-08):
// - error: negative counts (invalid state)
// - loose: no geometry or mixed fixed state
// - fixed: all geometry is fixed
// - solved: has geometry, none fixed, non-negative counts
static inline sketch_status_t scene_derive_sketch_status(ecs_scene_t *scene, ecs_entity_t sketch) {
    SketchComp *sk = ecs_world_get_sketch(scene->world, sketch);
    if (!sk) return SKETCH_STATUS_ERROR;

    int geometry_count = scene_count_sketch_geometry(scene, sketch);
    int fixed_count = scene_count_sketch_fixed_geometry(scene, sketch);
    int constraint_count = sk->constraint_count;

    if (geometry_count < 0 || fixed_count < 0 || constraint_count < 0 || fixed_count > geometry_count) {
        return SKETCH_STATUS_ERROR;
    }
    if (geometry_count == 0) {
        return SKETCH_STATUS_LOOSE;
    }
    if (fixed_count == geometry_count) {
        return SKETCH_STATUS_FIXED;
    }
    if (fixed_count == 0) {
        return SKETCH_STATUS_SOLVED;
    }
    return SKETCH_STATUS_LOOSE;
}

static inline void scene_refresh_sketch_metadata(ecs_scene_t *scene, ecs_entity_t sketch) {
    SketchComp *sk = ecs_world_get_sketch(scene->world, sketch);
    if (!sk) return;

    sk->geometry_count = scene_count_sketch_geometry(scene, sketch);
    sk->fixed_geometry_count = scene_count_sketch_fixed_geometry(scene, sketch);
    sk->constraint_count = scene_count_sketch_constraints(scene, sketch);
    if (sk->solve_completed_serial == 0) {
        sk->status = scene_derive_sketch_status(scene, sketch);
    }
}

//------------------------------------------------------------------------------
// Entity Property Modification
//------------------------------------------------------------------------------

// Update entity color
static inline void scene_set_color(ecs_scene_t *scene, ecs_entity_t e, vec4_t color) {
    GeometryComp *g = ecs_world_get_geometry(scene->world, e);
    RenderableComp *r = ecs_world_get_renderable(scene->world, e);
    if (!g || !r) return;

    g->color = color;
    r->instance_dirty = true;
}

// Update entity visibility
static inline void scene_set_visible(ecs_scene_t *scene, ecs_entity_t e, bool visible) {
    RenderableComp *r = ecs_world_get_renderable(scene->world, e);
    if (r) {
        r->visible = visible;
        // Note: Hidden entities still occupy slots, just don't render
        // For true removal, use scene_remove_entity
    }
}

// Update entity position (sets transform, marks dirty)
static inline void scene_set_position(ecs_scene_t *scene, ecs_entity_t e, vec3_t pos) {
    TransformComp *t = ecs_world_get_transform(scene->world, e);
    RenderableComp *r = ecs_world_get_renderable(scene->world, e);
    if (t && r) {
        t->position = pos;
        t->dirty = true;
        r->instance_dirty = true;
    }
}

//------------------------------------------------------------------------------
// Hierarchical Transform Update (internal helper)
//------------------------------------------------------------------------------

// Recursively update transforms for an entity and all its children
// Must be called on parents before children to ensure correct world matrices
// When called recursively for children, forces update even if child's local transform isn't dirty
// because the parent's world_matrix changed
static inline void ecs_scene_update_transform_recursive(ecs_scene_t *scene, ecs_entity_t e,
                                                         const mat4_t *parent_world) {
    ecs_world_state_t *w = scene->world;

    // Get this entity's transform
    TransformComp *t = (TransformComp*)ecs_get_id(w->world, e, w->TransformComp_id);
    if (!t) return;

    // Force the transform to be dirty so it updates
    // This is needed because even if local transform didn't change,
    // the world_matrix needs recalculating if parent's world_matrix changed
    t->dirty = true;

    // Update this entity's transform
    transform_comp_update_with_parent(t, parent_world);

    // Mark renderable dirty so GPU data gets updated
    RenderableComp *r = (RenderableComp*)ecs_get_id(w->world, e, w->RenderableComp_id);
    if (r) {
        r->instance_dirty = true;
    }

    // Recursively update children - they need updating since this entity's
    // world_matrix may have changed
    // Use ecs_children iterator directly to handle unlimited children
    ecs_iter_t child_it = ecs_children(w->world, e);
    while (ecs_children_next(&child_it)) {
        for (int i = 0; i < child_it.count; i++) {
            ecs_scene_update_transform_recursive(scene, child_it.entities[i], &t->world_matrix);
        }
    }
}

// Update all transforms in the scene respecting parent-child hierarchy
static inline void ecs_scene_update_transforms(ecs_scene_t *scene) {
    ecs_world_state_t *w = scene->world;

    // Query all entities with TransformComp (skip ImportPending)
    ecs_query_t *q = ecs_query(w->world, {
        .terms = {
            { .id = w->TransformComp_id },
            { .id = w->ImportPending_tag, .oper = EcsNot }
        }
    });

    // Process all dirty transforms
    ecs_iter_t it = ecs_query_iter(w->world, q);
    while (ecs_query_next(&it)) {
        TransformComp *transforms = ecs_field(&it, TransformComp, 0);

        for (int i = 0; i < it.count; i++) {
            ecs_entity_t e = it.entities[i];
            TransformComp *t = &transforms[i];

            // Only process if dirty
            if (!t->dirty) continue;

            // Check if this entity has a parent
            ecs_entity_t parent = ecs_get_parent(w->world, e);

            if (parent == 0) {
                // Root entity - update it and all descendants
                ecs_scene_update_transform_recursive(scene, e, NULL);
            } else {
                // Entity has a parent - get parent's world matrix
                TransformComp *parent_t = (TransformComp*)ecs_get_id(w->world, parent, w->TransformComp_id);
                if (parent_t) {
                    // If parent is also dirty, it will be processed by its own iteration
                    // and will recursively update this entity. But if parent is NOT dirty,
                    // we need to update this entity using parent's current world_matrix
                    if (!parent_t->dirty) {
                        ecs_scene_update_transform_recursive(scene, e, &parent_t->world_matrix);
                    }
                    // If parent IS dirty, this entity will be updated when parent is processed
                } else {
                    // Parent has no transform, treat as root
                    ecs_scene_update_transform_recursive(scene, e, NULL);
                }
            }
        }
    }
    ecs_query_fini(q);
}

//------------------------------------------------------------------------------
// Scene Update (sync dirty entities to GPU)
//------------------------------------------------------------------------------

static inline void ecs_scene_update(ecs_scene_t *scene) {
    ecs_world_state_t *w = scene->world;

    // First, update all transforms respecting hierarchy
    ecs_scene_update_transforms(scene);

    // Get theme-aware hover and selection colors once per frame
    // Use defaults in case ImGui not ready
    vec4_t hover_color = vec4_make(1.0f, 1.0f, 0.0f, 1.0f);      // Yellow
    vec4_t selection_color = vec4_make(1.0f, 0.5f, 0.0f, 1.0f);  // Orange

    // Try to get theme colors (may fail if ImGui not ready)
    ImGuiStyle* style = igGetStyle();
    if (style) {
        ImVec4 hover_col = style->Colors[ImGuiCol_HeaderHovered];
        hover_color = vec4_make(hover_col.x, hover_col.y, hover_col.z, 1.0f);

        ImVec4 select_col = style->Colors[ImGuiCol_HeaderActive];
        selection_color = vec4_make(select_col.x, select_col.y, select_col.z, 1.0f);
    }

    // Query all entities with geometry and renderable (skip ImportPending)
    ecs_query_t *q = ecs_query(w->world, {
        .terms = {
            { .id = w->GeometryComp_id },
            { .id = w->RenderableComp_id },
            { .id = w->TransformComp_id },
            { .id = w->ImportPending_tag, .oper = EcsNot }
        }
    });

    ecs_iter_t it = ecs_query_iter(w->world, q);
    while (ecs_query_next(&it)) {
        GeometryComp *geoms = ecs_field(&it, GeometryComp, 0);
        RenderableComp *renderables = ecs_field(&it, RenderableComp, 1);
        TransformComp *transforms = ecs_field(&it, TransformComp, 2);

        for (int i = 0; i < it.count; i++) {
            ecs_entity_t e = it.entities[i];
            RenderableComp *r = &renderables[i];
            if (!r->instance_dirty) continue;
            if (r->instance_slot == 0xFFFFFFFF) continue;  // No slot allocated

            GeometryComp *g = &geoms[i];
            TransformComp *t = &transforms[i];

            // Handle visibility: hidden entities get degenerate instance data
            if (!r->visible) {
                // Set degenerate instance data to effectively hide the entity
                vec3_t zero = vec3_make(0.0f, 0.0f, 0.0f);
                vec3_t far_away = vec3_make(1e10f, 1e10f, 1e10f);
                vec4_t invisible = vec4_make(0.0f, 0.0f, 0.0f, 0.0f);

                switch (g->type) {
                    case GEOM_LINE:
                        geom_line_batch_set(&scene->batches.lines, (int)r->instance_slot,
                                            zero, zero, invisible);
                        break;
                    case GEOM_POINT:
                        geom_point_batch_set(&scene->batches.points, (int)r->instance_slot,
                                             far_away, invisible);
                        break;
                    case GEOM_POINT_CLOUD:
                        // Hide all point cloud slots (segment_count holds point count)
                        for (uint32_t p = 0; p < r->segment_count; p++) {
                            geom_point_batch_set(&scene->batches.points,
                                                 (int)(r->instance_slot + p),
                                                 far_away, invisible);
                        }
                        break;
                    case GEOM_TRIANGLE: {
                        vec3_t tri_far = vec3_make(1e10f, 1e10f, 1e10f);
                        vec3_t tri_norm = vec3_make(0.0f, 1.0f, 0.0f);
                        geom_triangle_batch_set(&scene->batches.triangles, (int)r->instance_slot,
                                                tri_far, tri_far, tri_far, tri_norm, invisible);
                        break;
                    }
                    case GEOM_MESH: {
                        // Hide all mesh face slots
                        vec3_t tri_far = vec3_make(1e10f, 1e10f, 1e10f);
                        vec3_t tri_norm = vec3_make(0.0f, 1.0f, 0.0f);
                        for (uint32_t f = 0; f < r->segment_count; f++) {
                            geom_triangle_batch_set(&scene->batches.triangles,
                                                    (int)(r->instance_slot + f),
                                                    tri_far, tri_far, tri_far, tri_norm, invisible);
                        }
                        break;
                    }
                    case GEOM_POLYLINE:
                    case GEOM_ARC:
                    case GEOM_POLYGON:
                    case GEOM_HELIX:
                    case GEOM_BEZIER:
                        // Hide all segment slots
                        for (uint32_t s = 0; s < r->segment_count; s++) {
                            geom_line_batch_set(&scene->batches.lines,
                                                (int)(r->instance_slot + s),
                                                zero, zero, invisible);
                        }
                        // Hide all join slots
                        if (r->join_slot_start != 0xFFFFFFFF) {
                            for (uint32_t j = 0; j < r->join_count; j++) {
                                geom_point_batch_set(&scene->batches.points,
                                                     (int)(r->join_slot_start + j),
                                                     far_away, invisible);
                            }
                        }
                        break;
                    default:
                        break;
                }
                r->instance_dirty = false;
                continue;  // Skip to next entity
            }

            // Determine render color: Selected > Hovered > Normal
            vec4_t render_color = g->color;
            if (ecs_has_id(w->world, e, w->Selected_tag)) {
                render_color = selection_color;
            } else if (ecs_has_id(w->world, e, w->Hovered_tag)) {
                render_color = hover_color;
            }
            EndPointsComp *endpoint_meta = ecs_world_get_endpoints(scene->world, e);
            bool endpoint_point = endpoint_meta && endpoint_meta->is_endpoint_point;

            // Update instance data based on geometry type
            switch (g->type) {
                case GEOM_LINE: {
                    vec3_t world_a = ecs_scene_transform_point_world(&t->world_matrix, g->data.line.a);
                    vec3_t world_b = ecs_scene_transform_point_world(&t->world_matrix, g->data.line.b);
                    geom_line_batch_set(&scene->batches.lines, (int)r->instance_slot,
                                        world_a, world_b, render_color);
                    break;
                }
                case GEOM_POINT: {
                    vec3_t world_pos = ecs_scene_transform_point_world(&t->world_matrix, g->data.point.point);
                    if (endpoint_point) {
                        g->point_size = 8.0f;
                    }
                    geom_point_batch_set(&scene->batches.points, (int)r->instance_slot,
                                         world_pos, render_color);
                    break;
                }
                case GEOM_POLYLINE: {
                    // Update all segment slots
                    int point_count = g->data.polyline.count;
                    for (int s = 0; s < point_count - 1 && s < (int)r->segment_count; s++) {
                        vec3_t world_a = ecs_scene_transform_point_world(&t->world_matrix, g->data.polyline.points[s]);
                        vec3_t world_b = ecs_scene_transform_point_world(&t->world_matrix, g->data.polyline.points[s + 1]);
                        geom_line_batch_set(&scene->batches.lines,
                                            (int)(r->instance_slot + (uint32_t)s),
                                            world_a, world_b, render_color);
                    }
                    // Update all join slots (at interior vertices)
                    if (r->join_slot_start != 0xFFFFFFFF) {
                        for (int j = 0; j < point_count - 2 && j < (int)r->join_count; j++) {
                            vec3_t world_pos = ecs_scene_transform_point_world(&t->world_matrix, g->data.polyline.points[j + 1]);
                            geom_point_batch_set(&scene->batches.points,
                                                 (int)(r->join_slot_start + (uint32_t)j),
                                                 world_pos, render_color);
                        }
                    }
                    break;
                }
                case GEOM_ARC: {
                    scene_update_arc_renderable_slots(scene, e, r, g);
                    // Tessellate arc to points and update slots
                    int arc_point_count = 0;
                    vec3_t *arc_points = ecs_scene_tessellate_arc(
                        g->data.arc.center, g->data.arc.radius,
                        g->data.arc.start_angle, g->data.arc.end_angle,
                        g->data.arc.normal, &arc_point_count
                    );
                    if (arc_points && arc_point_count >= 2) {
                        // Update segment slots
                        for (int s = 0; s < arc_point_count - 1 && s < (int)r->segment_count; s++) {
                            vec3_t world_a = ecs_scene_transform_point_world(&t->world_matrix, arc_points[s]);
                            vec3_t world_b = ecs_scene_transform_point_world(&t->world_matrix, arc_points[s + 1]);
                            geom_line_batch_set(&scene->batches.lines,
                                                (int)(r->instance_slot + (uint32_t)s),
                                                world_a, world_b, render_color);
                        }
                        // Update join slots
                        if (r->join_slot_start != 0xFFFFFFFF) {
                            for (int j = 0; j < arc_point_count - 2 && j < (int)r->join_count; j++) {
                                vec3_t world_pos = ecs_scene_transform_point_world(&t->world_matrix, arc_points[j + 1]);
                                geom_point_batch_set(&scene->batches.points,
                                                     (int)(r->join_slot_start + (uint32_t)j),
                                                     world_pos, render_color);
                            }
                        }
                        free(arc_points);
                    }
                    break;
                }
                case GEOM_POLYGON: {
                    // Update all segment slots (closed polygon: N segments for N points)
                    int point_count = g->data.polygon.count;
                    for (int s = 0; s < point_count && s < (int)r->segment_count; s++) {
                        int next = (s + 1) % point_count;
                        vec3_t world_a = ecs_scene_transform_point_world(&t->world_matrix, g->data.polygon.points[s]);
                        vec3_t world_b = ecs_scene_transform_point_world(&t->world_matrix, g->data.polygon.points[next]);
                        geom_line_batch_set(&scene->batches.lines,
                                            (int)(r->instance_slot + (uint32_t)s),
                                            world_a, world_b, render_color);
                    }
                    // Update all join slots (all vertices in closed polygon)
                    if (r->join_slot_start != 0xFFFFFFFF) {
                        for (int j = 0; j < point_count && j < (int)r->join_count; j++) {
                            vec3_t world_pos = ecs_scene_transform_point_world(&t->world_matrix, g->data.polygon.points[j]);
                            geom_point_batch_set(&scene->batches.points,
                                                 (int)(r->join_slot_start + (uint32_t)j),
                                                 world_pos, render_color);
                        }
                    }
                    break;
                }
                case GEOM_BEZIER: {
                    // Tessellate bezier and update slots
                    int bezier_point_count = 0;
                    vec3_t *bezier_points = ecs_scene_tessellate_bezier(
                        g->data.bezier.p0, g->data.bezier.p1,
                        g->data.bezier.p2, g->data.bezier.p3,
                        g->data.bezier.segments, &bezier_point_count
                    );
                    if (bezier_points && bezier_point_count >= 2) {
                        for (int s = 0; s < bezier_point_count - 1 && s < (int)r->segment_count; s++) {
                            vec3_t world_a = ecs_scene_transform_point_world(&t->world_matrix, bezier_points[s]);
                            vec3_t world_b = ecs_scene_transform_point_world(&t->world_matrix, bezier_points[s + 1]);
                            geom_line_batch_set(&scene->batches.lines,
                                                (int)(r->instance_slot + (uint32_t)s),
                                                world_a, world_b, render_color);
                        }
                        if (r->join_slot_start != 0xFFFFFFFF) {
                            for (int j = 0; j < bezier_point_count - 2 && j < (int)r->join_count; j++) {
                                vec3_t world_pos = ecs_scene_transform_point_world(&t->world_matrix, bezier_points[j + 1]);
                                geom_point_batch_set(&scene->batches.points,
                                                     (int)(r->join_slot_start + (uint32_t)j),
                                                     world_pos, render_color);
                            }
                        }
                        free(bezier_points);
                    }
                    break;
                }
                case GEOM_HELIX: {
                    // Tessellate helix and update slots
                    int helix_point_count = 0;
                    vec3_t *helix_points = ecs_scene_tessellate_helix(
                        g->data.helix.axis_start, g->data.helix.axis_end,
                        g->data.helix.radius, g->data.helix.turns,
                        g->data.helix.segments, &helix_point_count
                    );
                    if (helix_points && helix_point_count >= 2) {
                        for (int s = 0; s < helix_point_count - 1 && s < (int)r->segment_count; s++) {
                            vec3_t world_a = ecs_scene_transform_point_world(&t->world_matrix, helix_points[s]);
                            vec3_t world_b = ecs_scene_transform_point_world(&t->world_matrix, helix_points[s + 1]);
                            geom_line_batch_set(&scene->batches.lines,
                                                (int)(r->instance_slot + (uint32_t)s),
                                                world_a, world_b, render_color);
                        }
                        if (r->join_slot_start != 0xFFFFFFFF) {
                            for (int j = 0; j < helix_point_count - 2 && j < (int)r->join_count; j++) {
                                vec3_t world_pos = ecs_scene_transform_point_world(&t->world_matrix, helix_points[j + 1]);
                                geom_point_batch_set(&scene->batches.points,
                                                     (int)(r->join_slot_start + (uint32_t)j),
                                                     world_pos, render_color);
                            }
                        }
                        free(helix_points);
                    }
                    break;
                }
                case GEOM_TRIANGLE: {
                    vec3_t world_a = ecs_scene_transform_point_world(&t->world_matrix, g->data.triangle.a);
                    vec3_t world_b = ecs_scene_transform_point_world(&t->world_matrix, g->data.triangle.b);
                    vec3_t world_c = ecs_scene_transform_point_world(&t->world_matrix, g->data.triangle.c);
                    vec3_t normal = geom_triangle_compute_normal(world_a, world_b, world_c);
                    // Use per-vertex colors if available and not overridden by hover/selection
                    bool is_highlighted = ecs_has_id(w->world, e, w->Selected_tag) ||
                                          ecs_has_id(w->world, e, w->Hovered_tag);
                    if (g->data.triangle.has_vertex_colors && !is_highlighted) {
                        geom_triangle_batch_set_colored(&scene->batches.triangles, (int)r->instance_slot,
                                                        world_a, world_b, world_c, normal,
                                                        g->data.triangle.color_a,
                                                        g->data.triangle.color_b,
                                                        g->data.triangle.color_c);
                    } else {
                        geom_triangle_batch_set(&scene->batches.triangles, (int)r->instance_slot,
                                                world_a, world_b, world_c, normal, render_color);
                    }
                    break;
                }
                case GEOM_MESH: {
                    // Re-expand indexed faces into contiguous triangle batch slots
                    int face_count = geom_mesh_face_count(&g->data.mesh);
                    bool mesh_highlighted = ecs_has_id(w->world, e, w->Selected_tag) ||
                                            ecs_has_id(w->world, e, w->Hovered_tag);
                    for (int f = 0; f < face_count && f < (int)r->segment_count; f++) {
                        uint32_t i0 = g->data.mesh.indices[f * 3 + 0];
                        uint32_t i1 = g->data.mesh.indices[f * 3 + 1];
                        uint32_t i2 = g->data.mesh.indices[f * 3 + 2];

                        vec3_t wa = ecs_scene_transform_point_world(&t->world_matrix, g->data.mesh.vertices[i0]);
                        vec3_t wb = ecs_scene_transform_point_world(&t->world_matrix, g->data.mesh.vertices[i1]);
                        vec3_t wc = ecs_scene_transform_point_world(&t->world_matrix, g->data.mesh.vertices[i2]);
                        vec3_t fn = geom_triangle_compute_normal(wa, wb, wc);

                        if (g->data.mesh.vertex_colors && !mesh_highlighted) {
                            geom_triangle_batch_set_colored(&scene->batches.triangles,
                                (int)(r->instance_slot + (uint32_t)f),
                                wa, wb, wc, fn,
                                g->data.mesh.vertex_colors[i0],
                                g->data.mesh.vertex_colors[i1],
                                g->data.mesh.vertex_colors[i2]);
                        } else {
                            geom_triangle_batch_set(&scene->batches.triangles,
                                (int)(r->instance_slot + (uint32_t)f),
                                wa, wb, wc, fn, render_color);
                        }
                    }
                    break;
                }
                case GEOM_POINT_CLOUD: {
                    // Update all point cloud slots (segment_count holds point count)
                    int pc_count = g->data.point_cloud.count;
                    for (int p = 0; p < pc_count && p < (int)r->segment_count; p++) {
                        vec3_t world_pos = ecs_scene_transform_point_world(&t->world_matrix, g->data.point_cloud.points[p]);
                        // Use per-point color if available, otherwise use render_color (which may be hover/selection color)
                        vec4_t pc_color;
                        if (g->data.point_cloud.colors) {
                            // For point clouds with per-point colors, still show hover/selection
                            if (ecs_has_id(w->world, e, w->Selected_tag) || ecs_has_id(w->world, e, w->Hovered_tag)) {
                                pc_color = render_color;
                            } else {
                                pc_color = g->data.point_cloud.colors[p];
                            }
                        } else {
                            pc_color = render_color;
                        }
                        geom_point_batch_set(&scene->batches.points,
                                             (int)(r->instance_slot + (uint32_t)p),
                                             world_pos, pc_color);
                    }
                    break;
                }
                default:
                    break;
            }

            r->instance_dirty = false;
        }
    }

    ecs_query_fini(q);

    // Upload instance data to GPU
    geometry_batch_manager_upload(&scene->batches);
}

//------------------------------------------------------------------------------
// Scene Rendering
//------------------------------------------------------------------------------

static inline void ecs_scene_draw(ecs_scene_t *scene,
                                   const geom_triangle_params_t *tri_params,
                                   float aspect_ratio) {
    if (!scene->visible) return;
    geometry_batch_manager_draw(&scene->batches, tri_params, aspect_ratio);
}

//------------------------------------------------------------------------------
// Utility Functions
//------------------------------------------------------------------------------

static inline int ecs_scene_line_count(ecs_scene_t *scene) {
    return instance_buffer_count(&scene->batches.lines.instances);
}

static inline int ecs_scene_point_count(ecs_scene_t *scene) {
    return instance_buffer_count(&scene->batches.points.instances);
}

static inline int ecs_scene_triangle_count(ecs_scene_t *scene) {
    return instance_buffer_count(&scene->batches.triangles.instances);
}

//------------------------------------------------------------------------------
// GPU Picking Support
//------------------------------------------------------------------------------

// Helper: Check if an AABB of projected NDC points overlaps the pick viewport.
// Returns true if the entity MIGHT be visible (conservative — never false-negatives).
// ndc_min/max are the bounding box of all projected points.
// margin accounts for point/line radius extending beyond the projected center.
static inline bool pick_ndc_aabb_overlaps(float ndc_min_x, float ndc_max_x,
                                           float ndc_min_y, float ndc_max_y,
                                           float margin) {
    // The pick buffer viewport in NDC is [-1,1] x [-1,1].
    // With margin, an entity is culled only if ALL its points are beyond margin on one side.
    return !(ndc_min_x > margin || ndc_max_x < -margin ||
             ndc_min_y > margin || ndc_max_y < -margin);
}

// Helper: Update NDC bounding box with a new projected point
static inline void pick_ndc_aabb_expand(float ndc_x, float ndc_y,
                                         float *min_x, float *max_x,
                                         float *min_y, float *max_y) {
    if (ndc_x < *min_x) *min_x = ndc_x;
    if (ndc_x > *max_x) *max_x = ndc_x;
    if (ndc_y < *min_y) *min_y = ndc_y;
    if (ndc_y > *max_y) *max_y = ndc_y;
}

// Populate pick buffer with all visible, pickable entities.
// pick_mvp is the combined pick-projection * view * model MVP used for the pick buffer render.
static inline void ecs_scene_populate_pick_buffer(ecs_scene_t *scene, pick_buffer_t *pb, mat4_t pick_mvp) {
    if (!scene->visible) return;

    ecs_world_state_t *w = scene->world;

    // Query all entities with geometry, renderable, selectable, and transform
    ecs_query_t *q = ecs_query(w->world, {
        .terms = {
            { .id = w->GeometryComp_id },
            { .id = w->RenderableComp_id },
            { .id = w->SelectableComp_id },
            { .id = w->TransformComp_id }
        }
    });

    // Margin values for NDC culling (accounts for point/line screen-space radius)
    const float POINT_MARGIN = 1.5f;
    const float LINE_MARGIN  = 2.0f;
    const float TRI_MARGIN   = 2.0f;

    ecs_iter_t it = ecs_query_iter(w->world, q);
    while (ecs_query_next(&it)) {
        GeometryComp *geoms = ecs_field(&it, GeometryComp, 0);
        RenderableComp *renderables = ecs_field(&it, RenderableComp, 1);
        SelectableComp *selectables = ecs_field(&it, SelectableComp, 2);
        TransformComp *transforms = ecs_field(&it, TransformComp, 3);

        for (int i = 0; i < it.count; i++) {
            RenderableComp *r = &renderables[i];
            SelectableComp *s = &selectables[i];

            // Skip hidden or non-pickable entities
            if (!r->visible || !s->pickable) continue;
            if (s->pick_id == 0) continue;  // Invalid pick ID

            GeometryComp *g = &geoms[i];
            TransformComp *t = &transforms[i];
            EndPointsComp *endpoint_meta = ecs_world_get_endpoints(scene->world, it.entities[i]);
            bool endpoint_point = endpoint_meta && endpoint_meta->is_endpoint_point;

            // Add to pick buffer based on geometry type, with screen-space frustum culling
            switch (g->type) {
                case GEOM_LINE: {
                    vec3_t world_a = ecs_scene_transform_point_world(&t->world_matrix, g->data.line.a);
                    vec3_t world_b = ecs_scene_transform_point_world(&t->world_matrix, g->data.line.b);
                    // Frustum cull: project both endpoints
                    float na_x, na_y, nb_x, nb_y;
                    bool va = clip_space_project(pick_mvp, world_a, &na_x, &na_y);
                    bool vb = clip_space_project(pick_mvp, world_b, &nb_x, &nb_y);
                    if (!va && !vb) break;  // Both behind camera
                    float min_x = 1e30f, max_x = -1e30f, min_y = 1e30f, max_y = -1e30f;
                    if (va) pick_ndc_aabb_expand(na_x, na_y, &min_x, &max_x, &min_y, &max_y);
                    if (vb) pick_ndc_aabb_expand(nb_x, nb_y, &min_x, &max_x, &min_y, &max_y);
                    if (!va || !vb) { min_x = -1e30f; max_x = 1e30f; min_y = -1e30f; max_y = 1e30f; } // Straddles camera — don't cull
                    if (!pick_ndc_aabb_overlaps(min_x, max_x, min_y, max_y, LINE_MARGIN)) break;
                    pick_buffer_add_line(pb, world_a, world_b, s->pick_id);
                    break;
                }
                case GEOM_POINT: {
                    vec3_t world_pos = ecs_scene_transform_point_world(&t->world_matrix, g->data.point.point);
                    float ndc_x, ndc_y;
                    if (!clip_space_project(pick_mvp, world_pos, &ndc_x, &ndc_y)) break;
                    if (ndc_x > POINT_MARGIN || ndc_x < -POINT_MARGIN ||
                        ndc_y > POINT_MARGIN || ndc_y < -POINT_MARGIN) break;
                    if (endpoint_point) {
                        pick_buffer_add_overlay_point(pb, world_pos, s->pick_id);
                    } else {
                        pick_buffer_add_point(pb, world_pos, s->pick_id);
                    }
                    break;
                }
                case GEOM_POLYLINE: {
                    // Pre-cull: build AABB of all projected polyline vertices
                    int point_count = g->data.polyline.count;
                    float min_x = 1e30f, max_x = -1e30f, min_y = 1e30f, max_y = -1e30f;
                    bool any_behind = false, any_visible = false;
                    for (int j = 0; j < point_count; j++) {
                        vec3_t wp = ecs_scene_transform_point_world(&t->world_matrix, g->data.polyline.points[j]);
                        float nx, ny;
                        if (clip_space_project(pick_mvp, wp, &nx, &ny)) {
                            pick_ndc_aabb_expand(nx, ny, &min_x, &max_x, &min_y, &max_y);
                            any_visible = true;
                        } else {
                            any_behind = true;
                        }
                    }
                    if (!any_visible && !any_behind) break;
                    if (any_behind) { min_x = -1e30f; max_x = 1e30f; min_y = -1e30f; max_y = 1e30f; }
                    if (!pick_ndc_aabb_overlaps(min_x, max_x, min_y, max_y, LINE_MARGIN)) break;
                    // Passed culling — add all segments
                    for (int seg = 0; seg < point_count - 1; seg++) {
                        vec3_t world_a = ecs_scene_transform_point_world(&t->world_matrix, g->data.polyline.points[seg]);
                        vec3_t world_b = ecs_scene_transform_point_world(&t->world_matrix, g->data.polyline.points[seg + 1]);
                        pick_buffer_add_line(pb, world_a, world_b, s->pick_id);
                    }
                    for (int j = 1; j < point_count - 1; j++) {
                        vec3_t world_pos = ecs_scene_transform_point_world(&t->world_matrix, g->data.polyline.points[j]);
                        pick_buffer_add_point(pb, world_pos, s->pick_id);
                    }
                    break;
                }
                case GEOM_ARC: {
                    int arc_point_count = 0;
                    vec3_t *arc_points = ecs_scene_tessellate_arc(
                        g->data.arc.center, g->data.arc.radius,
                        g->data.arc.start_angle, g->data.arc.end_angle,
                        g->data.arc.normal, &arc_point_count
                    );
                    if (arc_points && arc_point_count >= 2) {
                        // Pre-cull via AABB
                        float min_x = 1e30f, max_x = -1e30f, min_y = 1e30f, max_y = -1e30f;
                        bool any_behind = false, any_visible = false;
                        for (int j = 0; j < arc_point_count; j++) {
                            vec3_t wp = ecs_scene_transform_point_world(&t->world_matrix, arc_points[j]);
                            float nx, ny;
                            if (clip_space_project(pick_mvp, wp, &nx, &ny)) {
                                pick_ndc_aabb_expand(nx, ny, &min_x, &max_x, &min_y, &max_y);
                                any_visible = true;
                            } else {
                                any_behind = true;
                            }
                        }
                        if (any_behind) { min_x = -1e30f; max_x = 1e30f; min_y = -1e30f; max_y = 1e30f; }
                        if ((any_visible || any_behind) &&
                            pick_ndc_aabb_overlaps(min_x, max_x, min_y, max_y, LINE_MARGIN)) {
                            for (int seg = 0; seg < arc_point_count - 1; seg++) {
                                vec3_t world_a = ecs_scene_transform_point_world(&t->world_matrix, arc_points[seg]);
                                vec3_t world_b = ecs_scene_transform_point_world(&t->world_matrix, arc_points[seg + 1]);
                                pick_buffer_add_line(pb, world_a, world_b, s->pick_id);
                            }
                            for (int j = 1; j < arc_point_count - 1; j++) {
                                vec3_t world_pos = ecs_scene_transform_point_world(&t->world_matrix, arc_points[j]);
                                pick_buffer_add_point(pb, world_pos, s->pick_id);
                            }
                        }
                        free(arc_points);
                    }
                    break;
                }
                case GEOM_POLYGON: {
                    int point_count = g->data.polygon.count;
                    float min_x = 1e30f, max_x = -1e30f, min_y = 1e30f, max_y = -1e30f;
                    bool any_behind = false, any_visible = false;
                    for (int j = 0; j < point_count; j++) {
                        vec3_t wp = ecs_scene_transform_point_world(&t->world_matrix, g->data.polygon.points[j]);
                        float nx, ny;
                        if (clip_space_project(pick_mvp, wp, &nx, &ny)) {
                            pick_ndc_aabb_expand(nx, ny, &min_x, &max_x, &min_y, &max_y);
                            any_visible = true;
                        } else {
                            any_behind = true;
                        }
                    }
                    if (any_behind) { min_x = -1e30f; max_x = 1e30f; min_y = -1e30f; max_y = 1e30f; }
                    if ((any_visible || any_behind) &&
                        pick_ndc_aabb_overlaps(min_x, max_x, min_y, max_y, LINE_MARGIN)) {
                        for (int seg = 0; seg < point_count; seg++) {
                            int next = (seg + 1) % point_count;
                            vec3_t world_a = ecs_scene_transform_point_world(&t->world_matrix, g->data.polygon.points[seg]);
                            vec3_t world_b = ecs_scene_transform_point_world(&t->world_matrix, g->data.polygon.points[next]);
                            pick_buffer_add_line(pb, world_a, world_b, s->pick_id);
                        }
                        for (int j = 0; j < point_count; j++) {
                            vec3_t world_pos = ecs_scene_transform_point_world(&t->world_matrix, g->data.polygon.points[j]);
                            pick_buffer_add_point(pb, world_pos, s->pick_id);
                        }
                    }
                    break;
                }
                case GEOM_BEZIER: {
                    int bezier_point_count = 0;
                    vec3_t *bezier_points = ecs_scene_tessellate_bezier(
                        g->data.bezier.p0, g->data.bezier.p1,
                        g->data.bezier.p2, g->data.bezier.p3,
                        g->data.bezier.segments, &bezier_point_count
                    );
                    if (bezier_points && bezier_point_count >= 2) {
                        float min_x = 1e30f, max_x = -1e30f, min_y = 1e30f, max_y = -1e30f;
                        bool any_behind = false, any_visible = false;
                        for (int j = 0; j < bezier_point_count; j++) {
                            vec3_t wp = ecs_scene_transform_point_world(&t->world_matrix, bezier_points[j]);
                            float nx, ny;
                            if (clip_space_project(pick_mvp, wp, &nx, &ny)) {
                                pick_ndc_aabb_expand(nx, ny, &min_x, &max_x, &min_y, &max_y);
                                any_visible = true;
                            } else {
                                any_behind = true;
                            }
                        }
                        if (any_behind) { min_x = -1e30f; max_x = 1e30f; min_y = -1e30f; max_y = 1e30f; }
                        if ((any_visible || any_behind) &&
                            pick_ndc_aabb_overlaps(min_x, max_x, min_y, max_y, LINE_MARGIN)) {
                            for (int seg = 0; seg < bezier_point_count - 1; seg++) {
                                vec3_t world_a = ecs_scene_transform_point_world(&t->world_matrix, bezier_points[seg]);
                                vec3_t world_b = ecs_scene_transform_point_world(&t->world_matrix, bezier_points[seg + 1]);
                                pick_buffer_add_line(pb, world_a, world_b, s->pick_id);
                            }
                            for (int j = 1; j < bezier_point_count - 1; j++) {
                                vec3_t world_pos = ecs_scene_transform_point_world(&t->world_matrix, bezier_points[j]);
                                pick_buffer_add_point(pb, world_pos, s->pick_id);
                            }
                        }
                        free(bezier_points);
                    }
                    break;
                }
                case GEOM_HELIX: {
                    int helix_point_count = 0;
                    vec3_t *helix_points = ecs_scene_tessellate_helix(
                        g->data.helix.axis_start, g->data.helix.axis_end,
                        g->data.helix.radius, g->data.helix.turns,
                        g->data.helix.segments, &helix_point_count
                    );
                    if (helix_points && helix_point_count >= 2) {
                        float min_x = 1e30f, max_x = -1e30f, min_y = 1e30f, max_y = -1e30f;
                        bool any_behind = false, any_visible = false;
                        for (int j = 0; j < helix_point_count; j++) {
                            vec3_t wp = ecs_scene_transform_point_world(&t->world_matrix, helix_points[j]);
                            float nx, ny;
                            if (clip_space_project(pick_mvp, wp, &nx, &ny)) {
                                pick_ndc_aabb_expand(nx, ny, &min_x, &max_x, &min_y, &max_y);
                                any_visible = true;
                            } else {
                                any_behind = true;
                            }
                        }
                        if (any_behind) { min_x = -1e30f; max_x = 1e30f; min_y = -1e30f; max_y = 1e30f; }
                        if ((any_visible || any_behind) &&
                            pick_ndc_aabb_overlaps(min_x, max_x, min_y, max_y, LINE_MARGIN)) {
                            for (int seg = 0; seg < helix_point_count - 1; seg++) {
                                vec3_t world_a = ecs_scene_transform_point_world(&t->world_matrix, helix_points[seg]);
                                vec3_t world_b = ecs_scene_transform_point_world(&t->world_matrix, helix_points[seg + 1]);
                                pick_buffer_add_line(pb, world_a, world_b, s->pick_id);
                            }
                            for (int j = 1; j < helix_point_count - 1; j++) {
                                vec3_t world_pos = ecs_scene_transform_point_world(&t->world_matrix, helix_points[j]);
                                pick_buffer_add_point(pb, world_pos, s->pick_id);
                            }
                        }
                        free(helix_points);
                    }
                    break;
                }
                case GEOM_POINT_CLOUD: {
                    // Pre-cull with center point
                    vec3_t center = ecs_scene_transform_point_world(&t->world_matrix, g->data.point_cloud.points[0]);
                    float ndc_cx, ndc_cy;
                    bool center_visible = clip_space_project(pick_mvp, center, &ndc_cx, &ndc_cy);
                    // For point clouds, use a generous margin since points are spread out
                    // If center is way off screen, skip entirely
                    if (center_visible && (ndc_cx > 10.0f || ndc_cx < -10.0f ||
                                           ndc_cy > 10.0f || ndc_cy < -10.0f)) break;

                    int pc_count = g->data.point_cloud.count;
                    int max_pick_points = 1000;
                    int step = (pc_count > max_pick_points) ? (pc_count / max_pick_points) : 1;

                    for (int p = 0; p < pc_count; p += step) {
                        vec3_t world_pos = ecs_scene_transform_point_world(&t->world_matrix, g->data.point_cloud.points[p]);
                        pick_buffer_add_point(pb, world_pos, s->pick_id);
                    }
                    break;
                }
                case GEOM_TRIANGLE: {
                    vec3_t world_a = ecs_scene_transform_point_world(&t->world_matrix, g->data.triangle.a);
                    vec3_t world_b = ecs_scene_transform_point_world(&t->world_matrix, g->data.triangle.b);
                    vec3_t world_c = ecs_scene_transform_point_world(&t->world_matrix, g->data.triangle.c);
                    // Frustum cull triangle
                    float na_x, na_y, nb_x, nb_y, nc_x, nc_y;
                    bool va = clip_space_project(pick_mvp, world_a, &na_x, &na_y);
                    bool vb = clip_space_project(pick_mvp, world_b, &nb_x, &nb_y);
                    bool vc = clip_space_project(pick_mvp, world_c, &nc_x, &nc_y);
                    if (!va && !vb && !vc) break;
                    float min_x = 1e30f, max_x = -1e30f, min_y = 1e30f, max_y = -1e30f;
                    if (va) pick_ndc_aabb_expand(na_x, na_y, &min_x, &max_x, &min_y, &max_y);
                    if (vb) pick_ndc_aabb_expand(nb_x, nb_y, &min_x, &max_x, &min_y, &max_y);
                    if (vc) pick_ndc_aabb_expand(nc_x, nc_y, &min_x, &max_x, &min_y, &max_y);
                    if (!va || !vb || !vc) { min_x = -1e30f; max_x = 1e30f; min_y = -1e30f; max_y = 1e30f; }
                    if (!pick_ndc_aabb_overlaps(min_x, max_x, min_y, max_y, TRI_MARGIN)) break;
                    pick_buffer_add_triangle(pb, world_a, world_b, world_c, s->pick_id);
                    break;
                }
                case GEOM_MESH: {
                    // Coarse cull: project a few sample vertices to check if mesh is near viewport
                    int face_count = geom_mesh_face_count(&g->data.mesh);
                    int vert_count = g->data.mesh.vertex_count;
                    if (face_count == 0 || vert_count == 0) break;

                    // Sample up to 8 vertices for coarse cull
                    float min_x = 1e30f, max_x = -1e30f, min_y = 1e30f, max_y = -1e30f;
                    bool any_behind = false, any_visible = false;
                    int sample_step = (vert_count > 8) ? (vert_count / 8) : 1;
                    for (int sv = 0; sv < vert_count; sv += sample_step) {
                        vec3_t wp = ecs_scene_transform_point_world(&t->world_matrix, g->data.mesh.vertices[sv]);
                        float nx, ny;
                        if (clip_space_project(pick_mvp, wp, &nx, &ny)) {
                            pick_ndc_aabb_expand(nx, ny, &min_x, &max_x, &min_y, &max_y);
                            any_visible = true;
                        } else {
                            any_behind = true;
                        }
                    }
                    if (!any_visible && !any_behind) break;
                    if (any_behind) { min_x = -1e30f; max_x = 1e30f; min_y = -1e30f; max_y = 1e30f; }
                    if (!pick_ndc_aabb_overlaps(min_x, max_x, min_y, max_y, TRI_MARGIN)) break;

                    for (int f = 0; f < face_count; f++) {
                        uint32_t i0 = g->data.mesh.indices[f * 3 + 0];
                        uint32_t i1 = g->data.mesh.indices[f * 3 + 1];
                        uint32_t i2 = g->data.mesh.indices[f * 3 + 2];
                        vec3_t wa = ecs_scene_transform_point_world(&t->world_matrix, g->data.mesh.vertices[i0]);
                        vec3_t wb = ecs_scene_transform_point_world(&t->world_matrix, g->data.mesh.vertices[i1]);
                        vec3_t wc = ecs_scene_transform_point_world(&t->world_matrix, g->data.mesh.vertices[i2]);
                        pick_buffer_add_triangle(pb, wa, wb, wc, s->pick_id);
                    }
                    break;
                }
                default:
                    break;
            }
        }
    }

    ecs_query_fini(q);
}

// Find entity by pick ID (returns 0 if not found)
static inline ecs_entity_t ecs_scene_find_entity_by_pick_id(ecs_scene_t *scene, uint32_t pick_id) {
    if (pick_id == 0) return 0;

    if (endpoint_pick_is_encoded(pick_id)) {
        uint32_t owner_pick_id = 0u;
        constraint_participant_role_t role = CONSTRAINT_PARTICIPANT_ROLE_UNSPECIFIED;
        if (endpoint_pick_decode(pick_id, &owner_pick_id, &role)) {
            ecs_entity_t owner = ecs_scene_find_entity_by_pick_id(scene, owner_pick_id);
            if (owner != 0) {
                EndPointsComp *endpoints = ecs_world_get_endpoints(scene->world, owner);
                endpoint_binding_t binding = {0};
                if (endpoints && endpoints_comp_find_binding(endpoints, role, &binding)) {
                    ecs_entity_t endpoint_entity = (ecs_entity_t)binding.endpoint_entity;
                    if (endpoint_entity != 0 && ecs_is_alive(scene->world->world, endpoint_entity)) {
                        return endpoint_entity;
                    }
                }
            }
        }
    }

    ecs_world_state_t *w = scene->world;

    // Query all entities with SelectableComp
    ecs_query_t *q = ecs_query(w->world, {
        .terms = {
            { .id = w->SelectableComp_id }
        }
    });

    ecs_entity_t found = 0;

    ecs_iter_t it = ecs_query_iter(w->world, q);
    while (ecs_query_next(&it)) {
        SelectableComp *selectables = ecs_field(&it, SelectableComp, 0);

        for (int i = 0; i < it.count; i++) {
            if (selectables[i].pick_id == pick_id) {
                found = it.entities[i];
                // Breaking early - must call ecs_iter_fini before exiting
                ecs_iter_fini(&it);
                goto found_exit;
            }
        }
    }
    // If we get here, ecs_query_next returned false which already finalized the iterator
    // Do NOT call ecs_iter_fini again - that would be a double-finalization error

found_exit:
    ecs_query_fini(q);
    return found;
}

// Clear hovered state from all entities and mark them dirty for re-render
static inline void ecs_scene_clear_all_hovered(ecs_scene_t *scene) {
    ecs_world_state_t *w = scene->world;

    // Query all entities with Hovered tag
    ecs_query_t *q = ecs_query(w->world, {
        .terms = {
            { .id = w->Hovered_tag }
        }
    });

    ecs_iter_t it = ecs_query_iter(w->world, q);
    while (ecs_query_next(&it)) {
        for (int i = 0; i < it.count; i++) {
            ecs_entity_t e = it.entities[i];
            ecs_world_clear_hovered(w, e);
            // Mark as dirty so the color gets updated on next frame
            RenderableComp *r = ecs_world_get_renderable(w, e);
            if (r) {
                r->instance_dirty = true;
            }
        }
    }

    ecs_query_fini(q);
}

// Update hover state based on pick buffer result
static inline void ecs_scene_update_hover(ecs_scene_t *scene, pick_buffer_t *pb) {
    // Clear previous hover (also marks those entities as dirty)
    ecs_scene_clear_all_hovered(scene);

    // Get newly hovered entity
    uint32_t pick_id = pick_buffer_get_hovered_id(pb);
    if (pick_id != 0) {
        ecs_entity_t e = ecs_scene_find_entity_by_pick_id(scene, pick_id);
        if (e != 0) {
            ecs_world_set_hovered(scene->world, e);

            // Mark newly hovered entity as dirty so it gets the hover color
            RenderableComp *r = ecs_world_get_renderable(scene->world, e);
            if (r) {
                r->instance_dirty = true;
            }
        }
    }
}

// Get currently hovered entity (returns 0 if none)
static inline ecs_entity_t ecs_scene_get_hovered_entity(ecs_scene_t *scene) {
    ecs_world_state_t *w = scene->world;

    // Query for entity with Hovered tag
    ecs_query_t *q = ecs_query(w->world, {
        .terms = {
            { .id = w->Hovered_tag }
        }
    });

    ecs_entity_t hovered = 0;

    ecs_iter_t it = ecs_query_iter(w->world, q);
    while (ecs_query_next(&it)) {
        if (it.count > 0) {
            hovered = it.entities[0];  // Return first hovered (should be only one)
            // Breaking early - must call ecs_iter_fini before exiting
            ecs_iter_fini(&it);
            break;
        }
    }
    // If we didn't break (hovered is still 0), ecs_query_next returned false
    // which already finalized the iterator - do NOT call ecs_iter_fini again

    ecs_query_fini(q);
    return hovered;
}

//------------------------------------------------------------------------------
// Script round-trip façade
//------------------------------------------------------------------------------

#include "../scripting/sketch_script_parse.h"
#include "../scripting/sketch_script_emit.h"
#include "../scripting/sketch_script_apply.h"

static inline scene_script_io_state_t* scene_script_io_state_for_sketch(ecs_scene_t *scene,
                                                                         ecs_entity_t sketch,
                                                                         bool create_if_missing) {
    if (!scene || !scene_is_sketch(scene, sketch)) return NULL;
    for (int i = 0; i < ECS_SCENE_SCRIPT_IO_MAX_SKETCHES; i++) {
        if (scene->script_io_states[i].occupied && scene->script_io_states[i].sketch == sketch) {
            return &scene->script_io_states[i];
        }
    }
    if (!create_if_missing) return NULL;
    for (int i = 0; i < ECS_SCENE_SCRIPT_IO_MAX_SKETCHES; i++) {
        if (!scene->script_io_states[i].occupied) {
            memset(&scene->script_io_states[i], 0, sizeof(scene->script_io_states[i]));
            scene->script_io_states[i].occupied = true;
            scene->script_io_states[i].sketch = sketch;
            return &scene->script_io_states[i];
        }
    }
    return NULL;
}

static inline const scene_script_io_state_t* scene_script_io_state_for_sketch_const(const ecs_scene_t *scene,
                                                                                      ecs_entity_t sketch) {
    if (!scene || sketch == 0) return NULL;
    for (int i = 0; i < ECS_SCENE_SCRIPT_IO_MAX_SKETCHES; i++) {
        if (scene->script_io_states[i].occupied && scene->script_io_states[i].sketch == sketch) {
            return &scene->script_io_states[i];
        }
    }
    return NULL;
}

static inline void scene_script_io_state_copy_from_model(scene_script_io_state_t *state,
                                                         const sketch_script_model_t *model) {
    if (!state || !model) return;
    state->input_count = (model->input_count > ECS_SCENE_SCRIPT_IO_MAX_INPUTS)
        ? ECS_SCENE_SCRIPT_IO_MAX_INPUTS : model->input_count;
    state->output_count = (model->output_count > ECS_SCENE_SCRIPT_IO_MAX_OUTPUTS)
        ? ECS_SCENE_SCRIPT_IO_MAX_OUTPUTS : model->output_count;
    memset(state->inputs, 0, sizeof(state->inputs));
    memset(state->outputs, 0, sizeof(state->outputs));
    for (uint32_t i = 0; i < state->input_count; i++) {
        scene_script_io_entry_t *dst = &state->inputs[i];
        const sketch_script_io_model_t *src = &model->inputs[i];
        strncpy(dst->id, src->id, sizeof(dst->id) - 1);
        dst->id[sizeof(dst->id) - 1] = '\0';
        dst->value = src->value;
        dst->has_min = src->has_min;
        dst->min_value = src->min_value;
        dst->has_max = src->has_max;
        dst->max_value = src->max_value;
        dst->has_step = src->has_step;
        dst->step_value = src->step_value;
    }
    for (uint32_t i = 0; i < state->output_count; i++) {
        scene_script_io_entry_t *dst = &state->outputs[i];
        const sketch_script_io_model_t *src = &model->outputs[i];
        strncpy(dst->id, src->id, sizeof(dst->id) - 1);
        dst->id[sizeof(dst->id) - 1] = '\0';
        dst->value = src->value;
    }
}

static inline int scene_script_io_descriptor_count(const ecs_scene_t *scene, ecs_entity_t sketch) {
    const scene_script_io_state_t *state = scene_script_io_state_for_sketch_const(scene, sketch);
    if (!state) return 0;
    return (int)state->input_count + (int)state->output_count;
}

static inline bool scene_script_io_descriptor_at(const ecs_scene_t *scene,
                                                 ecs_entity_t sketch,
                                                 int index,
                                                 scene_script_io_descriptor_t *out_desc) {
    if (!out_desc || index < 0) return false;
    const scene_script_io_state_t *state = scene_script_io_state_for_sketch_const(scene, sketch);
    if (!state) return false;
    memset(out_desc, 0, sizeof(*out_desc));
    if (index < (int)state->input_count) {
        const scene_script_io_entry_t *src = &state->inputs[index];
        strncpy(out_desc->id, src->id, sizeof(out_desc->id) - 1);
        out_desc->id[sizeof(out_desc->id) - 1] = '\0';
        out_desc->is_input = true;
        out_desc->value = src->value;
        out_desc->has_min = src->has_min;
        out_desc->min_value = src->min_value;
        out_desc->has_max = src->has_max;
        out_desc->max_value = src->max_value;
        out_desc->has_step = src->has_step;
        out_desc->step_value = src->step_value;
        return true;
    }
    int output_index = index - (int)state->input_count;
    if (output_index < 0 || output_index >= (int)state->output_count) return false;
    const scene_script_io_entry_t *src = &state->outputs[output_index];
    strncpy(out_desc->id, src->id, sizeof(out_desc->id) - 1);
    out_desc->id[sizeof(out_desc->id) - 1] = '\0';
    out_desc->is_input = false;
    out_desc->value = src->value;
    return true;
}

static inline bool scene_script_io_read_value(const ecs_scene_t *scene,
                                              ecs_entity_t sketch,
                                              const char *id,
                                              double *out_value,
                                              bool *out_is_input) {
    if (!scene || !id || id[0] == '\0' || !out_value) return false;
    const scene_script_io_state_t *state = scene_script_io_state_for_sketch_const(scene, sketch);
    if (!state) return false;
    for (uint32_t i = 0; i < state->input_count; i++) {
        if (strcmp(state->inputs[i].id, id) == 0) {
            *out_value = state->inputs[i].value;
            if (out_is_input) *out_is_input = true;
            return true;
        }
    }
    for (uint32_t i = 0; i < state->output_count; i++) {
        if (strcmp(state->outputs[i].id, id) == 0) {
            *out_value = state->outputs[i].value;
            if (out_is_input) *out_is_input = false;
            return true;
        }
    }
    return false;
}

static inline bool scene_script_io_apply_input_value(ecs_scene_t *scene,
                                                     ecs_entity_t sketch,
                                                     const char *id,
                                                     double value,
                                                     sketch_script_error_t *out_error) {
    if (!scene || !scene_is_sketch(scene, sketch) || !id || id[0] == '\0') {
        if (out_error) {
            out_error->line = 0;
            out_error->column = 0;
            snprintf(out_error->message, sizeof(out_error->message), "Invalid scene/sketch/input id.");
            out_error->message[sizeof(out_error->message) - 1] = '\0';
        }
        return false;
    }
    scene_script_io_state_t *state = scene_script_io_state_for_sketch(scene, sketch, false);
    if (!state) {
        if (out_error) {
            out_error->line = 0;
            out_error->column = 0;
            snprintf(out_error->message, sizeof(out_error->message), "No script IO state for sketch.");
            out_error->message[sizeof(out_error->message) - 1] = '\0';
        }
        return false;
    }
    bool found = false;
    for (uint32_t i = 0; i < state->input_count; i++) {
        if (strcmp(state->inputs[i].id, id) == 0) {
            state->inputs[i].value = value;
            found = true;
            break;
        }
    }
    if (!found) {
        if (out_error) {
            out_error->line = 0;
            out_error->column = 0;
            snprintf(out_error->message, sizeof(out_error->message), "Input id '%s' not found.", id);
            out_error->message[sizeof(out_error->message) - 1] = '\0';
        }
        return false;
    }

    char script[ECS_SCENE_SCRIPT_TEXT_BUFFER_SIZE] = {0};
    if (!scene_script_emit_for_sketch(scene, sketch, script, sizeof(script), out_error)) {
        return false;
    }
    return scene_script_apply_commit(scene, sketch, script, out_error);
}

static inline bool scene_script_preview_parse(ecs_scene_t *scene,
                                              ecs_entity_t sketch,
                                              const char *script_text,
                                              sketch_script_error_t *out_error) {
    return sketch_script_apply_preview_model(scene, sketch, script_text, out_error);
}

static inline bool scene_script_apply_commit(ecs_scene_t *scene,
                                             ecs_entity_t sketch,
                                             const char *script_text,
                                             sketch_script_error_t *out_error) {
    scene_script_io_state_t *io_state = scene_script_io_state_for_sketch(scene, sketch, true);
    scene_script_io_state_t io_state_before = {0};
    if (io_state) {
        io_state_before = *io_state;
    }
    char before_script[ECS_SCENE_SCRIPT_TEXT_BUFFER_SIZE] = {0};
    bool capture_before = scene_script_emit_for_sketch(scene, sketch, before_script, sizeof(before_script), out_error);
    if (!capture_before) return false;

    if (!sketch_script_apply_commit_model(scene, sketch, script_text, out_error)) return false;

    sketch_script_model_t model = {0};
    if (!sketch_script_parse_model(script_text, &model, out_error)) {
        sketch_script_error_t rollback_error = {0};
        (void)sketch_script_apply_commit_model(scene, sketch, before_script, &rollback_error);
        if (io_state) *io_state = io_state_before;
        return false;
    }
    if (io_state) {
        scene_script_io_state_copy_from_model(io_state, &model);
    }

    if (!scene->script_apply_undo_suppressed && scene->script_undo_redo) {
        char after_script[ECS_SCENE_SCRIPT_TEXT_BUFFER_SIZE] = {0};
        if (!scene_script_emit_for_sketch(scene, sketch, after_script, sizeof(after_script), out_error)) {
            sketch_script_error_t rollback_error = {0};
            scene->script_apply_undo_suppressed = true;
            (void)sketch_script_apply_commit_model(scene, sketch, before_script, &rollback_error);
            scene->script_apply_undo_suppressed = false;
            if (io_state) *io_state = io_state_before;
            return false;
        }

        undo_command_t cmd = {0};
        cmd.type = CMD_SCRIPT_APPLY_TRANSACTION;
        cmd.data.script_apply_transaction.sketch_id = (uint64_t)sketch;
        cmd.data.script_apply_transaction.before_script = undo_strdup(before_script);
        cmd.data.script_apply_transaction.after_script = undo_strdup(after_script);
        if (!cmd.data.script_apply_transaction.before_script ||
            !cmd.data.script_apply_transaction.after_script) {
            if (cmd.data.script_apply_transaction.before_script) free(cmd.data.script_apply_transaction.before_script);
            if (cmd.data.script_apply_transaction.after_script) free(cmd.data.script_apply_transaction.after_script);
            sketch_script_error_t rollback_error = {0};
            scene->script_apply_undo_suppressed = true;
            (void)sketch_script_apply_commit_model(scene, sketch, before_script, &rollback_error);
            scene->script_apply_undo_suppressed = false;
            if (io_state) *io_state = io_state_before;
            if (out_error && out_error->message[0] == '\0') {
                out_error->line = 0;
                out_error->column = 0;
                snprintf(out_error->message, sizeof(out_error->message), "Failed recording undo transaction for script apply.");
                out_error->message[sizeof(out_error->message) - 1] = '\0';
            }
            return false;
        }
        undo_redo_push(scene->script_undo_redo, &cmd);
    }

    return true;
}

static inline void scene_script_bind_undo_redo(ecs_scene_t *scene, undo_redo_t *undo_redo) {
    if (!scene) return;
    scene->script_undo_redo = undo_redo;
}

#endif // ECS_SCENE_H
