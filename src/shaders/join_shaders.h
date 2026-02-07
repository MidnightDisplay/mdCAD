//------------------------------------------------------------------------------
// join_shaders.h - Circle join shaders for instanced polylines (header-only)
//
// Renders filled circles at polyline vertices for round joins.
// Uses GPU instancing: template circle geometry + per-instance point/color.
//
// Vertex attributes:
//   - location 0: template_pos (vec3) - x,y = circle offset, z = unused
//   - location 1: point (vec3) - join center in world space
//   - location 2: color (vec4) - RGBA color
//
// Uniforms:
//   - mvp (mat4) - model-view-projection matrix
//   - line_width (float) - circle diameter in NDC units
//   - aspect_ratio (float) - viewport width/height for circular geometry
//------------------------------------------------------------------------------
#ifndef JOIN_SHADERS_H
#define JOIN_SHADERS_H

//------------------------------------------------------------------------------
// OpenGL 3.3 GLSL
//------------------------------------------------------------------------------
#if defined(SOKOL_GLCORE)

static const char* join_vs_source =
    "#version 330\n"
    "uniform mat4 mvp;\n"
    "uniform float line_width;\n"
    "uniform float aspect_ratio;\n"
    "layout(location=0) in vec3 template_pos;\n"
    "layout(location=1) in vec3 point;\n"
    "layout(location=2) in vec4 color;\n"
    "out vec4 v_color;\n"
    "void main() {\n"
    "    vec4 clip_p = mvp * vec4(point, 1.0);\n"
    "    vec2 offset = vec2(template_pos.x / aspect_ratio, template_pos.y) * line_width;\n"
    "    gl_Position = clip_p;\n"
    "    gl_Position.xy += offset * clip_p.w;\n"
    "    v_color = color;\n"
    "}\n";

static const char* join_fs_source =
    "#version 330\n"
    "in vec4 v_color;\n"
    "out vec4 frag_color;\n"
    "void main() {\n"
    "    frag_color = v_color;\n"
    "}\n";

//------------------------------------------------------------------------------
// OpenGL ES 3.0 GLSL (Android)
//------------------------------------------------------------------------------
#elif defined(SOKOL_GLES3)

static const char* join_vs_source =
    "#version 300 es\n"
    "precision highp float;\n"
    "precision highp int;\n"
    "uniform mat4 mvp;\n"
    "uniform float line_width;\n"
    "uniform float aspect_ratio;\n"
    "layout(location=0) in vec3 template_pos;\n"
    "layout(location=1) in vec3 point;\n"
    "layout(location=2) in vec4 color;\n"
    "out vec4 v_color;\n"
    "void main() {\n"
    "    vec4 clip_p = mvp * vec4(point, 1.0);\n"
    "    vec2 offset = vec2(template_pos.x / aspect_ratio, template_pos.y) * line_width;\n"
    "    gl_Position = clip_p;\n"
    "    gl_Position.xy += offset * clip_p.w;\n"
    "    v_color = color;\n"
    "}\n";

static const char* join_fs_source =
    "#version 300 es\n"
    "precision highp float;\n"
    "in vec4 v_color;\n"
    "out vec4 frag_color;\n"
    "void main() {\n"
    "    frag_color = v_color;\n"
    "}\n";

//------------------------------------------------------------------------------
// Metal Shading Language
//------------------------------------------------------------------------------
#elif defined(SOKOL_METAL)

