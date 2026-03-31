//------------------------------------------------------------------------------
// scene_serializer.h - JSON scene serialization (header-only)
//
// Provides save/load functionality for ECS scenes.
// JSON format version 2.
//------------------------------------------------------------------------------
#ifndef SCENE_SERIALIZER_H
#define SCENE_SERIALIZER_H

#include "ecs/ecs_world.h"
#include "ecs/ecs_scene.h"
#include "components/geometry_comp.h"
#include "components/transform_comp.h"
#include "components/renderable_comp.h"
#include "math/cglm_entry.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

//------------------------------------------------------------------------------
// JSON Format Version
//------------------------------------------------------------------------------
#define SCENE_JSON_VERSION 2
#define SCENE_JSON_FORMAT "mdcad-scene"

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

static inline void json_write_string_escaped(json_builder_t *b, const char *str) {
    json_builder_append(b, "\"");
    if (str) {
        for (const char *c = str; *c; c++) {
            switch (*c) {
                case '\"': json_builder_append(b, "\\\""); break;
                case '\\': json_builder_append(b, "\\\\"); break;
                case '\n': json_builder_append(b, "\\n"); break;
                case '\r': json_builder_append(b, "\\r"); break;
                case '\t': json_builder_append(b, "\\t"); break;
                default: {
                    char ch[2] = { *c, '\0' };
                    json_builder_append(b, ch);
                    break;
                }
            }
        }
    }
    json_builder_append(b, "\"");
}

static inline vec3_t scene_ser_vec3(float x, float y, float z) {
    vec3s value = (vec3s){ { x, y, z } };
    return (vec3_t){ value.x, value.y, value.z };
}

