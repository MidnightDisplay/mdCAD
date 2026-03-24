# Coding Agent Instructions (ClaudeCode, Github Copilot, Cursors, OpenCode, etc...)

## Session Continuity

**On fresh sessions:** Always read `CHECKPOINT.md` first to understand the current project state and pick up where the previous session left off.

**Project blurb** README.md is the introduction to our project - must be kept up to date with ongoing development.

## New feature planning and implementation

**After running plan mode** Always save the plan file as `PLAN_*.md` to `.plans/` folder. 

The **user <-> agent** collaboration worklow is: 
1) the user specifies the new feature via propmpting the agent
2) the agent activates Plan Mode and works on planning out the new feature implementation
3) the resultant plan is saved to a file
4) the user can review and make amendments to the plan
5) the user starts a new agent session and provides the path to the plan file and any docs useful for understanding the full context
6) the agent runs implementation
7) first pass is tested by the user
8) after the user signs off on the feature, the agent marks off any todo's in the plan as completed and adds notes to `CHECKPOINT.md` as to what was done in the implementation session. Keep the "Most Recent Changes" section tidy, hold only one feature plan (with all sprints). When the feature is delivered archive it to `CHANGELOG.md`

## Project Overview

C/C++ cross-platform graphics application using:
- **Sokol** - cross-platform graphics/app library
- **Dear ImGui** (via cimgui) - immediate mode GUI
- **Flecs** - Entity Component System
- **cJSON** - ultralight JSON parser for ANSI C
- **CMake + Ninja/Mingw/MSVC/Gradle** - build system
- **Emscripten** - WebAssembly compilation (experimental, no filesystem support)

## Quick Commands

```bash
# Native build (macOS - Metal backend)
cmake -B build -G Ninja && ninja -C build
./build/bin/skl_tmp

# MSVC with Vulkan backend - requires Vulkan SDK installed (default on windows)
cmake -B build-vulkan -G "Visual Studio 18" -DUSE_VULKAN=ON
cmake --build build-vulkan --config Release
.\build-vulkan\bin\Release\mdCAD.exe

# Windows MinGW build (OpenGL backend)
cmake -B build-mingw -G "MinGW Makefiles"; cmake --build build-mingw

# Windows build (D3D11 backend) (optional, but deprecated)
cmake -B build-msvc -G "Visual Studio 18"
# Or with Ninja: cmake -B build -G Ninja && ninja -C build (untested)

# iOS build (iPad/iPhone - Metal backend)
cmake -B build-ios -G Xcode -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_DEPLOYMENT_TARGET=14.0
# Then open in Xcode:
open build-ios/skl_tmp.xcodeproj
# Or build from command line for simulator:
xcodebuild -project build-ios/skl_tmp.xcodeproj -scheme skl_tmp -sdk iphonesimulator
# Or for device (requires code signing):
xcodebuild -project build-ios/skl_tmp.xcodeproj -scheme skl_tmp -sdk iphoneos

# Android build (GLES3 backend, requires Android SDK/NDK)
cd android && ./gradlew assembleDebug
adb install -r app/build/outputs/apk/debug/app-debug.apk
# Debug logs (for investigating known issues - see CHECKPOINT.md):
# adb logcat -s imgui_storage:* alpha_polylines:*

# Web build (WebGPU backend)
emcmake cmake -B build-web && cmake --build build-web
open build-web/bin/skl_tmp.html

# Puppeteer tests (emscripten build only, experimental)
cd scripts && npm install  # First time only
node scripts/debug-wasm.mjs --screenshot
node scripts/test-imgui.mjs --visible
```

## Key Files

### Application
- `src/app.c` - Main application (Sokol callbacks, ImGui UI)
- `src/math3d.h` - 3D math library (vec3, mat4, transforms)
- `src/ecs/ecs_scene.h` - ECS scene API (entity creation, rendering)
- `src/gpu/geometry_batch.h` - GPU instanced rendering for lines/points

### Infrastructure
- `vendors/libsokol/sokol.c` - Sokol implementation (defines SOKOL_METAL, SOKOL_WGPU, SOKOL_D3D11, SOKOL_VULKAN etc.)
- `vendors/libsokol/CMakeLists.txt` - Sokol library CMake
- `web/shell.html` - Emscripten HTML template
- `scripts/test-imgui.mjs` - Puppeteer ImGui interaction tests

## Module Guide

### math3d.h
Header-only math library. Types: `vec3_t`, `mat4_t`. Functions: `mat4_identity`, `mat4_mul`, `mat4_perspective`, `mat4_lookat`, `mat4_rotate_x/y/z`, `mat4_scale`, `mat4_translate`.

