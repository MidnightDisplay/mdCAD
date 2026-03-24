#ifndef MDCAD_CGLM_ENTRY_H
#define MDCAD_CGLM_ENTRY_H

#include "math_conventions.h"

#ifndef CGLM_FORCE_DEPTH_ZERO_TO_ONE
#define CGLM_FORCE_DEPTH_ZERO_TO_ONE
#endif

#include <cglm/cglm.h>
#include <cglm/struct.h>

/*
 * Subsystems may choose the cglm array API or struct API per file, but all
 * must honor the global contract in math_conventions.h.
 *
 * compare, validate, and bench helpers must live in adjacent headers rather
 * than expanding this entrypoint into a wrapper API.
 */
_Static_assert(CGLM_CONFIG_CLIP_CONTROL == CGLM_CLIP_CONTROL_RH_ZO,
               "mdCAD cglm entrypoint must use RH_ZO clip control");
_Static_assert(sizeof(mat4) == sizeof(float) * 16, "cglm mat4 must remain 16 floats");
_Static_assert(sizeof(mat4s) == sizeof(mat4), "cglm struct API must stay layout-compatible with mat4");

static inline void mdcad_cglm_compile_anchor(void) {
    (void)sizeof(mat4);
    (void)sizeof(mat4s);
}

#endif
