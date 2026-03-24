#ifndef MDCAD_CGLM_ENTRY_H
#define MDCAD_CGLM_ENTRY_H

#include "math_conventions.h"
#include <cglm/cglm.h>
#include <cglm/struct.h>

/*
 * Subsystems may choose the cglm array API or struct API per file, but all
 * must honor the global contract in math_conventions.h.
 */
_Static_assert(sizeof(mat4) == sizeof(float) * 16, "cglm mat4 must remain 16 floats");
_Static_assert(sizeof(mat4s) == sizeof(mat4), "cglm struct API must stay layout-compatible with mat4");

static inline void mdcad_cglm_compile_anchor(void) {
    (void)sizeof(mat4);
    (void)sizeof(mat4s);
}

#endif
