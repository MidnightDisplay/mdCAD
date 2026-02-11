# Mesh Triangles & Lighting System - Implementation Plan

## Context

We want to add **instanced mesh triangles** as a new geometry primitive alongside existing points and lines. Triangles will be full ECS entities with Transform, Geometry, Selectable, and Renderable components. All existing user interactions (selection, hover, undo/redo, gizmo manipulation, serialization) must work. We also want a basic lighting system with 3-point studio lighting at app launch.

### Current Architecture (for reference)

The existing rendering uses **template geometry + GPU instancing**:
- **Lines**: Rectangle+cap template (30 verts) instanced per line segment, vertex shader morphs template to screen-space thick line between two world-space endpoints.
- **Points**: Circle template (17 verts) instanced per point, vertex shader positions circle at world-space center.
- Each batch type has: `instance_buffer_t` (slot allocation), `sg_pipeline`, `sg_shader`, template `sg_buffer`s.
- Multi-segment entities (polyline, polygon, arc, bezier, helix) use **contiguous slot allocation** in the line batch.
- Pick buffer uses separate instance buffers with the same template geometry but pick-color shaders.

### Triangle Instancing Approach

For triangles, the "template" is just 3 vertices with **barycentric selector coordinates**:
```
Vertex 0: selector = (1, 0, 0)  → maps to instance vertex A
Vertex 1: selector = (0, 1, 0)  → maps to instance vertex B
Vertex 2: selector = (0, 0, 1)  → maps to instance vertex C
```

The vertex shader computes: `world_pos = selector.x * A + selector.y * B + selector.z * C`

Per-vertex colors use the same mixing: `color = selector.x * colorA + selector.y * colorB + selector.z * colorC`

This follows the existing instancing pattern perfectly.

### Instance Data Layout (96 bytes per triangle)

```c
typedef struct {
    float ax, ay, az;      // Vertex A world position
    float bx, by, bz;      // Vertex B world position
    float cx, cy, cz;      // Vertex C world position
    float nx, ny, nz;      // Face normal
    float ra, ga, ba, aa;  // Color A (or uniform color replicated to all 3)
    float rb, gb, bb, ab;  // Color B
    float rc, gc, bc, ac;  // Color C
} geom_triangle_instance_t;  // 24 floats = 96 bytes
```

8 vertex attributes total (1 per-vertex template_pos + 7 per-instance). SG_MAX_VERTEX_ATTRIBUTES = 16, so plenty of headroom.

For uniform-color mode, all three colors are set identically. The shader always interpolates, which for identical values gives the uniform color. No branching needed.

---

## Sprint 1: Core Triangle Rendering Infrastructure (COMPLETED)

**Goal**: Flat-shaded triangles visible on screen alongside existing geometry.

### 1.1 Geometry Component — `src/components/geometry_comp.h`

- [x] Add `GEOM_TRIANGLE = 8` to `geometry_type_t` enum (after GEOM_POINT_CLOUD = 7)
- [x] Add `geom_triangle_data_t` struct (vec3_t a, b, c)
- [x] Add `geom_triangle_data_t triangle` to the GeometryComp union
- [x] Add `geometry_comp_triangle(a, b, c, color)` factory helper

### 1.2 Triangle Batch — `src/gpu/geometry_batch.h`

