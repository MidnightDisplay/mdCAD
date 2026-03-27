//------------------------------------------------------------------------------
// ply_loader.h - PLY file loader (header-only)
//
// Supports ASCII, binary_little_endian, and binary_big_endian PLY formats with:
// - Vertex positions (x, y, z) - required
// - Vertex colors (red, green, blue, alpha) - optional
// - Face elements (triangle/quad/polygon) with fan triangulation - optional
// - Per-face colors - optional
// - Unit conversion via scale factor
//------------------------------------------------------------------------------
#ifndef PLY_LOADER_H
#define PLY_LOADER_H

#include "math/math_import.h"
#include "components/component_types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
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
    bool is_list;                           // true for "property list ..." declarations
    ply_property_type_t list_count_type;    // type of the list count (e.g., uchar)
} ply_property_t;

//------------------------------------------------------------------------------
// Face data (triangulated indices + optional per-face colors)
//------------------------------------------------------------------------------

typedef struct {
    uint32_t *indices;          // Triangulated index buffer (3 per triangle)
    vec4_t *colors;             // Per-face colors (one per original face, NULL if none)
    int tri_count;              // Number of triangles
    int face_count;             // Number of original PLY faces
} ply_face_data_t;

//------------------------------------------------------------------------------
// Mesh data (vertices + faces combined output)
//------------------------------------------------------------------------------

typedef struct {
    vec3_t *vertices;           // Vertex positions
    vec4_t *vertex_colors;      // Per-vertex colors (NULL if none)
    int vertex_count;

    ply_face_data_t faces;      // Triangulated face data

    vec3_t min_bounds;
    vec3_t max_bounds;

    bool has_vertex_colors;
    bool has_face_colors;
} ply_mesh_data_t;

//------------------------------------------------------------------------------
// PLY header info
//------------------------------------------------------------------------------

typedef struct {
    bool is_ascii;
    bool is_little_endian;  // For binary formats
    int vertex_count;

    // Vertex property indices (-1 if not present)
    int prop_x;
    int prop_y;
    int prop_z;
    int prop_red;
    int prop_green;
    int prop_blue;
    int prop_alpha;

    // Vertex properties
    ply_property_t properties[PLY_MAX_PROPERTIES];
    int property_count;
    bool has_vertex_list_props;     // true if vertex element has list properties (unsupported in binary)

    // Binary vertex layout
    int vertex_byte_stride;
    int vertex_prop_offsets[PLY_MAX_PROPERTIES];    // byte offset of each vertex property

    // Face element info
    bool has_face_element;
    int face_count;
    ply_property_t face_properties[PLY_MAX_PROPERTIES];
    int face_property_count;
    int face_list_prop_index;       // Index of vertex_indices list property (-1 if none)
    int face_prop_red;              // Face color property indices (-1 if absent)
    int face_prop_green;
    int face_prop_blue;
    int face_prop_alpha;
    int skip_lines_before_faces;    // Lines of intermediate elements to skip (ASCII)
    long skip_bytes_before_faces;   // Bytes of intermediate elements to skip (binary)
    bool binary_skip_valid;         // false if intermediate elements have list properties

    // Header end position
    long data_start_pos;
} ply_header_t;

//------------------------------------------------------------------------------
// Loaded PLY data (point cloud - backward compatible)
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
// Binary helpers: property byte sizes, endianness, byte swapping, typed reads
//------------------------------------------------------------------------------

static inline int ply_property_byte_size(ply_property_type_t type) {
    switch (type) {
        case PLY_PROP_CHAR:   case PLY_PROP_UCHAR:  return 1;
        case PLY_PROP_SHORT:  case PLY_PROP_USHORT:  return 2;
        case PLY_PROP_INT:    case PLY_PROP_UINT:    case PLY_PROP_FLOAT: return 4;
        case PLY_PROP_DOUBLE: return 8;
        default: return 0;
    }
}

static inline bool ply_is_little_endian(void) {
    uint32_t val = 1;
    return *(uint8_t*)&val == 1;
}

static inline void ply_swap2(void *p) {
    uint8_t *b = (uint8_t*)p;
    uint8_t t = b[0]; b[0] = b[1]; b[1] = t;
}

static inline void ply_swap4(void *p) {
    uint8_t *b = (uint8_t*)p;
    uint8_t t;
    t = b[0]; b[0] = b[3]; b[3] = t;
    t = b[1]; b[1] = b[2]; b[2] = t;
}

static inline void ply_swap8(void *p) {
    uint8_t *b = (uint8_t*)p;
    uint8_t t;
    t = b[0]; b[0] = b[7]; b[7] = t;
    t = b[1]; b[1] = b[6]; b[6] = t;
    t = b[2]; b[2] = b[5]; b[5] = t;
    t = b[3]; b[3] = b[4]; b[4] = t;
}

// Read a typed value from buffer, byte-swap if needed, return as float
static inline float ply_read_as_float(const uint8_t *buf, ply_property_type_t type, bool swap) {
    switch (type) {
        case PLY_PROP_CHAR:   return (float)(*(int8_t*)buf);
        case PLY_PROP_UCHAR:  return (float)(*buf);
        case PLY_PROP_SHORT:  { int16_t v;  memcpy(&v, buf, 2); if (swap) ply_swap2(&v); return (float)v; }
        case PLY_PROP_USHORT: { uint16_t v; memcpy(&v, buf, 2); if (swap) ply_swap2(&v); return (float)v; }
        case PLY_PROP_INT:    { int32_t v;  memcpy(&v, buf, 4); if (swap) ply_swap4(&v); return (float)v; }
        case PLY_PROP_UINT:   { uint32_t v; memcpy(&v, buf, 4); if (swap) ply_swap4(&v); return (float)v; }
        case PLY_PROP_FLOAT:  { float v;    memcpy(&v, buf, 4); if (swap) ply_swap4(&v); return v; }
        case PLY_PROP_DOUBLE: { double v;   memcpy(&v, buf, 8); if (swap) ply_swap8(&v); return (float)v; }
        default: return 0.0f;
    }
}

