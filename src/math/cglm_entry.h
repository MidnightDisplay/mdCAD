#ifndef MDCAD_CGLM_ENTRY_H
#define MDCAD_CGLM_ENTRY_H

#include <cglm/cglm.h>
#include <cglm/struct.h>

static inline void mdcad_cglm_compile_anchor(void) {
    (void)sizeof(mat4);
    (void)sizeof(mat4s);
}

#endif
