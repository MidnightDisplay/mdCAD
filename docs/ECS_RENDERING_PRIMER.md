# ECS Rendering System Primer

A comprehensive guide to understanding the Entity Component System (ECS) and GPU-instanced rendering architecture.

## Table of Contents

1. [Architecture Overview](#architecture-overview)
2. [Component Design](#component-design)
3. [GPU Buffer Management](#gpu-buffer-management)
4. [Memory Layout](#memory-layout)
5. [Slot Allocation Strategy](#slot-allocation-strategy)
6. [Dirty Flag System](#dirty-flag-system)
7. [Rendering Pipeline](#rendering-pipeline)
8. [Examples: Entity Lifecycle](#examples-entity-lifecycle)

---

## Architecture Overview

The system combines **Flecs ECS** for entity management with **GPU instancing** for efficient rendering of thousands of geometric primitives.

```mermaid
flowchart TB
    subgraph ECS["ECS Layer (Flecs)"]
        E1[Entity 1]
        E2[Entity 2]
        E3[Entity N]
    end

    subgraph Components["Component Data"]
        TC[TransformComp<br/>position, rotation, scale]
        GC[GeometryComp<br/>type, points, color]
        RC[RenderableComp<br/>slot indices, dirty flag]
        SC[SelectableComp<br/>pick_id]
    end

    subgraph GPU["GPU Layer"]
        LB[Line Instance Buffer<br/>CPU staging + GPU buffer]
        PB[Point Instance Buffer<br/>CPU staging + GPU buffer]
    end

    subgraph Render["Rendering"]
        LP[Line Pipeline<br/>instanced draw]
        PP[Point Pipeline<br/>instanced draw]
    end

    E1 --> TC
    E1 --> GC
    E1 --> RC
    E1 --> SC

    RC -->|slot index| LB
    RC -->|slot index| PB

    LB --> LP
    PB --> PP
```

### Key Principles

1. **ECS for Logic**: Flecs manages entity lifecycle, component storage, and queries
2. **GPU Instancing for Rendering**: Template geometry is drawn once, instance data varies per entity
3. **CPU Staging Buffers**: All instance data lives in CPU memory, uploaded to GPU once per frame
4. **Slot-Based Allocation**: Each entity reserves slots in GPU buffers, tracked by `RenderableComp`

---

## Component Design

### Component Hierarchy

```mermaid
classDiagram
    class TransformComp {
        vec3_t position
        vec3_t rotation
        vec3_t scale
        mat4_t world_matrix
        bool dirty
    }

    class GeometryComp {
        geometry_type_t type
        union data
        float line_width
        float point_size
        vec4_t color
    }

    class RenderableComp {
        bool visible
        int layer
        uint32_t batch_id
        uint32_t instance_slot
        uint32_t segment_count
        uint32_t join_slot_start
        uint32_t join_count
        bool instance_dirty
    }

    class SelectableComp {
        uint32_t pick_id
        bool pickable
    }

    note for GeometryComp "Union contains:<br/>point, line, polyline,<br/>arc, polygon, bezier, helix"

    note for RenderableComp "Multi-segment entities use:<br/>segment_count + join_count<br/>for polylines, arcs, etc."
```

### Geometry Types

| Type | Segments | Joins | Description |
|------|----------|-------|-------------|
| `GEOM_POINT` | 0 | 0 | Single point (uses point buffer) |
| `GEOM_LINE` | 1 | 0 | Single line segment |
| `GEOM_POLYLINE` | N-1 | N-2 | Open path with N points |
| `GEOM_POLYGON` | N | N | Closed path with N points |
| `GEOM_ARC` | varies | varies | Tessellated to polyline |
| `GEOM_BEZIER` | varies | varies | Tessellated to polyline |
| `GEOM_HELIX` | varies | varies | Tessellated to polyline |

---

## GPU Buffer Management

### Instance Buffer Structure

```mermaid
flowchart LR
    subgraph IB["instance_buffer_t"]
        direction TB
        GPU[sg_buffer<br/>GPU Buffer]
        CPU[void* staging<br/>CPU Array]
        META[capacity: 64<br/>count: 5<br/>instance_size: 40]
        FREE[free_slots_array<br/>Free List Stack]
        DIRTY[needs_upload: true<br/>dirty_min: 2<br/>dirty_max: 4]
    end

    CPU -->|"sg_update_buffer()"| GPU
```

### Line Instance Data (40 bytes)

```c
typedef struct {
    float ax, ay, az;   // Point A (12 bytes)
    float bx, by, bz;   // Point B (12 bytes)
    float r, g, b, a;   // Color   (16 bytes)
} geom_line_instance_t;  // Total: 40 bytes
```

### Point Instance Data (28 bytes)

```c
typedef struct {
    float x, y, z;      // Center  (12 bytes)
    float r, g, b, a;   // Color   (16 bytes)
} geom_point_instance_t; // Total: 28 bytes
```

---

## Memory Layout

### Single-Slot Entities (Point, Line)

```
Line Instance Buffer (CPU staging):
┌─────────────────────────────────────────────────────────────┐
│ Slot 0 │ Slot 1 │ Slot 2 │ Slot 3 │ Slot 4 │ ... │ Slot 63 │
│ 40B    │ 40B    │ 40B    │ 40B    │ 40B    │     │ 40B     │
│ Line A │ Line B │ Line C │ (free) │ Line D │     │ (empty) │
└─────────────────────────────────────────────────────────────┘
         ▲                   ▲
         │                   │
    Entity "Line B"    Free list: [3]
    RenderableComp:
      instance_slot = 1
      segment_count = 1
```

### Multi-Slot Entities (Polyline, Arc, Polygon, etc.)

For a **polyline with 4 points** (3 segments, 2 joins):

```
Line Instance Buffer:
┌─────────────────────────────────────────────────────────────┐
│ Slot 0 │ Slot 1 │ Slot 2 │ Slot 3 │ Slot 4 │ Slot 5 │ ...  │
│ Line A │ Seg 0  │ Seg 1  │ Seg 2  │ Line B │ (free) │      │
└─────────────────────────────────────────────────────────────┘
         ├────────────────────┤
              Polyline P
              (contiguous!)

Point Instance Buffer:
┌─────────────────────────────────────────────────────────────┐
│ Slot 0 │ Slot 1 │ Slot 2 │ Slot 3 │ Slot 4 │ ...           │
│ Pt A   │ Join 0 │ Join 1 │ Pt B   │ (free) │               │
└─────────────────────────────────────────────────────────────┘
         ├─────────────┤
           Polyline P joins
           (contiguous!)

Entity "Polyline P" RenderableComp:
  instance_slot = 1      (first segment slot)
  segment_count = 3
  join_slot_start = 1    (first join slot)
  join_count = 2
```

### Why Contiguous Allocation Matters

```mermaid
flowchart TB
    subgraph Problem["❌ Non-Contiguous (Old Bug)"]
        direction LR
        A1["Alloc seg 0"] -->|"free list"| S2["Slot 2"]
        A2["Alloc seg 1"] -->|"free list"| S1["Slot 1"]
        A3["Alloc seg 2"] -->|"free list"| S0["Slot 0"]

        R1["first_slot = 2<br/>Update uses: 2,3,4"]
        R1 -->|"WRONG!"| X["Corrupts other entities"]
    end

    subgraph Solution["✅ Contiguous (Fixed)"]
        direction LR
        B1["Alloc 3 slots"] -->|"skip free list"| S345["Slots 5,6,7"]

        R2["first_slot = 5<br/>Update uses: 5,6,7"]
        R2 -->|"CORRECT"| Y["All slots belong to this entity"]
    end
```

---

## Slot Allocation Strategy

### Allocation Functions

```mermaid
flowchart TD
    subgraph Single["Single Slot (Points, Lines)"]
        A1[instance_buffer_alloc_slot]
        A1 --> C1{Free list<br/>has slots?}
        C1 -->|Yes| P1[Pop from free list<br/>LIFO]
        C1 -->|No| N1[Allocate at count++]
        P1 --> R1[Return slot]
        N1 --> G1{Need grow?}
        G1 -->|Yes| GR1[Double capacity<br/>Recreate GPU buffer]
        G1 -->|No| R1
        GR1 --> R1
    end

    subgraph Multi["Multiple Slots (Polylines, Arcs, etc.)"]
        A2[instance_buffer_alloc_contiguous n]
        A2 --> N2[Skip free list!<br/>Allocate at count]
        N2 --> G2{Need grow?}
        G2 -->|Yes| GR2[Double capacity<br/>Recreate GPU buffer]
        G2 -->|No| R2[count += n<br/>Return first_slot]
        GR2 --> R2
    end
```

### Free List Behavior (LIFO Stack)

```
Initial state: count=6, free_slots=[]

Delete entity at slot 2:
  free_slots = [2], free_count = 1

Delete entity at slot 4:
  free_slots = [2, 4], free_count = 2

Allocate single slot:
  Returns 4 (LIFO pop), free_slots = [2], free_count = 1

Allocate single slot:
  Returns 2 (LIFO pop), free_slots = [], free_count = 0

Allocate single slot:
  Returns 6 (new slot), count = 7
```

---

## Dirty Flag System

### Two-Level Dirty Tracking

```mermaid
flowchart TB
    subgraph Entity["Per-Entity (RenderableComp)"]
        ED[instance_dirty: bool]
        ED -->|"true"| U1["Update instance data<br/>in ecs_scene_update()"]
    end

    subgraph Buffer["Per-Buffer (instance_buffer_t)"]
        BD[needs_upload: bool]
        DM[dirty_min: int]
        DX[dirty_max: int]
        BD -->|"true"| U2["Upload to GPU<br/>in instance_buffer_upload()"]
    end

    U1 -->|"instance_buffer_set()"| BD
    U1 -->|"updates"| DM
    U1 -->|"updates"| DX
```

### Dirty Flag Flow

```mermaid
sequenceDiagram
    participant User
    participant ECS as ecs_scene_update()
    participant IB as Instance Buffer
    participant GPU

    User->>ECS: Modify entity (color, position, etc.)
    Note over ECS: Set RenderableComp.instance_dirty = true

    loop For each dirty entity
        ECS->>IB: instance_buffer_set(slot, data)
        Note over IB: Copy to staging buffer<br/>Set needs_upload = true<br/>Update dirty_min/max
    end

    ECS->>IB: instance_buffer_upload()

    alt needs_upload == true
        IB->>GPU: sg_update_buffer(staging, count * size)
        Note over IB: Reset needs_upload = false
    end
```

### What Triggers Dirty?

| Action | Sets `instance_dirty` | Sets `needs_upload` |
|--------|----------------------|---------------------|
| Entity created | Initially `false`* | `true` (via `_set()`) |
| Color changed | `true` | (after update) |
| Transform changed | `true` | (after update) |
| Visibility toggled | `true` | (after update) |
| Hover state changed | `true` | (after update) |
| Selection changed | `true` | (after update) |
| Slot freed | N/A | `true` (zeros data) |

*Entity creation sets data directly, then sets `instance_dirty = false` since data is already in staging buffer.

---

## Rendering Pipeline

### GPU Instancing Concept

```mermaid
flowchart LR
    subgraph Template["Template Geometry (drawn once)"]
        TL["Line Template:<br/>Rectangle + 2 Semicircle Caps"]
        TP["Point Template:<br/>Circle (16 segments)"]
    end

    subgraph Instances["Instance Data (varies per entity)"]
        I1["Instance 0: A=(0,0,0) B=(1,0,0) Red"]
        I2["Instance 1: A=(0,1,0) B=(0,2,0) Green"]
        I3["Instance N: ..."]
    end

    subgraph Draw["Instanced Draw Call"]
        D["sg_draw(0, index_count, instance_count)<br/>Draws template N times"]
    end

    Template --> Draw
    Instances --> Draw
    Draw --> Screen["Screen Output:<br/>N lines rendered<br/>with 1 draw call"]
```

### Vertex Shader: Screen-Space Width

The vertex shader transforms the template geometry to achieve **constant screen-space width**:

```mermaid
flowchart TD
    subgraph Input["Inputs"]
        T["template_pos: (along, perp, endpoint_select)"]
        PA["point_a: world position"]
        PB["point_b: world position"]
        C["color: RGBA"]
        U["uniforms: mvp, line_width, aspect_ratio"]
    end

    subgraph Transform["Vertex Shader Logic"]
        S1["1. Transform A,B to clip space"]
        S2["2. Compute direction in NDC"]
        S3["3. Compute perpendicular in NDC"]
        S4["4. Correct for aspect ratio"]
        S5["5. Select base position (A or B)"]
        S6["6. Apply template offset × line_width"]
    end

    Input --> S1 --> S2 --> S3 --> S4 --> S5 --> S6

    S6 --> Output["Output: Clip-space position<br/>with constant pixel width"]
```

### Frame Rendering Flow

```mermaid
sequenceDiagram
    participant App as demo.c
    participant Scene as ecs_scene
    participant Batch as geometry_batch
    participant GPU

    Note over App: frame() called

    App->>Scene: ecs_scene_update()
    Note over Scene: Process dirty entities<br/>Update instance data

    Scene->>Batch: geometry_batch_manager_upload()
    Batch->>GPU: sg_update_buffer() for lines
    Batch->>GPU: sg_update_buffer() for points

    App->>GPU: Begin offscreen pass

    App->>Scene: ecs_scene_draw(mvp, aspect)
    Scene->>Batch: geom_line_batch_draw()
    Note over Batch: sg_apply_pipeline()<br/>sg_apply_bindings()<br/>sg_draw(0, idx_count, instance_count)

    Scene->>Batch: geom_point_batch_draw()
    Note over Batch: sg_apply_pipeline()<br/>sg_apply_bindings()<br/>sg_draw(0, idx_count, instance_count)

    App->>GPU: End pass
```

---

## Examples: Entity Lifecycle

### Example 1: Adding a Point

```mermaid
sequenceDiagram
    participant User
    participant Scene as scene_add_point()
    participant ECS as Flecs
    participant PB as Point Buffer

    User->>Scene: scene_add_point(pos, color, size)

    Scene->>ECS: ecs_world_create_entity()
    Note over ECS: Creates entity with:<br/>- TransformComp (identity)<br/>- RenderableComp (empty)<br/>- SelectableComp (pick_id=N)

    Scene->>ECS: ecs_world_set_geometry(GEOM_POINT)
    Note over ECS: Sets GeometryComp:<br/>type=GEOM_POINT<br/>point=(x,y,z)<br/>color, point_size

    Scene->>PB: geom_point_batch_alloc()
    Note over PB: Returns slot 3<br/>(from free list or count++)

    Scene->>PB: geom_point_batch_set(slot=3, pos, color)
    Note over PB: staging[3] = {x,y,z,r,g,b,a}<br/>needs_upload = true

    Scene->>ECS: Update RenderableComp
    Note over ECS: batch_id = GEOM_POINT<br/>instance_slot = 3<br/>instance_dirty = false

    Scene-->>User: Return entity ID
```

**Memory State After:**

```
Point Instance Buffer:
┌────────┬────────┬────────┬─────────────┬────────┐
│ Slot 0 │ Slot 1 │ Slot 2 │ Slot 3      │ Slot 4 │
│ Pt X   │ Pt Y   │ (free) │ NEW POINT   │ (empty)│
│        │        │        │ x,y,z,r,g,b,a│        │
└────────┴────────┴────────┴─────────────┴────────┘
                            ▲
                            │
                    Entity's slot

Buffer metadata:
  count = 4 (if slot 3 was new)
  needs_upload = true
  dirty_min = 3, dirty_max = 3
```

---

### Example 2: Adding a Line

```mermaid
sequenceDiagram
    participant User
    participant Scene as scene_add_line()
    participant ECS as Flecs
    participant LB as Line Buffer

    User->>Scene: scene_add_line(a, b, color, width)

    Scene->>ECS: ecs_world_create_entity()

    Scene->>ECS: ecs_world_set_geometry(GEOM_LINE)
    Note over ECS: Sets GeometryComp:<br/>type=GEOM_LINE<br/>line.a, line.b<br/>color, line_width

    Scene->>LB: geom_line_batch_alloc()
    Note over LB: Returns slot 5

    Scene->>Scene: Transform points by world_matrix
    Note over Scene: world_a = mat4_transform_point(matrix, a)<br/>world_b = mat4_transform_point(matrix, b)

    Scene->>LB: geom_line_batch_set(slot=5, world_a, world_b, color)
    Note over LB: staging[5] = {ax,ay,az,bx,by,bz,r,g,b,a}<br/>needs_upload = true

    Scene->>ECS: Update RenderableComp
    Note over ECS: batch_id = GEOM_LINE<br/>instance_slot = 5<br/>instance_dirty = false

    Scene-->>User: Return entity ID
```

**Memory State After:**

```
Line Instance Buffer (40 bytes per slot):
┌──────────────────────────────────────────────────────────────────┐
│ Slot 0    │ Slot 1    │ ... │ Slot 5                   │ Slot 6 │
│ Line X    │ Line Y    │     │ NEW LINE                 │ (empty)│
│           │           │     │ ax,ay,az,bx,by,bz,r,g,b,a│        │
└──────────────────────────────────────────────────────────────────┘
                               ▲
                               │
                       Entity's instance_slot = 5
```

---

### Example 3: Adding an Arc (Multi-Segment)

An arc with `start_angle=0`, `end_angle=π/2` (quarter circle) tessellates to ~7 points → 6 segments + 5 joins.

```mermaid
sequenceDiagram
    participant User
    participant Scene as scene_add_arc()
    participant Tess as Tessellation
    participant ECS as Flecs
    participant LB as Line Buffer
    participant PB as Point Buffer

    User->>Scene: scene_add_arc(center, radius, 0, π/2, normal, color, width)

    Scene->>Tess: ecs_scene_tessellate_arc()
    Note over Tess: Generate 7 points along arc:<br/>P0, P1, P2, P3, P4, P5, P6
    Tess-->>Scene: points[], point_count=7

    Scene->>ECS: ecs_world_create_entity()
    Scene->>ECS: ecs_world_set_geometry(GEOM_ARC)

    Note over Scene: Need 6 contiguous segment slots
    Scene->>LB: geom_line_batch_alloc_contiguous(6)
    Note over LB: Skip free list!<br/>Allocate slots 10-15<br/>count += 6
    LB-->>Scene: first_slot = 10

    loop i = 0 to 5
        Scene->>LB: geom_line_batch_set(10+i, points[i], points[i+1], color)
    end

    Note over Scene: Need 5 contiguous join slots
    Scene->>PB: geom_point_batch_alloc_contiguous(5)
    Note over PB: Skip free list!<br/>Allocate slots 8-12<br/>count += 5
    PB-->>Scene: first_join = 8

    loop j = 0 to 4
        Scene->>PB: geom_point_batch_set(8+j, points[j+1], color)
    end

    Scene->>ECS: Update RenderableComp
    Note over ECS: batch_id = GEOM_ARC<br/>instance_slot = 10<br/>segment_count = 6<br/>join_slot_start = 8<br/>join_count = 5<br/>instance_dirty = false

    Scene-->>User: Return entity ID
```

**Memory State After:**

```
Line Instance Buffer:
┌─────────────────────────────────────────────────────────────────────────────┐
│ ... │ Slot 10   │ Slot 11   │ Slot 12   │ Slot 13   │ Slot 14   │ Slot 15   │
│     │ P0→P1     │ P1→P2     │ P2→P3     │ P3→P4     │ P4→P5     │ P5→P6     │
│     ├───────────┴───────────┴───────────┴───────────┴───────────┴───────────┤
│     │                    Arc Entity (6 contiguous segments)                  │
└─────────────────────────────────────────────────────────────────────────────┘

Point Instance Buffer:
┌───────────────────────────────────────────────────────────────────┐
│ ... │ Slot 8    │ Slot 9    │ Slot 10   │ Slot 11   │ Slot 12    │
│     │ Join @P1  │ Join @P2  │ Join @P3  │ Join @P4  │ Join @P5   │
│     ├───────────┴───────────┴───────────┴───────────┴────────────┤
│     │              Arc Entity (5 contiguous joins)                │
└───────────────────────────────────────────────────────────────────┘

Arc Entity RenderableComp:
  instance_slot = 10     (first segment in line buffer)
  segment_count = 6
  join_slot_start = 8    (first join in point buffer)
  join_count = 5
```

---

### Example 4: Deleting an Entity

```mermaid
sequenceDiagram
    participant User
    participant Scene as scene_remove_entity()
    participant ECS as Flecs
    participant LB as Line Buffer
    participant PB as Point Buffer

    User->>Scene: scene_remove_entity(arc_entity)

    Scene->>ECS: Get RenderableComp
    Note over ECS: instance_slot = 10<br/>segment_count = 6<br/>join_slot_start = 8<br/>join_count = 5

    loop i = 0 to 5
        Scene->>LB: geom_line_batch_free(10 + i)
        Note over LB: Zero staging[10+i]<br/>Add slot to free list<br/>needs_upload = true
    end

    loop j = 0 to 4
        Scene->>PB: geom_point_batch_free(8 + j)
        Note over PB: Zero staging[8+j]<br/>Add slot to free list<br/>needs_upload = true
    end

    Scene->>ECS: ecs_world_delete_entity()
    Note over ECS: Free pick_id<br/>Free geometry allocations<br/>Remove from world

    Scene-->>User: Entity deleted
```

**Memory State After Deletion:**

```
Line Instance Buffer:
┌─────────────────────────────────────────────────────────────────────────────┐
│ ... │ Slot 10   │ Slot 11   │ Slot 12   │ Slot 13   │ Slot 14   │ Slot 15   │
│     │ (zeroed)  │ (zeroed)  │ (zeroed)  │ (zeroed)  │ (zeroed)  │ (zeroed)  │
└─────────────────────────────────────────────────────────────────────────────┘

Free list (LIFO): [10, 11, 12, 13, 14, 15, ...]
                   ▲
                   Next single-slot alloc gets 15

Point Instance Buffer:
┌───────────────────────────────────────────────────────────────────┐
│ ... │ Slot 8    │ Slot 9    │ Slot 10   │ Slot 11   │ Slot 12    │
│     │ (zeroed)  │ (zeroed)  │ (zeroed)  │ (zeroed)  │ (zeroed)   │
└───────────────────────────────────────────────────────────────────┘

Free list (LIFO): [8, 9, 10, 11, 12, ...]
```

---

## Performance Characteristics

### Why This Design is Fast

| Technique | Benefit |
|-----------|---------|
| **GPU Instancing** | Thousands of lines rendered in 1 draw call |
| **CPU Staging Buffer** | Batch updates before single GPU upload |
| **Dirty Tracking** | Only update modified entities |
| **Contiguous Slots** | Sequential memory access, cache-friendly |
| **Free List Recycling** | No memory fragmentation for single-slot entities |

### Complexity Analysis

| Operation | Time Complexity | Notes |
|-----------|-----------------|-------|
| Add single entity | O(1) | Amortized (may trigger resize) |
| Add N-segment entity | O(N) | Set N instance slots |
| Delete entity | O(1) or O(N) | Single vs multi-segment |
| Update dirty entities | O(dirty count) | Per-frame |
| GPU upload | O(total count) | Full buffer upload |
| Draw call | O(1) | Single instanced draw |

### Memory Usage

```
Line buffer: count × 40 bytes (+ staging copy)
Point buffer: count × 28 bytes (+ staging copy)

Example: 1000 lines + 500 points + 10 arcs (60 segments, 50 joins)
  Lines: (1000 + 60) × 40 = 42,400 bytes
  Points: (500 + 50) × 28 = 15,400 bytes
  Total GPU: ~58 KB (×2 for staging = ~116 KB)
```

---

## File Reference

| File | Purpose |
|------|---------|
| `src/gpu/instance_buffer.h` | Slot allocation, staging, upload |
| `src/gpu/geometry_batch.h` | Batch manager, template geometry, pipelines |
| `src/ecs/ecs_world.h` | Flecs world, component registration |
| `src/ecs/ecs_scene.h` | High-level scene API, entity creation |
| `src/components/*.h` | Component definitions |
| `src/shaders/instanced_line_shaders.h` | Line vertex/fragment shaders |
| `src/shaders/join_shaders.h` | Point/join vertex/fragment shaders |

---

## Summary

```mermaid
flowchart TB
    subgraph Create["Entity Creation"]
        C1[Create ECS entity]
        C2[Set components]
        C3[Allocate buffer slots]
        C4[Set instance data]
        C5[Mark dirty=false]
    end

    subgraph Update["Per-Frame Update"]
        U1[Query dirty entities]
        U2[Transform geometry]
        U3[Apply color overrides]
        U4[Set staging buffer]
        U5[Upload to GPU]
    end

    subgraph Render["Rendering"]
        R1[Apply pipeline]
        R2[Bind template + instances]
        R3[Instanced draw]
    end

    C1 --> C2 --> C3 --> C4 --> C5
    C5 -.-> U1
    U1 --> U2 --> U3 --> U4 --> U5
    U5 --> R1 --> R2 --> R3
```

The key insight: **separate entity logic (ECS) from rendering data (GPU buffers)**, connected by slot indices stored in `RenderableComp`. This allows:

1. ECS to manage entity lifecycle without knowing GPU details
2. GPU buffers to be efficiently batched without knowing entity semantics
3. Dirty tracking to minimize per-frame work
4. Instancing to render thousands of primitives efficiently
