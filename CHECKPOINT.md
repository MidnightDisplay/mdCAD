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
9. **Non-native Format Loaders** - *.PLY (point clouds, meshes), *.JSONL (geometry logs with primitives and meshes) - implemented. More planned.
10. **Theme System** - Visual Studio Dark, iOS Light, Catppuccin Frappé

## Project Structure

The project file tree has grown extremely big. 
To get up to date tree structure use `tree` command with relevant flags. 
Available via both, bash and powershell.

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
- **LabelComp** - human readable entity data (name and description), currently only applies to *.jsonl imports
- **LightComp** - represents a light illuminating the scene, affecting materials that support lighting model

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

## Most Recent Changes (2026-02-13)

### Pick Buffer Octree Spatial Index (IMPLEMENTED)

Replaced O(N) entity iteration in `ecs_scene_populate_pick_buffer()` with an O(log N + K) loose octree frustum query, where K is the number of entities whose world-space AABBs overlap the pick frustum (~10-100 entities instead of all N). For 1M point entities with a moving cursor, this reduces pick buffer population from ~3ms (1M matrix-vector multiplies) to ~0.01ms (traverse ~30 nodes, test ~100 entities).

**Architecture:**
- **Loose octree:** Each node's effective bounds are 2x the tight bounds, guaranteeing every entity maps to exactly ONE node (no multi-node insertions). Array-based node pool with linked-list entries per node.
- **Gribb-Hartmann frustum extraction:** 6 clip planes extracted from the pick MVP matrix, normalized for correct distance.
- **AABB-frustum test:** Positive vertex method — for each of 6 planes, compute the AABB corner most aligned with plane normal; if signed distance < 0, AABB is entirely outside.
- **Stack-based query:** Iterative traversal pushes children onto a fixed-size stack, tests each node's loose AABB against frustum, and collects matching entity IDs into a reusable result buffer.
- **Per-type AABB computation:** Exact bounds for point/line/triangle/polyline/polygon/mesh; conservative sphere bounds for arc; convex hull bounds for bezier; axis ± radius for helix.

**Integration Points:**
- `ecs_scene_t` gains `pick_octree_t pick_octree` field, initialized with root bounds ±100km
- All 12 `scene_add_*` functions insert entities into octree via `scene_octree_insert_entity()` helper
- `scene_remove_entity()` removes from octree before ECS deletion (pick_id still valid)
- `ecs_scene_update_transform_recursive()` updates octree AABB when world_matrix changes (only fires for dirty entities)
- `ecs_scene_populate_pick_buffer()` rewritten: extracts frustum → queries octree → processes only matching entities with per-type geometry submission (no NDC projection culling needed)
- Removed unused NDC culling helpers (`pick_ndc_aabb_overlaps`, `pick_ndc_aabb_expand`)

**New File:**
- `src/gpu/pick_octree.h` - Complete loose octree implementation: types (`pick_aabb_t`, `pick_octree_entry_t`, `pick_octree_node_t`, `pick_frustum_t`, `pick_octree_t`); pool management with free lists; `pick_id → entry` O(1) lookup map; AABB computation for all 10 geometry types; insert/remove/move/split; Gribb-Hartmann frustum extraction; AABB-frustum test; stack-based frustum query

**Modified File:**
- `src/ecs/ecs_scene.h` - Added `#include "../gpu/pick_octree.h"`; added `pick_octree_t` to `ecs_scene_t`; init/shutdown hooks; `scene_octree_insert_entity()` helper called from 12 `scene_add_*` functions; remove hook in `scene_remove_entity()`; move hook in `ecs_scene_update_transform_recursive()`; rewrote `ecs_scene_populate_pick_buffer()` to use octree query

**Plan File:** `.plans/PLAN_PICK_BUFFER_OCTREE.md`

## Older Changes

For older changes please refer to `CHANGELOG.md`. Keep the index up to date with archived content.

### `CHANGELOG.md` Index:
- Session (2026-02-12):
  - ECS Batch Parenting and other fixes (IMPLEMENTED)
  - JSONL Mesh Support - MeshBody Import (IMPLEMENTED)
  - Bug Fix: Light Entity Jumping in Scene Hierarchy (FIXED)
  - Enhancement: Triangles Tab in Slot Buffer Debug Window (IMPLEMENTED)
- Session (2026-02-11):
  - Mesh Features - Sprint 6: OBJ to PLY Conversion Script (IMPLEMENTED)
  - Mesh Features - Sprint 5: PLY Mesh Import Job & UI (IMPLEMENTED)
  - Mesh Features - Sprint 4: PLY Mesh Parser - Binary (IMPLEMENTED)
  - Mesh Features - Sprint 3: PLY Mesh Parser - ASCII (IMPLEMENTED)
  - Mesh Features - Sprint 2: Light Entities in Hierarchy & Inspector (IMPLEMENTED)
  - Mesh Features - Sprint 1: Add Entity Menu Items (IMPLEMENTED)
  - Mesh Triangles - Sprint 6: Mesh Entities / Multi-Triangle Indexed (IMPLEMENTED)
  - Mesh Triangles - Sprint 5: Lighting System (IMPLEMENTED)
  - Mesh Triangles - Sprints 1-4 (IMPLEMENTED)
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