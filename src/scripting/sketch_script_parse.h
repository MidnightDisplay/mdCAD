//------------------------------------------------------------------------------
// sketch_script_parse.h - Declarative script parser to intermediate model
//------------------------------------------------------------------------------
#ifndef SKETCH_SCRIPT_PARSE_H
#define SKETCH_SCRIPT_PARSE_H

#include "sketch_script_contract.h"
#include "../components/constraint_comp.h"
#include "../components/script_identity_comp.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#define SKETCH_SCRIPT_MODEL_MAX_ENTITIES 512
#define SKETCH_SCRIPT_MODEL_MAX_CONSTRAINTS 512
#define SKETCH_SCRIPT_MODEL_MAX_INPUTS 32
#define SKETCH_SCRIPT_MODEL_MAX_OUTPUTS 32

typedef enum {
    SKETCH_SCRIPT_ENTITY_POINT = 0,
    SKETCH_SCRIPT_ENTITY_LINE = 1,
    SKETCH_SCRIPT_ENTITY_ARC = 2,
    SKETCH_SCRIPT_ENTITY_CIRCLE = 3
} sketch_script_entity_kind_t;

typedef struct {
    char id[SCRIPT_LOCAL_ID_MAX];
    sketch_script_entity_kind_t kind;
    vec3_t point;
    vec3_t a;
    vec3_t b;
    vec3_t center;
    float radius;
    float start_angle;
    float end_angle;
    vec3_t normal;
    vec4_t color;
} sketch_script_entity_model_t;

typedef struct {
    char id[SCRIPT_LOCAL_ID_MAX];
    constraint_participant_role_t role;
    uint8_t sub_index;
} sketch_script_participant_model_t;

typedef struct {
    char id[SCRIPT_LOCAL_ID_MAX];
    constraint_type_t type;
    bool has_value;
    float value;
    bool driven;
    bool has_pair;
    bool pair_is_owner;
    char pair_owner_id[SCRIPT_LOCAL_ID_MAX];
    char pair_paired_id[SCRIPT_LOCAL_ID_MAX];
    uint32_t participant_count;
    sketch_script_participant_model_t participants[CONSTRAINT_MAX_PARTICIPANTS];
} sketch_script_constraint_model_t;

typedef struct {
    char id[SCRIPT_LOCAL_ID_MAX];
    float value;
    bool has_min;
    float min_value;
    bool has_max;
    float max_value;
    bool has_step;
    float step_value;
} sketch_script_io_model_t;

typedef struct sketch_script_model_t {
    uint32_t entity_count;
    uint32_t constraint_count;
    uint32_t input_count;
    uint32_t output_count;
    sketch_script_entity_model_t entities[SKETCH_SCRIPT_MODEL_MAX_ENTITIES];
    sketch_script_constraint_model_t constraints[SKETCH_SCRIPT_MODEL_MAX_CONSTRAINTS];
    sketch_script_io_model_t inputs[SKETCH_SCRIPT_MODEL_MAX_INPUTS];
    sketch_script_io_model_t outputs[SKETCH_SCRIPT_MODEL_MAX_OUTPUTS];
} sketch_script_model_t;

static inline void sketch_script_parse_error(sketch_script_error_t *out_error,
                                             const char *message) {
    if (!out_error) return;
    out_error->line = 0;
    out_error->column = 0;
    snprintf(out_error->message, sizeof(out_error->message), "%s", message ? message : "Parse error.");
    out_error->message[sizeof(out_error->message) - 1] = '\0';
}

static inline bool sketch_script_find_matching_brace(const char *start, const char **out_end) {
    if (!start || *start != '{' || !out_end) return false;
    int depth = 0;
    bool in_string = false;
    for (const char *p = start; *p; p++) {
        if (*p == '"' && (p == start || p[-1] != '\\')) {
            in_string = !in_string;
            continue;
        }
        if (in_string) continue;
        if (*p == '{') depth++;
        if (*p == '}') {
            depth--;
            if (depth == 0) {
                *out_end = p;
                return true;
            }
        }
    }
    return false;
}