- [x] Define `geom_triangle_instance_t` (96 bytes, 24 floats)
- [x] Define `geom_triangle_batch_t` with instance_buffer, template buffers, pipeline, shader
- [x] Add `geom_triangle_batch_t triangles` to `geometry_batch_manager_t`
- [x] `geom_batch_generate_triangle_template()` — 3 vertices with barycentric selectors
- [x] `geom_triangle_batch_init()` — create template geometry, shader, pipeline (depth LESS_EQUAL, cull NONE)
- [x] `geom_triangle_batch_alloc()` — allocate 1 slot
- [x] `geom_triangle_batch_alloc_contiguous(n)` — allocate N contiguous slots
- [x] `geom_triangle_batch_set(slot, a, b, c, normal, color)` — uniform color
- [x] `geom_triangle_batch_set_colored(slot, a, b, c, normal, ca, cb, cc)` — per-vertex color
- [x] `geom_triangle_batch_set_entity(slot, entity_id)` — debug mapping
- [x] `geom_triangle_batch_free(slot)` — return slot to free list
- [x] `geom_triangle_batch_upload()` — sync CPU->GPU
- [x] `geom_triangle_batch_draw(mvp)` — bind pipeline, bind buffers, draw instances
- [x] Wire into `geometry_batch_manager_init/upload/draw/shutdown`
- [x] Helper: `geom_triangle_compute_normal(a, b, c)` — returns `normalize(cross(b-a, c-a))`

### 1.3 Triangle Shaders — `src/shaders/instanced_triangle_shaders.h`

- [x] Multi-backend vertex shader (GLCORE, GLES3, Metal, WGPU, D3D11, Vulkan)
- [x] Multi-backend fragment shader (flat color output, no lighting)
- [x] Expose as `instanced_triangle_vs_source` / `instanced_triangle_fs_source` per backend

### 1.4 SPIR-V Vulkan Shaders — `src/shaders/spirv/`

- [x] `instanced_triangle.vert` — GLSL 450 version
- [x] `instanced_triangle.frag` — GLSL 450 version
- [x] Compile to `.spv` bytecode (2264 + 448 bytes)
- [x] Generate C byte arrays in `spirv_bytecode.h`
- [x] Include bytecode in `instanced_triangle_shaders.h` for Vulkan backend

### 1.5 Scene API — `src/ecs/ecs_scene.h`

- [x] `scene_add_triangle(scene, a, b, c, color)` — full entity creation with slot allocation
- [x] Add `GEOM_TRIANGLE` case in `ecs_scene_update()` — visibility hiding + dirty transform update
- [x] Add `GEOM_TRIANGLE` case in `scene_free_entity_slots()` — free triangle batch slot
- [x] `ecs_scene_triangle_count()` utility function

### 1.6 App Integration — `src/app.c`

- [x] Triangle batch init/upload/draw wired through manager
- [x] 3 sample triangles at startup (red, green in XZ plane, blue tilted up)
- [x] Draw order: triangles first, then lines, then points (correct layering)

### 1.7 Testing

- [x] Build and run on Windows (Vulkan) — verified working
- [x] RGB triangles visible in viewport
- [x] Triangles depth-test correctly against lines/points
- [x] Camera orbit works correctly

---

## Sprint 2: Picking, Selection, Undo/Redo & Serialization (COMPLETED)

**Goal**: Fully interactive triangles with click-to-select, undo/redo, and save/load.

### 2.1 Triangle Pick Shaders — `src/shaders/pick_shaders.h`

- [x] Add `pick_triangle_vs_source` / `pick_triangle_fs_source` (all backends):
  - Same vertex shader as visual triangle but outputs pick_color (vec3) instead of visual color
  - Fragment shader outputs pick_color as RGB
  - Vertex attributes: template_pos + vertex_a/b/c + pick_color (no normal/colors needed)
- [x] SPIR-V: `pick_triangle.vert` / `pick_triangle.frag`, compile and include bytecode

### 2.2 Pick Buffer Integration — `src/gpu/pick_buffer.h`

- [x] Define `pick_triangle_instance_t`:
  ```c
  typedef struct {
      float ax, ay, az;   // Vertex A
      float bx, by, bz;   // Vertex B
      float cx, cy, cz;   // Vertex C
      float r, g, b;      // Pick color (encoded ID)
  } pick_triangle_instance_t;  // 12 floats
  ```
