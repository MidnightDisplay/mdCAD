//------------------------------------------------------------------------------
// pick_shaders.h - GPU picking shaders for all backends (header-only)
//
// Renders geometry with pick_id encoded as RGB color for entity selection.
// Uses same vertex transformation as visual shaders but outputs pick color.
//
// Line Vertex attributes:
//   - location 0: template_pos (vec3) - x=along dir, y=perpendicular, z=endpoint selector
//   - location 1: point_a (vec3) - line start in world space
//   - location 2: point_b (vec3) - line end in world space
//   - location 3: pick_color (vec3) - RGB encoded pick ID (0-1 range)
//
// Point Vertex attributes:
//   - location 0: template_pos (vec3) - x,y = circle offset, z = unused
//   - location 1: point (vec3) - center in world space
//   - location 2: pick_color (vec3) - RGB encoded pick ID (0-1 range)
//
// Uniforms:
//   - mvp (mat4) - model-view-projection matrix
//   - line_width (float) - line/point size in NDC units
//   - aspect_ratio (float) - viewport width/height
//------------------------------------------------------------------------------
#ifndef PICK_SHADERS_H
#define PICK_SHADERS_H

//------------------------------------------------------------------------------
// OpenGL 3.3 GLSL
//------------------------------------------------------------------------------
#if defined(SOKOL_GLCORE)

// Line pick shader
static const char* pick_line_vs_source =
    "#version 330\n"
    "uniform mat4 mvp;\n"
    "uniform float line_width;\n"
    "uniform float aspect_ratio;\n"
    "layout(location=0) in vec3 template_pos;\n"
    "layout(location=1) in vec3 point_a;\n"
    "layout(location=2) in vec3 point_b;\n"
    "layout(location=3) in vec3 pick_color;\n"
    "out vec3 v_pick_color;\n"
    "void main() {\n"
    "    vec4 clip_a = mvp * vec4(point_a, 1.0);\n"
    "    vec4 clip_b = mvp * vec4(point_b, 1.0);\n"
    "    vec2 ndc_a = clip_a.xy / clip_a.w;\n"
    "    vec2 ndc_b = clip_b.xy / clip_b.w;\n"
    "    vec2 dir = ndc_b - ndc_a;\n"
    "    float len = length(dir);\n"
    "    if (len < 0.0001) dir = vec2(1.0, 0.0);\n"
    "    else dir = dir / len;\n"
    "    vec2 perp = vec2(-dir.y, dir.x);\n"
    "    perp.x /= aspect_ratio;\n"
    "    dir.x /= aspect_ratio;\n"
    "    vec4 base_clip = mix(clip_a, clip_b, template_pos.z);\n"
    "    vec2 offset = dir * template_pos.x + perp * template_pos.y;\n"
    "    offset *= line_width;\n"
    "    gl_Position = base_clip;\n"
    "    gl_Position.xy += offset * base_clip.w;\n"
    "    v_pick_color = pick_color;\n"
    "}\n";

static const char* pick_line_fs_source =
    "#version 330\n"
    "in vec3 v_pick_color;\n"
    "out vec4 frag_color;\n"
    "void main() {\n"
    "    frag_color = vec4(v_pick_color, 1.0);\n"
    "}\n";

// Point pick shader
static const char* pick_point_vs_source =
    "#version 330\n"
    "uniform mat4 mvp;\n"
    "uniform float line_width;\n"
    "uniform float aspect_ratio;\n"
    "layout(location=0) in vec3 template_pos;\n"
    "layout(location=1) in vec3 point;\n"
    "layout(location=2) in vec3 pick_color;\n"
    "out vec3 v_pick_color;\n"
    "void main() {\n"
    "    vec4 clip_p = mvp * vec4(point, 1.0);\n"
    "    vec2 offset = vec2(template_pos.x / aspect_ratio, template_pos.y) * line_width;\n"
    "    gl_Position = clip_p;\n"
    "    gl_Position.xy += offset * clip_p.w;\n"
    "    v_pick_color = pick_color;\n"
    "}\n";

static const char* pick_point_fs_source =
    "#version 330\n"
    "in vec3 v_pick_color;\n"
    "out vec4 frag_color;\n"
    "void main() {\n"
    "    frag_color = vec4(v_pick_color, 1.0);\n"
    "}\n";

//------------------------------------------------------------------------------
// OpenGL ES 3.0 GLSL (Android)
//------------------------------------------------------------------------------
#elif defined(SOKOL_GLES3)

