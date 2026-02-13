# CHANGELOG

This file contains changes archived from CHECKPOINT.md and can be referred to if necessary.

## Session (2026-02-12)

### ECS Batch Parenting and other fixes (IMPLEMENTED)

  **Sprint 1: Infrastructure**

  - ecs_world.h: Added ImportPending_tag (zero-size tag), ecs_world_create_anchor_entity() (TransformComp-only), ecs_world_set_parent_batch()
  (all children → one parent, deferred), ecs_world_set_parents_batch() (each child → own parent, deferred)
  - ecs_scene.h: Added scene_add_anchor(), scene_set_parent_batch(), scene_set_parents_batch() wrappers. Updated both
  ecs_scene_update_transforms() and ecs_scene_update() queries to skip ImportPending entities

  **Sprint 2: PLY Import**

  - ply_import_job.h: Added PLY_JOB_PARENTING state, created_entities[] tracking array, anchor_entity field. Entity creation now tags with
  ImportPending and tracks IDs. After creation completes, transitions to PLY_JOB_PARENTING which creates a transform-only anchor and
  batch-parents all entities in one deferred block (removing ImportPending simultaneously). Updated cancel/reset/is_running handlers.
  - ui_scene_hierarchy.h: Progress UI shows "Parenting..." during PLY_JOB_PARENTING state

  **Sprint 3: JSONL Import**

  - jsonl_import_job.h: Replaced scene_add_point() invisible anchors with scene_add_anchor() (no GPU slot waste). Entity creation tags with
  ImportPending. Replaced chunked scene_set_parent() loop with single deferred batch (all parenting + ImportPending removal in one
  ecs_defer_begin/end block). Updated cancel handler to remove ImportPending tags.

  **Sprint 4: Scene Serializer**

  - scene_serializer.h: Replaced per-entity scene_set_parent() loop with scene_set_parents_batch() for O(n) batched parenting

  **Additional fixes**

  - geometry_comp.h: geometry_type_name() returns "Anchor" for GEOM_TYPE_COUNT sentinel
  - ui_scene_hierarchy.h: Hierarchy cache now includes anchor entities (LabelComp + TransformComp, no GeometryComp)
  - jsonl_loader.h: Fixed pre-existing _stricmp build error on non-Windows platforms (uses strcasecmp)

### JSONL Mesh Support - MeshBody Import (IMPLEMENTED)

Extended the JSONL geometry log loader to support `MeshBody` elements from .NET geometry exports. Meshes can be imported in two modes matching the PLY mesh import pattern.

**Features:**
- **Parser Extension:** Added `JSONL_GEOM_MESH` type with `jsonl_mesh_data_t` struct to parse `_Points`, `_Normals`, and `_Indices` arrays from MeshBody JSON
- **Two Import Modes:**
  - **Single Mesh Entity (efficient):** Indexed mesh rendered as one entity, single pick selection
  - **Individual Triangles (selectable):** Each face as separate entity for per-triangle selection/editing
- **Enhanced Colour Parsing:** Now supports named colours (White, Red, Blue, etc.), RGB format "R, G, B", and ARGB format "A, R, G, B"
- **UI Integration:** Import options dialog shows mesh vertex/face counts and mode selection radio buttons when MeshBody elements detected
- **Transform Support:** Mesh vertices participate in CoM shift, rotation, and scale transforms

**Modified Files:**
- `src/jsonl_loader.h` - Added `JSONL_GEOM_MESH` to `jsonl_geom_type_t`; added `jsonl_mesh_data_t` struct; implemented `jsonl_parse_mesh_body()` function; enhanced `jsonl_parse_colour()` with named colours and ARGB support; added `jsonl_quick_scan_mesh()` and mesh detection helpers
- `src/jsonl_import_job.h` - Added `mesh_import_mode` field; extended `jsonl_import_job_apply_transforms()` for mesh vertices/normals; added mesh entity creation in both modes with chunked triangle creation for Individual Triangles mode; added `jsonl_import_job_set_mesh_mode()` setter
- `src/ui/ui_scene_hierarchy.h` - Added mesh UI state fields (`jsonl_has_mesh_data`, `jsonl_mesh_import_mode`, vertex/face counts); updated quick scan to use `jsonl_quick_scan_mesh()`; added mesh import mode radio buttons to JSONL import options dialog

**Plan File:** `.plans/PLAN_JSONL_MESH_SUPPORT.md`

### Bug Fix: Light Entity Jumping in Scene Hierarchy (FIXED)

Fixed light entities jumping to the end of the list when selected in the Scene Hierarchy. Root cause: `selection_set_single()` calls `ecs_world_select()` which adds a `Selected` tag via `ecs_add_id()`, changing the entity's archetype in Flecs. This caused the ECS query iteration order to change on the next frame, making the selected light appear at a different position. Fix: collect light entities into a local array sorted by entity ID before rendering, matching the approach used by the geometry entity cache. Same underlying issue was previously solved for geometry entities by the cached/sorted entity list.

