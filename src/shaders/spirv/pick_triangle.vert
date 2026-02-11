#version 450

layout(binding=0) uniform vs_params {
    mat4 mvp;
};

layout(location=0) in vec3 template_pos;
layout(location=1) in vec3 vertex_a;
layout(location=2) in vec3 vertex_b;
layout(location=3) in vec3 vertex_c;
layout(location=4) in vec3 pick_color;

layout(location=0) out vec3 v_pick_color;

void main() {
    vec3 world_pos = template_pos.x * vertex_a
                   + template_pos.y * vertex_b
                   + template_pos.z * vertex_c;
    gl_Position = mvp * vec4(world_pos, 1.0);
    v_pick_color = pick_color;
}
