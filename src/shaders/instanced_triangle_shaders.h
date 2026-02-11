//------------------------------------------------------------------------------
// instanced_triangle_shaders.h - Instanced triangle shaders for all backends
//
// GPU instanced triangle rendering with per-vertex color interpolation
// and 3-point studio lighting (computed in vertex shader for flat shading).
//
// Vertex attributes:
//   - location 0: template_pos (vec3) - barycentric selector (1,0,0)/(0,1,0)/(0,0,1)
//   - location 1: vertex_a (vec3) - triangle vertex A in world space
//   - location 2: vertex_b (vec3) - triangle vertex B in world space
//   - location 3: vertex_c (vec3) - triangle vertex C in world space
//   - location 4: normal (vec3) - face normal
//   - location 5: color_a (vec4) - color at vertex A
//   - location 6: color_b (vec4) - color at vertex B
//   - location 7: color_c (vec4) - color at vertex C
//
// Uniforms (VS block 0, std140):
//   - mvp (mat4)           - model-view-projection matrix
//   - light_dirs[4] (vec4) - xyz=direction/position, w=type (0=directional, 1=point)
//   - light_colors[4](vec4)- rgb=color, w=intensity
//   - ambient_color (vec4) - rgb=color, w=intensity
//   - num_lights (float)   - active light count
//   - lighting_enabled (float) - 0.0=unlit, 1.0=lit
//------------------------------------------------------------------------------
#ifndef INSTANCED_TRIANGLE_SHADERS_H
#define INSTANCED_TRIANGLE_SHADERS_H

//------------------------------------------------------------------------------
// OpenGL 3.3 GLSL
//------------------------------------------------------------------------------
#if defined(SOKOL_GLCORE)

static const char* instanced_triangle_vs_source =
    "#version 330\n"
    "uniform mat4 mvp;\n"
    "uniform vec4 light_dirs[4];\n"
    "uniform vec4 light_colors[4];\n"
    "uniform vec4 ambient_color;\n"
    "uniform float num_lights;\n"
    "uniform float lighting_enabled;\n"
    "layout(location=0) in vec3 template_pos;\n"
    "layout(location=1) in vec3 vertex_a;\n"
    "layout(location=2) in vec3 vertex_b;\n"
    "layout(location=3) in vec3 vertex_c;\n"
    "layout(location=4) in vec3 normal;\n"
    "layout(location=5) in vec4 color_a;\n"
    "layout(location=6) in vec4 color_b;\n"
    "layout(location=7) in vec4 color_c;\n"
    "out vec4 v_color;\n"
    "out vec3 v_lighting;\n"
    "void main() {\n"
    "    vec3 world_pos = template_pos.x * vertex_a\n"
    "                   + template_pos.y * vertex_b\n"
    "                   + template_pos.z * vertex_c;\n"
    "    gl_Position = mvp * vec4(world_pos, 1.0);\n"
    "    v_color = template_pos.x * color_a\n"
    "            + template_pos.y * color_b\n"
    "            + template_pos.z * color_c;\n"
    "    if (lighting_enabled > 0.5) {\n"
    "        vec3 N = normalize(normal);\n"
    "        vec3 diffuse = vec3(0.0);\n"
    "        int n = int(num_lights);\n"
    "        for (int i = 0; i < n; i++) {\n"
    "            vec3 L = normalize(-light_dirs[i].xyz);\n"
    "            float NdotL = abs(dot(N, L));\n"
    "            diffuse += light_colors[i].rgb * light_colors[i].w * NdotL;\n"
    "        }\n"
    "        vec3 ambient = ambient_color.rgb * ambient_color.w;\n"
    "        v_lighting = ambient + diffuse;\n"
    "    } else {\n"
    "        v_lighting = vec3(1.0);\n"
    "    }\n"
    "}\n";

static const char* instanced_triangle_fs_source =
    "#version 330\n"
    "in vec4 v_color;\n"
    "in vec3 v_lighting;\n"
    "out vec4 frag_color;\n"
    "void main() {\n"
    "    frag_color = vec4(v_color.rgb * v_lighting, v_color.a);\n"
    "}\n";

//------------------------------------------------------------------------------
// OpenGL ES 3.0 GLSL (Android)
//------------------------------------------------------------------------------
#elif defined(SOKOL_GLES3)