**Modified Files:**
- `src/ui/ui_scene_hierarchy.h` - Rewrote `ui_scene_hierarchy_draw_lights_section()`: replaced two-pass query (count + render) with single-pass collect into local `light_entities[16]` array; added `qsort` by entity ID via new `ui_hierarchy_compare_light_entries()` comparator; render from sorted array using `ecs_world_get_light()` per-entity lookup

### Enhancement: Triangles Tab in Slot Buffer Debug Window (IMPLEMENTED)

Added a "Triangles" tab to the Slot Buffer Debug window (alongside existing Lines and Points/Joins tabs). Displays the triangle instance buffer grid with color-coded cells for Triangle (orange) and Mesh Face (purple) slot types. Tooltip on hover shows vertex positions (A/B/C), face normal, and color at vertex A. Supports the same cell size controls, pagination, and color legend as existing tabs.

**Modified Files:**
- `src/ui/ui_slot_buffer_debug.h` - Added `SLOT_TYPE_TRIANGLE` and `SLOT_TYPE_MESH_FACE` to `slot_type_t` enum with orange/purple colors; added `slot_debug_geom_to_slot_type_triangle()` mapping function; added `slot_buffer_type_t` enum replacing `is_point_buffer` bool parameter in `ui_slot_buffer_debug_draw_buffer()`; added `page_triangles` state field; added "Triangles" tab rendering `scene->batches.triangles.instances`; triangle tooltip shows `geom_triangle_instance_t` vertex/normal/color data

## Session (2026-02-11)

### Mesh Features - Sprint 6: OBJ to PLY Conversion Script (IMPLEMENTED)

Added Python utility `scripts/obj_to_colored_ply.py` for converting OBJ mesh files to colored PLY files. Parses OBJ vertices and faces (handles all face formats: `f v`, `f v/vt`, `f v/vt/vn`, `f v//vn`), fan-triangulates quads and n-gons, and generates random colors using four palettes (rainbow, pastel, earth, grayscale). Outputs both ASCII and binary_little_endian PLY format. Supports per-vertex and per-face color modes, configurable alpha, and reproducible colors via seed. Compatible with mdCAD's PLY mesh import (Sprint 5). Plan file: `.plans/PLAN_MESH_FEATURES.md`

**New File:**
- `scripts/obj_to_colored_ply.py` - Standalone Python script with argparse CLI; OBJ parser with negative index support; HSV-based color palettes; ASCII and binary PLY writers with proper header format

### Mesh Features - Sprint 5: PLY Mesh Import Job & UI (IMPLEMENTED)

Added full user-facing PLY mesh import workflow with file browser, import options dialog, and progress bar. Two import modes: "Single Mesh Entity" (efficient, one indexed mesh entity via `scene_add_mesh_colored()`/`scene_add_mesh()`) and "Individual Triangles" (each face as a separate selectable entity via chunked `scene_add_triangle()`). Supports per-vertex colors, per-face colors (auto-converted to per-vertex by averaging at shared vertices), and uniform default color. Import options include unit conversion (m/mm/in), CoM shift, and X/Y/Z rotation. Progress popup with timing plot for large meshes. Works with both ASCII and binary PLY files. Plan file: `.plans/PLAN_MESH_FEATURES.md`

**New File:**
- `src/ply_mesh_import_job.h` - Header-only mesh import job state machine with 3-phase pipeline (`PARSING_VERTICES` -> `PARSING_FACES` -> `CREATING_ENTITIES`), coordinate transforms, face-color-to-vertex-color conversion, progress tracking, cancellation support

**Modified Files:**
- `src/ui/ui_scene_hierarchy.h` - Added `#include "../ply_mesh_import_job.h"`; added 17 mesh import state fields to `ui_scene_hierarchy_state_t`; init/shutdown for mesh import browser and job; "Import PLY Mesh..." menu item under File; file browser handler with `ply_get_mesh_info()` validation; "Import PLY Mesh Options" modal popup (mode selection, color options, unit conversion, transforms); "Importing PLY Mesh" progress modal with timing plot

### Mesh Features - Sprint 4: PLY Mesh Parser - ASCII (IMPLEMENTED)

Extended `ply_loader.h` to parse triangle mesh data from binary PLY files.

**Modified Files:**
- `src/ply_loader.h` extended `ply_parse_vertices_chunk()` and `ply_parse_faces_chunk()` to handle binary mode: read `chunk_size * vertex_byte_stride` bytes at once; parse from memory buffer rather than line-by-lines

### Mesh Features - Sprint 3: PLY Mesh Parser - ASCII (IMPLEMENTED)

Extended `ply_loader.h` to parse triangle mesh data from ASCII PLY files. The existing parser only handled vertex positions and colors (point clouds), skipping face elements entirely. Now parses `element face` headers (vertex_indices list + optional face colors), ASCII face data lines with fan triangulation for quads/polygons, and provides both one-shot (`ply_load_mesh_file`) and incremental/chunked (`ply_parse_faces_chunk`) APIs for mesh loading. Output is compatible with `scene_add_mesh()` / `scene_add_mesh_colored()`. All changes are backward-compatible - existing point cloud import works unchanged. Plan file: `.plans/PLAN_MESH_FEATURES.md`

