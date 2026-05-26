# Manipulator Gizmo - Implementation Plan

## Context

We need a 3D manipulator gizmo with **two modes** that renders **completely separate from the ECS scene system**. Previous attempts to integrate gizmo entities into the existing ECS led to excessive complexity and edge cases. The new approach: the gizmo is a self-contained plain-C system with its own GPU buffers, pipelines, and pick IDs. It renders always-on-top using a depth-disabled pipeline, uses reserved pick IDs from the top of the 24-bit range, and is invisible to the scene hierarchy.

**Two modes (toggled with Tab):**
1. **Transform mode** — manipulate entity `TransformComp` (move whole entity)
2. **Geometry mode** — manipulate individual vertices of `GeometryComp` (line endpoints, polyline/polygon vertices)

Sprint 1 scope: **Translation only** for both modes. Rotation/scale deferred.

---

## Architecture Summary

```
gizmo.h (state machine, two modes, interaction logic)
  ├── gizmo_rendering.h (GPU resources: own pipelines + instance buffers)
  ├── gizmo_vertex_mode.h (geometry mode: vertex handles, selection, editing)
  ├── pick_buffer.h (reuse existing pick_buffer_add_line/point API)
  └── math3d.h (new: ray_t, ray-plane/axis intersection)

NOT touching: ecs_world.h components, ecs_scene.h rendering, geometry_batch.h
Only change to ecs_world.h: cap pick ID allocator below reserved range
```

---

## Pick ID Scheme (reserved at top of 24-bit range)

```c
#define GIZMO_PICK_RESERVED_START  16767200u   // Everything >= this is gizmo territory
#define GIZMO_HANDLE_BASE          16767200u   // 16 IDs: transform gizmo handles (16767200-16767215)
#define VERTEX_HANDLE_BASE         16767216u   // 10000 IDs: vertex handles (16767216-16777215)
#define VERTEX_HANDLE_MAX          10000       // Max vertices in geometry mode
// Entity allocator capped at 16767199
```

**Gizmo handle IDs** (GIZMO_HANDLE_BASE + offset):
- +0 = X axis, +1 = Y axis, +2 = Z axis
- +3 = XY plane, +4 = XZ plane, +5 = YZ plane
- +6..+15 reserved for future rotate/scale handles

**Vertex handle IDs** (VERTEX_HANDLE_BASE + vertex_index):
- Up to 10,000 vertices for geometry mode editing

---

## New Files

### 1. `src/gizmo/gizmo_rendering.h` — GPU resources

Own line + point instance buffers (separate from scene), own pipelines with `SG_COMPAREFUNC_ALWAYS` + `write_enabled=false` (always on top). Reuses existing shaders (`instanced_line_shaders.h`, `join_shaders.h`).

Capacity: 32 lines, 10016 points (16 for gizmo tips + up to 10000 for vertex handles).

Key functions:
- `gizmo_rendering_init(gr)` — create pipelines, buffers, template geometry
- `gizmo_rendering_upload(gr)` — stream-upload instance data
- `gizmo_rendering_draw(gr, mvp, aspect, line_width, point_size)` — draw lines then points
- `gizmo_rendering_shutdown(gr)` — destroy GPU resources

Pipeline creation mirrors `geometry_batch.h:312-336` exactly, with one change:
```c
.depth = { .compare = SG_COMPAREFUNC_ALWAYS, .write_enabled = false, .pixel_format = SG_PIXELFORMAT_DEPTH }
```

### 2. `src/gizmo/gizmo_vertex_mode.h` — Geometry mode vertex editing

Manages vertex handles for the selected entity when in geometry mode. Reads vertex positions from `GeometryComp`, renders them as overlay points (2x thickness of global ECS point size), assigns reserved pick IDs.

**Supported geometry types:**
- `GEOM_POINT`: 1 vertex (the point itself)
- `GEOM_LINE`: 2 vertices (endpoints a, b)
- `GEOM_POLYLINE`: N vertices (polyline.points[])
- `GEOM_POLYGON`: N vertices (polygon.points[])
- Arc, bezier, helix, point cloud: **out of scope** (ignored in geometry mode)

**State:**
```c
typedef struct {
    ecs_entity_t target_entity;      // Entity being edited (0 = none)
    int vertex_count;                // Total vertices
    int *selected_vertices;          // Array of selected vertex indices
    int selected_count;
    int selected_capacity;
    int point_slot_start;            // First slot in gizmo point instance buffer
    bool active;
} gizmo_vertex_mode_t;
```

