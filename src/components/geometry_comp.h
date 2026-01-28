//------------------------------------------------------------------------------
// geometry_comp.h - Geometry component for ECS entities
//------------------------------------------------------------------------------
#ifndef GEOMETRY_COMP_H
#define GEOMETRY_COMP_H

#include "component_types.h"

typedef enum {
    GEOM_POINT,
    GEOM_LINE,
    GEOM_POLYLINE,
    GEOM_ARC,
    GEOM_POLYGON,
    GEOM_HELIX,
    GEOM_BEZIER,        // Cubic bezier curve
    GEOM_TYPE_COUNT
} geometry_type_t;

// Point geometry data
typedef struct {
    vec3_t point;
} geom_point_data_t;

// Line geometry data
typedef struct {
    vec3_t a, b;
} geom_line_data_t;

// Polyline geometry data (dynamically allocated points)
typedef struct {
    vec3_t *points;
    int count;
    int capacity;
} geom_polyline_data_t;

// Arc geometry data
typedef struct {
    vec3_t center;
    float radius;
    float start_angle;
    float end_angle;
    vec3_t normal;
} geom_arc_data_t;

// Polygon geometry data (dynamically allocated, closed outline)
typedef struct {
    vec3_t *points;
    int count;
    int capacity;
    bool closed;  // Always true for polygon
} geom_polygon_data_t;

// Helix geometry data
typedef struct {
    vec3_t axis_start;
    vec3_t axis_end;
    float radius;
    float turns;
    int segments;
} geom_helix_data_t;

// Bezier curve geometry data (cubic, 4 control points)
typedef struct {
    vec3_t p0, p1, p2, p3;
    int segments;
} geom_bezier_data_t;

typedef struct {
    geometry_type_t type;

    // Geometry data (union per type)
    union {
        geom_point_data_t point;
        geom_line_data_t line;
        geom_polyline_data_t polyline;
        geom_arc_data_t arc;
        geom_polygon_data_t polygon;
        geom_helix_data_t helix;
        geom_bezier_data_t bezier;
    } data;

    // Rendering properties
    float line_width;       // Thickness for lines/polylines
    float point_size;       // Size for points
    vec4_t color;           // RGBA color
} GeometryComp;

//------------------------------------------------------------------------------
// Geometry creation helpers
//------------------------------------------------------------------------------

static inline GeometryComp geometry_comp_point(vec3_t point, vec4_t color, float size) {
    return (GeometryComp){
        .type = GEOM_POINT,
        .data.point = { .point = point },
        .point_size = size,
        .line_width = 0.0f,
        .color = color
    };
}

static inline GeometryComp geometry_comp_line(vec3_t a, vec3_t b, vec4_t color, float width) {
    return (GeometryComp){
        .type = GEOM_LINE,
        .data.line = { .a = a, .b = b },
        .line_width = width,
        .point_size = 0.0f,
        .color = color
    };
}

static inline GeometryComp geometry_comp_arc(vec3_t center, float radius,
                                              float start_angle, float end_angle,
                                              vec3_t normal, vec4_t color, float width) {
    return (GeometryComp){
        .type = GEOM_ARC,
        .data.arc = {
            .center = center,
            .radius = radius,
            .start_angle = start_angle,
            .end_angle = end_angle,
            .normal = normal
        },
        .line_width = width,
        .point_size = 0.0f,
        .color = color
    };
}

static inline GeometryComp geometry_comp_bezier(vec3_t p0, vec3_t p1, vec3_t p2, vec3_t p3,
                                                 int segments, vec4_t color, float width) {
    return (GeometryComp){
        .type = GEOM_BEZIER,
        .data.bezier = {
            .p0 = p0, .p1 = p1, .p2 = p2, .p3 = p3,
            .segments = segments
        },
        .line_width = width,
        .point_size = 0.0f,
        .color = color
    };
}