- [x] Add triangle template geometry buffers (same 3-vertex barycentric template)
- [x] Add `sg_pipeline triangle_pip` + `sg_shader triangle_shd` for pick
- [x] Add `instance_buffer_t triangle_instances`
- [x] Add overlay versions (for future gizmo-level triangle picking if needed) — deferred, not needed yet
- [x] `pick_buffer_add_triangle(pb, a, b, c, pick_id)` — allocate slot, encode pick_id as RGB
- [x] Update `pick_buffer_begin_frame()` — clear triangle instances
- [x] Update `pick_buffer_render()` — upload + draw triangle pick instances
- [x] Update `pick_buffer_init/shutdown` — create/destroy triangle resources

### 2.3 Scene Pick Integration — `src/ecs/ecs_scene.h`

- [x] Add `GEOM_TRIANGLE` case in `ecs_scene_populate_pick_buffer()`:
  - Transform 3 vertices through world_matrix
  - Call `pick_buffer_add_triangle()` with entity's pick_id

### 2.4 Undo/Redo — `src/undo_redo.h` + `src/undo_redo_exec.h`

Most commands already work generically (CMD_SET_POSITION, CMD_SET_COLOR, CMD_SET_VISIBLE, CMD_DELETE_ENTITY). We need:

- [x] Add `UNDO_GEOM_TRIANGLE` to `undo_geom_type_t` enum
- [x] Add triangle snapshot data to `undo_entity_snapshot_t` union:
  ```c
  struct { vec3_t a, b, c; } triangle;
  ```
- [x] Update `undo_snapshot_entity()` — capture triangle vertices
- [x] Update `undo_recreate_entity()` — recreate triangle from snapshot
- [x] `undo_set_vertex_pos()` updated to handle GEOM_TRIANGLE vertices
- [x] CMD_CREATE_ENTITY / CMD_DELETE_ENTITY work for triangles (automatic with snapshot)

### 2.5 Scene Serialization — `src/scene_serializer.h`

- [x] Add `"triangle"` case in `scene_serialize_geometry()`:
  ```json
  { "type": "triangle", "a": [x,y,z], "b": [x,y,z], "c": [x,y,z] }
  ```
- [x] Add `"triangle"` case in `scene_deserialize_geometry()`:
  - Parse a, b, c arrays → call `scene_add_triangle()`
- [x] Updated `scene_geom_type_to_string()` and `scene_string_to_geom_type()` for triangle
- [ ] Test round-trip: create triangles, save, clear, load — verify identical

### 2.6 Testing

- Click triangle to select → hover highlight, selection highlight
- Shift+Click multiple triangles → multi-select
- Delete selected triangle → triangle disappears
- Ctrl+Z → triangle reappears
- Move triangle via gizmo (transform mode) → Ctrl+Z restores position
- Save scene with triangles → load scene → triangles restored
- Verify pick works with overlapping triangles (depth-correct picking)

---

## Sprint 3: Gizmo Vertex Editing (COMPLETED)

**Goal**: Edit individual triangle vertices with the geometry mode gizmo.

### 3.1 Vertex Mode Support — `src/gizmo/gizmo_vertex_mode.h`

- [x] Add `GEOM_TRIANGLE` to `gizmo_vertex_mode_get_vertex_count()` → return 3
- [x] Add `GEOM_TRIANGLE` to `gizmo_vertex_mode_get_local_pos()`:
  ```c
  case GEOM_TRIANGLE:
      if (idx == 0) return geom->data.triangle.a;
      if (idx == 1) return geom->data.triangle.b;
      return geom->data.triangle.c;
  ```
- [x] Add `GEOM_TRIANGLE` to `gizmo_vertex_mode_set_local_pos()`:
  ```c
  case GEOM_TRIANGLE:
      if (idx == 0) geom->data.triangle.a = pos;
      else if (idx == 1) geom->data.triangle.b = pos;
      else geom->data.triangle.c = pos;
      break;
  ```

### 3.2 Undo/Redo for Vertex Edits

- [x] Verify `CMD_SET_GEOMETRY_VERTICES` works for GEOM_TRIANGLE (already works — `undo_set_vertex_pos()` handles GEOM_TRIANGLE since Sprint 2, and the gizmo snapshots vertices generically via `gizmo_vertex_mode_get/set_local_pos`)