Key functions:
- `gizmo_vertex_mode_enter(vm, entity, scene)` — read GeometryComp, allocate point slots
- `gizmo_vertex_mode_exit(vm)` — free slots, clear state
- `gizmo_vertex_mode_update(vm, scene, rendering, point_size)` — update vertex overlay positions + colors (highlight selected/hovered)
- `gizmo_vertex_mode_populate_pick(vm, scene, pb)` — add vertex points to pick buffer with VERTEX_HANDLE_BASE + index
- `gizmo_vertex_mode_select(vm, vertex_idx, shift, ctrl)` — vertex selection (click/shift/ctrl)
- `gizmo_vertex_mode_get_center(vm, scene)` — average position of selected vertices (for gizmo placement)
- `gizmo_vertex_mode_apply_delta(vm, scene, delta)` — move selected vertices by delta, mark entity dirty

### 3. `src/gizmo/gizmo.h` — Main state machine + interaction

Plain C struct (no ECS). Pre-allocates transform handle slots at init. Manages mode switching.

**Gizmo geometry (lines + points only):**
- 3 axis lines (center → tip) — RED, GREEN, BLUE
- 3 tip dots (thicker points at axis tips) — same colors
- 3 plane handle rectangles (4 lines each at XY/XZ/YZ offsets) — blended colors
- Total: 15 line slots + 3 point slots for transform handles

**State machine:** `HIDDEN → IDLE → HOVER → DRAGGING`

**Two modes:**
```c
typedef enum {
    GIZMO_TRANSFORM_MODE,   // Move whole entities (TransformComp)
    GIZMO_GEOMETRY_MODE     // Move individual vertices (GeometryComp)
} gizmo_edit_mode_t;
```

**Constant screen size:** Each frame compute `scale = dist * tan(fov/2) * 0.15` where `dist` = camera-to-gizmo distance.

**State:**
```c
typedef struct {
    gizmo_rendering_t rendering;
    gizmo_vertex_mode_t vertex_mode;
    gizmo_edit_mode_t edit_mode;       // TRANSFORM or GEOMETRY
    gizmo_mode_t mode;                 // HIDDEN/IDLE/HOVER/DRAGGING
    gizmo_handle_t hovered_handle;     // Which handle is hovered (-1 = none)
    gizmo_handle_t active_handle;      // Which handle is being dragged
    int hovered_vertex;                // Which vertex is hovered (-1 = none, geometry mode only)
    vec3_t center;                     // Gizmo position
    float scale;                       // Current world-space scale
    // Drag state...
    // Slot indices for transform handles...
    // Colors...
} gizmo_t;
```

Key functions:
- `gizmo_init(g)` — init rendering + vertex mode, alloc transform handle slots
- `gizmo_set_edit_mode(g, mode, scene, selection)` — switch between Transform/Geometry
- `gizmo_update(g, selection, scene, cam_pos, fov, ecs_point_size)` — recompute all geometry
- `gizmo_populate_pick_buffer(g, pb, scene)` — add handles + vertex points to pick buffer
- `gizmo_handle_hover(g, pick_id)` — route to handle hover or vertex hover
- `gizmo_begin_drag(g, mouse_ray)` — start constrained drag
- `gizmo_update_drag(g, mouse_ray, *delta)` — compute incremental delta
- `gizmo_end_drag(g, *total_delta)` — finish drag
- `gizmo_shutdown(g)`

---

## Modified Files

### 4. `src/math3d.h` — Ray intersection math

Add at end (before `#endif`):
```c
typedef struct { vec3_t origin, direction; } ray_t;
mat4_t mat4_inverse(mat4_t m);                    // General 4x4 inverse (cofactor method)
ray_t ray_from_screen(float ndc_x, float ndc_y, mat4_t inv_vp);  // Unproject screen → world ray
float ray_axis_closest_t(ray_t ray, vec3_t axis_origin, vec3_t axis_dir);  // Closest-point-on-axis
float ray_plane_intersect(ray_t ray, vec3_t plane_pt, vec3_t plane_n, vec3_t *hit);  // Ray-plane hit
```

### 5. `src/components/selectable_comp.h` — Define reserved ID ranges

Add the `GIZMO_PICK_RESERVED_START`, `GIZMO_HANDLE_BASE`, `VERTEX_HANDLE_BASE` defines.

### 6. `src/ecs/ecs_world.h` — Cap pick ID allocator

In `ecs_world_alloc_pick_id()` (line ~44), add guard:
```c
if (s->next_pick_id >= GIZMO_PICK_RESERVED_START) return 0;
```

### 7. `src/ui/ui_viewport.h` — Camera suppression flag

Add `bool suppress_camera_input;` to `ui_viewport_state_t`. Gate camera call:
```c
if (vp->camera && !vp->suppress_camera_input) {
    orbit_camera_handle_input(vp->camera, hovered, dx, dy, wheel, left, middle, shift);
}
```

### 8. `src/app.c` — Main integration (largest change)

