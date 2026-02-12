#version 450

layout(binding=0) uniform vs_params {
    mat4 mvp;
    vec4 light_dirs[4];
    vec4 light_colors[4];
    vec4 ambient_color;
    float num_lights;
    float lighting_enabled;
    float _pad0;
    float _pad1;
};

layout(location=0) in vec3 template_pos;
layout(location=1) in vec3 vertex_a;
layout(location=2) in vec3 vertex_b;
layout(location=3) in vec3 vertex_c;
layout(location=4) in vec3 normal;
layout(location=5) in vec4 color_a;
layout(location=6) in vec4 color_b;
layout(location=7) in vec4 color_c;

layout(location=0) out vec4 v_color;
layout(location=1) out vec3 v_lighting;

void main() {
    vec3 world_pos = template_pos.x * vertex_a
                   + template_pos.y * vertex_b
                   + template_pos.z * vertex_c;
    gl_Position = mvp * vec4(world_pos, 1.0);
    v_color = template_pos.x * color_a
            + template_pos.y * color_b
            + template_pos.z * color_c;

    if (lighting_enabled > 0.5) {
        vec3 N = normalize(normal);
        vec3 diffuse = vec3(0.0);
        int n = int(num_lights);
        for (int i = 0; i < n; i++) {
            vec3 L = normalize(-light_dirs[i].xyz);
            float NdotL = abs(dot(N, L));
            diffuse += light_colors[i].rgb * light_colors[i].w * NdotL;
        }
        vec3 ambient = ambient_color.rgb * ambient_color.w;
        v_lighting = ambient + diffuse;
    } else {
        v_lighting = vec3(1.0);
    }
}
