//------------------------------------------------------------------------------
// sketch_comp.h - Sketch container component for ECS entities
//------------------------------------------------------------------------------
#ifndef SKETCH_COMP_H
#define SKETCH_COMP_H

#include "component_types.h"
#include "geometry_comp.h"
#include "constraint_comp.h"
#include <math.h>
#include <stdbool.h>
#include <stdint.h>

typedef enum {
    SKETCH_STATUS_SOLVED,
    SKETCH_STATUS_LOOSE,
    SKETCH_STATUS_FIXED,
    SKETCH_STATUS_ERROR
} sketch_status_t;

typedef enum {
    SKETCH_SOLVER_DIAG_INFO,
    SKETCH_SOLVER_DIAG_WARNING,
    SKETCH_SOLVER_DIAG_ERROR
} sketch_solver_diagnostic_severity_t;

#define SKETCH_SOLVER_DIAGNOSTIC_MESSAGE_MAX 192
#define SKETCH_SOLVER_DIAGNOSTIC_TIMESTAMP_MAX 32
#define SKETCH_SOLVER_DIAGNOSTICS_MAX 100

typedef struct {
    sketch_solver_diagnostic_severity_t severity;
    char timestamp[SKETCH_SOLVER_DIAGNOSTIC_TIMESTAMP_MAX];
    char message[SKETCH_SOLVER_DIAGNOSTIC_MESSAGE_MAX];
    uint64_t implicated_constraint;
} sketch_solver_diagnostic_t;

typedef struct {
    sketch_status_t status;
    bool auto_solve_enabled;
    bool auto_solve_pending;
    uint32_t solve_request_serial;
    uint32_t solve_completed_serial;
    uint64_t last_solve_timestamp_ms;
    uint32_t solver_backend_id;
    vec4_t color;               // Sketch-level inherited color
    int geometry_count;         // Derived from sketch-owned geometry children
    int fixed_geometry_count;   // Derived from sketch-owned geometry state
    int constraint_count;       // Derived from sketch-owned constraint children
    uint32_t next_geometry_name_index[GEOM_TYPE_COUNT];
    uint32_t next_constraint_name_index[CONSTRAINT_TYPE_COUNT];
    uint32_t diagnostics_count;
    uint32_t diagnostics_head;
    sketch_solver_diagnostic_t diagnostics[SKETCH_SOLVER_DIAGNOSTICS_MAX];
} SketchComp;

static inline SketchComp sketch_comp_default(void) {
    return (SketchComp){
        .status = SKETCH_STATUS_LOOSE,
        .auto_solve_enabled = true,
        .auto_solve_pending = false,
        .solve_request_serial = 0,
        .solve_completed_serial = 0,
        .last_solve_timestamp_ms = 0,
        .solver_backend_id = 1,
        .color = vec4_make(1.0f, 1.0f, 1.0f, 1.0f),
        .geometry_count = 0,
        .fixed_geometry_count = 0,
        .constraint_count = 0,
        .diagnostics_count = 0,
        .diagnostics_head = 0
    };
}

static inline const char* sketch_status_name(sketch_status_t status) {
    switch (status) {
        case SKETCH_STATUS_SOLVED: return "solved";
        case SKETCH_STATUS_LOOSE:  return "loose";
        case SKETCH_STATUS_FIXED:  return "fixed";
        case SKETCH_STATUS_ERROR:  return "error";
        default:                   return "error";
    }
}

static inline const char* sketch_solver_diagnostic_severity_name(sketch_solver_diagnostic_severity_t severity) {
    switch (severity) {
        case SKETCH_SOLVER_DIAG_INFO: return "INFO";
        case SKETCH_SOLVER_DIAG_WARNING: return "WARNING";
        case SKETCH_SOLVER_DIAG_ERROR: return "ERROR";
        default: return "ERROR";
    }
}

static inline bool sketch_comp_color_is_black_rgb(vec4_t color) {
    const float eps = 0.0001f;
    return fabsf(color.x) <= eps && fabsf(color.y) <= eps && fabsf(color.z) <= eps;
}

static inline bool sketch_comp_geometry_overrides_inherited_color(vec4_t geometry_color) {
    return !sketch_comp_color_is_black_rgb(geometry_color);
}

// D-09 color policy:
// - Geometry with non-black RGB uses its explicit geometry color
// - Geometry with black RGB inherits sketch color RGB
static inline vec4_t sketch_comp_resolve_geometry_color(vec4_t sketch_color, vec4_t geometry_color) {
    if (sketch_comp_geometry_overrides_inherited_color(geometry_color)) {
        return geometry_color;
    }
    return vec4_make(sketch_color.x, sketch_color.y, sketch_color.z, geometry_color.w);
}

#endif // SKETCH_COMP_H
