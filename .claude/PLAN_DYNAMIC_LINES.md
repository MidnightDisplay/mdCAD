# Implementation Plan: Dynamic Lines System

## Overview

Render 10,000 dynamic lines within a spherical volume, with sin-wave wiggle animation and random colors. Uses Sokol best practices for efficient dynamic buffer updates.

## Data Architecture

### Line Data (CPU-side, persistent)
For each of the 10,000 lines:
- **Base position** (vec3): Random point within sphere
- **Direction** (vec3): Normalized random direction for line endpoint offset
- **Motion direction** (vec3): Normalized direction for sin-wave wiggle
- **Phase offset** (float): Random phase [0, 2π] so lines don't move in sync
- **Length** (float): Line half-length

### Vertex Buffer Data (GPU, updated every frame)
- 20,000 vertices (2 per line)
- Each vertex: position (vec3) + color (vec4) = 28 bytes
- Total: ~560 KB per frame upload

### Color Buffer Strategy
Colors are baked into vertex data (interleaved) for simplicity and single draw call.

## Sokol Buffer Configuration

```c
// Vertex buffer - STREAM usage for frequent CPU updates
sg_buffer vbuf = sg_make_buffer(&(sg_buffer_desc){
    .usage.vertex_buffer = true,
    .usage.stream_usage = true,  // Key: optimized for frequent updates
    .size = NUM_LINES * 2 * sizeof(line_vertex_t),
    .label = "lines-vertices"
});
```

Using `stream_usage` tells Sokol this buffer will be updated frequently via `sg_update_buffer()`.

## Module Structure

### 1. `src/shaders/line_shaders.h`
Multi-backend shaders (GLSL/MSL/WGSL/HLSL):
- **Vertex**: Takes position (vec3) + color (vec4), outputs position + color
- **Fragment**: Outputs interpolated color
- **Uniform**: MVP matrix only

### 2. `src/dynamic_lines.h`
Header-only module with:
```c
#define LINES_COUNT 10000
#define LINES_SPHERE_RADIUS 5.0f
#define LINES_WIGGLE_AMPLITUDE 0.3f
#define LINES_WIGGLE_FREQUENCY 2.0f

typedef struct {
    vec3_t base_pos;         // Center of line
    vec3_t direction;        // Line endpoint direction
    vec3_t motion_dir;       // Wiggle direction
    float phase;             // Sin wave phase offset
    float length;            // Half-length
    vec3_t color;            // RGB color
} line_data_t;

typedef struct {
    // GPU resources
    sg_pipeline pip;
    sg_buffer vbuf;
    sg_shader shd;

    // CPU data
    line_data_t lines[LINES_COUNT];
    line_vertex_t vertices[LINES_COUNT * 2];  // Updated each frame

    // Parameters (for UI control)
    float sphere_radius;
    float wiggle_amplitude;
    float wiggle_frequency;
    float line_length;
} dynamic_lines_t;

// API
void dynamic_lines_init(dynamic_lines_t* dl);
void dynamic_lines_regenerate_positions(dynamic_lines_t* dl);
void dynamic_lines_regenerate_colors(dynamic_lines_t* dl);
void dynamic_lines_regenerate_directions(dynamic_lines_t* dl);
void dynamic_lines_update(dynamic_lines_t* dl, float time);  // Updates vertex buffer
void dynamic_lines_draw(dynamic_lines_t* dl, mat4_t mvp);
void dynamic_lines_shutdown(dynamic_lines_t* dl);
```

### 3. `src/ui/ui_lines_controls.h`
ImGui panel with:
- Regenerate All button
- Regenerate Positions button
- Regenerate Colors button
- Regenerate Motion button
- Sliders: sphere radius, wiggle amplitude, wiggle frequency, line length

## Animation Logic

