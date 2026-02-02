//------------------------------------------------------------------------------
// scene_serializer.h - JSON scene serialization (header-only)
//
// Provides save/load functionality for ECS scenes.
// JSON format version 1.
//------------------------------------------------------------------------------
#ifndef SCENE_SERIALIZER_H
#define SCENE_SERIALIZER_H

#include "ecs/ecs_world.h"
#include "ecs/ecs_scene.h"
#include "components/geometry_comp.h"
#include "components/transform_comp.h"
#include "components/renderable_comp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

//------------------------------------------------------------------------------
// JSON Format Version
//------------------------------------------------------------------------------
#define SCENE_JSON_VERSION 1

//------------------------------------------------------------------------------
// String Builder for JSON Output
//------------------------------------------------------------------------------

typedef struct {
    char *data;
    size_t length;
    size_t capacity;
} json_builder_t;

static inline void json_builder_init(json_builder_t *b) {
    b->capacity = 4096;
    b->data = (char*)malloc(b->capacity);
    b->data[0] = '\0';
    b->length = 0;
}

static inline void json_builder_free(json_builder_t *b) {
    if (b->data) {
        free(b->data);
        b->data = NULL;
    }
    b->length = 0;
    b->capacity = 0;
}

static inline void json_builder_ensure(json_builder_t *b, size_t additional) {
    size_t needed = b->length + additional + 1;
    if (needed > b->capacity) {
        while (b->capacity < needed) {
            b->capacity *= 2;
        }
        b->data = (char*)realloc(b->data, b->capacity);
    }
}

static inline void json_builder_append(json_builder_t *b, const char *str) {
    size_t len = strlen(str);
    json_builder_ensure(b, len);
    memcpy(b->data + b->length, str, len + 1);
    b->length += len;
}

static inline void json_builder_appendf(json_builder_t *b, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    // First pass: calculate length
    va_list args_copy;
    va_copy(args_copy, args);
    int len = vsnprintf(NULL, 0, fmt, args_copy);
    va_end(args_copy);

    // Ensure capacity
    json_builder_ensure(b, len);

    // Second pass: write
    vsnprintf(b->data + b->length, len + 1, fmt, args);
    b->length += len;

    va_end(args);
}

//------------------------------------------------------------------------------
// JSON Writing Helpers
//------------------------------------------------------------------------------

static inline void json_write_vec3(json_builder_t *b, vec3_t v) {
    json_builder_appendf(b, "[%.6g, %.6g, %.6g]", v.x, v.y, v.z);
}

static inline void json_write_vec4(json_builder_t *b, vec4_t v) {
    json_builder_appendf(b, "[%.6g, %.6g, %.6g, %.6g]", v.x, v.y, v.z, v.w);
}

static inline void json_write_indent(json_builder_t *b, int depth) {
    for (int i = 0; i < depth; i++) {
        json_builder_append(b, "  ");
    }
}

//------------------------------------------------------------------------------
// Geometry Type Names
//------------------------------------------------------------------------------

static inline const char* scene_geom_type_to_string(geometry_type_t type) {
    static const char* names[] = {
        "point", "line", "polyline", "arc", "polygon", "helix", "bezier"
    };
    if (type >= 0 && type < GEOM_TYPE_COUNT) {
        return names[type];
    }
    return "unknown";
}

static inline geometry_type_t scene_string_to_geom_type(const char *str) {
    if (strcmp(str, "point") == 0) return GEOM_POINT;
    if (strcmp(str, "line") == 0) return GEOM_LINE;
    if (strcmp(str, "polyline") == 0) return GEOM_POLYLINE;
    if (strcmp(str, "arc") == 0) return GEOM_ARC;
    if (strcmp(str, "polygon") == 0) return GEOM_POLYGON;
    if (strcmp(str, "helix") == 0) return GEOM_HELIX;
    if (strcmp(str, "bezier") == 0) return GEOM_BEZIER;
    return GEOM_POINT;  // Default
}

//------------------------------------------------------------------------------
// Scene Save - Export to JSON
//------------------------------------------------------------------------------