**Modified Files:**
- `src/ply_loader.h` - Extended `ply_property_t` with list property support; added `ply_face_data_t` and `ply_mesh_data_t` types; extended `ply_header_t` with face element fields; updated `ply_parse_header()` with element state machine; added `ply_parse_ascii_faces()` with fan triangulation; added `ply_load_mesh_file()`, `ply_mesh_data_free()`, `ply_get_mesh_info()`; extended `ply_parse_state_t` with face arrays; updated `ply_open()`, `ply_get_progress()`, `ply_is_complete()`, `ply_parse_state_free()`; added `ply_vertices_complete()`, `ply_parse_faces_chunk()`, `ply_parse_state_to_mesh_data()`

### Mesh Features - Sprint 2: Light Entities in Hierarchy & Inspector (IMPLEMENTED)

Made light entities visible and editable from the UI. Scene hierarchy now has a collapsible "Lights" section (collapsed by default) that lists all light entities by type and ID. Clicking a light selects it; Ctrl+click toggles multi-select. The Entity Inspector shows a "Light" section for light entities with read-only type display, editable color picker, editable intensity drag, and direction/position info derived from the transform. Light property changes take effect immediately since `scene_collect_lights()` reads live component data each frame. No undo/redo for light properties in this sprint. Plan file: `.plans/PLAN_MESH_FEATURES.md`

**Modified Files:**
- `src/ui/ui_scene_hierarchy.h` - Added `ui_scene_hierarchy_draw_lights_section()` function that queries `LightComp` entities and renders them as selectable tree leaves; called from `ui_scene_hierarchy_draw()` after the entity list
- `src/ui/ui_entity_inspector.h` - Added Light section in `ui_entity_inspector_draw_single()` after the Renderable section, with type display, `igColorEdit3` for color, `igDragFloat` for intensity, and direction/position readout from transform

### Mesh Features - Sprint 1: Add Entity Menu Items (IMPLEMENTED)

Added "Triangle (Mesh)" and "Test Mesh (Box)" menu items to the Add Entity menu. Both appear after the Helix item, separated by a visual divider. Triangle creates an equilateral mesh triangle in the XY plane using `scene_add_triangle()`. Box creates a unit-sized mesh box at origin using `scene_add_mesh_box()`. Both use random colors and are covered by existing undo/redo infrastructure. Plan file: `.plans/PLAN_MESH_FEATURES.md`

**Modified Files:**
- `src/ui/ui_scene_hierarchy.h` - Added `igSeparator()`, "Triangle (Mesh)" and "Test Mesh (Box)" menu items in `ui_scene_hierarchy_draw_add_menu()`, before the undo recording block

### Mesh Triangles - Sprint 6: Mesh Entities / Multi-Triangle Indexed (IMPLEMENTED)

Added indexed triangle mesh as a new geometry type (`GEOM_MESH`). Meshes store shared vertices with an index buffer and expand to individual triangle batch slots each frame (reusing the existing instanced triangle infrastructure). Includes area-weighted smooth vertex normals, per-vertex colors, full undo/redo with deep-copy snapshots, JSON serialization, gizmo vertex editing, and GPU picking (all faces share one pick_id). Convenience helpers for quads and boxes. Plan file: `.plans/PLAN_MESH_TRIANGLES.md`

**Modified Files:**
- `src/components/geometry_comp.h` - Added `GEOM_MESH = 9` enum; `geom_mesh_data_t` struct (vertices, normals, vertex_colors, indices with counts/capacities); helpers: `geom_mesh_init`, `geom_mesh_add_vertex`, `geom_mesh_add_triangle`, `geom_mesh_face_count`, `geom_mesh_compute_normals`, `geom_mesh_free`; `geometry_comp_mesh()` factory; updated `geometry_comp_free()`
- `src/ecs/ecs_scene.h` - Added `scene_add_mesh()`, `scene_add_mesh_colored()`, `scene_add_mesh_quad()`, `scene_add_mesh_box()`; GEOM_MESH handling in `scene_free_entity_slots()`, `ecs_scene_update()` (visibility hide + dirty update with face-normal expansion and per-vertex color interpolation), `ecs_scene_populate_pick_buffer()`
- `src/gizmo/gizmo_vertex_mode.h` - Added GEOM_MESH cases in `get_vertex_count`, `get_local_pos`, `set_local_pos`
- `src/undo_redo.h` - Added `UNDO_GEOM_MESH` enum; mesh snapshot (vertices, normals, vertex_colors, indices) in `undo_entity_snapshot_t` union; updated `undo_command_free()` for mesh deep-copy cleanup
- `src/undo_redo_exec.h` - Updated `undo_snapshot_entity()` with GEOM_MESH deep-copy; `undo_create_from_snapshot()` with mesh recreation; `undo_set_vertex_pos()` for mesh vertices
- `src/scene_serializer.h` - Added `json_parse_uint_array()`, `json_parse_vec4_array()` helpers; mesh write (vertices/indices/vertex_colors arrays); mesh parse and `scene_create_from_loaded()` GEOM_MESH case; cleanup in all error/success paths
- `src/ui/ui_entity_inspector.h` - Added GEOM_MESH case showing vertex count, face count, index count, per-vertex color status
- `src/app.c` - Added sample gold quad and steel blue box after existing triangle samples

