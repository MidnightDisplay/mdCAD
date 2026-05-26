# Pick Buffer Performance Optimization Plan

## Context

**Problem:** When importing 1M+ individual ECS entities (e.g., PLY point cloud as individual points), FPS drops from 60/144 to 30-40. This does NOT happen when importing the same data as a single point-cloud entity (which uses only 1 pick ID and samples max 1000 points for picking).

**Root Cause:** `ecs_scene_populate_pick_buffer()` rebuilds the entire pick instance buffer **every frame**:
1. `instance_buffer_clear()` — memsets the entire staging buffer (~24MB for 1M points)
2. Iterates ALL entities via ECS query — O(N)
3. Transforms every vertex to world space — O(N)
4. Allocates slots + copies instance data — O(N) allocs + ~24MB writes
5. Uploads entire staging buffer to GPU — ~24MB DMA transfer
6. GPU renders all N instances into 20x20 pixels

**Why visual rendering is fast:** The visual instance buffers are **persistent** — populated once at entity creation, only re-uploaded when dirty. The pick buffer lacks this pattern.

**Key observation:** The pick buffer renders only a 20x20 pixel region around the cursor. When the cursor is stationary (the common case), the result doesn't change. Even when moving, 99.9%+ of entities project outside this tiny viewport.

## Strategy: Two-Phase Optimization

### Sprint 1: Cursor-Gated Rebuild + Screen-Space Frustum Culling (Quick Win)

**Goal:** Reduce per-frame CPU cost from O(N) to near-zero for stationary cursor, and from O(N) to O(N) cheap + O(visible) expensive for moving cursor.

**Changes:**

#### 1a. Cursor Movement Gate (`pick_buffer.h`)
- Track previous cursor position (`prev_center_x`, `prev_center_y`) in `pick_buffer_t`
- Add `bool needs_rebuild` flag
- In `pick_buffer_set_center()`: set `needs_rebuild = true` only if cursor moved > 1 pixel (configurable threshold)
- Also set `needs_rebuild = true` when scene changes (entity added/removed/moved) via a new `pick_buffer_invalidate()` function
- In `app.c`: skip the entire populate → render → readback cycle when `!needs_rebuild`
- **Benefit:** Static cursor = zero pick buffer CPU cost. This alone fixes the "hover and observe low FPS" case.

#### 1b. Screen-Space Frustum Culling (`ecs_scene.h`)
- Pass the pick MVP matrix into `ecs_scene_populate_pick_buffer()`
- Before adding any entity to the pick buffer, project its representative point(s) through the pick MVP to clip space
- **Cull rule:** If all projected points have `ndc_x > margin` OR `ndc_x < -margin` OR `ndc_y > margin` OR `ndc_y < -margin`, skip the entity
- Margin values (accounting for point/line radius): `1.5` for points, `2.0` for lines/triangles

**Per-geometry-type culling logic:**

| Type | Cull Approach | False Negative Risk |
|------|--------------|-------------------|
| **Point** | Project center to clip space, check NDC bounds | None (exact) |
| **Line** | Project both endpoints, check if AABB of projected coords overlaps NDC box | None (conservative) |
| **Triangle** | Project all 3 vertices, check if AABB overlaps NDC box | None (conservative) |
| **Polyline/Arc/Bezier/Helix** | Project all tessellated vertices, check AABB | None (conservative) |
| **Point Cloud** | Already sampled to max 1000 — add a center-point pre-cull | Minimal |
| **Mesh** | Project mesh bounding box or sample vertices | Minimal |

**Why AABB culling is safe for lines/triangles:**
- A line with both endpoints on the SAME side of a clip plane (e.g., both `ndc_x > 2.0`) cannot intersect the viewport
- If endpoints straddle the viewport (one left, one right), the AABB spans the viewport → NOT culled → correct
- Same logic extends to triangles with 3 vertices

