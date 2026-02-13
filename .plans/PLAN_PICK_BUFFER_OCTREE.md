# Pick Buffer Octree Spatial Index — Implementation Plan

## Context

Sprint 1+2 (already implemented) added cursor-gated rebuild and per-entity NDC frustum culling. The remaining bottleneck: **when the cursor moves, we still iterate ALL N entities** to perform `clip_space_project` culling. For 1M point entities, that's 1M matrix-vector multiplies per cursor-move frame (~3ms).

**Goal:** Replace the O(N) entity iteration with an O(log N + K) octree frustum query, where K ≈ 10-100 entities whose world-space AABBs overlap the pick frustum.

**Why an octree works here:**
- Entities are overwhelmingly **static** after batch import — build-once index is viable
- Pick frustum covers ~0.01% of screen (20x20 pixels at 54-108x zoom) — spatial culling eliminates 99.99% of entities
- Dirty tracking (`TransformComp.dirty`) already exists for incremental updates
- Entity moves are rare (gizmo drag only) — O(log N) per-move update is negligible

## Data Structures

### New file: `src/gpu/pick_octree.h`

**Loose octree** with array-based node pool. "Loose" = each node's effective bounds are 2x the tight bounds, guaranteeing every entity maps to exactly ONE node (the smallest node whose tight bounds contain the entity's center). This eliminates multi-node insertions.

```c
// Axis-aligned bounding box
typedef struct { vec3_t min, max; } pick_aabb_t;

// Entity entry (flat array, linked-list per node)
typedef struct {
    uint64_t entity;        // ecs_entity_t
    pick_aabb_t aabb;       // World-space AABB
    uint32_t node_idx;      // Containing node (for O(1) removal)
    uint32_t next;          // Next entry in same node, or NULL sentinel
} pick_octree_entry_t;     // 48 bytes

// Octree node (flat array pool)
typedef struct {
    vec3_t center;          // Center of tight region
    float half_size;        // Half-size of tight region (loose = 2x)
    uint32_t children[8];   // Child indices, or NULL sentinel
    uint32_t first_entry;   // Head of entity linked list
    uint16_t entry_count;   // Entities in this node
    uint16_t depth;
    bool is_leaf;
} pick_octree_node_t;      // ~56 bytes

// The octree
typedef struct {
    pick_octree_node_t *nodes;      // Node pool
    uint32_t node_count, node_capacity;
    pick_octree_entry_t *entries;   // Entry pool
    uint32_t entry_count, entry_capacity;
    uint32_t *free_nodes;           // Free list stacks
    uint32_t *free_entries;
    uint32_t *pick_id_to_entry;     // pick_id → entry index (O(1) lookup)
    uint32_t pick_id_map_capacity;
    uint64_t *query_results;        // Reusable result buffer
    uint32_t query_result_count, query_result_capacity;
    uint32_t root;
    bool initialized;
} pick_octree_t;
```

**Memory for 1M entities:** ~48MB entries + ~2MB nodes + ~4MB pick_id map = ~54MB total.

### Frustum types (also in `pick_octree.h`)

```c
typedef struct { float a, b, c, d; } pick_frustum_plane_t;
typedef struct { pick_frustum_plane_t planes[6]; } pick_frustum_t;
```

## Key Algorithms

### 1. Entity AABB Computation

`pick_octree_compute_entity_aabb(GeometryComp *g, TransformComp *t) → pick_aabb_t`

Transforms geometry vertices by `world_matrix` and computes min/max. Per-type strategy:

| Type | Bound Computation | Notes |
|------|------------------|-------|
| GEOM_POINT | Single point + epsilon pad | Avoids zero-size AABB |
| GEOM_LINE | AABB of 2 world endpoints | Exact |
| GEOM_TRIANGLE | AABB of 3 world vertices | Exact |
| GEOM_POLYLINE | AABB of N world vertices | Exact |
| GEOM_POLYGON | AABB of N world vertices | Exact |
| GEOM_ARC | center ± radius (all axes) | Conservative (sphere bound) — avoids tessellation alloc |
| GEOM_BEZIER | AABB of 4 control points | Conservative (convex hull property of bezier) |
| GEOM_HELIX | AABB of axis endpoints ± radius | Conservative |
| GEOM_POINT_CLOUD | Iterate all points | Runs once at insert time, not per-frame |
| GEOM_MESH | Iterate all vertices | Runs once at insert time, not per-frame |

### 2. Frustum Extraction (Gribb-Hartmann)

`pick_octree_extract_frustum(mat4_t pick_mvp) → pick_frustum_t`

Extracts 6 clip planes from the pick MVP matrix (column-major):
- Left: row3 + row0, Right: row3 - row0
- Bottom: row3 + row1, Top: row3 - row1
- Near: row3 + row2, Far: row3 - row2

Normalize each plane for correct distance.

### 3. AABB-Frustum Test