### ecs/ecs_scene.h
High-level scene API with entity creation, parent-child relationships, and GPU rendering via geometry batches.

### shaders/
Multi-backend shaders using `#ifdef SOKOL_*`. Provides `*_vs_source` and `*_fs_source` strings. Entry points: `vs_main`, `fs_main`.

## Common Gotchas
Avoid using `ImGuiInputTextFlags_EnterReturnsTrue` with any `InputScalar`-based ImGui functions. The flag is only valid for `InputText` and `InputTextMultiline`.

<!-- GSD:project-start source:PROJECT.md -->
## Project

**mdCAD**

mdCAD is a cross-platform CAD viewer and geometry editor built in C on top of Sokol, Dear ImGui, and Flecs. It already supports interactive scene editing, GPU-accelerated rendering, import/export, and multiple native and web targets; the current work is to modernize its math foundation by replacing the local `src/math3d.h` with a mature MIT-licensed C library that is safer to maintain and better optimized on native platforms.

**Core Value:** Interactive geometry editing and rendering must remain stable, responsive, and trustworthy on supported native platforms while the math foundation evolves underneath it.

### Constraints

- **License**: MIT-licensed C library only — reduces legal and maintenance friction and matches the project's current dependency posture
- **Language**: No C++ dependency — the codebase is intentionally C-first and must remain easy to build across the supported native targets
- **Platform Priority**: macOS Metal and Windows Vulkan must stay stable during migration — these are the native build paths currently considered reliable enough to gate changes
- **Performance**: Native performance must not regress and should improve where possible — the migration is partly justified by the chance to gain SIMD and better low-level implementations
- **Rollout Strategy**: Migration must be staged — the blast radius across camera, ECS, picking, gizmo, rendering, importers, and serializer code is too large for a single cutover
- **Build Simplicity**: Integration must fit the existing lightweight CMake workflow without materially increasing build times — mdCAD currently relies on a minimal build setup and should keep that advantage
<!-- GSD:project-end -->

<!-- GSD:stack-start source:codebase/STACK.md -->
## Technology Stack

## Languages
- C11/C17 - Application code in `src/`, vendored C libraries, and most platform glue
- C++17 - `cimgui` integration and Android/desktop build support files
- Objective-C - Apple platform glue and `vendors/libsokol/sokol.c`
- JavaScript (CommonJS) - Puppeteer tooling in `scripts/`
- Kotlin DSL - Android Gradle configuration in `android/`
- HTML/CSS - Emscripten shell template in `web/shell.html`
- PowerShell - Vulkan shader build scripts in `scripts/vulkan-win/`
## Runtime
- Native desktop app runtime via Sokol; no separate backend process
- Browser runtime via Emscripten/WebAssembly with WebGPU (`emdawnwebgpu`), not WebGL
- Android NativeActivity app targeting API 30+
- iOS app targeting iPhone/iPad with deployment target 14.0
- Node.js 18+ for the Puppeteer scripts in `scripts/`
- npm - lockfile v3 in `scripts/package-lock.json`
- Lockfile: `scripts/package-lock.json` present
## Frameworks
- Sokol - cross-platform app and GPU abstraction layer
- Dear ImGui via cimgui - immediate-mode UI
- Flecs - ECS scene/component runtime
- cJSON - JSON parsing and scene serialization
- Puppeteer 24.35.0 - browser automation for WASM/UI checks
- CMake 3.0..3.25 - top-level build system
- Ninja - default native build generator on macOS (`build`)
- Gradle 8.7 with Android Gradle Plugin 8.5.0 - Android build pipeline
- Emscripten - web build toolchain
- Xcode / `xcodebuild` - Apple platform builds
## Key Dependencies
- Sokol - graphics, app loop, and platform abstraction
- cimgui 1.92.5dock - Dear ImGui C bindings and UI implementation
- Flecs - ECS world, entity/component storage, and queries
- cJSON - lightweight JSON parser/writer used for import/export
- Puppeteer 24.35.0 - automated browser checks for the web build
- Vulkan SDK - required for Windows Vulkan builds and SPIR-V shader workflows
- Android SDK/NDK 27.0.12077973 - Android native build and packaging
- Emscripten `emdawnwebgpu` port - browser GPU backend
- Apple SDKs/Xcode - Metal-backed macOS and iOS builds
## Configuration
- Build selection is controlled by CMake generator/flags (`build`, `build-vulkan`, `build-mingw`, `build-ios`, `build-web`)
- `VULKAN_SDK` is required for Windows Vulkan builds
- Android builds rely on the local Android SDK/NDK and Gradle wrapper
- No app-level runtime environment variables are required for normal execution
- `CMakeLists.txt`, `src/CMakeLists.txt`, and `vendors/*/CMakeLists.txt`
- `android/build.gradle.kts`, `android/app/build.gradle.kts`, and `android/gradle/wrapper/gradle-wrapper.properties`
- `scripts/package.json`, `scripts/package-lock.json`, and `web/shell.html`
## Platform Requirements
- macOS Apple Silicon with Ninja is a supported native path today; the default native build directory is `build`
- Windows supports MSVC or MinGW; Vulkan SDK is required for the Vulkan path
- Linux is supported via the OpenGL backend and X11/GL toolchain
- Android, iOS, and web builds require their respective SDKs/toolchains
- Desktop binaries for macOS, Windows, and Linux
- Mobile binaries for iOS and Android
- Static web bundle generated by Emscripten; no backend service
<!-- GSD:stack-end -->

