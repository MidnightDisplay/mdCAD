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

## Most Recent Changes (2026-04-02)

### Phase 14 Script IO + API/Undo Integration Closure (IMPLEMENTED)

- Completed Phase 14 plan `14-03` and closed Phase 14 (3/3 plans complete).
- Delivered dedicated Script IO UX path:
  - `Open Script IO` launch control beside `Open Script Editor` in SketchManager.
  - Dedicated Script IO window lifecycle and rendering in app loop (`src/app.c`).
- Stabilized transactional Script IO behavior from UAT loop:
  - Startup Script IO seed now suppresses undo baseline noise.
  - Live IO slider and numeric edits coalesce to one interaction-level undo transaction.
  - Invalid IO edits preserve last-valid scene/script and surface diagnostics.
  - Restored cross-platform clipboard/undo shortcut behavior (`Ctrl`/`Cmd` path handling).
- Fixed script IO scaling and persistence edge cases:
  - Replaced `scene_script_reemit_for_sketch` fixed 4096 sink with shared script buffer contract (`src/scripting/sketch_script_emit.h`).
  - Preserved labels across script apply rebuild keyed by script identity (`src/scripting/sketch_script_apply.h`).
  - Stabilized large-script live-edit regression fixture to remain within parser model limits while keeping >4096 coverage (`src/tests/script_roundtrip_tests.c`).
- Validation/build evidence (Windows MSVC + Vulkan):
  - `cmake --build build-vulkan --config Release --target script_roundtrip_tests` passed.
  - `ctest --test-dir build-vulkan -C Release -R script_roundtrip_tests --output-on-failure` passed.
  - `ctest --test-dir build-vulkan -C Release --output-on-failure` passed.
  - `cmake --build build-vulkan --config Release --target mdCAD` passed.
- Added phase closure artifact:
  - `.planning/phases/14-script-io-api-undo-integration/14-03-SUMMARY.md`
- Updated planning state for next phase kickoff:
  - `.planning/ROADMAP.md` marks Phase 14 complete.
  - `.planning/STATE.md` advances focus to Phase 15 and updates progress counters.
  - `.planning/phases/14-script-io-api-undo-integration/14-VALIDATION.md` marked complete.

### Phase 16 Constraint UX Closure + Verification Debt Burn-Down (IMPLEMENTED)

- Executed Phase 16 plans `16-01` and `16-02` (gap-closure mode) and completed all tasks.
- Added shared participant-selection helper:
  - `src/constraints/constraint_selection.h` with `constraint_selection_apply_participants(...)`.
- Unified constraint selection behavior:
  - `src/app.c` glyph click now routes through shared participant selection.
  - `src/ui/ui_entity_inspector.h` manager row selection now uses the same helper in both manager sections.
  - Preserved `selected_constraint_entity`, dimensional double-click popup flow, and keybindings (`C` menu, `Tab` gizmo mode).
- Closed Phase 11 evidence debt:
  - Created `.planning/phases/11-constraint-authoring-ux/11-VERIFICATION.md` covering `SKCH-04`, `CONS-01..05`.
  - Upgraded `.planning/phases/11-constraint-authoring-ux/11-VALIDATION.md` to compliant (`status: complete`, `nyquist_compliant: true`, `wave_0_complete: true`).
- Added Phase 16 planning artifacts and validation contract:
  - `16-RESEARCH.md`, approved `16-UI-SPEC.md`, `16-VALIDATION.md`, `16-VERIFICATION.md`, `16-HUMAN-UAT.md`, `16-01/02-SUMMARY.md`.
- Human UAT for Phase 16 passed:
  - Glyph click participant highlight parity — PASS.
  - Dimensional glyph double-click popup + highlight persistence — PASS.
- Phase 16 marked complete in roadmap/requirements traceability.
- Build/test gate re-run:
  - `cmake --build build-vulkan --config Release --target mdCAD` passed.
  - `ctest --test-dir build-vulkan -C Release --output-on-failure` exits 0 in this environment (no registered tests found).

### Archived baseline notes

### Phase 11 UAT Stabilization + Sketch/Constraint Undo Hardening (IMPLEMENTED)

- Closed all remaining Phase 11 focused UAT items on Windows MSVC+Vulkan.
- Fixed dimensional precision/init UX:
  - no padded `1.0000` defaults for whole-number dimensional constraints
  - Enter accepts and Esc/Cancel closes Edit Dimension popup
  - creating Length constraint auto-opens Edit Dimension popup
- Stabilized constraint/geometry undo flows:
  - no duplicate constraint entities after undo in n-ary delete scenarios
  - restored geometry labels on undo
  - `Clear Scene` now supports atomic undo
  - deleting sketch/root nodes restores full subtree with correct hierarchy, participant links, and glyph visibility
- Reworked bulk delete undo restoration to:
  - snapshot recursive subtree entities
  - preserve component identity for recreated entities (sketch/constraint/sketch-state/light/label/renderable)
  - remap parent + constraint participant IDs old->new and rebuild participant backrefs
- Theme default switched to Visual Studio Dark with matching UI preselection.
- Rebuilt repeatedly via `cmake --build build-vulkan --config Release --target mdCAD` and passed final gate.

### Archived baseline notes

### ECS Batch Parenting and other fixes (IMPLEMENTED)

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

  Sprint 4: Scene Serializer

  - scene_serializer.h: Replaced per-entity scene_set_parent() loop with scene_set_parents_batch() for O(n) batched parenting

  Additional fixes

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

## Older Changes

For older changes please refer to `CHANGELOG.md`. Keep the index up to date with archived content.

### `CHANGELOG.md` Index:
- Session (2026-02-12):
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
