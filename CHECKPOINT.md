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

# Windows build (D3D11 backend - Visual Studio, default)
cmake -B build-msvc -G "Visual Studio 18"
cmake --build build-msvc --config Release

# Windows build (Vulkan backend - Visual Studio, optional)
$env:VULKAN_SDK = "C:\VulkanSDK\1.4.341.1"  # Set if not in environment, change path to exact installed.
cmake -B build-vulkan -G "Visual Studio 18" -DUSE_VULKAN=ON
cmake --build build-vulkan --config Release

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

### Mesh Triangles - Sprint 2: Picking, Selection, Undo/Redo & Serialization (IMPLEMENTED)

Added full interactivity to triangle entities: GPU picking for click-to-select, undo/redo for create/delete/vertex editing, and JSON scene serialization. Plan file: `.plans/PLAN_MESH_TRIANGLES.md`

**New Files:**
- `src/shaders/spirv/pick_triangle.vert` - GLSL 450 triangle pick vertex shader
- `src/shaders/spirv/pick_triangle.frag` - GLSL 450 triangle pick fragment shader
- `src/shaders/spirv/pick_triangle_vs.spv` - Compiled SPIR-V vertex bytecode (1880 bytes)
- `src/shaders/spirv/pick_triangle_fs.spv` - Compiled SPIR-V fragment bytecode (576 bytes)

**Modified Files:**
- `src/shaders/pick_shaders.h` - Added `pick_triangle_vs_source` / `pick_triangle_fs_source` for all backends (GLCORE, GLES3, Metal, WGPU, D3D11, Vulkan)
- `src/gpu/pick_buffer.h` - Added `pick_triangle_instance_t` (12 floats), `pick_triangle_params_t` (MVP-only), triangle pipeline/shader/template/instances, `pick_buffer_add_triangle()`, triangle draw in render pass (before lines/points)
- `src/ecs/ecs_scene.h` - Added `GEOM_TRIANGLE` case in `ecs_scene_populate_pick_buffer()` — transforms vertices through world_matrix and calls `pick_buffer_add_triangle()`
- `src/undo_redo.h` - Added `UNDO_GEOM_TRIANGLE` and `UNDO_GEOM_POINT_CLOUD` to `undo_geom_type_t`, added `triangle` member to snapshot union
- `src/undo_redo_exec.h` - Added GEOM_TRIANGLE snapshot/recreate, updated `undo_set_vertex_pos()` for triangle vertices
- `src/scene_serializer.h` - Added triangle serialization/deserialization (`"type": "triangle", "a": [...], "b": [...], "c": [...]`), updated type name tables, added triangle member to `loaded_entity_t`
- `src/shaders/spirv/spirv_bytecode.h` - Regenerated with pick_triangle shader bytecode

**Key Design Decisions:**
- **Pick triangle shader uses MVP-only uniform** (`pick_triangle_params_t`): No line_width/aspect_ratio needed since triangles are solid geometry
- **5 vertex attributes for pick**: template_pos + vertex_a/b/c + pick_color (vs 8 for visual shader)
- **Draw order in pick pass**: Triangles first, then lines, then points — matches visual draw order
- **No index buffer for pick triangles**: 3 vertices drawn directly via `sg_draw(0, 3, instance_count)`
- **Overlay triangle picking deferred**: Not needed until gizmo-level triangle picking is required

### Mesh Triangles - Sprint 3: Gizmo Vertex Editing (IMPLEMENTED)

Added triangle vertex editing via the geometry mode gizmo. Select a triangle, press Tab to enter geometry mode, and drag individual vertices with the gizmo. Undo/redo works automatically via the existing `CMD_SET_GEOMETRY_VERTICES` command. Plan file: `.plans/PLAN_MESH_TRIANGLES.md`

**Modified Files:**
- `src/gizmo/gizmo_vertex_mode.h` - Added `GEOM_TRIANGLE` cases to `gizmo_vertex_mode_get_vertex_count()` (returns 3), `gizmo_vertex_mode_get_local_pos()` (returns triangle.a/b/c by index), and `gizmo_vertex_mode_set_local_pos()` (sets triangle.a/b/c by index)

**Key Design Decisions:**
- **No undo/redo changes needed**: `undo_set_vertex_pos()` already handles `GEOM_TRIANGLE` (added in Sprint 2), and the gizmo records vertex edits generically via `CMD_SET_GEOMETRY_VERTICES`
- **Normal recomputation is automatic**: `ecs_scene_update()` recomputes the face normal from world-space vertices whenever `instance_dirty` is set

**Remaining Sprints (see plan):**
- Sprint 4: Per-vertex color & shader variants
- Sprint 5: Lighting system (3-point studio)
- Sprint 6: Mesh entities (multi-triangle indexed)

### Mesh Triangles - Sprint 2: Picking, Selection, Undo/Redo & Serialization (IMPLEMENTED)

Added instanced triangle rendering as a new geometry primitive alongside existing points and lines. Triangles are full ECS entities rendered via GPU instancing using the same architectural pattern as lines and points. Plan file: `.plans/PLAN_MESH_TRIANGLES.md`

**New Files:**
- `src/shaders/instanced_triangle_shaders.h` - Multi-backend triangle shaders (GLCORE, GLES3, Metal, WGPU, D3D11, Vulkan)
- `src/shaders/spirv/instanced_triangle.vert` - GLSL 450 triangle vertex shader
- `src/shaders/spirv/instanced_triangle.frag` - GLSL 450 triangle fragment shader
- `src/shaders/spirv/instanced_triangle_vs.spv` - Compiled SPIR-V vertex bytecode
- `src/shaders/spirv/instanced_triangle_fs.spv` - Compiled SPIR-V fragment bytecode

**Modified Files:**
- `src/components/geometry_comp.h` - Added `GEOM_TRIANGLE` enum, `geom_triangle_data_t` struct, `geometry_comp_triangle()` factory, updated type name table
- `src/gpu/geometry_batch.h` - Added `geom_triangle_instance_t` (96 bytes), `geom_triangle_batch_t`, full batch API (init/alloc/set/free/upload/draw/shutdown), `geom_triangle_compute_normal()`, wired into batch manager
- `src/ecs/ecs_scene.h` - Added `scene_add_triangle()`, GEOM_TRIANGLE cases in `ecs_scene_update()` (visibility hiding + dirty transform), `scene_free_entity_slots()`, `ecs_scene_triangle_count()`
- `src/app.c` - 3 sample triangles at startup (red, green, blue)
- `src/shaders/spirv/spirv_bytecode.h` - Regenerated with triangle shader bytecode

**Key Design Decisions:**
- **Barycentric selector template**: 3 vertices at (1,0,0), (0,1,0), (0,0,1) — vertex shader mixes instance vertex positions: `world_pos = sel.x * A + sel.y * B + sel.z * C`
- **96-byte instances**: 3 vertex positions (9 floats) + face normal (3 floats) + 3 per-vertex colors (12 floats). All 3 colors set identical for uniform-color mode — avoids shader branching
- **8 vertex attributes**: template_pos + vertex_a/b/c + normal + color_a/b/c (within SG_MAX_VERTEX_ATTRIBUTES = 16)
- **Draw order**: Triangles drawn first, then lines, then points — correct depth layering
- **MVP-only uniform block** (`geom_triangle_params_t`): No line_width/aspect_ratio needed — triangles are solid geometry, not screen-space shapes
- **Double-sided**: Cull mode NONE, consistent with lines/points
- **Normal stored per instance**: Computed as `normalize(cross(B-A, C-A))` — ready for future lighting (Sprint 5)

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