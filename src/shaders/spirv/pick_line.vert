#version 450

layout(binding=0) uniform vs_params {
    mat4 mvp;
    float line_width;
    float aspect_ratio;
};

layout(location=0) in vec3 template_pos;
layout(location=1) in vec3 point_a;
layout(location=2) in vec3 point_b;
layout(location=3) in vec3 pick_color;

layout(location=0) out vec3 v_pick_color;

void main() {
    vec4 clip_a = mvp * vec4(point_a, 1.0);
    vec4 clip_b = mvp * vec4(point_b, 1.0);
    vec2 ndc_a = clip_a.xy / clip_a.w;
    vec2 ndc_b = clip_b.xy / clip_b.w;
    vec2 dir = ndc_b - ndc_a;
    float len = length(dir);
    if (len < 0.0001) dir = vec2(1.0, 0.0);
    else dir = dir / len;
    vec2 perp = vec2(-dir.y, dir.x);
    perp.x /= aspect_ratio;
    dir.x /= aspect_ratio;
    vec4 base_clip = mix(clip_a, clip_b, template_pos.z);
    vec2 offset = dir * template_pos.x + perp * template_pos.y;
    offset *= line_width;
    gl_Position = base_clip;
    gl_Position.xy += offset * base_clip.w;
    v_pick_color = pick_color;
}
