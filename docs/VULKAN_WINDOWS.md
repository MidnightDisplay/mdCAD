# Vulkan Backend for Windows

This document explains the Vulkan backend implementation for mdCAD on Windows, covering both MinGW (default Vulkan) and Visual Studio 2026 (optional Vulkan) builds.

## Overview

mdCAD uses [Sokol](https://github.com/floooh/sokol) as its cross-platform graphics abstraction layer. Sokol supports multiple backends:

| Backend | Platforms | Shader Format |
|---------|-----------|---------------|
| Metal | macOS, iOS | MSL source strings |
| D3D11 | Windows (MSVC default) | HLSL source strings |
| OpenGL | Linux, Web (fallback) | GLSL source strings |
| WebGPU | Web | WGSL source strings |
| **Vulkan** | **Windows (MinGW, MSVC optional)**, Linux | **SPIR-V bytecode** |

The critical difference is that **Vulkan requires precompiled SPIR-V bytecode**, while all other backends accept shader source code as strings that are compiled at runtime.

## Why Vulkan for MinGW?

On Windows, we have two toolchain options:
- **MSVC (Visual Studio)**: Uses D3D11 backend by default, Vulkan optional via `-DUSE_VULKAN=ON`
- **MinGW (GCC)**: Uses Vulkan backend (default)

Vulkan provides better performance than OpenGL due to:
- Lower driver overhead
- Explicit memory management
- Pre-compiled pipeline state objects
- Better multi-threading support

## Shader Architecture

### Other Backends (Source Strings)

For Metal, D3D11, OpenGL, and WebGPU, shaders are embedded as C string literals:

```c
// Example: GLSL source string for OpenGL
static const char* my_vs_source =
    "#version 330\n"
    "uniform mat4 mvp;\n"
    "in vec3 position;\n"
    "void main() {\n"
    "    gl_Position = mvp * vec4(position, 1.0);\n"
    "}\n";

// Shader creation uses .source field
sg_make_shader(&(sg_shader_desc){
    .vertex_func = {
        .source = my_vs_source,
        .entry = "main",
    },
    // ...
});
```

### Vulkan Backend (SPIR-V Bytecode)

Vulkan shaders must be precompiled to SPIR-V binary format and embedded as byte arrays:

```c
// Example: SPIR-V bytecode array
static const uint8_t my_vs_spirv[1234] = {
    0x03, 0x02, 0x23, 0x07, 0x00, 0x00, 0x01, 0x00, // SPIR-V header
    // ... hundreds more bytes ...
};

// Shader creation uses .bytecode field with SG_RANGE macro
sg_make_shader(&(sg_shader_desc){
    .vertex_func = {
        .bytecode = SG_RANGE(my_vs_spirv),
        .entry = "main",  // Always "main" for SPIR-V
    },
    // ...
});
```

### Key Differences Summary

| Aspect | Other Backends | Vulkan |
|--------|---------------|--------|
| Shader format | Source strings | SPIR-V bytecode |
| Entry point | `vs_main`, `fs_main` | `main` |
| Compilation | Runtime (by driver/Sokol) | Offline (by glslc) |
| Field used | `.source` | `.bytecode` with `SG_RANGE()` |
| Uniform names | Required (for reflection) | Not needed |

## File Organization

```
src/shaders/
├── instanced_line_shaders.h    # Multi-backend shader header
├── join_shaders.h              # Multi-backend shader header
├── pick_shaders.h              # Multi-backend shader header
└── spirv/
    ├── instanced_line.vert     # GLSL 450 vertex source
    ├── instanced_line.frag     # GLSL 450 fragment source
    ├── instanced_line_vs.spv   # Compiled SPIR-V (vertex)
    ├── instanced_line_fs.spv   # Compiled SPIR-V (fragment)
    ├── join.vert
    ├── join.frag
    ├── join_vs.spv
    ├── join_fs.spv
    ├── pick_line.vert
    ├── pick_line.frag
    ├── pick_line_vs.spv
    ├── pick_line_fs.spv
    ├── pick_point.vert
    ├── pick_point.frag
    ├── pick_point_vs.spv
    ├── pick_point_fs.spv
    └── spirv_bytecode.h        # Generated: all bytecode arrays
```

## Adding New Shaders

### Step 1: Write GLSL 450 Source Files

Create `.vert` and `.frag` files in `src/shaders/spirv/`:

```glsl
// src/shaders/spirv/my_shader.vert
#version 450

// Uniform block at binding 0
layout(binding=0) uniform vs_params {
    mat4 mvp;
    // Add other uniforms here
};

// Vertex inputs with explicit locations
layout(location=0) in vec3 position;
layout(location=1) in vec4 color;

// Outputs to fragment shader
layout(location=0) out vec4 v_color;

void main() {
    gl_Position = mvp * vec4(position, 1.0);
    v_color = color;
}
```

```glsl
// src/shaders/spirv/my_shader.frag
#version 450

layout(location=0) in vec4 v_color;
layout(location=0) out vec4 frag_color;

void main() {
    frag_color = v_color;
}
```

### Step 2: Compile to SPIR-V

Run the compilation script:

```powershell
# From project root
.\scripts\vulkan-win\compile-spirv.ps1

# Or run the full pipeline
.\scripts\vulkan-win\build-all.ps1
```

This uses `glslc` from the Vulkan SDK to compile `.vert` → `_vs.spv` and `.frag` → `_fs.spv`.

### Step 3: Generate Bytecode Header

```powershell
.\scripts\vulkan-win\generate-bytecode-header.ps1
```

This reads all `.spv` files and generates `spirv_bytecode.h` with C byte arrays.

### Step 4: Create Multi-Backend Shader Header

Create a shader header with conditional compilation:

```c
// src/shaders/my_shaders.h
#ifndef MY_SHADERS_H
#define MY_SHADERS_H

#include "../platform.h"

//------------------------------------------------------------------------------
// OpenGL GLSL 330
//------------------------------------------------------------------------------
#if defined(SOKOL_GLCORE) || defined(SOKOL_GLES3)

static const char* my_vs_source = 
    "#version 330\n"
    // ... GLSL 330 source ...
    ;

static const char* my_fs_source = 
    "#version 330\n"
    // ... GLSL 330 source ...
    ;

//------------------------------------------------------------------------------
// Vulkan SPIR-V bytecode
//------------------------------------------------------------------------------
#elif defined(SOKOL_VULKAN)

#include "spirv/spirv_bytecode.h"
// Uses my_shader_vs_spirv and my_shader_fs_spirv from spirv_bytecode.h

//------------------------------------------------------------------------------
// Metal MSL
//------------------------------------------------------------------------------
#elif defined(SOKOL_METAL)

static const char* my_vs_source = 
    "#include <metal_stdlib>\n"
    // ... MSL source ...
    ;

// ... etc for other backends

#endif
#endif // MY_SHADERS_H
```

### Step 5: Use in GPU Code

```c
// In your GPU code (e.g., geometry_batch.h)

#if defined(SOKOL_VULKAN)
    shd = sg_make_shader(&(sg_shader_desc){
        .vertex_func = {
            .bytecode = SG_RANGE(my_shader_vs_spirv),
            .entry = "main",
        },
        .fragment_func = {
            .bytecode = SG_RANGE(my_shader_fs_spirv),
            .entry = "main",
        },
        .attrs = {
            [0] = { .hlsl_sem_name = "POSITION" },
            [1] = { .hlsl_sem_name = "COLOR" },
        },
        .uniform_blocks[0] = {
            .stage = SG_SHADERSTAGE_VERTEX,
            .size = sizeof(my_params_t),
            .layout = SG_UNIFORMLAYOUT_STD140,
            // Note: .glsl_uniforms NOT needed for Vulkan
        },
    });
#else
    shd = sg_make_shader(&(sg_shader_desc){
        .vertex_func = {
            .source = my_vs_source,
            .entry = "vs_main",
        },
        .fragment_func = {
            .source = my_fs_source,
            .entry = "fs_main",
        },
        .attrs = { ... },
        .uniform_blocks[0] = {
            .stage = SG_SHADERSTAGE_VERTEX,
            .size = sizeof(my_params_t),
            .layout = SG_UNIFORMLAYOUT_STD140,
            .glsl_uniforms = {
                [0] = { .type = SG_UNIFORMTYPE_MAT4, .glsl_name = "mvp" },
                // ... uniform reflection info for GLSL
            }
        },
    });
#endif
```

## Build Scripts Reference

All scripts are in `scripts/vulkan-win/`:

| Script | Purpose |
|--------|---------|
| `compile-spirv.ps1` | Compile `.vert`/`.frag` → `.spv` using glslc |
| `generate-bytecode-header.ps1` | Convert `.spv` files → `spirv_bytecode.h` |
| `build-all.ps1` | Run complete pipeline (both steps) |

### Prerequisites

1. **Vulkan SDK** installed from https://vulkan.lunarg.com/
2. **VULKAN_SDK** environment variable set (installer does this automatically)
3. **PowerShell** (included with Windows)

### Running the Pipeline

```powershell
# Set VULKAN_SDK if not already set
$env:VULKAN_SDK = "C:\VulkanSDK\1.4.341.1"

# Run full shader build
.\scripts\vulkan-win\build-all.ps1

# Then rebuild the project (MinGW config)
cmake --build build-mingw
```

## Build Commands

### MinGW Vulkan Build (Default)

```powershell
# MinGW always uses Vulkan backend
cmake -B build-mingw -G "MinGW Makefiles"
cmake --build build-mingw
.\build-mingw\bin\mdCAD.exe
```

### Visual Studio 2026 with Vulkan (Default)

```powershell
# MSVC with Vulkan backend - requires Vulkan SDK installed
cmake -B build-vulkan -G "Visual Studio 18" -DUSE_VULKAN=ON
cmake --build build-vulkan --config Release
.\build-vulkan\bin\Release\mdCAD.exe
```

### Visual Studio 2026 with D3D11 (Optional)

```powershell
# MSVC default is D3D11
cmake -B build-msvc -G "Visual Studio 18"
cmake --build build-msvc --config Release
.\build-msvc\bin\Release\mdCAD.exe
```

**Note:** The `USE_VULKAN=ON` option requires the Vulkan SDK to be installed and the `VULKAN_SDK` environment variable to be set. The CMake configuration will fail with an error message if the SDK is not found.

## Vulkan-Specific Configuration

### GPU Picking (Texture Readback)

Vulkan GPU picking requires reading pixels from the pick buffer texture back to CPU. Unlike Metal and D3D11, Sokol doesn't provide a built-in `sg_vk_query_image_info()` function to get native VkImage handles.

**Custom Solution:**

We added a custom extension in `vendors/libsokol/sokol.c`:

```c
// Custom Vulkan image query (not provided by Sokol)
typedef struct sg_vk_image_info_ext {
    VkImage image;
    VkDeviceMemory memory;
} sg_vk_image_info_ext;

sg_vk_image_info_ext sg_vk_query_image_info_ext(sg_image img_id);
```

The readback implementation in `src/gpu/pick_readback_vulkan.c`:
1. Gets VkDevice/VkQueue from `sapp_get_environment().vulkan`
2. Gets VkImage via our custom extension
3. Creates a host-visible staging buffer
4. Transitions image layout to TRANSFER_SRC_OPTIMAL
5. Copies image to buffer via `vkCmdCopyImageToBuffer`
6. Transitions image back to ATTACHMENT_OPTIMAL
7. Maps buffer and copies to CPU memory

**Note:** This depends on Sokol internals. If you update Sokol and picking breaks, check if `_sg_image_t` struct layout changed.

### Staging Buffer Size

Vulkan uses a per-frame staging buffer for dynamic buffer updates. The default is 16MB, which may be insufficient for large point clouds:

```c
// In app.c sg_setup()
sg_setup(&(sg_desc){
    .environment = sglue_environment(),
#if defined(SOKOL_VULKAN)
    // Increase for large point clouds (default: 16MB)
    .vulkan.stream_staging_buffer_size = 64 * 1024 * 1024,  // 64MB
#endif
});
```

If you see rendering glitches with large datasets, this buffer may need to be increased.

## Troubleshooting

### Shader Compilation Errors

```powershell
# Check glslc is available
& "$env:VULKAN_SDK\Bin\glslc.exe" --version

# Compile with verbose errors
& "$env:VULKAN_SDK\Bin\glslc.exe" -fshader-stage=vert my_shader.vert -o my_shader.spv
```

### Runtime Shader Failures

Enable debug output to see shader state:
```c
printf("Shader state: %d (2=valid, 4=failed)\n", sg_query_shader_state(shd));
```

### Vulkan Validation Layers

For detailed Vulkan debugging:
```powershell
# Enable validation layers (set before running app)
$env:VK_INSTANCE_LAYERS = "VK_LAYER_KHRONOS_validation"
.\build-mingw\bin\mdCAD.exe
```

## Further Reading

- [Sokol Shader Documentation](https://github.com/floooh/sokol/blob/master/sokol_gfx.h)
- [GLSL 450 Specification](https://www.khronos.org/registry/OpenGL/specs/gl/GLSLangSpec.4.50.pdf)
- [SPIR-V Specification](https://www.khronos.org/registry/SPIR-V/)
- [Vulkan SDK Documentation](https://vulkan.lunarg.com/doc/sdk)