### 3.3 Testing

- [x] Select triangle → Tab to geometry mode → 3 vertex dots appear
- [x] Click vertex → yellow highlight, gizmo arrows appear at vertex
- [x] Drag vertex with gizmo → triangle deforms in real time
- [x] Release → triangle stays deformed
- [x] Ctrl+Z → vertex returns to original position
- [x] Shift+Click multiple vertices → drag moves all selected
- [x] Tab back to transform mode → gizmo returns to entity center

---

## Sprint 4: Per-Vertex Color & Shader Variants

**Goal**: Support per-vertex color gradients and togglable shading modes.

### 4.1 Geometry Component Update — `src/components/geometry_comp.h`

- [ ] Extend `geom_triangle_data_t`:
  ```c
  typedef struct {
      vec3_t a, b, c;
      vec4_t color_a, color_b, color_c;  // Per-vertex colors (if has_vertex_colors)
      bool has_vertex_colors;
  } geom_triangle_data_t;
  ```
- [ ] Update `geometry_comp_triangle()` — default: no per-vertex colors (use GeometryComp.color)
- [ ] Add `geometry_comp_triangle_colored(a, b, c, color_a, color_b, color_c)` — per-vertex color version

### 4.2 Scene Update Integration

- [ ] Update GEOM_TRIANGLE case in `ecs_scene_update()`:
  - If `has_vertex_colors`: set instance color_a/b/c from vertex colors (with hover/selection override)
  - If not: replicate uniform color to all 3 instance color slots

### 4.3 Shading Mode Toggle

- [ ] Add `triangle_shading_mode` to scene or batch state:
  - `SHADING_FLAT_UNIFORM` — single color per triangle (default)
  - `SHADING_FLAT_VERTEX` — interpolated per-vertex colors
  - (Lit modes deferred to Sprint 5)
- [ ] For uniform mode: replicate GeometryComp.color to all 3 instance colors
- [ ] For vertex mode: use per-vertex colors if available, else fall back to uniform

### 4.4 UI Control

- [ ] Add shading mode dropdown to UI controls panel (or entity inspector for per-entity control)

### 4.5 Scene API Update

- [ ] `scene_add_triangle_colored(scene, a, b, c, color_a, color_b, color_c)` — per-vertex color variant

### 4.6 Serialization Update

- [ ] Serialize per-vertex colors when present:
  ```json
  { "type": "triangle", "a": [...], "b": [...], "c": [...],
    "color_a": [r,g,b,a], "color_b": [r,g,b,a], "color_c": [r,g,b,a] }
  ```

### 4.7 Testing

- Create triangle with 3 different vertex colors → smooth gradient visible
- Toggle between uniform and per-vertex mode → visual changes
- Save/load per-vertex color triangles → colors preserved
- Hover/selection coloring still works in per-vertex mode

---

## Sprint 5: Lighting System

**Goal**: 3-point studio lighting for triangles with togglable lit/unlit modes.

### 5.1 Light Component — `src/components/light_comp.h` (new file)

- [ ] Define light types:
  ```c
  typedef enum { LIGHT_DIRECTIONAL, LIGHT_POINT } light_type_t;
  ```
- [ ] Define `LightComp`:
  ```c
  typedef struct {
      light_type_t type;
      vec4_t color;       // RGB + unused alpha
      float intensity;    // Multiplier (0.0 - 10.0)
  } LightComp;
  ```
- [ ] Light position/direction derived from entity's TransformComp
  - Directional: forward direction from rotation (e.g., -Z in local space rotated by world_matrix)
  - Point: position from world_matrix translation

### 5.2 ECS World Integration — `src/ecs/ecs_world.h`

- [ ] Register `LightComp` as ECS component
- [ ] Register `Light` tag for easy querying
- [ ] `ecs_world_create_light()` helper

### 5.3 Scene API — `src/ecs/ecs_scene.h`

