//------------------------------------------------------------------------------
// sketch_script_contract.h - Declarative Phase 13 script contract baseline
//------------------------------------------------------------------------------
#ifndef SKETCH_SCRIPT_CONTRACT_H
#define SKETCH_SCRIPT_CONTRACT_H

#include "../components/geometry_comp.h"
#include "../components/constraint_comp.h"
#include "../constraints/constraint_types.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

typedef struct lua_State lua_State;
typedef struct {
    int line;
    int column;
    char message[192];
} sketch_script_error_t;

typedef struct {
    const char *type;
    bool has_id;
    const char *id;
} sketch_script_entity_decl_t;

typedef struct {
    const char *type;
    bool has_id;
    const char *id;
    uint32_t participant_count;
} sketch_script_constraint_decl_t;

static inline bool sketch_script_contract_geometry_type_allowed(const char *type) {
    if (!type) return false;
    return strcmp(type, "point") == 0 ||
           strcmp(type, "line") == 0 ||
           strcmp(type, "arc") == 0 ||
           strcmp(type, "circle") == 0;
}

static inline bool sketch_script_contract_constraint_type_allowed(const char *type) {
    if (!type) return false;
    for (int i = 0; i < CONSTRAINT_TYPE_COUNT; i++) {
        if (strcmp(type, constraint_type_display_name((constraint_type_t)i)) == 0) {
            return true;
        }
    }
    return false;
}

static inline bool sketch_script_contract_validate_entity_decl(const sketch_script_entity_decl_t *decl,
                                                               char *error_text,
                                                               size_t error_text_size) {
    if (!decl) return false;
    if (!decl->has_id || !decl->id || decl->id[0] == '\0') {
        if (error_text && error_text_size > 0) {
            snprintf(error_text, error_text_size, "Entity declaration must include non-empty id.");
            error_text[error_text_size - 1] = '\0';
        }
        return false;
    }
    if (!sketch_script_contract_geometry_type_allowed(decl->type)) {
        if (error_text && error_text_size > 0) {
            snprintf(error_text, error_text_size,
                     "Unsupported entity type '%s'. Phase 13 supports point/line/arc/circle.",
                     decl->type ? decl->type : "(null)");
            error_text[error_text_size - 1] = '\0';
        }
        return false;
    }
    return true;
}

static inline bool sketch_script_contract_validate_constraint_decl(const sketch_script_constraint_decl_t *decl,
                                                                   char *error_text,
                                                                   size_t error_text_size) {
    if (!decl) return false;
    if (!decl->has_id || !decl->id || decl->id[0] == '\0') {
        if (error_text && error_text_size > 0) {
            snprintf(error_text, error_text_size, "Constraint declaration must include non-empty id.");
            error_text[error_text_size - 1] = '\0';
        }
        return false;
    }
    if (!sketch_script_contract_constraint_type_allowed(decl->type)) {
        if (error_text && error_text_size > 0) {
            snprintf(error_text, error_text_size,
                     "Unsupported constraint type '%s' for Phase 13 contract.",
                     decl->type ? decl->type : "(null)");
            error_text[error_text_size - 1] = '\0';
        }
        return false;
    }
    return decl->participant_count <= CONSTRAINT_MAX_PARTICIPANTS;
}

static inline bool sketch_script_contract_validate(lua_State *L, sketch_script_error_t *out_error) {
    (void)L;
    if (out_error) {
        out_error->line = 0;
        out_error->column = 0;
        out_error->message[0] = '\0';
    }
    return true;
}

#endif // SKETCH_SCRIPT_CONTRACT_H
