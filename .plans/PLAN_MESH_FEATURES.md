# Plan: Mesh Features, PLY Mesh Import & Light Inspector

## Context

With the mesh triangle backbone complete (Sprints 1-6 in `PLAN_MESH_TRIANGLES.md`), we now add **user-facing features** on top of that foundation:

1. **Add Entity menu** - Quick-add triangle and test mesh from the UI
2. **PLY mesh loader** - Import triangle meshes from PLY files (ASCII + binary), with per-vertex and per-face colors, two import modes (fast single-mesh entity vs editable per-triangle entities)
3. **Light entity inspector** - Select lights from scene hierarchy, edit properties in Entity Inspector
4. **OBJ-to-PLY converter** - Python utility script for test asset generation

Broken into 6 sprints with clear, testable deliverables.

---

## Sprint 1: Add Entity Menu - Triangle & Test Mesh ✅ DONE

**Goal:** Extend the Add Entity menu with mesh test commands.

**Files to modify:**
- `src/ui/ui_scene_hierarchy.h` (lines 450-610, `ui_scene_hierarchy_draw_add_menu`)

**Implementation:**

After the existing "Helix" menu item (line 601) and before the undo recording block (line 603), add a new separator and two items:

```
igSeparator();  // --- Mesh section ---

"Triangle" -> scene_add_triangle(scene, a, b, c, random_color)
  vertices: equilateral triangle at origin, radius ~0.8, lying in XY plane

"Test Mesh (Box)" -> scene_add_mesh_box(scene, center, size, random_color)
  center: origin, size: (1.0, 1.0, 1.0)
```

Both already have scene API functions: `scene_add_triangle()` (line ~640 in ecs_scene.h) and `scene_add_mesh_box()` (line ~823).

**Verification:**
- Build and run
- Open Add Entity menu -> see "Triangle" and "Test Mesh (Box)" at the bottom
- Click each -> entity appears in viewport and scene hierarchy
- Undo/redo works for both

---

## Sprint 2: Light Entities in Hierarchy & Inspector ✅ DONE

**Goal:** Make light entities selectable from the scene hierarchy and editable in the Entity Inspector.

### Current State
- Light entities have only `TransformComp` + `LightComp` (no `GeometryComp`, `RenderableComp`, or `SelectableComp`)
- Created via `ecs_new()` directly, bypassing `ecs_world_create_entity()` which adds standard components
- Scene hierarchy queries only `GeometryComp` entities -> lights are invisible
- Entity inspector requires `GeometryComp` for its Geometry section -> no light editing

### 2A: Scene Hierarchy - Separate "Lights" Section

**File:** `src/ui/ui_scene_hierarchy.h`

Add a new function `ui_scene_hierarchy_draw_lights_section()` that:
1. Queries all entities with `LightComp` (using `ecs_query` with `w->LightComp_id`)
2. Draws a collapsible `"Lights"` header (collapsed by default to save space)
3. Inside, lists each light entity as a selectable item:
   - Label format: `"[icon] Directional Light #<id>"` or `"[icon] Point Light #<id>"`
   - Icon: use directional arrow or bulb unicode chars, or just text prefix
4. Click -> `selection_set(sel, entity)` to select
5. Ctrl+click -> `selection_toggle(sel, entity)` for multi-select

Call this function in the main `ui_scene_hierarchy_draw()` function, after the geometry entity list but before the end of the window. Place it after the drag-drop target zone.

### 2B: Entity Inspector - LightComp Section

**File:** `src/ui/ui_entity_inspector.h`

Add `#include "../components/light_comp.h"` at the top.

In `ui_entity_inspector_draw_single()`, after the Renderable section (line ~452), add:

```c
// Light section (for light entities)
LightComp *light = ecs_world_get_light(w, e);
if (light && igCollapsingHeader_TreeNodeFlags("Light", ImGuiTreeNodeFlags_DefaultOpen)) {
    // Type (read-only)
    igText("Type: %s", light->type == LIGHT_DIRECTIONAL ? "Directional" : "Point");

    // Color (editable)
    float lcolor[4] = { light->color.x, light->color.y, light->color.z, light->color.w };
    igColorEdit3("Light Color", lcolor, 0);
    // Update on change, mark lighting dirty

    // Intensity (editable)
    igDragFloat("Intensity", &light->intensity, 0.05f, 0.0f, 10.0f, "%.2f", 0);

    // Direction/position info (derived from transform)
    // Directional: direction = transform.position (normalized display)
    // Point: position = transform.position
}
```

Note: Light property changes take effect immediately since `scene_collect_lights()` reads live component data each frame. No `instance_dirty` flagging needed - the light uniform is rebuilt every frame in `app.c`.

