//------------------------------------------------------------------------------
// Sokol + cimgui demo with dockspace and 3D viewport rendering a cube
//------------------------------------------------------------------------------

// Define backend before includes (same as sokol.c)
#if defined(_WIN32)
#define SOKOL_D3D11
#elif defined(__EMSCRIPTEN__)
#define SOKOL_WGPU
#elif defined(__APPLE__)
#define SOKOL_METAL
#else
#define SOKOL_GLCORE
#endif

#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_log.h"
#include "sokol_glue.h"
#include "sokol_time.h"
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"
#include "sokol_imgui.h"
#include <math.h>
#include <string.h>

//------------------------------------------------------------------------------
// Simple math helpers
//------------------------------------------------------------------------------
typedef struct { float x, y, z; } vec3_t;
typedef struct { float m[16]; } mat4_t;

static mat4_t mat4_identity(void) {
    mat4_t m = {0};
    m.m[0] = m.m[5] = m.m[10] = m.m[15] = 1.0f;
    return m;
}

static mat4_t mat4_mul(mat4_t a, mat4_t b) {
    mat4_t result = {0};
    for (int col = 0; col < 4; col++) {
        for (int row = 0; row < 4; row++) {
            result.m[col * 4 + row] =
                a.m[0 * 4 + row] * b.m[col * 4 + 0] +
                a.m[1 * 4 + row] * b.m[col * 4 + 1] +
                a.m[2 * 4 + row] * b.m[col * 4 + 2] +
                a.m[3 * 4 + row] * b.m[col * 4 + 3];
        }
    }
    return result;
}

static mat4_t mat4_perspective(float fovy, float aspect, float near, float far) {
    mat4_t m = {0};
    float f = 1.0f / tanf(fovy * 0.5f);
    m.m[0] = f / aspect;
    m.m[5] = f;
    m.m[10] = (far + near) / (near - far);
    m.m[11] = -1.0f;
    m.m[14] = (2.0f * far * near) / (near - far);
    return m;
}

static mat4_t mat4_lookat(vec3_t eye, vec3_t target, vec3_t up) {
    // Forward (from target to eye for RH coordinate system)
    vec3_t f = {
        target.x - eye.x,
        target.y - eye.y,
        target.z - eye.z
    };
    float f_len = sqrtf(f.x*f.x + f.y*f.y + f.z*f.z);
    f.x /= f_len; f.y /= f_len; f.z /= f_len;

    // Right = f x up
    vec3_t r = {
        f.y * up.z - f.z * up.y,
        f.z * up.x - f.x * up.z,
        f.x * up.y - f.y * up.x
    };
    float r_len = sqrtf(r.x*r.x + r.y*r.y + r.z*r.z);
    r.x /= r_len; r.y /= r_len; r.z /= r_len;

    // True up = r x f
    vec3_t u = {
        r.y * f.z - r.z * f.y,
        r.z * f.x - r.x * f.z,
        r.x * f.y - r.y * f.x
    };

    mat4_t m = mat4_identity();
    m.m[0] = r.x;  m.m[4] = r.y;  m.m[8]  = r.z;
    m.m[1] = u.x;  m.m[5] = u.y;  m.m[9]  = u.z;
    m.m[2] = -f.x; m.m[6] = -f.y; m.m[10] = -f.z;
    m.m[12] = -(r.x * eye.x + r.y * eye.y + r.z * eye.z);
    m.m[13] = -(u.x * eye.x + u.y * eye.y + u.z * eye.z);
    m.m[14] = (f.x * eye.x + f.y * eye.y + f.z * eye.z);
    return m;
}

static mat4_t mat4_rotate_y(float angle) {
    mat4_t m = mat4_identity();
    float c = cosf(angle);
    float s = sinf(angle);
    m.m[0] = c;   m.m[8] = s;
    m.m[2] = -s;  m.m[10] = c;
    return m;
}