**Key Design Decisions:**
- **Indexed mesh expanding to triangle batch**: Meshes store compact shared-vertex data but expand to individual triangle instance slots each frame. This reuses the existing instanced triangle rendering pipeline with no shader changes.
- **Area-weighted smooth normals**: `geom_mesh_compute_normals()` accumulates cross-product normals (proportional to triangle area) at each vertex, then normalizes. Larger triangles contribute more to the average.
- **Contiguous slot allocation**: Like polylines/polygons, mesh faces use `instance_buffer_alloc_contiguous(face_count)` with `segment_count` tracking the slot count for proper cleanup.
- **Single pick_id for all faces**: All triangle slots of a mesh share the entity's pick_id, so clicking any face selects the whole mesh entity.

### Mesh Triangles - Sprint 5: Lighting System (IMPLEMENTED)

Added 3-point studio lighting for triangle entities. Lighting is computed in the vertex shader using Lambertian diffuse + ambient, with `abs(dot(N,L))` for double-sided triangles. Lights are ECS entities (TransformComp + LightComp) with full serialization. Global lighting toggle in the Visibility panel. Points and lines remain flat-shaded. Plan file: `.plans/PLAN_MESH_TRIANGLES.md`

**New File:**
- `src/components/light_comp.h` - `LightComp` with `light_type_t` (DIRECTIONAL/POINT), color, intensity, and factory helpers

**Modified Files:**
- `src/ecs/ecs_world.h` - Registered `LightComp_id`; added `ecs_world_get_light()` accessor
- `src/ecs/ecs_scene.h` - Added `scene_add_directional_light()`, `scene_add_point_light()`, `scene_collect_lights()`; light entities have TransformComp + LightComp only (no geometry/renderable/selectable)
- `src/gpu/geometry_batch.h` - Expanded `geom_triangle_params_t` from 64→224 bytes (MVP + 4 lights + ambient + flags); updated `geom_triangle_batch_draw()`, `geometry_batch_manager_draw()` signatures; shader descriptor declares full uniform layout
- `src/shaders/instanced_triangle_shaders.h` - All 5 text backends updated: VS computes `v_lighting` varying from face normal + light uniforms; FS multiplies `v_color.rgb * v_lighting`
- `src/shaders/spirv/instanced_triangle.vert` - Updated GLSL 450 with lighting uniforms and computation
- `src/shaders/spirv/instanced_triangle.frag` - Updated to receive and apply `v_lighting` varying
- `src/app.c` - Creates 3 studio lights at init (key/fill/rim); builds `geom_triangle_params_t` with collected light data per frame; added `lighting_enabled` state
- `src/ui/ui_visibility.h` - Added "Enable Lighting" checkbox with tooltip and `lighting_enabled` pointer
- `src/scene_serializer.h` - Added `scene_write_light_json()` for light entity save; save queries both geometry + light entities; added `json_parse_light()` for load; `loaded_entity_t` extended with light fields; scene clear also deletes light entities

**Key Design Decisions:**
- **VS-computed lighting**: Face normals are constant per-triangle, so VS gives identical results to FS for flat shading. Avoids adding a separate FS uniform block across 6 backends. Single VS uniform block (block 0, 224 bytes) carries MVP + lighting data.
- **Global toggle, not per-entity mode**: Lighting enabled/disabled globally via Visibility panel. Per-vertex vs uniform color is orthogonal (handled at instance level). Simpler than 4 per-entity shading modes.
- **Lights as ECS entities**: Enables serialization using existing infrastructure. Light data collected each frame and packed into shader uniforms (max 4 lights).
- **Double-sided via abs(NdotL)**: `abs(dot(N, L))` handles both front and back faces without requiring face culling or `gl_FrontFacing`.

**Note:** Vulkan SPIR-V bytecode must be recompiled after shader source changes: `scripts/vulkan-win/build-all.ps1`

### Mesh Triangles - Sprints 1-4 (IMPLEMENTED)

Sprint 1 added instanced triangle rendering as a new geometry primitive (shaders, batch, scene API). Sprint 2 added GPU picking, undo/redo, and JSON serialization. Sprint 3 added gizmo vertex editing. Sprint 4 added per-vertex color support with entity inspector UI. See `.plans/PLAN_MESH_TRIANGLES.md` for full details.

## Session (2026-02-10)

### Licensing, Attribution & Help -> About Window (IMPLEMENTED)

Added MIT license, third-party attributions, in-app About window, and project README.

**New Files:**
- `LICENSE` - MIT license (Copyright (c) 2026 Rodion Radchenko and mdCAD contributors)
- `THIRD_PARTY_LICENSES.md` - Full license texts for all third-party libraries
- `scripts/gather_licenses.py` - Script to regenerate `THIRD_PARTY_LICENSES.md` from vendor/build LICENSE files (with hardcoded fallbacks for vendored amalgamations)
- `src/ui/ui_about.h` - Help -> About window (header-only, follows project conventions)
- `README.md` - Project front page with features, quick start, platform table, architecture overview

