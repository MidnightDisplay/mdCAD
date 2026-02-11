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

# Windows build (D3D11 backend)
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
