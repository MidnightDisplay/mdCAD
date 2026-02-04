//------------------------------------------------------------------------------
// ply_loader.h - PLY point cloud file loader (header-only)
//
// Supports ASCII PLY format with:
// - Vertex positions (x, y, z) - required
// - Vertex colors (red, green, blue, alpha) - optional
// - Unit conversion via scale factor
//------------------------------------------------------------------------------
#ifndef PLY_LOADER_H
#define PLY_LOADER_H

#include "math3d.h"
#include "components/component_types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

//------------------------------------------------------------------------------
// Configuration
//------------------------------------------------------------------------------

#define PLY_MAX_LINE_LENGTH 1024
#define PLY_MAX_PROPERTIES 32

//------------------------------------------------------------------------------
// Error codes
//------------------------------------------------------------------------------

typedef enum {
    PLY_OK = 0,
    PLY_ERROR_FILE_NOT_FOUND,
    PLY_ERROR_INVALID_HEADER,
    PLY_ERROR_UNSUPPORTED_FORMAT,
    PLY_ERROR_MISSING_VERTEX_ELEMENT,
    PLY_ERROR_MISSING_POSITION_PROPERTIES,
    PLY_ERROR_MEMORY_ALLOCATION,
    PLY_ERROR_PARSE_ERROR,
} ply_error_t;

//------------------------------------------------------------------------------
// Property types
//------------------------------------------------------------------------------

typedef enum {
    PLY_PROP_NONE = 0,
    PLY_PROP_CHAR,
    PLY_PROP_UCHAR,
    PLY_PROP_SHORT,
    PLY_PROP_USHORT,
    PLY_PROP_INT,
    PLY_PROP_UINT,
    PLY_PROP_FLOAT,
    PLY_PROP_DOUBLE,
} ply_property_type_t;

typedef struct {
    char name[64];
    ply_property_type_t type;
} ply_property_t;

//------------------------------------------------------------------------------
// PLY header info
//------------------------------------------------------------------------------

typedef struct {
    bool is_ascii;
    bool is_little_endian;  // For binary formats (future)
    int vertex_count;

    // Property indices (-1 if not present)
    int prop_x;
    int prop_y;
    int prop_z;
    int prop_red;
    int prop_green;
    int prop_blue;
    int prop_alpha;

    // All properties
    ply_property_t properties[PLY_MAX_PROPERTIES];
    int property_count;

    // Header end position
    long data_start_pos;
} ply_header_t;

//------------------------------------------------------------------------------
// Loaded PLY data
//------------------------------------------------------------------------------

typedef struct {
    vec3_t *points;      // Dynamically allocated array of positions
    vec4_t *colors;      // Dynamically allocated array of colors (NULL if no colors)
    int count;           // Number of points
    bool has_colors;     // Whether PLY file had color data

    // Bounding box (computed during load)
    vec3_t min_bounds;
    vec3_t max_bounds;
} ply_data_t;

//------------------------------------------------------------------------------
// Helper: Parse property type from string
//------------------------------------------------------------------------------

static inline ply_property_type_t ply_parse_property_type(const char *type_str) {
    if (strcmp(type_str, "char") == 0 || strcmp(type_str, "int8") == 0) return PLY_PROP_CHAR;
    if (strcmp(type_str, "uchar") == 0 || strcmp(type_str, "uint8") == 0) return PLY_PROP_UCHAR;
    if (strcmp(type_str, "short") == 0 || strcmp(type_str, "int16") == 0) return PLY_PROP_SHORT;
    if (strcmp(type_str, "ushort") == 0 || strcmp(type_str, "uint16") == 0) return PLY_PROP_USHORT;
    if (strcmp(type_str, "int") == 0 || strcmp(type_str, "int32") == 0) return PLY_PROP_INT;
    if (strcmp(type_str, "uint") == 0 || strcmp(type_str, "uint32") == 0) return PLY_PROP_UINT;
    if (strcmp(type_str, "float") == 0 || strcmp(type_str, "float32") == 0) return PLY_PROP_FLOAT;
    if (strcmp(type_str, "double") == 0 || strcmp(type_str, "float64") == 0) return PLY_PROP_DOUBLE;
    return PLY_PROP_NONE;
}

//------------------------------------------------------------------------------
// Helper: Trim whitespace from string end
//------------------------------------------------------------------------------

static inline void ply_trim_end(char *str) {
    int len = (int)strlen(str);
    while (len > 0 && (str[len-1] == '\n' || str[len-1] == '\r' || str[len-1] == ' ')) {
        str[--len] = '\0';
    }
}

