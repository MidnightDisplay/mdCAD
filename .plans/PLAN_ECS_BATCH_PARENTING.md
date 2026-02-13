# Plan: ECS <-> Rendering Decoupling & Batch Parenting

## Context

When importing large PLY point clouds (1M points) in Editable Subtree mode, parenting individual point entities to an anchor entity was O(n^2). The parenting was disabled as a fallback - points now dump onto the scene without hierarchy. This plan reintroduces parenting via a batch deferred API, eliminating the quadratic cost.

### Root Cause Analysis

The O(n^2) comes from two interacting factors:

1. **Per-frame query iteration over a growing entity set**: The import creates ~500 entities/frame across ~2000 frames. Each frame, `ecs_scene_update()` queries ALL entities (even skipping non-dirty ones costs O(total) in iteration). Over 2000 frames: sum(500..1M) = O(n^2) in iteration alone.

2. **Coupling in `ecs_world_set_parent()`** (`ecs_world.h:366`): Each call does `ecs_add_pair()` (archetype change), then immediately accesses components via `ecs_get_id()` for dirty flagging, then recursively calls `ecs_world_mark_descendants_dirty()`. These cannot be batched because structural changes and data access are interleaved.

### Solution Architecture

- **Batch parenting API** using Flecs `ecs_defer_begin()/ecs_defer_end()` to group all `ecs_add_pair()` calls, then mark dirty in a single pass after flush
- **ImportPending tag** to exclude importing entities from per-frame update queries (eliminates O(n^2) iteration)
- **Transform-only anchor entities** for PLY and JSONL imports (no geometry, no GPU slots wasted)
- **PLY import parenting phase** that collects entity IDs during creation, then batch-parents after all entities exist
- **JSONL import migration** to use ImportPending + transform-only anchors + batch parenting
- **Scene serializer migration** to batch parenting

### Success Criteria

1. No new regressions in existing features
2. Deferred ECS operations work without runtime crashes
3. Large-scale parenting is O(n) not O(n^2)

### Design Decisions (confirmed with user)

- **Anchor entities**: Transform-only (TransformComp + LabelComp), no geometry, no GPU slot - for both PLY and JSONL imports
- **ImportPending scope**: Both PLY and JSONL importers
- **Scene serializer**: Include batch parenting migration in this refactor

---

## Files to Modify

| File | Sprint | Change |
|------|--------|--------|
| `src/ecs/ecs_world.h` | 1 | Add `ecs_world_set_parent_batch()`, `ecs_world_set_parents_batch()`, `ImportPending` tag, `ecs_world_create_anchor_entity()` |
| `src/ecs/ecs_scene.h` | 1 | Add `scene_set_parent_batch()`, `scene_add_anchor()`, update queries to skip ImportPending |
| `src/ply_import_job.h` | 2 | Add entity ID tracking, parenting phase, ImportPending tagging |
| `src/ui/ui_scene_hierarchy.h` | 2 | Update import UI for new PLY parenting phase |
| `src/jsonl_import_job.h` | 3 | Migrate to transform-only anchors, ImportPending, batch parenting |
| `src/scene_serializer.h` | 4 | Migrate parenting loop to batch API |

### Key Flecs APIs to Use (verified in `vendors/flecs/flecs.h`)

- `ecs_defer_begin(world)` / `ecs_defer_end(world)` - queue operations, flush in batch
- `ecs_get_id()` - safe during defer mode (reads current, not deferred data)
- `ecs_entity(world, { .name = "..." })` - zero-size tag component

---

## Sprint 1: Infrastructure (Batch API + ImportPending + Anchor Entity)

**Goal**: Add deferred batch parenting, ImportPending tag, and transform-only anchor helper

### 1.1 Add `ImportPending` tag to `ecs_world_state_t` (`ecs_world.h`)

Add field:
```c
ecs_entity_t ImportPending_tag;  // Entities being imported (excluded from update)
```

Register in `ecs_world_init()`:
```c
s->ImportPending_tag = ecs_entity(s->world, { .name = "ImportPending" });
```

### 1.2 Add `ecs_world_create_anchor_entity()` (`ecs_world.h`)

