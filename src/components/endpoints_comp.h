//------------------------------------------------------------------------------
// endpoints_comp.h - Sketch-scoped endpoint ownership/sync metadata
//------------------------------------------------------------------------------
#ifndef ENDPOINTS_COMP_H
#define ENDPOINTS_COMP_H

#include <stdint.h>
#include <stdbool.h>
#include "../constraints/constraint_types.h"

#define ENDPOINTS_COMP_MAX_ENDPOINTS 3u

typedef struct {
    uint64_t endpoint_entity;                    // Native endpoint point entity
    constraint_participant_role_t role;          // POINT_A / POINT_B on owner
    uint8_t sub_index;                           // Stable notable-vertex index
} endpoint_binding_t;

typedef struct {
    // Owner contract: attached to sketch-created line/arc geometry entities only.
    // Point contract: attached to spawned endpoint point entities.
    bool is_endpoint_point;

    // For point contract entries, this is the owning line/arc geometry entity.
    uint64_t owner_entity;
    constraint_participant_role_t role;
    uint8_t sub_index;

    // For owner contract entries, these map notable vertices -> child point entities.
    uint8_t endpoint_count;
    endpoint_binding_t endpoints[ENDPOINTS_COMP_MAX_ENDPOINTS];
} EndPointsComp;

static inline EndPointsComp endpoints_comp_owner_default(void) {
    EndPointsComp comp = {0};
    comp.is_endpoint_point = false;
    comp.endpoint_count = 0;
    return comp;
}

static inline EndPointsComp endpoints_comp_point(uint64_t owner_entity,
                                                 constraint_participant_role_t role,
                                                 uint8_t sub_index) {
    EndPointsComp comp = {0};
    comp.is_endpoint_point = true;
    comp.owner_entity = owner_entity;
    comp.role = role;
    comp.sub_index = sub_index;
    return comp;
}

static inline bool endpoints_comp_is_supported_role(constraint_participant_role_t role) {
    return role == CONSTRAINT_PARTICIPANT_ROLE_POINT_A ||
           role == CONSTRAINT_PARTICIPANT_ROLE_POINT_B ||
           role == CONSTRAINT_PARTICIPANT_ROLE_CENTER;
}

static inline bool endpoints_comp_set_binding(EndPointsComp *comp,
                                              constraint_participant_role_t role,
                                              uint8_t sub_index,
                                              uint64_t endpoint_entity) {
    if (!comp || !endpoints_comp_is_supported_role(role)) return false;

    for (uint8_t i = 0; i < comp->endpoint_count; i++) {
        if (comp->endpoints[i].role == role) {
            comp->endpoints[i].sub_index = sub_index;
            comp->endpoints[i].endpoint_entity = endpoint_entity;
            return true;
        }
    }

    if (comp->endpoint_count >= ENDPOINTS_COMP_MAX_ENDPOINTS) return false;
    endpoint_binding_t *entry = &comp->endpoints[comp->endpoint_count++];
    entry->role = role;
    entry->sub_index = sub_index;
    entry->endpoint_entity = endpoint_entity;
    return true;
}

static inline bool endpoints_comp_find_binding(const EndPointsComp *comp,
                                               constraint_participant_role_t role,
                                               endpoint_binding_t *out_binding) {
    if (!comp || !endpoints_comp_is_supported_role(role)) return false;
    for (uint8_t i = 0; i < comp->endpoint_count; i++) {
        if (comp->endpoints[i].role == role && comp->endpoints[i].endpoint_entity != 0) {
            if (out_binding) *out_binding = comp->endpoints[i];
            return true;
        }
    }
    return false;
}

#endif // ENDPOINTS_COMP_H