// Write a single entity to JSON
static inline void scene_write_entity_json(json_builder_t *b, ecs_scene_t *scene,
                                            ecs_entity_t e, int depth) {
    ecs_world_state_t *w = scene->world;

    TransformComp *t = ecs_world_get_transform(w, e);
    GeometryComp *g = ecs_world_get_geometry(w, e);
    RenderableComp *r = ecs_world_get_renderable(w, e);

    if (!t || !g) return;  // Skip entities without required components

    ecs_entity_t parent = ecs_world_get_parent(w, e);

    json_write_indent(b, depth);
    json_builder_append(b, "{\n");

    // Entity ID
    json_write_indent(b, depth + 1);
    json_builder_appendf(b, "\"id\": %llu,\n", (unsigned long long)e);

    // Parent (null if no parent)
    json_write_indent(b, depth + 1);
    if (parent != 0) {
        json_builder_appendf(b, "\"parent\": %llu,\n", (unsigned long long)parent);
    } else {
        json_builder_append(b, "\"parent\": null,\n");
    }

    // Components
    json_write_indent(b, depth + 1);
    json_builder_append(b, "\"components\": {\n");

    // Transform component
    json_write_indent(b, depth + 2);
    json_builder_append(b, "\"transform\": {\n");
    json_write_indent(b, depth + 3);
    json_builder_append(b, "\"position\": ");
    json_write_vec3(b, t->position);
    json_builder_append(b, ",\n");
    json_write_indent(b, depth + 3);
    json_builder_append(b, "\"rotation\": ");
    json_write_vec3(b, t->rotation);
    json_builder_append(b, ",\n");
    json_write_indent(b, depth + 3);
    json_builder_append(b, "\"scale\": ");
    json_write_vec3(b, t->scale);
    json_builder_append(b, "\n");
    json_write_indent(b, depth + 2);
    json_builder_append(b, "},\n");

    // Geometry component
    json_write_indent(b, depth + 2);
    json_builder_append(b, "\"geometry\": {\n");
    json_write_indent(b, depth + 3);
    json_builder_appendf(b, "\"type\": \"%s\",\n", scene_geom_type_to_string(g->type));
    json_write_indent(b, depth + 3);
    json_builder_append(b, "\"color\": ");
    json_write_vec4(b, g->color);
    json_builder_append(b, ",\n");
    json_write_indent(b, depth + 3);
    json_builder_appendf(b, "\"line_width\": %.6g,\n", g->line_width);
    json_write_indent(b, depth + 3);
    json_builder_appendf(b, "\"point_size\": %.6g,\n", g->point_size);

    // Type-specific data
    switch (g->type) {
        case GEOM_POINT:
            json_write_indent(b, depth + 3);
            json_builder_append(b, "\"point\": ");
            json_write_vec3(b, g->data.point.point);
            json_builder_append(b, "\n");
            break;

        case GEOM_LINE:
            json_write_indent(b, depth + 3);
            json_builder_append(b, "\"a\": ");
            json_write_vec3(b, g->data.line.a);
            json_builder_append(b, ",\n");
            json_write_indent(b, depth + 3);
            json_builder_append(b, "\"b\": ");
            json_write_vec3(b, g->data.line.b);
            json_builder_append(b, "\n");
            break;

        case GEOM_POLYLINE:
            json_write_indent(b, depth + 3);
            json_builder_append(b, "\"points\": [\n");
            for (int i = 0; i < g->data.polyline.count; i++) {
                json_write_indent(b, depth + 4);
                json_write_vec3(b, g->data.polyline.points[i]);
                if (i < g->data.polyline.count - 1) {
                    json_builder_append(b, ",");
                }
                json_builder_append(b, "\n");
            }
            json_write_indent(b, depth + 3);
            json_builder_append(b, "]\n");
            break;

        case GEOM_POLYGON:
            json_write_indent(b, depth + 3);
            json_builder_append(b, "\"points\": [\n");
            for (int i = 0; i < g->data.polygon.count; i++) {
                json_write_indent(b, depth + 4);
                json_write_vec3(b, g->data.polygon.points[i]);
                if (i < g->data.polygon.count - 1) {
                    json_builder_append(b, ",");
                }
                json_builder_append(b, "\n");
            }
            json_write_indent(b, depth + 3);
            json_builder_append(b, "]\n");
            break;

        case GEOM_ARC:
            json_write_indent(b, depth + 3);
            json_builder_append(b, "\"center\": ");
            json_write_vec3(b, g->data.arc.center);
            json_builder_append(b, ",\n");
            json_write_indent(b, depth + 3);
            json_builder_appendf(b, "\"radius\": %.6g,\n", g->data.arc.radius);
            json_write_indent(b, depth + 3);
            json_builder_appendf(b, "\"start_angle\": %.6g,\n", g->data.arc.start_angle);
            json_write_indent(b, depth + 3);
            json_builder_appendf(b, "\"end_angle\": %.6g,\n", g->data.arc.end_angle);
            json_write_indent(b, depth + 3);
            json_builder_append(b, "\"normal\": ");
            json_write_vec3(b, g->data.arc.normal);
            json_builder_append(b, "\n");
            break;

        case GEOM_BEZIER:
            json_write_indent(b, depth + 3);
            json_builder_append(b, "\"p0\": ");
            json_write_vec3(b, g->data.bezier.p0);
            json_builder_append(b, ",\n");
            json_write_indent(b, depth + 3);
            json_builder_append(b, "\"p1\": ");
            json_write_vec3(b, g->data.bezier.p1);
            json_builder_append(b, ",\n");
            json_write_indent(b, depth + 3);
            json_builder_append(b, "\"p2\": ");
            json_write_vec3(b, g->data.bezier.p2);
            json_builder_append(b, ",\n");
            json_write_indent(b, depth + 3);
            json_builder_append(b, "\"p3\": ");
            json_write_vec3(b, g->data.bezier.p3);
            json_builder_append(b, ",\n");
            json_write_indent(b, depth + 3);
            json_builder_appendf(b, "\"segments\": %d\n", g->data.bezier.segments);
            break;

        case GEOM_HELIX:
            json_write_indent(b, depth + 3);
            json_builder_append(b, "\"axis_start\": ");
            json_write_vec3(b, g->data.helix.axis_start);
            json_builder_append(b, ",\n");
            json_write_indent(b, depth + 3);
            json_builder_append(b, "\"axis_end\": ");
            json_write_vec3(b, g->data.helix.axis_end);
            json_builder_append(b, ",\n");
            json_write_indent(b, depth + 3);
            json_builder_appendf(b, "\"radius\": %.6g,\n", g->data.helix.radius);
            json_write_indent(b, depth + 3);
            json_builder_appendf(b, "\"turns\": %.6g,\n", g->data.helix.turns);
            json_write_indent(b, depth + 3);
            json_builder_appendf(b, "\"segments\": %d\n", g->data.helix.segments);
            break;

        default:
            break;
    }

    json_write_indent(b, depth + 2);
    json_builder_append(b, "},\n");

    // Renderable component (just visibility and layer)
    json_write_indent(b, depth + 2);
    json_builder_append(b, "\"renderable\": {\n");
    json_write_indent(b, depth + 3);
    json_builder_appendf(b, "\"visible\": %s,\n", r && r->visible ? "true" : "false");
    json_write_indent(b, depth + 3);
    json_builder_appendf(b, "\"layer\": %d\n", r ? r->layer : 0);
    json_write_indent(b, depth + 2);
    json_builder_append(b, "}\n");

    json_write_indent(b, depth + 1);
    json_builder_append(b, "}\n");

    json_write_indent(b, depth);
    json_builder_append(b, "}");
}

