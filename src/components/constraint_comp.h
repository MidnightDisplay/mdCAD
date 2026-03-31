//------------------------------------------------------------------------------
// constraint_comp.h - Constraint component for sketch constraints
//------------------------------------------------------------------------------
#ifndef CONSTRAINT_COMP_H
#define CONSTRAINT_COMP_H

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

typedef enum {
    CONSTRAINT_FIXED,
    CONSTRAINT_COINCIDENT,
    CONSTRAINT_COLLINEAR,
    CONSTRAINT_PARALLEL,
    CONSTRAINT_PERPENDICULAR,
    CONSTRAINT_ALONG_X,
    CONSTRAINT_ALONG_Y,
    CONSTRAINT_ALONG_Z,
    CONSTRAINT_CORADIAL,
    CONSTRAINT_CONCENTRIC,
    CONSTRAINT_LENGTH,
    CONSTRAINT_ANGLE,
    CONSTRAINT_TANGENTIAL,
    CONSTRAINT_TYPE_COUNT
} constraint_type_t;

#define CONSTRAINT_MAX_PARTICIPANTS 8

typedef struct {
    constraint_type_t type;
    bool has_value;                             // true for dimensional constraints
    bool driven;                                // CONS-05 for LENGTH/ANGLE
    float value;                                // LENGTH/ANGLE value
    uint32_t participant_count;
    uint64_t participants[CONSTRAINT_MAX_PARTICIPANTS];
} ConstraintComp;

static inline ConstraintComp constraint_comp_default(void) {
    ConstraintComp c;
    memset(&c, 0, sizeof(c));
    c.type = CONSTRAINT_COINCIDENT;
    return c;
}

static inline bool constraint_comp_is_dimensional(const ConstraintComp *c) {
    if (!c) return false;
    return c->type == CONSTRAINT_LENGTH || c->type == CONSTRAINT_ANGLE;
}

static inline const char* constraint_type_name(constraint_type_t type) {
    static const char *names[] = {
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
    if (type < 0 || type >= CONSTRAINT_TYPE_COUNT) {
        return "Constraint";
    }
    return names[type];
}

static inline ConstraintComp constraint_comp_make(constraint_type_t type,
                                                  const uint64_t *participants,
                                                  uint32_t participant_count,
                                                  float value,
                                                  bool driven) {
    ConstraintComp c = constraint_comp_default();
    c.type = type;
    c.has_value = (type == CONSTRAINT_LENGTH || type == CONSTRAINT_ANGLE);
    c.driven = c.has_value ? driven : false;
    c.value = c.has_value ? value : 0.0f;

    if (participants && participant_count > 0) {
        if (participant_count > CONSTRAINT_MAX_PARTICIPANTS) {
            participant_count = CONSTRAINT_MAX_PARTICIPANTS;
        }
        memcpy(c.participants, participants, participant_count * sizeof(uint64_t));
        c.participant_count = participant_count;
    }
    return c;
}

#endif // CONSTRAINT_COMP_H
