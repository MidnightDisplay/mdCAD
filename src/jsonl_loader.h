//------------------------------------------------------------------------------
// jsonl_loader.h - JSONL geometry log parser (header-only)
//
// Parses .jsonl files containing geometry log entries from .NET geometry libs.
// Each line is a JSON object with Name, Elements[] containing typed geometry.
// Supports: Point3D, Line3D, Arc3D, PolyLine3D, Polygon3D.
//
// Uses cJSON for JSON parsing. Parser matches only the class name suffix
// (e.g., "Point3D") and does NOT rely on hardcoded .NET namespaces.
//------------------------------------------------------------------------------
#ifndef JSONL_LOADER_H
#define JSONL_LOADER_H

#include "math3d.h"
#include "components/component_types.h"
#include "cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

//------------------------------------------------------------------------------
// Error codes
//------------------------------------------------------------------------------

typedef enum {
    JSONL_OK = 0,
    JSONL_ERROR_FILE_NOT_FOUND,
    JSONL_ERROR_PARSE_ERROR,
    JSONL_ERROR_MEMORY_ALLOCATION,
} jsonl_error_t;

static inline const char* jsonl_error_string(jsonl_error_t err) {
    switch (err) {
        case JSONL_OK:                      return "OK";
        case JSONL_ERROR_FILE_NOT_FOUND:    return "File not found";
        case JSONL_ERROR_PARSE_ERROR:       return "Parse error";
        case JSONL_ERROR_MEMORY_ALLOCATION: return "Memory allocation failed";
        default:                            return "Unknown error";
    }
}

//------------------------------------------------------------------------------
// Geometry element types
//------------------------------------------------------------------------------

typedef enum {
    JSONL_GEOM_POINT,
    JSONL_GEOM_LINE,
    JSONL_GEOM_ARC,
    JSONL_GEOM_POLYLINE,
    JSONL_GEOM_POLYGON,
    JSONL_GEOM_UNKNOWN,
} jsonl_geom_type_t;

//------------------------------------------------------------------------------
// Parsed geometry element
//------------------------------------------------------------------------------

typedef struct {
    jsonl_geom_type_t type;
    char name[128];
    char description[256];
    vec4_t colour;          // RGBA, alpha always 1.0

    union {
        struct { vec3_t point; } point;
        struct { vec3_t start, end; } line;
        struct {
            vec3_t center;
            float radius;
            vec3_t normal;
            float start_angle;
            float end_angle;
        } arc;
        struct {
            vec3_t *points;
            int count;
        } polyline;  // also used for polygon
    } data;
} jsonl_element_t;

//------------------------------------------------------------------------------
// One log entry (one JSONL line)
//------------------------------------------------------------------------------

typedef struct {
    char name[128];
    char description[256];
    jsonl_element_t *elements;
    int element_count;
    int element_capacity;
} jsonl_log_entry_t;

//------------------------------------------------------------------------------
// Complete parsed file
//------------------------------------------------------------------------------

typedef struct {
    jsonl_log_entry_t *entries;
    int entry_count;
    int entry_capacity;
    int total_elements;     // sum of all elements across entries
} jsonl_data_t;

//------------------------------------------------------------------------------
// Incremental parse state
//------------------------------------------------------------------------------

typedef struct {
    FILE *file;
    jsonl_data_t data;
    int lines_parsed;
    int total_lines;        // counted in first pass
    jsonl_error_t error;
} jsonl_parse_state_t;

//------------------------------------------------------------------------------
// Internal: Extract class name suffix from $type string
// e.g., "Geo.NET.Geometry.Line3D, Geo.NET Core" -> "Line3D"
//------------------------------------------------------------------------------

