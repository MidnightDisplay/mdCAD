# Session Checkpoint - 2026-02-11

## Project Overview

A C/C++ cross-platform graphics application using:
- **Sokol** - cross-platform graphics/app library
- **Dear ImGui** (via cimgui) - immediate mode GUI
- **Flecs** - Entity Component System
- **cJSON** - ultralight JSON parser for ANSI C
- **CMake + Ninja/Mingw/MSVC/Gradle** - build system
- **Emscripten** - WebAssembly compilation (experimental, no filesystem support)

## Current Features

1. **ImGui Dockspace** - Flexible window layout
2. **Offscreen Render Target** - 3D content rendered to texture, displayed in ImGui
3. **Orbital Camera System** - Full orbit/pan/zoom controls with inertia
4. **ECS Scene Management** - Entity creation, hierarchy, serialization
5. **GPU Picking** - Mouse hover/click selection of entities
6. **Undo/Redo System** - 100-command stack with Ctrl+Z/Ctrl+Shift+Z
7. **Manipulator Gizmo** - On-screen draggable manipulator - one movement mode: Translation (Rotation & Scale to be added); two target modes: Entity Transform Component and Geometery Component Vertex direct edit.
8. **Scene Serialisation** - JSON save/load.
9. **Non-native Format Loaders** - *.PLY, *.JSONL - implemented. More planned.
10. **Theme System** - Visual Studio Dark, iOS Light, Catppuccin Frappé

## Project Structure

```
src/
├── app.c                   # Main app - Sokol callbacks, orchestrator
├── platform.h              # Backend detection (SOKOL_METAL/WGPU/D3D11/GLCORE)
├── math3d.h                # 3D math (vec3_t, mat4_t, transforms)
├── imgui_storage.h         # ImGui settings persistence
├── render_target.h         # Offscreen render target management
├── orbit_camera.h          # Orbital camera with inertia
├── selection.h             # Entity selection buffer management
├── scene_serializer.h      # JSON scene save/load
├── undo_redo.h             # Undo/redo core data structures
├── undo_redo_exec.h        # Undo/redo command execution logic
├── ecs/
│   ├── ecs_world.h         # ECS world init/shutdown, component registration
│   └── ecs_scene.h         # High-level scene API (entity creation, rendering)
├── gpu/
│   ├── instance_buffer.h   # Dynamic GPU buffer with slot allocation
│   ├── geometry_batch.h    # Batch manager for geometry types (lines, points)
│   ├── pick_buffer.h       # GPU picking system (20x20 offscreen buffer)
│   ├── pick_readback.h     # Cross-platform readback interface
│   ├── pick_readback_metal.m   # Metal CPU readback
│   ├── pick_readback_d3d11.c   # D3D11 CPU readback
│   └── pick_readback_vulkan.c  # Vulkan CPU readback
├── components/
│   ├── component_types.h   # Common types (vec4_t)
│   ├── transform_comp.h    # Transform component (position, rotation, scale)
│   ├── geometry_comp.h     # Geometry component (point, line, polyline, arc, etc.)
│   ├── renderable_comp.h   # Renderable component (visibility, batch, instance)
│   ├── selectable_comp.h   # Selectable component (pick ID for GPU picking)
│   └── label_comp.h        # Stores human readable entity data (name and description)
├── ui/
│   ├── ui_theme.h          # Theme system
│   ├── ui_controls.h       # Controls window
│   ├── ui_viewport.h       # 3D Viewport window
│   ├── ui_camera_debug.h   # Camera Debug window
│   ├── ui_visibility.h     # Visibility controls
│   ├── ui_pick_debug.h     # Pick buffer debug visualization
│   ├── ui_entity_inspector.h   # Entity property inspector
│   ├── ui_scene_hierarchy.h    # Scene hierarchy with entity list
│   ├── ui_about.h              # Help -> About window with license info
│   ├── ui_slot_buffer_debug.h  # Slot buffer debug viewer
│   ├── ui_fps_debug.h          # FPS counter with rolling plot
│   └── ui_file_browser.h       # Cross-platform file browser
├── gizmo/
│   ├── gizmo.h              # Main gizmo state machine + interaction
│   ├── gizmo_rendering.h    # Own GPU pipelines + stream instance buffers
│   └── gizmo_vertex_mode.h  # Geometry mode vertex editing
└── shaders/
    ├── instanced_line_shaders.h     # Thick line shaders (all backends)
    ├── instanced_triangle_shaders.h # Triangle shaders (all backends)
    ├── join_shaders.h               # Circle join shaders
    ├── pick_shaders.h               # GPU picking shaders
    └── spirv/                       # Vulkan SPIR-V shaders
        ├── *.vert, *.frag           # GLSL 450 source files
        ├── *.spv                    # Compiled SPIR-V bytecode
        └── spirv_bytecode.h         # Generated C byte arrays

scripts/
├── gather_licenses.py   # Regenerate THIRD_PARTY_LICENSES.md from vendor LICENSE files
└── vulkan-win/          # Vulkan shader build scripts (Windows)
    ├── compile-spirv.ps1
    ├── generate-bytecode-header.ps1
    └── build-all.ps1

vendors/
├── cjson/              # JSON parser
├── flecs/              # Flecs ECS library
├── libcimgui/          # Dear ImGui C bindings
└── libsokol/           # Sokol cross-platform library

```

## Build Commands