//------------------------------------------------------------------------------
// Parse PLY header
//------------------------------------------------------------------------------

static inline ply_error_t ply_parse_header(FILE *file, ply_header_t *header) {
    char line[PLY_MAX_LINE_LENGTH];

    // Initialize header
    memset(header, 0, sizeof(ply_header_t));
    header->prop_x = -1;
    header->prop_y = -1;
    header->prop_z = -1;
    header->prop_red = -1;
    header->prop_green = -1;
    header->prop_blue = -1;
    header->prop_alpha = -1;

    // Check magic number
    if (!fgets(line, sizeof(line), file)) return PLY_ERROR_INVALID_HEADER;
    ply_trim_end(line);
    if (strcmp(line, "ply") != 0) return PLY_ERROR_INVALID_HEADER;

    bool in_vertex_element = false;

    // Parse header lines
    while (fgets(line, sizeof(line), file)) {
        ply_trim_end(line);

        // Skip empty lines and comments
        if (line[0] == '\0') continue;
        if (strncmp(line, "comment", 7) == 0) continue;

        // Check for end_header
        if (strcmp(line, "end_header") == 0) {
            header->data_start_pos = ftell(file);
            break;
        }

        // Parse format
        if (strncmp(line, "format", 6) == 0) {
            char format[64];
            if (sscanf(line, "format %63s", format) == 1) {
                if (strcmp(format, "ascii") == 0) {
                    header->is_ascii = true;
                } else if (strncmp(format, "binary_little_endian", 20) == 0) {
                    header->is_ascii = false;
                    header->is_little_endian = true;
                    return PLY_ERROR_UNSUPPORTED_FORMAT;  // Binary not yet supported
                } else if (strncmp(format, "binary_big_endian", 17) == 0) {
                    header->is_ascii = false;
                    header->is_little_endian = false;
                    return PLY_ERROR_UNSUPPORTED_FORMAT;  // Binary not yet supported
                } else {
                    return PLY_ERROR_UNSUPPORTED_FORMAT;
                }
            }
            continue;
        }

        // Parse element
        if (strncmp(line, "element", 7) == 0) {
            char element_name[64];
            int element_count;
            if (sscanf(line, "element %63s %d", element_name, &element_count) == 2) {
                if (strcmp(element_name, "vertex") == 0) {
                    header->vertex_count = element_count;
                    in_vertex_element = true;
                } else {
                    in_vertex_element = false;
                }
            }
            continue;
        }

        // Parse property (only for vertex element)
        if (strncmp(line, "property", 8) == 0 && in_vertex_element) {
            char type_str[64], name[64];

            // Skip list properties (e.g., "property list uchar int vertex_indices")
            if (strstr(line, "list") != NULL) continue;

            if (sscanf(line, "property %63s %63s", type_str, name) == 2) {
                if (header->property_count >= PLY_MAX_PROPERTIES) continue;

                int prop_idx = header->property_count;
                strncpy(header->properties[prop_idx].name, name, 63);
                header->properties[prop_idx].name[63] = '\0';
                header->properties[prop_idx].type = ply_parse_property_type(type_str);
                header->property_count++;

                // Track position and color properties
                if (strcmp(name, "x") == 0) header->prop_x = prop_idx;
                else if (strcmp(name, "y") == 0) header->prop_y = prop_idx;
                else if (strcmp(name, "z") == 0) header->prop_z = prop_idx;
                else if (strcmp(name, "red") == 0) header->prop_red = prop_idx;
                else if (strcmp(name, "green") == 0) header->prop_green = prop_idx;
                else if (strcmp(name, "blue") == 0) header->prop_blue = prop_idx;
                else if (strcmp(name, "alpha") == 0) header->prop_alpha = prop_idx;
            }
            continue;
        }
    }

    // Validate header
    if (header->vertex_count == 0) return PLY_ERROR_MISSING_VERTEX_ELEMENT;
    if (header->prop_x < 0 || header->prop_y < 0 || header->prop_z < 0) {
        return PLY_ERROR_MISSING_POSITION_PROPERTIES;
    }

    return PLY_OK;
}

//------------------------------------------------------------------------------
// Parse ASCII vertex data
//------------------------------------------------------------------------------