static inline bool sketch_script_parse_id_field(const char *block,
                                                char *out_id,
                                                size_t out_id_size) {
    if (!block || !out_id || out_id_size == 0) return false;
    const char *id_key = strstr(block, "id");
    if (!id_key) return false;
    const char *eq = strchr(id_key, '=');
    if (!eq) return false;
    const char *q1 = strchr(eq, '"');
    if (!q1) return false;
    const char *q2 = strchr(q1 + 1, '"');
    if (!q2) return false;
    size_t len = (size_t)(q2 - (q1 + 1));
    if (len == 0 || len >= out_id_size) return false;
    memcpy(out_id, q1 + 1, len);
    out_id[len] = '\0';
    return true;
}

static inline bool sketch_script_parse_string_named(const char *block,
                                                    const char *key,
                                                    char *out_text,
                                                    size_t out_text_size) {
    if (!block || !key || !out_text || out_text_size == 0) return false;
    const char *p = strstr(block, key);
    if (!p) return false;
    const char *eq = strchr(p, '=');
    if (!eq) return false;
    const char *q1 = strchr(eq, '"');
    if (!q1) return false;
    const char *q2 = strchr(q1 + 1, '"');
    if (!q2) return false;
    size_t len = (size_t)(q2 - (q1 + 1));
    if (len == 0 || len >= out_text_size) return false;
    memcpy(out_text, q1 + 1, len);
    out_text[len] = '\0';
    return true;
}

static inline bool sketch_script_parse_type_field(const char *block,
                                                  char *out_type,
                                                  size_t out_type_size) {
    return sketch_script_parse_string_named(block, "type", out_type, out_type_size);
}

static inline bool sketch_script_parse_uint8_named(const char *block,
                                                   const char *key,
                                                   uint8_t *out_value) {
    if (!block || !key || !out_value) return false;
    const char *p = strstr(block, key);
    if (!p) return false;
    p = strchr(p, '=');
    if (!p) return false;
    p++;
    while (*p == ' ' || *p == '\t') p++;
    char *end = NULL;
    unsigned long v = strtoul(p, &end, 10);
    if (end == p || v > 255UL) return false;
    *out_value = (uint8_t)v;
    return true;
}

static inline bool sketch_script_parse_vec4_named(const char *block,
                                                  const char *key,
                                                  vec4_t *out) {
    if (!block || !key || !out) return false;
    const char *p = strstr(block, key);
    if (!p) return false;
    const char *brace = strchr(p, '{');
    if (!brace) return false;
    float r = 0.0f, g = 0.0f, b = 0.0f, a = 0.0f;
    if (sscanf(brace, "{%f , %f , %f , %f}", &r, &g, &b, &a) != 4 &&
        sscanf(brace, "{%f,%f,%f,%f}", &r, &g, &b, &a) != 4) {
        return false;
    }
    *out = vec4_make(r, g, b, a);
    return true;
}

static inline bool sketch_script_parse_vec3_named(const char *block,
                                                  const char *key,
                                                  vec3_t *out) {
    if (!block || !key || !out) return false;
    const char *p = strstr(block, key);
    if (!p) return false;
    const char *brace = strchr(p, '{');
    if (!brace) return false;
    float x = 0.0f, y = 0.0f, z = 0.0f;
    if (sscanf(brace, "{%f , %f , %f}", &x, &y, &z) != 3 &&
        sscanf(brace, "{%f,%f,%f}", &x, &y, &z) != 3) {
        return false;
    }
    *out = vec3_make(x, y, z);
    return true;
}

static inline bool sketch_script_parse_float_named(const char *block,
                                                   const char *key,
                                                   float *out_value) {
    if (!block || !key || !out_value) return false;
    const char *p = strstr(block, key);
    if (!p) return false;
    p = strchr(p, '=');
    if (!p) return false;
    p++;
    while (*p == ' ' || *p == '\t') p++;
    char *end = NULL;
    float v = strtof(p, &end);
    if (end == p) return false;
    *out_value = v;
    return true;
}