**No undo/redo for light properties in this sprint** - light editing is simple enough to skip undo for now. Can be added later.

### 2C: Ensure `ecs_world_get_light()` accessor exists

**File:** `src/ecs/ecs_world.h`

Verify `ecs_world_get_light()` exists (it was added in Sprint 5 of mesh triangles). If not, add:
```c
static inline LightComp* ecs_world_get_light(ecs_world_state_t *w, ecs_entity_t e) {
    return (LightComp*)ecs_get_id(w->world, e, w->LightComp_id);
}
```

**Verification:**
- Build and run
- Scene hierarchy shows "Lights" section with 3 studio lights (key, fill, rim)
- Click a light in hierarchy -> Entity Inspector shows Transform (position = direction) and Light (type, color, intensity)
- Change light color -> triangles in scene update in real-time
- Change intensity -> lighting brightness changes immediately
- Light direction editable via Transform.Position -> lighting direction changes

---

## Sprint 3: PLY Mesh Parser - ASCII

**Goal:** Extend `ply_loader.h` to parse triangle meshes from ASCII PLY files.

### Current State
- Parser reads `element vertex` with properties x/y/z/red/green/blue/alpha
- Skips `element face` and `property list` lines entirely
- Returns `ply_data_t` with points + colors only

### New Data Structures

**File:** `src/ply_loader.h`

```c
// Face data from PLY
typedef struct {
    uint32_t *indices;       // Triangle indices (3 per face, triangulated)
    vec4_t *face_colors;     // Per-face colors (NULL if no face colors)
    int face_count;          // Number of triangles
    int index_count;         // = face_count * 3
} ply_face_data_t;

// Extended loaded data (replaces ply_data_t for mesh mode)
typedef struct {
    vec3_t *vertices;        // Vertex positions
    vec4_t *vertex_colors;   // Per-vertex colors (NULL if none)
    int vertex_count;

    ply_face_data_t faces;   // Face data

    bool has_vertex_colors;  // Colors on vertex element
    bool has_face_colors;    // Colors on face element
    bool has_faces;          // Whether file has face data at all

    vec3_t min_bounds;
    vec3_t max_bounds;
} ply_mesh_data_t;
```

### Header Parsing Extensions

Extend `ply_parse_header()` to also detect:
- `element face <count>` -> store `face_count`
- `property list uchar int vertex_indices` (or `vertex_index`) -> mark face list property
- `property uchar red/green/blue` on the face element -> per-face colors
- Track which element we're in (vertex vs face) to correctly assign property indices

New header fields:
```c
typedef struct {
    // ... existing vertex fields ...

    // Face element
    int face_count;
    bool has_face_element;
    int face_prop_list_index;     // Index of vertex_indices list property

    // Face color properties (-1 if absent)
    int face_prop_red;
    int face_prop_green;
    int face_prop_blue;
    int face_prop_alpha;

    // Face element property tracking
    ply_property_t face_properties[PLY_MAX_PROPERTIES];
    int face_property_count;

    // Track data offsets for multi-element parsing
    long face_data_start_pos;     // For seeking to face data
} ply_header_t;
```

### ASCII Face Parsing

New function `ply_parse_ascii_faces()`:
1. After vertex parsing completes, parse face lines
2. Each face line format: `<vertex_count> <idx0> <idx1> ... [red green blue [alpha]]`
3. If vertex_count == 3: direct triangle
4. If vertex_count == 4: triangulate as two triangles (0,1,2) + (0,2,3)
5. If vertex_count > 4: fan triangulation from vertex 0
6. Collect per-face colors if present (after the indices on the same line)

### Main API

```c
// Load PLY file as mesh (vertices + faces)
static inline ply_error_t ply_load_mesh_file(const char *filepath, ply_mesh_data_t *data);

// Quick info scan that also reports face count
static inline ply_error_t ply_get_mesh_info(const char *filepath,
    int *vertex_count, int *face_count, bool *has_vertex_colors, bool *has_face_colors);

// Free mesh data
static inline void ply_mesh_data_free(ply_mesh_data_t *data);
```

### Incremental Parsing Extension

Extend `ply_parse_state_t` with face parsing fields:
```c
typedef struct {
    // ... existing fields ...

    // Face parsing state
    uint32_t *face_indices;
    vec4_t *face_colors;
    int face_parsed_count;
    int face_capacity;
    int face_total;
    bool parsing_faces;      // true when vertex parsing done, parsing faces
    bool has_face_colors;
} ply_parse_state_t;
```

Add `ply_parse_faces_chunk()` for incremental face parsing.

**Verification:**
- Create a simple test PLY file (ASCII) with vertices + faces
- Call `ply_load_mesh_file()` -> returns correct vertex + face data
- Test with per-vertex colors, per-face colors, and no colors
- Test quad triangulation (4-vertex faces -> 2 triangles)
- Test `ply_get_mesh_info()` returns correct counts