- [ ] `scene_add_directional_light(scene, direction, color, intensity)`:
  - Creates entity with TransformComp + LightComp
  - Direction encoded as rotation to orient -Z toward desired direction
  - No geometry/renderable (lights are invisible for now)
  - Returns entity handle
- [ ] `scene_add_point_light(scene, position, color, intensity)`:
  - Similar but uses position
- [ ] `scene_collect_lights(scene, out_lights, max_lights)`:
  - Query all Light entities, extract world-space position/direction + color/intensity
  - Output packed for shader uniform block
  - Returns count of lights found

### 5.4 Default 3-Point Studio Lighting

- [ ] Create at app init (in `app.c` or scene initialization):
  - **Key Light**: Directional, warm white (1.0, 0.95, 0.9), intensity 1.0, direction roughly (0.5, -0.7, -0.5) — front-right-above
  - **Fill Light**: Directional, cool blue-white (0.8, 0.85, 1.0), intensity 0.4, direction roughly (-0.5, -0.3, -0.5) — front-left
  - **Rim Light**: Directional, neutral (1.0, 1.0, 1.0), intensity 0.3, direction roughly (0.0, -0.2, 0.8) — behind

### 5.5 Shader Lighting Uniforms

- [ ] Define `triangle_fs_params_t` uniform block:
  ```c
  typedef struct {
      vec4_t light_dirs[8];       // Direction (directional) or position (point)
      vec4_t light_colors[8];     // RGB + intensity in W
      vec4_t ambient_color;       // Ambient light color + intensity in W
      int num_lights;             // Active light count
      int shading_mode;           // 0=flat, 1=flat+vertex_color, 2=lit, 3=lit+vertex_color
      float _pad[2];
  } triangle_fs_params_t;
  ```
- [ ] Update all triangle fragment shaders (all backends) with lighting calculation:
  ```glsl
  if (shading_mode >= 2) {
      vec3 N = normalize(v_normal);
      vec3 diffuse = vec3(0.0);
      for (int i = 0; i < num_lights; i++) {
          vec3 L = normalize(-light_dirs[i].xyz);  // directional
          float NdotL = max(dot(N, L), 0.0);
          diffuse += light_colors[i].rgb * light_colors[i].w * NdotL;
      }
      vec3 ambient = ambient_color.rgb * ambient_color.w;
      color.rgb *= (ambient + diffuse);
  }
  ```
- [ ] Update SPIR-V shaders and recompile

### 5.6 Rendering Integration

- [ ] Before triangle draw call: collect lights from ECS, fill `triangle_fs_params_t`
- [ ] Pass as fragment shader uniform block to `sg_apply_uniforms()`
- [ ] Existing points and lines remain unaffected (flat shaded, no lighting uniform)

### 5.7 Shading Mode Extension

- [ ] Extend shading modes to 4 variants:
  - `SHADING_FLAT_UNIFORM = 0` — flat, single color
  - `SHADING_FLAT_VERTEX = 1` — flat, per-vertex color gradient
  - `SHADING_LIT_UNIFORM = 2` — lit, single color
  - `SHADING_LIT_VERTEX = 3` — lit, per-vertex color gradient
- [ ] UI dropdown to select mode
- [ ] Default: `SHADING_LIT_UNIFORM` for newly created triangles

### 5.8 Light Serialization

- [ ] Serialize lights in scene JSON:
  ```json
  { "type": "directional_light", "color": [r,g,b,a], "intensity": 1.0 }
  ```
  (Direction derived from transform rotation)
- [ ] Deserialize lights on scene load

### 5.9 Testing

- Triangles lit by 3-point studio setup → visible shading, shadows on faces
- Toggle SHADING_FLAT ↔ SHADING_LIT → lighting appears/disappears
- Rotate camera → lighting changes on triangle faces
- Move light entity (via inspector or gizmo) → triangle shading updates
- Save/load scene with lights → lighting preserved
- Points and lines remain flat-shaded (unaffected by lights)