static inline const char* jsonl_extract_class_name(const char *type_str) {
    static char class_buf[128];
    if (!type_str) return NULL;

    // Find the comma (separates class from assembly)
    const char *comma = strchr(type_str, ',');
    const char *search_end = comma ? comma : type_str + strlen(type_str);

    // Find the last dot before the comma (or end)
    const char *last_dot = NULL;
    for (const char *p = type_str; p < search_end; p++) {
        if (*p == '.') last_dot = p;
    }

    const char *start = last_dot ? (last_dot + 1) : type_str;
    size_t len = (size_t)(search_end - start);
    if (len >= sizeof(class_buf)) len = sizeof(class_buf) - 1;
    memcpy(class_buf, start, len);
    class_buf[len] = '\0';
    return class_buf;
}

//------------------------------------------------------------------------------
// Internal: Parse Point3D from cJSON object
//------------------------------------------------------------------------------

static inline vec3_t jsonl_parse_point3d(const cJSON *json) {
    vec3_t p = {0, 0, 0};
    if (!json) return p;

    cJSON *x = cJSON_GetObjectItemCaseSensitive(json, "X");
    cJSON *y = cJSON_GetObjectItemCaseSensitive(json, "Y");
    cJSON *z = cJSON_GetObjectItemCaseSensitive(json, "Z");

    if (x && cJSON_IsNumber(x)) p.x = (float)x->valuedouble;
    if (y && cJSON_IsNumber(y)) p.y = (float)y->valuedouble;
    if (z && cJSON_IsNumber(z)) p.z = (float)z->valuedouble;

    return p;
}

//------------------------------------------------------------------------------
// Internal: Parse Vector3D from cJSON object (I, J, K)
//------------------------------------------------------------------------------

static inline vec3_t jsonl_parse_vector3d(const cJSON *json) {
    vec3_t v = {0, 0, 0};
    if (!json) return v;

    cJSON *i = cJSON_GetObjectItemCaseSensitive(json, "I");
    cJSON *j = cJSON_GetObjectItemCaseSensitive(json, "J");
    cJSON *k = cJSON_GetObjectItemCaseSensitive(json, "K");

    if (i && cJSON_IsNumber(i)) v.x = (float)i->valuedouble;
    if (j && cJSON_IsNumber(j)) v.y = (float)j->valuedouble;
    if (k && cJSON_IsNumber(k)) v.z = (float)k->valuedouble;

    return v;
}

//------------------------------------------------------------------------------
// Internal: Parse colour string "R, G, B" or "R, G, B, A" -> vec4_t
// Alpha always defaults to 1.0 regardless of file value.
//------------------------------------------------------------------------------

static inline vec4_t jsonl_parse_colour(const char *str) {
    vec4_t c = {1.0f, 1.0f, 1.0f, 1.0f};
    if (!str) return c;

    int r = 255, g = 255, b = 255;
    // Try parsing R, G, B, A first, then R, G, B
    int a_unused = 0;
    int parsed = sscanf(str, "%d, %d, %d, %d", &r, &g, &b, &a_unused);
    if (parsed < 3) {
        // Try without spaces after commas
        parsed = sscanf(str, "%d,%d,%d,%d", &r, &g, &b, &a_unused);
    }
    if (parsed >= 3) {
        c.x = (float)r / 255.0f;
        c.y = (float)g / 255.0f;
        c.z = (float)b / 255.0f;
        c.w = 1.0f;  // Alpha always 1.0
    }

    return c;
}

//------------------------------------------------------------------------------
// Internal: Parse Arc3D from cJSON object
//------------------------------------------------------------------------------