static inline vec4_t scene_ser_vec4(float x, float y, float z, float w) {
    vec4s value = (vec4s){ { x, y, z, w } };
    return (vec4_t){ value.x, value.y, value.z, value.w };
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
        "point", "line", "polyline", "arc", "polygon", "helix", "bezier",
        "point_cloud", "triangle", "mesh"
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
    if (strcmp(str, "triangle") == 0) return GEOM_TRIANGLE;
    if (strcmp(str, "mesh") == 0) return GEOM_MESH;
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
    LabelComp *label = ecs_world_get_label(w, e);
    SketchComp *sketch = ecs_world_get_sketch(w, e);
    SketchGeometryStateComp *sketch_state = ecs_world_get_sketch_geometry_state(w, e);
    ConstraintComp *constraint = ecs_world_get_constraint(w, e);

    // Only serialize entities that contribute to persisted scene state.
    // This keeps transient anchor entities out of save files.
    if (!t) return;
    if (!g && !sketch && !constraint) return;

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

    // Components (optional, order-stable for readability)
    json_write_indent(b, depth + 1);
    json_builder_append(b, "\"components\": {\n");
    bool wrote_component = false;

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
    json_builder_append(b, "}");
    wrote_component = true;

    // Geometry component
    if (g) {
        if (wrote_component) json_builder_append(b, ",\n");
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

            case GEOM_TRIANGLE:
                json_write_indent(b, depth + 3);
                json_builder_append(b, "\"a\": ");
                json_write_vec3(b, g->data.triangle.a);
                json_builder_append(b, ",\n");
                json_write_indent(b, depth + 3);
                json_builder_append(b, "\"b\": ");
                json_write_vec3(b, g->data.triangle.b);
                json_builder_append(b, ",\n");
                json_write_indent(b, depth + 3);
                json_builder_append(b, "\"c\": ");
                json_write_vec3(b, g->data.triangle.c);
                if (g->data.triangle.has_vertex_colors) {
                    json_builder_append(b, ",\n");
                    json_write_indent(b, depth + 3);
                    json_builder_append(b, "\"color_a\": ");
                    json_write_vec4(b, g->data.triangle.color_a);
                    json_builder_append(b, ",\n");
                    json_write_indent(b, depth + 3);
                    json_builder_append(b, "\"color_b\": ");
                    json_write_vec4(b, g->data.triangle.color_b);
                    json_builder_append(b, ",\n");
                    json_write_indent(b, depth + 3);
                    json_builder_append(b, "\"color_c\": ");
                    json_write_vec4(b, g->data.triangle.color_c);
                }
                json_builder_append(b, "\n");
                break;

            case GEOM_MESH:
                // Vertices array
                json_write_indent(b, depth + 3);
                json_builder_append(b, "\"vertices\": [\n");
                for (int i = 0; i < g->data.mesh.vertex_count; i++) {
                    json_write_indent(b, depth + 4);
                    json_write_vec3(b, g->data.mesh.vertices[i]);
                    if (i < g->data.mesh.vertex_count - 1) json_builder_append(b, ",");
                    json_builder_append(b, "\n");
                }
                json_write_indent(b, depth + 3);
                json_builder_append(b, "],\n");
                // Indices array
                json_write_indent(b, depth + 3);
                json_builder_append(b, "\"indices\": [");
                for (int i = 0; i < g->data.mesh.index_count; i++) {
                    if (i > 0) json_builder_append(b, ", ");
                    json_builder_appendf(b, "%u", g->data.mesh.indices[i]);
                }
                json_builder_append(b, "]");
                // Per-vertex colors (optional)
                if (g->data.mesh.vertex_colors) {
                    json_builder_append(b, ",\n");
                    json_write_indent(b, depth + 3);
                    json_builder_append(b, "\"vertex_colors\": [\n");
                    for (int i = 0; i < g->data.mesh.vertex_count; i++) {
                        json_write_indent(b, depth + 4);
                        json_write_vec4(b, g->data.mesh.vertex_colors[i]);
                        if (i < g->data.mesh.vertex_count - 1) json_builder_append(b, ",");
                        json_builder_append(b, "\n");
                    }
                    json_write_indent(b, depth + 3);
                    json_builder_append(b, "]");
                }
                json_builder_append(b, "\n");
                break;

            default:
                break;
        }

        json_write_indent(b, depth + 2);
        json_builder_append(b, "}");
        wrote_component = true;
    }

    if (r) {
        if (wrote_component) json_builder_append(b, ",\n");
        json_write_indent(b, depth + 2);
        json_builder_append(b, "\"renderable\": {\n");
        json_write_indent(b, depth + 3);
        json_builder_appendf(b, "\"visible\": %s,\n", r->visible ? "true" : "false");
        json_write_indent(b, depth + 3);
        json_builder_appendf(b, "\"layer\": %d\n", r->layer);
        json_write_indent(b, depth + 2);
        json_builder_append(b, "}");
        wrote_component = true;
    }

    if (label) {
        if (wrote_component) json_builder_append(b, ",\n");
        json_write_indent(b, depth + 2);
        json_builder_append(b, "\"label\": {\n");
        json_write_indent(b, depth + 3);
        json_builder_append(b, "\"name\": ");
        json_write_string_escaped(b, label->name);
        json_builder_append(b, ",\n");
        json_write_indent(b, depth + 3);
        json_builder_append(b, "\"description\": ");
        json_write_string_escaped(b, label->description);
        json_builder_append(b, "\n");
        json_write_indent(b, depth + 2);
        json_builder_append(b, "}");
        wrote_component = true;
    }

    if (sketch) {
        if (wrote_component) json_builder_append(b, ",\n");
        json_write_indent(b, depth + 2);
        json_builder_append(b, "\"sketch\": {\n");
        json_write_indent(b, depth + 3);
        json_builder_appendf(b, "\"status\": %d,\n", (int)sketch->status);
        json_write_indent(b, depth + 3);
        json_builder_append(b, "\"color\": ");
        json_write_vec4(b, sketch->color);
        json_builder_append(b, ",\n");

        json_write_indent(b, depth + 3);
        json_builder_append(b, "\"next_geometry_name_index\": [");
        for (int i = 0; i < GEOM_TYPE_COUNT; i++) {
            if (i > 0) json_builder_append(b, ", ");
            json_builder_appendf(b, "%u", sketch->next_geometry_name_index[i]);
        }
        json_builder_append(b, "],\n");

        json_write_indent(b, depth + 3);
        json_builder_append(b, "\"next_constraint_name_index\": [");
        for (int i = 0; i < CONSTRAINT_TYPE_COUNT; i++) {
            if (i > 0) json_builder_append(b, ", ");
            json_builder_appendf(b, "%u", sketch->next_constraint_name_index[i]);
        }
        json_builder_append(b, "]\n");

        json_write_indent(b, depth + 2);
        json_builder_append(b, "}");
        wrote_component = true;
    }

    if (sketch_state) {
        if (wrote_component) json_builder_append(b, ",\n");
        json_write_indent(b, depth + 2);
        json_builder_append(b, "\"sketch_geometry_state\": {\n");
        json_write_indent(b, depth + 3);
        json_builder_appendf(b, "\"fixed\": %s\n", sketch_state->fixed ? "true" : "false");
        json_write_indent(b, depth + 2);
        json_builder_append(b, "}");
        wrote_component = true;
    }

    if (constraint) {
        if (wrote_component) json_builder_append(b, ",\n");
        json_write_indent(b, depth + 2);
        json_builder_append(b, "\"constraint\": {\n");
        json_write_indent(b, depth + 3);
        json_builder_appendf(b, "\"type\": %d,\n", (int)constraint->type);
        json_write_indent(b, depth + 3);
        json_builder_appendf(b, "\"has_value\": %s,\n", constraint->has_value ? "true" : "false");
        json_write_indent(b, depth + 3);
        json_builder_appendf(b, "\"driven\": %s,\n", constraint->driven ? "true" : "false");
        json_write_indent(b, depth + 3);
        json_builder_appendf(b, "\"value\": %.6g,\n", constraint->value);
        json_write_indent(b, depth + 3);
        json_builder_appendf(b, "\"display_decimals\": %u,\n", (unsigned int)constraint->display_decimals);
        json_write_indent(b, depth + 3);
        json_builder_append(b, "\"participants\": [");
        for (uint32_t i = 0; i < constraint->participant_count; i++) {
            if (i > 0) json_builder_append(b, ", ");
            json_builder_appendf(b, "%llu", (unsigned long long)constraint->participants[i]);
        }
        json_builder_append(b, "]\n");
        json_write_indent(b, depth + 2);
        json_builder_append(b, "}");
        wrote_component = true;
    }

    json_builder_append(b, "\n");

    json_write_indent(b, depth + 1);
    json_builder_append(b, "}\n");

    json_write_indent(b, depth);
    json_builder_append(b, "}");
}