static inline bool sketch_script_parse_bool_named(const char *block,
                                                  const char *key,
                                                  bool *out_value) {
    if (!block || !key || !out_value) return false;
    const char *p = strstr(block, key);
    if (!p) return false;
    p = strchr(p, '=');
    if (!p) return false;
    p++;
    while (*p == ' ' || *p == '\t') p++;
    if (strncmp(p, "true", 4) == 0) {
        *out_value = true;
        return true;
    }
    if (strncmp(p, "false", 5) == 0) {
        *out_value = false;
        return true;
    }
    return false;
}

static inline bool sketch_script_parse_pair_block(const char *block,
                                                  sketch_script_constraint_model_t *out_constraint,
                                                  sketch_script_error_t *out_error) {
    if (!block || !out_constraint) return false;
    const char *pair_key = NULL;
    const char *scan = block;
    while ((scan = strstr(scan, "pair")) != NULL) {
        const char *after = scan + 4;
        while (*after == ' ' || *after == '\t') after++;
        if (*after == '=') {
            pair_key = scan;
            break;
        }
        scan++;
    }
    if (!pair_key) {
        out_constraint->has_pair = false;
        return true;
    }
    const char *start = strchr(pair_key, '{');
    if (!start) {
        sketch_script_parse_error(out_error, "Constraint pair block missing '{'.");
        return false;
    }
    const char *end = NULL;
    if (!sketch_script_find_matching_brace(start, &end)) {
        sketch_script_parse_error(out_error, "Constraint pair block missing matching '}'.");
        return false;
    }
    size_t pair_len = (size_t)(end - start + 1);
    char pair_block[512] = {0};
    if (pair_len >= sizeof(pair_block)) {
        sketch_script_parse_error(out_error, "Constraint pair block too large.");
        return false;
    }
    memcpy(pair_block, start, pair_len);
    pair_block[pair_len] = '\0';

    if (!sketch_script_parse_bool_named(pair_block, "is_owner", &out_constraint->pair_is_owner)) {
        sketch_script_parse_error(out_error, "Constraint pair block requires is_owner.");
        return false;
    }
    if (!sketch_script_parse_string_named(pair_block, "owner", out_constraint->pair_owner_id,
                                          sizeof(out_constraint->pair_owner_id))) {
        sketch_script_parse_error(out_error, "Constraint pair block requires owner.");
        return false;
    }
    if (!sketch_script_parse_string_named(pair_block, "paired", out_constraint->pair_paired_id,
                                          sizeof(out_constraint->pair_paired_id))) {
        sketch_script_parse_error(out_error, "Constraint pair block requires paired.");
        return false;
    }
    out_constraint->has_pair = true;
    return true;
}

static inline constraint_type_t sketch_script_constraint_type_from_name(const char *name) {
    constraint_type_t type = CONSTRAINT_TYPE_COUNT;
    if (!sketch_script_capability_constraint_type_from_name(name, &type)) {
        return CONSTRAINT_TYPE_COUNT;
    }
    return type;
}

static inline bool sketch_script_parse_delimiter_segment_ok(const char *start, const char *end);