static inline bool jsonl_parse_arc3d(const cJSON *json, jsonl_element_t *element) {
    cJSON *circle = cJSON_GetObjectItemCaseSensitive(json, "Circle");
    cJSON *start_pt = cJSON_GetObjectItemCaseSensitive(json, "StartPoint");
    cJSON *end_pt = cJSON_GetObjectItemCaseSensitive(json, "EndPoint");
    cJSON *direction = cJSON_GetObjectItemCaseSensitive(json, "Direction");

    if (!circle || !start_pt || !end_pt) return false;

    // Extract Circle properties
    cJSON *center_json = cJSON_GetObjectItemCaseSensitive(circle, "Center");
    cJSON *radius_json = cJSON_GetObjectItemCaseSensitive(circle, "Radius");
    cJSON *axis_json = cJSON_GetObjectItemCaseSensitive(circle, "Axis");

    if (!center_json || !radius_json || !axis_json) return false;

    vec3_t center = jsonl_parse_point3d(center_json);
    float radius = (float)radius_json->valuedouble;
    vec3_t axis = jsonl_parse_vector3d(axis_json);
    vec3_t normal = vec3_normalize(axis);

    vec3_t sp = jsonl_parse_point3d(start_pt);
    vec3_t ep = jsonl_parse_point3d(end_pt);
    int dir = (direction && cJSON_IsNumber(direction)) ? (int)direction->valuedouble : 1;

    // Build local coordinate system from normal (same method as ecs_scene_tessellate_arc)
    vec3_t up = normal;
    vec3_t arbitrary = (fabsf(up.y) < 0.9f) ? vec3_make(0, 1, 0) : vec3_make(1, 0, 0);
    vec3_t x_axis = vec3_normalize(vec3_cross(arbitrary, up));
    vec3_t y_axis = vec3_cross(up, x_axis);

    // Compute angles
    vec3_t sp_rel = vec3_sub(sp, center);
    vec3_t ep_rel = vec3_sub(ep, center);

    float start_angle = atan2f(vec3_dot(sp_rel, y_axis), vec3_dot(sp_rel, x_axis));
    float end_angle = atan2f(vec3_dot(ep_rel, y_axis), vec3_dot(ep_rel, x_axis));

    // Direction handling
    float pi2 = 2.0f * 3.14159265359f;
    if (dir >= 0) {
        // CCW: if end_angle <= start_angle, add 2*PI to end_angle
        if (end_angle <= start_angle) {
            end_angle += pi2;
        }
    } else {
        // CW: if end_angle >= start_angle, subtract 2*PI from end_angle
        if (end_angle >= start_angle) {
            end_angle -= pi2;
        }
    }

    element->type = JSONL_GEOM_ARC;
    element->data.arc.center = center;
    element->data.arc.radius = radius;
    element->data.arc.normal = normal;
    element->data.arc.start_angle = start_angle;
    element->data.arc.end_angle = end_angle;

    return true;
}

//------------------------------------------------------------------------------
// Internal: Parse a single geometry element from cJSON
//------------------------------------------------------------------------------