---

## Sprint 4: PLY Binary Support

**Goal:** Add binary_little_endian and binary_big_endian parsing for both vertices and faces.

### Binary Vertex Parsing

New function `ply_parse_binary_vertices()`:
1. Read vertex data as raw bytes based on property types and sizes
2. Handle endian swapping for big-endian files
3. Extract x/y/z positions and optional colors from correct byte offsets
4. Property size lookup: char/uchar=1, short/ushort=2, int/uint/float=4, double=8

### Binary Face Parsing

New function `ply_parse_binary_faces()`:
1. Read list property: first byte = vertex count (uchar), then N x int32 indices
2. Read any additional face properties (colors) after indices
3. Handle endian swapping

### Byte-order Helpers

```c
static inline void ply_swap_bytes_16(void *p);
static inline void ply_swap_bytes_32(void *p);
static inline void ply_swap_bytes_64(void *p);
static inline float ply_read_float(const uint8_t *buf, bool swap);
static inline int32_t ply_read_int32(const uint8_t *buf, bool swap);
// etc.
```

### Key Implementation Detail

For binary parsing, we need to know the byte stride of each element row:
- Compute `vertex_byte_stride` = sum of all vertex property sizes
- Compute per-face variable stride (list property means variable-length rows)
- For faces: read one face at a time since list properties have variable length

### Incremental Binary Parsing

Extend `ply_parse_vertices_chunk()` and `ply_parse_faces_chunk()` to handle binary mode:
- Read `chunk_size * vertex_byte_stride` bytes at once
- Parse from memory buffer rather than line-by-line

**Verification:**
- Export a test mesh from MeshLab/Blender as binary_little_endian PLY
- Parse and compare results to ASCII export of same mesh
- Test with the OBJ→PLY script (Sprint 6) outputting binary format
- Verify endian handling with both LE and BE if possible

---

## Sprint 5: PLY Mesh Import Job & UI

**Goal:** Full user-facing PLY mesh import with progress bar and two modes.

### New File: `src/ply_mesh_import_job.h`

Modeled closely after `ply_import_job.h` (514 lines). Key differences:

```c
typedef struct {
    ply_job_state_t state;       // Reuse same state enum
    ply_parse_state_t parse_state;
    char filepath[512];

    // Import options
    int import_mode;             // 0 = Single Mesh Entity, 1 = Individual Triangles
    float scale;
    vec4_t default_color;
    bool use_ply_colors;         // Use vertex/face colors from file

    // Transformations
    bool shift_to_com;
    float rotation_x, rotation_y, rotation_z;
    vec3_t com;
    mat4_t transform_matrix;
    bool transforms_applied;

    // Entity creation state
    int created_count;

    // Progress
    float progress;
    char status_message[128];

    // Timing (same pattern as point cloud import)
    double last_iteration_time_ms;
    float iteration_times[PLY_JOB_MAX_TIMING_SAMPLES];
    float iteration_progress[PLY_JOB_MAX_TIMING_SAMPLES];
    int timing_sample_count;
    uint64_t iteration_start_time;
    float last_sampled_progress;

    // Result
    ply_error_t error;
    int total_vertices;
    int total_faces;
} ply_mesh_import_job_t;
```

**State Machine:**
1. `PLY_JOB_IDLE` -> Start
2. `PLY_JOB_PARSING_VERTICES` -> Parse vertices in chunks
3. `PLY_JOB_PARSING_FACES` (new state) -> Parse faces in chunks
4. `PLY_JOB_CREATING_ENTITIES` -> Create mesh/triangle entities
5. `PLY_JOB_COMPLETE` / `PLY_JOB_ERROR`

**Entity Creation (Mode 0 - Single Mesh Entity):**
- One call to `scene_add_mesh_colored()` (if vertex colors) or `scene_add_mesh()` (uniform color)
- If face colors present but not vertex colors, convert face colors to vertex colors by averaging at shared vertices
- Instantaneous for any size (single entity creation)

**Entity Creation (Mode 1 - Individual Triangles):**
- Chunked creation: `PLY_ENTITY_CHUNK_SIZE` triangles per frame
- Each triangle: `scene_add_triangle()` or `scene_add_triangle_colored()`
- Progress tracking for creation phase

### UI Integration

**File:** `src/ui/ui_scene_hierarchy.h`