static inline bool sketch_script_parse_participants(const char *block,
                                                    sketch_script_constraint_model_t *out_constraint,
                                                    sketch_script_error_t *out_error) {
    if (!block || !out_constraint) {
        sketch_script_parse_error(out_error, "Constraint participants are missing.");
        return false;
    }
    const char *p = strstr(block, "participants");
    if (!p) {
        sketch_script_parse_error(out_error, "Constraint requires participants block.");
        return false;
    }
    const char *start = strchr(p, '{');
    if (!start) {
        sketch_script_parse_error(out_error, "Constraint participants block missing '{'.");
        return false;
    }
    const char *end = NULL;
    if (!sketch_script_find_matching_brace(start, &end)) {
        sketch_script_parse_error(out_error, "Constraint participants block missing matching '}'.");
        return false;
    }

    out_constraint->participant_count = 0;
    const char *cur = start + 1;
    const char *cursor = cur;
    while (cur < end) {
        while (cur < end && *cur != '{' && *cur != '"') cur++;
        if (cur >= end) break;
        if (!sketch_script_parse_delimiter_segment_ok(cursor, cur)) {
            sketch_script_parse_error(out_error, "Unexpected token in participants block.");
            return false;
        }
        if (*cur == '"') {
            sketch_script_parse_error(
                out_error,
                "Constraint participants must be descriptor objects: { id = \"...\", role = \"...\", sub_index = N }.");
            return false;
        }
        const char *entry_end = NULL;
        if (!sketch_script_find_matching_brace(cur, &entry_end) || entry_end > end) {
            sketch_script_parse_error(out_error, "Participant descriptor missing matching '}'.");
            return false;
        }
        if (out_constraint->participant_count >= CONSTRAINT_MAX_PARTICIPANTS) {
            sketch_script_parse_error(out_error, "Too many participant descriptors.");
            return false;
        }

        size_t entry_len = (size_t)(entry_end - cur + 1);
        char entry[512] = {0};
        if (entry_len >= sizeof(entry)) {
            sketch_script_parse_error(out_error, "Participant descriptor too large.");
            return false;
        }
        memcpy(entry, cur, entry_len);
        entry[entry_len] = '\0';

        sketch_script_participant_model_t *dst =
            &out_constraint->participants[out_constraint->participant_count];
        memset(dst, 0, sizeof(*dst));
        if (!sketch_script_parse_id_field(entry, dst->id, sizeof(dst->id))) {
            sketch_script_parse_error(out_error, "Participant descriptor requires non-empty id.");
            return false;
        }
        char role_text[32] = {0};
        if (!sketch_script_parse_string_named(entry, "role", role_text, sizeof(role_text))) {
            sketch_script_parse_error(out_error, "Participant descriptor requires role.");
            return false;
        }
        if (!sketch_script_capability_parse_role(role_text, &dst->role)) {
            sketch_script_parse_error(out_error, "Participant descriptor role is not supported.");
            return false;
        }
        if (!sketch_script_parse_uint8_named(entry, "sub_index", &dst->sub_index)) {
            sketch_script_parse_error(out_error, "Participant descriptor requires numeric sub_index.");
            return false;
        }

        out_constraint->participant_count++;
        cur = entry_end + 1;
        cursor = cur;
    }
    if (!sketch_script_parse_delimiter_segment_ok(cursor, end)) {
        sketch_script_parse_error(out_error, "Unexpected token in participants block.");
        return false;
    }
    return out_constraint->participant_count > 0;
}

