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

typedef enum {
    SKETCH_STATUS_SOLVED,
    SKETCH_STATUS_LOOSE,
    SKETCH_STATUS_FIXED,
    SKETCH_STATUS_ERROR
} sketch_status_t;

typedef struct {
    sketch_status_t status;
    vec4_t color;               // Sketch-level inherited color
    int geometry_count;         // Derived from sketch-owned geometry children
    int fixed_geometry_count;   // Derived from sketch-owned geometry state
    int constraint_count;       // Derived from sketch-owned constraint children
    uint32_t next_geometry_name_index[GEOM_TYPE_COUNT];
    uint32_t next_constraint_name_index[CONSTRAINT_TYPE_COUNT];
} SketchComp;

static inline SketchComp sketch_comp_default(void) {
    return (SketchComp){
        .status = SKETCH_STATUS_LOOSE,
        .color = vec4_make(1.0f, 1.0f, 1.0f, 1.0f),
        .geometry_count = 0,
        .fixed_geometry_count = 0,
        .constraint_count = 0
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