static const char* instanced_triangle_vs_source =
    "#version 300 es\n"
    "precision highp float;\n"
    "precision highp int;\n"
    "uniform mat4 mvp;\n"
    "uniform vec4 light_dirs[4];\n"
    "uniform vec4 light_colors[4];\n"
    "uniform vec4 ambient_color;\n"
    "uniform float num_lights;\n"
    "uniform float lighting_enabled;\n"
    "layout(location=0) in vec3 template_pos;\n"
    "layout(location=1) in vec3 vertex_a;\n"
    "layout(location=2) in vec3 vertex_b;\n"
    "layout(location=3) in vec3 vertex_c;\n"
    "layout(location=4) in vec3 normal;\n"
    "layout(location=5) in vec4 color_a;\n"
    "layout(location=6) in vec4 color_b;\n"
    "layout(location=7) in vec4 color_c;\n"
    "out vec4 v_color;\n"
    "out vec3 v_lighting;\n"
    "void main() {\n"
    "    vec3 world_pos = template_pos.x * vertex_a\n"
    "                   + template_pos.y * vertex_b\n"
    "                   + template_pos.z * vertex_c;\n"
    "    gl_Position = mvp * vec4(world_pos, 1.0);\n"
    "    v_color = template_pos.x * color_a\n"
    "            + template_pos.y * color_b\n"
    "            + template_pos.z * color_c;\n"
    "    if (lighting_enabled > 0.5) {\n"
    "        vec3 N = normalize(normal);\n"
    "        vec3 diffuse = vec3(0.0);\n"
    "        int n = int(num_lights);\n"
    "        for (int i = 0; i < n; i++) {\n"
    "            vec3 L = normalize(-light_dirs[i].xyz);\n"
    "            float NdotL = abs(dot(N, L));\n"
    "            diffuse += light_colors[i].rgb * light_colors[i].w * NdotL;\n"
    "        }\n"
    "        vec3 ambient = ambient_color.rgb * ambient_color.w;\n"
    "        v_lighting = ambient + diffuse;\n"
    "    } else {\n"
    "        v_lighting = vec3(1.0);\n"
    "    }\n"
    "}\n";

static const char* instanced_triangle_fs_source =
    "#version 300 es\n"
    "precision highp float;\n"
    "in vec4 v_color;\n"
    "in vec3 v_lighting;\n"
    "out vec4 frag_color;\n"
    "void main() {\n"
    "    frag_color = vec4(v_color.rgb * v_lighting, v_color.a);\n"
    "}\n";

//------------------------------------------------------------------------------
// Metal Shading Language
//------------------------------------------------------------------------------
#elif defined(SOKOL_METAL)

static const char* instanced_triangle_vs_source =
    "#include <metal_stdlib>\n"
    "using namespace metal;\n"
    "struct vs_in {\n"
    "    float3 template_pos [[attribute(0)]];\n"
    "    float3 vertex_a [[attribute(1)]];\n"
    "    float3 vertex_b [[attribute(2)]];\n"
    "    float3 vertex_c [[attribute(3)]];\n"
    "    float3 normal [[attribute(4)]];\n"
    "    float4 color_a [[attribute(5)]];\n"
    "    float4 color_b [[attribute(6)]];\n"
    "    float4 color_c [[attribute(7)]];\n"
    "};\n"
    "struct vs_out {\n"
    "    float4 pos [[position]];\n"
    "    float4 color;\n"
    "    float3 lighting;\n"
    "};\n"
    "struct vs_params {\n"
    "    float4x4 mvp;\n"
    "    float4 light_dirs[4];\n"
    "    float4 light_colors[4];\n"
    "    float4 ambient_color;\n"
    "    float num_lights;\n"
    "    float lighting_enabled;\n"
    "    float _pad[2];\n"
    "};\n"
    "vertex vs_out vs_main(vs_in in [[stage_in]], constant vs_params& params [[buffer(0)]]) {\n"
    "    vs_out out;\n"
    "    float3 world_pos = in.template_pos.x * in.vertex_a\n"
    "                     + in.template_pos.y * in.vertex_b\n"
    "                     + in.template_pos.z * in.vertex_c;\n"
    "    out.pos = params.mvp * float4(world_pos, 1.0);\n"
    "    out.color = in.template_pos.x * in.color_a\n"
    "             + in.template_pos.y * in.color_b\n"
    "             + in.template_pos.z * in.color_c;\n"
    "    if (params.lighting_enabled > 0.5) {\n"
    "        float3 N = normalize(in.normal);\n"
    "        float3 diffuse = float3(0.0);\n"
    "        int n = int(params.num_lights);\n"
    "        for (int i = 0; i < n; i++) {\n"
    "            float3 L = normalize(-params.light_dirs[i].xyz);\n"
    "            float NdotL = abs(dot(N, L));\n"
    "            diffuse += params.light_colors[i].rgb * params.light_colors[i].w * NdotL;\n"
    "        }\n"
    "        float3 ambient = params.ambient_color.rgb * params.ambient_color.w;\n"
    "        out.lighting = ambient + diffuse;\n"
    "    } else {\n"
    "        out.lighting = float3(1.0);\n"
    "    }\n"
    "    return out;\n"
    "}\n";