```bash
# Native build (macOS - Metal backend)
cmake -B build -G Ninja && ninja -C build
./build/bin/mdCAD

# Windows build (Vulkan backend - Visual Studio, default)
$env:VULKAN_SDK = "C:\VulkanSDK\1.4.341.1"  # Set if not in environment, change path to exact installed.
cmake -B build-vulkan -G "Visual Studio 18" -DUSE_VULKAN=ON
cmake --build build-vulkan --config Release

# Windows build (D3D11 backend - Visual Studio, optional)
cmake -B build-msvc -G "Visual Studio 18"
cmake --build build-msvc --config Release

# Windows build (Vulkan backend - MinGW)
$env:VULKAN_SDK = "C:\VulkanSDK\1.4.341.1"  # Set if not in environment, change path to exact installed.
cmake -B build-mingw -G "MinGW Makefiles"
cmake --build build-mingw
.\build-mingw\bin\mdCAD.exe

# iOS build (Metal backend)
cmake -B build-ios -G Xcode -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_DEPLOYMENT_TARGET=14.0
open build-ios/mdCAD.xcodeproj

# Android build (GLES3 backend)
cd android && ./gradlew assembleDebug
adb install -r app/build/outputs/apk/debug/app-debug.apk

# Web build (WebGPU backend)
emcmake cmake -B build-web && cmake --build build-web
open build-web/bin/mdCAD.html
```

## ECS Architecture

### Components
- **TransformComp** - position, rotation, scale, world_matrix
- **GeometryComp** - type (point, line, polyline, arc, polygon, bezier, helix), color, dimensions
- **RenderableComp** - visible, layer, batch/instance slot tracking
- **SelectableComp** - pick_id for GPU picking (RGB encoded)
- **LableComp** - human readable entity data (name and description), currently only applies to *.jsonl imports

### Scene API
```c
scene_add_point(scene, pos, color, size);
scene_add_line(scene, a, b, color, width);
scene_add_polyline(scene, points, count, color, width);
scene_add_arc(scene, center, radius, start, end, color, width);
scene_add_polygon(scene, points, count, color, width);
scene_add_bezier(scene, p0, p1, p2, p3, color, width);
scene_add_helix(scene, center, radius, height, turns, color, width);

scene_set_position(scene, entity, pos);
scene_set_parent(scene, child, parent);
scene_remove_entity(scene, entity);
```

### GPU Picking
- 20x20 offscreen buffer centered on cursor
- Entities rendered with pick_id as RGB color
- Platform-specific readback (Metal, D3D11 and Vulkan implemented, OpenGL/WebGPU placeholder)
- Theme-aware hover/selection colors

### Scene Serialization
JSON format with version field, supports all geometry types and parent-child relationships.

### Further Reading
`docs/ECS_RENDERING_PRIMER.md` - Explainer with text and diagrams on how ECS and RENDERING work in tandem to achive high performance rendering with interactive entity targets.

## Platform Support

| Platform        | Backend | Build Status | GPU Picking |
|-----------------|---------|--------------|-------------|
| macOS           | Metal   | Working      | Full        |
| iOS             | Metal   | Working      | Full        |
| Windows (MSVC)  | D3D11   | Working      | Full        |
| Windows (MSVC)  | Vulkan  | Working*     | Full        |
| Windows (MinGW) | Vulkan  | Working      | Full        |
| Web             | WebGPU  | Working      | Placeholder |
| Android         | GLES3   | Working      | Placeholder |
| Linux           | OpenGL  | Ready**      | Placeholder |

*Requires `-DUSE_VULKAN=ON` CMake option. Use `docs/VULKAN_WINDOWS.md` for in-depth system knowledge.
**Ready = CMake configured but not tested on actual hardware

## Key Technical Notes

### Modular Architecture
- Header-only modules with consistent patterns
- Include guards: `#ifndef MODULE_H`
- Types: `snake_case_t` suffix
- Functions: `module_action()` pattern
- All functions use `static inline`

### GPU Instancing
- Template geometry (rectangle + caps) instanced per entity
- Vertex shader computes screen-space offset for constant pixel width
- Aspect ratio correction ensures circular caps

### Contiguous Slot Allocation
Multi-segment geometries (polylines, arcs, etc.) require contiguous instance buffer slots.
Use `instance_buffer_alloc_contiguous(n)` to avoid free-list fragmentation issues.

### Flecs Query Patterns
When iterating with `ecs_query_next()`:
- Only call `ecs_iter_fini()` when breaking early from the loop
- Loop exhaustion auto-finalizes; calling `ecs_iter_fini()` again causes crash

## Most Recent Changes (2026-02-11)

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

## Older Changes

For older changes please refer to `CHANGELOG.md`. Keep the index up to date with archived content.

### `CHANGELOG.md` Index:
- Session (2026-02-10):
  - Licensing, Attribution & Help -> About Window (IMPLEMENTED)
  - Documentation Tidy-Up
- Session (2026-02-09):
  - 3D Manipulator Gizmo (IMPLEMENTED)
  - Vulkan Backend for MinGW (COMPLETED - Phase 1)
  - Vulkan Pick Readback (IMPLEMENTED)
  - Visual Studio 2026 Vulkan Option (COMPLETED - Phase 2)
  - FPS Debug Window (IMPLEMENTED)
- Session (2026-02-08):
  - JSONL Geometry Log Importer (IMPLEMENTED)
- Session (2026-02-04):
  - PLY Import Quality of Life Improvements (IMPLEMENTED)
  - PLY Import Progress Bar (IMPLEMENTED)
- Session (2026-02-03):
  - PLY Point Cloud Loader (IMPLEMENTED)
  - PLY Import: Removed 10k Limit for Editable Subtree
  - Bug Fix: Parent-Child Hierarchy (64 children limit)
  - Tidy-Up Branch, removal of old geometry examples not using ECS
  - Added Windows/D3D11 Support