// Save scene to JSON string (caller must free returned string)
static inline char* scene_save_to_string(ecs_scene_t *scene) {
    json_builder_t b;
    json_builder_init(&b);

    ecs_world_state_t *w = scene->world;

    // Start JSON object
    json_builder_append(&b, "{\n");
    json_builder_appendf(&b, "  \"version\": %d,\n", SCENE_JSON_VERSION);
    json_builder_append(&b, "  \"entities\": [\n");

    // Query all entities with GeometryComp
    ecs_query_t *q = ecs_query(w->world, {
        .terms = {
            { .id = w->GeometryComp_id }
        }
    });

    // Collect entities first (to handle commas correctly)
    ecs_entity_t *entities = NULL;
    int entity_count = 0;
    int entity_capacity = 0;

    ecs_iter_t it = ecs_query_iter(w->world, q);
    while (ecs_query_next(&it)) {
        for (int i = 0; i < it.count; i++) {
            if (entity_count >= entity_capacity) {
                entity_capacity = entity_capacity ? entity_capacity * 2 : 64;
                entities = (ecs_entity_t*)realloc(entities, entity_capacity * sizeof(ecs_entity_t));
            }
            entities[entity_count++] = it.entities[i];
        }
    }
    ecs_query_fini(q);

    // Write entities
    for (int i = 0; i < entity_count; i++) {
        scene_write_entity_json(&b, scene, entities[i], 2);
        if (i < entity_count - 1) {
            json_builder_append(&b, ",");
        }
        json_builder_append(&b, "\n");
    }

    free(entities);

    json_builder_append(&b, "  ]\n");
    json_builder_append(&b, "}\n");

    // Return the string (caller must free)
    char *result = b.data;
    b.data = NULL;  // Prevent double-free
    b.length = 0;
    b.capacity = 0;
    return result;
}

// Save scene to file
static inline bool scene_save_to_file(ecs_scene_t *scene, const char *filepath) {
    char *json = scene_save_to_string(scene);
    if (!json) return false;

    FILE *f = fopen(filepath, "w");
    if (!f) {
        free(json);
        return false;
    }

    fputs(json, f);
    fclose(f);
    free(json);
    return true;
}

//------------------------------------------------------------------------------
// Simple JSON Parser (minimal tokenizer)
//------------------------------------------------------------------------------

typedef enum {
    JSON_TOK_NONE,
    JSON_TOK_LBRACE,      // {
    JSON_TOK_RBRACE,      // }
    JSON_TOK_LBRACKET,    // [
    JSON_TOK_RBRACKET,    // ]
    JSON_TOK_COLON,       // :
    JSON_TOK_COMMA,       // ,
    JSON_TOK_STRING,
    JSON_TOK_NUMBER,
    JSON_TOK_TRUE,
    JSON_TOK_FALSE,
    JSON_TOK_NULL,
    JSON_TOK_EOF,
    JSON_TOK_ERROR
} json_token_type_t;

typedef struct {
    const char *json;
    size_t pos;
    size_t length;

    // Current token
    json_token_type_t token;
    char str_value[256];   // For strings
    double num_value;      // For numbers
} json_parser_t;

static inline void json_parser_init(json_parser_t *p, const char *json) {
    p->json = json;
    p->pos = 0;
    p->length = strlen(json);
    p->token = JSON_TOK_NONE;
    p->str_value[0] = '\0';
    p->num_value = 0.0;
}

static inline void json_skip_whitespace(json_parser_t *p) {
    while (p->pos < p->length && isspace((unsigned char)p->json[p->pos])) {
        p->pos++;
    }
}

static inline bool json_parse_string(json_parser_t *p) {
    if (p->json[p->pos] != '"') return false;
    p->pos++;  // Skip opening quote

    size_t start = p->pos;
    size_t out_idx = 0;

    while (p->pos < p->length && p->json[p->pos] != '"') {
        if (p->json[p->pos] == '\\') {
            p->pos++;
            if (p->pos >= p->length) return false;
            // Handle escape sequences
            char escaped = p->json[p->pos];
            switch (escaped) {
                case 'n': p->str_value[out_idx++] = '\n'; break;
                case 't': p->str_value[out_idx++] = '\t'; break;
                case 'r': p->str_value[out_idx++] = '\r'; break;
                case '"': p->str_value[out_idx++] = '"'; break;
                case '\\': p->str_value[out_idx++] = '\\'; break;
                default: p->str_value[out_idx++] = escaped; break;
            }
        } else {
            if (out_idx < sizeof(p->str_value) - 1) {
                p->str_value[out_idx++] = p->json[p->pos];
            }
        }
        p->pos++;
    }

    if (p->pos >= p->length) return false;  // No closing quote
    p->pos++;  // Skip closing quote
    p->str_value[out_idx] = '\0';
    p->token = JSON_TOK_STRING;
    return true;
}

