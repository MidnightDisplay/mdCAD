# ECS Scene Management Implementation Plan

**Branch:** `ecs-scene` (branched from `main`)
**Created:** 2026-01-27
**Status:** Planning Phase - Ready to Begin

---

## Phase Overview

| Phase | Focus | Status |
|-------|-------|--------|
| 1 | Flecs Integration & Foundation | Complete |
| 2 | Dynamic Buffer Management | Complete |
| 3 | GPU Picking System | Complete |
| 4 | Selection System | Complete |
| 5 | ImGui Integration | Complete |
| 6 | Core Geometry Types | Complete |
| 7 | Entity Hierarchy | Complete |
| 8 | Scene Serialization | Complete |
| 9 | Undo/Redo | Complete |
| 10 | Polish & Performance (Future) | Planned |

---

## Progress Checklist

Use this checklist to track implementation progress across sessions.

### Phase 1: Foundation
- [x] 1.1 Download and integrate Flecs library into `vendors/flecs/`
- [x] 1.2 Create `vendors/flecs/CMakeLists.txt` and link to project
- [x] 1.3 Create `src/ecs/ecs_world.h` with world init/shutdown
- [x] 1.4 Create `src/components/transform_comp.h`
- [x] 1.5 Create `src/components/geometry_comp.h`
- [x] 1.6 Create `src/components/renderable_comp.h`
- [x] 1.7 Create `src/components/selectable_comp.h`
- [x] 1.8 Integrate ECS world into demo.c with test entities
- [x] 1.9 Verify native build works with ECS
- [x] 1.10 Verify web build works with ECS

### Phase 2: Dynamic Buffers
- [x] 2.1 Create `src/gpu/instance_buffer.h` with slot allocation
- [x] 2.2 Implement capacity-doubling growth strategy
- [x] 2.3 Create `src/gpu/geometry_batch.h` for per-type batching
- [x] 2.4 Create `src/ecs/ecs_scene.h` with entity creation API
- [x] 2.5 Implement entity deletion with slot/ID recycling
- [x] 2.6 Test runtime add/remove of entities

### Phase 3: GPU Picking
- [x] 3.1 Create `src/gpu/pick_buffer.h` with 20×20 render target
- [x] 3.2 Implement pick ID allocator with encoding/decoding (reused from selectable_comp.h)
- [x] 3.3 Create `src/shaders/pick_shaders.h` (all backends)
- [x] 3.4 Implement pick pipeline for lines
- [x] 3.5 Implement pick pipeline for points (circles)
- [x] 3.6 Implement CPU readback mechanism (Metal implemented, others placeholder)
- [x] 3.7 Implement hover detection (frontmost entity only)
- [x] 3.8 Create `src/ui/ui_pick_debug.h` debug visualization
- [x] 3.9 Fix pick buffer viewport alignment (use logical coords, not DPI-scaled)
- [x] 3.10 Fix line thickness in pick buffer (scale by zoom factor)
- [x] 3.11 Implement hover visual feedback (theme-aware color override)
- [x] 3.12 Add thickness controls to UI (line width, point size, pick multiplier)

### Phase 4: Selection System
- [x] 4.1 Create `src/selection.h` with selection buffer
- [x] 4.2 Implement click selection (replace)
- [x] 4.3 Implement Ctrl+click (toggle)
- [x] 4.4 Implement Shift+click (add to selection)
- [x] 4.5 Add Selected/Hovered tag components (already in ecs_world.h)
- [x] 4.6 Implement theme-aware color override for selection
- [x] 4.7 Implement theme-aware color override for hover (already in Phase 3)

### Phase 5: ImGui Integration
- [x] 5.1 Create `src/ui/ui_entity_inspector.h`
- [x] 5.2 Implement single-entity property editing
- [x] 5.3 Implement multi-selection display
- [x] 5.4 Create `src/ui/ui_scene_hierarchy.h`
- [x] 5.5 Add entity creation menu
- [x] 5.6 Add Delete key for entity deletion

### Phase 6: Core Geometry Types
- [x] 6.1 Implement point rendering (circle shader, same as joins) - Already done in Phase 2
- [x] 6.2 Implement polyline rendering with dynamic point allocation
- [x] 6.3 Implement arc rendering (polyline tessellation)
- [x] 6.4 Implement polygon rendering (outline mode)
- [x] 6.5 Implement helix rendering
- [x] 6.6 Implement bezier curve rendering (polyline tessellation)
- [x] 6.7 Add creation API for all geometry types (Scene Hierarchy menu)

**BUG: Invisible Segments (Phase 6) - FIXED**
- [x] 6.8 Fix invisible segments bug
  - **Symptom:** Some segments on entities, or entire entities, become invisible randomly
  - **Behavior:** Reappear when hovering over the geometry (hover triggers dirty flag → re-upload)
  - **Root cause:** Non-contiguous slot allocation from free list
    - When entities are deleted, their slots go to a LIFO free list
    - New polylines/arcs/etc. would get non-contiguous slots (e.g., [2,1,0] instead of [0,1,2])
    - Code stored only `first_segment_slot` and assumed contiguous slots
    - `ecs_scene_update()` used `first_segment_slot + i` which was wrong for non-contiguous slots
  - **Fix:** Added `instance_buffer_alloc_contiguous(n)` function that skips the free list and allocates n consecutive slots from the end of the buffer
    - Updated `scene_add_polyline`, `scene_add_polygon`, `scene_add_arc`, `scene_add_bezier`, `scene_add_helix` to use contiguous allocation

### Phase 7: Entity Hierarchy
- [x] 7.1 Enable Flecs parent-child relationships
- [x] 7.2 Add parent field to TransformComp for hierarchy tracking
- [x] 7.3 Update transform system to compute world matrices from hierarchy
- [x] 7.4 Add scene API for parenting (scene_set_parent, scene_get_children)
- [x] 7.5 Update scene hierarchy UI for tree view with expand/collapse
- [x] 7.6 Add drag-and-drop reparenting in hierarchy UI
- [x] 7.7 Test nested transforms (child moves with parent)
- [x] 7.8 Fix: Parent transform changes propagate to all descendants
- [x] 7.9 Fix: Deleting parent properly cleans up children's GPU resources
- [x] 7.10 Fix: Rotation/scale transforms apply correctly
- [x] 7.11 Fix: Reparenting subtree updates entire subtree immediately

### Phase 8: Scene Serialization (Complete)
- [x] 8.1 Design JSON scene format
- [x] 8.2 Implement scene save
- [x] 8.3 Implement scene load
- [x] 8.4 Add file browser UI for save/load dialogs

### Phase 9: Undo/Redo System (Complete)
- [x] 9.1 Design command pattern for entity operations
- [x] 9.2 Implement undo stack with fixed capacity
- [x] 9.3 Implement redo stack (cleared on new action)
- [x] 9.4 Integrate with property editing in inspector
- [x] 9.5 Add keyboard shortcuts (Ctrl+Z / Ctrl+Shift+Z)
- [x] 9.6 Add Edit menu with Undo/Redo options

### Phase 10: Polish & Performance (Future)
- [ ] 10.1 Implement frustum culling for entities
- [ ] 10.2 Implement LOD for distant polylines
- [ ] 10.3 Implement spatial hashing for efficient picking (optional)
- [ ] 10.4 Stress test with 100K+ entities
- [ ] 10.5 Profile and optimize hot paths
- [ ] 10.6 Periodic instance buffer compaction (optional)
  - Currently freed slots get far-away positions (GPU frustum-culled, essentially free)
  - If many entities deleted without creating new ones, slot count grows unbounded
  - Compaction would require reverse lookup (slot → entity) to update RenderableComp.instance_slot
  - Consider triggering when free_count exceeds threshold (e.g., 50% of count)

---

## Overview