Creates a lightweight entity with only TransformComp (no geometry, no renderable, no GPU slot):
```c
static inline ecs_entity_t ecs_world_create_anchor_entity(ecs_world_state_t *s) {
    ecs_entity_t e = ecs_new(s->world);
    TransformComp t = transform_comp_default();
    ecs_set_id(s->world, e, s->TransformComp_id, sizeof(TransformComp), &t);
    return e;
}
```

### 1.3 Add `scene_add_anchor()` (`ecs_scene.h`)

Scene-level wrapper:
```c
static inline ecs_entity_t scene_add_anchor(ecs_scene_t *scene,
                                              const char *name, const char *desc) {
    ecs_entity_t e = ecs_world_create_anchor_entity(scene->world);
    if (name || desc) {
        LabelComp label = label_comp_make(name ? name : "", desc ? desc : "");
        ecs_world_set_label(scene->world, e, &label);
    }
    return e;
}
```

### 1.4 Add `ecs_world_set_parent_batch()` (`ecs_world.h`)

Single-parent batch (all children → one parent):
```c
static inline void ecs_world_set_parent_batch(
    ecs_world_state_t *s,
    ecs_entity_t *children, int count,
    ecs_entity_t parent)
{
    if (count <= 0 || parent == 0 || !ecs_is_alive(s->world, parent)) return;

    // Phase 1: Batch structural changes (deferred)
    ecs_defer_begin(s->world);
    for (int i = 0; i < count; i++) {
        if (!ecs_is_alive(s->world, children[i])) continue;
        ecs_entity_t current = ecs_get_parent(s->world, children[i]);
        if (current != 0) {
            ecs_remove_pair(s->world, children[i], EcsChildOf, current);
        }
        ecs_add_pair(s->world, children[i], EcsChildOf, parent);
    }
    ecs_defer_end(s->world);

    // Phase 2: Mark dirty (after flush - pointers are valid now)
    for (int i = 0; i < count; i++) {
        if (!ecs_is_alive(s->world, children[i])) continue;
        TransformComp *t = ecs_world_get_transform(s, children[i]);
        if (t) t->dirty = true;
        RenderableComp *r = ecs_world_get_renderable(s, children[i]);
        if (r) r->instance_dirty = true;
    }
}
```

### 1.5 Add `ecs_world_set_parents_batch()` (`ecs_world.h`)

Multi-parent batch (each child → its own parent, for scene serializer):
```c
static inline void ecs_world_set_parents_batch(
    ecs_world_state_t *s,
    ecs_entity_t *children, ecs_entity_t *parents, int count)
{
    if (count <= 0) return;

    // Phase 1: Batch structural changes
    ecs_defer_begin(s->world);
    for (int i = 0; i < count; i++) {
        if (!ecs_is_alive(s->world, children[i])) continue;
        if (parents[i] == 0 || !ecs_is_alive(s->world, parents[i])) continue;
        ecs_entity_t current = ecs_get_parent(s->world, children[i]);
        if (current != 0) {
            ecs_remove_pair(s->world, children[i], EcsChildOf, current);
        }
        ecs_add_pair(s->world, children[i], EcsChildOf, parents[i]);
    }
    ecs_defer_end(s->world);

    // Phase 2: Mark dirty
    for (int i = 0; i < count; i++) {
        if (!ecs_is_alive(s->world, children[i])) continue;
        TransformComp *t = ecs_world_get_transform(s, children[i]);
        if (t) t->dirty = true;
        RenderableComp *r = ecs_world_get_renderable(s, children[i]);
        if (r) r->instance_dirty = true;
    }
}
```

### 1.6 Add `scene_set_parent_batch()` and `scene_set_parents_batch()` (`ecs_scene.h`)

Thin wrappers:
```c
static inline void scene_set_parent_batch(ecs_scene_t *scene,
    ecs_entity_t *children, int count, ecs_entity_t parent) {
    ecs_world_set_parent_batch(scene->world, children, count, parent);
}

static inline void scene_set_parents_batch(ecs_scene_t *scene,
    ecs_entity_t *children, ecs_entity_t *parents, int count) {
    ecs_world_set_parents_batch(scene->world, children, parents, count);
}
```

### 1.7 Update update queries to skip ImportPending (`ecs_scene.h`)

