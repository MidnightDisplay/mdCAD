# JSONL Geometry Log Importer - Implementation Plan

## Context

The project already has a PLY point cloud importer with file browser, options dialog, chunked import with progress bar, and timing plot. We need an analogous importer for `.jsonl` geometry log files exported from a .NET geometry library. These files contain mixed geometry (lines, arcs, points, polylines, polygons) organized into named log entries, which we map to our ECS scene hierarchy.

## JSONL Format Summary

Each line is a JSON object representing a `GeometryLogEntry`:
```json
{"$type":"...GeometryLogEntry...","Name":"Part Edges","Description":"","Elements":[
  {"$type":"...GeometryLogElement...","Name":"BBox Line","Description":"",
   "Element":{"$type":"...Line3D...","StartPoint":{"$type":"...Point3D...","X":1.0,"Y":2.0,"Z":3.0},
              "EndPoint":{"$type":"...Point3D...","X":4.0,"Y":5.0,"Z":6.0}},
   "Colour":"50, 79, 190, 0"}
]}
```

**Key design rule**: The parser must NOT rely on hardcoded .NET namespaces. We match only the class name suffix (e.g., `Point3D`, `Line3D`, `Arc3D`, `PolyLine3D`, `Polygon3D`).

## Entity Hierarchy

```
Root Anchor (invisible GEOM_POINT at 0,0,0)
├── Entry "Part Edges" (invisible GEOM_POINT at 0,0,0)
│   ├── GEOM_LINE "BBox Line"
│   ├── GEOM_LINE "BBox Line"
│   └── ...
├── Entry "Sliced Faces" (invisible GEOM_POINT at 0,0,0)
│   ├── GEOM_ARC "Circular/Arc Edge"
│   ├── GEOM_LINE "Linear Edge"
│   └── ...
```

- All `TransformComp` positions are (0,0,0) - geometry coordinates go into `GeometryComp` data
- Anchor nodes use `scene_add_point()` then `scene_set_visible(scene, entity, false)` to hide them
- Scale and CoM shift are baked into geometry coordinates before entity creation

## Geometry Type Mapping

| .NET Class | ECS Type | Data Extraction |
|---|---|---|
| `Point3D` | `GEOM_POINT` | X, Y, Z |
| `Line3D` | `GEOM_LINE` | StartPoint, EndPoint |
| `Arc3D` | `GEOM_ARC` | Circle.Center, Circle.Radius, Circle.Axis -> normal, StartPoint/EndPoint -> angles, Direction -> winding |
| `PolyLine3D` | `GEOM_POLYLINE` | Points[] (IEnumerable<Point3D>), open |
| `Polygon3D` | `GEOM_POLYGON` | Points[] (IEnumerable<Point3D>), closed |

### Arc3D Conversion

The .NET `Arc3D` stores: `Circle{Center, Radius, Axis}`, `StartPoint`, `EndPoint`, `Direction`.
Our `geom_arc_data_t` stores: `center`, `radius`, `start_angle`, `end_angle`, `normal`.

Conversion algorithm:
1. `center` = Circle.Center, `radius` = Circle.Radius, `normal` = normalize(Circle.Axis)
2. Build local coordinate system from normal (same method as `ecs_scene_tessellate_arc`):
   - `x_axis` = normalize(cross(arbitrary, normal))
   - `y_axis` = cross(normal, x_axis)
3. `start_angle` = atan2(dot(StartPoint - center, y_axis), dot(StartPoint - center, x_axis))
4. `end_angle` = atan2(dot(EndPoint - center, y_axis), dot(EndPoint - center, x_axis))
5. Direction handling:
   - Direction=1 (CCW): if end_angle <= start_angle, add 2*PI to end_angle
   - Direction=-1 (CW): if end_angle >= start_angle, subtract 2*PI from end_angle

### Colour Parsing

Format: `"R, G, B"` or `"R, G, B, A"` (0-255 integer range).
**Alpha always defaults to 255** (opaque) regardless of file value.
Parse by splitting on commas, converting to float 0-1 range: `value / 255.0f`.

## Import Options (Dialog)

Mirroring the PLY import dialog:
- **File info**: path, entry count, total element count
- **Units**: Meters (1:1), Millimeters (0.001), Inches (0.0254)
- **Import Colours**: checkbox (use JSONL colours or default colour picker)
- **Shift to Centre of Mass**: checkbox
  - CoM computed from: point positions, line start/end points, arc centre points, polyline/polygon vertex positions
- **Rotation X/Y/Z**: drag floats (degrees), applied after CoM shift

## Import Job State Machine