static inline bool sketch_script_parse_delimiter_segment_ok(const char *start, const char *end) {
    if (!start || !end || end < start) return false;
    for (const char *p = start; p < end; p++) {
        if (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n' || *p == ',') continue;
        return false;
    }
    return true;
}

static inline bool sketch_script_parse_number_token(const char *p, float *out_value) {
    if (!p || !out_value) return false;
    while (*p == ' ' || *p == '\t') p++;
    if (strncmp(p, "true", 4) == 0 || strncmp(p, "false", 5) == 0 || *p == '"') {
        return false;
    }
    char *end = NULL;
    float v = strtof(p, &end);
    if (end == p) return false;
    *out_value = v;
    return true;
}

static inline bool sketch_script_parse_numeric_named(const char *block,
                                                     const char *key,
                                                     float *out_value) {
    if (!block || !key || !out_value) return false;
    const char *p = strstr(block, key);
    if (!p) return false;
    p = strchr(p, '=');
    if (!p) return false;
    p++;
    return sketch_script_parse_number_token(p, out_value);
}

static inline bool sketch_script_parse_io_block(const char *script_text,
                                                const char *block_name,
                                                sketch_script_io_model_t *entries,
                                                uint32_t max_entries,
                                                uint32_t *out_count,
                                                sketch_script_error_t *out_error) {
    if (!script_text || !block_name || !entries || !out_count) return false;
    *out_count = 0;
    const char *io = strstr(script_text, block_name);
    if (!io) return true;
    const char *list_start = strchr(io, '{');
    if (!list_start) {
        sketch_script_parse_error(out_error, "io block missing '{'.");
        return false;
    }
    const char *list_end = NULL;
    if (!sketch_script_find_matching_brace(list_start, &list_end)) {
        sketch_script_parse_error(out_error, "io block missing matching '}'.");
        return false;
    }

    const char *p = list_start + 1;
    const char *cursor = p;
    while (p < list_end) {
        while (p < list_end && *p != '{') p++;
        if (p >= list_end) break;
        if (!sketch_script_parse_delimiter_segment_ok(cursor, p)) {
            sketch_script_parse_error(out_error, "Unexpected token in io block.");
            return false;
        }
        const char *entry_end = NULL;
        if (!sketch_script_find_matching_brace(p, &entry_end) || entry_end > list_end) {
            sketch_script_parse_error(out_error, "io entry missing matching '}'.");
            return false;
        }
        if (*out_count >= max_entries) {
            sketch_script_parse_error(out_error, "too many io entries in script model.");
            return false;
        }

        size_t entry_len = (size_t)(entry_end - p + 1);
        char entry[1024];
        if (entry_len >= sizeof(entry)) {
            sketch_script_parse_error(out_error, "io declaration too large.");
            return false;
        }
        memcpy(entry, p, entry_len);
        entry[entry_len] = '\0';

        sketch_script_io_model_t *dst = &entries[*out_count];
        memset(dst, 0, sizeof(*dst));
        if (!sketch_script_parse_id_field(entry, dst->id, sizeof(dst->id))) {
            sketch_script_parse_error(out_error, "io declaration requires id.");
            return false;
        }
        if (!sketch_script_parse_numeric_named(entry, "value", &dst->value)) {
            sketch_script_parse_error(out_error, "io value must be numeric.");
            return false;
        }
        dst->has_min = sketch_script_parse_numeric_named(entry, "min", &dst->min_value);
        dst->has_max = sketch_script_parse_numeric_named(entry, "max", &dst->max_value);
        dst->has_step = sketch_script_parse_numeric_named(entry, "step", &dst->step_value);
        (*out_count)++;
        p = entry_end + 1;
        cursor = p;
    }
    if (!sketch_script_parse_delimiter_segment_ok(cursor, list_end)) {
        sketch_script_parse_error(out_error, "Unexpected token in io block.");
        return false;
    }
    return true;
}

static inline bool sketch_script_parse_entities_block(const char *script_text,
                                                      sketch_script_model_t *out_model,
                                                      sketch_script_error_t *out_error) {
    const char *entities = strstr(script_text, "entities");
    if (!entities) return true;
    const char *list_start = strchr(entities, '{');
    if (!list_start) {
        sketch_script_parse_error(out_error, "entities block missing '{'.");
        return false;
    }
    const char *list_end = NULL;
    if (!sketch_script_find_matching_brace(list_start, &list_end)) {
        sketch_script_parse_error(out_error, "entities block missing matching '}'.");
        return false;
    }

    const char *p = list_start + 1;
    const char *cursor = p;
    while (p < list_end) {
        while (p < list_end && *p != '{') p++;
        if (p >= list_end) break;
        if (!sketch_script_parse_delimiter_segment_ok(cursor, p)) {
            sketch_script_parse_error(out_error, "Unexpected token in entities block.");
            return false;
        }
        const char *entry_end = NULL;
        if (!sketch_script_find_matching_brace(p, &entry_end) || entry_end > list_end) {
            sketch_script_parse_error(out_error, "entity block missing matching '}'.");
            return false;
        }
        if (out_model->entity_count >= SKETCH_SCRIPT_MODEL_MAX_ENTITIES) {
            sketch_script_parse_error(out_error, "too many entities in script model.");
            return false;
        }

        size_t entry_len = (size_t)(entry_end - p + 1);
        char entry[2048];
        if (entry_len >= sizeof(entry)) {
            sketch_script_parse_error(out_error, "entity declaration too large.");
            return false;
        }
        memcpy(entry, p, entry_len);
        entry[entry_len] = '\0';

        sketch_script_entity_model_t *dst = &out_model->entities[out_model->entity_count];
        memset(dst, 0, sizeof(*dst));

        char type[32];
        if (!sketch_script_parse_id_field(entry, dst->id, sizeof(dst->id)) ||
            !sketch_script_parse_type_field(entry, type, sizeof(type))) {
            sketch_script_parse_error(out_error, "entity must include id and type.");
            return false;
        }

        sketch_script_entity_decl_t decl = {.type = type, .has_id = true, .id = dst->id};
        char contract_error[192] = {0};
        if (!sketch_script_contract_validate_entity_decl(&decl, contract_error, sizeof(contract_error))) {
            sketch_script_parse_error(out_error, contract_error);
            return false;
        }

        if (strcmp(type, "point") == 0) {
            dst->kind = SKETCH_SCRIPT_ENTITY_POINT;
            if (!sketch_script_parse_vec3_named(entry, "point", &dst->point)) {
                sketch_script_parse_error(out_error, "point entity requires point = {x,y,z}.");
                return false;
            }
        } else if (strcmp(type, "line") == 0) {
            dst->kind = SKETCH_SCRIPT_ENTITY_LINE;
            if (!sketch_script_parse_vec3_named(entry, "a", &dst->a) ||
                !sketch_script_parse_vec3_named(entry, "b", &dst->b)) {
                sketch_script_parse_error(out_error, "line entity requires a and b vectors.");
                return false;
            }
        } else {
            if (strcmp(type, "circle") == 0) {
                dst->kind = SKETCH_SCRIPT_ENTITY_CIRCLE;
                dst->start_angle = 0.0f;
                dst->end_angle = 6.28318530718f;
            } else {
                dst->kind = SKETCH_SCRIPT_ENTITY_ARC;
            }
            if (!sketch_script_parse_vec3_named(entry, "center", &dst->center) ||
                !sketch_script_parse_float_named(entry, "radius", &dst->radius)) {
                sketch_script_parse_error(out_error, "arc/circle requires center and radius.");
                return false;
            }
            if (!sketch_script_parse_vec3_named(entry, "normal", &dst->normal)) {
                dst->normal = vec3_make(0.0f, 0.0f, 1.0f);
            }
            if (dst->kind == SKETCH_SCRIPT_ENTITY_ARC) {
                if (!sketch_script_parse_float_named(entry, "start_angle", &dst->start_angle) ||
                    !sketch_script_parse_float_named(entry, "end_angle", &dst->end_angle)) {
                    sketch_script_parse_error(out_error, "arc requires start_angle and end_angle.");
                    return false;
                }
            }
        }
        dst->color = vec4_make(1.0f, 1.0f, 1.0f, 1.0f);
        if (strstr(entry, "color")) {
            if (!sketch_script_parse_vec4_named(entry, "color", &dst->color)) {
                sketch_script_parse_error(out_error, "entity color must be color = {r,g,b,a}.");
                return false;
            }
        }

        out_model->entity_count++;
        p = entry_end + 1;
        cursor = p;
    }
    if (!sketch_script_parse_delimiter_segment_ok(cursor, list_end)) {
        sketch_script_parse_error(out_error, "Unexpected token in entities block.");
        return false;
    }
    return true;
}

static inline bool sketch_script_parse_constraints_block(const char *script_text,
                                                         sketch_script_model_t *out_model,
                                                         sketch_script_error_t *out_error) {
    const char *constraints = strstr(script_text, "constraints");
    if (!constraints) return true;
    const char *list_start = strchr(constraints, '{');
    if (!list_start) {
        sketch_script_parse_error(out_error, "constraints block missing '{'.");
        return false;
    }
    const char *list_end = NULL;
    if (!sketch_script_find_matching_brace(list_start, &list_end)) {
        sketch_script_parse_error(out_error, "constraints block missing matching '}'.");
        return false;
    }

    const char *p = list_start + 1;
    const char *cursor = p;
    while (p < list_end) {
        while (p < list_end && *p != '{') p++;
        if (p >= list_end) break;
        if (!sketch_script_parse_delimiter_segment_ok(cursor, p)) {
            sketch_script_parse_error(out_error, "Unexpected token in constraints block.");
            return false;
        }
        const char *entry_end = NULL;
        if (!sketch_script_find_matching_brace(p, &entry_end) || entry_end > list_end) {
            sketch_script_parse_error(out_error, "constraint block missing matching '}'.");
            return false;
        }
        if (out_model->constraint_count >= SKETCH_SCRIPT_MODEL_MAX_CONSTRAINTS) {
            sketch_script_parse_error(out_error, "too many constraints in script model.");
            return false;
        }

        size_t entry_len = (size_t)(entry_end - p + 1);
        char entry[2048];
        if (entry_len >= sizeof(entry)) {
            sketch_script_parse_error(out_error, "constraint declaration too large.");
            return false;
        }
        memcpy(entry, p, entry_len);
        entry[entry_len] = '\0';

        sketch_script_constraint_model_t *dst = &out_model->constraints[out_model->constraint_count];
        memset(dst, 0, sizeof(*dst));

        char type[64];
        if (!sketch_script_parse_id_field(entry, dst->id, sizeof(dst->id)) ||
            !sketch_script_parse_type_field(entry, type, sizeof(type))) {
            sketch_script_parse_error(out_error, "constraint requires id and type.");
            return false;
        }
        if (!sketch_script_parse_participants(entry, dst, out_error)) {
            return false;
        }
        if (!sketch_script_parse_pair_block(entry, dst, out_error)) {
            return false;
        }

        sketch_script_constraint_decl_t decl = {
            .type = type,
            .has_id = true,
            .id = dst->id,
            .participant_count = dst->participant_count
        };
        char contract_error[192] = {0};
        if (!sketch_script_contract_validate_constraint_decl(&decl, contract_error, sizeof(contract_error))) {
            sketch_script_parse_error(out_error, contract_error);
            return false;
        }

        dst->type = sketch_script_constraint_type_from_name(type);
        if (dst->type == CONSTRAINT_TYPE_COUNT) {
            sketch_script_parse_error(out_error, "Unsupported constraint type.");
            return false;
        }
        dst->has_value = sketch_script_parse_float_named(entry, "value", &dst->value);
        dst->driven = false;
        sketch_script_parse_bool_named(entry, "driven", &dst->driven);
        out_model->constraint_count++;
        p = entry_end + 1;
        cursor = p;
    }
    if (!sketch_script_parse_delimiter_segment_ok(cursor, list_end)) {
        sketch_script_parse_error(out_error, "Unexpected token in constraints block.");
        return false;
    }
    return true;
}

static inline bool sketch_script_parse_model(const char *script_text,
                                             sketch_script_model_t *out_model,
                                             sketch_script_error_t *out_error) {
    if (!script_text || !out_model) {
        sketch_script_parse_error(out_error, "script text/model cannot be null.");
        return false;
    }
    memset(out_model, 0, sizeof(*out_model));
    if (!sketch_script_parse_io_block(script_text,
                                      "inputs",
                                      out_model->inputs,
                                      SKETCH_SCRIPT_MODEL_MAX_INPUTS,
                                      &out_model->input_count,
                                      out_error)) return false;
    if (!sketch_script_parse_io_block(script_text,
                                      "outputs",
                                      out_model->outputs,
                                      SKETCH_SCRIPT_MODEL_MAX_OUTPUTS,
                                      &out_model->output_count,
                                      out_error)) return false;
    if (!sketch_script_parse_entities_block(script_text, out_model, out_error)) return false;
    if (!sketch_script_parse_constraints_block(script_text, out_model, out_error)) return false;
    return true;
}

#endif // SKETCH_SCRIPT_PARSE_H