**State additions:**
```c
gizmo_t gizmo;
bool gizmo_drag_active;
// Transform mode drag snapshots:
vec3_t *gizmo_drag_start_positions;
ecs_entity_t *gizmo_drag_entities;
int gizmo_drag_entity_count;
// Geometry mode drag snapshots:
vec3_t *gizmo_drag_start_vertices;
int *gizmo_drag_vertex_indices;
int gizmo_drag_vertex_count;
```

**`init()`:** Call `gizmo_init(&state.gizmo)`

**`frame()` changes (in order):**

1. **Before `ui_viewport_draw`** — set camera suppression:
   ```c
   state.viewport.suppress_camera_input =
       state.gizmo.hovered_handle != GIZMO_HANDLE_NONE ||
       state.gizmo.hovered_vertex >= 0 ||
       state.gizmo.mode == GIZMO_MODE_DRAGGING;
   ```

2. **Handle Tab key** — toggle edit mode:
   ```c
   if (igIsKeyPressed_Bool(ImGuiKey_Tab, false) && !io->WantCaptureKeyboard) {
       gizmo_edit_mode_t new_mode = (state.gizmo.edit_mode == GIZMO_TRANSFORM_MODE)
           ? GIZMO_GEOMETRY_MODE : GIZMO_TRANSFORM_MODE;
       gizmo_set_edit_mode(&state.gizmo, new_mode, &state.ecs_scene, &state.selection);
   }
   ```

3. **After `ecs_scene_update`, before offscreen pass** — update gizmo:
   ```c
   float ecs_point_size = /* global app point size */;
   gizmo_update(&state.gizmo, &state.selection, &state.ecs_scene, cam_eye, fov, ecs_point_size);
   ```

4. **Inside offscreen pass, after `ecs_scene_draw`** — draw gizmo on top:
   ```c
   gizmo_rendering_upload(&state.gizmo.rendering);
   gizmo_rendering_draw(&state.gizmo.rendering, mvp, aspect_ratio, line_width, point_size);
   ```

5. **After `pick_buffer_begin_frame` + `ecs_scene_populate_pick_buffer`** — add gizmo + vertices to pick buffer:
   ```c
   gizmo_populate_pick_buffer(&state.gizmo, &state.pick_buffer, &state.ecs_scene);
   ```

6. **After `pick_buffer_readback`** — route hover/click based on pick ID:
   - If pick ID in gizmo handle range → gizmo handle hover/drag
   - If pick ID in vertex handle range → vertex hover/select (geometry mode)
   - Otherwise → normal ECS hover + entity selection

7. **Drag logic** — same translation gizmo drag mechanics for both modes:
   - **Transform mode**: delta applied to `TransformComp.position` of selected entities
   - **Geometry mode**: delta applied to selected vertex positions in `GeometryComp.data`, entity marked dirty via `RenderableComp.instance_dirty`

8. **Undo integration**:
   - **Transform mode**: `undo_cmd_set_position()` per entity
   - **Geometry mode**: new `CMD_SET_GEOMETRY_VERTICES` undo command (stores entity, vertex indices, old/new positions)

**`cleanup()`:** Call `gizmo_shutdown(&state.gizmo)`, free drag arrays.

---

## Geometry Mode Detailed Flow

1. User selects entity, presses **Tab** → enters Geometry mode
2. `gizmo_vertex_mode_enter()` reads `GeometryComp`:
   - GEOM_LINE → 2 vertices (a, b)
   - GEOM_POLYLINE → N vertices
   - GEOM_POLYGON → N vertices
   - Others → geometry mode not available, stay in transform mode
3. Vertex overlay dots rendered in gizmo point buffer (2x ECS point size, always on top)
4. Vertices get pick IDs: `VERTEX_HANDLE_BASE + vertex_index`
5. Click vertex → select it. Shift-click = add. Ctrl-click = toggle.
6. Translation gizmo appears at center of selected vertices
7. Drag gizmo handle → selected vertices move along constrained axis/plane
8. On drag end → update `GeometryComp` data, record undo
9. Press **Tab** again → exit geometry mode, return to transform mode

**Vertex position reads/writes are in LOCAL space** — vertices are stored in GeometryComp in local coordinates. The overlay rendering transforms them to world space via the entity's `TransformComp.world_matrix` for display. Drag deltas are inverse-transformed back to local space before applying to vertex data.

---

## Implementation Phases

### Phase 1: Render-only transform gizmo (see gizmo, no interaction)
1. Add reserved ID defines to `selectable_comp.h`, cap allocator in `ecs_world.h`
2. Create `src/gizmo/gizmo_rendering.h` (pipelines, buffers, draw)
3. Create `src/gizmo/gizmo.h` with init/update/shutdown only (transform handles)
4. Integrate into `app.c`: init, compute selection center, update gizmo, draw in offscreen pass
5. **Verify:** Select an entity → gizmo appears at its center, always on top, constant screen size