Add to the hierarchy state struct:
```c
// PLY Mesh import state
file_browser_t ply_mesh_browser;
bool ply_mesh_import_popup_open;
char ply_mesh_import_path[512];
int ply_mesh_vertex_count;
int ply_mesh_face_count;
bool ply_mesh_has_vertex_colors;
bool ply_mesh_has_face_colors;
int ply_mesh_import_mode;        // 0 = Single Mesh, 1 = Individual Triangles
int ply_mesh_unit_index;
bool ply_mesh_use_colors;
float ply_mesh_default_color[3];
bool ply_mesh_shift_to_com;
float ply_mesh_rotation[3];

// Import job
ply_mesh_import_job_t mesh_import_job;
bool mesh_import_progress_popup_open;
```

**Menu Item:** Under File menu, after "Import PLY Point Cloud...":
```
Import PLY Mesh...
```

**Import Dialog** (modal popup, same pattern as PLY point cloud):
- File info: vertex count, face count, has vertex colors, has face colors
- Import mode: "Single Mesh Entity" / "Individual Triangles"
- Color options: Use PLY colors / Override with default color
- Scale / Unit conversion
- Coordinate transforms: CoM shift, rotation X/Y/Z
- Import / Cancel buttons

**Progress Dialog** (modal popup, same pattern):
- Progress bar
- Status text ("Parsing vertices...", "Parsing faces...", "Creating entities...")
- Speed plot (reuse timing infrastructure)
- Cancel button

**Verification:**
- Build and run
- File -> Import PLY Mesh... -> opens file browser
- Select a PLY mesh file -> shows import options
- Import as Single Mesh Entity -> one entity in hierarchy, rendered with lighting
- Import as Individual Triangles -> N entities in hierarchy, each independently selectable
- Test with colored PLY -> colors display correctly
- Test progress bar with large mesh (>10k faces)
- Test with both ASCII and binary PLY files

---

## Sprint 6: OBJ to PLY Conversion Script

**Goal:** Python utility to convert OBJ meshes to colored PLY files.

### New File: `scripts/obj_to_colored_ply.py`

```
Usage: python obj_to_colored_ply.py input.obj output.ply [options]

Options:
  --color-mode vertex|face    Per-vertex or per-face random colors (default: vertex)
  --format ascii|binary       Output PLY format (default: ascii)
  --seed N                    Random seed for reproducible colors (default: random)
  --palette NAME              Color palette: "rainbow", "pastel", "earth", "grayscale" (default: rainbow)
  --alpha FLOAT               Alpha value 0-1 (default: 1.0)
```

**Implementation:**
1. Parse OBJ: read `v` lines (vertices) and `f` lines (faces)
   - Handle OBJ face format: `f v1 v2 v3`, `f v1/vt1 v2/vt2 v3/vt3`, `f v1/vt1/vn1 ...`, `f v1//vn1 ...`
   - Triangulate quads (split into 2 triangles)
2. Generate random colors based on mode:
   - **Per-vertex**: One random RGB per vertex
   - **Per-face**: One random RGB per face
3. Write PLY:
   - Header with element vertex (x y z [red green blue alpha]) + element face
   - For per-vertex: colors on vertex element
   - For per-face: colors on face element
   - Support both ASCII and binary_little_endian output

**Color Palettes:**
- `rainbow`: Full HSV range, random hue, high saturation
- `pastel`: Random hue, low saturation, high value
- `earth`: Browns, greens, tans
- `grayscale`: Random gray values

**Verification:**
- Run: `python scripts/obj_to_colored_ply.py test.obj test_vertex.ply --color-mode vertex`
- Run: `python scripts/obj_to_colored_ply.py test.obj test_face.ply --color-mode face`
- Run: `python scripts/obj_to_colored_ply.py test.obj test_binary.ply --format binary`
- Open outputs in MeshLab to verify correctness
- Import into mdCAD using Sprint 5's PLY Mesh Import -> verify colors display

---

## Sprint Summary

| Sprint | Deliverable | Key Files | Testable Outcome |
|--------|-------------|-----------|------------------|
| S1 ✅ | Add Entity menu items | `ui_scene_hierarchy.h` | Triangle & Box from menu |
| S2 ✅ | Light inspector | `ui_scene_hierarchy.h`, `ui_entity_inspector.h` | Select & edit lights |
| S3 | PLY mesh parser (ASCII) | `ply_loader.h` | Parse PLY mesh files |
| S4 | PLY binary support | `ply_loader.h` | Parse binary PLY files |
| S5 | PLY mesh import UI | `ply_mesh_import_job.h` (new), `ui_scene_hierarchy.h` | Full import workflow |
| S6 | OBJ→PLY script | `scripts/obj_to_colored_ply.py` (new) | Convert & import test assets |

**Dependencies:** S5 depends on S3+S4. S6 is independent. S1 and S2 are independent of everything else.

**Recommended order:** S1 ✅ -> S2 ✅ -> S3 -> S4 -> S5 -> S6
