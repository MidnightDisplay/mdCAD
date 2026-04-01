//------------------------------------------------------------------------------
// constraint_selection.h - Shared constraint participant selection helpers
//------------------------------------------------------------------------------
#ifndef CONSTRAINT_SELECTION_H
#define CONSTRAINT_SELECTION_H

#include <stdint.h>
#include "../selection.h"
#include "../ecs/ecs_world.h"
#include "../components/constraint_comp.h"

// Clears selection and adds all alive participants for the given constraint.
// Returns the number of participants successfully added to the selection.
static inline uint32_t constraint_selection_apply_participants(selection_buffer_t *selection,
                                                               ecs_world_state_t *world,
                                                               ecs_entity_t constraint_entity) {
    if (!selection) return 0;

    selection_clear(selection);

    if (!world || !world->world || constraint_entity == 0) return 0;
    if (!ecs_is_alive(world->world, constraint_entity)) return 0;

    ConstraintComp *constraint = ecs_world_get_constraint(world, constraint_entity);
    if (!constraint) return 0;

    uint32_t added = 0;
    uint32_t participant_count = constraint->participant_count;
    if (participant_count > CONSTRAINT_MAX_PARTICIPANTS) {
        participant_count = CONSTRAINT_MAX_PARTICIPANTS;
    }

    for (uint32_t i = 0; i < participant_count; i++) {
        ecs_entity_t participant = (ecs_entity_t)constraint->participants[i];
        if (!ecs_is_alive(world->world, participant)) continue;

        int before = selection_count(selection);
        selection_add(selection, participant);
        if (selection_count(selection) > before) {
            added++;
        }
    }

    return added;
}

#endif // CONSTRAINT_SELECTION_H