<!-- GSD:conventions-start source:CONVENTIONS.md -->
## Conventions

## Naming Patterns
- `snake_case` for source and headers, with `src/` modules typically using short feature names like `src/app.c`, `src/math3d.h`, `src/ui/ui_scene_hierarchy.h`.
- Header-only modules are the norm; most reusable code lives in `.h` files with include guards, while `src/app.c` remains the main translation unit.
- Script/tooling files keep native extensions and platform conventions: `.py`, `.mjs`, `.ps1`, `.sh`, plus platform-specific `.m` for Objective-C (`src/gpu/pick_readback_metal.m`).
- `snake_case` for all C/C++-side functions, usually prefixed by the owning module, such as `ecs_world_init()`, `scene_add_point()`, `ui_scene_hierarchy_draw()`, and `mat4_mul()`.
- `static inline` is the dominant function pattern in headers; public helpers are exposed directly from the header rather than through separate `.c` implementations.
- Event-style helpers also stay `snake_case` rather than using `handle*` or `on*` prefixes.
- `snake_case` for local variables, parameters, struct fields, and state objects.
- Simple short loop indices such as `i`, `j`, and `k` are common in tight loops.
- No underscore prefix convention for private members, because the codebase is C-first and mostly struct-based.
- `PascalCase` for typedef names that model concepts, such as `TransformComp`, `GeometryComp`, `LightComp`, and `undo_redo_t`.
- Enums use `snake_case` type names with `UPPER_SNAKE_CASE` values in many places, for example `geometry_type_t` and `GEOM_TRIANGLE`.
- Structs and enums are typically paired with factory/default helpers like `transform_comp_default()` and `light_comp_point()`.
## Code Style
- No formatter config is checked in; the code follows a consistent hand-written style.
- Four-space indentation is the prevailing style across C, CMake, JavaScript, Python, and shell snippets.
- Braces usually stay on the same line for functions and control blocks, with section separators like `//------------------------------------------------------------------------------`.
- Semicolons are used where the language requires them; Python scripts follow standard Python syntax without extra tooling.
- No repo-wide lint configuration is present for C/C++ or Node scripts.
- `scripts/package.json` does not define a lint target; `npm test` is a placeholder that exits with an error.
- Style validation is therefore mostly review-based and compile-based rather than tool-enforced.
## Import Organization
- Include groups are separated by blank lines and often annotated with comment banners.
- Within modules, related declarations are grouped into labeled sections such as `Types`, `Initialization`, `Error codes`, `Geometry creation helpers`, and `Scene Save`.
- No include path aliasing convention is used in source.
- Module-relative includes and explicit `../` paths are preferred over virtual aliases.
## Error Handling
- Functions usually return `bool`, enum error codes, `0`, `NULL`, or `0`-equivalent values for failure instead of throwing.
- Parsing code favors early returns and explicit status enums, such as `ply_error_t` in `src/ply_loader.h` and `jsonl_error_t` in `src/jsonl_loader.h`.
- Memory ownership is manual and paired with explicit `*_init()`, `*_free()`, and `*_shutdown()` helpers.
- Invalid input and file problems are surfaced through error enums and human-readable string helpers, not exceptions.
- Invariants are usually guarded with conservative fallbacks, such as returning identity matrices or empty results when inputs are unusable.
- Runtime failure reporting in the app leans on Sokol/cimgui logger hooks and UI status strings rather than a central exception/logging framework.
## Logging
- The native app passes `slog_func` to Sokol and cimgui setup in `src/app.c`.
- Tooling scripts use standard console output (`print`, `console.log`) for progress and diagnostics.
- Logging is mostly diagnostic and boundary-level, not a pervasive application service.
- Scripts print command progress and artifact paths, while the app relies on status text in the UI for user-facing feedback.
- There is no dedicated structured logging library in the repo.
## Comments
- Comments are used heavily for subsystem banners, ownership, and reasoning around edge cases.
- The codebase documents why a workaround exists, especially for platform quirks, GPU picking, binary parsing, and undo/redo behavior.
- Obvious line-by-line comments are avoided when the code is self-explanatory.
- JavaScript and Python scripts use docstrings for module purpose and CLI usage, but there is no formal JSDoc/TSDoc requirement.
- C headers use block comments for module summaries and function intent instead of API docs.
- TODO-style notes are not standardized beyond occasional inline comments.
- Long-lived work is tracked in planning docs such as `.plans/` and `CHECKPOINT.md` instead of relying on TODO comments.
## Function Design
- Small, composable helpers are preferred, especially in header-only modules.
- Larger features are broken into focused files like `src/ply_import_job.h`, `src/ply_mesh_import_job.h`, and `src/jsonl_import_job.h`.
- Parameter lists are explicit and usually stay positional; related values are often passed as separate scalars or small structs.
- Output parameters are used where a function needs to return multiple values, especially in math and parsing helpers.
- Functions return early on invalid state, empty input, or allocation failure.
- Many helpers return simple status values rather than adopting a `Result<T, E>`-style abstraction.
## Module Design
- The codebase is intentionally C-style and does not use namespaces.
- Public module entry points are prefixed by their domain, such as `ui_*`, `ecs_*`, `scene_*`, `geometry_*`, `ply_*`, and `jsonl_*`.
- Barrel files are not part of the style.
- Modules are included directly from the owning header, and the top-level app wires subsystems together explicitly.
<!-- GSD:conventions-end -->