---

## Sprint 6: Mesh Entities (Multi-Triangle)

**Goal**: Support indexed mesh entities containing multiple triangles as a single ECS entity.

### 6.1 Geometry Component — `src/components/geometry_comp.h`

- [ ] Add `GEOM_MESH = 9` to `geometry_type_t`
- [ ] Define `geom_mesh_data_t`:
  ```c
  typedef struct {
      vec3_t *vertices;         // Unique vertex positions
      vec3_t *normals;          // Per-vertex normals (smooth) or NULL (compute per-face)
      vec4_t *vertex_colors;    // Optional per-vertex colors, NULL if uniform
      int vertex_count;
      int vertex_capacity;
      uint32_t *indices;        // Triangle indices (groups of 3)
      int index_count;          // Must be multiple of 3
      int index_capacity;
  } geom_mesh_data_t;
  ```
- [ ] Add to GeometryComp union
- [ ] Dynamic allocation helpers: `geom_mesh_init()`, `geom_mesh_add_vertex()`, `geom_mesh_add_triangle()`, `geom_mesh_free()`
- [ ] `geom_mesh_compute_normals()` — average face normals at shared vertices for smooth shading
- [ ] `geom_mesh_face_count()` — returns `index_count / 3`

### 6.2 Scene API — `src/ecs/ecs_scene.h`

- [ ] `scene_add_mesh(scene, vertices, vertex_count, indices, index_count, color)`:
  1. Allocate `index_count / 3` contiguous triangle batch slots
  2. Create entity with Transform + Geometry (GEOM_MESH) + Selectable + Renderable
  3. Compute face normals (or vertex normals if provided)
  4. Expand indexed triangles → fill triangle instance slots
  5. Configure RenderableComp: `batch_id = GEOM_MESH`, `instance_slot = first_slot`, `segment_count = face_count`
  6. Return entity handle
- [ ] Add `GEOM_MESH` case in `ecs_scene_update()`:
  - For each face: transform 3 vertices through world_matrix
  - Recompute normals (or transform existing normals)
  - Apply hover/selection color
  - Write to contiguous triangle batch slots
- [ ] Add `GEOM_MESH` case in `scene_free_entity_slots()`:
  - Free `segment_count` contiguous triangle batch slots
- [ ] Add `GEOM_MESH` case in `ecs_scene_populate_pick_buffer()`:
  - All faces share entity's pick_id
  - Add each face as a triangle to pick buffer

### 6.3 Mesh Vertex Editing — `src/gizmo/gizmo_vertex_mode.h`

- [ ] Add `GEOM_MESH` to `gizmo_vertex_mode_get_vertex_count()` → return mesh vertex_count (unique vertices)
- [ ] Add `GEOM_MESH` to `gizmo_vertex_mode_get_local_pos()` → return mesh.vertices[idx]
- [ ] Add `GEOM_MESH` to `gizmo_vertex_mode_set_local_pos()` → set mesh.vertices[idx]
- [ ] Vertex editing automatically updates all faces sharing that vertex (because `ecs_scene_update` re-expands from indices each frame)

### 6.4 Undo/Redo & Serialization

- [ ] Add `UNDO_GEOM_MESH` to undo snapshot types
- [ ] Snapshot: deep copy vertices + indices arrays
- [ ] Recreate: allocate fresh mesh from snapshot data
- [ ] Serialize mesh in JSON:
  ```json
  {
    "type": "mesh",
    "vertices": [[x,y,z], ...],
    "indices": [0, 1, 2, 0, 2, 3, ...],
    "vertex_colors": [[r,g,b,a], ...] // optional
  }
  ```

### 6.5 Mesh Helper API

- [ ] `scene_add_mesh_quad(scene, a, b, c, d, color)` — convenience: 2 triangles
- [ ] `scene_add_mesh_box(scene, center, size, color)` — convenience: 12 triangles (6 faces)

### 6.6 Testing