static inline ply_error_t ply_parse_ascii_vertices(FILE *file, const ply_header_t *header, ply_data_t *data) {
    char line[PLY_MAX_LINE_LENGTH];
    float values[PLY_MAX_PROPERTIES];

    bool has_colors = (header->prop_red >= 0 && header->prop_green >= 0 && header->prop_blue >= 0);

    // Allocate arrays
    data->count = header->vertex_count;
    data->has_colors = has_colors;
    data->points = (vec3_t*)malloc(data->count * sizeof(vec3_t));
    data->colors = has_colors ? (vec4_t*)malloc(data->count * sizeof(vec4_t)) : NULL;

    if (!data->points || (has_colors && !data->colors)) {
        if (data->points) free(data->points);
        if (data->colors) free(data->colors);
        return PLY_ERROR_MEMORY_ALLOCATION;
    }

    // Initialize bounds
    data->min_bounds = vec3_make(1e30f, 1e30f, 1e30f);
    data->max_bounds = vec3_make(-1e30f, -1e30f, -1e30f);

    // Parse each vertex
    for (int v = 0; v < header->vertex_count; v++) {
        if (!fgets(line, sizeof(line), file)) {
            free(data->points);
            if (data->colors) free(data->colors);
            return PLY_ERROR_PARSE_ERROR;
        }

        // Parse all values from line
        char *ptr = line;
        for (int p = 0; p < header->property_count; p++) {
            while (*ptr && isspace((unsigned char)*ptr)) ptr++;  // Skip whitespace
            if (!*ptr) {
                free(data->points);
                if (data->colors) free(data->colors);
                return PLY_ERROR_PARSE_ERROR;
            }

            char *end;
            values[p] = strtof(ptr, &end);
            ptr = end;
        }

        // Extract position
        float x = values[header->prop_x];
        float y = values[header->prop_y];
        float z = values[header->prop_z];
        data->points[v] = vec3_make(x, y, z);

        // Update bounds
        if (x < data->min_bounds.x) data->min_bounds.x = x;
        if (y < data->min_bounds.y) data->min_bounds.y = y;
        if (z < data->min_bounds.z) data->min_bounds.z = z;
        if (x > data->max_bounds.x) data->max_bounds.x = x;
        if (y > data->max_bounds.y) data->max_bounds.y = y;
        if (z > data->max_bounds.z) data->max_bounds.z = z;

        // Extract color if available
        if (has_colors) {
            float r = values[header->prop_red];
            float g = values[header->prop_green];
            float b = values[header->prop_blue];
            float a = (header->prop_alpha >= 0) ? values[header->prop_alpha] : 255.0f;

            // Detect if colors are 0-255 (uchar) or 0-1 (float) by checking type
            ply_property_type_t r_type = header->properties[header->prop_red].type;
            if (r_type == PLY_PROP_UCHAR || r_type == PLY_PROP_CHAR ||
                r_type == PLY_PROP_USHORT || r_type == PLY_PROP_SHORT ||
                r_type == PLY_PROP_UINT || r_type == PLY_PROP_INT) {
                // Integer colors: normalize to 0-1
                r /= 255.0f;
                g /= 255.0f;
                b /= 255.0f;
                a /= 255.0f;
            }
            // else: assume float colors are already 0-1

            data->colors[v] = vec4_make(r, g, b, a);
        }
    }

    return PLY_OK;
}

//------------------------------------------------------------------------------
// Main load function
//------------------------------------------------------------------------------

static inline ply_error_t ply_load_file(const char *filepath, ply_data_t *data) {
    FILE *file = fopen(filepath, "r");
    if (!file) return PLY_ERROR_FILE_NOT_FOUND;

    // Initialize data
    memset(data, 0, sizeof(ply_data_t));

    // Parse header
    ply_header_t header;
    ply_error_t err = ply_parse_header(file, &header);
    if (err != PLY_OK) {
        fclose(file);
        return err;
    }

    // Parse vertex data
    if (header.is_ascii) {
        err = ply_parse_ascii_vertices(file, &header, data);
    } else {
        // Binary not yet supported
        err = PLY_ERROR_UNSUPPORTED_FORMAT;
    }

    fclose(file);
    return err;
}

//------------------------------------------------------------------------------
// Free loaded data
//------------------------------------------------------------------------------

static inline void ply_data_free(ply_data_t *data) {
    if (data->points) {
        free(data->points);
        data->points = NULL;
    }
    if (data->colors) {
        free(data->colors);
        data->colors = NULL;
    }
    data->count = 0;
    data->has_colors = false;
}

//------------------------------------------------------------------------------
// Get error message
//------------------------------------------------------------------------------

