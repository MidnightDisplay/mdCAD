# Architecture

**Analysis Date:** 2026-03-24

## Pattern Overview

**Overall:** Single-executable native GUI application with header-only subsystems, ECS-managed scene state, and platform-specific GPU backends

**Key Characteristics:**
- Single app entry point driven by Sokol callbacks in `src/app.c`
- Header-only modules with `static inline` APIs for most engine and UI code
- ECS scene entities own transform, geometry, renderable, selectable, and light data
- Rendering is split into an offscreen 3D viewport pass plus an ImGui UI pass
- Backend selection is compile-time via `src/platform.h` and CMake
- Platform-specific readback and shader paths exist for Metal, D3D11, Vulkan, OpenGL, GLES3, and WebGPU

## Layers

**Application Shell:**
- Purpose: Owns the process lifecycle, window setup, frame orchestration, and global app state
- Contains: `sokol_main()`, `init()`, `frame()`, `cleanup()`, `event()`, and the top-level `state` struct in `src/app.c`
- Depends on: Sokol, ImGui, ECS scene/world, render target, camera, picking, gizmo, UI panels
- Used by: All runtime subsystems

**Scene and ECS Layer:**
- Purpose: Model the editable CAD scene as entities and components
- Contains: `src/ecs/ecs_world.h`, `src/ecs/ecs_scene.h`, components under `src/components/`, selection helpers, scene serialization
- Depends on: Flecs, geometry math, GPU batch managers, pick buffer
- Used by: UI panels, gizmo interaction, importers, serializer, rendering

**Rendering Layer:**
- Purpose: Convert ECS geometry into GPU-friendly batches and draw them
- Contains: `src/gpu/geometry_batch.h`, `src/gpu/instance_buffer.h`, `src/gpu/pick_buffer.h`, `src/render_target.h`, shader headers under `src/shaders/`
- Depends on: Sokol gfx/app glue, geometry components, platform macros, compiled SPIR-V for Vulkan
- Used by: `ecs_scene_update()`, `ecs_scene_draw()`, `ecs_scene_populate_pick_buffer()`, gizmo rendering

**UI Layer:**
- Purpose: Present state and editing controls through Dear ImGui
- Contains: `src/ui/ui_scene_hierarchy.h`, `src/ui/ui_entity_inspector.h`, `src/ui/ui_viewport.h`, `src/ui/ui_visibility.h`, `src/ui/ui_controls.h`, debug panels
- Depends on: `cimgui`, scene and selection state, undo/redo, file browsers, import jobs
- Used by: `frame()` in `src/app.c`

**Import/Serialization Layer:**
- Purpose: Load, save, and transform scene data from external formats
- Contains: `src/scene_serializer.h`, `src/ply_loader.h`, `src/ply_import_job.h`, `src/ply_mesh_import_job.h`, `src/jsonl_loader.h`, `src/jsonl_import_job.h`
- Depends on: ECS world/scene, component factories, file browsers, progress UI
- Used by: Scene hierarchy file actions and import dialogs

**Platform/Build Layer:**
- Purpose: Select backend, package per-platform targets, and provide platform-specific entry glue
- Contains: `src/platform.h`, root `CMakeLists.txt`, `src/CMakeLists.txt`, `vendors/CMakeLists.txt`, `android/app/src/main/cpp/CMakeLists.txt`, `web/shell.html`, `ios/Info.plist`
- Depends on: Compiler/platform defines, Sokol variants, vendored libraries
- Used by: The build system and runtime bootstrap

## Data Flow

**Frame Lifecycle:**

1. The platform entry point calls `sokol_main()` in `src/app.c`.
2. Sokol invokes `init()`, which configures graphics, ImGui, ECS, selection, picking, render targets, debug UIs, undo/redo, gizmo state, and default scene content.
3. Each frame, `frame()` updates ImGui persistence, camera input, UI panels, undo/redo shortcuts, ECS world progress, scene transforms, gizmo state, and active import jobs.
4. The app builds an offscreen viewport matrix, collects lighting data, and calls `ecs_scene_draw()` to render visible ECS geometry into the offscreen target.
5. The app renders gizmo overlays and then performs a dedicated GPU picking pass through `pick_buffer_*` helpers when the viewport is hovered.
6. Hover and click results route back into selection, entity inspector, hierarchy, or gizmo state.
7. The frame ends with the main ImGui pass, `sg_commit()`, and cleanup is handled in `cleanup()` when the app exits.

