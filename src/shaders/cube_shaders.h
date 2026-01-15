//------------------------------------------------------------------------------
// cube_shaders.h - Cube shader sources for all backends (header-only)
//------------------------------------------------------------------------------
#ifndef CUBE_SHADERS_H
#define CUBE_SHADERS_H

//------------------------------------------------------------------------------
// OpenGL 3.3 GLSL
//------------------------------------------------------------------------------
#if defined(SOKOL_GLCORE)

static const char* cube_vs_source =
    "#version 330\n"
    "uniform mat4 mvp;\n"
    "layout(location=0) in vec3 position;\n"
    "layout(location=1) in vec3 normal;\n"
    "out vec3 v_normal;\n"
    "void main() {\n"
    "    gl_Position = mvp * vec4(position, 1.0);\n"
    "    v_normal = normal;\n"
    "}\n";

static const char* cube_fs_source =
    "#version 330\n"
    "in vec3 v_normal;\n"
    "out vec4 frag_color;\n"
    "void main() {\n"
    "    // Map normal from [-1,1] to [0,1] for RGB color\n"
    "    vec3 color = v_normal * 0.5 + 0.5;\n"
    "    frag_color = vec4(color, 1.0);\n"
    "}\n";

//------------------------------------------------------------------------------
// Metal Shading Language
//------------------------------------------------------------------------------
#elif defined(SOKOL_METAL)

static const char* cube_vs_source =
    "#include <metal_stdlib>\n"
    "using namespace metal;\n"
    "struct vs_in {\n"
    "    float3 position [[attribute(0)]];\n"
    "    float3 normal [[attribute(1)]];\n"
    "};\n"
    "struct vs_out {\n"
    "    float4 pos [[position]];\n"
    "    float3 normal;\n"
    "};\n"
    "struct vs_params {\n"
    "    float4x4 mvp;\n"
    "};\n"
    "vertex vs_out vs_main(vs_in in [[stage_in]], constant vs_params& params [[buffer(0)]]) {\n"
    "    vs_out out;\n"
    "    out.pos = params.mvp * float4(in.position, 1.0);\n"
    "    out.normal = in.normal;\n"
    "    return out;\n"
    "}\n";

static const char* cube_fs_source =
    "#include <metal_stdlib>\n"
    "using namespace metal;\n"
    "struct fs_in {\n"
    "    float3 normal;\n"
    "};\n"
    "fragment float4 fs_main(fs_in in [[stage_in]]) {\n"
    "    float3 color = in.normal * 0.5 + 0.5;\n"
    "    return float4(color, 1.0);\n"
    "}\n";

//------------------------------------------------------------------------------
// WebGPU WGSL
//------------------------------------------------------------------------------
#elif defined(SOKOL_WGPU)

static const char* cube_vs_source =
    "struct vs_params {\n"
    "    mvp: mat4x4<f32>,\n"
    "};\n"
    "@group(0) @binding(0) var<uniform> params: vs_params;\n"
    "struct vs_out {\n"
    "    @builtin(position) pos: vec4<f32>,\n"
    "    @location(0) normal: vec3<f32>,\n"
    "};\n"
    "@vertex\n"
    "fn vs_main(@location(0) position: vec3<f32>, @location(1) normal: vec3<f32>) -> vs_out {\n"
    "    var out: vs_out;\n"
    "    out.pos = params.mvp * vec4<f32>(position, 1.0);\n"
    "    out.normal = normal;\n"
    "    return out;\n"
    "}\n";

static const char* cube_fs_source =
    "@fragment\n"
    "fn fs_main(@location(0) normal: vec3<f32>) -> @location(0) vec4<f32> {\n"
    "    let color = normal * 0.5 + 0.5;\n"
    "    return vec4<f32>(color, 1.0);\n"
    "}\n";

//------------------------------------------------------------------------------
// DirectX 11 HLSL
//------------------------------------------------------------------------------
#elif defined(SOKOL_D3D11)

static const char* cube_vs_source =
    "cbuffer vs_params : register(b0) {\n"
    "    float4x4 mvp;\n"
    "};\n"
    "struct vs_in {\n"
    "    float3 position : POSITION;\n"
    "    float3 normal : NORMAL;\n"
    "};\n"
    "struct vs_out {\n"
    "    float3 normal : NORMAL;\n"
    "    float4 pos : SV_Position;\n"
    "};\n"
    "vs_out main(vs_in inp) {\n"
    "    vs_out outp;\n"
    "    outp.pos = mul(mvp, float4(inp.position, 1.0));\n"
    "    outp.normal = inp.normal;\n"
    "    return outp;\n"
    "}\n";

static const char* cube_fs_source =
    "struct fs_in {\n"
    "    float3 normal : NORMAL;\n"
    "};\n"
    "float4 main(fs_in inp) : SV_Target0 {\n"
    "    float3 color = inp.normal * 0.5 + 0.5;\n"
    "    return float4(color, 1.0);\n"
    "}\n";

#else
#error "Unknown graphics backend - define SOKOL_GLCORE, SOKOL_METAL, SOKOL_WGPU, or SOKOL_D3D11"
#endif

#endif // CUBE_SHADERS_H
