# PLY Point Cloud Loader Implementation Plan

## Overview

Load iPad LiDAR point cloud PLY files into the ECS scene. Two import modes available:
1. **Editable Subtree** - Individual point entities as children of a root
2. **Point Cloud Node** - Single entity with all points (more efficient)

## Import Modes (User Choice)

### Mode 1: Editable Subtree (Individual Entities)
```
Root Point Entity (at 0,0,0)
├── TransformComp.scale = unit_conversion_factor
├── GeometryComp.point = (0,0,0)
└── Children:
    ├── Point 1 (child entity)
    ├── Point 2 (child entity)
    └── ... N child entities
```
- Each point is a selectable ECS entity
- Can edit individual point colors, positions
- Heavier on memory and slower for large clouds
- Best for: Small clouds (<10k), when editing individual points is needed

### Mode 2: Point Cloud Node (GEOM_POINT_CLOUD)
```
Point Cloud Entity
└── GeometryComp.type = GEOM_POINT_CLOUD
    └── data.point_cloud.points[] = all coordinates
    └── data.point_cloud.colors[] = all colors
    └── data.point_cloud.count = N
```
- Single entity with all points in one geometry component
- Dedicated GPU buffer for efficient rendering
- Points NOT individually selectable (select whole cloud)
- Best for: Large clouds (10k+), visualization-only use cases

## Unit Conversion

| Unit | Scale Factor | Applied To |
|------|-------------|------------|
| Meters (M) | 1.0 | TransformComp.scale |
| Millimeters (mm) | 0.001 | TransformComp.scale |
| Inches (inch) | 0.0254 | TransformComp.scale |

## New Geometry Type: GEOM_POINT_CLOUD

### Add to `src/components/geometry_comp.h`:

```c
// Add to geometry_type_t enum
GEOM_POINT_CLOUD = 7,

// Add point cloud data structure
typedef struct {
    vec3_t *points;      // Dynamically allocated array
    vec4_t *colors;      // Dynamically allocated array (optional, NULL if uniform color)
    int count;           // Number of points
    int capacity;        // Allocated capacity
} geom_point_cloud_data_t;

// Add to GeometryComp union
union {
    // ... existing types ...
    geom_point_cloud_data_t point_cloud;
} data;
```

### Add to `src/gpu/geometry_batch.h`:

New batch type for point clouds with dedicated instance buffer:
- Per-point: vec3 position + vec4 color (28 bytes per point)
- Single draw call for entire cloud

### Add to `src/ecs/ecs_scene.h`:

```c
ecs_entity_t scene_add_point_cloud(
    ecs_scene_t *scene,
    vec3_t *points,
    vec4_t *colors,      // NULL for uniform color
    int count,
    vec4_t uniform_color,
    float point_size
);
```

## Files to Create/Modify

### NEW FILES:
1. `src/ply_loader.h` - Header-only PLY parser with both import modes

### MODIFY FILES:
1. `src/components/geometry_comp.h` - Add GEOM_POINT_CLOUD type
2. `src/gpu/geometry_batch.h` - Add point cloud batch rendering
3. `src/ecs/ecs_scene.h` - Add `scene_add_point_cloud()` API
4. `src/ui/ui_scene_hierarchy.h` - Add import UI with mode selection
5. `src/shaders/pick_shaders.h` - Add point cloud picking support

## PLY Format Support

**ASCII PLY (required):**
```ply
ply
format ascii 1.0
element vertex 1000
property float x
property float y
property float z
property uchar red
property uchar green
property uchar blue
end_header
0.123 0.456 0.789 255 128 64
...
```

**Binary PLY (phase 2):** `format binary_little_endian 1.0`

## Import Options UI

```
┌─────────────────────────────────────────┐
│ Import PLY Point Cloud                  │
├─────────────────────────────────────────┤
│ File: /path/to/scan.ply                 │
│ Points: 50,000                          │
│                                         │
│ Import Mode:                            │
│ ○ Editable Subtree (individual points)  │
│ ● Point Cloud Node (single entity)      │
│                                         │
│ Units: [Millimeters ▼]                  │
│ Point Size: [====○====] 0.005           │
│                                         │
│ ☑ Use PLY colors                        │
│ Default Color: [■] (when no PLY colors) │
│                                         │
│ [Import]  [Cancel]                      │
└─────────────────────────────────────────┘
```

