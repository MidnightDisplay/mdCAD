#version 450

layout(location=0) in vec3 v_pick_color;
layout(location=0) out vec4 frag_color;

void main() {
    frag_color = vec4(v_pick_color, 1.0);
}