// Line pick shader
static const char* pick_line_vs_source =
    "#version 300 es\n"
    "precision highp float;\n"
    "precision highp int;\n"
    "uniform mat4 mvp;\n"
    "uniform float line_width;\n"
    "uniform float aspect_ratio;\n"
    "layout(location=0) in vec3 template_pos;\n"
    "layout(location=1) in vec3 point_a;\n"
    "layout(location=2) in vec3 point_b;\n"
    "layout(location=3) in vec3 pick_color;\n"
    "out vec3 v_pick_color;\n"
    "void main() {\n"
    "    vec4 clip_a = mvp * vec4(point_a, 1.0);\n"
    "    vec4 clip_b = mvp * vec4(point_b, 1.0);\n"
    "    vec2 ndc_a = clip_a.xy / clip_a.w;\n"
    "    vec2 ndc_b = clip_b.xy / clip_b.w;\n"
    "    vec2 dir = ndc_b - ndc_a;\n"
    "    float len = length(dir);\n"
    "    if (len < 0.0001) dir = vec2(1.0, 0.0);\n"
    "    else dir = dir / len;\n"
    "    vec2 perp = vec2(-dir.y, dir.x);\n"
    "    perp.x /= aspect_ratio;\n"
    "    dir.x /= aspect_ratio;\n"
    "    vec4 base_clip = mix(clip_a, clip_b, template_pos.z);\n"
    "    vec2 offset = dir * template_pos.x + perp * template_pos.y;\n"
    "    offset *= line_width;\n"
    "    gl_Position = base_clip;\n"
    "    gl_Position.xy += offset * base_clip.w;\n"
    "    v_pick_color = pick_color;\n"
    "}\n";

static const char* pick_line_fs_source =
    "#version 300 es\n"
    "precision highp float;\n"
    "in vec3 v_pick_color;\n"
    "out vec4 frag_color;\n"
    "void main() {\n"
    "    frag_color = vec4(v_pick_color, 1.0);\n"
    "}\n";

// Point pick shader
static const char* pick_point_vs_source =
    "#version 300 es\n"
    "precision highp float;\n"
    "precision highp int;\n"
    "uniform mat4 mvp;\n"
    "uniform float line_width;\n"
    "uniform float aspect_ratio;\n"
    "layout(location=0) in vec3 template_pos;\n"
    "layout(location=1) in vec3 point;\n"
    "layout(location=2) in vec3 pick_color;\n"
    "out vec3 v_pick_color;\n"
    "void main() {\n"
    "    vec4 clip_p = mvp * vec4(point, 1.0);\n"
    "    vec2 offset = vec2(template_pos.x / aspect_ratio, template_pos.y) * line_width;\n"
    "    gl_Position = clip_p;\n"
    "    gl_Position.xy += offset * clip_p.w;\n"
    "    v_pick_color = pick_color;\n"
    "}\n";

static const char* pick_point_fs_source =
    "#version 300 es\n"
    "precision highp float;\n"
    "in vec3 v_pick_color;\n"
    "out vec4 frag_color;\n"
    "void main() {\n"
    "    frag_color = vec4(v_pick_color, 1.0);\n"
    "}\n";

//------------------------------------------------------------------------------
// Metal Shading Language
//------------------------------------------------------------------------------
#elif defined(SOKOL_METAL)

// Line pick shader
static const char* pick_line_vs_source =
    "#include <metal_stdlib>\n"
    "using namespace metal;\n"
    "struct vs_in {\n"
    "    float3 template_pos [[attribute(0)]];\n"
    "    float3 point_a [[attribute(1)]];\n"
    "    float3 point_b [[attribute(2)]];\n"
    "    float3 pick_color [[attribute(3)]];\n"
    "};\n"
    "struct vs_out {\n"
    "    float4 pos [[position]];\n"
    "    float3 pick_color;\n"
    "};\n"
    "struct vs_params {\n"
    "    float4x4 mvp;\n"
    "    float line_width;\n"
    "    float aspect_ratio;\n"
    "};\n"
    "vertex vs_out vs_main(vs_in in [[stage_in]], constant vs_params& params [[buffer(0)]]) {\n"
    "    vs_out out;\n"
    "    float4 clip_a = params.mvp * float4(in.point_a, 1.0);\n"
    "    float4 clip_b = params.mvp * float4(in.point_b, 1.0);\n"
    "    float2 ndc_a = clip_a.xy / clip_a.w;\n"
    "    float2 ndc_b = clip_b.xy / clip_b.w;\n"
    "    float2 dir = ndc_b - ndc_a;\n"
    "    float len = length(dir);\n"
    "    if (len < 0.0001) dir = float2(1.0, 0.0);\n"
    "    else dir = dir / len;\n"
    "    float2 perp = float2(-dir.y, dir.x);\n"
    "    perp.x /= params.aspect_ratio;\n"
    "    dir.x /= params.aspect_ratio;\n"
    "    float4 base_clip = mix(clip_a, clip_b, in.template_pos.z);\n"
    "    float2 offset = dir * in.template_pos.x + perp * in.template_pos.y;\n"
    "    offset *= params.line_width;\n"
    "    out.pos = base_clip;\n"
    "    out.pos.xy += offset * base_clip.w;\n"
    "    out.pick_color = in.pick_color;\n"
    "    return out;\n"
    "}\n";