static inline bool json_parse_number(json_parser_t *p) {
    char buf[64];
    size_t i = 0;

    // Handle negative
    if (p->json[p->pos] == '-') {
        buf[i++] = p->json[p->pos++];
    }

    // Integer part
    while (p->pos < p->length && isdigit((unsigned char)p->json[p->pos])) {
        if (i < sizeof(buf) - 1) buf[i++] = p->json[p->pos];
        p->pos++;
    }

    // Decimal part
    if (p->pos < p->length && p->json[p->pos] == '.') {
        if (i < sizeof(buf) - 1) buf[i++] = p->json[p->pos];
        p->pos++;
        while (p->pos < p->length && isdigit((unsigned char)p->json[p->pos])) {
            if (i < sizeof(buf) - 1) buf[i++] = p->json[p->pos];
            p->pos++;
        }
    }

    // Exponent part
    if (p->pos < p->length && (p->json[p->pos] == 'e' || p->json[p->pos] == 'E')) {
        if (i < sizeof(buf) - 1) buf[i++] = p->json[p->pos];
        p->pos++;
        if (p->pos < p->length && (p->json[p->pos] == '+' || p->json[p->pos] == '-')) {
            if (i < sizeof(buf) - 1) buf[i++] = p->json[p->pos];
            p->pos++;
        }
        while (p->pos < p->length && isdigit((unsigned char)p->json[p->pos])) {
            if (i < sizeof(buf) - 1) buf[i++] = p->json[p->pos];
            p->pos++;
        }
    }

    buf[i] = '\0';
    p->num_value = strtod(buf, NULL);
    p->token = JSON_TOK_NUMBER;
    return true;
}

static inline bool json_next_token(json_parser_t *p) {
    json_skip_whitespace(p);

    if (p->pos >= p->length) {
        p->token = JSON_TOK_EOF;
        return true;
    }

    char c = p->json[p->pos];

    switch (c) {
        case '{': p->token = JSON_TOK_LBRACE; p->pos++; return true;
        case '}': p->token = JSON_TOK_RBRACE; p->pos++; return true;
        case '[': p->token = JSON_TOK_LBRACKET; p->pos++; return true;
        case ']': p->token = JSON_TOK_RBRACKET; p->pos++; return true;
        case ':': p->token = JSON_TOK_COLON; p->pos++; return true;
        case ',': p->token = JSON_TOK_COMMA; p->pos++; return true;
        case '"': return json_parse_string(p);
        case '-':
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            return json_parse_number(p);
        case 't':
            if (strncmp(p->json + p->pos, "true", 4) == 0) {
                p->token = JSON_TOK_TRUE;
                p->pos += 4;
                return true;
            }
            break;
        case 'f':
            if (strncmp(p->json + p->pos, "false", 5) == 0) {
                p->token = JSON_TOK_FALSE;
                p->pos += 5;
                return true;
            }
            break;
        case 'n':
            if (strncmp(p->json + p->pos, "null", 4) == 0) {
                p->token = JSON_TOK_NULL;
                p->pos += 4;
                return true;
            }
            break;
    }

    p->token = JSON_TOK_ERROR;
    return false;
}

// Skip a JSON value (for fields we don't care about)
static inline bool json_skip_value(json_parser_t *p);

static inline bool json_skip_object(json_parser_t *p) {
    if (p->token != JSON_TOK_LBRACE) return false;

    if (!json_next_token(p)) return false;

    while (p->token != JSON_TOK_RBRACE) {
        // Key
        if (p->token != JSON_TOK_STRING) return false;
        if (!json_next_token(p)) return false;  // :
        if (p->token != JSON_TOK_COLON) return false;
        if (!json_next_token(p)) return false;  // value
        if (!json_skip_value(p)) return false;

        if (p->token == JSON_TOK_COMMA) {
            if (!json_next_token(p)) return false;
        }
    }

    return json_next_token(p);  // Skip }
}

static inline bool json_skip_array(json_parser_t *p) {
    if (p->token != JSON_TOK_LBRACKET) return false;

    if (!json_next_token(p)) return false;

    while (p->token != JSON_TOK_RBRACKET) {
        if (!json_skip_value(p)) return false;

        if (p->token == JSON_TOK_COMMA) {
            if (!json_next_token(p)) return false;
        }
    }

    return json_next_token(p);  // Skip ]
}

static inline bool json_skip_value(json_parser_t *p) {
    switch (p->token) {
        case JSON_TOK_LBRACE: return json_skip_object(p);
        case JSON_TOK_LBRACKET: return json_skip_array(p);
        case JSON_TOK_STRING:
        case JSON_TOK_NUMBER:
        case JSON_TOK_TRUE:
        case JSON_TOK_FALSE:
        case JSON_TOK_NULL:
            return json_next_token(p);
        default:
            return false;
    }
}

//------------------------------------------------------------------------------
// JSON Parsing Helpers
//------------------------------------------------------------------------------

static inline bool json_parse_vec3(json_parser_t *p, vec3_t *out) {
    if (p->token != JSON_TOK_LBRACKET) return false;

    if (!json_next_token(p)) return false;
    if (p->token != JSON_TOK_NUMBER) return false;
    out->x = (float)p->num_value;

    if (!json_next_token(p)) return false;  // ,
    if (p->token != JSON_TOK_COMMA) return false;

    if (!json_next_token(p)) return false;
    if (p->token != JSON_TOK_NUMBER) return false;
    out->y = (float)p->num_value;

    if (!json_next_token(p)) return false;  // ,
    if (p->token != JSON_TOK_COMMA) return false;

    if (!json_next_token(p)) return false;
    if (p->token != JSON_TOK_NUMBER) return false;
    out->z = (float)p->num_value;

    if (!json_next_token(p)) return false;  // ]
    if (p->token != JSON_TOK_RBRACKET) return false;

    return json_next_token(p);
}