**Helper function to add to `component_types.h`:**
```c
// Project world-space point to pick buffer clip space
// Returns false if behind camera (w <= 0)
static inline bool clip_space_project(mat4_t mvp, vec3_t p, float *out_ndc_x, float *out_ndc_y) {
    float cx = mvp.m[0]*p.x + mvp.m[4]*p.y + mvp.m[8]*p.z  + mvp.m[12];
    float cy = mvp.m[1]*p.x + mvp.m[5]*p.y + mvp.m[9]*p.z  + mvp.m[13];
    float cw = mvp.m[3]*p.x + mvp.m[7]*p.y + mvp.m[11]*p.z + mvp.m[15];
    if (cw <= 0.0f) return false;
    float inv_w = 1.0f / cw;
    *out_ndc_x = cx * inv_w;
    *out_ndc_y = cy * inv_w;
    return true;
}
```

**Deliverables:**
- FPS stays at display refresh rate when cursor is stationary (the most common case)
- FPS significantly improved when cursor is moving (only ~10-100 entities pass culling out of 1M)
- No regressions — culling is conservative, no geometry type is missed
- Testable: Import 1M PLY as individual points, observe FPS in FPS debug window

**Files to modify:**
- `src/gpu/pick_buffer.h` — cursor gate, invalidation, pass MVP out
- `src/ecs/ecs_scene.h` — frustum culling in `ecs_scene_populate_pick_buffer()`
- `src/components/component_types.h` — `clip_space_project()` helper
- `src/app.c` — wire up cursor gate, pass MVP, call invalidate on scene changes

---

### Sprint 2: Scene-Change Invalidation Hooks

**Goal:** Ensure the cursor gate from Sprint 1 correctly invalidates when the scene changes.

**Changes:**
- Call `pick_buffer_invalidate()` when:
  - Entity is created or deleted (from `scene_add_*` / `scene_remove_entity`)
  - Entity transform changes (from `scene_set_position` / gizmo drag)
  - Entity visibility toggled
  - Camera moves (view/proj matrix changes)
- Track previous view+proj hash or camera position to detect camera movement
- **Camera movement note:** Camera changes ALWAYS require rebuild since the pick MVP depends on view+proj. So the gate should also compare view/proj matrices (or just the camera position/orientation).

**Deliverables:**
- Pick buffer correctly updates when user interacts with the scene
- No stale hover highlights
- Testable: Move entities with gizmo, verify hover still works correctly

**Files to modify:**
- `src/gpu/pick_buffer.h` — add camera hash tracking
- `src/app.c` — pass camera state to pick buffer, invalidate on changes
- `src/ecs/ecs_scene.h` — invalidate on entity changes (optional: only needed if cursor gate is aggressive)

---

### Sprint 3 (Optional/Future): Persistent Pick Instance Buffer

**Goal:** Eliminate per-frame CPU cost entirely, even when cursor moves.

**Approach:** Mirror the visual rendering pattern — maintain persistent pick instance slots allocated at entity creation time. Only update on entity changes. The GPU draws all instances every frame but vertex processing of 1M instances into 20x20 pixels is <1ms.

**Why this is Sprint 3 (not Sprint 1):** Sprint 1+2 should be sufficient for the target performance. This sprint is reserved if profiling shows the remaining per-cursor-move cost is still too high. It's also more invasive (requires changes to all `scene_add_*` functions and `RenderableComp`).

---

## Expected Performance After Sprint 1+2

| Scenario | Before | After |
|----------|--------|-------|
| 1M points, cursor stationary | 30-40 FPS | 60/144 FPS (display cap) |
| 1M points, cursor moving slowly | 30-40 FPS | ~55-60 FPS (minor dip from culled rebuild) |
| 1M points, cursor moving fast | 30-40 FPS | ~50-60 FPS |
| Normal scene (<10k entities) | 60 FPS | 60 FPS (no change) |

## Verification Plan

1. **Build:** `cmake -B build -G Ninja && ninja -C build`
2. **Test 1 — Baseline:** Import 1M PLY as individual points. Observe FPS with stationary cursor → should be at display cap.
3. **Test 2 — Moving cursor:** Move cursor slowly over the point cloud → FPS should stay high.
4. **Test 3 — No regression on lines:** Create polylines/beziers, verify hover detection still works at edges where vertices are outside pick buffer but line passes through.
5. **Test 4 — No regression on triangles:** Import mesh as individual triangles, verify hover works for triangles that partially intersect the pick viewport.
6. **Test 5 — Scene changes:** Move entities with gizmo while hovering → verify hover updates correctly.
7. **Test 6 — Gizmo picks:** Verify gizmo handles still respond to hover/click with 1M entities in scene.