static inline const char* ply_error_string(ply_error_t err) {
    switch (err) {
        case PLY_OK: return "OK";
        case PLY_ERROR_FILE_NOT_FOUND: return "File not found";
        case PLY_ERROR_INVALID_HEADER: return "Invalid PLY header";
        case PLY_ERROR_UNSUPPORTED_FORMAT: return "Unsupported PLY format (only ASCII supported)";
        case PLY_ERROR_MISSING_VERTEX_ELEMENT: return "Missing vertex element";
        case PLY_ERROR_MISSING_POSITION_PROPERTIES: return "Missing x, y, z properties";
        case PLY_ERROR_MEMORY_ALLOCATION: return "Memory allocation failed";
        case PLY_ERROR_PARSE_ERROR: return "Parse error in vertex data";
        default: return "Unknown error";
    }
}

//------------------------------------------------------------------------------
// Get quick stats from header without loading all data
//------------------------------------------------------------------------------

static inline ply_error_t ply_get_info(const char *filepath, int *vertex_count, bool *has_colors) {
    FILE *file = fopen(filepath, "r");
    if (!file) return PLY_ERROR_FILE_NOT_FOUND;

    ply_header_t header;
    ply_error_t err = ply_parse_header(file, &header);
    fclose(file);

    if (err == PLY_OK) {
        *vertex_count = header.vertex_count;
        *has_colors = (header.prop_red >= 0 && header.prop_green >= 0 && header.prop_blue >= 0);
    }

    return err;
}

//------------------------------------------------------------------------------
// Incremental parsing state (for chunked loading with progress)
//------------------------------------------------------------------------------

typedef struct {
    FILE *file;                 // Open file handle
    ply_header_t header;        // Parsed header

    // Parsed data (grows during parsing)
    vec3_t *points;
    vec4_t *colors;
    int parsed_count;           // Number of vertices parsed so far
    int capacity;               // Allocated capacity

    // Bounding box (computed during load)
    vec3_t min_bounds;
    vec3_t max_bounds;

    // Error state
    ply_error_t error;
    bool has_colors;
} ply_parse_state_t;

//------------------------------------------------------------------------------
// Open file and parse header only (first step of incremental parsing)
//------------------------------------------------------------------------------

static inline ply_error_t ply_open(const char *filepath, ply_parse_state_t *state) {
    memset(state, 0, sizeof(ply_parse_state_t));

    state->file = fopen(filepath, "r");
    if (!state->file) {
        state->error = PLY_ERROR_FILE_NOT_FOUND;
        return PLY_ERROR_FILE_NOT_FOUND;
    }

    // Parse header
    ply_error_t err = ply_parse_header(state->file, &state->header);
    if (err != PLY_OK) {
        fclose(state->file);
        state->file = NULL;
        state->error = err;
        return err;
    }

    // Check for ASCII format (binary not yet supported)
    if (!state->header.is_ascii) {
        fclose(state->file);
        state->file = NULL;
        state->error = PLY_ERROR_UNSUPPORTED_FORMAT;
        return PLY_ERROR_UNSUPPORTED_FORMAT;
    }

    // Determine if we have colors
    state->has_colors = (state->header.prop_red >= 0 &&
                         state->header.prop_green >= 0 &&
                         state->header.prop_blue >= 0);

    // Initialize bounds
    state->min_bounds = vec3_make(1e30f, 1e30f, 1e30f);
    state->max_bounds = vec3_make(-1e30f, -1e30f, -1e30f);

    // Pre-allocate arrays for all vertices
    int total = state->header.vertex_count;
    state->points = (vec3_t*)malloc(total * sizeof(vec3_t));
    if (state->has_colors) {
        state->colors = (vec4_t*)malloc(total * sizeof(vec4_t));
    }

    if (!state->points || (state->has_colors && !state->colors)) {
        if (state->points) free(state->points);
        if (state->colors) free(state->colors);
        state->points = NULL;
        state->colors = NULL;
        fclose(state->file);
        state->file = NULL;
        state->error = PLY_ERROR_MEMORY_ALLOCATION;
        return PLY_ERROR_MEMORY_ALLOCATION;
    }

    state->capacity = total;
    state->parsed_count = 0;
    state->error = PLY_OK;

    return PLY_OK;
}

//------------------------------------------------------------------------------
// Parse up to max_vertices, returns number parsed (0 when done or error)
//------------------------------------------------------------------------------