// Write a light entity to JSON
static inline void scene_write_light_json(json_builder_t *b, ecs_scene_t *scene,
                                           ecs_entity_t e, int depth) {
    ecs_world_state_t *w = scene->world;

    TransformComp *t = ecs_world_get_transform(w, e);
    LightComp *l = ecs_world_get_light(w, e);

    if (!t || !l) return;

    json_write_indent(b, depth);
    json_builder_append(b, "{\n");

    json_write_indent(b, depth + 1);
    json_builder_appendf(b, "\"id\": %llu,\n", (unsigned long long)e);

    json_write_indent(b, depth + 1);
    json_builder_append(b, "\"parent\": null,\n");

    json_write_indent(b, depth + 1);
    json_builder_append(b, "\"components\": {\n");

    // Transform (direction/position stored in position field)
    json_write_indent(b, depth + 2);
    json_builder_append(b, "\"transform\": {\n");
    json_write_indent(b, depth + 3);
    json_builder_append(b, "\"position\": ");
    json_write_vec3(b, t->position);
    json_builder_append(b, "\n");
    json_write_indent(b, depth + 2);
    json_builder_append(b, "},\n");

    // Light component
    json_write_indent(b, depth + 2);
    json_builder_append(b, "\"light\": {\n");
    json_write_indent(b, depth + 3);
    json_builder_appendf(b, "\"type\": \"%s\",\n",
        l->type == LIGHT_DIRECTIONAL ? "directional" : "point");
    json_write_indent(b, depth + 3);
    json_builder_append(b, "\"color\": ");
    json_write_vec4(b, l->color);
    json_builder_append(b, ",\n");
    json_write_indent(b, depth + 3);
    json_builder_appendf(b, "\"intensity\": %.6g\n", l->intensity);
    json_write_indent(b, depth + 2);
    json_builder_append(b, "}\n");

    json_write_indent(b, depth + 1);
    json_builder_append(b, "}\n");

    json_write_indent(b, depth);
    json_builder_append(b, "}");
}

static inline bool scene_entity_list_contains(const ecs_entity_t *entities, int count, ecs_entity_t e) {
    if (!entities || e == 0) return false;
    for (int i = 0; i < count; i++) {
        if (entities[i] == e) return true;
    }
    return false;
}

static inline void scene_entity_list_push_unique(ecs_entity_t **entities, int *count, int *capacity, ecs_entity_t e) {
    if (!entities || !count || !capacity || e == 0) return;
    if (scene_entity_list_contains(*entities, *count, e)) return;

    if (*count >= *capacity) {
        *capacity = (*capacity) ? (*capacity * 2) : 64;
        *entities = (ecs_entity_t*)realloc(*entities, (*capacity) * sizeof(ecs_entity_t));
    }
    (*entities)[(*count)++] = e;
}