`pick_aabb_in_frustum(pick_aabb_t aabb, pick_frustum_t *f) → bool`

For each of 6 planes, compute the "positive vertex" (AABB corner most aligned with plane normal). If its signed distance < 0, AABB is entirely outside that plane → outside frustum. If no plane rejects, AABB is (conservatively) inside.

### 4. Loose Octree Insert

`pick_octree_insert(tree, entity, pick_id, aabb)`

1. Compute entity center and max extent
2. Walk from root: at each node, choose child octant based on entity center
3. Stop descending when: (a) entity extent > child half_size (too large for children), (b) leaf with room, or (c) max depth reached
4. If leaf is full and not at max depth → split (redistribute existing entries into children)
5. Link entry into node's list, store `node_idx` in entry for O(1) removal

### 5. Frustum Query (iterative stack)

`pick_octree_query_frustum(tree, frustum)`

Stack-based traversal:
1. Push root onto stack
2. Pop node, compute its **loose AABB** (center ± 2 * half_size)
3. Test loose AABB against frustum → if outside, skip subtree
4. Test each entity in node against frustum → add passing entities to results
5. Push non-null children onto stack
6. Result: `query_results[]` array of entity IDs

### 6. Remove / Move

- **Remove** `pick_octree_remove(tree, pick_id)`: Lookup entry via `pick_id_to_entry`, unlink from containing node's linked list using stored `node_idx`. O(1).
- **Move** `pick_octree_move(tree, entity, pick_id, new_aabb)`: Remove + Insert. O(log N).

## Integration Points

### A. `ecs_scene_t` (ecs_scene.h:154)

Add `pick_octree_t pick_octree;` field. Init in `ecs_scene_init()`, shutdown in `ecs_scene_shutdown()`.

Root bounds: `center=(0,0,0), half_size=100000.0f` (covers ±100km — any CAD model).

### B. Entity creation — 12 `scene_add_*` functions

At the end of each function, after geometry is set and transform is computed, insert into octree:

```c
// Insert into spatial index for pick buffer
SelectableComp *sel = ecs_world_get_selectable(scene->world, e);
if (sel && sel->pick_id > 0) {
    pick_aabb_t aabb = pick_octree_compute_entity_aabb(&g, t);
    pick_octree_insert(&scene->pick_octree, (uint64_t)e, sel->pick_id, aabb);
}
```

Functions to modify (all in `ecs_scene.h`):
- `scene_add_line` (L181), `scene_add_polyline` (L218), `scene_add_polygon` (L283)
- `scene_add_point` (L346), `scene_add_arc` (L380), `scene_add_bezier` (L450)
- `scene_add_helix` (L521), `scene_add_point_cloud` (L593)
- `scene_add_triangle` (L640), `scene_add_triangle_colored` (L674)
- `scene_add_mesh` (L709), `scene_add_mesh_colored` (L761)

Note: `scene_add_mesh_quad` (L814) and `scene_add_mesh_box` (L823) delegate to `scene_add_mesh` — no change needed.
Note: `scene_add_directional_light`, `scene_add_point_light`, `scene_add_anchor` have no geometry/SelectableComp — no change needed.

### C. Entity deletion — `scene_remove_entity` (L1003)

Before `ecs_world_delete_entity()` (which frees the pick_id), remove from octree:

```c
const SelectableComp *sel = ecs_get_id(w->world, e, w->SelectableComp_id);
if (sel && sel->pick_id > 0) {
    pick_octree_remove(&scene->pick_octree, sel->pick_id);
}
```

Insert after `scene_free_entity_slots(scene, e);` (L1020), before `ecs_world_delete_entity()` (L1023).

### D. Transform updates — `ecs_scene_update_transform_recursive` (L1127)

After `transform_comp_update_with_parent(t, parent_world)` resolves the world_matrix, update octree if entity has geometry+selectable:

```c
const GeometryComp *g = ecs_get_id(w->world, e, w->GeometryComp_id);
const SelectableComp *sel = ecs_get_id(w->world, e, w->SelectableComp_id);
if (g && sel && sel->pick_id > 0) {
    pick_aabb_t aabb = pick_octree_compute_entity_aabb(g, t);
    pick_octree_move(&scene->pick_octree, (uint64_t)e, sel->pick_id, aabb);
}
```

This only fires for dirty entities (already O(dirty), not O(N)).

### E. Replace `ecs_scene_populate_pick_buffer` (L1625)

Replace the body with octree-queried iteration:

```c
static inline void ecs_scene_populate_pick_buffer(
    ecs_scene_t *scene, pick_buffer_t *pb, mat4_t pick_mvp)
{
    if (!scene->visible) return;

    // Extract frustum from pick MVP and query octree
    pick_frustum_t frustum = pick_octree_extract_frustum(pick_mvp);
    pick_octree_query_frustum(&scene->pick_octree, &frustum);

    // Process only entities returned by octree query (~10-100 instead of 1M)
    ecs_world_state_t *w = scene->world;
    for (uint32_t qi = 0; qi < scene->pick_octree.query_result_count; qi++) {
        ecs_entity_t e = (ecs_entity_t)scene->pick_octree.query_results[qi];
        if (!ecs_is_alive(w->world, e)) continue;

        // Fetch components (random access — acceptable for ~100 entities)
        const GeometryComp *g = ecs_get_id(w->world, e, w->GeometryComp_id);
        const RenderableComp *r = ecs_get_id(w->world, e, w->RenderableComp_id);
        const SelectableComp *s = ecs_get_id(w->world, e, w->SelectableComp_id);
        const TransformComp *t = ecs_get_id(w->world, e, w->TransformComp_id);
        if (!g || !r || !s || !t) continue;
        if (!r->visible || !s->pickable || s->pick_id == 0) continue;

        // Same per-type switch as current code, but WITHOUT NDC culling
        // (octree already culled spatially)
        switch (g->type) { /* ... existing geometry submission code ... */ }
    }
}
```

**Defense-in-depth:** Optionally keep the existing NDC AABB culling as a secondary filter inside the switch cases. This is cheap for ~100 entities and catches any octree imprecision. Can be removed later once the octree is proven correct.

### F. `app.c` — No additional changes needed

The existing cursor gate (`pick_buffer_needs_rebuild`) and MVP passing already work. The octree is transparent to app.c.

## Configuration Constants

```c
#define PICK_OCTREE_MAX_DEPTH       20      // 100000 / 2^20 ≈ 0.1 unit min resolution
#define PICK_OCTREE_LEAF_CAPACITY   64      // Split when a leaf exceeds this
#define PICK_OCTREE_INITIAL_NODES   4096    // Pre-allocated node pool
#define PICK_OCTREE_INITIAL_ENTRIES 8192    // Pre-allocated entry pool
#define PICK_OCTREE_QUERY_STACK     256     // Max traversal stack depth
#define PICK_OCTREE_NULL            0xFFFFFFFF
```

## Edge Cases

| Case | Handling |
|------|----------|
| Entity behind camera | Near frustum plane rejects it |
| Degenerate AABB (single point) | Padded by epsilon at AABB computation |
| Huge entity (mesh spanning scene) | Stored high in tree (root/near-root), always visited |
| Entity outside octree root bounds | Clamp to root — stays at root level, always visited |
| Import in progress (ImportPending) | Entities inserted into octree at creation. Populate function checks `r->visible` which handles pending state |
| Octree empty (no entities) | Query returns 0 results, no iteration |

## Expected Performance

| Scenario | Before (Sprint 1) | After (Octree) |
|----------|-------------------|----------------|
| 1M points, cursor stationary | 0ms (cursor gate) | 0ms (cursor gate) |
| 1M points, cursor moving | ~3ms (1M projections) | ~0.01ms (traverse ~30 nodes, test ~100 entities) |
| 1M points, cursor moving fast | ~3ms | ~0.01ms |
| Normal scene (<10k) | ~0.05ms | ~0.01ms |

## Implementation Sprints

### Sprint A: Core octree data structure
Create `src/gpu/pick_octree.h` with all types, pool management, AABB computation for all 10 geometry types, insert/remove/split/move, frustum extraction, AABB-frustum test, frustum query.

### Sprint B: Integration into ecs_scene.h
- Add `pick_octree_t` to `ecs_scene_t`, init/shutdown
- Insert hooks in 12 `scene_add_*` functions
- Remove hook in `scene_remove_entity`
- Move hook in `ecs_scene_update_transform_recursive`

### Sprint C: Replace populate function
- Rewrite `ecs_scene_populate_pick_buffer()` to use octree query
- Keep NDC culling as optional secondary filter

### Sprint D: Build, test, tune
- Build and verify: `cmake -B build -G Ninja && ninja -C build`
- Test 1: Import 1M PLY as individual points → FPS should be at display cap with moving cursor
- Test 2: Hover over polylines/beziers at edges → no missed picks
- Test 3: Hover over triangles partially in pick viewport → no missed picks
- Test 4: Gizmo drag entities → hover still works correctly
- Test 5: Create/delete entities → no crashes or stale picks
- Tune `LEAF_CAPACITY` and `MAX_DEPTH` if profiling shows room for improvement

## Files Summary

| File | Action | Purpose |
|------|--------|---------|
| `src/gpu/pick_octree.h` | **CREATE** | Entire octree implementation |
| `src/ecs/ecs_scene.h` | MODIFY | Add octree field, insert/remove/move hooks in ~15 functions, replace populate_pick_buffer |
| `src/components/component_types.h` | (no change) | Already has `mat4_transform_point` used by AABB computation |
| `src/app.c` | (no change) | Cursor gate + MVP passing already done |
