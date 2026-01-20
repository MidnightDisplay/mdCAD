//------------------------------------------------------------------------------
// line_shaders.h - Line shader sources for all backends (header-only)
//
// Simple vertex-colored line shaders for dynamic line rendering.
// Vertex format: position (vec3) + color (vec4)
//------------------------------------------------------------------------------
#ifndef LINE_SHADERS_H
#define LINE_SHADERS_H

//------------------------------------------------------------------------------
// OpenGL 3.3 GLSL
//------------------------------------------------------------------------------
#if defined(SOKOL_GLCORE)

static const char* line_vs_source =
    "#version 330\n"
    "uniform mat4 mvp;\n"
    "layout(location=0) in vec3 position;\n"
    "layout(location=1) in vec4 color;\n"
    "out vec4 v_color;\n"
    "void main() {\n"
    "    gl_Position = mvp * vec4(position, 1.0);\n"
    "    v_color = color;\n"
    "}\n";

static const char* line_fs_source =
    "#version 330\n"
    "in vec4 v_color;\n"
    "out vec4 frag_color;\n"
    "void main() {\n"
    "    frag_color = v_color;\n"
    "}\n";

//------------------------------------------------------------------------------
// Metal Shading Language
//------------------------------------------------------------------------------
#elif defined(SOKOL_METAL)

static const char* line_vs_source =
    "#include <metal_stdlib>\n"
    "using namespace metal;\n"
    "struct vs_in {\n"
    "    float3 position [[attribute(0)]];\n"
    "    float4 color [[attribute(1)]];\n"
    "};\n"
    "struct vs_out {\n"
    "    float4 pos [[position]];\n"
    "    float4 color;\n"
    "};\n"
    "struct vs_params {\n"
    "    float4x4 mvp;\n"
    "};\n"
    "vertex vs_out vs_main(vs_in in [[stage_in]], constant vs_params& params [[buffer(0)]]) {\n"
    "    vs_out out;\n"
    "    out.pos = params.mvp * float4(in.position, 1.0);\n"
    "    out.color = in.color;\n"
    "    return out;\n"
    "}\n";

static const char* line_fs_source =
    "#include <metal_stdlib>\n"
    "using namespace metal;\n"
    "struct fs_in {\n"
    "    float4 color;\n"
    "};\n"
    "fragment float4 fs_main(fs_in in [[stage_in]]) {\n"
    "    return in.color;\n"
    "}\n";

//------------------------------------------------------------------------------
// WebGPU WGSL
//------------------------------------------------------------------------------
#elif defined(SOKOL_WGPU)

static const char* line_vs_source =
    "struct vs_params {\n"
    "    mvp: mat4x4<f32>,\n"
    "};\n"
    "@group(0) @binding(0) var<uniform> params: vs_params;\n"
    "struct vs_out {\n"
    "    @builtin(position) pos: vec4<f32>,\n"
    "    @location(0) color: vec4<f32>,\n"
    "};\n"
    "@vertex\n"
    "fn vs_main(@location(0) position: vec3<f32>, @location(1) color: vec4<f32>) -> vs_out {\n"
    "    var out: vs_out;\n"
    "    out.pos = params.mvp * vec4<f32>(position, 1.0);\n"
    "    out.color = color;\n"
    "    return out;\n"
    "}\n";

static const char* line_fs_source =
    "@fragment\n"
    "fn fs_main(@location(0) color: vec4<f32>) -> @location(0) vec4<f32> {\n"
    "    return color;\n"
    "}\n";

//------------------------------------------------------------------------------
// DirectX 11 HLSL
//------------------------------------------------------------------------------
#elif defined(SOKOL_D3D11)

static const char* line_vs_source =
    "cbuffer vs_params : register(b0) {\n"
    "    float4x4 mvp;\n"
    "};\n"
    "struct vs_in {\n"
    "    float3 position : POSITION;\n"
    "    float4 color : COLOR;\n"
    "};\n"
    "struct vs_out {\n"
    "    float4 color : COLOR;\n"
    "    float4 pos : SV_Position;\n"
    "};\n"
    "vs_out main(vs_in inp) {\n"
    "    vs_out outp;\n"
    "    outp.pos = mul(mvp, float4(inp.position, 1.0));\n"
    "    outp.color = inp.color;\n"
    "    return outp;\n"
    "}\n";

static const char* line_fs_source =
    "struct fs_in {\n"
    "    float4 color : COLOR;\n"
    "};\n"
    "float4 main(fs_in inp) : SV_Target0 {\n"
    "    return inp.color;\n"
    "}\n";

#else
#error "Unknown graphics backend - define SOKOL_GLCORE, SOKOL_METAL, SOKOL_WGPU, or SOKOL_D3D11"
#endif

#endif // LINE_SHADERS_H