// Save scene to JSON string (caller must free returned string)
static inline char* scene_save_to_string(ecs_scene_t *scene) {
    json_builder_t b;
    json_builder_init(&b);

    ecs_world_state_t *w = scene->world;

    // Start JSON object
    json_builder_append(&b, "{\n");
    json_builder_appendf(&b, "  \"format\": \"%s\",\n", SCENE_JSON_FORMAT);
    json_builder_appendf(&b, "  \"version\": %d,\n", SCENE_JSON_VERSION);
    json_builder_append(&b, "  \"entities\": [\n");

    // Collect all persisted scene entities:
    // - Geometry entities
    // - Sketch anchors
    // - Constraint anchors
    ecs_entity_t *entities = NULL;
    int entity_count = 0;
    int entity_capacity = 0;
    ecs_entity_t persisted_component_ids[3] = {
        w->GeometryComp_id,
        w->SketchComp_id,
        w->ConstraintComp_id
    };

    for (int c = 0; c < 3; c++) {
        ecs_query_t *q = ecs_query(w->world, {
            .terms = {
                { .id = persisted_component_ids[c] }
            }
        });

        ecs_iter_t it = ecs_query_iter(w->world, q);
        while (ecs_query_next(&it)) {
            for (int i = 0; i < it.count; i++) {
                scene_entity_list_push_unique(&entities, &entity_count, &entity_capacity, it.entities[i]);
            }
        }
        ecs_query_fini(q);
    }

    // Also collect light entities
    ecs_query_t *lq = ecs_query(w->world, {
        .terms = {
            { .id = w->LightComp_id }
        }
    });

    ecs_entity_t *light_entities = NULL;
    int light_count = 0;
    int light_capacity = 0;

    ecs_iter_t lit = ecs_query_iter(w->world, lq);
    while (ecs_query_next(&lit)) {
        for (int i = 0; i < lit.count; i++) {
            if (light_count >= light_capacity) {
                light_capacity = light_capacity ? light_capacity * 2 : 8;
                light_entities = (ecs_entity_t*)realloc(light_entities, light_capacity * sizeof(ecs_entity_t));
            }
            light_entities[light_count++] = lit.entities[i];
        }
    }
    ecs_query_fini(lq);

    int total_count = entity_count + light_count;

    // Write geometry entities
    for (int i = 0; i < entity_count; i++) {
        scene_write_entity_json(&b, scene, entities[i], 2);
        if (i < total_count - 1) {
            json_builder_append(&b, ",");
        }
        json_builder_append(&b, "\n");
    }

    // Write light entities
    for (int i = 0; i < light_count; i++) {
        scene_write_light_json(&b, scene, light_entities[i], 2);
        if (entity_count + i < total_count - 1) {
            json_builder_append(&b, ",");
        }
        json_builder_append(&b, "\n");
    }

    free(entities);
    free(light_entities);

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

// Parse an array of uint32 (for mesh indices)
static inline bool json_parse_uint_array(json_parser_t *p, uint32_t **out_values, int *out_count) {
    if (p->token != JSON_TOK_LBRACKET) return false;

    int capacity = 16;
    int count = 0;
    uint32_t *values = (uint32_t*)malloc(capacity * sizeof(uint32_t));

    if (!json_next_token(p)) {
        free(values);
        return false;
    }

    while (p->token != JSON_TOK_RBRACKET) {
        if (p->token != JSON_TOK_NUMBER) {
            free(values);
            return false;
        }

        if (count >= capacity) {
            capacity *= 2;
            values = (uint32_t*)realloc(values, capacity * sizeof(uint32_t));
        }
        values[count++] = (uint32_t)p->num_value;

        if (!json_next_token(p)) {
            free(values);
            return false;
        }

        if (p->token == JSON_TOK_COMMA) {
            if (!json_next_token(p)) {
                free(values);
                return false;
            }
        }
    }

    if (!json_next_token(p)) {  // Skip ]
        free(values);
        return false;
    }

    *out_values = values;
    *out_count = count;
    return true;
}

// Parse an array of vec4 (for mesh vertex colors)
static inline bool json_parse_vec4_array(json_parser_t *p, vec4_t **out_colors, int *out_count) {
    if (p->token != JSON_TOK_LBRACKET) return false;

    int capacity = 16;
    int count = 0;
    vec4_t *colors = (vec4_t*)malloc(capacity * sizeof(vec4_t));

    if (!json_next_token(p)) {
        free(colors);
        return false;
    }

    while (p->token != JSON_TOK_RBRACKET) {
        if (count >= capacity) {
            capacity *= 2;
            colors = (vec4_t*)realloc(colors, capacity * sizeof(vec4_t));
        }

        if (!json_parse_vec4(p, &colors[count])) {
            free(colors);
            return false;
        }
        count++;

        if (p->token == JSON_TOK_COMMA) {
            if (!json_next_token(p)) {
                free(colors);
                return false;
            }
        }
    }

    if (!json_next_token(p)) {  // Skip ]
        free(colors);
        return false;
    }

    *out_colors = colors;
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

    bool has_transform;
    bool has_geometry;
    bool has_renderable;
    bool has_label;
    bool has_sketch;
    bool has_sketch_geometry_state;
    bool has_constraint;

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
        struct {
            vec3_t a, b, c;
            vec4_t color_a, color_b, color_c;
            bool has_vertex_colors;
        } triangle;
        struct {
            vec3_t *vertices;
            int vertex_count;
            uint32_t *indices;
            int index_count;
            vec4_t *vertex_colors;  // NULL if uniform color
        } mesh;
    } geom_data;

    // Renderable
    bool visible;
    int layer;

    // Label
    LabelComp label;

    // Sketch
    SketchComp sketch;
    SketchGeometryStateComp sketch_geometry_state;

    // Constraint
    ConstraintComp constraint;

    // Light (when is_light is true, geometry fields are unused)
    bool is_light;
    light_type_t light_type;
    vec4_t light_color;
    float light_intensity;
} loaded_entity_t;

//------------------------------------------------------------------------------
// Parse Transform Component
//------------------------------------------------------------------------------

static inline bool json_parse_transform(json_parser_t *p, loaded_entity_t *ent) {
    if (p->token != JSON_TOK_LBRACE) return false;

    // Set defaults
    ent->position = scene_ser_vec3(0.0f, 0.0f, 0.0f);
    ent->rotation = scene_ser_vec3(0.0f, 0.0f, 0.0f);
    ent->scale = scene_ser_vec3(1.0f, 1.0f, 1.0f);

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

    ent->has_transform = true;
    return json_next_token(p);  // Skip }
}

//------------------------------------------------------------------------------
// Parse Geometry Component
//------------------------------------------------------------------------------

static inline bool json_parse_geometry(json_parser_t *p, loaded_entity_t *ent) {
    if (p->token != JSON_TOK_LBRACE) return false;

    // Set defaults
    ent->geom_type = GEOM_POINT;
    ent->color = scene_ser_vec4(1.0f, 1.0f, 1.0f, 1.0f);
    ent->line_width = 0.005f;
    ent->point_size = 0.006f;

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
            if (ent->geom_type == GEOM_TRIANGLE) {
                if (!json_parse_vec3(p, &ent->geom_data.triangle.a)) return false;
            } else {
                if (!json_parse_vec3(p, &ent->geom_data.line.a)) return false;
            }
        } else if (strcmp(key, "b") == 0) {
            if (ent->geom_type == GEOM_TRIANGLE) {
                if (!json_parse_vec3(p, &ent->geom_data.triangle.b)) return false;
            } else {
                if (!json_parse_vec3(p, &ent->geom_data.line.b)) return false;
            }
        } else if (strcmp(key, "c") == 0) {
            if (!json_parse_vec3(p, &ent->geom_data.triangle.c)) return false;
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
        } else if (strcmp(key, "vertices") == 0) {
            // For mesh
            vec3_t *verts = NULL;
            int count = 0;
            if (!json_parse_vec3_array(p, &verts, &count)) return false;
            ent->geom_data.mesh.vertices = verts;
            ent->geom_data.mesh.vertex_count = count;
        } else if (strcmp(key, "indices") == 0) {
            uint32_t *idx = NULL;
            int count = 0;
            if (!json_parse_uint_array(p, &idx, &count)) return false;
            ent->geom_data.mesh.indices = idx;
            ent->geom_data.mesh.index_count = count;
        } else if (strcmp(key, "vertex_colors") == 0) {
            vec4_t *colors = NULL;
            int count = 0;
            if (!json_parse_vec4_array(p, &colors, &count)) return false;
            ent->geom_data.mesh.vertex_colors = colors;
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
        } else if (strcmp(key, "color_a") == 0) {
            if (!json_parse_vec4(p, &ent->geom_data.triangle.color_a)) return false;
            ent->geom_data.triangle.has_vertex_colors = true;
        } else if (strcmp(key, "color_b") == 0) {
            if (!json_parse_vec4(p, &ent->geom_data.triangle.color_b)) return false;
        } else if (strcmp(key, "color_c") == 0) {
            if (!json_parse_vec4(p, &ent->geom_data.triangle.color_c)) return false;
        } else {
            if (!json_skip_value(p)) return false;
        }

        if (p->token == JSON_TOK_COMMA) {
            if (!json_next_token(p)) return false;
        }
    }

    ent->has_geometry = true;
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

    ent->has_renderable = true;
    return json_next_token(p);  // Skip }
}