This plan describes the implementation of an Entity Component System (ECS) based 3D scene management system using the [Flecs](https://github.com/SanderMertens/flecs) library. The system will:

1. Manage CAD-like wireframe geometry entities (points, lines, polylines, arcs, polygons, helices, bezier curves)
2. Render geometry using the existing GPU-instanced rendering infrastructure
3. Provide robust GPU-based picking for entity selection
4. Support multi-selection with a selection buffer
5. Enable entity inspection and property manipulation via ImGui
6. Use dynamic GPU buffer management for efficient memory use

---

## Part 1: Flecs Integration

### 1.1 Library Setup

**Add Flecs as a vendor library:**

```
vendors/
└── flecs/
    ├── flecs.h       # Single-header amalgamation
    └── flecs.c       # Single-file implementation
```

- Download from: https://github.com/SanderMertens/flecs/releases
- Use the single-header amalgamation for simplicity
- Create `vendors/flecs/CMakeLists.txt` to build as static library

**CMakeLists.txt addition:**
```cmake
add_subdirectory(vendors/flecs)
target_link_libraries(${PROJECT_NAME} flecs_static)
```

### 1.2 ECS World Initialization

Create `src/ecs_world.h` - header-only module:

```c
#ifndef ECS_WORLD_H
#define ECS_WORLD_H

#include "flecs.h"

// Global ECS world
typedef struct {
    ecs_world_t *world;
    // Component IDs (registered at init)
    ecs_entity_t TransformComp;
    ecs_entity_t GeometryComp;
    ecs_entity_t RenderableComp;
    ecs_entity_t SelectableComp;
    ecs_entity_t SelectedComp;      // Tag component
    ecs_entity_t HoveredComp;       // Tag component
} ecs_world_state_t;

static inline void ecs_world_init(ecs_world_state_t *s);
static inline void ecs_world_progress(ecs_world_state_t *s, float dt);
static inline void ecs_world_shutdown(ecs_world_state_t *s);

#endif
```

---

## Part 2: Component Design

### 2.1 Core Components

**Transform Component** (`src/components/transform_comp.h`):
```c
typedef struct {
    vec3_t position;      // World position
    vec3_t rotation;      // Euler angles (radians)
    vec3_t scale;         // Non-uniform scale
    mat4_t world_matrix;  // Cached world transform (computed by system)
    bool dirty;           // Needs recalculation
} TransformComp;
```

**Geometry Component** (`src/components/geometry_comp.h`):
```c
typedef enum {
    GEOM_POINT,
    GEOM_LINE,
    GEOM_POLYLINE,
    GEOM_ARC,
    GEOM_POLYGON,
    GEOM_HELIX,
    GEOM_BEZIER,        // Cubic bezier curve
    GEOM_TYPE_COUNT
} geometry_type_t;

typedef struct {
    geometry_type_t type;

    // Geometry data (union or variant per type)
    union {
        struct { vec3_t point; } point;
        struct { vec3_t a, b; } line;
        struct { vec3_t *points; int count; int capacity; } polyline;  // Dynamic allocation
        struct { vec3_t center; float radius; float start_angle, end_angle; vec3_t normal; } arc;
        struct { vec3_t *points; int count; bool closed; } polygon;    // Dynamic allocation
        struct { vec3_t axis_start, axis_end; float radius; float turns; int segments; } helix;
        struct { vec3_t p0, p1, p2, p3; int segments; } bezier;        // Cubic bezier (4 control points)
    } data;

    // Rendering properties
    float line_width;       // Thickness for lines/polylines
    float point_size;       // Size for points
    vec4_t color;           // RGBA color
} GeometryComp;
```

**Renderable Component** (`src/components/renderable_comp.h`):
```c
typedef struct {
    bool visible;
    int layer;                    // Render layer (for ordering)
    uint32_t batch_id;            // Which GPU batch this belongs to
    uint32_t instance_slot;       // Slot index in the instance buffer
    bool instance_dirty;          // Needs GPU buffer update
} RenderableComp;
```

**Selectable Component** (`src/components/selectable_comp.h`):
```c
typedef struct {
    uint32_t pick_id;             // Unique ID for GPU picking (1-16777215)
    bool pickable;                // Can be picked
} SelectableComp;
```

**Tag Components** (no data, just markers):
- `SelectedComp` - Entity is currently selected
- `HoveredComp` - Entity is under cursor

### 2.2 Pick ID Allocation

Implement a simple ID allocator for pick IDs:

```c
typedef struct {
    uint32_t next_id;             // Start at 1 (0 = no entity)
    uint32_t *free_list;          // Recycled IDs
    int free_count, free_capacity;
} pick_id_allocator_t;

uint32_t pick_id_alloc(pick_id_allocator_t *a);   // Get new ID
void pick_id_free(pick_id_allocator_t *a, uint32_t id);  // Return ID
```

**ID ↔ RGB Encoding:**
```c
// Encode entity pick_id to RGB (for rendering to pick buffer)
static inline void pick_id_to_rgb(uint32_t id, uint8_t *r, uint8_t *g, uint8_t *b) {
    *r = (id >> 16) & 0xFF;
    *g = (id >> 8) & 0xFF;
    *b = id & 0xFF;
}

// Decode RGB back to pick_id
static inline uint32_t rgb_to_pick_id(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
}
```

---

## Part 3: GPU Buffer Management

### 3.1 Dynamic Instance Buffer Strategy

Based on research ([Sokol streaming](https://github.com/floooh/sokol/issues/679), [OpenGL streaming](https://www.khronos.org/opengl/wiki/Buffer_Object_Streaming)), use a **capacity-doubling approach** with ring buffer semantics.

**Instance Buffer Manager** (`src/gpu/instance_buffer.h`):

```c
typedef struct {
    sg_buffer gpu_buffer;         // Sokol GPU buffer
    void *cpu_staging;            // CPU-side staging array

    size_t instance_size;         // sizeof(instance_t)
    int capacity;                 // Max instances (power of 2)
    int count;                    // Current used count

    // Slot management
    int *free_slots;              // Stack of free slot indices
    int free_count;

    bool needs_upload;            // Dirty flag for this frame
} instance_buffer_t;

// Initialize with starting capacity (e.g., 1024)
void instance_buffer_init(instance_buffer_t *ib, size_t instance_size, int initial_capacity);

// Allocate a slot, returns slot index (-1 if resize needed)
int instance_buffer_alloc_slot(instance_buffer_t *ib);

// Free a slot for reuse
void instance_buffer_free_slot(instance_buffer_t *ib, int slot);

// Update instance data at slot
void instance_buffer_set(instance_buffer_t *ib, int slot, const void *data);

// Upload dirty data to GPU (call once per frame before rendering)
void instance_buffer_upload(instance_buffer_t *ib);

// Grow buffer capacity (recreates GPU buffer)
void instance_buffer_grow(instance_buffer_t *ib);

void instance_buffer_shutdown(instance_buffer_t *ib);
```

### 3.2 Capacity Growth Strategy

```c
// Constants
#define INSTANCE_BUFFER_INITIAL_CAPACITY  1024
#define INSTANCE_BUFFER_MAX_CAPACITY      (1 << 20)  // 1M instances

void instance_buffer_grow(instance_buffer_t *ib) {
    int new_capacity = ib->capacity * 2;
    if (new_capacity > INSTANCE_BUFFER_MAX_CAPACITY) {
        // Error: buffer full
        return;
    }

    // 1. Allocate new CPU staging buffer
    void *new_staging = malloc(new_capacity * ib->instance_size);
    memcpy(new_staging, ib->cpu_staging, ib->capacity * ib->instance_size);
    free(ib->cpu_staging);
    ib->cpu_staging = new_staging;

    // 2. Destroy old GPU buffer, create new
    sg_destroy_buffer(ib->gpu_buffer);
    ib->gpu_buffer = sg_make_buffer(&(sg_buffer_desc){
        .usage.vertex_buffer = true,
        .usage.stream_update = true,
        .size = new_capacity * ib->instance_size,
    });

    // 3. Add new slots to free list
    ib->free_slots = realloc(ib->free_slots, new_capacity * sizeof(int));
    for (int i = ib->capacity; i < new_capacity; i++) {
        ib->free_slots[ib->free_count++] = i;
    }

    ib->capacity = new_capacity;
    ib->needs_upload = true;
}
```

### 3.3 Batching by Geometry Type

Create separate instance buffers for each geometry type (different instance layouts):

```c
typedef struct {
    // One buffer per geometry pipeline
    instance_buffer_t line_instances;      // line_instance_t (2 points + color)
    instance_buffer_t polyline_segments;   // segment instances
    instance_buffer_t polyline_joins;      // join instances
    instance_buffer_t point_instances;     // point_instance_t (1 point + size + color)
    // ... arc, polygon, helix as needed
} geometry_batch_manager_t;
```

---

## Part 4: GPU Picking System

### 4.1 Pick Render Target

Create a small offscreen render target (20×20 pixels recommended):

```c
typedef struct {
    sg_image color_img;           // RGBA8 for pick colors
    sg_image depth_img;           // Depth buffer
    sg_view color_att;
    sg_view depth_att;
    sg_view tex_view;             // For ImGui debug display
    sg_sampler sampler;

    int size;                     // 20×20
    vec2_t center;                // Center in screen coords (follows mouse)

    // CPU readback buffer
    uint8_t *pixel_data;          // size * size * 4 bytes

    // Current hovered entity
    uint32_t hovered_pick_id;
    ecs_entity_t hovered_entity;
} pick_buffer_t;

void pick_buffer_init(pick_buffer_t *pb, int size);
void pick_buffer_set_center(pick_buffer_t *pb, float screen_x, float screen_y);
void pick_buffer_render(pick_buffer_t *pb, /* scene data */);
void pick_buffer_readback(pick_buffer_t *pb);  // CPU readback
uint32_t pick_buffer_sample(pick_buffer_t *pb, int x, int y);  // Get pick_id at offset
void pick_buffer_shutdown(pick_buffer_t *pb);
```

### 4.2 Pick Shader

Create `src/shaders/pick_shaders.h`:

The pick shader is simpler than the visual shader - it just outputs the pick_id as RGB:

```c
// Vertex shader: Same as instanced_line but passes pick_id instead of color
// Fragment shader: Output pick_id as RGB, alpha = 1.0

// Per-instance: point_a, point_b, pick_id (as uint packed into float3)
```

**Pick Instance Data:**
```c
typedef struct {
    float ax, ay, az;    // Point A
    float bx, by, bz;    // Point B
    float pick_r, pick_g, pick_b;  // Encoded pick ID (0-1 range)
} line_pick_instance_t;
```

### 4.3 Picking Pipeline

Separate pick pipelines for each geometry type, using pick shaders:

```c
typedef struct {
    sg_pipeline line_pick_pip;
    sg_pipeline point_pick_pip;
    sg_pipeline polyline_pick_pip;
    // ... etc

    sg_shader pick_shd;
} pick_pipelines_t;
```

### 4.4 Pick Buffer Readback

**Platform-specific readback:**

- **Metal/WebGPU:** Use `sg_query_image_pixels()` (if available) or render to staging buffer
- **OpenGL/GLES:** `glReadPixels()`
- **Workaround:** Render pick buffer to CPU-readable staging texture

For Sokol, the simplest approach is using a CPU-readable image:

```c
// Create pick buffer with CPU access hint
sg_image pick_img = sg_make_image(&(sg_image_desc){
    .width = 20,
    .height = 20,
    .pixel_format = SG_PIXELFORMAT_RGBA8,
    .usage.render_attachment = true,
    .usage.cpu_readable = true,  // If supported
});
```

**Alternative: Async Readback Pattern:**
Use a ring buffer of pick textures to avoid GPU stalls:

```c
#define PICK_BUFFER_RING_SIZE 3
sg_image pick_images[PICK_BUFFER_RING_SIZE];
int pick_frame_index = 0;

// Each frame: render to pick_images[pick_frame_index]
// Read back from pick_images[(pick_frame_index + 2) % 3] (2 frames old)
```

### 4.5 Debug Visualization

Display pick buffer in ImGui for debugging:

```c
void ui_pick_debug_draw(pick_buffer_t *pb) {
    if (igBegin("Pick Buffer Debug", NULL, 0)) {
        // Display the pick buffer texture scaled up
        ImVec2 size = { 200, 200 };  // Scale 20×20 → 200×200
        uint64_t tex_id = simgui_imtextureid_with_sampler(pb->tex_view, pb->sampler);
        igImage((ImTextureRef_c){ ._TexID = tex_id }, size, (ImVec2){0,0}, (ImVec2){1,1});

        // Show current hovered entity info
        igText("Hovered ID: %u", pb->hovered_pick_id);
        if (pb->hovered_entity) {
            igText("Entity: %llu", (uint64_t)pb->hovered_entity);
        }
    }
    igEnd();
}
```

---

## Part 5: Selection System

### 5.1 Selection Buffer

```c
typedef struct {
    ecs_entity_t *entities;       // Array of selected entities
    int count;
    int capacity;
} selection_buffer_t;

void selection_clear(selection_buffer_t *sel);
void selection_add(selection_buffer_t *sel, ecs_entity_t e);
void selection_remove(selection_buffer_t *sel, ecs_entity_t e);
void selection_toggle(selection_buffer_t *sel, ecs_entity_t e);
bool selection_contains(selection_buffer_t *sel, ecs_entity_t e);
void selection_set_single(selection_buffer_t *sel, ecs_entity_t e);
```

### 5.2 Selection Input Handling

In the viewport input handler:

```c
void handle_selection_input(pick_buffer_t *pb, selection_buffer_t *sel,
                            bool click, bool shift_held, bool ctrl_held) {
    if (!click) return;

    ecs_entity_t clicked = pb->hovered_entity;

    if (!clicked) {
        // Clicked on empty space
        if (!shift_held && !ctrl_held) {
            selection_clear(sel);
        }
        return;
    }

    if (ctrl_held) {
        // Ctrl+click: toggle selection
        selection_toggle(sel, clicked);
    } else if (shift_held) {
        // Shift+click: add to selection
        selection_add(sel, clicked);
    } else {
        // Normal click: replace selection
        selection_set_single(sel, clicked);
    }
}
```

### 5.3 Selection Visual Feedback

Use **color override only** (no outlines, no glow). Colors are derived from the active ImGui theme for visual consistency.

**Theme-Aware Selection Colors** (`src/selection_colors.h`):

```c
typedef struct {
    vec4_t selected_color;    // Color for selected entities
    vec4_t hovered_color;     // Color for hovered entity
} selection_colors_t;

// Get selection colors from current ImGui theme
static inline selection_colors_t get_theme_selection_colors(void) {
    selection_colors_t colors;

    // Use ImGui's Header color for selection (highlighted state)
    ImVec4 header;
    igGetStyleColorVec4(&header, ImGuiCol_Header);
    colors.selected_color = (vec4_t){header.x, header.y, header.z, 1.0f};

    // Use ImGui's HeaderHovered for hover state
    ImVec4 hovered;
    igGetStyleColorVec4(&hovered, ImGuiCol_HeaderHovered);
    colors.hovered_color = (vec4_t){hovered.x, hovered.y, hovered.z, 1.0f};

    return colors;
}
```

**In Render System:**

```c
// Get theme colors once per frame
selection_colors_t sel_colors = get_theme_selection_colors();

// In render system, check entity state
vec4_t render_color = geom->color;
if (ecs_has(world, entity, SelectedComp)) {
    render_color = sel_colors.selected_color;
} else if (ecs_has(world, entity, HoveredComp)) {
    render_color = sel_colors.hovered_color;
}
```

**Theme Color Examples:**
| Theme | Selected | Hovered |
|-------|----------|---------|
| Visual Studio Dark | Blue (#0077C8) | Lighter blue |
| iOS Light | Gray accent | Lighter gray |
| Catppuccin Frappé | Lavender (#babbf1) | Lighter lavender |

---

## Part 6: ECS Systems

### 6.1 System Overview

```
┌─────────────────────────────────────────────────────────────┐
│                        Frame Loop                           │
├─────────────────────────────────────────────────────────────┤
│  1. Input Phase                                             │
│     └─ Update pick buffer center from mouse                 │
│                                                             │
│  2. ECS Progress (ecs_progress)                             │
│     ├─ TransformSystem: Recalculate dirty world matrices    │
│     ├─ InstanceUpdateSystem: Update GPU instance data       │
│     └─ PickBufferSystem: Prepare pick instances             │
│                                                             │
│  3. Render Phase                                            │
│     ├─ Upload instance buffers to GPU                       │
│     ├─ Render pick buffer (offscreen)                       │
│     ├─ Readback pick buffer, update hovered entity          │
│     ├─ Render main scene (offscreen viewport)               │
│     └─ Render ImGui (main pass)                             │
│                                                             │
│  4. Post-Frame                                              │
│     └─ Handle selection input                               │
└─────────────────────────────────────────────────────────────┘
```

### 6.2 Transform System

```c
// Updates world matrices for entities with dirty transforms
ECS_SYSTEM(world, TransformSystem, EcsOnUpdate, TransformComp);

void TransformSystem(ecs_iter_t *it) {
    TransformComp *t = ecs_field(it, TransformComp, 0);

    for (int i = 0; i < it->count; i++) {
        if (t[i].dirty) {
            t[i].world_matrix = mat4_identity();
            t[i].world_matrix = mat4_translate(t[i].world_matrix, t[i].position);
            t[i].world_matrix = mat4_rotate_x(t[i].world_matrix, t[i].rotation.x);
            t[i].world_matrix = mat4_rotate_y(t[i].world_matrix, t[i].rotation.y);
            t[i].world_matrix = mat4_rotate_z(t[i].world_matrix, t[i].rotation.z);
            t[i].world_matrix = mat4_scale(t[i].world_matrix, t[i].scale);
            t[i].dirty = false;
        }
    }
}
```

### 6.3 Instance Update System

```c
// Updates instance buffer data from entity geometry + transform
ECS_SYSTEM(world, InstanceUpdateSystem, EcsOnUpdate,
           GeometryComp, TransformComp, RenderableComp);

void InstanceUpdateSystem(ecs_iter_t *it) {
    GeometryComp *g = ecs_field(it, GeometryComp, 0);
    TransformComp *t = ecs_field(it, TransformComp, 1);
    RenderableComp *r = ecs_field(it, RenderableComp, 2);

    for (int i = 0; i < it->count; i++) {
        if (!r[i].instance_dirty) continue;

        ecs_entity_t e = it->entities[i];
        bool selected = ecs_has(it->world, e, SelectedComp);

        // Compute instance data based on geometry type
        switch (g[i].type) {
            case GEOM_LINE: {
                line_instance_t inst;
                vec3_t a = mat4_transform_point(t[i].world_matrix, g[i].data.line.a);
                vec3_t b = mat4_transform_point(t[i].world_matrix, g[i].data.line.b);
                inst.ax = a.x; inst.ay = a.y; inst.az = a.z;
                inst.bx = b.x; inst.by = b.y; inst.bz = b.z;
                inst.r = selected ? 1.0f : g[i].color.x;
                inst.g = selected ? 0.8f : g[i].color.y;
                inst.b = selected ? 0.0f : g[i].color.z;
                inst.a = g[i].color.w;

                instance_buffer_set(&batch_mgr.line_instances, r[i].instance_slot, &inst);
                break;
            }
            // ... other geometry types
        }

        r[i].instance_dirty = false;
    }
}
```

### 6.4 Scene API

High-level API for adding/removing entities:

```c
// src/ecs_scene.h

// Create a line entity
ecs_entity_t scene_add_line(ecs_world_t *world, vec3_t a, vec3_t b,
                            vec4_t color, float width);

// Create a polyline entity
ecs_entity_t scene_add_polyline(ecs_world_t *world, vec3_t *points, int count,
                                vec4_t color, float width);

// Create a point entity
ecs_entity_t scene_add_point(ecs_world_t *world, vec3_t pos,
                             vec4_t color, float size);

// Create an arc entity
ecs_entity_t scene_add_arc(ecs_world_t *world, vec3_t center, float radius,
                           float start_angle, float end_angle, vec3_t normal,
                           vec4_t color, float width);

// Create a bezier curve entity (cubic bezier with 4 control points)
ecs_entity_t scene_add_bezier(ecs_world_t *world, vec3_t p0, vec3_t p1,
                              vec3_t p2, vec3_t p3, int segments,
                              vec4_t color, float width);

// Create a helix entity
ecs_entity_t scene_add_helix(ecs_world_t *world, vec3_t axis_start, vec3_t axis_end,
                             float radius, float turns, int segments,
                             vec4_t color, float width);

// Create a polygon entity (closed outline)
ecs_entity_t scene_add_polygon(ecs_world_t *world, vec3_t *points, int count,
                               vec4_t color, float width);

// Delete entity (frees instance slot, pick ID)
void scene_remove_entity(ecs_world_t *world, ecs_entity_t e);

// Modify entity properties
void scene_set_color(ecs_world_t *world, ecs_entity_t e, vec4_t color);
void scene_set_position(ecs_world_t *world, ecs_entity_t e, vec3_t pos);
void scene_set_visible(ecs_world_t *world, ecs_entity_t e, bool visible);
```

---

## Part 7: ImGui Integration

### 7.1 Entity Inspector Panel

```c
// src/ui/ui_entity_inspector.h

typedef struct {
    selection_buffer_t *selection;
    ecs_world_t *world;
} ui_entity_inspector_state_t;

void ui_entity_inspector_draw(ui_entity_inspector_state_t *state) {
    if (!igBegin("Entity Inspector", NULL, 0)) {
        igEnd();
        return;
    }

    int sel_count = state->selection->count;
    igText("Selected: %d entities", sel_count);
    igSeparator();

    if (sel_count == 0) {
        igTextDisabled("No selection");
    } else if (sel_count == 1) {
        // Single entity inspector
        ecs_entity_t e = state->selection->entities[0];
        draw_single_entity_inspector(state->world, e);
    } else {
        // Multi-selection: show common properties or list
        igText("Multiple entities selected:");
        for (int i = 0; i < sel_count; i++) {
            ecs_entity_t e = state->selection->entities[i];
            igBulletText("Entity %llu", (uint64_t)e);
        }
    }

    igEnd();
}

void draw_single_entity_inspector(ecs_world_t *world, ecs_entity_t e) {
    // Transform section
    TransformComp *t = ecs_get_mut(world, e, TransformComp);
    if (t && igCollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
        bool changed = false;
        changed |= igDragFloat3("Position", &t->position.x, 0.1f, -FLT_MAX, FLT_MAX, "%.2f", 0);
        changed |= igDragFloat3("Rotation", &t->rotation.x, 0.01f, -FLT_MAX, FLT_MAX, "%.3f", 0);
        changed |= igDragFloat3("Scale", &t->scale.x, 0.01f, 0.01f, 100.0f, "%.2f", 0);
        if (changed) t->dirty = true;
    }

    // Geometry section
    GeometryComp *g = ecs_get_mut(world, e, GeometryComp);
    if (g && igCollapsingHeader("Geometry", ImGuiTreeNodeFlags_DefaultOpen)) {
        const char *type_names[] = {"Point", "Line", "Polyline", "Arc", "Polygon", "Helix"};
        igText("Type: %s", type_names[g->type]);
        igColorEdit4("Color", &g->color.x, 0);
        igDragFloat("Line Width", &g->line_width, 0.001f, 0.001f, 0.5f, "%.3f", 0);

        // Type-specific editors
        switch (g->type) {
            case GEOM_LINE:
                igDragFloat3("Point A", &g->data.line.a.x, 0.1f, -FLT_MAX, FLT_MAX, "%.2f", 0);
                igDragFloat3("Point B", &g->data.line.b.x, 0.1f, -FLT_MAX, FLT_MAX, "%.2f", 0);
                break;
            // ... other types
        }
    }

    // Renderable section
    RenderableComp *r = ecs_get_mut(world, e, RenderableComp);
    if (r && igCollapsingHeader("Rendering", 0)) {
        igCheckbox("Visible", &r->visible);
        igInputInt("Layer", &r->layer, 1, 10, 0);
    }
}
```

### 7.2 Scene Hierarchy Panel

```c
// src/ui/ui_scene_hierarchy.h

void ui_scene_hierarchy_draw(ecs_world_t *world, selection_buffer_t *sel) {
    if (!igBegin("Scene Hierarchy", NULL, 0)) {
        igEnd();
        return;
    }

    // Query all entities with GeometryComp
    ecs_query_t *q = ecs_query(world, {
        .terms = {{ .id = ecs_id(GeometryComp) }}
    });

    ecs_iter_t it = ecs_query_iter(world, q);
    while (ecs_query_next(&it)) {
        GeometryComp *g = ecs_field(&it, GeometryComp, 0);

        for (int i = 0; i < it.count; i++) {
            ecs_entity_t e = it.entities[i];
            bool is_selected = selection_contains(sel, e);

            char label[64];
            snprintf(label, sizeof(label), "%s_%llu",
                     geom_type_name(g[i].type), (uint64_t)e);

            if (igSelectable_Bool(label, is_selected, 0, (ImVec2){0,0})) {
                if (igGetIO()->KeyCtrl) {
                    selection_toggle(sel, e);
                } else {
                    selection_set_single(sel, e);
                }
            }
        }
    }

    igEnd();
}
```

### 7.3 Add Entity Menu

```c
void ui_add_entity_menu(ecs_world_t *world) {
    if (igBeginMenu("Add Entity", true)) {
        if (igMenuItem_Bool("Point", NULL, false, true)) {
            scene_add_point(world, (vec3_t){0,0,0}, (vec4_t){1,1,1,1}, 0.05f);
        }
        if (igMenuItem_Bool("Line", NULL, false, true)) {
            scene_add_line(world, (vec3_t){-1,0,0}, (vec3_t){1,0,0},
                          (vec4_t){1,0,0,1}, 0.02f);
        }
        if (igMenuItem_Bool("Random Polyline", NULL, false, true)) {
            // Generate random polyline
        }
        // ... more types
        igEndMenu();
    }
}
```

---

## Part 8: Implementation Phases

### Phase 1: Foundation (Week 1)
1. Integrate Flecs library into build system
2. Create basic ECS world initialization
3. Implement core components (Transform, Geometry, Renderable)
4. Port existing instanced_lines to ECS-based rendering
5. Verify rendering still works

**Deliverables:**
- `vendors/flecs/` - Library integration
- `src/ecs_world.h` - World initialization
- `src/components/*.h` - Component definitions
- Modified `demo.c` - ECS world in main loop

### Phase 2: Dynamic Buffers (Week 2)
1. Implement instance_buffer.h with dynamic sizing
2. Create geometry batch manager
3. Implement slot allocation/deallocation
4. Add entity creation/deletion API
5. Test with adding/removing entities at runtime

**Deliverables:**
- `src/gpu/instance_buffer.h` - Dynamic buffer management
- `src/gpu/geometry_batch.h` - Batch manager
- `src/ecs_scene.h` - Scene API

### Phase 3: GPU Picking (Week 3)
1. Implement pick buffer render target
2. Create pick shaders (all backends)
3. Implement pick ID encoding/decoding
4. Add CPU readback mechanism
5. Implement hover detection
6. Add pick buffer debug view

**Deliverables:**
- `src/gpu/pick_buffer.h` - Pick system
- `src/shaders/pick_shaders.h` - Pick shaders
- `src/ui/ui_pick_debug.h` - Debug visualization

### Phase 4: Selection System (Week 4)
1. Implement selection buffer
2. Add selection input handling (click, shift+click, ctrl+click)
3. Implement selection visual feedback
4. Add Selected/Hovered tag components
5. Integrate selection with ECS systems

**Deliverables:**
- `src/selection.h` - Selection buffer
- Modified input handling in viewport
- Selection highlighting in render system

### Phase 5: ImGui Integration (Week 5)
1. Implement entity inspector panel
2. Implement scene hierarchy panel
3. Add entity creation menu
4. Add entity deletion (Delete key)
5. Add property editing with undo (stretch goal)

**Deliverables:**
- `src/ui/ui_entity_inspector.h`
- `src/ui/ui_scene_hierarchy.h`
- Property editing integrated

### Phase 6: Core Geometry Types (Week 6)
1. Implement point rendering using circle shader (same as polyline joins)
2. Implement polyline rendering with dynamic point array allocation
3. Implement arc rendering (tessellated to polyline)
4. Implement polygon rendering (outline mode)
5. Implement helix rendering (tessellated to polyline)
6. Implement bezier curve rendering (tessellated to polyline)
7. Add creation API for all types

**Point Rendering Approach:**

Points use the **same circle shader as polyline joins** (from `join_shaders.h`). This is:
- More efficient than billboarded quads (single instanced draw call)
- Consistent with the existing rendering pipeline
- Works seamlessly with the picking renderer (same geometry)

```c
// Point instance data - same as join instance
typedef struct {
    float x, y, z;           // Center position
    float r, g, b, a;        // Color
} point_instance_t;

// Uses circle template geometry from instanced_polylines
// Point size controlled by line_width uniform
```

**Why not A=B fat lines?** Setting `point_a == point_b` would create a zero-length direction vector, breaking the perpendicular calculation in the vertex shader. The circle approach avoids this issue entirely.

**Deliverables:**
- Full geometry type support (point, line, polyline, arc, polygon, helix, bezier)
- Dynamic allocation for polylines/polygons
- Creation API for all types

### Phase 7: Entity Hierarchy

Enable parent-child relationships using Flecs built-in hierarchy.

1. Enable parent-child via `ecs_add_pair(e, EcsChildOf, parent)`
2. Add parent tracking to TransformComp
3. Update transform system to compute world matrices from hierarchy chain
4. Add scene API: `scene_set_parent()`, `scene_get_parent()`, `scene_get_children()`
5. Update scene hierarchy UI with tree view (expand/collapse nodes)
6. Add drag-and-drop reparenting in hierarchy UI (optional)
7. Test nested transforms (child inherits parent transform)

**Deliverables:**
- Hierarchical transform propagation
- Tree view UI with expand/collapse
- Parenting API

---

### Phase 8: Scene Serialization

Save and load scenes to/from JSON files.

1. Design JSON schema for scene format
2. Implement scene export (all entities + components)
3. Implement scene import (recreate entities)
4. Add file dialog integration (native) or download/upload (web)
5. Handle dynamic allocations (polyline points, etc.)
6. Handle parent-child relationships in serialization

**JSON Schema Example:**
```json
{
  "version": 1,
  "entities": [
    {
      "id": 12345,
      "parent": null,
      "components": {
        "transform": { "position": [0, 0, 0], "rotation": [0, 0, 0], "scale": [1, 1, 1] },
        "geometry": { "type": "line", "a": [-1, 0, 0], "b": [1, 0, 0], "color": [1, 0, 0, 1], "line_width": 0.02 }
      }
    }
  ]
}
```

**Deliverables:**
- `src/scene_serializer.h` - Save/load functions
- File dialog integration

---

### Phase 9: Undo/Redo System (Complete)

Implemented command pattern for reversible entity operations.

1. ✅ Designed command interface (Create, Delete, Modify, Reparent)
2. ✅ Implemented undo stack with fixed capacity (100 commands)
3. ✅ Implemented redo stack (cleared on new action)
4. ✅ Integrated with property editing in inspector (transform, geometry, visibility)
5. ✅ Added Ctrl+Z / Ctrl+Shift+Z keyboard shortcuts
6. ✅ Added Edit menu with Undo/Redo options

**Deliverables:**
- `src/undo_redo.h` - Core data structures and stack operations
- `src/undo_redo_exec.h` - Command execution logic with ECS integration
- Keyboard shortcut integration in demo.c
- Edit menu integration in ui_scene_hierarchy.h
- Property change tracking in ui_entity_inspector.h

---

### Phase 10: Polish & Performance (Future)

1. Frustum culling for entities
2. LOD for distant entities (reduce polyline segments)
3. Spatial hashing for efficient picking (optional)
4. Stress test with 100K+ entities
5. Profile and optimize hot paths

**Deliverables:**
- Performance optimizations
- Stress test demo

---

## File Structure (Proposed)

```
src/
├── ecs/
│   ├── ecs_world.h           # ECS world init/shutdown
│   ├── ecs_scene.h           # High-level scene API
│   └── ecs_systems.h         # System registration
│
├── components/
│   ├── transform_comp.h      # Transform component
│   ├── geometry_comp.h       # Geometry component (point, line, polyline, arc, polygon, helix, bezier)
│   ├── renderable_comp.h     # Renderable component
│   └── selectable_comp.h     # Selectable component
│
├── gpu/
│   ├── instance_buffer.h     # Dynamic instance buffer with slot allocation
│   ├── geometry_batch.h      # Batch manager per geometry type
│   └── pick_buffer.h         # GPU picking system (20×20 buffer)
│
├── shaders/
│   ├── pick_shaders.h        # Pick ID shaders (all backends)
│   └── ... existing shaders
│
├── ui/
│   ├── ui_entity_inspector.h # Property inspector (single + multi-select)
│   ├── ui_scene_hierarchy.h  # Entity list with selection
│   ├── ui_pick_debug.h       # Pick buffer visualization
│   └── ... existing UI
│
├── selection.h               # Selection buffer
├── selection_colors.h        # Theme-aware selection/hover colors
│
├── undo_redo.h               # (Phase 8) Command stack system
└── scene_serializer.h        # (Phase 10) JSON save/load

vendors/
└── flecs/
    ├── flecs.h               # Single-header amalgamation
    ├── flecs.c               # Single-file implementation
    └── CMakeLists.txt        # Build as static library
```

---

## Design Decisions (Resolved)

1. **Polyline memory management:** Dynamic allocation with capacity tracking. Points array freed on entity delete. Implemented in Phase 6.

2. **Multi-pass picking:** Frontmost entity only (using depth buffer). Simpler implementation, matches CAD-like behavior where you click what you see.

3. **Point rendering:** Use circle shader (same as polyline joins) rather than billboarded quads or A=B fat lines. More efficient and consistent with existing pipeline.

4. **Selection visual feedback:** Color override only. No outlines, no glow. Colors derived from active ImGui theme (Header/HeaderHovered colors).

5. **Undo/Redo:** Deferred to Phase 9. Design with command pattern in mind during earlier phases.

---

## References

- [Flecs ECS Library](https://github.com/SanderMertens/flecs)
- [PlayCanvas Entity Picking](https://developer.playcanvas.com/tutorials/entity-picking/)
- [Sokol Buffer Streaming](https://github.com/floooh/sokol/issues/679)
- [OpenGL Buffer Streaming](https://www.khronos.org/opengl/wiki/Buffer_Object_Streaming)
- [Wicked Engine Visibility Buffer](https://wickedengine.net/2024/12/wicked-engines-graphics-in-2024/)
- [Dynamic Instance Rendering](https://medium.com/@kacper.szwajka842/efficient-gpu-rendering-for-dynamic-instances-in-game-development-9cef0b1eeeb6)

---

## Next Steps

1. Review this plan and approve overall approach
2. Start Phase 1: Integrate Flecs library
3. Create minimal proof-of-concept: single line entity rendered via ECS

---

## Session Notes

### 2026-01-27 - Planning Session
- Created initial implementation plan
- Decided on Flecs as ECS library
- Key decisions made:
  - Points use circle shader (not billboarded quads, not A=B fat lines)
  - Selection uses color override only (no outline/glow)
  - Selection/hover colors derived from ImGui theme
  - Polylines use dynamic allocation (freed on entity delete)
  - GPU picking returns frontmost entity only
  - Added bezier curves to geometry types
  - Undo/redo, hierarchy, serialization deferred to Phases 8-10

**Next session:** Begin Phase 1 - Flecs integration

### 2026-01-27 - Implementation Session 1
- Downloaded Flecs single-header amalgamation (flecs.h ~1MB, flecs.c ~2.8MB)
- Created `vendors/flecs/CMakeLists.txt` to build as static library
- Created `src/components/component_types.h` with vec4_t and mat4_transform_point
- Created all component headers:
  - `transform_comp.h` - position, rotation, scale, world_matrix, dirty flag
  - `geometry_comp.h` - all geometry types with creation helpers and dynamic allocation for polylines/polygons
  - `renderable_comp.h` - visibility, layer, batch_id, instance_slot, dirty flag
  - `selectable_comp.h` - pick_id with RGB encoding/decoding helpers
- Created `src/ecs/ecs_world.h` with:
  - Component registration
  - Pick ID allocator with free list recycling
  - Entity creation/deletion helpers
  - Selection/hover tag components
  - Component access helpers
- Fixed imgui_storage.h EMSCRIPTEN_KEEPALIVE conflict
- Verified both native (Metal) and web (WebGPU) builds work

**Completed:** Phase 1 Foundation - all items done
- Integrated ECS world into demo.c init/frame/cleanup
- Created 3 test line entities (RGB axis lines)
- Fixed Emscripten build by disabling HTTP/REST/APP features
- Both native and web builds verified working

**Phase 1 Status:** Complete

### 2026-01-27 - Implementation Session 2
- Created `src/gpu/instance_buffer.h`:
  - Dynamic GPU instance buffer with slot allocation
  - Free list recycling for slot reuse
  - Capacity-doubling growth strategy (64 → 128 → 256 → ...)
  - CPU staging buffer with dirty tracking
  - Per-frame GPU upload via `sg_update_buffer`
- Created `src/gpu/geometry_batch.h`:
  - `geom_line_batch_t` for GEOM_LINE entities (uses instanced line shaders)
  - `geom_point_batch_t` for GEOM_POINT entities (uses join shaders for circles)
  - Template geometry generation (rectangle + caps for lines, circle for points)
  - `geometry_batch_manager_t` combines all batches
- Created `src/ecs/ecs_scene.h`:
  - High-level scene API: `scene_add_line`, `scene_add_point`, etc.
  - Auto-allocates instance buffer slots on entity creation
  - `scene_remove_entity` with slot/ID recycling
  - `ecs_scene_update` syncs dirty entities to GPU
  - `ecs_scene_draw` renders all batched entities
- Integrated into demo.c:
  - 3 RGB axis lines and 3 endpoint points now rendered via ECS
  - Both native (Metal) and web (WebGPU) builds verified working

**Phase 2 Status:** Complete - ready for Phase 3 (GPU Picking)

### 2026-01-27 - Implementation Session 3
- Created `src/shaders/pick_shaders.h`:
  - Multi-backend pick shaders (GLCORE, GLES3, Metal, WGPU, D3D11)
  - Line pick shader (same vertex transform as visual, outputs pick_id as RGB)
  - Point pick shader (same vertex transform as join shader, outputs pick_id as RGB)
- Created `src/gpu/pick_buffer.h`:
  - 20×20 offscreen render target for GPU picking
  - Pick pipelines for lines and points
  - Template geometry generation (reuses patterns from geometry_batch.h)
  - Modified MVP computation for zoomed-in pick region around cursor
  - CPU readback buffer (placeholder - needs platform-specific implementation)
  - Hover detection via center pixel sampling
- Created `src/ui/ui_pick_debug.h`:
  - Debug visualization window for pick buffer
  - Shows 20×20 buffer scaled to 200×200
  - Displays hovered pick ID and RGB encoding
  - Crosshair overlay to show sample point
- Updated `src/ecs/ecs_scene.h`:
  - Added `ecs_scene_populate_pick_buffer()` to fill pick buffer with entity geometry
  - Added `ecs_scene_find_entity_by_pick_id()` for entity lookup
  - Added `ecs_scene_update_hover()` to update ECS hover tags
  - Added `ecs_scene_clear_all_hovered()` helper
- Updated `src/ui/ui_viewport.h`:
  - Added `window_pos_x`, `window_pos_y` for mouse coordinate conversion
  - Added `hovered` flag for viewport hover state
- Updated `src/gpu/instance_buffer.h`:
  - Added `instance_buffer_clear()` function for pick buffer frame reset
- Integrated into demo.c:
  - Pick buffer initialized/shutdown
  - Pick debug UI panel added (closed by default)
  - Pick pass runs after main render pass when viewport is hovered
  - Native (Metal) build verified working

**Note:** CPU readback not yet implemented - requires platform-specific APIs (Metal getBytes, OpenGL glReadPixels, WebGPU buffer mapping). The pick buffer renders correctly but hover detection needs readback to work.

**Phase 3 Status:** Complete

### 2026-01-28 - Implementation Session 4
- Implemented Metal CPU readback for GPU picking:
  - Created `src/gpu/pick_readback.h` - Cross-platform readback interface
  - Created `src/gpu/pick_readback_metal.m` - Metal-specific implementation using MTLTexture getBytes
  - Updated `src/CMakeLists.txt` to compile Objective-C file on Apple platforms
- Updated `pick_buffer.h` to use the new readback system
- Added ECS visibility controls to `ui_visibility.h`:
  - "Show ECS Entities" checkbox
  - "Pick Buffer Debug" checkbox (toggles debug window)
- Updated demo.c to wire up visibility controls
- Native (Metal) build verified working with GPU picking

**GPU Picking now functional on Metal (macOS/iOS)**
- Hover over ECS entities (RGB axis lines and points) to see pick ID in debug panel
- Enable "Pick Buffer Debug" in Visibility panel to see the 20×20 pick buffer
- Other backends (OpenGL, WebGPU, D3D11) have placeholder readback (returns no hover)

### 2026-01-28 - Implementation Session 5 (Pick System Polish)
**Focus:** Fix picking accuracy and add hover visual feedback

**Issues Fixed:**
1. **Thin/dotted lines in pick buffer** - Lines were rendering too thin for accurate picking
   - Root cause: Line width wasn't scaling with the zoom factor
   - Fix: Added uniform zoom (avoids distortion) and multiply line_width by zoom_factor
2. **Viewport offset** - Pick buffer was misaligned with main viewport
   - Root cause: Using DPI-scaled render target size for coordinate conversion
   - Fix: Use logical viewport size (content_width/height) for mouse-to-normalized conversion
3. **No hover feedback** - Entities didn't visually change when hovered
   - Fix: Added theme-aware hover color from ImGui (HeaderHovered color)
   - Entities marked dirty when hover state changes, color updated in ecs_scene_update()

**Features Added:**
- **Theme-aware hover colors** - Uses ImGui's HeaderHovered color for visual consistency
- **ECS Thickness Controls** in Visibility panel:
  - ECS Line Width slider (0.005 - 0.1)
  - ECS Point Size slider (0.01 - 0.2)
  - Pick Thickness Multiplier slider (1.0x - 5.0x) - makes entities easier to pick
- **Debug info** in Pick Buffer Debug window:
  - Zoom factor display
  - Scaled line width display

**Files Modified:**
- `src/gpu/pick_buffer.h` - thickness_multiplier, uniform zoom, scaled line width
- `src/ecs/ecs_scene.h` - Theme-aware hover colors, dirty marking on hover change
- `src/ui/ui_visibility.h` - Thickness control sliders and setter function
- `src/ui/ui_pick_debug.h` - Additional debug info display
- `src/ui/ui_theme.h` - Added `ui_theme_get_hover_color()` and `ui_theme_get_selection_color()`
- `src/demo.c` - Fixed coordinate conversion, wired up thickness controls

**Phase 3 Status:** Complete - GPU picking fully functional on Metal
- Picking accurately identifies entities under cursor
- Hover visual feedback works (entities change to theme's hover color)
- Thickness is configurable for fine-tuning

### 2026-01-28 - Implementation Session 6 (Selection System)
**Focus:** Implement Phase 4 - Selection System

**Files Created:**
- `src/selection.h` - Selection buffer with entity management:
  - `selection_buffer_t` struct with dynamic entity array
  - `selection_init/shutdown` lifecycle
  - `selection_add/remove/toggle/clear/set_single` operations
  - `selection_handle_click` helper for modifier key handling
  - Auto-syncs with ECS Selected tag on all operations
  - Marks entities dirty for re-render on selection changes

**Files Modified:**
- `src/ecs/ecs_scene.h` - Added selection color override:
  - Gets selection color from `ImGuiCol_HeaderActive`
  - Priority: Selected > Hovered > Normal color
- `src/ui/ui_viewport.h` - Added click detection:
  - Tracks `clicked`, `shift_held`, `ctrl_held` state
  - Uses `igIsMouseClicked_Bool` for click detection
- `src/ui/ui_visibility.h` - Added selection count display:
  - Shows "No selection", "1 entity selected", or "N entities selected"
  - Pointer to selection count from demo.c
- `src/demo.c` - Integrated selection system:
  - Added `selection_buffer_t` to app state
  - Initialize/shutdown selection buffer
  - Handle clicks via `selection_handle_click`
  - Wire up selection count to visibility panel

**Selection Behavior:**
- Click: Replace selection with clicked entity
- Ctrl+Click: Toggle entity's selection state
- Shift+Click: Add entity to selection
- Click empty space: Clear selection (unless modifiers held)
- Selected entities display with theme's active color (HeaderActive)
- Hovered entities display with theme's hover color (HeaderHovered)

**Phase 4 Status:** Complete - Selection system fully functional on Metal
- Single and multi-selection works
- Modifier keys (Ctrl, Shift) work correctly
- Selection count displayed in Visibility panel
- Theme-aware selection highlighting

### 2026-01-28 - Implementation Session 7 (ImGui Integration)
**Focus:** Implement Phase 5 - Entity Inspector and Scene Hierarchy

**Files Created:**
- `src/ui/ui_entity_inspector.h` - Entity property inspector panel:
  - Single entity view with Transform, Geometry, Renderable sections
  - Multi-selection view with entity list and common color editing
  - Show All/Hide All buttons for batch visibility
- `src/ui/ui_scene_hierarchy.h` - Scene hierarchy panel:
  - Lists all entities with GeometryComp (type + ID)
  - Click/Ctrl+Click/Shift+Click selection support
  - Right-click context menu (Select, Add to Selection, Delete)
  - Menu bar with "Add Entity" submenu (Point, Line X/Y/Z, Random Line)

**Files Modified:**
- `src/selection.h` - Added bulk operation helpers:
  - `selection_get_entities()` - Get internal entity array
  - `selection_copy_entities()` - Copy entities to external buffer
- `src/demo.c` - Integrated Phase 5:
  - Added ui_entity_inspector and ui_scene_hierarchy includes
  - Added state structs for new panels
  - Initialize panels in init()
  - Draw panels in frame() when UI visible
  - Delete key handling (Delete/Backspace deletes selected entities)

**Phase 5 Status:** Complete - ImGui integration fully functional on Metal
- Entity Inspector shows/edits selected entity properties
- Scene Hierarchy lists all entities with selection support
- Entity creation via menu (Point, Line variants)
- Delete key removes selected entities
- Both native (Metal) and web builds verified working

**Bug Fixes (same session):**
1. **Visibility checkbox fix** - Hidden entities now get degenerate instance data (zero-size/off-screen)
2. **Scene hierarchy order fix** - Entities sorted by ID for stable display order

### 2026-01-29 - Phase 7 Bug Fixes (Hierarchy Transform Issues)
**Focus:** Fix hierarchy-related bugs discovered during testing

**Issues Fixed:**

1. **Moving parent didn't move children** (Task 7.8)
   - Inspector only marked entity's own transform dirty
   - Fix: Call `ecs_world_mark_descendants_dirty()` when transform changes

2. **Deleting parent left children rendering** (Task 7.9)
   - Children's GPU buffer slots weren't freed
   - Fix: Recursive deletion that frees each child's slots first

3. **Rotation/scale didn't apply** (Task 7.10)
   - Dirty children with non-dirty parents weren't processed
   - Fix: Process dirty entities with parents using parent's world_matrix

4. **Reparenting subtree didn't update grandchildren** (Task 7.11)
   - Only immediate child was marked dirty on reparent
   - Recursive update skipped non-dirty descendants
   - Fix: Mark entire subtree dirty on reparent; force dirty in recursive update

**Files Modified:**
- `src/ui/ui_entity_inspector.h` - Mark descendants dirty on transform change
- `src/ecs/ecs_scene.h` - Recursive deletion, improved transform propagation
- `src/ecs/ecs_world.h` - set_parent marks descendants dirty

**Phase 7 Status:** Complete with bug fixes - hierarchical transforms fully functional

### 2026-01-29 - Phase 7.6 Drag-and-Drop Reparenting
**Focus:** Implement drag-and-drop reparenting in the Scene Hierarchy UI

**Files Modified:**
- `src/ui/ui_scene_hierarchy.h` - Added drag-and-drop functionality

**Implementation Details:**
1. Added `UI_HIERARCHY_DRAG_DROP_TYPE` constant for ImGui drag-drop payload
2. Added `ui_hierarchy_is_descendant()` helper to prevent cycle creation
3. Updated `ui_scene_hierarchy_draw_entity_leaf()` with drag source and drop target
4. Updated `ui_scene_hierarchy_draw_entity_tree()` with drag source and drop target
5. Added root-level drop targets (top and bottom of list) for unparenting

**Drag-and-Drop Features:**
- Drag any entity to reparent it
- Drop on another entity to make it a child
- Drop on empty space (top/bottom) to unparent (make it a root)
- Cycle prevention: Can't drop a parent onto its own descendant
- Self-drop prevention: Can't drop entity onto itself
- Visual feedback: Tooltip shows "Move [Type] #[ID]" during drag

**Phase 7 Status:** Complete - All hierarchy features implemented

---

## Implementation Notes for 7.6: Drag-and-Drop Reparenting

### ImGui Drag-and-Drop API
Use ImGui's built-in drag-and-drop system:
- `igBeginDragDropSource()` / `igEndDragDropSource()` - make tree node draggable
- `igSetDragDropPayload()` - attach entity ID as payload
- `igBeginDragDropTarget()` / `igEndDragDropTarget()` - make tree node a drop target
- `igAcceptDragDropPayload()` - receive dropped entity

### Implementation Steps
1. In `ui_scene_hierarchy_draw_entity_tree()`, after drawing tree node:
   - If node is hovered and drag starts, begin drag source with entity ID
   - Make each tree node a drop target
2. On drop: call `scene_set_parent(scene, dragged_entity, target_entity)`
3. Handle edge cases:
   - Prevent dropping entity onto itself
   - Prevent dropping parent onto its own descendant (would create cycle)
   - Allow dropping onto empty space to unparent
4. Visual feedback:
   - Show insertion indicator during drag
   - Highlight valid drop targets

---

## Phase 8: Scene Serialization (Complete)

### 2026-01-29 - JSON Scene Serialization
**Focus:** Implement JSON save/load for ECS scenes

**Files Created:**
- `src/ui/ui_file_browser.h` - Cross-platform ImGui file browser:
  - Directory listing with icons for folders vs files
  - Navigation (up button, path editing, double-click to enter)
  - File filtering by extension (.json)
  - Open/Save modes
  - File size display
  - Sorted listing (directories first, then alphabetically)
- `src/scene_serializer.h` - Complete JSON serialization module:
  - JSON string builder with automatic capacity growth
  - JSON tokenizer/parser (minimal, no external dependencies)
  - Support for all geometry types (point, line, polyline, arc, polygon, bezier, helix)
  - Support for parent-child relationships via ID mapping
  - Transform, geometry, and renderable component serialization

**Files Modified:**
- `src/ui/ui_scene_hierarchy.h` - Added File menu with file browser:
  - "Save Scene..." (Ctrl+S) - Full file browser dialog
  - "Load Scene..." (Ctrl+O) - Full file browser dialog
  - "Clear on Load" checkbox option
  - "Clear Scene" - Remove all entities
  - Status messages for save/load operations

**JSON Schema (v1):**
```json
{
  "version": 1,
  "entities": [
    {
      "id": 12345,
      "parent": null,
      "components": {
        "transform": {
          "position": [0, 0, 0],
          "rotation": [0, 0, 0],
          "scale": [1, 1, 1]
        },
        "geometry": {
          "type": "line",
          "color": [1, 0, 0, 1],
          "line_width": 0.02,
          "point_size": 0,
          "a": [-1, 0, 0],
          "b": [1, 0, 0]
        },
        "renderable": {
          "visible": true,
          "layer": 0
        }
      }
    }
  ]
}
```

**Features:**
- **Save:** Exports all entities with their components to human-readable JSON
- **Load:** Parses JSON and recreates entities with proper ID mapping for parent-child relationships
- **File browser:** Full directory navigation, file filtering, double-click to open folders
- **Clear option:** Load can optionally clear existing entities first
- **All geometry types:** Points, lines, polylines, arcs, polygons, bezier curves, helices
- **Dynamic arrays:** Polyline/polygon points stored as arrays in JSON
- **Hierarchies preserved:** Parent-child relationships serialized via ID references
- **Keyboard shortcuts:** Ctrl+S to save, Ctrl+O to load

**Phase 8 Status:** Complete - JSON serialization with file browser fully functional

---

## Debug Tools

### Slot Buffer Debug Viewer (2026-02-02)

A debug UI panel for visualizing the entity slot buffer state in real time.

**Files:**
- `src/ui/ui_slot_buffer_debug.h` - Grid-based slot buffer viewer

**Features:**
- Tabbed view for Lines and Points/Joins buffers
- Grid visualization with color coding by entity type
- Paging with navigation controls
- Tooltips showing slot/entity details

### Bug Fix: ImGuiInputTextFlags_EnterReturnsTrue Deprecation

**Issue:** App crashed when opening Slot Buffer Debug panel with assertion:
```
Assertion failed: ((flags & ImGuiInputTextFlags_EnterReturnsTrue) == 0)
function InputScalar, file imgui_widgets.cpp, line 3762
```

**Root cause:** Modern Dear ImGui (1.90+) deprecated `ImGuiInputTextFlags_EnterReturnsTrue` for `InputScalar`-based functions (`InputInt`, `InputFloat`, etc.). The flag is now explicitly rejected via assertion because these functions already return true on Enter press by default.

**Fix:** Remove the flag from `igInputInt()` calls. Use `0` for flags parameter instead.

```c
// Before (crashes):
igInputInt("##page", &display_page, 0, 0, ImGuiInputTextFlags_EnterReturnsTrue);

// After (works):
igInputInt("##page", &display_page, 0, 0, 0);
```

**Note for future development:** Avoid using `ImGuiInputTextFlags_EnterReturnsTrue` with any `InputScalar`-based ImGui functions. The flag is only valid for `InputText` and `InputTextMultiline`.