// Read a typed value from buffer, byte-swap if needed, return as int32
static inline int32_t ply_read_as_int32(const uint8_t *buf, ply_property_type_t type, bool swap) {
    switch (type) {
        case PLY_PROP_CHAR:   return (int32_t)(*(int8_t*)buf);
        case PLY_PROP_UCHAR:  return (int32_t)(*buf);
        case PLY_PROP_SHORT:  { int16_t v;  memcpy(&v, buf, 2); if (swap) ply_swap2(&v); return (int32_t)v; }
        case PLY_PROP_USHORT: { uint16_t v; memcpy(&v, buf, 2); if (swap) ply_swap2(&v); return (int32_t)v; }
        case PLY_PROP_INT:    { int32_t v;  memcpy(&v, buf, 4); if (swap) ply_swap4(&v); return v; }
        case PLY_PROP_UINT:   { uint32_t v; memcpy(&v, buf, 4); if (swap) ply_swap4(&v); return (int32_t)v; }
        case PLY_PROP_FLOAT:  { float v;    memcpy(&v, buf, 4); if (swap) ply_swap4(&v); return (int32_t)v; }
        case PLY_PROP_DOUBLE: { double v;   memcpy(&v, buf, 8); if (swap) ply_swap8(&v); return (int32_t)v; }
        default: return 0;
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
    header->face_list_prop_index = -1;
    header->face_prop_red = -1;
    header->face_prop_green = -1;
    header->face_prop_blue = -1;
    header->face_prop_alpha = -1;
    header->binary_skip_valid = true;

    // Check magic number
    if (!fgets(line, sizeof(line), file)) return PLY_ERROR_INVALID_HEADER;
    ply_trim_end(line);
    if (strcmp(line, "ply") != 0) return PLY_ERROR_INVALID_HEADER;

    // Element tracking: 0=none, 1=vertex, 2=face, 3=other
    int current_element = 0;
    bool vertex_seen = false;
    bool face_seen = false;
    int other_element_count = 0;     // Lines to skip for current "other" element
    int other_byte_stride = 0;       // Byte stride of current "other" element (binary)
    bool other_has_list = false;     // Current "other" element has a list property

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
                } else if (strncmp(format, "binary_big_endian", 17) == 0) {
                    header->is_ascii = false;
                    header->is_little_endian = false;
                } else {
                    return PLY_ERROR_UNSUPPORTED_FORMAT;
                }
            }
            continue;
        }

        // Parse element
        if (strncmp(line, "element", 7) == 0) {
            // If we were in an "other" element, accumulate its skip count
            if (current_element == 3 && vertex_seen && !face_seen) {
                header->skip_lines_before_faces += other_element_count;
                if (other_has_list) {
                    header->binary_skip_valid = false;
                } else {
                    header->skip_bytes_before_faces += (long)other_element_count * other_byte_stride;
                }
            }

            char element_name[64];
            int element_count;
            if (sscanf(line, "element %63s %d", element_name, &element_count) == 2) {
                if (strcmp(element_name, "vertex") == 0) {
                    header->vertex_count = element_count;
                    current_element = 1;
                    vertex_seen = true;
                } else if (strcmp(element_name, "face") == 0) {
                    header->face_count = element_count;
                    header->has_face_element = true;
                    current_element = 2;
                    face_seen = true;
                } else {
                    current_element = 3;
                    other_element_count = element_count;
                    other_byte_stride = 0;
                    other_has_list = false;
                }
            }
            continue;
        }

        // Parse property
        if (strncmp(line, "property", 8) == 0) {
            // Vertex properties
            if (current_element == 1) {
                // Track list properties in vertex element (unsupported in binary)
                if (strstr(line, "list") != NULL) {
                    header->has_vertex_list_props = true;
                    continue;
                }

                char type_str[64], name[64];
                if (sscanf(line, "property %63s %63s", type_str, name) == 2) {
                    if (header->property_count >= PLY_MAX_PROPERTIES) continue;

                    int prop_idx = header->property_count;
                    strncpy(header->properties[prop_idx].name, name, 63);
                    header->properties[prop_idx].name[63] = '\0';
                    header->properties[prop_idx].type = ply_parse_property_type(type_str);
                    header->properties[prop_idx].is_list = false;
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
            }
            // Face properties
            else if (current_element == 2) {
                if (header->face_property_count >= PLY_MAX_PROPERTIES) continue;

                int fp_idx = header->face_property_count;

                if (strstr(line, "list") != NULL) {
                    // "property list <count_type> <value_type> <name>"
                    char count_type_str[64], value_type_str[64], name[64];
                    if (sscanf(line, "property list %63s %63s %63s",
                               count_type_str, value_type_str, name) == 3) {
                        strncpy(header->face_properties[fp_idx].name, name, 63);
                        header->face_properties[fp_idx].name[63] = '\0';
                        header->face_properties[fp_idx].type = ply_parse_property_type(value_type_str);
                        header->face_properties[fp_idx].is_list = true;
                        header->face_properties[fp_idx].list_count_type = ply_parse_property_type(count_type_str);
                        header->face_property_count++;

                        // Track vertex_indices or vertex_index list property
                        if (strcmp(name, "vertex_indices") == 0 || strcmp(name, "vertex_index") == 0) {
                            header->face_list_prop_index = fp_idx;
                        }
                    }
                } else {
                    // Scalar face property
                    char type_str[64], name[64];
                    if (sscanf(line, "property %63s %63s", type_str, name) == 2) {
                        strncpy(header->face_properties[fp_idx].name, name, 63);
                        header->face_properties[fp_idx].name[63] = '\0';
                        header->face_properties[fp_idx].type = ply_parse_property_type(type_str);
                        header->face_properties[fp_idx].is_list = false;
                        header->face_property_count++;

                        // Track face color properties
                        if (strcmp(name, "red") == 0) header->face_prop_red = fp_idx;
                        else if (strcmp(name, "green") == 0) header->face_prop_green = fp_idx;
                        else if (strcmp(name, "blue") == 0) header->face_prop_blue = fp_idx;
                        else if (strcmp(name, "alpha") == 0) header->face_prop_alpha = fp_idx;
                    }
                }
            }
            // Other element properties - track byte stride for binary skipping
            else if (current_element == 3) {
                if (strstr(line, "list") != NULL) {
                    other_has_list = true;
                } else {
                    char type_str[64], name[64];
                    if (sscanf(line, "property %63s %63s", type_str, name) == 2) {
                        other_byte_stride += ply_property_byte_size(ply_parse_property_type(type_str));
                    }
                }
            }
            continue;
        }
    }

    // If last element was "other" and faces haven't been seen yet but vertex was,
    // accumulate skip lines (handles case where other elements come after vertex but no face)
    if (current_element == 3 && vertex_seen && !face_seen) {
        header->skip_lines_before_faces += other_element_count;
        if (other_has_list) {
            header->binary_skip_valid = false;
        } else {
            header->skip_bytes_before_faces += (long)other_element_count * other_byte_stride;
        }
    }

    // Validate header
    if (header->vertex_count == 0) return PLY_ERROR_MISSING_VERTEX_ELEMENT;
    if (header->prop_x < 0 || header->prop_y < 0 || header->prop_z < 0) {
        return PLY_ERROR_MISSING_POSITION_PROPERTIES;
    }

    // Compute vertex byte stride and property offsets (for binary parsing)
    {
        int offset = 0;
        for (int i = 0; i < header->property_count; i++) {
            header->vertex_prop_offsets[i] = offset;
            offset += ply_property_byte_size(header->properties[i].type);
        }
        header->vertex_byte_stride = offset;
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
    mdcad_import_bounds_reset(&data->min_bounds, &data->max_bounds);

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
        data->points[v] = mdcad_import_vec3_make(x, y, z);

        // Update bounds
        mdcad_import_bounds_expand(&data->min_bounds, &data->max_bounds, data->points[v]);

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
// Parse ASCII face data (after vertices have been parsed)
//------------------------------------------------------------------------------

static inline ply_error_t ply_parse_ascii_faces(FILE *file, const ply_header_t *header,
                                                 ply_face_data_t *face_data) {
    char line[PLY_MAX_LINE_LENGTH];

    if (!header->has_face_element || header->face_count <= 0 || header->face_list_prop_index < 0) {
        memset(face_data, 0, sizeof(ply_face_data_t));
        return PLY_OK;
    }

    bool has_face_colors = (header->face_prop_red >= 0 &&
                            header->face_prop_green >= 0 &&
                            header->face_prop_blue >= 0);

    // Skip intermediate element lines between vertex data and face data
    for (int i = 0; i < header->skip_lines_before_faces; i++) {
        if (!fgets(line, sizeof(line), file)) {
            return PLY_ERROR_PARSE_ERROR;
        }
    }

    // Allocate with initial capacity (face_count * 2 triangles worth of indices)
    int tri_capacity = header->face_count * 2;
    uint32_t *indices = (uint32_t*)malloc(tri_capacity * 3 * sizeof(uint32_t));
    vec4_t *face_colors = has_face_colors ? (vec4_t*)malloc(header->face_count * sizeof(vec4_t)) : NULL;

    if (!indices || (has_face_colors && !face_colors)) {
        if (indices) free(indices);
        if (face_colors) free(face_colors);
        return PLY_ERROR_MEMORY_ALLOCATION;
    }

    int tri_count = 0;

    for (int f = 0; f < header->face_count; f++) {
        if (!fgets(line, sizeof(line), file)) {
            free(indices);
            if (face_colors) free(face_colors);
            return PLY_ERROR_PARSE_ERROR;
        }

        char *ptr = line;
        int face_vertex_indices[256];   // Max polygon vertices
        int face_vertex_count = 0;
        float face_scalar_values[PLY_MAX_PROPERTIES];

        // Parse face properties in declared order
        for (int p = 0; p < header->face_property_count; p++) {
            if (header->face_properties[p].is_list) {
                // Read count
                while (*ptr && isspace((unsigned char)*ptr)) ptr++;
                if (!*ptr) { free(indices); if (face_colors) free(face_colors); return PLY_ERROR_PARSE_ERROR; }
                char *end;
                int count = (int)strtol(ptr, &end, 10);
                ptr = end;

                // Read N index values
                if (p == header->face_list_prop_index) {
                    face_vertex_count = count;
                    for (int j = 0; j < count && j < 256; j++) {
                        while (*ptr && isspace((unsigned char)*ptr)) ptr++;
                        if (!*ptr) { free(indices); if (face_colors) free(face_colors); return PLY_ERROR_PARSE_ERROR; }
                        face_vertex_indices[j] = (int)strtol(ptr, &end, 10);
                        ptr = end;
                    }
                } else {
                    // Skip unknown list property values
                    for (int j = 0; j < count; j++) {
                        while (*ptr && isspace((unsigned char)*ptr)) ptr++;
                        if (!*ptr) break;
                        strtof(ptr, &end);
                        ptr = end;
                    }
                }
            } else {
                // Scalar property
                while (*ptr && isspace((unsigned char)*ptr)) ptr++;
                if (!*ptr) { free(indices); if (face_colors) free(face_colors); return PLY_ERROR_PARSE_ERROR; }
                char *end;
                face_scalar_values[p] = strtof(ptr, &end);
                ptr = end;
            }
        }

        // Fan triangulation
        int new_tris = (face_vertex_count >= 3) ? (face_vertex_count - 2) : 0;

        // Grow index buffer if needed
        while (tri_count + new_tris > tri_capacity) {
            tri_capacity *= 2;
            uint32_t *new_indices = (uint32_t*)realloc(indices, tri_capacity * 3 * sizeof(uint32_t));
            if (!new_indices) {
                free(indices);
                if (face_colors) free(face_colors);
                return PLY_ERROR_MEMORY_ALLOCATION;
            }
            indices = new_indices;
        }

        // Emit triangles using fan from vertex 0
        for (int t = 0; t < new_tris; t++) {
            int base = (tri_count + t) * 3;
            indices[base + 0] = (uint32_t)face_vertex_indices[0];
            indices[base + 1] = (uint32_t)face_vertex_indices[t + 1];
            indices[base + 2] = (uint32_t)face_vertex_indices[t + 2];
        }
        tri_count += new_tris;

        // Extract face color if present
        if (has_face_colors) {
            float r = face_scalar_values[header->face_prop_red];
            float g = face_scalar_values[header->face_prop_green];
            float b = face_scalar_values[header->face_prop_blue];
            float a = (header->face_prop_alpha >= 0) ? face_scalar_values[header->face_prop_alpha] : 255.0f;

            // Normalize integer colors
            ply_property_type_t r_type = header->face_properties[header->face_prop_red].type;
            if (r_type == PLY_PROP_UCHAR || r_type == PLY_PROP_CHAR ||
                r_type == PLY_PROP_USHORT || r_type == PLY_PROP_SHORT ||
                r_type == PLY_PROP_UINT || r_type == PLY_PROP_INT) {
                r /= 255.0f;
                g /= 255.0f;
                b /= 255.0f;
                a /= 255.0f;
            }

            face_colors[f] = vec4_make(r, g, b, a);
        }
    }

    face_data->indices = indices;
    face_data->colors = face_colors;
    face_data->tri_count = tri_count;
    face_data->face_count = header->face_count;

    return PLY_OK;
}

//------------------------------------------------------------------------------
// Parse binary vertex data (one-shot, bulk read)
//------------------------------------------------------------------------------

static inline ply_error_t ply_parse_binary_vertices(FILE *file, const ply_header_t *header, ply_data_t *data) {
    bool has_colors = (header->prop_red >= 0 && header->prop_green >= 0 && header->prop_blue >= 0);
    bool need_swap = (header->is_little_endian != ply_is_little_endian());
    int stride = header->vertex_byte_stride;

    if (stride <= 0) return PLY_ERROR_PARSE_ERROR;

    // Allocate output arrays
    data->count = header->vertex_count;
    data->has_colors = has_colors;
    data->points = (vec3_t*)malloc(data->count * sizeof(vec3_t));
    data->colors = has_colors ? (vec4_t*)malloc(data->count * sizeof(vec4_t)) : NULL;

    if (!data->points || (has_colors && !data->colors)) {
        if (data->points) free(data->points);
        if (data->colors) free(data->colors);
        return PLY_ERROR_MEMORY_ALLOCATION;
    }

    // Read all vertex data at once
    size_t total_bytes = (size_t)header->vertex_count * stride;
    uint8_t *raw = (uint8_t*)malloc(total_bytes);
    if (!raw) {
        free(data->points);
        if (data->colors) free(data->colors);
        return PLY_ERROR_MEMORY_ALLOCATION;
    }

    if (fread(raw, 1, total_bytes, file) != total_bytes) {
        free(raw);
        free(data->points);
        if (data->colors) free(data->colors);
        return PLY_ERROR_PARSE_ERROR;
    }

    // Initialize bounds
    mdcad_import_bounds_reset(&data->min_bounds, &data->max_bounds);

    for (int v = 0; v < header->vertex_count; v++) {
        uint8_t *row = raw + (size_t)v * stride;

        float x = ply_read_as_float(row + header->vertex_prop_offsets[header->prop_x],
                                     header->properties[header->prop_x].type, need_swap);
        float y = ply_read_as_float(row + header->vertex_prop_offsets[header->prop_y],
                                     header->properties[header->prop_y].type, need_swap);
        float z = ply_read_as_float(row + header->vertex_prop_offsets[header->prop_z],
                                     header->properties[header->prop_z].type, need_swap);
        data->points[v] = mdcad_import_vec3_make(x, y, z);

        // Update bounds
        mdcad_import_bounds_expand(&data->min_bounds, &data->max_bounds, data->points[v]);

        // Extract color if available
        if (has_colors) {
            float r = ply_read_as_float(row + header->vertex_prop_offsets[header->prop_red],
                                         header->properties[header->prop_red].type, need_swap);
            float g = ply_read_as_float(row + header->vertex_prop_offsets[header->prop_green],
                                         header->properties[header->prop_green].type, need_swap);
            float b = ply_read_as_float(row + header->vertex_prop_offsets[header->prop_blue],
                                         header->properties[header->prop_blue].type, need_swap);
            float a = (header->prop_alpha >= 0) ?
                ply_read_as_float(row + header->vertex_prop_offsets[header->prop_alpha],
                                   header->properties[header->prop_alpha].type, need_swap) : 1.0f;

            // Normalize integer colors to 0-1
            ply_property_type_t r_type = header->properties[header->prop_red].type;
            if (r_type == PLY_PROP_UCHAR || r_type == PLY_PROP_CHAR ||
                r_type == PLY_PROP_USHORT || r_type == PLY_PROP_SHORT ||
                r_type == PLY_PROP_UINT || r_type == PLY_PROP_INT) {
                r /= 255.0f;
                g /= 255.0f;
                b /= 255.0f;
                a /= 255.0f;
            }

            data->colors[v] = vec4_make(r, g, b, a);
        }
    }

    free(raw);
    return PLY_OK;
}

//------------------------------------------------------------------------------
// Parse binary face data (one-shot, sequential read per face)
//------------------------------------------------------------------------------

static inline ply_error_t ply_parse_binary_faces(FILE *file, const ply_header_t *header,
                                                   ply_face_data_t *face_data) {
    if (!header->has_face_element || header->face_count <= 0 || header->face_list_prop_index < 0) {
        memset(face_data, 0, sizeof(ply_face_data_t));
        return PLY_OK;
    }

    bool need_swap = (header->is_little_endian != ply_is_little_endian());
    bool has_face_colors = (header->face_prop_red >= 0 &&
                            header->face_prop_green >= 0 &&
                            header->face_prop_blue >= 0);

    // Skip intermediate element bytes
    if (header->skip_bytes_before_faces > 0) {
        if (fseek(file, header->skip_bytes_before_faces, SEEK_CUR) != 0) {
            return PLY_ERROR_PARSE_ERROR;
        }
    }

    // Allocate with initial capacity
    int tri_capacity = header->face_count * 2;
    uint32_t *indices = (uint32_t*)malloc(tri_capacity * 3 * sizeof(uint32_t));
    vec4_t *face_colors = has_face_colors ? (vec4_t*)malloc(header->face_count * sizeof(vec4_t)) : NULL;

    if (!indices || (has_face_colors && !face_colors)) {
        if (indices) free(indices);
        if (face_colors) free(face_colors);
        return PLY_ERROR_MEMORY_ALLOCATION;
    }

    int tri_count = 0;
    uint8_t buf[8];

    for (int f = 0; f < header->face_count; f++) {
        int face_vertex_indices[256];
        int face_vertex_count = 0;
        float face_scalar_values[PLY_MAX_PROPERTIES];

        // Parse face properties in declared order
        for (int p = 0; p < header->face_property_count; p++) {
            const ply_property_t *prop = &header->face_properties[p];

            if (prop->is_list) {
                // Read list count
                int count_size = ply_property_byte_size(prop->list_count_type);
                if (fread(buf, 1, count_size, file) != (size_t)count_size) {
                    free(indices); if (face_colors) free(face_colors);
                    return PLY_ERROR_PARSE_ERROR;
                }
                int count = ply_read_as_int32(buf, prop->list_count_type, need_swap);

                if (p == header->face_list_prop_index) {
                    face_vertex_count = count;
                    int val_size = ply_property_byte_size(prop->type);
                    for (int j = 0; j < count && j < 256; j++) {
                        if (fread(buf, 1, val_size, file) != (size_t)val_size) {
                            free(indices); if (face_colors) free(face_colors);
                            return PLY_ERROR_PARSE_ERROR;
                        }
                        face_vertex_indices[j] = ply_read_as_int32(buf, prop->type, need_swap);
                    }
                } else {
                    // Skip unknown list values
                    int val_size = ply_property_byte_size(prop->type);
                    for (int j = 0; j < count; j++) {
                        if (fread(buf, 1, val_size, file) != (size_t)val_size) {
                            free(indices); if (face_colors) free(face_colors);
                            return PLY_ERROR_PARSE_ERROR;
                        }
                    }
                }
            } else {
                // Scalar property
                int sz = ply_property_byte_size(prop->type);
                if (fread(buf, 1, sz, file) != (size_t)sz) {
                    free(indices); if (face_colors) free(face_colors);
                    return PLY_ERROR_PARSE_ERROR;
                }
                face_scalar_values[p] = ply_read_as_float(buf, prop->type, need_swap);
            }
        }

        // Fan triangulation
        int new_tris = (face_vertex_count >= 3) ? (face_vertex_count - 2) : 0;

        // Grow index buffer if needed
        while (tri_count + new_tris > tri_capacity) {
            tri_capacity *= 2;
            uint32_t *new_indices = (uint32_t*)realloc(indices, tri_capacity * 3 * sizeof(uint32_t));
            if (!new_indices) {
                free(indices); if (face_colors) free(face_colors);
                return PLY_ERROR_MEMORY_ALLOCATION;
            }
            indices = new_indices;
        }

        // Emit triangles using fan from vertex 0
        for (int t = 0; t < new_tris; t++) {
            int base = (tri_count + t) * 3;
            indices[base + 0] = (uint32_t)face_vertex_indices[0];
            indices[base + 1] = (uint32_t)face_vertex_indices[t + 1];
            indices[base + 2] = (uint32_t)face_vertex_indices[t + 2];
        }
        tri_count += new_tris;

        // Extract face color if present
        if (has_face_colors) {
            float r = face_scalar_values[header->face_prop_red];
            float g = face_scalar_values[header->face_prop_green];
            float b = face_scalar_values[header->face_prop_blue];
            float a = (header->face_prop_alpha >= 0) ? face_scalar_values[header->face_prop_alpha] : 1.0f;

            // Normalize integer colors to 0-1
            ply_property_type_t r_type = header->face_properties[header->face_prop_red].type;
            if (r_type == PLY_PROP_UCHAR || r_type == PLY_PROP_CHAR ||
                r_type == PLY_PROP_USHORT || r_type == PLY_PROP_SHORT ||
                r_type == PLY_PROP_UINT || r_type == PLY_PROP_INT) {
                r /= 255.0f;
                g /= 255.0f;
                b /= 255.0f;
                a /= 255.0f;
            }

            face_colors[f] = vec4_make(r, g, b, a);
        }
    }

    face_data->indices = indices;
    face_data->colors = face_colors;
    face_data->tri_count = tri_count;
    face_data->face_count = header->face_count;

    return PLY_OK;
}

//------------------------------------------------------------------------------
// Main load function (point cloud - backward compatible)
//------------------------------------------------------------------------------

static inline ply_error_t ply_load_file(const char *filepath, ply_data_t *data) {
    FILE *file = fopen(filepath, "rb");
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
        if (header.has_vertex_list_props) {
            fclose(file);
            return PLY_ERROR_UNSUPPORTED_FORMAT;
        }
        err = ply_parse_binary_vertices(file, &header, data);
    }

    fclose(file);
    return err;
}