States (analogous to `ply_import_job.h`):
```
JSONL_JOB_IDLE
JSONL_JOB_PARSING_FILE        → Read and parse JSONL (chunked by lines)
JSONL_JOB_CREATING_ENTITIES   → Create ECS geometry entities (chunked)
JSONL_JOB_PARENTING_ENTITIES  → Set parent-child relationships (chunked)
JSONL_JOB_COMPLETE
JSONL_JOB_CANCELLED
JSONL_JOB_ERROR
```

### Progress Weighting
- Parsing: 0-20%
- Entity creation: 20-80%
- Parenting: 80-100%

### Chunk Sizes
- `JSONL_PARSE_CHUNK_SIZE`: 50 lines per frame
- `JSONL_ENTITY_CHUNK_SIZE`: 200 entities per frame
- `JSONL_PARENT_CHUNK_SIZE`: 500 parenting operations per frame
- `JSONL_SYNC_THRESHOLD`: 100 total elements (below this, import synchronously)

### Timing & Plot
Same pattern as PLY: ring buffer of iteration times, sampled every ~1% progress, displayed with `igPlotLines_FloatPtr`.

## Files to Create

### 1. `vendors/cjson/cJSON.h` + `vendors/cjson/cJSON.c`
Download from https://github.com/DaveGamble/cJSON (MIT license, single-file JSON parser).

### 2. `vendors/cjson/CMakeLists.txt`
```cmake
add_library(cjson STATIC cJSON.c)
target_include_directories(cjson PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
```

### 3. `src/jsonl_loader.h` - JSONL Parser (header-only)

**Data structures:**
```c
// Error codes
typedef enum {
    JSONL_OK = 0,
    JSONL_ERROR_FILE_NOT_FOUND,
    JSONL_ERROR_PARSE_ERROR,
    JSONL_ERROR_MEMORY_ALLOCATION,
} jsonl_error_t;

// Geometry element types
typedef enum {
    JSONL_GEOM_POINT,
    JSONL_GEOM_LINE,
    JSONL_GEOM_ARC,
    JSONL_GEOM_POLYLINE,
    JSONL_GEOM_POLYGON,
    JSONL_GEOM_UNKNOWN,
} jsonl_geom_type_t;

// Single parsed geometry element
typedef struct {
    jsonl_geom_type_t type;
    char name[128];
    vec4_t colour;          // RGBA, alpha always 1.0

    union {
        struct { vec3_t point; } point;
        struct { vec3_t start, end; } line;
        struct {
            vec3_t center;
            float radius;
            vec3_t normal;          // Circle.Axis
            float start_angle;     // computed
            float end_angle;       // computed
        } arc;
        struct {
            vec3_t *points;
            int count;
        } polyline;  // also used for polygon
    } data;
} jsonl_element_t;

// One log entry (one JSONL line)
typedef struct {
    char name[128];
    jsonl_element_t *elements;
    int element_count;
    int element_capacity;
} jsonl_log_entry_t;

// Complete parsed file
typedef struct {
    jsonl_log_entry_t *entries;
    int entry_count;
    int entry_capacity;
    int total_elements;     // sum of all elements across entries
} jsonl_data_t;

// Incremental parse state
typedef struct {
    FILE *file;
    jsonl_data_t data;
    int lines_parsed;
    int total_lines;        // counted in first pass
    jsonl_error_t error;
} jsonl_parse_state_t;
```

**Key functions:**
- `jsonl_count_lines(filepath)` - quick line count for progress
- `jsonl_open(filepath, state)` - open file, count lines
- `jsonl_parse_lines_chunk(state, max_lines)` - parse N lines per call
- `jsonl_get_progress(state)` - 0.0-1.0
- `jsonl_is_complete(state)` - bool
- `jsonl_close(state)` - close file, keep data
- `jsonl_data_free(data)` - free all dynamic allocations
- `jsonl_parse_state_free(state)` - free everything including file

**Internal helpers:**
- `jsonl_extract_class_name(type_str)` - extracts e.g. "Line3D" from `"Geo.NET.Geometry.Line3D, Geo.NET Core"`
- `jsonl_parse_point3d(json)` - extracts vec3_t from `{"X":...,"Y":...,"Z":...}` cJSON object
- `jsonl_parse_colour(str)` - parses "R, G, B" or "R, G, B, A" string to vec4_t (alpha=1.0)
- `jsonl_parse_arc3d(json, element)` - full arc conversion including angle computation
- `jsonl_parse_element(json, element)` - dispatches to type-specific parser
- `jsonl_parse_entry(json_str, entry)` - parses one JSONL line into an entry

### 4. `src/jsonl_import_job.h` - Import Job State Machine (header-only)

Closely follows `ply_import_job.h` patterns:

**Structure:**
```c
typedef struct {
    jsonl_job_state_t state;
    char filepath[512];

    // Import options
    float scale;
    bool use_jsonl_colours;
    vec4_t default_colour;
    bool shift_to_com;
    float rotation_x, rotation_y, rotation_z;  // radians

    // Parse state
    jsonl_parse_state_t parse_state;

    // Entity creation state
    int current_entry_idx;      // which log entry we're processing
    int current_element_idx;    // which element within that entry
    int total_entities_created;
    ecs_entity_t root_entity;
    ecs_entity_t *entry_entities;  // one per log entry (sub-anchors)

    // Parenting state
    int parented_count;
    int total_to_parent;
    ecs_entity_t *all_created_entities;   // flat array of all geometry entities
    int *entity_to_entry_map;             // maps each entity to its entry index

    // Computed transforms
    vec3_t com;
    mat4_t transform_matrix;
    bool transforms_applied;

    // Progress & timing (same pattern as PLY)
    float progress;
    char status_message[128];
    double last_iteration_time_ms;
    float iteration_times[100];
    float iteration_progress[100];
    int timing_sample_count;
    float last_sampled_progress;

    // Result
    jsonl_error_t error;
    int total_elements;
} jsonl_import_job_t;
```

**Key functions:**
- `jsonl_import_job_init(job)` - zero state
- `jsonl_import_job_start(job, filepath, scale, ...)` - begin import
- `jsonl_import_job_tick(job, scene)` - process one chunk, returns true when done
- `jsonl_import_job_cancel(job, scene)` - cancel with cleanup
- `jsonl_import_job_is_running(job)` - bool
- `jsonl_import_job_reset(job)` - reset to idle

**Transform application** (after parsing, before entity creation):
1. Collect all characteristic points (point positions, line endpoints, arc centres, polyline/polygon vertices)
2. If shift_to_com: compute CoM from all characteristic points, subtract from all coordinates
3. If rotation: build rotation matrix, apply to all coordinates
4. Apply scale factor to all coordinates

## Files to Modify

### 5. `vendors/CMakeLists.txt`
Add: `add_subdirectory(cjson)`

### 6. `src/CMakeLists.txt`
Add `cjson` to `target_link_libraries`.

### 7. `src/ui/ui_scene_hierarchy.h`

**Add to includes:**
```c
#include "../jsonl_loader.h"
#include "../jsonl_import_job.h"
```

**Add to state struct** (`ui_scene_hierarchy_state_t`):
```c
// JSONL Import state
file_browser_t jsonl_browser;
bool jsonl_import_popup_open;
char jsonl_import_path[512];
int jsonl_entry_count;
int jsonl_element_count;
int jsonl_unit_index;
bool jsonl_use_colours;
float jsonl_default_colour[3];
bool jsonl_shift_to_com;
float jsonl_rotation[3];

// JSONL Import job
jsonl_import_job_t jsonl_import_job;
bool jsonl_import_progress_popup_open;
```

**Add to init function:**
- Initialize `jsonl_browser`, default options

**Add to shutdown function:**
- Shutdown `jsonl_browser`, cleanup job

**Add to File menu:**
```c
if (igMenuItem_Bool("Import JSONL Geometry Log...", NULL, false, true)) {
    file_browser_open_file(&state->jsonl_browser, "Import JSONL", ".jsonl",
                           state->last_folder[0] ? state->last_folder : NULL);
}
```

**Add JSONL file browser handler:**
- On file selection: quick-scan the file (count lines + elements), open import options popup

**Add "Import JSONL Options" modal popup:**
- File info (path, entries, elements)
- Units combo (M/mm/inch)
- Import Colours checkbox + colour picker
- Shift to CoM checkbox
- Rotation X/Y/Z drag floats
- Import / Cancel buttons

**Add "Importing JSONL Geometry Log" progress modal:**
- Same pattern as PLY progress: filename, progress bar, status message, timing info, speed plot, Cancel/Close button
- Ticks the import job each frame

## Implementation Order

1. Add cJSON vendor library (download + CMake)
2. Create `jsonl_loader.h` with parsing logic
3. Create `jsonl_import_job.h` with state machine
4. Modify `ui_scene_hierarchy.h` to add menu, dialog, progress
5. Update `vendors/CMakeLists.txt` and `src/CMakeLists.txt`
6. Build and test with `example.jsonl`

## Verification

1. `cmake -B build -G Ninja && ninja -C build` - must compile without errors
2. Run `./build/bin/skl_tmp`
3. File > Import JSONL Geometry Log... > select `example.jsonl`
4. Verify import options dialog shows correct counts (4 entries, ~41 elements)
5. Import with default options - geometry appears in viewport
6. Verify scene hierarchy shows correct parent-child structure
7. Test CoM shift option - geometry re-centered
8. Test scale option (mm) - geometry scaled correctly
9. Test cancel during import (create a large test file if needed)
10. Test with colours enabled/disabled