static inline GeometryComp geometry_comp_helix(vec3_t axis_start, vec3_t axis_end,
                                                float radius, float turns, int segments,
                                                vec4_t color, float width) {
    return (GeometryComp){
        .type = GEOM_HELIX,
        .data.helix = {
            .axis_start = axis_start,
            .axis_end = axis_end,
            .radius = radius,
            .turns = turns,
            .segments = segments
        },
        .line_width = width,
        .point_size = 0.0f,
        .color = color
    };
}

// Get geometry type name for display
static inline const char* geometry_type_name(geometry_type_t type) {
    static const char* names[] = {
        "Point", "Line", "Polyline", "Arc", "Polygon", "Helix", "Bezier"
    };
    if (type >= 0 && type < GEOM_TYPE_COUNT) {
        return names[type];
    }
    return "Unknown";
}

//------------------------------------------------------------------------------
// Polyline/polygon dynamic allocation helpers
//------------------------------------------------------------------------------

#include <stdlib.h>
#include <string.h>

static inline void geom_polyline_init(geom_polyline_data_t *pl, int initial_capacity) {
    pl->points = (vec3_t*)malloc(sizeof(vec3_t) * initial_capacity);
    pl->count = 0;
    pl->capacity = initial_capacity;
}

static inline void geom_polyline_add_point(geom_polyline_data_t *pl, vec3_t point) {
    if (pl->count >= pl->capacity) {
        pl->capacity *= 2;
        pl->points = (vec3_t*)realloc(pl->points, sizeof(vec3_t) * pl->capacity);
    }
    pl->points[pl->count++] = point;
}

static inline void geom_polyline_free(geom_polyline_data_t *pl) {
    if (pl->points) {
        free(pl->points);
        pl->points = NULL;
    }
    pl->count = 0;
    pl->capacity = 0;
}

static inline void geom_polygon_init(geom_polygon_data_t *pg, int initial_capacity) {
    pg->points = (vec3_t*)malloc(sizeof(vec3_t) * initial_capacity);
    pg->count = 0;
    pg->capacity = initial_capacity;
    pg->closed = true;
}

static inline void geom_polygon_add_point(geom_polygon_data_t *pg, vec3_t point) {
    if (pg->count >= pg->capacity) {
        pg->capacity *= 2;
        pg->points = (vec3_t*)realloc(pg->points, sizeof(vec3_t) * pg->capacity);
    }
    pg->points[pg->count++] = point;
}

static inline void geom_polygon_free(geom_polygon_data_t *pg) {
    if (pg->points) {
        free(pg->points);
        pg->points = NULL;
    }
    pg->count = 0;
    pg->capacity = 0;
}

// Create polyline from array of points (copies the points)
static inline GeometryComp geometry_comp_polyline(vec3_t *points, int count,
                                                   vec4_t color, float width) {
    GeometryComp g = {
        .type = GEOM_POLYLINE,
        .line_width = width,
        .point_size = 0.0f,
        .color = color
    };
    geom_polyline_init(&g.data.polyline, count);
    for (int i = 0; i < count; i++) {
        g.data.polyline.points[i] = points[i];
    }
    g.data.polyline.count = count;
    return g;
}

// Create polygon from array of points (copies the points)
static inline GeometryComp geometry_comp_polygon(vec3_t *points, int count,
                                                  vec4_t color, float width) {
    GeometryComp g = {
        .type = GEOM_POLYGON,
        .line_width = width,
        .point_size = 0.0f,
        .color = color
    };
    geom_polygon_init(&g.data.polygon, count);
    for (int i = 0; i < count; i++) {
        g.data.polygon.points[i] = points[i];
    }
    g.data.polygon.count = count;
    return g;
}

// Free dynamic allocations in geometry component
static inline void geometry_comp_free(GeometryComp *g) {
    switch (g->type) {
        case GEOM_POLYLINE:
            geom_polyline_free(&g->data.polyline);
            break;
        case GEOM_POLYGON:
            geom_polygon_free(&g->data.polygon);
            break;
        default:
            break;
    }
}

#endif // GEOMETRY_COMP_H
