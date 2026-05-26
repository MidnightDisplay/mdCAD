# Vulkan Backend Implementation Plan

## Overview

This document outlines the implementation plan for adding Vulkan as a graphics backend to mdCAD, targeting:
1. **Phase 1**: MinGW builds (replacing OpenGL for better performance)
2. **Phase 2**: Visual Studio 2026 builds (optional alongside D3D11)

### Performance Expectations

| Backend | Expected Performance | Notes |
|---------|---------------------|-------|
| Metal | Baseline (excellent) | Current macOS/iOS backend |
| Vulkan | Near-Metal | Low-overhead, explicit API |
| D3D11 | Good | Current Windows MSVC default |
| OpenGL | Moderate | Current MinGW backend, higher driver overhead |

Vulkan should provide Metal-comparable performance due to:
- Explicit memory management
- Pre-compiled pipeline state objects
- Minimal driver overhead
- Multi-threaded command buffer recording

---

## Phase 0: Development Environment Setup (Windows)

### 0.1 Vulkan SDK Installation

**Step 1: Download Vulkan SDK**
- URL: https://vulkan.lunarg.com/sdk/home#windows
- Download the latest Windows SDK installer (e.g., `VulkanSDK-1.3.xxx.x-Installer.exe`)
- Run installer with default options

**Step 2: Verify Installation**
```powershell
# Check environment variable was set
$env:VULKAN_SDK
# Should output something like: C:\VulkanSDK\1.3.xxx.x

# Verify Vulkan runtime works
& "$env:VULKAN_SDK\Bin\vulkaninfo.exe" --summary
```

**Step 3: Verify GPU Support**
```powershell
& "$env:VULKAN_SDK\Bin\vulkaninfo.exe" | Select-String "GPU|deviceName"
```

### 0.2 MinGW Toolchain Requirements

The Vulkan SDK provides libraries compatible with both MSVC and MinGW. For MinGW builds:

**Required Components (included in Vulkan SDK):**
- `$env:VULKAN_SDK\Lib\vulkan-1.lib` - Import library
- `$env:VULKAN_SDK\Include\vulkan\` - Headers

**MinGW-specific Notes:**
- The Vulkan loader (`vulkan-1.dll`) is a runtime dependency
- No additional MinGW packages required beyond standard SDK

**Step 4: Test MinGW Vulkan Compilation**
Create a test file to verify the toolchain:
```c
// test_vulkan.c
#include <vulkan/vulkan.h>
#include <stdio.h>

int main() {
    uint32_t version;
    vkEnumerateInstanceVersion(&version);
    printf("Vulkan version: %d.%d.%d\n",
        VK_VERSION_MAJOR(version),
        VK_VERSION_MINOR(version),
        VK_VERSION_PATCH(version));
    return 0;
}
```

```powershell
# Compile with MinGW
gcc test_vulkan.c -I"$env:VULKAN_SDK\Include" -L"$env:VULKAN_SDK\Lib" -lvulkan-1 -o test_vulkan.exe
.\test_vulkan.exe
```

### 0.3 Shader Compilation Tools

Vulkan requires SPIR-V bytecode. The SDK includes compilers:

| Tool | Purpose | Location |
|------|---------|----------|
| `glslangValidator` | GLSL → SPIR-V | `$env:VULKAN_SDK\Bin\glslangValidator.exe` |
| `glslc` | GLSL → SPIR-V (Google) | `$env:VULKAN_SDK\Bin\glslc.exe` |
| `dxc` | HLSL → SPIR-V | `$env:VULKAN_SDK\Bin\dxc.exe` |
| `spirv-cross` | SPIR-V ↔ GLSL/HLSL/MSL | `$env:VULKAN_SDK\Bin\spirv-cross.exe` |

**Recommended: Use glslc for compilation**
```powershell
# Example: Compile vertex shader
& "$env:VULKAN_SDK\Bin\glslc.exe" -fshader-stage=vert shader.vert -o shader.vert.spv

