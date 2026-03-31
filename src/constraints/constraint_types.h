//------------------------------------------------------------------------------
// constraint_types.h - v1.2 constraint type helpers and legality checks
//------------------------------------------------------------------------------
#ifndef CONSTRAINT_TYPES_H
#define CONSTRAINT_TYPES_H

#include <stdbool.h>
#include <stdint.h>
#include "../components/constraint_comp.h"
#include "../components/geometry_comp.h"

typedef enum {
    CONSTRAINT_PARTICIPANT_ROLE_UNSPECIFIED = 0,
    CONSTRAINT_PARTICIPANT_ROLE_ENTITY = 1,
    CONSTRAINT_PARTICIPANT_ROLE_POINT_A = 2,
    CONSTRAINT_PARTICIPANT_ROLE_POINT_B = 3,
    CONSTRAINT_PARTICIPANT_ROLE_CENTER = 4
} constraint_participant_role_t;

typedef struct {
    uint32_t count;
    geometry_type_t geometry_types[CONSTRAINT_MAX_PARTICIPANTS];
    constraint_participant_role_t roles[CONSTRAINT_MAX_PARTICIPANTS];
} constraint_selection_signature_t;

static inline const char* constraint_type_display_name(constraint_type_t type) {
    static const char *names[CONSTRAINT_TYPE_COUNT] = {
        "Fixed",
        "Coincident",
        "Collinear",
        "Parallel",
        "Perpendicular",
        "Along X",
        "Along Y",
        "Along Z",
        "Coradial",
        "Concentric",
        "Length",
        "Angle",
        "Tangential"
    };
    if (type < 0 || type >= CONSTRAINT_TYPE_COUNT) return "Constraint";
    return names[type];
}

static inline bool constraint_type_is_dimensional(constraint_type_t type) {
    return type == CONSTRAINT_LENGTH || type == CONSTRAINT_ANGLE;
}

static inline bool constraint_type_allows_geometry(geometry_type_t type) {
    return type == GEOM_POINT || type == GEOM_LINE || type == GEOM_ARC;
}

static inline bool constraint_type_is_selection_legal(constraint_selection_signature_t *sig,
                                                      constraint_type_t type) {
    if (!sig || sig->count == 0 || sig->count > CONSTRAINT_MAX_PARTICIPANTS) return false;

    for (uint32_t i = 0; i < sig->count; i++) {
        if (!constraint_type_allows_geometry(sig->geometry_types[i])) return false;
    }

    if (type == CONSTRAINT_FIXED) {
        return sig->count == 1;
    }
    if (type == CONSTRAINT_COINCIDENT) {
        return sig->count == 2;
    }
    if (type == CONSTRAINT_COLLINEAR || type == CONSTRAINT_PARALLEL ||
        type == CONSTRAINT_PERPENDICULAR || type == CONSTRAINT_ANGLE) {
        return sig->count == 2 &&
               sig->geometry_types[0] == GEOM_LINE &&
               sig->geometry_types[1] == GEOM_LINE;
    }
    if (type == CONSTRAINT_ALONG_X || type == CONSTRAINT_ALONG_Y || type == CONSTRAINT_ALONG_Z) {
        return sig->count == 1 && sig->geometry_types[0] == GEOM_LINE;
    }
    if (type == CONSTRAINT_CORADIAL || type == CONSTRAINT_CONCENTRIC || type == CONSTRAINT_TANGENTIAL) {
        return sig->count == 2 &&
               sig->geometry_types[0] == GEOM_ARC &&
               sig->geometry_types[1] == GEOM_ARC;
    }
    if (type == CONSTRAINT_LENGTH) {
        return sig->count == 1 &&
               (sig->geometry_types[0] == GEOM_LINE || sig->geometry_types[0] == GEOM_ARC);
    }
    return false;
}

#endif // CONSTRAINT_TYPES_H