<!-- GSD:architecture-start source:ARCHITECTURE.md -->
## Architecture

## Pattern Overview
- Single app entry point driven by Sokol callbacks in `src/app.c`
- Header-only modules with `static inline` APIs for most engine and UI code
- ECS scene entities own transform, geometry, renderable, selectable, and light data
- Rendering is split into an offscreen 3D viewport pass plus an ImGui UI pass
- Backend selection is compile-time via `src/platform.h` and CMake
- Platform-specific readback and shader paths exist for Metal, D3D11, Vulkan, OpenGL, GLES3, and WebGPU
## Layers
- Purpose: Owns the process lifecycle, window setup, frame orchestration, and global app state
- Contains: `sokol_main()`, `init()`, `frame()`, `cleanup()`, `event()`, and the top-level `state` struct in `src/app.c`
- Depends on: Sokol, ImGui, ECS scene/world, render target, camera, picking, gizmo, UI panels
- Used by: All runtime subsystems
- Purpose: Model the editable CAD scene as entities and components
- Contains: `src/ecs/ecs_world.h`, `src/ecs/ecs_scene.h`, components under `src/components/`, selection helpers, scene serialization
- Depends on: Flecs, geometry math, GPU batch managers, pick buffer
- Used by: UI panels, gizmo interaction, importers, serializer, rendering
- Purpose: Convert ECS geometry into GPU-friendly batches and draw them
- Contains: `src/gpu/geometry_batch.h`, `src/gpu/instance_buffer.h`, `src/gpu/pick_buffer.h`, `src/render_target.h`, shader headers under `src/shaders/`
- Depends on: Sokol gfx/app glue, geometry components, platform macros, compiled SPIR-V for Vulkan
- Used by: `ecs_scene_update()`, `ecs_scene_draw()`, `ecs_scene_populate_pick_buffer()`, gizmo rendering
- Purpose: Present state and editing controls through Dear ImGui
- Contains: `src/ui/ui_scene_hierarchy.h`, `src/ui/ui_entity_inspector.h`, `src/ui/ui_viewport.h`, `src/ui/ui_visibility.h`, `src/ui/ui_controls.h`, debug panels
- Depends on: `cimgui`, scene and selection state, undo/redo, file browsers, import jobs
- Used by: `frame()` in `src/app.c`
- Purpose: Load, save, and transform scene data from external formats
- Contains: `src/scene_serializer.h`, `src/ply_loader.h`, `src/ply_import_job.h`, `src/ply_mesh_import_job.h`, `src/jsonl_loader.h`, `src/jsonl_import_job.h`
- Depends on: ECS world/scene, component factories, file browsers, progress UI
- Used by: Scene hierarchy file actions and import dialogs
- Purpose: Select backend, package per-platform targets, and provide platform-specific entry glue
- Contains: `src/platform.h`, root `CMakeLists.txt`, `src/CMakeLists.txt`, `vendors/CMakeLists.txt`, `android/app/src/main/cpp/CMakeLists.txt`, `web/shell.html`, `ios/Info.plist`
- Depends on: Compiler/platform defines, Sokol variants, vendored libraries
- Used by: The build system and runtime bootstrap
## Data Flow
- Most state is held in memory inside the top-level `state` struct in `src/app.c`
- ECS world state lives in Flecs and owns persistent scene entities
- UI panels keep cached view state and pointers into ECS or scene state
- Import jobs are incremental state machines with explicit progress and cancel paths
- Persisted user preferences are handled through ImGui settings storage
## Key Abstractions
- Purpose: Represent editable scene objects as small, composable pieces of state
- Examples: `TransformComp`, `GeometryComp`, `RenderableComp`, `SelectableComp`, `LabelComp`, `LightComp`
- Pattern: ECS composition with tag components for selection/hover/import state
- Purpose: Convert geometry components into reusable GPU instance buffers and pipelines
- Examples: `geom_line_batch_t`, `geom_point_batch_t`, `geom_triangle_batch_t`, `geometry_batch_manager_t`
- Pattern: Batched instanced rendering with contiguous slot allocation for multi-segment shapes
- Purpose: Provide higher-level creation and manipulation helpers on top of raw ECS calls
- Examples: `scene_add_point()`, `scene_add_line()`, `scene_add_triangle()`, `scene_add_mesh_box()`, `scene_set_parent()`
- Pattern: Thin facade over ECS world operations plus GPU batch bookkeeping
- Purpose: Turn large imports into cancellable, progress-reporting work units
- Examples: `ply_import_job_t`, `ply_mesh_import_job_t`, `jsonl_import_job_t`
- Pattern: Incremental parsing/creation phases with UI-driven progress polling
- Purpose: Encapsulate a single ImGui window or tool panel behind init/draw/shutdown functions
- Examples: `ui_viewport_*`, `ui_scene_hierarchy_*`, `ui_entity_inspector_*`, `ui_visibility_*`
- Pattern: Header-only module with state struct and lifecycle helpers
## Entry Points
- Location: `src/app.c`
- Triggers: Sokol calls `sokol_main()`, then `init()`, `frame()`, `cleanup()`, and `event()`
- Responsibilities: Set up runtime state, drive one frame of UI and rendering, handle app suspend/resume, and shut everything down cleanly
- Location: `CMakeLists.txt`, `src/CMakeLists.txt`, `android/app/src/main/cpp/CMakeLists.txt`
- Triggers: CMake or Gradle build configuration
- Responsibilities: Select backend, choose platform-specific sources, and produce the correct executable or shared library
## Error Handling
- Importers and serializers return status flags, `NULL`, or partially filled state on failure
- UI actions guard on current selection, hover, and import-job state before mutating scene data
- Shutdown paths free owned buffers and dispose of GPU/ECS resources in reverse creation order
- Logging is routed through Sokol's logger hooks and surfaced in the console
## Cross-Cutting Concerns
- Sokol logger callbacks are wired in `src/app.c`
- Debug and test output also appears through ImGui panels and platform console output
- ImGui panels validate user interactions at the UI boundary before invoking scene mutations
- Importers validate file contents and expose progress before committing entities
- `src/platform.h` maps the host to `SOKOL_METAL`, `SOKOL_D3D11`, `SOKOL_VULKAN`, `SOKOL_GLES3`, `SOKOL_GLCORE`, or `SOKOL_WGPU`
- Vulkan uses separate SPIR-V assets under `src/shaders/spirv/`
- Scene save/load goes through `src/scene_serializer.h`
- ImGui settings persistence is handled by `src/imgui_storage.h`
- On this machine the active working build is macOS + Ninja in `build/`, driven by `cmake -B build -G Ninja && ninja -C build`
<!-- GSD:architecture-end -->

<!-- GSD:workflow-start source:GSD defaults -->
## GSD Workflow Enforcement

Before using Edit, Write, or other file-changing tools, start work through a GSD command so planning artifacts and execution context stay in sync.

Use these entry points:
- `/gsd:quick` for small fixes, doc updates, and ad-hoc tasks
- `/gsd:debug` for investigation and bug fixing
- `/gsd:execute-phase` for planned phase work

Do not make direct repo edits outside a GSD workflow unless the user explicitly asks to bypass it.
<!-- GSD:workflow-end -->

<!-- GSD:profile-start -->
## Developer Profile

> Profile not yet configured. Run `/gsd:profile-user` to generate your developer profile.
> This section is managed by `generate-claude-profile` -- do not edit manually.
<!-- GSD:profile-end -->
