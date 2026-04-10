//------------------------------------------------------------------------------
// sketch_script_capability_registry.h - Shared script/solver capability contract
//------------------------------------------------------------------------------
#ifndef SKETCH_SCRIPT_CAPABILITY_REGISTRY_H
#define SKETCH_SCRIPT_CAPABILITY_REGISTRY_H

#include "../constraints/constraint_types.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

static inline void sketch_script_capability_write_error(char *error_text,
                                                        size_t error_text_size,
                                                        const char *fmt,
                                                        ...) {
    if (!error_text || error_text_size == 0 || !fmt) return;
    va_list args;
    va_start(args, fmt);
    vsnprintf(error_text, error_text_size, fmt, args);
    va_end(args);
    error_text[error_text_size - 1] = '\0';
}

static inline const char* sketch_script_capability_role_name(constraint_participant_role_t role) {
    switch (role) {
        case CONSTRAINT_PARTICIPANT_ROLE_ENTITY: return "entity";
        case CONSTRAINT_PARTICIPANT_ROLE_POINT_A: return "point_a";
        case CONSTRAINT_PARTICIPANT_ROLE_POINT_B: return "point_b";
        case CONSTRAINT_PARTICIPANT_ROLE_CENTER: return "center";
        default: return "unspecified";
    }
}

static inline bool sketch_script_capability_parse_role(const char *text,
                                                       constraint_participant_role_t *out_role) {
    if (!text || !out_role) return false;
    if (strcmp(text, "entity") == 0) {
        *out_role = CONSTRAINT_PARTICIPANT_ROLE_ENTITY;
        return true;
    }
    if (strcmp(text, "point_a") == 0) {
        *out_role = CONSTRAINT_PARTICIPANT_ROLE_POINT_A;
        return true;
    }
    if (strcmp(text, "point_b") == 0) {
        *out_role = CONSTRAINT_PARTICIPANT_ROLE_POINT_B;
        return true;
    }
    if (strcmp(text, "center") == 0) {
        *out_role = CONSTRAINT_PARTICIPANT_ROLE_CENTER;
        return true;
    }
    return false;
}

static inline bool sketch_script_capability_entity_type_allowed(const char *type) {
    if (!type) return false;
    return strcmp(type, "point") == 0 ||
           strcmp(type, "line") == 0 ||
           strcmp(type, "arc") == 0 ||
           strcmp(type, "circle") == 0;
}

static inline bool sketch_script_capability_constraint_type_from_name(const char *name,
                                                                       constraint_type_t *out_type) {
    if (!name || !out_type) return false;
    for (int i = 0; i < CONSTRAINT_TYPE_COUNT; i++) {
        if (strcmp(name, constraint_type_display_name((constraint_type_t)i)) == 0) {
            *out_type = (constraint_type_t)i;
            return true;
        }
    }
    return false;
}

static inline bool sketch_script_capability_constraint_type_allowed(const char *type) {
    constraint_type_t parsed = CONSTRAINT_TYPE_COUNT;
    return sketch_script_capability_constraint_type_from_name(type, &parsed);
}

static inline bool sketch_script_capability_role_allowed_for_geometry(geometry_type_t geom,
                                                                      constraint_participant_role_t role) {
    if (role == CONSTRAINT_PARTICIPANT_ROLE_UNSPECIFIED) {
        role = CONSTRAINT_PARTICIPANT_ROLE_ENTITY;
    }
    if (geom == GEOM_POINT) {
        return role == CONSTRAINT_PARTICIPANT_ROLE_ENTITY;
    }
    if (geom == GEOM_LINE) {
        return role == CONSTRAINT_PARTICIPANT_ROLE_ENTITY ||
               role == CONSTRAINT_PARTICIPANT_ROLE_POINT_A ||
               role == CONSTRAINT_PARTICIPANT_ROLE_POINT_B;
    }
    if (geom == GEOM_ARC) {
        return role == CONSTRAINT_PARTICIPANT_ROLE_ENTITY ||
               role == CONSTRAINT_PARTICIPANT_ROLE_POINT_A ||
               role == CONSTRAINT_PARTICIPANT_ROLE_POINT_B ||
               role == CONSTRAINT_PARTICIPANT_ROLE_CENTER;
    }
    return false;
}

static inline bool sketch_script_capability_validate_signature(const constraint_selection_signature_t *sig,
                                                               constraint_type_t type,
                                                               char *error_text,
                                                               size_t error_text_size) {
    if (!sig || sig->count == 0 || sig->count > CONSTRAINT_MAX_PARTICIPANTS) {
        sketch_script_capability_write_error(error_text, error_text_size,
                                             "Constraint signature count is out of range.");
        return false;
    }
    if (sig->count < constraint_type_min_participants(type)) {
        sketch_script_capability_write_error(error_text, error_text_size,
                                             "Constraint '%s' requires at least %u participants.",
                                             constraint_type_display_name(type),
                                             (unsigned int)constraint_type_min_participants(type));
        return false;
    }

    for (uint32_t i = 0; i < sig->count; i++) {
        if (!constraint_type_allows_geometry(sig->geometry_types[i])) {
            sketch_script_capability_write_error(error_text, error_text_size,
                                                 "Unsupported participant geometry for constraint '%s'.",
                                                 constraint_type_display_name(type));
            return false;
        }
        if (!sketch_script_capability_role_allowed_for_geometry(sig->geometry_types[i], sig->roles[i])) {
            sketch_script_capability_write_error(error_text, error_text_size,
                                                 "Role '%s' is not supported for participant geometry in '%s'.",
                                                 sketch_script_capability_role_name(sig->roles[i]),
                                                 constraint_type_display_name(type));
            return false;
        }
    }

    if (!constraint_type_is_selection_legal((constraint_selection_signature_t*)sig, type)) {
        sketch_script_capability_write_error(error_text, error_text_size,
                                             "Constraint participants are not legal for type '%s' (capability registry).",
                                             constraint_type_display_name(type));
        return false;
    }
    return true;
}

#endif // SKETCH_SCRIPT_CAPABILITY_REGISTRY_H