**Modified Files:**
- `src/ui/ui_scene_hierarchy.h` - Integrated About window: added include, state field, init call, Help menu after Edit menu, draw call

**About Window Contents:**
- App name "mdCAD" + version (v0.1.0)
- MIT license notice + copyright line
- Collapsing headers for each third-party library (Sokol, Dear ImGui, cimgui, Flecs, cJSON) showing copyright, license type, and URL
- Reference to `THIRD_PARTY_LICENSES.md` for full texts
- Close button

**Third-Party Libraries Covered:**

| Library | License | Copyright |
|---------|---------|-----------|
| Sokol | zlib/libpng | (c) 2018 Andre Weissflog |
| Dear ImGui | MIT | (c) 2014-2024 Omar Cornut |
| cimgui | MIT | (c) 2015 Stephan Dilly |
| Flecs | MIT | (c) 2019 Sander Mertens |
| cJSON | MIT | (c) 2009-2017 Dave Gamble and cJSON contributors |

### Documentation Tidy-Up
- Fixed stale binary name references in CHECKPOINT.md build commands (`skl_tmp` -> `mdCAD`)
- Removed obsolete `docs/CMAKE_FIX.md`

## Session (2026-02-09)

### 3D Manipulator Gizmo (IMPLEMENTED)
Full translation gizmo with two modes, completely separate from ECS scene system.

**New Files:**
- `src/gizmo/gizmo.h` - Main state machine (HIDDEN/IDLE/HOVER/DRAGGING), two modes (Transform/Geometry)
- `src/gizmo/gizmo_rendering.h` - Own GPU pipelines (depth-always, no depth write) + stream instance buffers
- `src/gizmo/gizmo_vertex_mode.h` - Geometry mode vertex handles, selection, and editing

**Modified Files:**
- `src/math3d.h` - Added ray_t, mat4_inverse, ray_from_screen, ray_axis_closest_t, ray_plane_intersect
- `src/components/selectable_comp.h` - Added reserved pick ID ranges (GIZMO_HANDLE_BASE, VERTEX_HANDLE_BASE)
- `src/ecs/ecs_world.h` - Capped pick ID allocator below reserved range
- `src/ui/ui_viewport.h` - Added suppress_camera_input flag
- `src/undo_redo.h` - Added CMD_SET_GEOMETRY_VERTICES command type + data struct
- `src/undo_redo_exec.h` - Added recording, apply, and unapply for geometry vertex editing
- `src/gpu/pick_buffer.h` - Added overlay pipelines + instance buffers for depth-always gizmo picking
- `src/app.c` - Full gizmo integration: init, Tab toggle, update, draw, pick routing, drag, undo

**Key Design:**
- Gizmo uses reserved pick IDs (16767200+) at top of 24-bit range
- Translation only (rotation/scale deferred to future sprints)
- Transform mode: moves whole entities via TransformComp.position
- Geometry mode (Tab): moves individual vertices of GEOM_POINT/LINE/POLYLINE/POLYGON
- Constant screen-size: scale = dist * tan(fov/2) * 0.15
- Camera suppressed when gizmo is hovered or dragging
- 3 axis lines (RGB) + 3 tip dots + 3 plane handle rectangles (XY/XZ/YZ)
- Drag constraints: axis (closest-point-on-axis) or plane (ray-plane intersection)
- Undo: CMD_SET_POSITION per entity (transform mode), CMD_SET_GEOMETRY_VERTICES (geometry mode)
- Auto-selects vertex 0 when entering geometry mode or selecting entity in geometry mode

**Pick Buffer Overlay:**
- Gizmo handles and vertex dots use `pick_buffer_add_overlay_line/point()` instead of normal add functions
- Overlay uses separate instance buffers + `SG_COMPAREFUNC_ALWAYS` pipelines in pick pass
- Overlay rendered after normal scene picks → gizmo always pickable on top of scene geometry
- Fixes issue where gizmo handles were depth-tested against scene geometry in pick buffer

**Bugs Fixed:**
- Axis drag feedback loop: `ray_axis_closest_t` used moving `g->center` → exponential runaway. Fixed by storing `drag_origin` at drag start as fixed reference.
- Axis drag reversed: `w` vector in `ray_axis_closest_t` had wrong sign. Fixed `vec3_sub(ray.origin, axis_origin)` → `vec3_sub(axis_origin, ray.origin)`.
- Pick buffer depth: gizmo picks were buried behind scene geometry. Fixed by adding overlay pipelines with depth-always to pick buffer.

### Vulkan Backend for MinGW (COMPLETED - Phase 1)

Implemented Vulkan graphics backend for MinGW builds on Windows, replacing OpenGL for better performance.

**Key Implementation Details:**
- Vulkan requires precompiled SPIR-V bytecode (other backends use source strings)
- Created GLSL 450 shader sources in `src/shaders/spirv/*.vert/*.frag`
- Compiled to `.spv` bytecode using `glslc` from Vulkan SDK
- Generated `spirv_bytecode.h` with embedded C byte arrays
- Shader loading uses `.bytecode` + `SG_RANGE()` instead of `.source`