//------------------------------------------------------------------------------
// Load mesh file (vertices + faces, one-shot)
//------------------------------------------------------------------------------

static inline ply_error_t ply_load_mesh_file(const char *filepath, ply_mesh_data_t *mesh) {
    FILE *file = fopen(filepath, "rb");
    if (!file) return PLY_ERROR_FILE_NOT_FOUND;

    memset(mesh, 0, sizeof(ply_mesh_data_t));

    // Parse header
    ply_header_t header;
    ply_error_t err = ply_parse_header(file, &header);
    if (err != PLY_OK) {
        fclose(file);
        return err;
    }

    // Check binary prerequisites
    if (!header.is_ascii) {
        if (header.has_vertex_list_props) {
            fclose(file);
            return PLY_ERROR_UNSUPPORTED_FORMAT;
        }
        if (header.has_face_element && !header.binary_skip_valid) {
            fclose(file);
            return PLY_ERROR_UNSUPPORTED_FORMAT;
        }
    }

    // Parse vertices
    ply_data_t vdata;
    memset(&vdata, 0, sizeof(ply_data_t));
    if (header.is_ascii) {
        err = ply_parse_ascii_vertices(file, &header, &vdata);
    } else {
        err = ply_parse_binary_vertices(file, &header, &vdata);
    }
    if (err != PLY_OK) {
        fclose(file);
        return err;
    }

    // Transfer vertex data to mesh
    mesh->vertices = vdata.points;
    mesh->vertex_colors = vdata.colors;
    mesh->vertex_count = vdata.count;
    mesh->has_vertex_colors = vdata.has_colors;
    mesh->min_bounds = vdata.min_bounds;
    mesh->max_bounds = vdata.max_bounds;

    // Parse faces
    if (header.is_ascii) {
        err = ply_parse_ascii_faces(file, &header, &mesh->faces);
    } else {
        err = ply_parse_binary_faces(file, &header, &mesh->faces);
    }
    if (err != PLY_OK) {
        free(mesh->vertices);
        if (mesh->vertex_colors) free(mesh->vertex_colors);
        memset(mesh, 0, sizeof(ply_mesh_data_t));
        fclose(file);
        return err;
    }

    mesh->has_face_colors = (mesh->faces.colors != NULL);

    fclose(file);
    return PLY_OK;
}