//------------------------------------------------------------------------------
// Parse Label Component
//------------------------------------------------------------------------------

static inline bool json_parse_label(json_parser_t *p, loaded_entity_t *ent) {
    if (p->token != JSON_TOK_LBRACE) return false;

    ent->label = label_comp_default();
    ent->has_label = true;

    if (!json_next_token(p)) return false;

    while (p->token != JSON_TOK_RBRACE) {
        if (p->token != JSON_TOK_STRING) return false;
        char key[64];
        strncpy(key, p->str_value, sizeof(key) - 1);
        key[sizeof(key) - 1] = '\0';

        if (!json_next_token(p)) return false;  // :
        if (p->token != JSON_TOK_COLON) return false;
        if (!json_next_token(p)) return false;  // value

        if (strcmp(key, "name") == 0) {
            if (p->token != JSON_TOK_STRING) return false;
            strncpy(ent->label.name, p->str_value, LABEL_NAME_MAX - 1);
            ent->label.name[LABEL_NAME_MAX - 1] = '\0';
            if (!json_next_token(p)) return false;
        } else if (strcmp(key, "description") == 0) {
            if (p->token != JSON_TOK_STRING) return false;
            strncpy(ent->label.description, p->str_value, LABEL_DESC_MAX - 1);
            ent->label.description[LABEL_DESC_MAX - 1] = '\0';
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
// Parse Sketch Component
//------------------------------------------------------------------------------

static inline bool json_parse_uint32_array_fixed(json_parser_t *p, uint32_t *out_values, int out_count) {
    if (!out_values || out_count <= 0) return false;
    if (p->token != JSON_TOK_LBRACKET) return false;

    for (int i = 0; i < out_count; i++) out_values[i] = 0;

    if (!json_next_token(p)) return false;
    int i = 0;
    while (p->token != JSON_TOK_RBRACKET) {
        if (p->token != JSON_TOK_NUMBER) return false;
        if (i < out_count) {
            out_values[i] = (uint32_t)p->num_value;
        }
        i++;

        if (!json_next_token(p)) return false;
        if (p->token == JSON_TOK_COMMA) {
            if (!json_next_token(p)) return false;
        }
    }

    return json_next_token(p);  // Skip ]
}

static inline bool json_parse_sketch(json_parser_t *p, loaded_entity_t *ent) {
    if (p->token != JSON_TOK_LBRACE) return false;

    ent->sketch = sketch_comp_default();
    ent->has_sketch = true;

    if (!json_next_token(p)) return false;

    while (p->token != JSON_TOK_RBRACE) {
        if (p->token != JSON_TOK_STRING) return false;
        char key[64];
        strncpy(key, p->str_value, sizeof(key) - 1);
        key[sizeof(key) - 1] = '\0';

        if (!json_next_token(p)) return false;  // :
        if (p->token != JSON_TOK_COLON) return false;
        if (!json_next_token(p)) return false;  // value

        if (strcmp(key, "status") == 0) {
            if (p->token != JSON_TOK_NUMBER) return false;
            int status = (int)p->num_value;
            if (status < (int)SKETCH_STATUS_SOLVED || status > (int)SKETCH_STATUS_ERROR) {
                status = (int)SKETCH_STATUS_LOOSE;
            }
            ent->sketch.status = (sketch_status_t)status;
            if (!json_next_token(p)) return false;
        } else if (strcmp(key, "color") == 0) {
            if (!json_parse_vec4(p, &ent->sketch.color)) return false;
        } else if (strcmp(key, "next_geometry_name_index") == 0) {
            if (!json_parse_uint32_array_fixed(p, ent->sketch.next_geometry_name_index, GEOM_TYPE_COUNT)) return false;
        } else if (strcmp(key, "next_constraint_name_index") == 0) {
            if (!json_parse_uint32_array_fixed(p, ent->sketch.next_constraint_name_index, CONSTRAINT_TYPE_COUNT)) return false;
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
// Parse Sketch Geometry State Component
//------------------------------------------------------------------------------

static inline bool json_parse_sketch_geometry_state(json_parser_t *p, loaded_entity_t *ent) {
    if (p->token != JSON_TOK_LBRACE) return false;

    ent->has_sketch_geometry_state = true;
    ent->sketch_geometry_state = sketch_geometry_state_comp_default();

    if (!json_next_token(p)) return false;

    while (p->token != JSON_TOK_RBRACE) {
        if (p->token != JSON_TOK_STRING) return false;
        char key[64];
        strncpy(key, p->str_value, sizeof(key) - 1);
        key[sizeof(key) - 1] = '\0';

        if (!json_next_token(p)) return false;  // :
        if (p->token != JSON_TOK_COLON) return false;
        if (!json_next_token(p)) return false;  // value

        if (strcmp(key, "fixed") == 0) {
            if (p->token != JSON_TOK_TRUE && p->token != JSON_TOK_FALSE) return false;
            ent->sketch_geometry_state.fixed = (p->token == JSON_TOK_TRUE);
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
// Parse Constraint Component
//------------------------------------------------------------------------------

static inline bool json_parse_constraint(json_parser_t *p, loaded_entity_t *ent) {
    if (p->token != JSON_TOK_LBRACE) return false;

    ent->constraint = constraint_comp_default();
    ent->has_constraint = true;
    bool has_display_decimals = false;

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
            if (p->token != JSON_TOK_NUMBER) return false;
            int type = (int)p->num_value;
            if (type < 0 || type >= CONSTRAINT_TYPE_COUNT) return false;
            ent->constraint.type = (constraint_type_t)type;
            if (!json_next_token(p)) return false;
        } else if (strcmp(key, "has_value") == 0) {
            if (p->token != JSON_TOK_TRUE && p->token != JSON_TOK_FALSE) return false;
            ent->constraint.has_value = (p->token == JSON_TOK_TRUE);
            if (!json_next_token(p)) return false;
        } else if (strcmp(key, "driven") == 0) {
            if (p->token != JSON_TOK_TRUE && p->token != JSON_TOK_FALSE) return false;
            ent->constraint.driven = (p->token == JSON_TOK_TRUE);
            if (!json_next_token(p)) return false;
        } else if (strcmp(key, "value") == 0) {
            if (p->token != JSON_TOK_NUMBER) return false;
            ent->constraint.value = (float)p->num_value;
            if (!json_next_token(p)) return false;
        } else if (strcmp(key, "display_decimals") == 0) {
            if (p->token != JSON_TOK_NUMBER) return false;
            int decimals = (int)p->num_value;
            if (decimals < 0) decimals = 0;
            if (decimals > 6) decimals = 6;
            ent->constraint.display_decimals = (uint8_t)decimals;
            has_display_decimals = true;
            if (!json_next_token(p)) return false;
        } else if (strcmp(key, "participants") == 0) {
            if (p->token != JSON_TOK_LBRACKET) return false;
            ent->constraint.participant_count = 0;
            if (!json_next_token(p)) return false;
            while (p->token != JSON_TOK_RBRACKET) {
                if (p->token != JSON_TOK_NUMBER) return false;
                if (ent->constraint.participant_count >= CONSTRAINT_MAX_PARTICIPANTS) return false;
                ent->constraint.participants[ent->constraint.participant_count++] = (uint64_t)p->num_value;
                if (!json_next_token(p)) return false;
                if (p->token == JSON_TOK_COMMA) {
                    if (!json_next_token(p)) return false;
                }
            }
            if (!json_next_token(p)) return false;  // Skip ]
        } else {
            if (!json_skip_value(p)) return false;
        }

        if (p->token == JSON_TOK_COMMA) {
            if (!json_next_token(p)) return false;
        }
    }

    if (constraint_comp_is_dimensional(&ent->constraint)) {
        if (!ent->constraint.has_value) {
            ent->constraint.display_decimals = 0;
        } else if (!has_display_decimals) {
            ent->constraint.display_decimals = constraint_value_infer_decimals(ent->constraint.value, 0);
        } else if (ent->constraint.display_decimals > 6) {
            ent->constraint.display_decimals = 6;
        }
        ent->constraint.value = constraint_round_to_decimals(ent->constraint.value, ent->constraint.display_decimals);
    }

    return json_next_token(p);  // Skip }
}

//------------------------------------------------------------------------------
// Parse Light Component
//------------------------------------------------------------------------------

static inline bool json_parse_light(json_parser_t *p, loaded_entity_t *ent) {
    if (p->token != JSON_TOK_LBRACE) return false;

    ent->is_light = true;
    ent->light_type = LIGHT_DIRECTIONAL;
    ent->light_color = scene_ser_vec4(1.0f, 1.0f, 1.0f, 1.0f);
    ent->light_intensity = 1.0f;

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
            if (p->token == JSON_TOK_STRING) {
                if (strcmp(p->str_value, "point") == 0) {
                    ent->light_type = LIGHT_POINT;
                }
            }
            if (!json_next_token(p)) return false;
        } else if (strcmp(key, "color") == 0) {
            if (!json_parse_vec4(p, &ent->light_color)) return false;
        } else if (strcmp(key, "intensity") == 0) {
            if (p->token != JSON_TOK_NUMBER) return false;
            ent->light_intensity = (float)p->num_value;
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
    ent->scale = scene_ser_vec3(1.0f, 1.0f, 1.0f);
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
                } else if (strcmp(comp_key, "label") == 0) {
                    if (!json_parse_label(p, ent)) return false;
                } else if (strcmp(comp_key, "sketch") == 0) {
                    if (!json_parse_sketch(p, ent)) return false;
                } else if (strcmp(comp_key, "sketch_geometry_state") == 0) {
                    if (!json_parse_sketch_geometry_state(p, ent)) return false;
                } else if (strcmp(comp_key, "constraint") == 0) {
                    if (!json_parse_constraint(p, ent)) return false;
                } else if (strcmp(comp_key, "light") == 0) {
                    if (!json_parse_light(p, ent)) return false;
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

    // Handle light entities
    if (ent->is_light) {
        if (ent->light_type == LIGHT_DIRECTIONAL) {
            e = scene_add_directional_light(scene, ent->position, ent->light_color, ent->light_intensity);
        } else {
            e = scene_add_point_light(scene, ent->position, ent->light_color, ent->light_intensity);
        }
        return e;
    }

    // Handle sketch anchor entities (non-geometry)
    if (ent->has_sketch && !ent->has_geometry && !ent->has_constraint) {
        const char *label_name = (ent->has_label && ent->label.name[0] != '\0') ? ent->label.name : "Sketch";
        const char *label_desc = ent->has_label ? ent->label.description : "";
        e = scene_add_anchor(scene, label_name, label_desc);
        if (e == 0) return 0;

        ecs_world_set_sketch(scene->world, e, &ent->sketch);

        TransformComp *t = ecs_world_get_transform(scene->world, e);
        if (t) {
            t->position = ent->position;
            t->rotation = ent->rotation;
            t->scale = ent->scale;
            t->dirty = true;
        }
        return e;
    }

    // Handle constraint anchor entities (non-geometry)
    if (ent->has_constraint && !ent->has_geometry) {
        const char *label_name = (ent->has_label && ent->label.name[0] != '\0') ? ent->label.name : "";
        const char *label_desc = ent->has_label ? ent->label.description : "";
        e = scene_add_anchor(scene, label_name, label_desc);
        if (e == 0) return 0;

        ecs_world_set_constraint(scene->world, e, &ent->constraint);

        TransformComp *t = ecs_world_get_transform(scene->world, e);
        if (t) {
            t->position = ent->position;
            t->rotation = ent->rotation;
            t->scale = ent->scale;
            t->dirty = true;
        }
        return e;
    }

    if (!ent->has_geometry) {
        return 0;
    }

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

        case GEOM_TRIANGLE:
            if (ent->geom_data.triangle.has_vertex_colors) {
                e = scene_add_triangle_colored(scene,
                    ent->geom_data.triangle.a, ent->geom_data.triangle.b,
                    ent->geom_data.triangle.c,
                    ent->geom_data.triangle.color_a, ent->geom_data.triangle.color_b,
                    ent->geom_data.triangle.color_c);
            } else {
                e = scene_add_triangle(scene,
                    ent->geom_data.triangle.a, ent->geom_data.triangle.b,
                    ent->geom_data.triangle.c, ent->color);
            }
            break;

        case GEOM_MESH:
            if (ent->geom_data.mesh.vertices && ent->geom_data.mesh.indices &&
                ent->geom_data.mesh.vertex_count >= 3 && ent->geom_data.mesh.index_count >= 3) {
                if (ent->geom_data.mesh.vertex_colors) {
                    e = scene_add_mesh_colored(scene,
                        ent->geom_data.mesh.vertices, ent->geom_data.mesh.vertex_count,
                        ent->geom_data.mesh.indices, ent->geom_data.mesh.index_count,
                        ent->geom_data.mesh.vertex_colors);
                } else {
                    e = scene_add_mesh(scene,
                        ent->geom_data.mesh.vertices, ent->geom_data.mesh.vertex_count,
                        ent->geom_data.mesh.indices, ent->geom_data.mesh.index_count,
                        ent->color);
                }
            }
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

        if (ent->has_label) {
            ecs_world_set_label(scene->world, e, &ent->label);
        }
        if (ent->has_sketch_geometry_state) {
            ecs_world_set_sketch_geometry_state(scene->world, e, &ent->sketch_geometry_state);
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
    bool has_format = false;
    bool has_version = false;
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

        if (strcmp(key, "format") == 0) {
            if (p.token != JSON_TOK_STRING) { free(entities); return -1; }
            if (strcmp(p.str_value, SCENE_JSON_FORMAT) != 0) { free(entities); return -1; }
            has_format = true;
            if (!json_next_token(&p)) { free(entities); return -1; }
        } else if (strcmp(key, "version") == 0) {
            if (p.token != JSON_TOK_NUMBER) { free(entities); return -1; }
            version = (int)p.num_value;
            has_version = true;
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
                        } else if (entities[i].geom_type == GEOM_MESH) {
                            if (entities[i].geom_data.mesh.vertices) free(entities[i].geom_data.mesh.vertices);
                            if (entities[i].geom_data.mesh.indices) free(entities[i].geom_data.mesh.indices);
                            if (entities[i].geom_data.mesh.vertex_colors) free(entities[i].geom_data.mesh.vertex_colors);
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

    // Check version (allow loading older scene versions for backward compatibility).
    if (!has_format || !has_version || version <= 0 || version > SCENE_JSON_VERSION) {
        free(entities);
        return -1;
    }

    // Clear existing scene if requested
    if (clear_existing) {
        ecs_world_state_t *w = scene->world;

        ecs_entity_t *to_delete = NULL;
        int delete_count = 0;
        int delete_capacity = 0;
        ecs_entity_t clear_components[3] = {
            w->GeometryComp_id,
            w->SketchComp_id,
            w->ConstraintComp_id
        };

        // Collect all persisted non-light entities (unique) and delete via scene API.
        for (int c = 0; c < 3; c++) {
            ecs_query_t *q = ecs_query(w->world, {
                .terms = {{ .id = clear_components[c] }}
            });

            ecs_iter_t it = ecs_query_iter(w->world, q);
            while (ecs_query_next(&it)) {
                for (int i = 0; i < it.count; i++) {
                    scene_entity_list_push_unique(&to_delete, &delete_count, &delete_capacity, it.entities[i]);
                }
            }
            ecs_query_fini(q);
        }

        for (int i = 0; i < delete_count; i++) {
            scene_remove_entity(scene, to_delete[i]);
        }
        free(to_delete);

        // Delete all lights.
        ecs_query_t *lq = ecs_query(w->world, {
            .terms = {{ .id = w->LightComp_id }}
        });

        ecs_entity_t *lights_to_delete = NULL;
        int ld_count = 0;
        int ld_capacity = 0;

        ecs_iter_t lit = ecs_query_iter(w->world, lq);
        while (ecs_query_next(&lit)) {
            for (int i = 0; i < lit.count; i++) {
                if (ld_count >= ld_capacity) {
                    ld_capacity = ld_capacity ? ld_capacity * 2 : 8;
                    lights_to_delete = (ecs_entity_t*)realloc(lights_to_delete, ld_capacity * sizeof(ecs_entity_t));
                }
                lights_to_delete[ld_count++] = lit.entities[i];
            }
        }
        ecs_query_fini(lq);

        for (int i = 0; i < ld_count; i++) {
            ecs_delete(w->world, lights_to_delete[i]);
        }
        free(lights_to_delete);
    }

    // Create entities (first pass - create all entities)
    for (int i = 0; i < entity_count; i++) {
        entities[i].new_entity = scene_create_from_loaded(scene, &entities[i]);
    }

    // Set up parent-child relationships (second pass - batch for O(n) perf)
    {
        ecs_entity_t *load_children = (ecs_entity_t*)malloc(entity_count * sizeof(ecs_entity_t));
        ecs_entity_t *load_parents = (ecs_entity_t*)malloc(entity_count * sizeof(ecs_entity_t));
        int parent_count = 0;

        for (int i = 0; i < entity_count; i++) {
            if (entities[i].parent_old_id != 0 && entities[i].new_entity != 0) {
                ecs_entity_t new_parent = scene_find_new_entity(entities, entity_count,
                                                                 entities[i].parent_old_id);
                if (new_parent != 0) {
                    load_children[parent_count] = entities[i].new_entity;
                    load_parents[parent_count] = new_parent;
                    parent_count++;
                }
            }
        }

        if (parent_count > 0) {
            scene_set_parents_batch(scene, load_children, load_parents, parent_count);
        }
        free(load_children);
        free(load_parents);
    }

    // Rebuild participant back-references on geometry after all entities exist.
    // This guarantees ConstraintParticipantComp consistency regardless of load order.
    for (int i = 0; i < entity_count; i++) {
        if (!entities[i].has_constraint || entities[i].new_entity == 0) continue;
        ConstraintComp *constraint = ecs_world_get_constraint(scene->world, entities[i].new_entity);
        if (!constraint) continue;
        for (uint32_t p_idx = 0; p_idx < constraint->participant_count; p_idx++) {
            ecs_entity_t participant = scene_find_new_entity(entities, entity_count, constraint->participants[p_idx]);
            if (participant == 0) continue;
            ConstraintParticipantComp *refs = ecs_world_get_constraint_participant(scene->world, participant);
            if (!refs) {
                ConstraintParticipantComp init_refs = constraint_participant_comp_default();
                ecs_world_set_constraint_participant(scene->world, participant, &init_refs);
                refs = ecs_world_get_constraint_participant(scene->world, participant);
            }
            if (refs) {
                constraint_participant_add(refs, (uint64_t)entities[i].new_entity);
                // Rewrite participants to current entity IDs so scene state is fully normalized.
                constraint->participants[p_idx] = (uint64_t)participant;
            }
        }
    }

    // Recompute sketch metadata after hierarchy + constraints are restored.
    for (int i = 0; i < entity_count; i++) {
        if (!entities[i].has_sketch || entities[i].new_entity == 0) continue;
        scene_refresh_sketch_metadata(scene, entities[i].new_entity);
    }

    // Free loaded entity data
    for (int i = 0; i < entity_count; i++) {
        if (entities[i].geom_type == GEOM_POLYLINE && entities[i].geom_data.polyline.points) {
            free(entities[i].geom_data.polyline.points);
        } else if (entities[i].geom_type == GEOM_POLYGON && entities[i].geom_data.polygon.points) {
            free(entities[i].geom_data.polygon.points);
        } else if (entities[i].geom_type == GEOM_MESH) {
            if (entities[i].geom_data.mesh.vertices) free(entities[i].geom_data.mesh.vertices);
            if (entities[i].geom_data.mesh.indices) free(entities[i].geom_data.mesh.indices);
            if (entities[i].geom_data.mesh.vertex_colors) free(entities[i].geom_data.mesh.vertex_colors);
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