static inline bool json_parse_vec4(json_parser_t *p, vec4_t *out) {
    if (p->token != JSON_TOK_LBRACKET) return false;

    if (!json_next_token(p)) return false;
    if (p->token != JSON_TOK_NUMBER) return false;
    out->x = (float)p->num_value;

    if (!json_next_token(p)) return false;
    if (p->token != JSON_TOK_COMMA) return false;

    if (!json_next_token(p)) return false;
    if (p->token != JSON_TOK_NUMBER) return false;
    out->y = (float)p->num_value;

    if (!json_next_token(p)) return false;
    if (p->token != JSON_TOK_COMMA) return false;

    if (!json_next_token(p)) return false;
    if (p->token != JSON_TOK_NUMBER) return false;
    out->z = (float)p->num_value;

    if (!json_next_token(p)) return false;
    if (p->token != JSON_TOK_COMMA) return false;

    if (!json_next_token(p)) return false;
    if (p->token != JSON_TOK_NUMBER) return false;
    out->w = (float)p->num_value;

    if (!json_next_token(p)) return false;
    if (p->token != JSON_TOK_RBRACKET) return false;

    return json_next_token(p);
}

// Parse an array of vec3 (for polyline/polygon points)
static inline bool json_parse_vec3_array(json_parser_t *p, vec3_t **out_points, int *out_count) {
    if (p->token != JSON_TOK_LBRACKET) return false;

    int capacity = 16;
    int count = 0;
    vec3_t *points = (vec3_t*)malloc(capacity * sizeof(vec3_t));

    if (!json_next_token(p)) {
        free(points);
        return false;
    }

    while (p->token != JSON_TOK_RBRACKET) {
        if (count >= capacity) {
            capacity *= 2;
            points = (vec3_t*)realloc(points, capacity * sizeof(vec3_t));
        }

        if (!json_parse_vec3(p, &points[count])) {
            free(points);
            return false;
        }
        count++;

        if (p->token == JSON_TOK_COMMA) {
            if (!json_next_token(p)) {
                free(points);
                return false;
            }
        }
    }

    if (!json_next_token(p)) {  // Skip ]
        free(points);
        return false;
    }

    *out_points = points;
    *out_count = count;
    return true;
}

//------------------------------------------------------------------------------
// Entity Data Structure for Loading
//------------------------------------------------------------------------------

typedef struct {
    uint64_t old_id;           // ID from file
    uint64_t parent_old_id;    // Parent ID from file (0 if no parent)
    ecs_entity_t new_entity;   // Created entity in scene

    // Transform
    vec3_t position;
    vec3_t rotation;
    vec3_t scale;

    // Geometry
    geometry_type_t geom_type;
    vec4_t color;
    float line_width;
    float point_size;

    // Type-specific geometry data
    union {
        struct { vec3_t point; } point;
        struct { vec3_t a, b; } line;
        struct { vec3_t *points; int count; } polyline;
        struct { vec3_t *points; int count; } polygon;
        struct { vec3_t center; float radius; float start_angle, end_angle; vec3_t normal; } arc;
        struct { vec3_t p0, p1, p2, p3; int segments; } bezier;
        struct { vec3_t axis_start, axis_end; float radius, turns; int segments; } helix;
    } geom_data;

    // Renderable
    bool visible;
    int layer;
} loaded_entity_t;

//------------------------------------------------------------------------------
// Parse Transform Component
//------------------------------------------------------------------------------

static inline bool json_parse_transform(json_parser_t *p, loaded_entity_t *ent) {
    if (p->token != JSON_TOK_LBRACE) return false;

    // Set defaults
    ent->position = vec3_make(0, 0, 0);
    ent->rotation = vec3_make(0, 0, 0);
    ent->scale = vec3_make(1, 1, 1);

    if (!json_next_token(p)) return false;

    while (p->token != JSON_TOK_RBRACE) {
        if (p->token != JSON_TOK_STRING) return false;
        char key[64];
        strncpy(key, p->str_value, sizeof(key) - 1);
        key[sizeof(key) - 1] = '\0';

        if (!json_next_token(p)) return false;  // :
        if (p->token != JSON_TOK_COLON) return false;
        if (!json_next_token(p)) return false;  // value

        if (strcmp(key, "position") == 0) {
            if (!json_parse_vec3(p, &ent->position)) return false;
        } else if (strcmp(key, "rotation") == 0) {
            if (!json_parse_vec3(p, &ent->rotation)) return false;
        } else if (strcmp(key, "scale") == 0) {
            if (!json_parse_vec3(p, &ent->scale)) return false;
        } else {
            if (!json_skip_value(p)) return false;
        }

        if (p->token == JSON_TOK_COMMA) {
            if (!json_next_token(p)) return false;
        }
    }

    return json_next_token(p);  // Skip }
}

//------------------------------------------------------------------------------
// Parse Geometry Component
//------------------------------------------------------------------------------