static inline bool jsonl_parse_element(const cJSON *elem_json, jsonl_element_t *element) {
    memset(element, 0, sizeof(jsonl_element_t));
    element->type = JSONL_GEOM_UNKNOWN;
    element->colour = vec4_make(1.0f, 1.0f, 1.0f, 1.0f);

    // Get name
    cJSON *name = cJSON_GetObjectItemCaseSensitive(elem_json, "Name");
    if (name && cJSON_IsString(name)) {
        strncpy(element->name, name->valuestring, sizeof(element->name) - 1);
        element->name[sizeof(element->name) - 1] = '\0';
    }

    // Get description
    cJSON *desc = cJSON_GetObjectItemCaseSensitive(elem_json, "Description");
    if (desc && cJSON_IsString(desc)) {
        strncpy(element->description, desc->valuestring, sizeof(element->description) - 1);
        element->description[sizeof(element->description) - 1] = '\0';
    }

    // Get colour
    cJSON *colour = cJSON_GetObjectItemCaseSensitive(elem_json, "Colour");
    if (colour && cJSON_IsString(colour)) {
        element->colour = jsonl_parse_colour(colour->valuestring);
    }

    // Get the Element object (the actual geometry)
    cJSON *geom = cJSON_GetObjectItemCaseSensitive(elem_json, "Element");
    if (!geom) return false;

    // Get $type to determine geometry type
    cJSON *type_field = cJSON_GetObjectItemCaseSensitive(geom, "$type");
    if (!type_field || !cJSON_IsString(type_field)) return false;

    const char *class_name = jsonl_extract_class_name(type_field->valuestring);
    if (!class_name) return false;

    // Dispatch based on class name suffix
    if (strcmp(class_name, "Point3D") == 0) {
        element->type = JSONL_GEOM_POINT;
        element->data.point.point = jsonl_parse_point3d(geom);
        return true;
    }
    else if (strcmp(class_name, "Line3D") == 0) {
        cJSON *sp = cJSON_GetObjectItemCaseSensitive(geom, "StartPoint");
        cJSON *ep = cJSON_GetObjectItemCaseSensitive(geom, "EndPoint");
        if (!sp || !ep) return false;

        element->type = JSONL_GEOM_LINE;
        element->data.line.start = jsonl_parse_point3d(sp);
        element->data.line.end = jsonl_parse_point3d(ep);
        return true;
    }
    else if (strcmp(class_name, "Arc3D") == 0) {
        return jsonl_parse_arc3d(geom, element);
    }
    else if (strcmp(class_name, "PolyLine3D") == 0) {
        cJSON *points_arr = cJSON_GetObjectItemCaseSensitive(geom, "Points");
        if (!points_arr) points_arr = cJSON_GetObjectItemCaseSensitive(geom, "points");
        if (!points_arr || !cJSON_IsArray(points_arr)) return false;

        int count = cJSON_GetArraySize(points_arr);
        if (count < 2) return false;

        element->type = JSONL_GEOM_POLYLINE;
        element->data.polyline.points = (vec3_t*)malloc(sizeof(vec3_t) * count);
        if (!element->data.polyline.points) return false;
        element->data.polyline.count = count;

        int idx = 0;
        cJSON *pt;
        cJSON_ArrayForEach(pt, points_arr) {
            element->data.polyline.points[idx++] = jsonl_parse_point3d(pt);
        }
        return true;
    }
    else if (strcmp(class_name, "Polygon3D") == 0) {
        cJSON *points_arr = cJSON_GetObjectItemCaseSensitive(geom, "Points");
        if (!points_arr) points_arr = cJSON_GetObjectItemCaseSensitive(geom, "points");
        if (!points_arr || !cJSON_IsArray(points_arr)) return false;

        int count = cJSON_GetArraySize(points_arr);
        if (count < 3) return false;

        element->type = JSONL_GEOM_POLYGON;
        element->data.polyline.points = (vec3_t*)malloc(sizeof(vec3_t) * count);
        if (!element->data.polyline.points) return false;
        element->data.polyline.count = count;

        int idx = 0;
        cJSON *pt;
        cJSON_ArrayForEach(pt, points_arr) {
            element->data.polyline.points[idx++] = jsonl_parse_point3d(pt);
        }
        return true;
    }

    // Unknown type - skip
    return false;
}

//------------------------------------------------------------------------------
// Internal: Parse one JSONL line into a log entry
//------------------------------------------------------------------------------