//------------------------------------------------------------------------------
// Cube vertex data: position (vec3) + normal (vec3)
//------------------------------------------------------------------------------
typedef struct {
    float px, py, pz;   // position
    float nx, ny, nz;   // normal
} vertex_t;

// Cube centered at origin, size 1.0
static const vertex_t cube_vertices[] = {
    // Front face (z = +0.5), normal (0, 0, 1)
    { -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f },
    {  0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f },
    {  0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f },
    { -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f },
    // Back face (z = -0.5), normal (0, 0, -1)
    {  0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f },
    { -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f },
    { -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f },
    {  0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f },
    // Right face (x = +0.5), normal (1, 0, 0)
    {  0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f },
    {  0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f },
    {  0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f },
    {  0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f },
    // Left face (x = -0.5), normal (-1, 0, 0)
    { -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f },
    { -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f },
    { -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f },
    { -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f },
    // Top face (y = +0.5), normal (0, 1, 0)
    { -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f },
    {  0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f },
    {  0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f },
    { -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f },
    // Bottom face (y = -0.5), normal (0, -1, 0)
    { -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f },
    {  0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f },
    {  0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f },
    { -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f },
};

static const uint16_t cube_indices[] = {
    // CCW winding when viewed from outside (for correct backface culling)
    0,  2,  1,   0,  3,  2,   // front
    4,  6,  5,   4,  7,  6,   // back
    8,  10, 9,   8,  11, 10,  // right
    12, 14, 13,  12, 15, 14,  // left
    16, 18, 17,  16, 19, 18,  // top
    20, 22, 21,  20, 23, 22,  // bottom
};

//------------------------------------------------------------------------------
// Shader sources
//------------------------------------------------------------------------------
#if defined(SOKOL_GLCORE)
// OpenGL 3.3 GLSL
static const char* vs_source =
    "#version 330\n"
    "uniform mat4 mvp;\n"
    "layout(location=0) in vec3 position;\n"
    "layout(location=1) in vec3 normal;\n"
    "out vec3 v_normal;\n"
    "void main() {\n"
    "    gl_Position = mvp * vec4(position, 1.0);\n"
    "    v_normal = normal;\n"
    "}\n";

static const char* fs_source =
    "#version 330\n"
    "in vec3 v_normal;\n"
    "out vec4 frag_color;\n"
    "void main() {\n"
    "    // Map normal from [-1,1] to [0,1] for RGB color\n"
    "    vec3 color = v_normal * 0.5 + 0.5;\n"
    "    frag_color = vec4(color, 1.0);\n"
    "}\n";

#elif defined(SOKOL_METAL)
// Metal Shading Language
static const char* vs_source =
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

static const char* fs_source =
    "#include <metal_stdlib>\n"
    "using namespace metal;\n"
    "struct fs_in {\n"
    "    float3 normal;\n"
    "};\n"
    "fragment float4 fs_main(fs_in in [[stage_in]]) {\n"
    "    float3 color = in.normal * 0.5 + 0.5;\n"
    "    return float4(color, 1.0);\n"
    "}\n";

#elif defined(SOKOL_WGPU)
// WebGPU WGSL
static const char* vs_source =
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

static const char* fs_source =
    "@fragment\n"
    "fn fs_main(@location(0) normal: vec3<f32>) -> @location(0) vec4<f32> {\n"
    "    let color = normal * 0.5 + 0.5;\n"
    "    return vec4<f32>(color, 1.0);\n"
    "}\n";

#elif defined(SOKOL_D3D11)
// HLSL
static const char* vs_source =
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

static const char* fs_source =
    "struct fs_in {\n"
    "    float3 normal : NORMAL;\n"
    "};\n"
    "float4 main(fs_in inp) : SV_Target0 {\n"
    "    float3 color = inp.normal * 0.5 + 0.5;\n"
    "    return float4(color, 1.0);\n"
    "}\n";

#else
#error "Unknown graphics backend"
#endif

