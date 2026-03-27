#ifndef MDCAD_CGLM_ENTRY_H
#define MDCAD_CGLM_ENTRY_H

#include "math_conventions.h"

#ifndef CGLM_FORCE_DEPTH_ZERO_TO_ONE
#define CGLM_FORCE_DEPTH_ZERO_TO_ONE
#endif

#include <cglm/cglm.h>
#include <cglm/struct.h>

#if defined(_MSC_VER) && !defined(__clang__)
#define MDCAD_CGLM_STATIC_ASSERT(name, condition) typedef char name[(condition) ? 1 : -1]
#else
#define MDCAD_CGLM_STATIC_ASSERT(name, condition) _Static_assert((condition), #name)
#endif

/*
 * Subsystems may choose the cglm array API or struct API per file, but all
 * must honor the global contract in math_conventions.h.
 *
 * compare, validate, and bench helpers must live in adjacent headers rather
 * than expanding this entrypoint into a wrapper API.
 */
MDCAD_CGLM_STATIC_ASSERT(mdcad_cglm_clip_control_must_be_rh_zo,
                         CGLM_CONFIG_CLIP_CONTROL == CGLM_CLIP_CONTROL_RH_ZO);
MDCAD_CGLM_STATIC_ASSERT(mdcad_cglm_mat4_size_must_be_16_floats,
                         sizeof(mat4) == sizeof(float) * 16);
MDCAD_CGLM_STATIC_ASSERT(mdcad_cglm_struct_layout_must_match,
                         sizeof(mat4s) == sizeof(mat4));

static inline void mdcad_cglm_compile_anchor(void) {
    (void)sizeof(mat4);
    (void)sizeof(mat4s);
}

#undef MDCAD_CGLM_STATIC_ASSERT

#endif