- Create mesh with multiple faces → all faces rendered as one entity
- Click any face → entire mesh entity selected
- Move mesh → all faces move together
- Tab to geometry mode → vertex handles at unique vertices
- Drag shared vertex → all connected faces update
- Ctrl+Z → vertex returns, faces restore
- Save/load mesh → vertices + topology preserved
- Delete mesh → all face instances freed

---

## File Change Summary

### New Files
| File | Sprint | Description |
|------|--------|-------------|
| `src/shaders/instanced_triangle_shaders.h` | S1 | Triangle visual shaders (all backends) |
| `src/shaders/spirv/instanced_triangle.vert` | S1 | Vulkan triangle vertex shader source |
| `src/shaders/spirv/instanced_triangle.frag` | S1 | Vulkan triangle fragment shader source |
| `src/components/light_comp.h` | S5 | Light component definition |

### Modified Files
| File | Sprint(s) | Changes |
|------|-----------|---------|
| `src/components/geometry_comp.h` | S1, S4, S6 | GEOM_TRIANGLE, GEOM_MESH types + data structs |
| `src/gpu/geometry_batch.h` | S1 | Triangle batch type + functions |
| `src/gpu/pick_buffer.h` | S2 | Triangle pick pipeline + instances |
| `src/shaders/pick_shaders.h` | S2 | Triangle pick shaders |
| `src/ecs/ecs_scene.h` | S1-S6 | scene_add_triangle/mesh/light, update, pick, free |
| `src/ecs/ecs_world.h` | S5 | Register LightComp |
| `src/gizmo/gizmo_vertex_mode.h` | S3, S6 | GEOM_TRIANGLE + GEOM_MESH vertex editing |
| `src/undo_redo.h` | S2, S6 | Triangle/mesh snapshot types |
| `src/undo_redo_exec.h` | S2, S6 | Triangle/mesh snapshot/recreate |
| `src/scene_serializer.h` | S2, S4, S5, S6 | Triangle/mesh/light serialization |
| `src/app.c` | S1, S5 | Sample triangles, studio lights, integration |
| `src/shaders/spirv/spirv_bytecode.h` | S1, S2 | New SPIR-V bytecode arrays |
| `scripts/vulkan-win/compile-spirv.ps1` | S1, S2 | New shader entries |

### Unchanged
- `src/gpu/instance_buffer.h` — generic, works for any instance size
- `src/shaders/instanced_line_shaders.h` — untouched
- `src/shaders/join_shaders.h` — untouched
- `src/orbit_camera.h` — untouched
- `src/render_target.h` — untouched
- Points and lines rendering — completely unaffected

---

## Key Design Decisions

1. **96-byte instances with 3 colors always present**: Avoids shader branching and separate pipelines for uniform vs per-vertex color. Memory cost is modest (1M triangles = 96MB, well within GPU budget).

2. **Face normal stored per-instance**: Allows flat shading per triangle. For smooth shading (future), we'd interpolate normals in the vertex shader using the same barycentric selector trick.

3. **Lights as ECS entities**: Enables gizmo manipulation, serialization, undo/redo for lights using existing infrastructure. Light data collected each frame and packed into shader uniforms.

4. **GEOM_TRIANGLE (single) vs GEOM_MESH (indexed)**: Mirrors GEOM_LINE vs GEOM_POLYLINE pattern. Single triangle is simple and complete first. Mesh uses contiguous slot allocation for multiple faces per entity.

5. **Double-sided triangles**: Cull mode NONE initially. Face culling can be added as a per-entity or global toggle later.

6. **Deferred tessellation pattern NOT used for triangles**: Unlike arcs/beziers which re-tessellate each frame, triangles use their stored vertices directly. Mesh `ecs_scene_update` expands indexed faces each frame (similar to how polyline segments are updated each frame).

7. **Lighting only affects triangles**: Points and lines remain flat-shaded. The triangle shader has its own uniform block with light data. This avoids touching any existing shader code.