//------------------------------------------------------------------------------
// Free mesh data
//------------------------------------------------------------------------------

static inline void ply_mesh_data_free(ply_mesh_data_t *mesh) {
    if (mesh->vertices) { free(mesh->vertices); mesh->vertices = NULL; }
    if (mesh->vertex_colors) { free(mesh->vertex_colors); mesh->vertex_colors = NULL; }
    if (mesh->faces.indices) { free(mesh->faces.indices); mesh->faces.indices = NULL; }
    if (mesh->faces.colors) { free(mesh->faces.colors); mesh->faces.colors = NULL; }
    mesh->vertex_count = 0;
    mesh->faces.tri_count = 0;
    mesh->faces.face_count = 0;
    mesh->has_vertex_colors = false;
    mesh->has_face_colors = false;
}

//------------------------------------------------------------------------------
// Free loaded point cloud data
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
        case PLY_ERROR_UNSUPPORTED_FORMAT: return "Unsupported PLY format (e.g., vertex list properties in binary mode)";
        case PLY_ERROR_MISSING_VERTEX_ELEMENT: return "Missing vertex element";
        case PLY_ERROR_MISSING_POSITION_PROPERTIES: return "Missing x, y, z properties";
        case PLY_ERROR_MEMORY_ALLOCATION: return "Memory allocation failed";
        case PLY_ERROR_PARSE_ERROR: return "Parse error in vertex data";
        default: return "Unknown error";
    }
}