# Example: Compile fragment shader
& "$env:VULKAN_SDK\Bin\glslc.exe" -fshader-stage=frag shader.frag -o shader.frag.spv
```

---

## Phase 1: MinGW Vulkan Backend

### 1.1 Platform Detection Updates

**File: `src/platform.h`**

Current logic:
```c
#if defined(_WIN32)
    #if defined(__MINGW32__) || defined(__MINGW64__)
        #define SOKOL_GLCORE  // MinGW: OpenGL (current)
    #else
        #define SOKOL_D3D11   // MSVC: D3D11
    #endif
```

Updated logic:
```c
#if defined(_WIN32)
    #if defined(__MINGW32__) || defined(__MINGW64__)
        #define SOKOL_VULKAN  // MinGW: Vulkan (new default)
    #else
        #define SOKOL_D3D11   // MSVC: D3D11
    #endif
    #define PLATFORM_WINDOWS
```

### 1.2 CMake Build System Changes

**File: `vendors/libsokol/CMakeLists.txt`**

Add Vulkan detection and linking for MinGW:

```cmake
elseif(WIN32)
    add_library(libsokol STATIC sokol.c ${SOKOL_HEADERS})
    
    if(MINGW)
        # MinGW: Use Vulkan backend
        # Find Vulkan SDK via environment variable
        if(DEFINED ENV{VULKAN_SDK})
            set(VULKAN_SDK_PATH $ENV{VULKAN_SDK})
            message(STATUS "Found Vulkan SDK: ${VULKAN_SDK_PATH}")
            
            target_include_directories(libsokol PUBLIC "${VULKAN_SDK_PATH}/Include")
            target_link_directories(libsokol PUBLIC "${VULKAN_SDK_PATH}/Lib")
            target_link_libraries(libsokol PUBLIC vulkan-1 user32 shell32 gdi32)
        else()
            message(FATAL_ERROR "VULKAN_SDK environment variable not set. Install Vulkan SDK from https://vulkan.lunarg.com/")
        endif()
    else()
        # MSVC: D3D11 backend (default)
        target_link_libraries(libsokol PUBLIC d3d11 dxgi d3dcompiler user32 shell32 gdi32)
    endif()
endif()
```

### 1.3 Shader Implementation (Vulkan GLSL 450)

Sokol supports runtime GLSL compilation for Vulkan via `sokol_shdc` or expects GLSL 450 strings that it compiles to SPIR-V at runtime using its internal compiler.

**Important**: Sokol's Vulkan backend uses glslang internally to compile GLSL 450 to SPIR-V at runtime. No pre-compilation needed.

**File: `src/shaders/instanced_line_shaders.h`**

Add Vulkan GLSL section:

```c
#elif defined(SOKOL_VULKAN)

static const char* instanced_line_vs_source =
    "#version 450\n"
    "\n"
    "layout(binding=0) uniform vs_params {\n"
    "    mat4 mvp;\n"
    "    float line_width;\n"
    "    float aspect_ratio;\n"
    "    vec2 _pad;\n"
    "};\n"
    "\n"
    "layout(location=0) in vec4 in_pos;\n"
    "layout(location=1) in vec4 inst_start;\n"
    "layout(location=2) in vec4 inst_end;\n"
    "layout(location=3) in vec4 inst_color;\n"
    "\n"
    "layout(location=0) out vec4 fs_color;\n"
    "\n"
    "void main() {\n"
    "    vec4 start_clip = mvp * vec4(inst_start.xyz, 1.0);\n"
    "    vec4 end_clip = mvp * vec4(inst_end.xyz, 1.0);\n"
    "    vec2 start_ndc = start_clip.xy / start_clip.w;\n"
    "    vec2 end_ndc = end_clip.xy / end_clip.w;\n"
    "    vec2 dir = normalize(end_ndc - start_ndc);\n"
    "    vec2 perp = vec2(-dir.y, dir.x);\n"
    "    perp.x /= aspect_ratio;\n"
    "    float t = in_pos.x;\n"
    "    float side = in_pos.y;\n"
    "    vec4 base_clip = mix(start_clip, end_clip, t);\n"
    "    float half_width = line_width * 0.5;\n"
    "    vec2 offset = perp * side * half_width;\n"
    "    gl_Position = vec4(base_clip.xy + offset * base_clip.w, base_clip.zw);\n"
    "    fs_color = inst_color;\n"
    "}\n";