**Files Added:**
- `src/shaders/spirv/` - GLSL 450 sources and compiled SPIR-V
- `scripts/vulkan-win/compile-spirv.ps1` - GLSL → SPIR-V compilation
- `scripts/vulkan-win/generate-bytecode-header.ps1` - SPIR-V → C header
- `scripts/vulkan-win/build-all.ps1` - Full shader build pipeline
- `docs/VULKAN_WINDOWS.md` - Developer documentation

**Files Modified:**
- `src/platform.h` - Added `SOKOL_VULKAN` for MinGW
- `vendors/libsokol/CMakeLists.txt` - Vulkan SDK linking for MinGW
- `src/CMakeLists.txt` - Vulkan readback source for MinGW
- `src/shaders/*_shaders.h` - Added SPIR-V bytecode includes for Vulkan
- `src/gpu/geometry_batch.h` - Conditional bytecode vs source shader loading
- `src/gpu/pick_buffer.h` - Same conditional loading pattern
- `src/app.c` - Increased Vulkan staging buffer to 64MB for large point clouds

**Vulkan-Specific Quirk:**
The Vulkan staging buffer defaults to 16MB. Large point clouds (1M+ points) caused rendering glitches until we increased `sg_desc.vulkan.stream_staging_buffer_size` to 64MB.

### Vulkan Pick Readback (IMPLEMENTED)

Implemented full GPU picking support for the Vulkan backend:

**Challenge:**
Sokol provides `sg_mtl_query_image_info()` for Metal and `sg_d3d11_query_image_info()` for D3D11 to get native texture handles, but no equivalent `sg_vk_query_image_info()` exists for Vulkan.

**Solution:**
Created a custom Sokol extension (`sg_vk_query_image_info_ext`) in `vendors/libsokol/sokol.c` that accesses Sokol's internal `_sg` state to extract `VkImage` handles.

**Implementation Details:**
- Uses `sapp_get_environment()` to get VkDevice, VkPhysicalDevice, VkQueue from sokol_app
- Creates a host-visible staging buffer for CPU readback
- Issues image layout transition (ATTACHMENT_OPTIMAL → TRANSFER_SRC_OPTIMAL)
- Copies image to staging buffer via `vkCmdCopyImageToBuffer`
- Transitions image back to ATTACHMENT_OPTIMAL for continued rendering
- Maps staging buffer and copies to CPU pixel array

**Files Modified:**
- `vendors/libsokol/sokol.c` - Added `sg_vk_query_image_info_ext()` custom extension
- `src/gpu/pick_readback_vulkan.c` - Full Vulkan readback implementation (was placeholder)

**Note:** The custom extension depends on Sokol internals and may need updating if Sokol changes its internal structures. If Sokol adds official `sg_vk_query_image_info()`, this code should be migrated.

### Visual Studio 2026 Vulkan Option (COMPLETED - Phase 2)

Added Vulkan as an optional backend for Visual Studio 2026 (MSVC) builds.

**CMake Option:**
```powershell
# Default: D3D11 backend
cmake -B build-msvc -G "Visual Studio 18"

# Optional: Vulkan backend
cmake -B build-vulkan -G "Visual Studio 18" -DUSE_VULKAN=ON
```

**Files Modified:**
- `vendors/libsokol/CMakeLists.txt` - Added `USE_VULKAN` option with conditional Vulkan/D3D11 linking
- `src/platform.h` - Added `USE_VULKAN` compile definition handling for MSVC
- `src/CMakeLists.txt` - Updated pick readback source selection for MSVC+Vulkan

**Implementation Notes:**
- `USE_VULKAN=ON` passes `USE_VULKAN=1` compile definition via `target_compile_definitions()`
- Platform detection in `platform.h` checks for `USE_VULKAN` before defaulting to D3D11
- Same SPIR-V bytecode shaders used for both MinGW and MSVC Vulkan builds
- Pick readback uses the same custom `sg_vk_query_image_info_ext()` extension

### FPS Debug Window (IMPLEMENTED)

Added FPS counter with rolling plot visualization:
- Live FPS counter displayed at top-center
- Rolling plot sampling every 2 seconds (last 100 samples)
- Autoscaling Y-axis to highest observed value
- Toggle via Visibility panel → Debug Windows → FPS Debug

**Files Added:**
- `src/ui/ui_fps_debug.h` - FPS debug window implementation

**Files Modified:**
- `src/ui/ui_visibility.h` - Added FPS debug toggle
- `src/app.c` - Integrated FPS debug state and rendering

## Session (2026-02-08)

### JSONL Geometry Log Importer (IMPLEMENTED)
Full JSONL geometry log import system, mirroring PLY import patterns:

