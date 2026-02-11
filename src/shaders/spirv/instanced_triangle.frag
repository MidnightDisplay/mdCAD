#version 450

layout(location=0) in vec4 v_color;
layout(location=1) in vec3 v_lighting;

layout(location=0) out vec4 frag_color;

void main() {
    frag_color = vec4(v_color.rgb * v_lighting, v_color.a);
}