static const char* instanced_line_fs_source =
    "#version 450\n"
    "\n"
    "layout(location=0) in vec4 fs_color;\n"
    "layout(location=0) out vec4 frag_color;\n"
    "\n"
    "void main() {\n"
    "    frag_color = fs_color;\n"
    "}\n";
```

**Files requiring Vulkan GLSL sections:**
- [ ] `src/shaders/instanced_line_shaders.h`
- [ ] `src/shaders/pick_shaders.h`
- [ ] `src/shaders/join_shaders.h`

### 1.4 Pick Buffer Readback Implementation

**New File: `src/gpu/pick_readback_vulkan.c`**

```c
#include "../platform.h"

#if defined(SOKOL_VULKAN)

#include "sokol_gfx.h"
#include <vulkan/vulkan.h>
#include <string.h>
#include <stdlib.h>

// Sokol exposes Vulkan handles via these functions (check sokol_gfx.h for availability)
// extern VkDevice sg_vk_device(void);
// extern VkPhysicalDevice sg_vk_physical_device(void);
// extern VkQueue sg_vk_queue(void);
// extern VkCommandPool sg_vk_command_pool(void);

bool pick_readback_pixels(sg_image img, int width, int height, uint8_t *pixel_data) {
    // Implementation approach:
    // 1. Query Vulkan handles from Sokol
    // 2. Create staging buffer (HOST_VISIBLE | HOST_COHERENT)
    // 3. Record command buffer with vkCmdCopyImageToBuffer
    // 4. Submit and wait for completion
    // 5. Map staging buffer and copy to pixel_data
    // 6. Cleanup staging buffer
    
    // NOTE: Sokol may not expose all necessary Vulkan internals.
    // If sg_vk_* functions aren't available, we may need to:
    // Option A: Use Sokol's sg_query_image_info() if it provides Vulkan image handle
    // Option B: Maintain parallel Vulkan resources for pick buffer
    // Option C: Render pick buffer to a mapped persistent buffer
    
    // Placeholder - needs investigation of Sokol's Vulkan API exposure
    memset(pixel_data, 0, width * height * 4);
    return false;  // Not implemented yet
}

#endif // SOKOL_VULKAN
```

**Investigation Required:**
Check `vendors/libsokol/sokol_gfx.h` for Vulkan-specific query functions:
```c
// Look for functions like:
SOKOL_GFX_API_DECL const void* sg_vk_image_handle(sg_image img);
SOKOL_GFX_API_DECL VkDevice sg_vk_device(void);
```

### 1.5 Update Source CMakeLists.txt

**File: `src/CMakeLists.txt`**

Add Vulkan readback source:

```cmake
# GPU picking readback implementations
if(APPLE)
    list(APPEND SOURCES gpu/pick_readback_metal.m)
elseif(WIN32)
    if(MINGW)
        # MinGW: Vulkan backend
        list(APPEND SOURCES gpu/pick_readback_vulkan.c)
    else()
        # MSVC: D3D11 backend
        list(APPEND SOURCES gpu/pick_readback_d3d11.c)
    endif()
endif()
```

### 1.6 Update pick_readback.h

**File: `src/gpu/pick_readback.h`**

Add Vulkan declaration:

```c
#elif defined(SOKOL_VULKAN)
// Vulkan implementation in pick_readback_vulkan.c
bool pick_readback_pixels(sg_image img, int width, int height, uint8_t *pixel_data);
```

---

## Phase 1 Implementation Checklist

### Environment Setup
- [x] Install Vulkan SDK on development machine
- [x] Verify `VULKAN_SDK` environment variable
- [x] Test `vulkaninfo.exe` shows GPU support
- [x] Verify MinGW can compile/link Vulkan test program

### Build System
- [x] Update `vendors/libsokol/CMakeLists.txt` with Vulkan linking
- [x] Update `src/CMakeLists.txt` for Vulkan readback source
- [x] Test CMake configuration with MinGW

### Platform Detection
- [x] Update `src/platform.h` to use `SOKOL_VULKAN` for MinGW

### Shaders
- [x] Add Vulkan GLSL to `instanced_line_shaders.h`
- [x] Add Vulkan GLSL to `pick_shaders.h`
- [x] Add Vulkan GLSL to `join_shaders.h`
- [x] Verify shader syntax with `glslangValidator --target-env vulkan1.0`

### Pick Readback
- [x] Investigate Sokol Vulkan API exposure
- [x] Implement `pick_readback_vulkan.c`
- [x] Update `pick_readback.h` header

### Testing
- [x] Build MinGW Vulkan version
- [x] Verify basic rendering works
- [x] Verify line rendering works
- [x] Verify join rendering works
- [x] Test pick buffer (fully implemented)
- [x] Performance comparison vs OpenGL build (result: significant visible difference ~20 FPS)

---

## Phase 2: Visual Studio 2026 Vulkan Option (COMPLETED)

Phase 2 has been implemented, adding Vulkan as an optional backend for MSVC builds.

### 2.1 Build Option (IMPLEMENTED)

CMake option `USE_VULKAN` added for MSVC builds:

```cmake
# In vendors/libsokol/CMakeLists.txt
option(USE_VULKAN "Use Vulkan backend instead of D3D11 (MSVC only)" OFF)