**New Files:**
- `vendors/cjson/cJSON.h`, `vendors/cjson/cJSON.c` - cJSON library (MIT, single-file JSON parser)
- `vendors/cjson/CMakeLists.txt` - CMake for cJSON static library
- `src/jsonl_loader.h` - Header-only JSONL parser with:
  - Incremental line-by-line parsing (chunked across frames)
  - Geometry type extraction from .NET $type field (class name suffix only, no namespace dependency)
  - Support for Point3D, Line3D, Arc3D, PolyLine3D, Polygon3D
  - Arc3D conversion (Circle.Center/Radius/Axis + StartPoint/EndPoint/Direction -> center/radius/normal/start_angle/end_angle)
  - Colour parsing ("R, G, B" or "R, G, B, A", alpha always 1.0)
  - Quick scan function for entry/element counts without full parse
- `src/jsonl_import_job.h` - Import job state machine with:
  - States: IDLE, PARSING_FILE, CREATING_ENTITIES, PARENTING_ENTITIES, COMPLETE, CANCELLED, ERROR
  - Progress weighting: 0-20% parsing, 20-80% entity creation, 80-100% parenting
  - Chunk sizes: 50 lines/frame, 200 entities/frame, 500 parenting ops/frame
  - Synchronous import for small files (<5 lines)
  - Transform application: CoM shift, rotation (X->Y->Z), scale - baked into coordinates
  - Entity hierarchy: Root anchor -> Entry anchors -> Geometry entities
  - Timing data with ring buffer for speed plot

**Modified Files:**
- `vendors/CMakeLists.txt` - Added cjson subdirectory
- `src/CMakeLists.txt` - Added cjson to target_link_libraries
- `src/ui/ui_scene_hierarchy.h`:
  - Added JSONL import state fields (browser, options, job)
  - Added "Import JSONL Geometry Log..." menu item in File menu
  - Added JSONL file browser handler with quick-scan
  - Added "Import JSONL Options" modal popup (units, colours, CoM shift, rotation)
  - Added "Importing JSONL Geometry Log" progress modal (progress bar, timing, speed plot)
  - Added JSONL initialization/shutdown code

## Session (2026-02-04)

### PLY Import Quality of Life Improvements (IMPLEMENTED)

Added import transformation options and simplified the import pipeline:

**New Import Options (both Point Cloud and Editable modes):**
- **Shift to Centre of Mass (CoM)**: Checkbox to calculate CoM and shift all points so CoM is at origin
- **Rotation around X, Y, Z axes**: Drag controls for rotation in degrees - useful for coordinate system conversion when scanner systems have different orientations

**Transformation Order:**
1. CoM shift (if enabled)
2. Rotation (X → Y → Z order)
3. Scale (unit conversion)

**Simplified Pipeline:**
- Removed parenting entirely for Editable mode - points are now top-level entities
- Scale, rotation, and CoM shift are "baked" into point coordinates at import time
- This eliminates the O(K²) parenting bottleneck completely
- Progress is now: 0-30% parsing, 30-100% entity creation (Editable mode)

**Modified Files:**
- `src/math3d.h`:
  - Added `vec3_neg()` helper
  - Added `mat4_mul_point()` to transform a point by a matrix

- `src/ply_import_job.h`:
  - Removed `PLY_JOB_PARENTING_ENTITIES` state
  - Removed `root_entity`, `created_entities`, `parented_count` fields
  - Added `shift_to_com`, `rotation_x/y/z` import options
  - Added `com`, `transform_matrix`, `transforms_applied` for computed transforms
  - Added `ply_import_job_apply_transforms()` internal function
  - Updated `ply_import_job_start()` signature with new parameters
  - Simplified entity creation - no parenting, transforms applied to coordinates

- `src/ui/ui_scene_hierarchy.h`:
  - Added `ply_shift_to_com` and `ply_rotation[3]` state fields
  - Added UI controls for CoM checkbox and rotation drag floats
  - Updated progress display to remove parenting phase references

### PLY Import Progress Bar (IMPLEMENTED)
Implemented chunked PLY import with progress bar for large files:

**New Files:**
- `src/ply_import_job.h` - Import job state machine with:
  - Job states: IDLE, PARSING_VERTICES, CREATING_ENTITIES, COMPLETE, CANCELLED, ERROR
  - Chunked processing: 5000 vertices/frame parsing, 500 entities/frame creation
  - Progress tracking (0.0-1.0 with status messages)
  - Cancellation support with proper cleanup

**Modified Files:**
- `src/ply_loader.h`:
  - Added `ply_parse_state_t` struct for incremental parsing
  - Added `ply_open()` - open file and parse header only
  - Added `ply_parse_vertices_chunk()` - parse N vertices per call
  - Added `ply_get_progress()`, `ply_is_complete()`, `ply_close()`
  - Added `ply_parse_state_free()` and `ply_parse_state_to_data()` helpers

- `src/ui/ui_scene_hierarchy.h`:
  - Added `import_job` and `import_progress_popup_open` to state struct
  - Added modal progress popup with progress bar, status, and Cancel button
  - Files < 1000 points import synchronously (no progress bar)
  - Files >= 1000 points show progress bar during import

**Key Design Decisions:**
- Modal popup blocks other UI during import
- Threshold of 1000 points (`PLY_SYNC_THRESHOLD`) for sync vs async import
- Progress weighting: Point Cloud mode 95% parsing/5% creation, Editable mode 30% parsing/70% creation
- Cancellation cleans up partial state and deletes incomplete entities