static const char* join_vs_source =
    "#include <metal_stdlib>\n"
    "using namespace metal;\n"
    "struct vs_in {\n"
    "    float3 template_pos [[attribute(0)]];\n"
    "    float3 point [[attribute(1)]];\n"
    "    float4 color [[attribute(2)]];\n"
    "};\n"
    "struct vs_out {\n"
    "    float4 pos [[position]];\n"
    "    float4 color;\n"
    "};\n"
    "struct vs_params {\n"
    "    float4x4 mvp;\n"
    "    float line_width;\n"
    "    float aspect_ratio;\n"
    "};\n"
    "vertex vs_out vs_main(vs_in in [[stage_in]], constant vs_params& params [[buffer(0)]]) {\n"
    "    vs_out out;\n"
    "    float4 clip_p = params.mvp * float4(in.point, 1.0);\n"
    "    float2 offset = float2(in.template_pos.x / params.aspect_ratio, in.template_pos.y) * params.line_width;\n"
    "    out.pos = clip_p;\n"
    "    out.pos.xy += offset * clip_p.w;\n"
    "    out.color = in.color;\n"
    "    return out;\n"
    "};\n";

static const char* join_fs_source =
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

static const char* join_vs_source =
    "struct vs_params {\n"
    "    mvp: mat4x4<f32>,\n"
    "    line_width: f32,\n"
    "    aspect_ratio: f32,\n"
    "};\n"
    "@group(0) @binding(0) var<uniform> params: vs_params;\n"
    "struct vs_out {\n"
    "    @builtin(position) pos: vec4<f32>,\n"
    "    @location(0) color: vec4<f32>,\n"
    "};\n"
    "@vertex\n"
    "fn vs_main(\n"
    "    @location(0) template_pos: vec3<f32>,\n"
    "    @location(1) point: vec3<f32>,\n"
    "    @location(2) color: vec4<f32>\n"
    ") -> vs_out {\n"
    "    var out: vs_out;\n"
    "    let clip_p = params.mvp * vec4<f32>(point, 1.0);\n"
    "    let offset = vec2<f32>(template_pos.x / params.aspect_ratio, template_pos.y) * params.line_width;\n"
    "    out.pos = clip_p;\n"
    "    out.pos.x = out.pos.x + offset.x * clip_p.w;\n"
    "    out.pos.y = out.pos.y + offset.y * clip_p.w;\n"
    "    out.color = color;\n"
    "    return out;\n"
    "}\n";

static const char* join_fs_source =
    "@fragment\n"
    "fn fs_main(@location(0) color: vec4<f32>) -> @location(0) vec4<f32> {\n"
    "    return color;\n"
    "}\n";

//------------------------------------------------------------------------------
// DirectX 11 HLSL
//------------------------------------------------------------------------------
#elif defined(SOKOL_D3D11)

static const char* join_vs_source =
    "cbuffer vs_params : register(b0) {\n"
    "    float4x4 mvp;\n"
    "    float line_width;\n"
    "    float aspect_ratio;\n"
    "};\n"
    "struct vs_in {\n"
    "    float3 template_pos : POSITION;\n"
    "    float3 point : TEXCOORD0;\n"
    "    float4 color : COLOR;\n"
    "};\n"
    "struct vs_out {\n"
    "    float4 color : COLOR;\n"
    "    float4 pos : SV_Position;\n"
    "};\n"
    "vs_out vs_main(vs_in inp) {\n"
    "    vs_out outp;\n"
    "    float4 clip_p = mul(mvp, float4(inp.point, 1.0));\n"
    "    float2 offset = float2(inp.template_pos.x / aspect_ratio, inp.template_pos.y) * line_width;\n"
    "    outp.pos = clip_p;\n"
    "    outp.pos.xy += offset * clip_p.w;\n"
    "    outp.color = inp.color;\n"
    "    return outp;\n"
    "}\n";

static const char* join_fs_source =
    "struct fs_in {\n"
    "    float4 color : COLOR;\n"
    "};\n"
    "float4 fs_main(fs_in inp) : SV_Target0 {\n"
    "    return inp.color;\n"
    "}\n";

#else
#error "Unknown graphics backend - define SOKOL_GLCORE, SOKOL_GLES3, SOKOL_METAL, SOKOL_WGPU, or SOKOL_D3D11"
#endif

#endif // JOIN_SHADERS_H