static const char* instanced_triangle_fs_source =
    "#include <metal_stdlib>\n"
    "using namespace metal;\n"
    "struct fs_in {\n"
    "    float4 color;\n"
    "    float3 lighting;\n"
    "};\n"
    "fragment float4 fs_main(fs_in in [[stage_in]]) {\n"
    "    return float4(in.color.rgb * in.lighting, in.color.a);\n"
    "}\n";

//------------------------------------------------------------------------------
// WebGPU WGSL
//------------------------------------------------------------------------------
#elif defined(SOKOL_WGPU)

static const char* instanced_triangle_vs_source =
    "struct vs_params {\n"
    "    mvp: mat4x4<f32>,\n"
    "    light_dirs: array<vec4<f32>, 4>,\n"
    "    light_colors: array<vec4<f32>, 4>,\n"
    "    ambient_color: vec4<f32>,\n"
    "    num_lights: f32,\n"
    "    lighting_enabled: f32,\n"
    "    _pad0: f32,\n"
    "    _pad1: f32,\n"
    "};\n"
    "@group(0) @binding(0) var<uniform> params: vs_params;\n"
    "struct vs_out {\n"
    "    @builtin(position) pos: vec4<f32>,\n"
    "    @location(0) color: vec4<f32>,\n"
    "    @location(1) lighting: vec3<f32>,\n"
    "};\n"
    "@vertex\n"
    "fn vs_main(\n"
    "    @location(0) template_pos: vec3<f32>,\n"
    "    @location(1) vertex_a: vec3<f32>,\n"
    "    @location(2) vertex_b: vec3<f32>,\n"
    "    @location(3) vertex_c: vec3<f32>,\n"
    "    @location(4) normal: vec3<f32>,\n"
    "    @location(5) color_a: vec4<f32>,\n"
    "    @location(6) color_b: vec4<f32>,\n"
    "    @location(7) color_c: vec4<f32>\n"
    ") -> vs_out {\n"
    "    var out: vs_out;\n"
    "    let world_pos = template_pos.x * vertex_a\n"
    "                  + template_pos.y * vertex_b\n"
    "                  + template_pos.z * vertex_c;\n"
    "    out.pos = params.mvp * vec4<f32>(world_pos, 1.0);\n"
    "    out.color = template_pos.x * color_a\n"
    "             + template_pos.y * color_b\n"
    "             + template_pos.z * color_c;\n"
    "    if (params.lighting_enabled > 0.5) {\n"
    "        let N = normalize(normal);\n"
    "        var diffuse = vec3<f32>(0.0);\n"
    "        let n = i32(params.num_lights);\n"
    "        for (var i: i32 = 0; i < n; i++) {\n"
    "            let L = normalize(-params.light_dirs[i].xyz);\n"
    "            let NdotL = abs(dot(N, L));\n"
    "            diffuse += params.light_colors[i].xyz * params.light_colors[i].w * NdotL;\n"
    "        }\n"
    "        let ambient = params.ambient_color.xyz * params.ambient_color.w;\n"
    "        out.lighting = ambient + diffuse;\n"
    "    } else {\n"
    "        out.lighting = vec3<f32>(1.0);\n"
    "    }\n"
    "    return out;\n"
    "}\n";

static const char* instanced_triangle_fs_source =
    "@fragment\n"
    "fn fs_main(@location(0) color: vec4<f32>, @location(1) lighting: vec3<f32>) -> @location(0) vec4<f32> {\n"
    "    return vec4<f32>(color.rgb * lighting, color.a);\n"
    "}\n";

