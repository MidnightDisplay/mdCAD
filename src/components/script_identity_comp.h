//------------------------------------------------------------------------------
// script_identity_comp.h - Stable script-local identity for sketch entities
//------------------------------------------------------------------------------
#ifndef SCRIPT_IDENTITY_COMP_H
#define SCRIPT_IDENTITY_COMP_H

#include <string.h>
#include <stdbool.h>

#define SCRIPT_LOCAL_ID_MAX 64

typedef struct {
    char script_local_id[SCRIPT_LOCAL_ID_MAX];
} ScriptIdentityComp;

static inline ScriptIdentityComp script_identity_comp_default(void) {
    ScriptIdentityComp id;
    memset(&id, 0, sizeof(id));
    return id;
}

static inline ScriptIdentityComp script_identity_comp_make(const char *script_local_id) {
    ScriptIdentityComp id = script_identity_comp_default();
    if (script_local_id) {
        strncpy(id.script_local_id, script_local_id, SCRIPT_LOCAL_ID_MAX - 1);
        id.script_local_id[SCRIPT_LOCAL_ID_MAX - 1] = '\0';
    }
    return id;
}

static inline bool script_identity_comp_has_id(const ScriptIdentityComp *id) {
    return id && id->script_local_id[0] != '\0';
}

#endif // SCRIPT_IDENTITY_COMP_H