**State Management:**
- Most state is held in memory inside the top-level `state` struct in `src/app.c`
- ECS world state lives in Flecs and owns persistent scene entities
- UI panels keep cached view state and pointers into ECS or scene state
- Import jobs are incremental state machines with explicit progress and cancel paths
- Persisted user preferences are handled through ImGui settings storage

## Key Abstractions

**Entity + Component Scene Model:**
- Purpose: Represent editable scene objects as small, composable pieces of state
- Examples: `TransformComp`, `GeometryComp`, `RenderableComp`, `SelectableComp`, `LabelComp`, `LightComp`
- Pattern: ECS composition with tag components for selection/hover/import state

**Geometry Batch:**
- Purpose: Convert geometry components into reusable GPU instance buffers and pipelines
- Examples: `geom_line_batch_t`, `geom_point_batch_t`, `geom_triangle_batch_t`, `geometry_batch_manager_t`
- Pattern: Batched instanced rendering with contiguous slot allocation for multi-segment shapes

**Scene API Wrapper:**
- Purpose: Provide higher-level creation and manipulation helpers on top of raw ECS calls
- Examples: `scene_add_point()`, `scene_add_line()`, `scene_add_triangle()`, `scene_add_mesh_box()`, `scene_set_parent()`
- Pattern: Thin facade over ECS world operations plus GPU batch bookkeeping

**Import Job State Machine:**
- Purpose: Turn large imports into cancellable, progress-reporting work units
- Examples: `ply_import_job_t`, `ply_mesh_import_job_t`, `jsonl_import_job_t`
- Pattern: Incremental parsing/creation phases with UI-driven progress polling

**UI Panel Modules:**
- Purpose: Encapsulate a single ImGui window or tool panel behind init/draw/shutdown functions
- Examples: `ui_viewport_*`, `ui_scene_hierarchy_*`, `ui_entity_inspector_*`, `ui_visibility_*`
- Pattern: Header-only module with state struct and lifecycle helpers

## Entry Points

**Native App Entry:**
- Location: `src/app.c`
- Triggers: Sokol calls `sokol_main()`, then `init()`, `frame()`, `cleanup()`, and `event()`
- Responsibilities: Set up runtime state, drive one frame of UI and rendering, handle app suspend/resume, and shut everything down cleanly

**Platform Build Entrypoints:**
- Location: `CMakeLists.txt`, `src/CMakeLists.txt`, `android/app/src/main/cpp/CMakeLists.txt`
- Triggers: CMake or Gradle build configuration
- Responsibilities: Select backend, choose platform-specific sources, and produce the correct executable or shared library

## Error Handling

**Strategy:** Prefer explicit return values, early exits, and cleanup-on-shutdown rather than exception-based control flow

**Patterns:**
- Importers and serializers return status flags, `NULL`, or partially filled state on failure
- UI actions guard on current selection, hover, and import-job state before mutating scene data
- Shutdown paths free owned buffers and dispose of GPU/ECS resources in reverse creation order
- Logging is routed through Sokol's logger hooks and surfaced in the console

## Cross-Cutting Concerns

**Logging:**
- Sokol logger callbacks are wired in `src/app.c`
- Debug and test output also appears through ImGui panels and platform console output

**Validation:**
- ImGui panels validate user interactions at the UI boundary before invoking scene mutations
- Importers validate file contents and expose progress before committing entities

**Backend Selection:**
- `src/platform.h` maps the host to `SOKOL_METAL`, `SOKOL_D3D11`, `SOKOL_VULKAN`, `SOKOL_GLES3`, `SOKOL_GLCORE`, or `SOKOL_WGPU`
- Vulkan uses separate SPIR-V assets under `src/shaders/spirv/`

**Persistence:**
- Scene save/load goes through `src/scene_serializer.h`
- ImGui settings persistence is handled by `src/imgui_storage.h`

**Current Local Workflow:**
- On this machine the active working build is macOS + Ninja in `build/`, driven by `cmake -B build -G Ninja && ninja -C build`

---

*Architecture analysis: 2026-03-24*
*Update when major patterns change*