In `ecs_scene_update_transforms()` (~line 1142):
```c
ecs_query_t *q = ecs_query(w->world, {
    .terms = {
        { .id = w->TransformComp_id },
        { .id = w->ImportPending_tag, .oper = EcsNot }
    }
});
```

In `ecs_scene_update()` main query (~line 1213):
```c
ecs_query_t *q = ecs_query(w->world, {
    .terms = {
        { .id = w->GeometryComp_id },
        { .id = w->RenderableComp_id },
        { .id = w->TransformComp_id },
        { .id = w->ImportPending_tag, .oper = EcsNot }
    }
});
```

### 1.8 Test Deliverables

- Build: `cmake -B build -G Ninja && ninja -C build`
- Run: `./build/bin/mdCAD`
- Create entities via Add menu (point, line, arc) - verify no regression from ImportPending query change
- Parent entities via drag-drop in Scene Hierarchy (existing `scene_set_parent` path still works)
- Verify transforms propagate: move parent, children follow
- Verify undo/redo of parenting still works
- Verify GPU picking on parented entities

---

## Sprint 2: PLY Import Parenting Reintroduction

**Goal**: Track created entity IDs, add ImportPending tagging, batch-parent to transform-only anchor

### 2.1 Add entity tracking + anchor to `ply_import_job_t` (`ply_import_job.h`)

New fields:
```c
ecs_entity_t *created_entities;  // Array of created entity IDs
int created_capacity;            // Capacity of array
ecs_entity_t anchor_entity;      // Transform-only parent anchor
```

Allocate in `ply_import_job_start()`:
```c
job->created_entities = malloc(job->total_points * sizeof(ecs_entity_t));
job->created_capacity = job->total_points;
job->anchor_entity = 0;
```

Free in completion/cancellation/reset handlers.

### 2.2 Add new job state: `PLY_JOB_PARENTING`

```c
typedef enum {
    PLY_JOB_IDLE = 0,
    PLY_JOB_PARSING_VERTICES,
    PLY_JOB_CREATING_ENTITIES,
    PLY_JOB_PARENTING,          // NEW: batch-parent to anchor
    PLY_JOB_COMPLETE,
    PLY_JOB_CANCELLED,
    PLY_JOB_ERROR
} ply_job_state_t;
```

Update `ply_import_job_is_running()` to include `PLY_JOB_PARENTING`.

### 2.3 Tag entities with ImportPending during creation

In the `PLY_JOB_CREATING_ENTITIES` handler, after `scene_add_point()`:
```c
ecs_entity_t pt = scene_add_point(scene, ...);
if (pt != 0) {
    // Tag as import-pending (excluded from update loop)
    ecs_add_id(scene->world->world, pt, scene->world->ImportPending_tag);
    job->created_entities[job->created_count + i] = pt;
    actually_created++;
}
```

### 2.4 Implement `PLY_JOB_PARENTING` state handler

After entity creation completes, transition to parenting state:
```c
if (job->state == PLY_JOB_PARENTING) {
    // Create transform-only anchor
    job->anchor_entity = scene_add_anchor(scene, filename, "PLY import");

    // Batch-parent all entities + remove ImportPending in one defer block
    ecs_world_state_t *w = scene->world;
    ecs_defer_begin(w->world);
    for (int i = 0; i < job->created_count; i++) {
        ecs_add_pair(w->world, job->created_entities[i], EcsChildOf, job->anchor_entity);
        ecs_remove_id(w->world, job->created_entities[i], w->ImportPending_tag);
    }
    ecs_defer_end(w->world);

    // Mark all dirty (after flush)
    for (int i = 0; i < job->created_count; i++) {
        TransformComp *t = ecs_world_get_transform(w, job->created_entities[i]);
        if (t) t->dirty = true;
        RenderableComp *r = ecs_world_get_renderable(w, job->created_entities[i]);
        if (r) r->instance_dirty = true;
    }

    // Free tracking array
    free(job->created_entities);
    job->created_entities = NULL;

    job->state = PLY_JOB_COMPLETE;
}
```

Note: Both parenting AND ImportPending removal are combined in one defer block for efficiency (one archetype migration per entity, not two).

### 2.5 Update progress UI (`ui_scene_hierarchy.h`)

Update the import progress popup to show parenting phase:
- Show "Parenting..." status during `PLY_JOB_PARENTING`
- Progress bar stays at ~95% during parenting, jumps to 100% on complete