//------------------------------------------------------------------------------
// Application state
//------------------------------------------------------------------------------
#define OFFSCREEN_WIDTH 512
#define OFFSCREEN_HEIGHT 512

typedef struct {
    mat4_t mvp;
} vs_params_t;

static struct {
    // Offscreen rendering
    sg_image color_img;
    sg_image depth_img;
    sg_view color_att_view;
    sg_view depth_att_view;
    sg_view tex_view;
    sg_pass_action offscreen_pass_action;
    sg_pipeline pip;
    sg_bindings bind;
    sg_sampler sampler;

    // Main pass
    sg_pass_action main_pass_action;

    // State
    float clear_color[3];
    float rotation;
    uint64_t last_time;

    // Viewport size tracking
    int viewport_width;
    int viewport_height;
} state;

//------------------------------------------------------------------------------
// Recreate offscreen resources when viewport size changes
//------------------------------------------------------------------------------
static void create_offscreen_resources(int width, int height) {
    // Destroy old resources if they exist
    if (state.color_img.id) {
        sg_destroy_view(state.color_att_view);
        sg_destroy_view(state.depth_att_view);
        sg_destroy_view(state.tex_view);
        sg_destroy_image(state.color_img);
        sg_destroy_image(state.depth_img);
    }

    // Create color render target
    state.color_img = sg_make_image(&(sg_image_desc){
        .usage.color_attachment = true,
        .width = width,
        .height = height,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .sample_count = 1,
        .label = "offscreen-color"
    });

    // Create depth buffer
    state.depth_img = sg_make_image(&(sg_image_desc){
        .usage.depth_stencil_attachment = true,
        .width = width,
        .height = height,
        .pixel_format = SG_PIXELFORMAT_DEPTH,
        .sample_count = 1,
        .label = "offscreen-depth"
    });

    // Create color attachment view
    state.color_att_view = sg_make_view(&(sg_view_desc){
        .color_attachment.image = state.color_img,
        .label = "color-att-view"
    });

    // Create depth attachment view
    state.depth_att_view = sg_make_view(&(sg_view_desc){
        .depth_stencil_attachment.image = state.depth_img,
        .label = "depth-att-view"
    });

    // Create texture view for sampling in ImGui
    state.tex_view = sg_make_view(&(sg_view_desc){
        .texture.image = state.color_img,
        .label = "tex-view"
    });

    state.viewport_width = width;
    state.viewport_height = height;
}