static const char* pick_line_fs_source =
    "#include <metal_stdlib>\n"
    "using namespace metal;\n"
    "struct fs_in {\n"
    "    float3 pick_color;\n"
    "};\n"
    "fragment float4 fs_main(fs_in in [[stage_in]]) {\n"
    "    return float4(in.pick_color, 1.0);\n"
    "}\n";

// Point pick shader
static const char* pick_point_vs_source =
    "#include <metal_stdlib>\n"
    "using namespace metal;\n"
    "struct vs_in {\n"
    "    float3 template_pos [[attribute(0)]];\n"
    "    float3 point [[attribute(1)]];\n"
    "    float3 pick_color [[attribute(2)]];\n"
    "};\n"
    "struct vs_out {\n"
    "    float4 pos [[position]];\n"
    "    float3 pick_color;\n"
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
    "    out.pick_color = in.pick_color;\n"
    "    return out;\n"
    "};\n";

static const char* pick_point_fs_source =
    "#include <metal_stdlib>\n"
    "using namespace metal;\n"
    "struct fs_in {\n"
    "    float3 pick_color;\n"
    "};\n"
    "fragment float4 fs_main(fs_in in [[stage_in]]) {\n"
    "    return float4(in.pick_color, 1.0);\n"
    "}\n";

//------------------------------------------------------------------------------
// WebGPU WGSL
//------------------------------------------------------------------------------
#elif defined(SOKOL_WGPU)

// Line pick shader
static const char* pick_line_vs_source =
    "struct vs_params {\n"
    "    mvp: mat4x4<f32>,\n"
    "    line_width: f32,\n"
    "    aspect_ratio: f32,\n"
    "};\n"
    "@group(0) @binding(0) var<uniform> params: vs_params;\n"
    "struct vs_out {\n"
    "    @builtin(position) pos: vec4<f32>,\n"
    "    @location(0) pick_color: vec3<f32>,\n"
    "};\n"
    "@vertex\n"
    "fn vs_main(\n"
    "    @location(0) template_pos: vec3<f32>,\n"
    "    @location(1) point_a: vec3<f32>,\n"
    "    @location(2) point_b: vec3<f32>,\n"
    "    @location(3) pick_color: vec3<f32>\n"
    ") -> vs_out {\n"
    "    var out: vs_out;\n"
    "    let clip_a = params.mvp * vec4<f32>(point_a, 1.0);\n"
    "    let clip_b = params.mvp * vec4<f32>(point_b, 1.0);\n"
    "    let ndc_a = clip_a.xy / clip_a.w;\n"
    "    let ndc_b = clip_b.xy / clip_b.w;\n"
    "    var dir = ndc_b - ndc_a;\n"
    "    let len = length(dir);\n"
    "    if (len < 0.0001) {\n"
    "        dir = vec2<f32>(1.0, 0.0);\n"
    "    } else {\n"
    "        dir = dir / len;\n"
    "    }\n"
    "    var perp = vec2<f32>(-dir.y, dir.x);\n"
    "    perp.x = perp.x / params.aspect_ratio;\n"
    "    dir.x = dir.x / params.aspect_ratio;\n"
    "    let base_clip = mix(clip_a, clip_b, template_pos.z);\n"
    "    var offset = dir * template_pos.x + perp * template_pos.y;\n"
    "    offset = offset * params.line_width;\n"
    "    out.pos = base_clip;\n"
    "    out.pos.x = out.pos.x + offset.x * base_clip.w;\n"
    "    out.pos.y = out.pos.y + offset.y * base_clip.w;\n"
    "    out.pick_color = pick_color;\n"
    "    return out;\n"
    "}\n";

static const char* pick_line_fs_source =
    "@fragment\n"
    "fn fs_main(@location(0) pick_color: vec3<f32>) -> @location(0) vec4<f32> {\n"
    "    return vec4<f32>(pick_color, 1.0);\n"
    "}\n";

// Point pick shader
static const char* pick_point_vs_source =
    "struct vs_params {\n"
    "    mvp: mat4x4<f32>,\n"
    "    line_width: f32,\n"
    "    aspect_ratio: f32,\n"
    "};\n"
    "@group(0) @binding(0) var<uniform> params: vs_params;\n"
    "struct vs_out {\n"
    "    @builtin(position) pos: vec4<f32>,\n"
    "    @location(0) pick_color: vec3<f32>,\n"
    "};\n"
    "@vertex\n"
    "fn vs_main(\n"
    "    @location(0) template_pos: vec3<f32>,\n"
    "    @location(1) point: vec3<f32>,\n"
    "    @location(2) pick_color: vec3<f32>\n"
    ") -> vs_out {\n"
    "    var out: vs_out;\n"
    "    let clip_p = params.mvp * vec4<f32>(point, 1.0);\n"
    "    let offset = vec2<f32>(template_pos.x / params.aspect_ratio, template_pos.y) * params.line_width;\n"
    "    out.pos = clip_p;\n"
    "    out.pos.x = out.pos.x + offset.x * clip_p.w;\n"
    "    out.pos.y = out.pos.y + offset.y * clip_p.w;\n"
    "    out.pick_color = pick_color;\n"
    "    return out;\n"
    "}\n";

