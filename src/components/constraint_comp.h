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
    uint64_t entity;
    uint8_t role;
    uint8_t sub_index;
    uint16_t reserved;
} constraint_participant_descriptor_t;

static inline constraint_participant_descriptor_t constraint_participant_descriptor_make(uint64_t entity,
                                                                                        uint8_t role,
                                                                                        uint8_t sub_index) {
    constraint_participant_descriptor_t descriptor = {0};
    descriptor.entity = entity;
    descriptor.role = role;
    descriptor.sub_index = sub_index;
    return descriptor;
}

typedef struct {
    constraint_type_t type;
    bool has_value;                             // true for dimensional constraints
    bool driven;                                // CONS-05 for LENGTH/ANGLE
    float value;                                // LENGTH/ANGLE value
    uint8_t display_decimals;                   // UI display precision for dimensional values
    uint32_t participant_count;
    uint64_t participants[CONSTRAINT_MAX_PARTICIPANTS];
    constraint_participant_descriptor_t participant_descriptors[CONSTRAINT_MAX_PARTICIPANTS];
} ConstraintComp;

static inline ConstraintComp constraint_comp_default(void) {
    ConstraintComp c;
    memset(&c, 0, sizeof(c));
    c.type = CONSTRAINT_COINCIDENT;
    c.display_decimals = 4;
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
    c.display_decimals = 4;

    if (participants && participant_count > 0) {
        if (participant_count > CONSTRAINT_MAX_PARTICIPANTS) {
            participant_count = CONSTRAINT_MAX_PARTICIPANTS;
        }
        memcpy(c.participants, participants, participant_count * sizeof(uint64_t));
        c.participant_count = participant_count;
        for (uint32_t i = 0; i < participant_count; i++) {
            c.participant_descriptors[i] = constraint_participant_descriptor_make(c.participants[i], 1, 0);
        }
    }
    return c;
}

#endif // CONSTRAINT_COMP_H