## Implementation Phases

### Phase 1: Core Infrastructure ✅ COMPLETED
1. ✅ Add GEOM_POINT_CLOUD to geometry_comp.h
2. ✅ Add point cloud batch to geometry_batch.h
3. ✅ Add scene_add_point_cloud() to ecs_scene.h
4. ✅ Update pick buffer for point cloud type

### Phase 2: PLY Loader ✅ COMPLETED
5. ✅ Create ply_loader.h with header parsing
6. ✅ Add ASCII vertex parsing
7. ✅ Implement both import modes
8. ✅ Add unit conversion via transform scale

### Phase 3: UI Integration ✅ COMPLETED
9. ✅ Add import state to ui_scene_hierarchy.h
10. ✅ Add File > Import PLY menu item
11. ✅ Create import options popup with mode selection
12. ✅ Wire up file browser for .ply files

## Implementation Summary

All three phases implemented in session 2026-02-03.

### Files Created
- `src/ply_loader.h` - Header-only PLY file parser (ASCII format)

### Files Modified
- `src/components/geometry_comp.h`:
  - Added `GEOM_POINT_CLOUD` enum value (7)
  - Added `geom_point_cloud_data_t` struct
  - Added helper functions: `geom_point_cloud_init()`, `geom_point_cloud_add_point()`, `geom_point_cloud_free()`
  - Added `geometry_comp_point_cloud()` factory function
  - Updated `geometry_comp_free()` to handle point clouds
  - Updated `geometry_type_name()` to include "Point Cloud"

- `src/gpu/geometry_batch.h`:
  - Added include for `geometry_comp.h`
  - Added `geom_point_cloud_batch_alloc()` - allocates contiguous slots
  - Added `geom_point_cloud_batch_set()` - sets point cloud data
  - Added `geom_point_cloud_batch_set_entity()` - entity mapping
  - Added `geom_point_cloud_batch_free()` - frees slots

- `src/ecs/ecs_scene.h`:
  - Added `scene_add_point_cloud()` - creates point cloud entity
  - Updated `scene_free_entity_slots()` - handles GEOM_POINT_CLOUD
  - Updated `ecs_scene_update()` - visibility/dirty handling for point clouds
  - Updated `ecs_scene_populate_pick_buffer()` - sampled picking for large clouds

- `src/ui/ui_scene_hierarchy.h`:
  - Added PLY import state fields to `ui_scene_hierarchy_state_t`
  - Added "Import PLY Point Cloud..." menu item in File menu
  - Added `Import PLY Options` popup modal with all import options
  - Added PLY import execution with unit scaling

### Build Status
✅ Compiles successfully with `cmake -B build -G Ninja && ninja -C build`
✅ Test PLY files available in `scripts/` directory

## Color Handling

PLY colors are typically uint8 (0-255), convert to float (0.0-1.0):
```c
vec4_t color = vec4_make(
    (float)r / 255.0f,
    (float)g / 255.0f,
    (float)b / 255.0f,
    has_alpha ? (float)a / 255.0f : 1.0f
);
```

## Error Handling

- File not found
- Invalid PLY header (missing "ply" magic)
- Unsupported format (binary when only ASCII supported)
- Missing required properties (x, y, z)
- Malformed data lines
- Memory allocation failure

## Verification

1. Build: `cmake -B build -G Ninja && ninja -C build`
2. Run: `./build/bin/skl_tmp`
3. Test: File > Import PLY Point Cloud...
4. Select a `.ply` file from iPad LiDAR
5. Verify: Points appear in scene
6. Verify: Mode selection works (editable vs point cloud)
7. Verify: Unit conversion scales correctly
8. Verify: Colors display correctly (if PLY has colors)
9. Verify: Moving entity moves all points
