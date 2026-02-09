#version 450

layout(binding=0) uniform vs_params {
    mat4 mvp;
    float line_width;
    float aspect_ratio;
};

layout(location=0) in vec3 template_pos;
layout(location=1) in vec3 center_pt;
layout(location=2) in vec3 pick_color;

layout(location=0) out vec3 v_pick_color;

void main() {
    vec4 clip_p = mvp * vec4(center_pt, 1.0);
    vec2 offset = vec2(template_pos.x / aspect_ratio, template_pos.y) * line_width;
    gl_Position = clip_p;
    gl_Position.xy += offset * clip_p.w;
    v_pick_color = pick_color;
}