//------------------------------------------------------------------------------
// Init
//------------------------------------------------------------------------------
static void init(void) {
    stm_setup();
    state.last_time = stm_now();

    sg_setup(&(sg_desc){
        .environment = sglue_environment(),
        .logger.func = slog_func,
    });

    // Setup ImGui with docking enabled
    simgui_setup(&(simgui_desc_t){
        .logger.func = slog_func,
    });

    // Enable docking
    ImGuiIO* io = igGetIO_Nil();
    io->ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // Initial clear color (cornflower blue)
    state.clear_color[0] = 0.39f;
    state.clear_color[1] = 0.58f;
    state.clear_color[2] = 0.93f;

    // Create initial offscreen resources
    create_offscreen_resources(OFFSCREEN_WIDTH, OFFSCREEN_HEIGHT);

    // Offscreen pass action
    state.offscreen_pass_action = (sg_pass_action){
        .colors[0] = {
            .load_action = SG_LOADACTION_CLEAR,
            .clear_value = { state.clear_color[0], state.clear_color[1], state.clear_color[2], 1.0f }
        },
        .depth = { .load_action = SG_LOADACTION_CLEAR, .clear_value = 1.0f }
    };

    // Main pass action (just clear to dark gray)
    state.main_pass_action = (sg_pass_action){
        .colors[0] = { .load_action = SG_LOADACTION_CLEAR, .clear_value = { 0.1f, 0.1f, 0.1f, 1.0f } }
    };

    // Create vertex buffer
    sg_buffer vbuf = sg_make_buffer(&(sg_buffer_desc){
        .usage.vertex_buffer = true,
        .data = SG_RANGE(cube_vertices),
        .label = "cube-vertices"
    });

    // Create index buffer
    sg_buffer ibuf = sg_make_buffer(&(sg_buffer_desc){
        .usage.index_buffer = true,
        .data = SG_RANGE(cube_indices),
        .label = "cube-indices"
    });

    // Create sampler for ImGui to display the render target
    state.sampler = sg_make_sampler(&(sg_sampler_desc){
        .min_filter = SG_FILTER_LINEAR,
        .mag_filter = SG_FILTER_LINEAR,
        .label = "viewport-sampler"
    });

    // Create shader
    sg_shader shd = sg_make_shader(&(sg_shader_desc){
        .vertex_func = {
            .source = vs_source,
            .entry = "vs_main",  // WGSL entry point (ignored on other backends)
        },
        .fragment_func = {
            .source = fs_source,
            .entry = "fs_main",  // WGSL entry point (ignored on other backends)
        },
        .uniform_blocks[0] = {
            .stage = SG_SHADERSTAGE_VERTEX,
            .size = sizeof(vs_params_t),
            .layout = SG_UNIFORMLAYOUT_STD140,
            .glsl_uniforms[0] = { .type = SG_UNIFORMTYPE_MAT4, .glsl_name = "mvp" }
        },
        .label = "cube-shader"
    });

    // Create pipeline
    state.pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = shd,
        .layout = {
            .attrs = {
                [0] = { .format = SG_VERTEXFORMAT_FLOAT3 },  // position
                [1] = { .format = SG_VERTEXFORMAT_FLOAT3 }   // normal
            }
        },
        .index_type = SG_INDEXTYPE_UINT16,
        .cull_mode = SG_CULLMODE_BACK,
        .depth = {
            .compare = SG_COMPAREFUNC_LESS_EQUAL,
            .write_enabled = true,
            .pixel_format = SG_PIXELFORMAT_DEPTH
        },
        .colors[0].pixel_format = SG_PIXELFORMAT_RGBA8,
        .label = "cube-pipeline"
    });

    // Setup bindings
    state.bind = (sg_bindings){
        .vertex_buffers[0] = vbuf,
        .index_buffer = ibuf
    };

    state.rotation = 0.0f;
}