static inline bool jsonl_parse_entry(const char *json_str, jsonl_log_entry_t *entry) {
    memset(entry, 0, sizeof(jsonl_log_entry_t));

    cJSON *root = cJSON_Parse(json_str);
    if (!root) return false;

    // Get entry name
    cJSON *name = cJSON_GetObjectItemCaseSensitive(root, "Name");
    if (name && cJSON_IsString(name)) {
        strncpy(entry->name, name->valuestring, sizeof(entry->name) - 1);
        entry->name[sizeof(entry->name) - 1] = '\0';
    }

    // Get entry description
    cJSON *desc = cJSON_GetObjectItemCaseSensitive(root, "Description");
    if (desc && cJSON_IsString(desc)) {
        strncpy(entry->description, desc->valuestring, sizeof(entry->description) - 1);
        entry->description[sizeof(entry->description) - 1] = '\0';
    }

    // Get elements array
    cJSON *elements = cJSON_GetObjectItemCaseSensitive(root, "Elements");
    if (!elements || !cJSON_IsArray(elements)) {
        cJSON_Delete(root);
        return false;
    }

    int count = cJSON_GetArraySize(elements);
    if (count > 0) {
        entry->element_capacity = count;
        entry->elements = (jsonl_element_t*)malloc(sizeof(jsonl_element_t) * count);
        if (!entry->elements) {
            cJSON_Delete(root);
            return false;
        }
    }

    cJSON *elem;
    cJSON_ArrayForEach(elem, elements) {
        jsonl_element_t parsed;
        if (jsonl_parse_element(elem, &parsed)) {
            entry->elements[entry->element_count++] = parsed;
        }
    }

    cJSON_Delete(root);
    return true;
}

//------------------------------------------------------------------------------
// Count lines in file (for progress tracking)
//------------------------------------------------------------------------------

static inline int jsonl_count_lines(const char *filepath) {
    FILE *f = fopen(filepath, "r");
    if (!f) return -1;

    int count = 0;
    int ch;
    bool in_line = false;
    while ((ch = fgetc(f)) != EOF) {
        if (ch == '\n') {
            if (in_line) count++;
            in_line = false;
        } else if (ch != '\r' && ch != ' ' && ch != '\t') {
            in_line = true;
        }
    }
    if (in_line) count++;  // Last line without newline

    fclose(f);
    return count;
}

//------------------------------------------------------------------------------
// Quick scan: count entries and total elements without full parse
//------------------------------------------------------------------------------

static inline jsonl_error_t jsonl_quick_scan(const char *filepath,
                                              int *out_entry_count,
                                              int *out_element_count) {
    FILE *f = fopen(filepath, "r");
    if (!f) return JSONL_ERROR_FILE_NOT_FOUND;

    *out_entry_count = 0;
    *out_element_count = 0;

    char *line_buf = (char*)malloc(1024 * 1024);  // 1MB line buffer
    if (!line_buf) {
        fclose(f);
        return JSONL_ERROR_MEMORY_ALLOCATION;
    }

    while (fgets(line_buf, 1024 * 1024, f)) {
        // Skip empty lines
        char *p = line_buf;
        while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
        if (*p == '\0') continue;

        // Quick parse just to count elements
        cJSON *root = cJSON_Parse(p);
        if (root) {
            (*out_entry_count)++;
            cJSON *elements = cJSON_GetObjectItemCaseSensitive(root, "Elements");
            if (elements && cJSON_IsArray(elements)) {
                *out_element_count += cJSON_GetArraySize(elements);
            }
            cJSON_Delete(root);
        }
    }

    free(line_buf);
    fclose(f);
    return JSONL_OK;
}

//------------------------------------------------------------------------------
// Open file for incremental parsing
//------------------------------------------------------------------------------

static inline jsonl_error_t jsonl_open(const char *filepath, jsonl_parse_state_t *state) {
    memset(state, 0, sizeof(jsonl_parse_state_t));

    // Count lines first for progress tracking
    state->total_lines = jsonl_count_lines(filepath);
    if (state->total_lines < 0) {
        state->error = JSONL_ERROR_FILE_NOT_FOUND;
        return state->error;
    }

    state->file = fopen(filepath, "r");
    if (!state->file) {
        state->error = JSONL_ERROR_FILE_NOT_FOUND;
        return state->error;
    }

    // Initialize data with reasonable capacity
    state->data.entry_capacity = (state->total_lines > 0) ? state->total_lines : 16;
    state->data.entries = (jsonl_log_entry_t*)malloc(
        sizeof(jsonl_log_entry_t) * state->data.entry_capacity);
    if (!state->data.entries) {
        fclose(state->file);
        state->file = NULL;
        state->error = JSONL_ERROR_MEMORY_ALLOCATION;
        return state->error;
    }

    state->error = JSONL_OK;
    return JSONL_OK;
}