### 2.6 Test Deliverables

- Import small PLY file (100 points) in Editable Subtree mode
  - Verify all points appear in Scene Hierarchy under a named anchor
  - Verify anchor has LabelComp with filename
  - Verify moving anchor moves all points
  - Verify GPU picking works on individual points
- Import medium PLY file (10k points)
  - Verify no crash, parenting completes
  - Time the import
- Import large PLY file (100k points)
  - Verify no freeze/crash
  - Verify frame times stay constant during import (ImportPending working)
- Point Cloud mode (import_mode=0) should be unaffected

---

## Sprint 3: JSONL Import Migration

**Goal**: Migrate JSONL import to transform-only anchors, ImportPending, and batch parenting

### 3.1 Change anchor creation in JSONL import (`jsonl_import_job.h`)

**Currently** (line 500-514): Root and entry anchors use `scene_add_point()` with invisible color:
```c
job->root_entity = scene_add_point(scene, vec3_make(0,0,0), vec4_make(0,0,0,0), 0.0f);
scene_set_visible(scene, job->root_entity, false);
```

**Change to**:
```c
job->root_entity = scene_add_anchor(scene, fname, "JSONL import root");
```

Same for entry anchors (line 518-528):
```c
job->entry_entities[i] = scene_add_anchor(scene, name, desc);
```

This eliminates GPU slot waste (anchors no longer consume point buffer slots).

### 3.2 Add ImportPending tagging to JSONL entity creation

In the `JSONL_JOB_CREATING_ENTITIES` handler, after each entity is created via `scene_add_*()`, tag with ImportPending:
```c
ecs_add_id(scene->world->world, entity, scene->world->ImportPending_tag);
```

### 3.3 Migrate parenting to batch API

**Currently** (line 687-708): Individual `scene_set_parent()` calls, 500/frame chunk.

**Change to**: After all entities are created, batch-parent in one step:
```c
// Phase 1: Parent geometry entities to entry anchors
ecs_defer_begin(w->world);
for (int i = 0; i < job->all_created_count; i++) {
    ecs_entity_t child = job->all_created_entities[i];
    int entry_idx = job->entity_to_entry_map[i];
    ecs_entity_t parent = job->entry_entities[entry_idx];
    if (child && parent) {
        ecs_add_pair(w->world, child, EcsChildOf, parent);
        ecs_remove_id(w->world, child, w->ImportPending_tag);
    }
}
// Phase 2: Parent entry anchors to root
for (int i = 0; i < data->entry_count; i++) {
    if (job->entry_entities[i] && job->root_entity) {
        ecs_add_pair(w->world, job->entry_entities[i], EcsChildOf, job->root_entity);
    }
}
ecs_defer_end(w->world);

// Phase 3: Mark all dirty
for (int i = 0; i < job->all_created_count; i++) {
    // ... mark transform + renderable dirty
}
```

### 3.4 Test Deliverables

- Import JSONL file with geometry entries, verify hierarchy:
  - Root anchor → Entry anchors → Geometry entities
  - Anchors show with LabelComp name (no invisible point artifacts)
- Verify anchors don't consume GPU point buffer slots
- Verify GPU picking works on imported geometry
- Verify large JSONL files import without frame drops
- Verify mesh import modes (single mesh, individual triangles) still work

---

## Sprint 4: Scene Serializer Migration & Regression Testing

**Goal**: Apply batch parenting to scene load; full regression test pass

### 4.1 Migrate scene serializer parenting to batch API (`scene_serializer.h`)

**Currently** (line 1694-1703): Individual `scene_set_parent()` calls in a loop, with O(n) `scene_find_new_entity()` linear search per entity = O(n^2).

**Change to**: Collect all (child, parent) pairs, then call `scene_set_parents_batch()`:
```c
// Collect parent relationships
ecs_entity_t *load_children = malloc(entity_count * sizeof(ecs_entity_t));
ecs_entity_t *load_parents = malloc(entity_count * sizeof(ecs_entity_t));
int parent_count = 0;

for (int i = 0; i < entity_count; i++) {
    if (entities[i].parent_old_id != 0 && entities[i].new_entity != 0) {
        ecs_entity_t new_parent = scene_find_new_entity(entities, entity_count,
                                                         entities[i].parent_old_id);
        if (new_parent != 0) {
            load_children[parent_count] = entities[i].new_entity;
            load_parents[parent_count] = new_parent;
            parent_count++;
        }
    }
}

// Single batch call
scene_set_parents_batch(scene, load_children, load_parents, parent_count);
free(load_children);
free(load_parents);
```