//------------------------------------------------------------------------------
// Frame
//------------------------------------------------------------------------------
static void frame(void) {
    // Calculate delta time
    uint64_t now = stm_now();
    float dt = (float)stm_sec(stm_diff(now, state.last_time));
    state.last_time = now;

    // Rotate cube slowly
    state.rotation += dt * 0.5f;

    const int width = sapp_width();
    const int height = sapp_height();

    simgui_new_frame(&(simgui_frame_desc_t){
        .width = width,
        .height = height,
        .delta_time = sapp_frame_duration(),
        .dpi_scale = sapp_dpi_scale(),
    });

    //=== UI CODE ===

    // Create dockspace over the entire viewport
    igDockSpaceOverViewport(0, NULL, ImGuiDockNodeFlags_None, NULL);

    // Control window
    igBegin("Controls", NULL, ImGuiWindowFlags_None);
    igText("3D Viewport Settings");
    igSeparator();
    if (igColorEdit3("Clear Color", state.clear_color, ImGuiColorEditFlags_None)) {
        // Update pass action when color changes
        state.offscreen_pass_action.colors[0].clear_value.r = state.clear_color[0];
        state.offscreen_pass_action.colors[0].clear_value.g = state.clear_color[1];
        state.offscreen_pass_action.colors[0].clear_value.b = state.clear_color[2];
    }
    igText("Rotation: %.2f rad", state.rotation);
    if (igButton("Reset Rotation", (ImVec2){0, 0})) {
        state.rotation = 0.0f;
    }
    igEnd();

    // 3D Viewport window
    igPushStyleVar_Vec2(ImGuiStyleVar_WindowPadding, (ImVec2){0, 0});
    igBegin("3D Viewport", NULL, ImGuiWindowFlags_None);

    // Get available content region size
    ImVec2 content_size = igGetContentRegionAvail();
    int vp_width = (int)content_size.x;
    int vp_height = (int)content_size.y;

    // Ensure minimum size
    if (vp_width < 64) vp_width = 64;
    if (vp_height < 64) vp_height = 64;

    // Recreate offscreen resources if size changed
    if (vp_width != state.viewport_width || vp_height != state.viewport_height) {
        create_offscreen_resources(vp_width, vp_height);
    }

    // Display the rendered image using ImGui
    uint64_t tex_id = simgui_imtextureid_with_sampler(state.tex_view, state.sampler);
    ImTextureRef_c tex_ref = { ._TexID = tex_id };
    igImage(tex_ref, (ImVec2_c){(float)vp_width, (float)vp_height},
            (ImVec2_c){0, 0}, (ImVec2_c){1, 1});

    igEnd();
    igPopStyleVar(1);

    //=== RENDER CUBE TO OFFSCREEN TARGET ===

    // Calculate MVP matrix for isometric-like view
    // Camera positioned at 45-degree angle to see 3 faces of the cube
    float cam_dist = 3.0f;
    float cam_height = 2.0f;
    float cam_angle = 0.785398f; // 45 degrees
    vec3_t eye = {
        cam_dist * sinf(cam_angle),
        cam_height,
        cam_dist * cosf(cam_angle)
    };
    vec3_t target = { 0.0f, 0.0f, 0.0f };
    vec3_t up = { 0.0f, 1.0f, 0.0f };

    mat4_t view = mat4_lookat(eye, target, up);
    mat4_t proj = mat4_perspective(0.785398f, (float)vp_width / (float)vp_height, 0.1f, 100.0f);
    mat4_t model = mat4_rotate_y(state.rotation);
    mat4_t vp_mat = mat4_mul(proj, view);
    mat4_t mvp = mat4_mul(vp_mat, model);

    vs_params_t vs_params = { .mvp = mvp };

    // Offscreen pass - render the cube
    sg_begin_pass(&(sg_pass){
        .action = state.offscreen_pass_action,
        .attachments = {
            .colors[0] = state.color_att_view,
            .depth_stencil = state.depth_att_view,
        }
    });
    sg_apply_pipeline(state.pip);
    sg_apply_bindings(&state.bind);
    sg_apply_uniforms(0, &SG_RANGE(vs_params));
    sg_draw(0, 36, 1);
    sg_end_pass();

    //=== MAIN PASS - RENDER IMGUI ===
    sg_begin_pass(&(sg_pass){
        .swapchain = sglue_swapchain(),
        .action = state.main_pass_action
    });
    simgui_render();
    sg_end_pass();

    sg_commit();
}

//------------------------------------------------------------------------------
// Cleanup
//------------------------------------------------------------------------------
static void cleanup(void) {
    sg_destroy_view(state.color_att_view);
    sg_destroy_view(state.depth_att_view);
    sg_destroy_view(state.tex_view);
    sg_destroy_image(state.color_img);
    sg_destroy_image(state.depth_img);
    simgui_shutdown();
    sg_shutdown();
}

//------------------------------------------------------------------------------
// Event handling
//------------------------------------------------------------------------------
static void event(const sapp_event* ev) {
    simgui_handle_event(ev);
}

//------------------------------------------------------------------------------
// Main entry point
//------------------------------------------------------------------------------
sapp_desc sokol_main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    return (sapp_desc){
        .init_cb = init,
        .frame_cb = frame,
        .cleanup_cb = cleanup,
        .event_cb = event,
        .window_title = "Sokol Cube Viewport",
        .width = 1280,
        .height = 720,
        .icon.sokol_default = true,
        .logger.func = slog_func,
    };
}