elseif(WIN32)
    add_library(libsokol STATIC sokol.c ${SOKOL_HEADERS})
    
    if(MINGW)
        # MinGW: Always Vulkan
        # ... (Phase 1 code)
    elseif(USE_VULKAN)
        # MSVC with Vulkan option
        if(DEFINED ENV{VULKAN_SDK})
            set(VULKAN_SDK_PATH $ENV{VULKAN_SDK})
            target_include_directories(libsokol PUBLIC "${VULKAN_SDK_PATH}/Include")
            target_link_directories(libsokol PUBLIC "${VULKAN_SDK_PATH}/Lib")
            target_link_libraries(libsokol PUBLIC vulkan-1 user32 shell32 gdi32)
            target_compile_definitions(libsokol PRIVATE USE_VULKAN=1)
        else()
            message(FATAL_ERROR "VULKAN_SDK not set for Vulkan build")
        endif()
    else()
        # MSVC: Default D3D11
        target_link_libraries(libsokol PUBLIC d3d11 dxgi d3dcompiler user32 shell32 gdi32)
    endif()
endif()
```

### 2.2 Platform Header Update (IMPLEMENTED)

Platform detection updated to handle `USE_VULKAN` compile definition:

```c
// src/platform.h
#if defined(_WIN32)
    #if defined(__MINGW32__) || defined(__MINGW64__)
        #define SOKOL_VULKAN  // MinGW: Vulkan
    #elif defined(USE_VULKAN)
        #define SOKOL_VULKAN  // MSVC: Vulkan (when USE_VULKAN defined)
    #else
        #define SOKOL_D3D11   // MSVC: D3D11 (default)
    #endif
    #define PLATFORM_WINDOWS
```

### 2.3 Build Commands (VERIFIED)

All build configurations tested and working:

```powershell
# MinGW build (Vulkan - Phase 1)
cmake -B build-mingw -G "MinGW Makefiles"
cmake --build build-mingw

# MSVC D3D11 build (default)
cmake -B build -G "Visual Studio 18 2026"
cmake --build build --config Release

# MSVC Vulkan build (Phase 2 - future)
cmake -B build-vulkan -G "Visual Studio 18 2026" -DUSE_VULKAN=ON
cmake --build build-vulkan --config Release
```

---

## Shader Syntax Reference

### GLSL 450 for Vulkan vs Other Backends

| Feature | GLSL 450 (Vulkan) | GLSL Core (OpenGL) | HLSL (D3D11) |
|---------|-------------------|--------------------|--------------| 
| Version | `#version 450` | `#version 330` | N/A |
| UBO binding | `layout(binding=0) uniform` | `layout(std140) uniform` | `cbuffer` |
| Input | `layout(location=X) in` | `in` | `: POSITION` |
| Output | `layout(location=X) out` | `out` | `: SV_Target` |
| Frag output | `layout(location=0) out vec4` | `out vec4` | `: SV_Target` |

### Vulkan GLSL Uniform Block Rules

```glsl
// Vulkan requires explicit binding
layout(binding=0) uniform vs_params {
    mat4 mvp;
    float line_width;
    float aspect_ratio;
    vec2 _pad;  // Explicit padding for std140 alignment
};
```