### Phase 2: Pick + hover highlighting
1. Add `gizmo_populate_pick_buffer()` for transform handles
2. Add `gizmo_handle_hover()` with color change
3. Route pick IDs in `app.c` (gizmo handles first, then entities)
4. **Verify:** Hover over handles → they highlight, entity hover still works

### Phase 3: Ray math
1. Add `ray_t`, `mat4_inverse`, `ray_from_screen`, `ray_axis_closest_t`, `ray_plane_intersect` to `math3d.h`
2. **Verify:** Test ray intersections with known values

### Phase 4: Transform mode drag + undo
1. Add camera suppression to `ui_viewport.h`
2. Add `gizmo_begin_drag`, `gizmo_update_drag`, `gizmo_end_drag`
3. Integrate drag loop in `app.c` (snapshot positions, apply deltas, record undo)
4. **Verify:** Drag handle → entities move along constrained axis/plane, Ctrl+Z restores

### Phase 5: Geometry mode — vertex overlay + selection
1. Create `src/gizmo/gizmo_vertex_mode.h`
2. Add Tab key toggle in `app.c`
3. Implement vertex overlay rendering (2x point size, always on top)
4. Implement vertex pick buffer population + hover highlight
5. Implement vertex selection (click, shift, ctrl)
6. **Verify:** Tab into geometry mode → vertex dots appear on line/polyline, hover highlights, click selects

### Phase 6: Geometry mode — vertex dragging + undo
1. Place translation gizmo at selected vertices center
2. Drag applies delta to selected vertex positions (inverse-transform to local space)
3. Mark entity `RenderableComp.instance_dirty` → scene re-renders
4. Add `CMD_SET_GEOMETRY_VERTICES` undo command type to `undo_redo.h` + `undo_redo_exec.h`
5. **Verify:** Drag vertex → geometry deforms, Ctrl+Z restores, Tab back to transform mode

---

## Key Technical Details

**Camera suppression timing:** Gizmo hover resolved at END of frame. `ui_viewport_draw` (camera input) runs at START. We use previous-frame hover state to suppress camera — 1-frame latency matches existing hover behavior.

**Degenerate cases:** When camera looks along an axis, ray-axis intersection degenerates. Guard with denominator check — return no-delta when denominator < epsilon.

**Multi-selection (transform mode):** Gizmo center = average of selected positions. All entities translate by same delta. Undo records one `CMD_SET_POSITION` per entity.

**Vertex local/world space:** Vertices stored in local space in `GeometryComp`. For overlay rendering and pick buffer: transform to world via `mat4_mul_point(world_matrix, local_vertex)`. For applying drag delta: inverse-transform delta from world to local space.

**Vertex overlay thickness:** Point size = 2x the global ECS point size parameter, making vertices visually prominent above the geometry.

**Existing code reuse:**
- `pick_buffer_add_line/point()` from `src/gpu/pick_buffer.h:506,526` — add to pick buffer
- `instance_buffer_*` from `src/gpu/instance_buffer.h` — slot management
- `geom_line_instance_t` / `geom_point_instance_t` from `src/gpu/geometry_batch.h` — instance data format
- `undo_cmd_set_position()` from `src/undo_redo_exec.h:233` — transform mode undo
- Shaders: `instanced_line_vs/fs_source`, `join_vs/fs_source` — reused verbatim
- Template geometry: `geom_batch_generate_line_template()`, `geom_batch_generate_point_template()`
- `mat4_mul_point()` from `src/math3d.h` — vertex world-space transform

---

## New Undo Command (Phase 6)

```c
// In undo_redo.h:
CMD_SET_GEOMETRY_VERTICES,

typedef struct {
    uint64_t entity_id;
    int *vertex_indices;      // Which vertices changed
    vec3_t *old_positions;    // Original positions (local space)
    vec3_t *new_positions;    // New positions (local space)
    int count;
} cmd_set_geometry_vertices_t;
```

Execute: write new_positions to GeometryComp vertices, mark dirty.
Undo: write old_positions back, mark dirty.

---

## Verification

After each phase, build and run:
```bash
cmake -B build -G Ninja && ninja -C build && ./build/bin/mdCAD
```

1. **Phase 1:** Create geometry, select it → 3 colored axis lines + tip dots + plane rectangles appear
2. **Phase 2:** Hover handles → highlight. Entity hover unaffected.
3. **Phase 3:** Math only — tested via assertions
4. **Phase 4:** Drag X handle → entity moves along X. Ctrl+Z restores. Camera blocked during drag.
5. **Phase 5:** Select line, press Tab → vertex dots appear at endpoints (2x size). Click vertex → selects. Tab again → back to transform mode.
6. **Phase 6:** Select vertex, drag gizmo → line endpoint moves. Ctrl+Z restores vertex position.