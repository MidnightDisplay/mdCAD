//------------------------------------------------------------------------------
// sketch_geometry_state_comp.h - Per-geometry sketch manager state
//------------------------------------------------------------------------------
#ifndef SKETCH_GEOMETRY_STATE_COMP_H
#define SKETCH_GEOMETRY_STATE_COMP_H

#include <stdbool.h>

typedef struct {
    bool fixed;
} SketchGeometryStateComp;

static inline SketchGeometryStateComp sketch_geometry_state_comp_default(void) {
    return (SketchGeometryStateComp){
        .fixed = false
    };
}

static inline const char* sketch_geometry_state_label(SketchGeometryStateComp state) {
    return state.fixed ? "fixed" : "loose";
}

#endif // SKETCH_GEOMETRY_STATE_COMP_H
