//------------------------------------------------------------------------------
// gcode_loader.h - G-code file parser (header-only)
//
// Parses G-code files and extracts toolpath coordinates.
// Only processes G1 (linear move) commands with X, Y, Z coordinates.
//------------------------------------------------------------------------------
#ifndef GCODE_LOADER_H
#define GCODE_LOADER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>

//------------------------------------------------------------------------------
// Configuration
//------------------------------------------------------------------------------
#define GCODE_MAX_POINTS 300000
#define GCODE_LINE_BUFFER_SIZE 256

//------------------------------------------------------------------------------
// Types
//------------------------------------------------------------------------------
typedef struct {
    float x, y, z;
} gcode_point_t;

typedef struct {
    gcode_point_t* points;
    float* cumulative_lengths;  // Cumulative path length at each point
    int count;
    int capacity;
    float total_length;

    // Bounding box
    float min_x, min_y, min_z;
    float max_x, max_y, max_z;
} gcode_path_t;

//------------------------------------------------------------------------------
// Helper functions
//------------------------------------------------------------------------------

// Parse a float value from a G-code parameter (e.g., "X123.456" -> 123.456)
static inline bool gcode_parse_param(const char* line, char param, float* value) {
    const char* p = line;
    while (*p) {
        if (*p == param || *p == (param + 32)) {  // Handle both upper and lower case
            p++;
            if (*p == '-' || *p == '+' || (*p >= '0' && *p <= '9') || *p == '.') {
                *value = (float)strtod(p, NULL);
                return true;
            }
        }
        p++;
    }
    return false;
}

// Check if a line is a G1 or G0 command
static inline bool gcode_is_g1_or_g0(const char* line) {
    // Skip whitespace
    while (*line == ' ' || *line == '\t') line++;
    // Check for G1, g1, G0 or g0
    if ((line[0] == 'G' || line[0] == 'g') && (line[1] == '1' || line[1] == '0') &&
        (line[2] == ' ' || line[2] == '\t' || line[2] == '\0' || line[2] == '\r' || line[2] == '\n')) {
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
// Core functions
//------------------------------------------------------------------------------

static inline void gcode_path_init(gcode_path_t* path) {
    path->capacity = 10000;
    path->points = (gcode_point_t*)malloc(path->capacity * sizeof(gcode_point_t));
    path->cumulative_lengths = (float*)malloc(path->capacity * sizeof(float));
    path->count = 0;
    path->total_length = 0.0f;

    path->min_x = path->min_y = path->min_z = 1e9f;
    path->max_x = path->max_y = path->max_z = -1e9f;
}

static inline void gcode_path_add_point(gcode_path_t* path, float x, float y, float z) {
    // Grow arrays if needed
    if (path->count >= path->capacity) {
        path->capacity *= 2;
        if (path->capacity > GCODE_MAX_POINTS) {
            path->capacity = GCODE_MAX_POINTS;
            if (path->count >= path->capacity) return;  // At max capacity
        }
        path->points = (gcode_point_t*)realloc(path->points, path->capacity * sizeof(gcode_point_t));
        path->cumulative_lengths = (float*)realloc(path->cumulative_lengths, path->capacity * sizeof(float));
    }

    // Calculate distance from previous point
    float dist = 0.0f;
    if (path->count > 0) {
        gcode_point_t* prev = &path->points[path->count - 1];
        float dx = x - prev->x;
        float dy = y - prev->y;
        float dz = z - prev->z;
        dist = sqrtf(dx * dx + dy * dy + dz * dz);
    }

    // Add point
    path->points[path->count] = (gcode_point_t){x, y, z};
    path->total_length += dist;
    path->cumulative_lengths[path->count] = path->total_length;
    path->count++;

    // Update bounding box
    if (x < path->min_x) path->min_x = x;
    if (y < path->min_y) path->min_y = y;
    if (z < path->min_z) path->min_z = z;
    if (x > path->max_x) path->max_x = x;
    if (y > path->max_y) path->max_y = y;
    if (z > path->max_z) path->max_z = z;
}

static inline bool gcode_path_load(gcode_path_t* path, const char* filename) {
    FILE* f = fopen(filename, "r");
    if (!f) {
        return false;
    }

    char line[GCODE_LINE_BUFFER_SIZE];
    float current_x = 0.0f, current_y = 0.0f, current_z = 0.0f;
    bool has_position = false;

    while (fgets(line, sizeof(line), f)) {
        // Skip comments and empty lines
        if (line[0] == ';' || line[0] == '\n' || line[0] == '\r') {
            continue;
        }

        // Check for G1 command
        if (gcode_is_g1_or_g0(line)) {
            float x = current_x, y = current_y, z = current_z;
            bool has_x = gcode_parse_param(line, 'X', &x);
            bool has_y = gcode_parse_param(line, 'Y', &y);
            bool has_z = gcode_parse_param(line, 'Z', &z);

            // Only add point if there's actual movement
            if (has_x || has_y || has_z) {
                // Skip pure Z moves at the start (nozzle lift commands)
                if (!has_position && !has_x && !has_y) {
                    current_z = z;
                    continue;
                }

                current_x = x;
                current_y = y;
                current_z = z;
                has_position = true;

                gcode_path_add_point(path, current_x, current_y, current_z);
            }
        }
    }

    fclose(f);
    return path->count > 0;
}

static inline void gcode_path_center(gcode_path_t* path) {
    if (path->count == 0) return;

    // Calculate center
    float cx = (path->min_x + path->max_x) * 0.5f;
    float cy = (path->min_y + path->max_y) * 0.5f;
    float cz = path->min_z;  // Keep Z at base

    // Offset all points to center
    for (int i = 0; i < path->count; i++) {
        path->points[i].x -= cx;
        path->points[i].y -= cy;
        path->points[i].z -= cz;
    }

    // Update bounding box
    path->max_x -= cx;
    path->min_x -= cx;
    path->max_y -= cy;
    path->min_y -= cy;
    path->max_z -= cz;
    path->min_z -= cz;
}

static inline void gcode_path_get_bounds_size(const gcode_path_t* path, float* width, float* height, float* depth) {
    *width = path->max_x - path->min_x;
    *height = path->max_y - path->min_y;
    *depth = path->max_z - path->min_z;
}

static inline void gcode_path_shutdown(gcode_path_t* path) {
    if (path->points) {
        free(path->points);
        path->points = NULL;
    }
    if (path->cumulative_lengths) {
        free(path->cumulative_lengths);
        path->cumulative_lengths = NULL;
    }
    path->count = 0;
    path->capacity = 0;
}

#endif // GCODE_LOADER_H
