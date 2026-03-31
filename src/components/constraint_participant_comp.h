//------------------------------------------------------------------------------
// constraint_participant_comp.h - Geometry-side back-references to constraints
//------------------------------------------------------------------------------
#ifndef CONSTRAINT_PARTICIPANT_COMP_H
#define CONSTRAINT_PARTICIPANT_COMP_H

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define CONSTRAINT_PARTICIPANT_MAX_REFS 32

typedef struct {
    uint32_t constraint_count;
    uint64_t constraints[CONSTRAINT_PARTICIPANT_MAX_REFS];
} ConstraintParticipantComp;

static inline ConstraintParticipantComp constraint_participant_comp_default(void) {
    ConstraintParticipantComp c;
    memset(&c, 0, sizeof(c));
    return c;
}

static inline bool constraint_participant_contains(const ConstraintParticipantComp *c, uint64_t constraint_entity) {
    if (!c || constraint_entity == 0) return false;
    for (uint32_t i = 0; i < c->constraint_count; i++) {
        if (c->constraints[i] == constraint_entity) {
            return true;
        }
    }
    return false;
}

static inline bool constraint_participant_add(ConstraintParticipantComp *c, uint64_t constraint_entity) {
    if (!c || constraint_entity == 0) return false;
    if (constraint_participant_contains(c, constraint_entity)) return true;
    if (c->constraint_count >= CONSTRAINT_PARTICIPANT_MAX_REFS) return false;
    c->constraints[c->constraint_count++] = constraint_entity;
    return true;
}

static inline bool constraint_participant_remove(ConstraintParticipantComp *c, uint64_t constraint_entity) {
    if (!c || constraint_entity == 0) return false;
    for (uint32_t i = 0; i < c->constraint_count; i++) {
        if (c->constraints[i] == constraint_entity) {
            c->constraints[i] = c->constraints[c->constraint_count - 1];
            c->constraints[c->constraint_count - 1] = 0;
            c->constraint_count--;
            return true;
        }
    }
    return false;
}

#endif // CONSTRAINT_PARTICIPANT_COMP_H
