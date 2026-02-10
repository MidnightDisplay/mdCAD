# Session Checkpoint - 2026-02-10

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
    ├── instanced_line_shaders.h # Thick line shaders (all backends)
    ├── join_shaders.h           # Circle join shaders
    ├── pick_shaders.h           # GPU picking shaders
    └── spirv/                   # Vulkan SPIR-V shaders
        ├── *.vert, *.frag       # GLSL 450 source files
        ├── *.spv                # Compiled SPIR-V bytecode
        └── spirv_bytecode.h     # Generated C byte arrays

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

## Recent Changes (2026-02-10)

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

## Recent Changes (2026-02-09)

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

## Recent Changes (2026-02-08)

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

## Recent Changes (2026-02-04)

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

## Recent Changes (2026-02-03)

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

### Tidy-Up Branch
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