static inline bool json_parse_geometry(json_parser_t *p, loaded_entity_t *ent) {
    if (p->token != JSON_TOK_LBRACE) return false;

    // Set defaults
    ent->geom_type = GEOM_POINT;
    ent->color = vec4_make(1, 1, 1, 1);
    ent->line_width = 0.02f;
    ent->point_size = 0.05f;

    if (!json_next_token(p)) return false;

    while (p->token != JSON_TOK_RBRACE) {
        if (p->token != JSON_TOK_STRING) return false;
        char key[64];
        strncpy(key, p->str_value, sizeof(key) - 1);
        key[sizeof(key) - 1] = '\0';

        if (!json_next_token(p)) return false;  // :
        if (p->token != JSON_TOK_COLON) return false;
        if (!json_next_token(p)) return false;  // value

        if (strcmp(key, "type") == 0) {
            if (p->token != JSON_TOK_STRING) return false;
            ent->geom_type = scene_string_to_geom_type(p->str_value);
            if (!json_next_token(p)) return false;
        } else if (strcmp(key, "color") == 0) {
            if (!json_parse_vec4(p, &ent->color)) return false;
        } else if (strcmp(key, "line_width") == 0) {
            if (p->token != JSON_TOK_NUMBER) return false;
            ent->line_width = (float)p->num_value;
            if (!json_next_token(p)) return false;
        } else if (strcmp(key, "point_size") == 0) {
            if (p->token != JSON_TOK_NUMBER) return false;
            ent->point_size = (float)p->num_value;
            if (!json_next_token(p)) return false;
        } else if (strcmp(key, "point") == 0) {
            if (!json_parse_vec3(p, &ent->geom_data.point.point)) return false;
        } else if (strcmp(key, "a") == 0) {
            if (!json_parse_vec3(p, &ent->geom_data.line.a)) return false;
        } else if (strcmp(key, "b") == 0) {
            if (!json_parse_vec3(p, &ent->geom_data.line.b)) return false;
        } else if (strcmp(key, "points") == 0) {
            // For polyline or polygon
            vec3_t *pts = NULL;
            int count = 0;
            if (!json_parse_vec3_array(p, &pts, &count)) return false;
            if (ent->geom_type == GEOM_POLYLINE) {
                ent->geom_data.polyline.points = pts;
                ent->geom_data.polyline.count = count;
            } else {
                ent->geom_data.polygon.points = pts;
                ent->geom_data.polygon.count = count;
            }
        } else if (strcmp(key, "center") == 0) {
            if (!json_parse_vec3(p, &ent->geom_data.arc.center)) return false;
        } else if (strcmp(key, "radius") == 0) {
            if (p->token != JSON_TOK_NUMBER) return false;
            if (ent->geom_type == GEOM_ARC) {
                ent->geom_data.arc.radius = (float)p->num_value;
            } else {
                ent->geom_data.helix.radius = (float)p->num_value;
            }
            if (!json_next_token(p)) return false;
        } else if (strcmp(key, "start_angle") == 0) {
            if (p->token != JSON_TOK_NUMBER) return false;
            ent->geom_data.arc.start_angle = (float)p->num_value;
            if (!json_next_token(p)) return false;
        } else if (strcmp(key, "end_angle") == 0) {
            if (p->token != JSON_TOK_NUMBER) return false;
            ent->geom_data.arc.end_angle = (float)p->num_value;
            if (!json_next_token(p)) return false;
        } else if (strcmp(key, "normal") == 0) {
            if (!json_parse_vec3(p, &ent->geom_data.arc.normal)) return false;
        } else if (strcmp(key, "p0") == 0) {
            if (!json_parse_vec3(p, &ent->geom_data.bezier.p0)) return false;
        } else if (strcmp(key, "p1") == 0) {
            if (!json_parse_vec3(p, &ent->geom_data.bezier.p1)) return false;
        } else if (strcmp(key, "p2") == 0) {
            if (!json_parse_vec3(p, &ent->geom_data.bezier.p2)) return false;
        } else if (strcmp(key, "p3") == 0) {
            if (!json_parse_vec3(p, &ent->geom_data.bezier.p3)) return false;
        } else if (strcmp(key, "segments") == 0) {
            if (p->token != JSON_TOK_NUMBER) return false;
            if (ent->geom_type == GEOM_BEZIER) {
                ent->geom_data.bezier.segments = (int)p->num_value;
            } else {
                ent->geom_data.helix.segments = (int)p->num_value;
            }
            if (!json_next_token(p)) return false;
        } else if (strcmp(key, "axis_start") == 0) {
            if (!json_parse_vec3(p, &ent->geom_data.helix.axis_start)) return false;
        } else if (strcmp(key, "axis_end") == 0) {
            if (!json_parse_vec3(p, &ent->geom_data.helix.axis_end)) return false;
        } else if (strcmp(key, "turns") == 0) {
            if (p->token != JSON_TOK_NUMBER) return false;
            ent->geom_data.helix.turns = (float)p->num_value;
            if (!json_next_token(p)) return false;
        } else {
            if (!json_skip_value(p)) return false;
        }

        if (p->token == JSON_TOK_COMMA) {
            if (!json_next_token(p)) return false;
        }
    }

    return json_next_token(p);  // Skip }
}

//------------------------------------------------------------------------------
// Parse Renderable Component
//------------------------------------------------------------------------------

static inline bool json_parse_renderable(json_parser_t *p, loaded_entity_t *ent) {
    if (p->token != JSON_TOK_LBRACE) return false;

    // Set defaults
    ent->visible = true;
    ent->layer = 0;

    if (!json_next_token(p)) return false;

    while (p->token != JSON_TOK_RBRACE) {
        if (p->token != JSON_TOK_STRING) return false;
        char key[64];
        strncpy(key, p->str_value, sizeof(key) - 1);
        key[sizeof(key) - 1] = '\0';

        if (!json_next_token(p)) return false;  // :
        if (p->token != JSON_TOK_COLON) return false;
        if (!json_next_token(p)) return false;  // value

        if (strcmp(key, "visible") == 0) {
            ent->visible = (p->token == JSON_TOK_TRUE);
            if (!json_next_token(p)) return false;
        } else if (strcmp(key, "layer") == 0) {
            if (p->token != JSON_TOK_NUMBER) return false;
            ent->layer = (int)p->num_value;
            if (!json_next_token(p)) return false;
        } else {
            if (!json_skip_value(p)) return false;
        }

        if (p->token == JSON_TOK_COMMA) {
            if (!json_next_token(p)) return false;
        }
    }

    return json_next_token(p);  // Skip }
}

//------------------------------------------------------------------------------
// Parse Single Entity
//------------------------------------------------------------------------------