**Test Files:**
- `scripts/test_small.ply` (25 pts) - sync import
- `scripts/test_100pts.ply` (100 pts) - sync import
- `scripts/test_2500pts.ply` (2500 pts) - async with progress bar
- `scripts/test_50k_3d.ply` (50000 pts) - async with progress bar

## Session (2026-02-03)

### PLY Point Cloud Loader (IMPLEMENTED)
Full PLY point cloud import system implemented in this session:

**Phase 1: Core Infrastructure ✅**
- Added `GEOM_POINT_CLOUD` (value 7) to `geometry_type_t` enum in `geometry_comp.h`
- Added `geom_point_cloud_data_t` struct with dynamic points/colors arrays
- Added point cloud helpers: `geom_point_cloud_init()`, `geom_point_cloud_add_point()`, `geom_point_cloud_free()`
- Added `geometry_comp_point_cloud()` helper to create point cloud geometry
- Added point cloud batch functions to `geometry_batch.h`: `geom_point_cloud_batch_alloc()`, `geom_point_cloud_batch_set()`, etc.
- Added `scene_add_point_cloud()` to `ecs_scene.h` with full slot management
- Updated `scene_free_entity_slots()` to handle GEOM_POINT_CLOUD
- Updated `ecs_scene_update()` to handle point cloud visibility and dirty updates
- Updated `ecs_scene_populate_pick_buffer()` with sampled picking (max 1000 points)

**Phase 2: PLY Loader ✅**
- Created `src/ply_loader.h` - header-only ASCII PLY parser
- Parses PLY header to detect format, vertex count, and properties
- Supports vertex positions (x, y, z) - required
- Supports optional colors: red, green, blue, alpha (uint8 or float)
- Computes bounding box during load
- Error handling with `ply_error_t` enum and `ply_error_string()`
- Quick info function `ply_get_info()` to get vertex count without full load

**Phase 3: UI Integration ✅**
- Added "Import PLY Point Cloud..." menu item in File menu
- Created import options popup (`Import PLY Options`) with:
  - File info display (path, point count, has colors)
  - Two import modes: Point Cloud Node (efficient) vs Editable Subtree (individual points)
  - Unit selection: Meters (1:1), Millimeters (0.001), Inches (0.0254)
  - Point size slider (0.001 to 0.1)
  - Color options: use PLY colors or default color picker
- Scale applied via TransformComp.scale for unit conversion
- Editable Subtree mode limited to 10k points to prevent performance issues

**Technical Notes:**
- Point clouds use the existing point batch infrastructure with contiguous slot allocation
- For pick buffer, large point clouds are sampled (max 1000 points) to avoid overwhelming GPU
- Test PLY files available in `scripts/` directory (test_small.ply, test_100pts.ply, test_2500pts.ply, test_50k_3d.ply)

### PLY Import: Removed 10k Limit for Editable Subtree
- Removed artificial 10k point limit for editable subtree imports
- Successfully tested with 50k points (works but freezes UI during import)
- Created design document: `.claude/PLY_LOAD_PROGRESS_BAR_PLAN.md`
- Next session: Implement chunked processing with progress bar

### Bug Fix: Parent-Child Hierarchy (64 children limit)
Fixed a critical bug where entities with more than 64 children were not handled correctly:

**Problem:**
- `scene_remove_entity()`, `ecs_scene_update_transform_recursive()`, and `ecs_world_mark_descendants_dirty()` all had hardcoded `children[64]` arrays
- Entities with >64 children (e.g., point cloud editable subtrees with 100 points) would only process the first 64
- Remaining children would not be transformed, not deleted, and leave orphaned render slots

**Fix:**
- `ecs_scene_update_transform_recursive()`: Now uses `ecs_children()` iterator directly to handle unlimited children
- `scene_remove_entity()`: Now loops with batches of 64 until all children are deleted
- `ecs_world_mark_descendants_dirty()`: Now uses `ecs_children()` iterator directly
- Added `ecs_world_count_children()` and `scene_count_children()` helper functions
- Fixed `undo_redo_exec.h` to dynamically allocate child arrays based on actual count

### Tidy-Up Branch, removal of old geometry examples not using ECS
Removed standalone rendering examples:
- Static cube, thin lines, instanced lines, instanced polylines
- Alpha-blended lines, G-code path viewer
- Related UI panels, shaders, and utilities

Kept only ECS-related rendering system.

### Added Windows/D3D11 Support
- D3D11 pick buffer readback implementation (`pick_readback_d3d11.c`)
- CMake configured for Visual Studio builds
- Sokol linked with D3D11/DXGI libraries on Windows
- **HLSL shader semantics**: All shader descriptions now include `.attrs[]` array with `hlsl_sem_name` and `hlsl_sem_index` for D3D11 validation
- **HLSL reserved keyword fix**: Renamed `point` to `center_pt` in join and pick shaders (`point` is reserved in HLSL)
- **Windows entry point**: Set WIN32_EXECUTABLE for Windows GUI apps (fixes missing `main` linker error)
- **strcasecmp portability**: Defined as `_stricmp` on Windows before first use