//------------------------------------------------------------------------------
// Get quick stats from header without loading all data (point cloud)
//------------------------------------------------------------------------------

static inline ply_error_t ply_get_info(const char *filepath, int *vertex_count, bool *has_colors) {
    FILE *file = fopen(filepath, "rb");
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
// Get mesh info from header without loading data
//------------------------------------------------------------------------------

static inline ply_error_t ply_get_mesh_info(const char *filepath,
                                             int *vertex_count, int *face_count,
                                             bool *has_vertex_colors, bool *has_face_colors) {
    FILE *file = fopen(filepath, "rb");
    if (!file) return PLY_ERROR_FILE_NOT_FOUND;

    ply_header_t header;
    ply_error_t err = ply_parse_header(file, &header);
    fclose(file);

    if (err == PLY_OK) {
        *vertex_count = header.vertex_count;
        *face_count = header.has_face_element ? header.face_count : 0;
        *has_vertex_colors = (header.prop_red >= 0 && header.prop_green >= 0 && header.prop_blue >= 0);
        *has_face_colors = (header.face_prop_red >= 0 && header.face_prop_green >= 0 && header.face_prop_blue >= 0);
    }

    return err;
}

//------------------------------------------------------------------------------
// Incremental parsing state (for chunked loading with progress)
//------------------------------------------------------------------------------

typedef struct {
    FILE *file;                 // Open file handle
    ply_header_t header;        // Parsed header

    // Vertex data (grows during parsing)
    vec3_t *points;
    vec4_t *colors;
    int parsed_count;           // Number of vertices parsed so far
    int capacity;               // Allocated capacity

    // Face data (grows during parsing)
    uint32_t *face_indices;     // Triangulated index buffer
    vec4_t *face_colors;        // Per-face colors (NULL if none)
    int face_parsed_count;      // Number of original PLY faces parsed so far
    int face_tri_count;         // Number of triangles emitted so far
    int face_tri_capacity;      // Allocated capacity (in triangles)
    int face_total;             // Total faces to parse (from header)
    bool parsing_faces;         // Currently parsing faces (vs vertices)
    bool has_face_colors;       // Face element has color properties
    int face_lines_skipped;     // Intermediate lines already skipped (ASCII)

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

    state->file = fopen(filepath, "rb");
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

    // Check binary prerequisites
    if (!state->header.is_ascii) {
        if (state->header.has_vertex_list_props) {
            fclose(state->file);
            state->file = NULL;
            state->error = PLY_ERROR_UNSUPPORTED_FORMAT;
            return PLY_ERROR_UNSUPPORTED_FORMAT;
        }
        if (state->header.has_face_element && !state->header.binary_skip_valid) {
            fclose(state->file);
            state->file = NULL;
            state->error = PLY_ERROR_UNSUPPORTED_FORMAT;
            return PLY_ERROR_UNSUPPORTED_FORMAT;
        }
    }

    // Determine if we have vertex colors
    state->has_colors = (state->header.prop_red >= 0 &&
                         state->header.prop_green >= 0 &&
                         state->header.prop_blue >= 0);

    // Initialize bounds
    mdcad_import_bounds_reset(&state->min_bounds, &state->max_bounds);

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

    // Initialize face parsing state
    state->face_total = (state->header.has_face_element && state->header.face_count > 0 &&
                         state->header.face_list_prop_index >= 0) ? state->header.face_count : 0;
    state->has_face_colors = (state->header.face_prop_red >= 0 &&
                              state->header.face_prop_green >= 0 &&
                              state->header.face_prop_blue >= 0);
    state->parsing_faces = false;
    state->face_parsed_count = 0;
    state->face_tri_count = 0;
    state->face_lines_skipped = 0;

    if (state->face_total > 0) {
        state->face_tri_capacity = state->face_total * 2;
        state->face_indices = (uint32_t*)malloc(state->face_tri_capacity * 3 * sizeof(uint32_t));
        if (state->has_face_colors) {
            state->face_colors = (vec4_t*)malloc(state->face_total * sizeof(vec4_t));
        }

        if (!state->face_indices || (state->has_face_colors && !state->face_colors)) {
            free(state->points);
            if (state->colors) free(state->colors);
            if (state->face_indices) free(state->face_indices);
            if (state->face_colors) free(state->face_colors);
            state->points = NULL;
            state->colors = NULL;
            state->face_indices = NULL;
            state->face_colors = NULL;
            fclose(state->file);
            state->file = NULL;
            state->error = PLY_ERROR_MEMORY_ALLOCATION;
            return PLY_ERROR_MEMORY_ALLOCATION;
        }
    }

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

    if (state->header.is_ascii) {
        // --- ASCII vertex parsing ---
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
            state->points[v] = mdcad_import_vec3_make(x, y, z);

            // Update bounds
            mdcad_import_bounds_expand(&state->min_bounds, &state->max_bounds, state->points[v]);

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
    } else {
        // --- Binary vertex parsing ---
        bool need_swap = (state->header.is_little_endian != ply_is_little_endian());
        int stride = state->header.vertex_byte_stride;

        size_t chunk_bytes = (size_t)to_parse * stride;
        uint8_t *raw = (uint8_t*)malloc(chunk_bytes);
        if (!raw) {
            state->error = PLY_ERROR_MEMORY_ALLOCATION;
            return 0;
        }

        size_t bytes_read = fread(raw, 1, chunk_bytes, state->file);
        int actually_parsed = (stride > 0) ? (int)(bytes_read / (size_t)stride) : 0;
        if (actually_parsed <= 0) {
            free(raw);
            state->error = PLY_ERROR_PARSE_ERROR;
            return 0;
        }

        for (int i = 0; i < actually_parsed; i++) {
            uint8_t *row = raw + (size_t)i * stride;
            int v = state->parsed_count + i;

            float x = ply_read_as_float(row + state->header.vertex_prop_offsets[state->header.prop_x],
                                         state->header.properties[state->header.prop_x].type, need_swap);
            float y = ply_read_as_float(row + state->header.vertex_prop_offsets[state->header.prop_y],
                                         state->header.properties[state->header.prop_y].type, need_swap);
            float z = ply_read_as_float(row + state->header.vertex_prop_offsets[state->header.prop_z],
                                         state->header.properties[state->header.prop_z].type, need_swap);
            state->points[v] = mdcad_import_vec3_make(x, y, z);

            // Update bounds
            mdcad_import_bounds_expand(&state->min_bounds, &state->max_bounds, state->points[v]);

            // Extract color if available
            if (state->has_colors) {
                float r = ply_read_as_float(row + state->header.vertex_prop_offsets[state->header.prop_red],
                                             state->header.properties[state->header.prop_red].type, need_swap);
                float g = ply_read_as_float(row + state->header.vertex_prop_offsets[state->header.prop_green],
                                             state->header.properties[state->header.prop_green].type, need_swap);
                float b = ply_read_as_float(row + state->header.vertex_prop_offsets[state->header.prop_blue],
                                             state->header.properties[state->header.prop_blue].type, need_swap);
                float a = (state->header.prop_alpha >= 0) ?
                    ply_read_as_float(row + state->header.vertex_prop_offsets[state->header.prop_alpha],
                                       state->header.properties[state->header.prop_alpha].type, need_swap) : 1.0f;

                // Normalize integer colors to 0-1
                ply_property_type_t r_type = state->header.properties[state->header.prop_red].type;
                if (r_type == PLY_PROP_UCHAR || r_type == PLY_PROP_CHAR ||
                    r_type == PLY_PROP_USHORT || r_type == PLY_PROP_SHORT ||
                    r_type == PLY_PROP_UINT || r_type == PLY_PROP_INT) {
                    r /= 255.0f;
                    g /= 255.0f;
                    b /= 255.0f;
                    a /= 255.0f;
                }

                state->colors[v] = vec4_make(r, g, b, a);
            }
        }

        free(raw);
        state->parsed_count += actually_parsed;
        return actually_parsed;
    }
}

//------------------------------------------------------------------------------
// Check if all vertices have been parsed (caller uses to switch to face parsing)
//------------------------------------------------------------------------------

static inline bool ply_vertices_complete(const ply_parse_state_t *state) {
    return state->parsed_count >= state->header.vertex_count;
}

//------------------------------------------------------------------------------
// Parse up to max_faces original PLY faces, with triangulation and dynamic growth
// Returns number of original faces parsed (0 when done or error)
//------------------------------------------------------------------------------

static inline int ply_parse_faces_chunk(ply_parse_state_t *state, int max_faces) {
    if (!state->file || state->error != PLY_OK || state->face_total <= 0) {
        return 0;
    }

    // Ensure vertices are fully parsed first
    if (!ply_vertices_complete(state)) {
        return 0;
    }

    if (state->header.is_ascii) {
        // --- ASCII face parsing ---

        // Skip intermediate element lines (once)
        if (!state->parsing_faces) {
            char skip_line[PLY_MAX_LINE_LENGTH];
            int to_skip = state->header.skip_lines_before_faces - state->face_lines_skipped;
            for (int i = 0; i < to_skip; i++) {
                if (!fgets(skip_line, sizeof(skip_line), state->file)) {
                    state->error = PLY_ERROR_PARSE_ERROR;
                    return 0;
                }
                state->face_lines_skipped++;
            }
            state->parsing_faces = true;
        }

        int remaining = state->face_total - state->face_parsed_count;
        if (remaining <= 0) return 0;

        int to_parse = (remaining < max_faces) ? remaining : max_faces;

        char line[PLY_MAX_LINE_LENGTH];
        int parsed = 0;

        for (int i = 0; i < to_parse; i++) {
            if (!fgets(line, sizeof(line), state->file)) {
                state->error = PLY_ERROR_PARSE_ERROR;
                return parsed;
            }

            char *ptr = line;
            int face_vertex_indices[256];
            int face_vertex_count = 0;
            float face_scalar_values[PLY_MAX_PROPERTIES];

            // Parse face properties in declared order
            for (int p = 0; p < state->header.face_property_count; p++) {
                if (state->header.face_properties[p].is_list) {
                    // Read count
                    while (*ptr && isspace((unsigned char)*ptr)) ptr++;
                    if (!*ptr) { state->error = PLY_ERROR_PARSE_ERROR; return parsed; }
                    char *end;
                    int count = (int)strtol(ptr, &end, 10);
                    ptr = end;

                    if (p == state->header.face_list_prop_index) {
                        face_vertex_count = count;
                        for (int j = 0; j < count && j < 256; j++) {
                            while (*ptr && isspace((unsigned char)*ptr)) ptr++;
                            if (!*ptr) { state->error = PLY_ERROR_PARSE_ERROR; return parsed; }
                            face_vertex_indices[j] = (int)strtol(ptr, &end, 10);
                            ptr = end;
                        }
                    } else {
                        for (int j = 0; j < count; j++) {
                            while (*ptr && isspace((unsigned char)*ptr)) ptr++;
                            if (!*ptr) break;
                            strtof(ptr, &end);
                            ptr = end;
                        }
                    }
                } else {
                    while (*ptr && isspace((unsigned char)*ptr)) ptr++;
                    if (!*ptr) { state->error = PLY_ERROR_PARSE_ERROR; return parsed; }
                    char *end;
                    face_scalar_values[p] = strtof(ptr, &end);
                    ptr = end;
                }
            }

            // Fan triangulation
            int new_tris = (face_vertex_count >= 3) ? (face_vertex_count - 2) : 0;

            // Grow index buffer if needed
            while (state->face_tri_count + new_tris > state->face_tri_capacity) {
                int new_cap = state->face_tri_capacity * 2;
                uint32_t *new_indices = (uint32_t*)realloc(state->face_indices, new_cap * 3 * sizeof(uint32_t));
                if (!new_indices) {
                    state->error = PLY_ERROR_MEMORY_ALLOCATION;
                    return parsed;
                }
                state->face_indices = new_indices;
                state->face_tri_capacity = new_cap;
            }

            for (int t = 0; t < new_tris; t++) {
                int base = (state->face_tri_count + t) * 3;
                state->face_indices[base + 0] = (uint32_t)face_vertex_indices[0];
                state->face_indices[base + 1] = (uint32_t)face_vertex_indices[t + 1];
                state->face_indices[base + 2] = (uint32_t)face_vertex_indices[t + 2];
            }
            state->face_tri_count += new_tris;

            // Extract face color if present
            int f = state->face_parsed_count + parsed;
            if (state->has_face_colors && state->face_colors) {
                float r = face_scalar_values[state->header.face_prop_red];
                float g = face_scalar_values[state->header.face_prop_green];
                float b = face_scalar_values[state->header.face_prop_blue];
                float a = (state->header.face_prop_alpha >= 0) ? face_scalar_values[state->header.face_prop_alpha] : 255.0f;

                ply_property_type_t r_type = state->header.face_properties[state->header.face_prop_red].type;
                if (r_type == PLY_PROP_UCHAR || r_type == PLY_PROP_CHAR ||
                    r_type == PLY_PROP_USHORT || r_type == PLY_PROP_SHORT ||
                    r_type == PLY_PROP_UINT || r_type == PLY_PROP_INT) {
                    r /= 255.0f;
                    g /= 255.0f;
                    b /= 255.0f;
                    a /= 255.0f;
                }

                state->face_colors[f] = vec4_make(r, g, b, a);
            }

            parsed++;
        }

        state->face_parsed_count += parsed;
        return parsed;
    } else {
        // --- Binary face parsing ---

        // Skip intermediate element bytes (once)
        if (!state->parsing_faces) {
            if (state->header.skip_bytes_before_faces > 0) {
                if (fseek(state->file, state->header.skip_bytes_before_faces, SEEK_CUR) != 0) {
                    state->error = PLY_ERROR_PARSE_ERROR;
                    return 0;
                }
            }
            state->parsing_faces = true;
        }

        int remaining = state->face_total - state->face_parsed_count;
        if (remaining <= 0) return 0;

        int to_parse = (remaining < max_faces) ? remaining : max_faces;

        bool need_swap = (state->header.is_little_endian != ply_is_little_endian());
        uint8_t buf[8];
        int parsed = 0;

        for (int i = 0; i < to_parse; i++) {
            int face_vertex_indices[256];
            int face_vertex_count = 0;
            float face_scalar_values[PLY_MAX_PROPERTIES];

            // Parse face properties in declared order
            for (int p = 0; p < state->header.face_property_count; p++) {
                const ply_property_t *prop = &state->header.face_properties[p];

                if (prop->is_list) {
                    int count_size = ply_property_byte_size(prop->list_count_type);
                    if (fread(buf, 1, count_size, state->file) != (size_t)count_size) {
                        state->error = PLY_ERROR_PARSE_ERROR;
                        return parsed;
                    }
                    int count = ply_read_as_int32(buf, prop->list_count_type, need_swap);

                    if (p == state->header.face_list_prop_index) {
                        face_vertex_count = count;
                        int val_size = ply_property_byte_size(prop->type);
                        for (int j = 0; j < count && j < 256; j++) {
                            if (fread(buf, 1, val_size, state->file) != (size_t)val_size) {
                                state->error = PLY_ERROR_PARSE_ERROR;
                                return parsed;
                            }
                            face_vertex_indices[j] = ply_read_as_int32(buf, prop->type, need_swap);
                        }
                    } else {
                        int val_size = ply_property_byte_size(prop->type);
                        for (int j = 0; j < count; j++) {
                            if (fread(buf, 1, val_size, state->file) != (size_t)val_size) {
                                state->error = PLY_ERROR_PARSE_ERROR;
                                return parsed;
                            }
                        }
                    }
                } else {
                    int sz = ply_property_byte_size(prop->type);
                    if (fread(buf, 1, sz, state->file) != (size_t)sz) {
                        state->error = PLY_ERROR_PARSE_ERROR;
                        return parsed;
                    }
                    face_scalar_values[p] = ply_read_as_float(buf, prop->type, need_swap);
                }
            }

            // Fan triangulation
            int new_tris = (face_vertex_count >= 3) ? (face_vertex_count - 2) : 0;

            // Grow index buffer if needed
            while (state->face_tri_count + new_tris > state->face_tri_capacity) {
                int new_cap = state->face_tri_capacity * 2;
                uint32_t *new_indices = (uint32_t*)realloc(state->face_indices, new_cap * 3 * sizeof(uint32_t));
                if (!new_indices) {
                    state->error = PLY_ERROR_MEMORY_ALLOCATION;
                    return parsed;
                }
                state->face_indices = new_indices;
                state->face_tri_capacity = new_cap;
            }

            for (int t = 0; t < new_tris; t++) {
                int base = (state->face_tri_count + t) * 3;
                state->face_indices[base + 0] = (uint32_t)face_vertex_indices[0];
                state->face_indices[base + 1] = (uint32_t)face_vertex_indices[t + 1];
                state->face_indices[base + 2] = (uint32_t)face_vertex_indices[t + 2];
            }
            state->face_tri_count += new_tris;

            // Extract face color if present
            int f = state->face_parsed_count + parsed;
            if (state->has_face_colors && state->face_colors) {
                float r = face_scalar_values[state->header.face_prop_red];
                float g = face_scalar_values[state->header.face_prop_green];
                float b = face_scalar_values[state->header.face_prop_blue];
                float a = (state->header.face_prop_alpha >= 0) ? face_scalar_values[state->header.face_prop_alpha] : 1.0f;

                ply_property_type_t r_type = state->header.face_properties[state->header.face_prop_red].type;
                if (r_type == PLY_PROP_UCHAR || r_type == PLY_PROP_CHAR ||
                    r_type == PLY_PROP_USHORT || r_type == PLY_PROP_SHORT ||
                    r_type == PLY_PROP_UINT || r_type == PLY_PROP_INT) {
                    r /= 255.0f;
                    g /= 255.0f;
                    b /= 255.0f;
                    a /= 255.0f;
                }

                state->face_colors[f] = vec4_make(r, g, b, a);
            }

            parsed++;
        }

        state->face_parsed_count += parsed;
        return parsed;
    }
}

//------------------------------------------------------------------------------
// Get current parsing progress (0.0 - 1.0), accounts for both vertices and faces
//------------------------------------------------------------------------------

static inline float ply_get_progress(const ply_parse_state_t *state) {
    int total = state->header.vertex_count + state->face_total;
    if (total <= 0) return 1.0f;
    return (float)(state->parsed_count + state->face_parsed_count) / (float)total;
}

//------------------------------------------------------------------------------
// Check if parsing is complete (both vertices and faces)
//------------------------------------------------------------------------------

static inline bool ply_is_complete(const ply_parse_state_t *state) {
    return state->parsed_count >= state->header.vertex_count &&
           state->face_parsed_count >= state->face_total;
}

//------------------------------------------------------------------------------
// Close file handle (caller takes ownership of data arrays)
//------------------------------------------------------------------------------

static inline void ply_close(ply_parse_state_t *state) {
    if (state->file) {
        fclose(state->file);
        state->file = NULL;
    }
    // Note: data arrays are not freed here - caller takes ownership
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
    if (state->face_indices) {
        free(state->face_indices);
        state->face_indices = NULL;
    }
    if (state->face_colors) {
        free(state->face_colors);
        state->face_colors = NULL;
    }
    state->parsed_count = 0;
    state->capacity = 0;
    state->face_parsed_count = 0;
    state->face_tri_count = 0;
    state->face_tri_capacity = 0;
}

//------------------------------------------------------------------------------
// Transfer ownership of parsed data to ply_data_t (point cloud, backward compat)
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

//------------------------------------------------------------------------------
// Transfer ownership of all parsed data to ply_mesh_data_t
//------------------------------------------------------------------------------

static inline void ply_parse_state_to_mesh_data(ply_parse_state_t *state, ply_mesh_data_t *mesh) {
    mesh->vertices = state->points;
    mesh->vertex_colors = state->colors;
    mesh->vertex_count = state->parsed_count;
    mesh->has_vertex_colors = state->has_colors;
    mesh->min_bounds = state->min_bounds;
    mesh->max_bounds = state->max_bounds;

    mesh->faces.indices = state->face_indices;
    mesh->faces.colors = state->face_colors;
    mesh->faces.tri_count = state->face_tri_count;
    mesh->faces.face_count = state->face_parsed_count;
    mesh->has_face_colors = state->has_face_colors;

    // Clear state pointers (ownership transferred)
    state->points = NULL;
    state->colors = NULL;
    state->parsed_count = 0;
    state->capacity = 0;
    state->face_indices = NULL;
    state->face_colors = NULL;
    state->face_parsed_count = 0;
    state->face_tri_count = 0;
    state->face_tri_capacity = 0;
}

#endif // PLY_LOADER_H