static inline bool json_parse_entity(json_parser_t *p, loaded_entity_t *ent) {
    if (p->token != JSON_TOK_LBRACE) return false;

    // Initialize
    memset(ent, 0, sizeof(*ent));
    ent->scale = vec3_make(1, 1, 1);
    ent->visible = true;

    if (!json_next_token(p)) return false;

    while (p->token != JSON_TOK_RBRACE) {
        if (p->token != JSON_TOK_STRING) return false;
        char key[64];
        strncpy(key, p->str_value, sizeof(key) - 1);
        key[sizeof(key) - 1] = '\0';

        if (!json_next_token(p)) return false;  // :
        if (p->token != JSON_TOK_COLON) return false;
        if (!json_next_token(p)) return false;  // value

        if (strcmp(key, "id") == 0) {
            if (p->token != JSON_TOK_NUMBER) return false;
            ent->old_id = (uint64_t)p->num_value;
            if (!json_next_token(p)) return false;
        } else if (strcmp(key, "parent") == 0) {
            if (p->token == JSON_TOK_NULL) {
                ent->parent_old_id = 0;
            } else if (p->token == JSON_TOK_NUMBER) {
                ent->parent_old_id = (uint64_t)p->num_value;
            }
            if (!json_next_token(p)) return false;
        } else if (strcmp(key, "components") == 0) {
            if (p->token != JSON_TOK_LBRACE) return false;
            if (!json_next_token(p)) return false;

            while (p->token != JSON_TOK_RBRACE) {
                if (p->token != JSON_TOK_STRING) return false;
                char comp_key[64];
                strncpy(comp_key, p->str_value, sizeof(comp_key) - 1);
                comp_key[sizeof(comp_key) - 1] = '\0';

                if (!json_next_token(p)) return false;  // :
                if (p->token != JSON_TOK_COLON) return false;
                if (!json_next_token(p)) return false;  // value

                if (strcmp(comp_key, "transform") == 0) {
                    if (!json_parse_transform(p, ent)) return false;
                } else if (strcmp(comp_key, "geometry") == 0) {
                    if (!json_parse_geometry(p, ent)) return false;
                } else if (strcmp(comp_key, "renderable") == 0) {
                    if (!json_parse_renderable(p, ent)) return false;
                } else {
                    if (!json_skip_value(p)) return false;
                }

                if (p->token == JSON_TOK_COMMA) {
                    if (!json_next_token(p)) return false;
                }
            }

            if (!json_next_token(p)) return false;  // Skip }
        } else {
            if (!json_skip_value(p)) return false;
        }

        if (p->token == JSON_TOK_COMMA) {
            if (!json_next_token(p)) return false;
        }
    }

    return json_next_token(p);  // Skip }
}

//------------------------------------------------------------------------------
// Scene Load - Import from JSON
//------------------------------------------------------------------------------

// Helper to create entity in scene based on loaded data
static inline ecs_entity_t scene_create_from_loaded(ecs_scene_t *scene, loaded_entity_t *ent) {
    ecs_entity_t e = 0;

    switch (ent->geom_type) {
        case GEOM_POINT:
            e = scene_add_point(scene, ent->geom_data.point.point, ent->color, ent->point_size);
            break;

        case GEOM_LINE:
            e = scene_add_line(scene, ent->geom_data.line.a, ent->geom_data.line.b,
                              ent->color, ent->line_width);
            break;

        case GEOM_POLYLINE:
            if (ent->geom_data.polyline.points && ent->geom_data.polyline.count >= 2) {
                e = scene_add_polyline(scene, ent->geom_data.polyline.points,
                                       ent->geom_data.polyline.count,
                                       ent->color, ent->line_width);
            }
            break;

        case GEOM_POLYGON:
            if (ent->geom_data.polygon.points && ent->geom_data.polygon.count >= 3) {
                e = scene_add_polygon(scene, ent->geom_data.polygon.points,
                                      ent->geom_data.polygon.count,
                                      ent->color, ent->line_width);
            }
            break;

        case GEOM_ARC:
            e = scene_add_arc(scene,
                             ent->geom_data.arc.center, ent->geom_data.arc.radius,
                             ent->geom_data.arc.start_angle, ent->geom_data.arc.end_angle,
                             ent->geom_data.arc.normal,
                             ent->color, ent->line_width);
            break;

        case GEOM_BEZIER:
            e = scene_add_bezier(scene,
                                ent->geom_data.bezier.p0, ent->geom_data.bezier.p1,
                                ent->geom_data.bezier.p2, ent->geom_data.bezier.p3,
                                ent->geom_data.bezier.segments,
                                ent->color, ent->line_width);
            break;

        case GEOM_HELIX:
            e = scene_add_helix(scene,
                               ent->geom_data.helix.axis_start, ent->geom_data.helix.axis_end,
                               ent->geom_data.helix.radius, ent->geom_data.helix.turns,
                               ent->geom_data.helix.segments,
                               ent->color, ent->line_width);
            break;

        default:
            break;
    }

    if (e != 0) {
        // Set transform
        TransformComp *t = ecs_world_get_transform(scene->world, e);
        if (t) {
            t->position = ent->position;
            t->rotation = ent->rotation;
            t->scale = ent->scale;
            t->dirty = true;
        }

        // Set visibility
        RenderableComp *r = ecs_world_get_renderable(scene->world, e);
        if (r) {
            r->visible = ent->visible;
            r->layer = ent->layer;
            r->instance_dirty = true;
        }
    }

    return e;
}

// Find old ID -> new entity mapping
static inline ecs_entity_t scene_find_new_entity(loaded_entity_t *entities, int count,
                                                   uint64_t old_id) {
    for (int i = 0; i < count; i++) {
        if (entities[i].old_id == old_id) {
            return entities[i].new_entity;
        }
    }
    return 0;
}

