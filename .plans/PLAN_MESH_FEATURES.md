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

## Sprint 3: PLY Mesh Parser - ASCII ✅ DONE

**Goal:** Extend `ply_loader.h` to parse triangle meshes from ASCII PLY files.

**File modified:** `src/ply_loader.h`

**What was implemented:**
1. Extended `ply_property_t` with `is_list` and `list_count_type` for face list properties
2. Added `ply_face_data_t` (triangulated indices + optional per-face colors) and `ply_mesh_data_t` (vertices + faces combined output)
3. Extended `ply_header_t` with face element fields: `face_count`, `has_face_element`, `face_properties[]`, `face_list_prop_index`, face color indices, `skip_lines_before_faces`
4. Updated `ply_parse_header()` with `current_element` state machine (0=none, 1=vertex, 2=face, 3=other) to track vertex_indices list properties, face scalar color properties, and intermediate element line counts
5. Added `ply_parse_ascii_faces()` with fan triangulation (tri=direct, quad=2 tris, polygon=N-2 tris), dynamic index buffer growth, and integer/float color normalization
6. Added `ply_load_mesh_file()` one-shot API (header + vertices + faces)
7. Added `ply_mesh_data_free()` and `ply_get_mesh_info()` (header-only scan)
8. Extended `ply_parse_state_t` with face parsing fields (face_indices, face_colors, face_tri_count, face_tri_capacity, face_total, parsing_faces, etc.)
9. Updated `ply_open()` to initialize face arrays when face element present
10. Added `ply_parse_faces_chunk()` (incremental face parsing) and `ply_vertices_complete()` (phase gate)
11. Updated `ply_get_progress()` (vertex+face combined) and `ply_is_complete()` (both phases)
12. Updated `ply_parse_state_free()` (frees face arrays) and added `ply_parse_state_to_mesh_data()` (transfers all arrays)

All changes backward-compatible: existing point cloud import (`ply_load_file`, `ply_open` + `ply_parse_vertices_chunk`, `ply_get_info`) works unchanged since face_total=0 for files without faces.

---

## Sprint 4: PLY Binary Support ✅ DONE

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

## Sprint 5: PLY Mesh Import Job & UI ✅ DONE

**Goal:** Full user-facing PLY mesh import with progress bar and two modes.

**New File:** `src/ply_mesh_import_job.h`

Header-only mesh import job state machine modeled after `ply_import_job.h`. 3-phase state machine: `PLY_MESH_JOB_PARSING_VERTICES` -> `PLY_MESH_JOB_PARSING_FACES` -> `PLY_MESH_JOB_CREATING_ENTITIES`. Two import modes: Single Mesh Entity (calls `scene_add_mesh_colored()`/`scene_add_mesh()` instantaneously) and Individual Triangles (chunked `scene_add_triangle()`/`scene_add_triangle_colored()` with progress). Face-color-to-vertex-color conversion via averaging at shared vertices. Transforms: CoM shift, X/Y/Z rotation, unit scale. Progress weighting: 30% vertices, 30% faces, 40% entity creation.

**Modified File:** `src/ui/ui_scene_hierarchy.h`

- Added `#include "../ply_mesh_import_job.h"` and 17 mesh import state fields to struct
- Added init/shutdown code for mesh import state (file browser, job lifecycle)
- Added "Import PLY Mesh..." menu item under File, after "Import PLY Point Cloud..."
- Added file browser handler with `ply_get_mesh_info()` validation (rejects files with no faces)
- Added "Import PLY Mesh Options" modal: file info (vertices/faces/colors), import mode radio buttons with tooltips, unit conversion, color options with smart source display, transformation controls
- Added "Importing PLY Mesh" progress modal: progress bar, status text, timing info, speed plot, Cancel/Close button, chunked per-frame processing

**Verified:**
- File -> Import PLY Mesh... -> file browser opens, selects .ply files
- Import as Single Mesh Entity -> one GEOM_MESH entity, rendered with lighting
- Import as Individual Triangles -> N triangle entities, each independently selectable
- Colored PLY files display vertex/face colors correctly
- Progress bar works for large meshes with timing plot
- Both ASCII and binary PLY files supported

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
| S3 ✅ | PLY mesh parser (ASCII) | `ply_loader.h` | Parse PLY mesh files |
| S4 ✅ | PLY binary support | `ply_loader.h` | Parse binary PLY files |
| S5 ✅ | PLY mesh import UI | `ply_mesh_import_job.h` (new), `ui_scene_hierarchy.h` | Full import workflow |
| S6 | OBJ→PLY script | `scripts/obj_to_colored_ply.py` (new) | Convert & import test assets |

**Dependencies:** S5 depends on S3+S4. S6 is independent. S1 and S2 are independent of everything else.

**Recommended order:** S1 ✅ -> S2 ✅ -> S3 ✅ -> S4 ✅ -> S5 ✅ -> S6