static inline int ply_parse_vertices_chunk(ply_parse_state_t *state, int max_vertices) {
    if (!state->file || state->error != PLY_OK) {
        return 0;
    }

    int total = state->header.vertex_count;
    int remaining = total - state->parsed_count;
    if (remaining <= 0) {
        return 0;  // Done parsing
    }

    int to_parse = (remaining < max_vertices) ? remaining : max_vertices;

    char line[PLY_MAX_LINE_LENGTH];
    float values[PLY_MAX_PROPERTIES];

    int parsed = 0;
    for (int i = 0; i < to_parse; i++) {
        if (!fgets(line, sizeof(line), state->file)) {
            state->error = PLY_ERROR_PARSE_ERROR;
            return parsed;
        }

        // Parse all values from line
        char *ptr = line;
        for (int p = 0; p < state->header.property_count; p++) {
            while (*ptr && isspace((unsigned char)*ptr)) ptr++;
            if (!*ptr) {
                state->error = PLY_ERROR_PARSE_ERROR;
                return parsed;
            }

            char *end;
            values[p] = strtof(ptr, &end);
            ptr = end;
        }

        int v = state->parsed_count + parsed;

        // Extract position
        float x = values[state->header.prop_x];
        float y = values[state->header.prop_y];
        float z = values[state->header.prop_z];
        state->points[v] = vec3_make(x, y, z);

        // Update bounds
        if (x < state->min_bounds.x) state->min_bounds.x = x;
        if (y < state->min_bounds.y) state->min_bounds.y = y;
        if (z < state->min_bounds.z) state->min_bounds.z = z;
        if (x > state->max_bounds.x) state->max_bounds.x = x;
        if (y > state->max_bounds.y) state->max_bounds.y = y;
        if (z > state->max_bounds.z) state->max_bounds.z = z;

        // Extract color if available
        if (state->has_colors) {
            float r = values[state->header.prop_red];
            float g = values[state->header.prop_green];
            float b = values[state->header.prop_blue];
            float a = (state->header.prop_alpha >= 0) ? values[state->header.prop_alpha] : 255.0f;

            // Detect if colors are 0-255 (uchar) or 0-1 (float) by checking type
            ply_property_type_t r_type = state->header.properties[state->header.prop_red].type;
            if (r_type == PLY_PROP_UCHAR || r_type == PLY_PROP_CHAR ||
                r_type == PLY_PROP_USHORT || r_type == PLY_PROP_SHORT ||
                r_type == PLY_PROP_UINT || r_type == PLY_PROP_INT) {
                // Integer colors: normalize to 0-1
                r /= 255.0f;
                g /= 255.0f;
                b /= 255.0f;
                a /= 255.0f;
            }

            state->colors[v] = vec4_make(r, g, b, a);
        }

        parsed++;
    }

    state->parsed_count += parsed;
    return parsed;
}

//------------------------------------------------------------------------------
// Get current parsing progress (0.0 - 1.0)
//------------------------------------------------------------------------------

static inline float ply_get_progress(const ply_parse_state_t *state) {
    if (state->header.vertex_count <= 0) return 1.0f;
    return (float)state->parsed_count / (float)state->header.vertex_count;
}

//------------------------------------------------------------------------------
// Check if parsing is complete
//------------------------------------------------------------------------------

static inline bool ply_is_complete(const ply_parse_state_t *state) {
    return state->parsed_count >= state->header.vertex_count;
}

//------------------------------------------------------------------------------
// Close and cleanup parse state
//------------------------------------------------------------------------------

static inline void ply_close(ply_parse_state_t *state) {
    if (state->file) {
        fclose(state->file);
        state->file = NULL;
    }
    // Note: points/colors arrays are not freed here - caller takes ownership
}

//------------------------------------------------------------------------------
// Free parse state data (call when cancelling or on error)
//------------------------------------------------------------------------------

static inline void ply_parse_state_free(ply_parse_state_t *state) {
    if (state->file) {
        fclose(state->file);
        state->file = NULL;
    }
    if (state->points) {
        free(state->points);
        state->points = NULL;
    }
    if (state->colors) {
        free(state->colors);
        state->colors = NULL;
    }
    state->parsed_count = 0;
    state->capacity = 0;
}

//------------------------------------------------------------------------------
// Transfer ownership of parsed data to ply_data_t (for final result)
//------------------------------------------------------------------------------

static inline void ply_parse_state_to_data(ply_parse_state_t *state, ply_data_t *data) {
    data->points = state->points;
    data->colors = state->colors;
    data->count = state->parsed_count;
    data->has_colors = state->has_colors;
    data->min_bounds = state->min_bounds;
    data->max_bounds = state->max_bounds;

    // Clear state pointers (ownership transferred)
    state->points = NULL;
    state->colors = NULL;
    state->parsed_count = 0;
    state->capacity = 0;
}

#endif // PLY_LOADER_H