Each frame in `dynamic_lines_update()`:
```c
for (int i = 0; i < LINES_COUNT; i++) {
    line_data_t* line = &dl->lines[i];

    // Calculate wiggle offset
    float wave = sinf(time * dl->wiggle_frequency + line->phase);
    vec3_t offset = {
        line->motion_dir.x * wave * dl->wiggle_amplitude,
        line->motion_dir.y * wave * dl->wiggle_amplitude,
        line->motion_dir.z * wave * dl->wiggle_amplitude
    };

    // Compute final positions for both endpoints
    vec3_t center = {
        line->base_pos.x + offset.x,
        line->base_pos.y + offset.y,
        line->base_pos.z + offset.z
    };

    // Endpoint 1
    dl->vertices[i * 2].pos = (vec3_t){
        center.x - line->direction.x * line->length,
        center.y - line->direction.y * line->length,
        center.z - line->direction.z * line->length
    };
    dl->vertices[i * 2].color = line->color;

    // Endpoint 2
    dl->vertices[i * 2 + 1].pos = (vec3_t){
        center.x + line->direction.x * line->length,
        center.y + line->direction.y * line->length,
        center.z + line->direction.z * line->length
    };
    dl->vertices[i * 2 + 1].color = line->color;
}

// Upload to GPU
sg_update_buffer(dl->vbuf, &(sg_range){
    .ptr = dl->vertices,
    .size = sizeof(dl->vertices)
});
```

## Pipeline Configuration

```c
sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
    .shader = shd,
    .layout = {
        .attrs = {
            [0] = { .format = SG_VERTEXFORMAT_FLOAT3 },  // position
            [1] = { .format = SG_VERTEXFORMAT_FLOAT4 }   // color (RGBA for alignment)
        }
    },
    .primitive_type = SG_PRIMITIVETYPE_LINES,  // Key: line primitive
    .depth = {
        .compare = SG_COMPAREFUNC_LESS_EQUAL,
        .write_enabled = true,
        .pixel_format = SG_PIXELFORMAT_DEPTH
    },
    .colors[0].pixel_format = SG_PIXELFORMAT_RGBA8,
    .label = "lines-pipeline"
});
```

## Random Number Generation

Simple LCG for deterministic seeds (allows reproducible regeneration):
```c
static uint32_t lines_rng_state = 12345;

static inline float lines_randf(void) {
    lines_rng_state = lines_rng_state * 1103515245 + 12345;
    return (float)(lines_rng_state & 0x7FFFFFFF) / (float)0x7FFFFFFF;
}

static inline vec3_t random_point_in_sphere(float radius) {
    // Rejection sampling for uniform distribution
    vec3_t p;
    do {
        p.x = (lines_randf() * 2.0f - 1.0f);
        p.y = (lines_randf() * 2.0f - 1.0f);
        p.z = (lines_randf() * 2.0f - 1.0f);
    } while (p.x*p.x + p.y*p.y + p.z*p.z > 1.0f);
    p.x *= radius; p.y *= radius; p.z *= radius;
    return p;
}

static inline vec3_t random_direction(void) {
    vec3_t d = random_point_in_sphere(1.0f);
    float len = sqrtf(d.x*d.x + d.y*d.y + d.z*d.z);
    if (len > 0.0001f) {
        d.x /= len; d.y /= len; d.z /= len;
    } else {
        d = (vec3_t){0, 1, 0};  // fallback
    }
    return d;
}
```

## Integration with demo.c

1. Add to state struct:
   ```c
   dynamic_lines_t lines;
   ui_lines_controls_state_t lines_controls;
   float elapsed_time;
   ```

2. In `init()`:
   ```c
   dynamic_lines_init(&state.lines);
   ui_lines_controls_init(&state.lines_controls, &state.lines);
   ```

3. In `frame()`:
   ```c
   state.elapsed_time += dt;
   dynamic_lines_update(&state.lines, state.elapsed_time);

   // In offscreen pass, after cube:
   dynamic_lines_draw(&state.lines, mvp);
   ```

4. In `cleanup()`:
   ```c
   dynamic_lines_shutdown(&state.lines);
   ```

## Files to Create/Modify

1. **Create**: `src/shaders/line_shaders.h` - Multi-backend line shaders
2. **Create**: `src/dynamic_lines.h` - Lines system module
3. **Create**: `src/ui/ui_lines_controls.h` - UI panel
4. **Modify**: `src/demo.c` - Integration

## Implementation Order

1. Create `line_shaders.h` with all backend variants
2. Create `dynamic_lines.h` with full implementation
3. Create `ui_lines_controls.h` for regeneration controls
4. Integrate into `demo.c`
5. Test native build
6. Test web build