Note: The `scene_find_new_entity()` O(n) lookup is still used per entity. This could be optimized with a hashmap in a future session but is not in scope here - the batch parenting alone removes the O(n*depth) dirty marking overhead.

### 4.2 Full Regression Test Checklist

| Feature | Test | Pass? |
|---------|------|-------|
| Scene save/load | Save scene with hierarchy, reload, verify structure | |
| Undo/redo parenting | Parent entity, undo, redo, verify | |
| Undo/redo delete parent | Delete parent with children, undo, verify restored | |
| GPU picking | Hover/click parented entities, verify selection | |
| Manipulator gizmo | Select parented entity, drag gizmo, verify transform | |
| Scene hierarchy drag-drop | Drag entity onto parent, verify hierarchy | |
| Scene hierarchy unparent | Right-click > Unparent, verify | |
| JSONL import | Import JSONL file, verify hierarchy + transform-only anchors | |
| PLY Point Cloud mode | Import PLY as point cloud (no parenting), verify | |
| PLY Editable mode | Import PLY as editable WITH parenting, verify hierarchy | |
| PLY Mesh import | Import PLY mesh, verify rendering | |
| Visibility toggle | Hide parent, verify children hidden | |
| Camera orbit/pan/zoom | Verify camera works during/after import | |
| Add Entity menu | Add point/line/arc/triangle/mesh, verify | |
| FPS counter | Check FPS stable during normal operation | |

### 4.3 Test Deliverables

- Save scene with 3-level hierarchy, load back, verify all relationships intact
- Import PLY 1M points with parenting: time it, verify O(n) completion
- Import PLY 100k points: verify constant frame times during import
- Run through full regression checklist above
- Compare import times: before refactor vs after

---

## Implementation Notes

### Flecs Defer Behavior (verified in `vendors/flecs/flecs.h`)

- `ecs_defer_begin(world)` / `ecs_defer_end(world)` - queue ops, flush in batch
- `ecs_get_id()` is safe during defer (reads current, not queued data)
- `ecs_add_pair()` during defer is queued, entity doesn't move until flush
- After `ecs_defer_end()`, all queued ops execute; component pointers from before may be stale

### Frame Loop Order (from `src/app.c` frame())

```
frame():
  1. UI phase (import job tick called here, inside ui_scene_hierarchy draw)
  2. ecs_world_progress()
  3. ecs_scene_update()    <- transforms + GPU sync (queries skip ImportPending)
  4. Render pass
```

Import ticks happen BEFORE `ecs_scene_update()`. With ImportPending tag, newly created entities are excluded from the update query, eliminating the O(n^2) iteration growth.

### Memory Management

- `created_entities` array: allocated at import start, freed at completion/cancellation
- For 1M entities: ~8MB (`sizeof(ecs_entity_t)` = 8 bytes) - acceptable
- JSONL already has `all_created_entities` tracking array

### Crash Prevention (Success Criterion #2)

1. **No component access during defer block**: All `ecs_get_id()` calls happen AFTER `ecs_defer_end()`. Prevents stale pointer issues from entities moving between archetype tables during flush.

2. **No iterator invalidation**: Batch parenting runs outside of any query iteration. Import ticks create entities (not during a query), then batch-parent in a separate step.

3. **Proper defer nesting**: `ecs_defer_begin/end` called in matched pairs, never nested.

4. **Entity liveness checks**: All batch functions check `ecs_is_alive()` before operating on entities.

### Backward Compatibility

- `scene_set_parent()` **unchanged** for single-entity operations (UI drag-drop, undo/redo)
- `scene_set_parent_batch()` / `scene_set_parents_batch()` are **additive** new functions
- `ImportPending` tag only active during import, removed after - no effect on non-import code paths
- Transform-only anchors are **visible in hierarchy** (via LabelComp) but don't render or consume GPU slots
- Existing `scene_add_point()` anchors in JSONL are replaced - this is intentional (no invisible point artifacts)