//------------------------------------------------------------------------------
// Parse N lines per call (incremental)
//------------------------------------------------------------------------------

static inline int jsonl_parse_lines_chunk(jsonl_parse_state_t *state, int max_lines) {
    if (!state->file || state->error != JSONL_OK) return 0;

    // Allocate line buffer (JSONL lines can be very long)
    char *line_buf = (char*)malloc(4 * 1024 * 1024);  // 4MB line buffer
    if (!line_buf) {
        state->error = JSONL_ERROR_MEMORY_ALLOCATION;
        return 0;
    }

    int parsed = 0;
    while (parsed < max_lines && fgets(line_buf, 4 * 1024 * 1024, state->file)) {
        // Skip empty lines
        char *p = line_buf;
        while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
        if (*p == '\0') continue;

        // Grow entries array if needed
        if (state->data.entry_count >= state->data.entry_capacity) {
            state->data.entry_capacity *= 2;
            state->data.entries = (jsonl_log_entry_t*)realloc(
                state->data.entries,
                sizeof(jsonl_log_entry_t) * state->data.entry_capacity);
            if (!state->data.entries) {
                state->error = JSONL_ERROR_MEMORY_ALLOCATION;
                free(line_buf);
                return parsed;
            }
        }

        // Parse this line
        jsonl_log_entry_t entry;
        if (jsonl_parse_entry(p, &entry)) {
            state->data.entries[state->data.entry_count] = entry;
            state->data.total_elements += entry.element_count;
            state->data.entry_count++;
        }

        state->lines_parsed++;
        parsed++;
    }

    free(line_buf);
    return parsed;
}

//------------------------------------------------------------------------------
// Progress and completion
//------------------------------------------------------------------------------

static inline float jsonl_get_progress(const jsonl_parse_state_t *state) {
    if (state->total_lines <= 0) return 1.0f;
    return (float)state->lines_parsed / (float)state->total_lines;
}

static inline bool jsonl_is_complete(const jsonl_parse_state_t *state) {
    if (!state->file) return true;
    return feof(state->file) != 0;
}

//------------------------------------------------------------------------------
// Close file (keep parsed data)
//------------------------------------------------------------------------------

static inline void jsonl_close(jsonl_parse_state_t *state) {
    if (state->file) {
        fclose(state->file);
        state->file = NULL;
    }
}

//------------------------------------------------------------------------------
// Free parsed data
//------------------------------------------------------------------------------

static inline void jsonl_data_free(jsonl_data_t *data) {
    if (data->entries) {
        for (int i = 0; i < data->entry_count; i++) {
            jsonl_log_entry_t *entry = &data->entries[i];
            if (entry->elements) {
                // Free dynamically allocated point arrays in polyline/polygon elements
                for (int j = 0; j < entry->element_count; j++) {
                    jsonl_element_t *elem = &entry->elements[j];
                    if ((elem->type == JSONL_GEOM_POLYLINE || elem->type == JSONL_GEOM_POLYGON) &&
                        elem->data.polyline.points) {
                        free(elem->data.polyline.points);
                    }
                }
                free(entry->elements);
            }
        }
        free(data->entries);
        data->entries = NULL;
    }
    data->entry_count = 0;
    data->entry_capacity = 0;
    data->total_elements = 0;
}

//------------------------------------------------------------------------------
// Free everything including file handle
//------------------------------------------------------------------------------

static inline void jsonl_parse_state_free(jsonl_parse_state_t *state) {
    jsonl_close(state);
    jsonl_data_free(&state->data);
    memset(state, 0, sizeof(jsonl_parse_state_t));
}

#endif // JSONL_LOADER_H