//------------------------------------------------------------------------------
// Vulkan SPIR-V bytecode
// NOTE: SPIR-V bytecode must be recompiled after shader source changes.
// Run: scripts/vulkan-win/build-all.ps1
//------------------------------------------------------------------------------
#elif defined(SOKOL_VULKAN)

#include "spirv/spirv_bytecode.h"

// Bytecode pointers and sizes for Vulkan
static const uint8_t* instanced_triangle_vs_bytecode = instanced_triangle_vs_spirv;
static const size_t instanced_triangle_vs_bytecode_size = sizeof(instanced_triangle_vs_spirv);
static const uint8_t* instanced_triangle_fs_bytecode = instanced_triangle_fs_spirv;
static const size_t instanced_triangle_fs_bytecode_size = sizeof(instanced_triangle_fs_spirv);

//------------------------------------------------------------------------------
// DirectX 11 HLSL
//------------------------------------------------------------------------------
#elif defined(SOKOL_D3D11)

static const char* instanced_triangle_vs_source =
    "cbuffer vs_params : register(b0) {\n"
    "    float4x4 mvp;\n"
    "    float4 light_dirs[4];\n"
    "    float4 light_colors[4];\n"
    "    float4 ambient_color;\n"
    "    float num_lights;\n"
    "    float lighting_enabled;\n"
    "    float2 _pad;\n"
    "};\n"
    "struct vs_in {\n"
    "    float3 template_pos : POSITION;\n"
    "    float3 vertex_a : TEXCOORD0;\n"
    "    float3 vertex_b : TEXCOORD1;\n"
    "    float3 vertex_c : TEXCOORD2;\n"
    "    float3 normal : TEXCOORD3;\n"
    "    float4 color_a : TEXCOORD4;\n"
    "    float4 color_b : TEXCOORD5;\n"
    "    float4 color_c : TEXCOORD6;\n"
    "};\n"
    "struct vs_out {\n"
    "    float4 color : COLOR;\n"
    "    float3 lighting : TEXCOORD0;\n"
    "    float4 pos : SV_Position;\n"
    "};\n"
    "vs_out vs_main(vs_in inp) {\n"
    "    vs_out outp;\n"
    "    float3 world_pos = inp.template_pos.x * inp.vertex_a\n"
    "                     + inp.template_pos.y * inp.vertex_b\n"
    "                     + inp.template_pos.z * inp.vertex_c;\n"
    "    outp.pos = mul(mvp, float4(world_pos, 1.0));\n"
    "    outp.color = inp.template_pos.x * inp.color_a\n"
    "              + inp.template_pos.y * inp.color_b\n"
    "              + inp.template_pos.z * inp.color_c;\n"
    "    if (lighting_enabled > 0.5) {\n"
    "        float3 N = normalize(inp.normal);\n"
    "        float3 diffuse = float3(0.0, 0.0, 0.0);\n"
    "        int n = (int)num_lights;\n"
    "        for (int i = 0; i < n; i++) {\n"
    "            float3 L = normalize(-light_dirs[i].xyz);\n"
    "            float NdotL = abs(dot(N, L));\n"
    "            diffuse += light_colors[i].rgb * light_colors[i].w * NdotL;\n"
    "        }\n"
    "        float3 ambient = ambient_color.rgb * ambient_color.w;\n"
    "        outp.lighting = ambient + diffuse;\n"
    "    } else {\n"
    "        outp.lighting = float3(1.0, 1.0, 1.0);\n"
    "    }\n"
    "    return outp;\n"
    "}\n";

static const char* instanced_triangle_fs_source =
    "struct fs_in {\n"
    "    float4 color : COLOR;\n"
    "    float3 lighting : TEXCOORD0;\n"
    "};\n"
    "float4 fs_main(fs_in inp) : SV_Target0 {\n"
    "    return float4(inp.color.rgb * inp.lighting, inp.color.a);\n"
    "}\n";

#else
#error "Unknown graphics backend - define SOKOL_GLCORE, SOKOL_GLES3, SOKOL_METAL, SOKOL_WGPU, SOKOL_VULKAN, or SOKOL_D3D11"
#endif

#endif // INSTANCED_TRIANGLE_SHADERS_H