**Alignment Rules (std140):**
- `float`: 4 bytes
- `vec2`: 8 bytes (2×4)
- `vec3`: 16 bytes (padded)
- `vec4`: 16 bytes
- `mat4`: 64 bytes (4× vec4)

Always pad to 16-byte boundaries for uniform blocks.

---

## Risk Assessment

| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|------------|
| Sokol doesn't expose Vulkan internals for readback | Medium | High | Use placeholder, investigate alternatives |
| Shader compilation errors | Medium | Medium | Test with glslangValidator before runtime |
| Driver compatibility | Low | Medium | Test on multiple GPUs |
| Performance not meeting expectations | Low | Medium | Profile and optimize if needed |
| MinGW Vulkan linking issues | Low | Medium | SDK provides compatible libraries |

---

## Performance Benchmarking Plan

Once Vulkan build is working, measure:

1. **Frame time** - Average ms per frame with test scene
2. **Draw call overhead** - Time per sg_draw()
3. **Buffer upload** - Instance buffer update latency
4. **GPU memory** - Total VRAM usage

**Test Scene:**
- 10,000 line entities
- 5,000 point entities
- Orbit camera animation

**Comparison Matrix:**
| Metric | OpenGL (MinGW) | Vulkan (MinGW) | D3D11 (MSVC) | Metal (macOS) |
|--------|----------------|----------------|--------------|---------------|
| Avg frame time | ? ms | ? ms | ? ms | baseline |
| Draw call cost | ? μs | ? μs | ? μs | baseline |

---

## Timeline Estimate

| Phase | Task | Estimate |
|-------|------|----------|
| 0 | Environment setup | 1 hour |
| 1.1-1.2 | Build system changes | 1-2 hours |
| 1.3 | Shader conversion | 2-3 hours |
| 1.4-1.6 | Pick readback | 2-4 hours (depends on Sokol API) |
| 1.x | Testing & debugging | 2-4 hours |
| **Phase 1 Total** | | **8-14 hours** |
| 2 | MSVC Vulkan option | 2-3 hours (reuses Phase 1 work) |

---

## References

- [Sokol GitHub](https://github.com/floooh/sokol)
- [Sokol Vulkan Backend Notes](https://github.com/floooh/sokol/blob/master/sokol_gfx.h) - Search for `SOKOL_VULKAN`
- [Vulkan SDK Documentation](https://vulkan.lunarg.com/doc/sdk)
- [GLSL 450 Specification](https://registry.khronos.org/OpenGL/specs/gl/GLSLangSpec.4.50.pdf)
- [Vulkan Memory Allocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator) - May be useful for readback

---

## Appendix A: Quick Verification Commands

```powershell
# Check Vulkan SDK installation
Write-Host "VULKAN_SDK: $env:VULKAN_SDK"
if (Test-Path "$env:VULKAN_SDK\Bin\vulkaninfo.exe") {
    Write-Host "Vulkan SDK installed correctly"
    & "$env:VULKAN_SDK\Bin\vulkaninfo.exe" --summary
} else {
    Write-Host "ERROR: Vulkan SDK not found"
}

# Check MinGW installation
where.exe gcc
gcc --version

# Check CMake
cmake --version
```

## Appendix B: Troubleshooting

### "vulkan-1.lib not found" (MinGW)
```powershell
# Verify library exists
Test-Path "$env:VULKAN_SDK\Lib\vulkan-1.lib"

# Check CMake finds it
cmake -B build-mingw -G "MinGW Makefiles" --debug-find
```

### "vk*" symbols undefined
- Ensure `vulkan-1` is linked (not just `vulkan`)
- Check link order: Vulkan library should come before system libraries

### Shader compilation errors at runtime
```powershell
# Pre-validate shaders
& "$env:VULKAN_SDK\Bin\glslangValidator.exe" --target-env vulkan1.0 -e main shader.vert
```

### Black screen but no errors
- Enable Vulkan validation layers:
  ```c
  sg_setup(&(sg_desc){
      .environment = sglue_environment(),
      .logger.func = slog_func,
  });
  ```
- Check Sokol log output for Vulkan-specific messages

---

*Document created: February 9, 2026*
*Based on: plan-d3d12VsBackend.prompt.md*
