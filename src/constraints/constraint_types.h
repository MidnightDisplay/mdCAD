//------------------------------------------------------------------------------
// constraint_types.h - v1.2 constraint type helpers and legality checks
//------------------------------------------------------------------------------
#ifndef CONSTRAINT_TYPES_H
#define CONSTRAINT_TYPES_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
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

static inline uint8_t constraint_clamp_decimals(int decimals) {
    if (decimals < 0) return 0;
    if (decimals > 6) return 6;
    return (uint8_t)decimals;
}

static inline float constraint_round_to_decimals(float value, uint8_t decimals) {
    static const float scales[] = {1.0f, 10.0f, 100.0f, 1000.0f, 10000.0f, 100000.0f, 1000000.0f};
    uint8_t d = constraint_clamp_decimals((int)decimals);
    float scale = scales[d];
    return roundf(value * scale) / scale;
}

static inline uint8_t constraint_value_infer_decimals(float value, uint8_t fallback) {
    uint8_t fb = constraint_clamp_decimals((int)fallback);
    // Infer the smallest decimal count that reproduces the value within a tight float tolerance.
    for (uint8_t d = 0; d <= 6; d++) {
        float rounded = constraint_round_to_decimals(value, d);
        float tolerance = 0.5e-6f * fmaxf(1.0f, fabsf(value));
        if (fabsf(value - rounded) <= tolerance) {
            return d;
        }
    }
    return fb;
}

static inline void constraint_build_float_format(char *out_fmt, size_t out_fmt_size, uint8_t decimals) {
    if (!out_fmt || out_fmt_size == 0) return;
    uint8_t clamped = constraint_clamp_decimals((int)decimals);
    snprintf(out_fmt, out_fmt_size, "%%.%uf", (unsigned int)clamped);
}

static inline void constraint_format_value(char *out_text, size_t out_text_size, float value, uint8_t decimals) {
    if (!out_text || out_text_size == 0) return;
    char fmt[16];
    constraint_build_float_format(fmt, sizeof(fmt), decimals);
    snprintf(out_text, out_text_size, fmt, value);
}

static inline uint32_t constraint_type_min_participants(constraint_type_t type) {
    switch (type) {
        case CONSTRAINT_FIXED:
        case CONSTRAINT_ALONG_X:
        case CONSTRAINT_ALONG_Y:
        case CONSTRAINT_ALONG_Z:
        case CONSTRAINT_LENGTH:
            return 1;
        case CONSTRAINT_COINCIDENT:
        case CONSTRAINT_COLLINEAR:
        case CONSTRAINT_PARALLEL:
        case CONSTRAINT_PERPENDICULAR:
        case CONSTRAINT_CORADIAL:
        case CONSTRAINT_CONCENTRIC:
        case CONSTRAINT_ANGLE:
        case CONSTRAINT_TANGENTIAL:
            return 2;
        default:
            return 1;
    }
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
        if (sig->count != 2) return false;
        for (uint32_t i = 0; i < sig->count; i++) {
            bool endpoint_role =
                sig->roles[i] == CONSTRAINT_PARTICIPANT_ROLE_POINT_A ||
                sig->roles[i] == CONSTRAINT_PARTICIPANT_ROLE_POINT_B;
            if (endpoint_role &&
                !(sig->geometry_types[i] == GEOM_LINE || sig->geometry_types[i] == GEOM_ARC)) {
                return false;
            }
        }
        return true;
    }
    if (type == CONSTRAINT_COLLINEAR || type == CONSTRAINT_PARALLEL) {
        if (sig->count < 2) return false;
        for (uint32_t i = 0; i < sig->count; i++) {
            if (sig->geometry_types[i] != GEOM_LINE) return false;
        }
        return true;
    }
    if (type == CONSTRAINT_PERPENDICULAR || type == CONSTRAINT_ANGLE) {
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