// Load scene from JSON string
// Returns number of entities loaded, or -1 on error
static inline int scene_load_from_string(ecs_scene_t *scene, const char *json,
                                          bool clear_existing) {
    json_parser_t p;
    json_parser_init(&p, json);

    if (!json_next_token(&p)) return -1;
    if (p.token != JSON_TOK_LBRACE) return -1;

    if (!json_next_token(&p)) return -1;

    // Parse top-level object
    int version = 0;
    loaded_entity_t *entities = NULL;
    int entity_count = 0;
    int entity_capacity = 0;

    while (p.token != JSON_TOK_RBRACE && p.token != JSON_TOK_EOF) {
        if (p.token != JSON_TOK_STRING) {
            free(entities);
            return -1;
        }

        char key[64];
        strncpy(key, p.str_value, sizeof(key) - 1);
        key[sizeof(key) - 1] = '\0';

        if (!json_next_token(&p)) { free(entities); return -1; }
        if (p.token != JSON_TOK_COLON) { free(entities); return -1; }
        if (!json_next_token(&p)) { free(entities); return -1; }

        if (strcmp(key, "version") == 0) {
            if (p.token != JSON_TOK_NUMBER) { free(entities); return -1; }
            version = (int)p.num_value;
            if (!json_next_token(&p)) { free(entities); return -1; }
        } else if (strcmp(key, "entities") == 0) {
            if (p.token != JSON_TOK_LBRACKET) { free(entities); return -1; }
            if (!json_next_token(&p)) { free(entities); return -1; }

            while (p.token != JSON_TOK_RBRACKET) {
                if (entity_count >= entity_capacity) {
                    entity_capacity = entity_capacity ? entity_capacity * 2 : 64;
                    entities = (loaded_entity_t*)realloc(entities,
                                                          entity_capacity * sizeof(loaded_entity_t));
                }

                if (!json_parse_entity(&p, &entities[entity_count])) {
                    // Free any allocated points
                    for (int i = 0; i < entity_count; i++) {
                        if (entities[i].geom_type == GEOM_POLYLINE && entities[i].geom_data.polyline.points) {
                            free(entities[i].geom_data.polyline.points);
                        } else if (entities[i].geom_type == GEOM_POLYGON && entities[i].geom_data.polygon.points) {
                            free(entities[i].geom_data.polygon.points);
                        }
                    }
                    free(entities);
                    return -1;
                }
                entity_count++;

                if (p.token == JSON_TOK_COMMA) {
                    if (!json_next_token(&p)) {
                        free(entities);
                        return -1;
                    }
                }
            }

            if (!json_next_token(&p)) { free(entities); return -1; }
        } else {
            if (!json_skip_value(&p)) { free(entities); return -1; }
        }

        if (p.token == JSON_TOK_COMMA) {
            if (!json_next_token(&p)) { free(entities); return -1; }
        }
    }

    // Check version
    if (version != SCENE_JSON_VERSION) {
        // Could add version migration here in the future
    }

    // Clear existing scene if requested
    if (clear_existing) {
        // Query all existing entities with GeometryComp and delete them
        ecs_world_state_t *w = scene->world;
        ecs_query_t *q = ecs_query(w->world, {
            .terms = {{ .id = w->GeometryComp_id }}
        });

        // Collect entities to delete (can't delete during iteration)
        ecs_entity_t *to_delete = NULL;
        int delete_count = 0;
        int delete_capacity = 0;

        ecs_iter_t it = ecs_query_iter(w->world, q);
        while (ecs_query_next(&it)) {
            for (int i = 0; i < it.count; i++) {
                if (delete_count >= delete_capacity) {
                    delete_capacity = delete_capacity ? delete_capacity * 2 : 64;
                    to_delete = (ecs_entity_t*)realloc(to_delete, delete_capacity * sizeof(ecs_entity_t));
                }
                to_delete[delete_count++] = it.entities[i];
            }
        }
        ecs_query_fini(q);

        // Delete collected entities
        for (int i = 0; i < delete_count; i++) {
            scene_remove_entity(scene, to_delete[i]);
        }
        free(to_delete);
    }

    // Create entities (first pass - create all entities)
    for (int i = 0; i < entity_count; i++) {
        entities[i].new_entity = scene_create_from_loaded(scene, &entities[i]);
    }

    // Set up parent-child relationships (second pass)
    for (int i = 0; i < entity_count; i++) {
        if (entities[i].parent_old_id != 0 && entities[i].new_entity != 0) {
            ecs_entity_t new_parent = scene_find_new_entity(entities, entity_count,
                                                             entities[i].parent_old_id);
            if (new_parent != 0) {
                scene_set_parent(scene, entities[i].new_entity, new_parent);
            }
        }
    }

    // Free loaded entity data
    for (int i = 0; i < entity_count; i++) {
        if (entities[i].geom_type == GEOM_POLYLINE && entities[i].geom_data.polyline.points) {
            free(entities[i].geom_data.polyline.points);
        } else if (entities[i].geom_type == GEOM_POLYGON && entities[i].geom_data.polygon.points) {
            free(entities[i].geom_data.polygon.points);
        }
    }
    free(entities);

    return entity_count;
}

// Load scene from file
// Returns number of entities loaded, or -1 on error
static inline int scene_load_from_file(ecs_scene_t *scene, const char *filepath,
                                        bool clear_existing) {
    FILE *f = fopen(filepath, "r");
    if (!f) return -1;

    // Get file size
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    // Read entire file
    char *json = (char*)malloc(size + 1);
    if (!json) {
        fclose(f);
        return -1;
    }

    size_t read = fread(json, 1, size, f);
    fclose(f);

    json[read] = '\0';

    int result = scene_load_from_string(scene, json, clear_existing);
    free(json);

    return result;
}

#endif // SCENE_SERIALIZER_H