static const char* pick_point_fs_source =
    "@fragment\n"
    "fn fs_main(@location(0) pick_color: vec3<f32>) -> @location(0) vec4<f32> {\n"
    "    return vec4<f32>(pick_color, 1.0);\n"
    "}\n";

//------------------------------------------------------------------------------
// DirectX 11 HLSL
//------------------------------------------------------------------------------
#elif defined(SOKOL_D3D11)

// Line pick shader
static const char* pick_line_vs_source =
    "cbuffer vs_params : register(b0) {\n"
    "    float4x4 mvp;\n"
    "    float line_width;\n"
    "    float aspect_ratio;\n"
    "};\n"
    "struct vs_in {\n"
    "    float3 template_pos : POSITION;\n"
    "    float3 point_a : TEXCOORD0;\n"
    "    float3 point_b : TEXCOORD1;\n"
    "    float3 pick_color : COLOR;\n"
    "};\n"
    "struct vs_out {\n"
    "    float3 pick_color : COLOR;\n"
    "    float4 pos : SV_Position;\n"
    "};\n"
    "vs_out main(vs_in inp) {\n"
    "    vs_out outp;\n"
    "    float4 clip_a = mul(mvp, float4(inp.point_a, 1.0));\n"
    "    float4 clip_b = mul(mvp, float4(inp.point_b, 1.0));\n"
    "    float2 ndc_a = clip_a.xy / clip_a.w;\n"
    "    float2 ndc_b = clip_b.xy / clip_b.w;\n"
    "    float2 dir = ndc_b - ndc_a;\n"
    "    float len = length(dir);\n"
    "    if (len < 0.0001) dir = float2(1.0, 0.0);\n"
    "    else dir = dir / len;\n"
    "    float2 perp = float2(-dir.y, dir.x);\n"
    "    perp.x /= aspect_ratio;\n"
    "    dir.x /= aspect_ratio;\n"
    "    float4 base_clip = lerp(clip_a, clip_b, inp.template_pos.z);\n"
    "    float2 offset = dir * inp.template_pos.x + perp * inp.template_pos.y;\n"
    "    offset *= line_width;\n"
    "    outp.pos = base_clip;\n"
    "    outp.pos.xy += offset * base_clip.w;\n"
    "    outp.pick_color = inp.pick_color;\n"
    "    return outp;\n"
    "}\n";

static const char* pick_line_fs_source =
    "struct fs_in {\n"
    "    float3 pick_color : COLOR;\n"
    "};\n"
    "float4 main(fs_in inp) : SV_Target0 {\n"
    "    return float4(inp.pick_color, 1.0);\n"
    "}\n";

// Point pick shader
static const char* pick_point_vs_source =
    "cbuffer vs_params : register(b0) {\n"
    "    float4x4 mvp;\n"
    "    float line_width;\n"
    "    float aspect_ratio;\n"
    "};\n"
    "struct vs_in {\n"
    "    float3 template_pos : POSITION;\n"
    "    float3 point : TEXCOORD0;\n"
    "    float3 pick_color : COLOR;\n"
    "};\n"
    "struct vs_out {\n"
    "    float3 pick_color : COLOR;\n"
    "    float4 pos : SV_Position;\n"
    "};\n"
    "vs_out main(vs_in inp) {\n"
    "    vs_out outp;\n"
    "    float4 clip_p = mul(mvp, float4(inp.point, 1.0));\n"
    "    float2 offset = float2(inp.template_pos.x / aspect_ratio, inp.template_pos.y) * line_width;\n"
    "    outp.pos = clip_p;\n"
    "    outp.pos.xy += offset * clip_p.w;\n"
    "    outp.pick_color = inp.pick_color;\n"
    "    return outp;\n"
    "}\n";

static const char* pick_point_fs_source =
    "struct fs_in {\n"
    "    float3 pick_color : COLOR;\n"
    "};\n"
    "float4 main(fs_in inp) : SV_Target0 {\n"
    "    return float4(inp.pick_color, 1.0);\n"
    "}\n";

#else
#error "Unknown graphics backend - define SOKOL_GLCORE, SOKOL_GLES3, SOKOL_METAL, SOKOL_WGPU, or SOKOL_D3D11"
#endif

#endif // PICK_SHADERS_H
