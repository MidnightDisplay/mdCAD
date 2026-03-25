# Phase 4: Interaction Math and API Expansion - Research

**Researched:** 2026-03-25
**Domain:** Direct `cglm` migration of pick/unproject/ray + gizmo drag math, with scoped helper expansion under `src/math/`
**Confidence:** HIGH

## User Constraints (from 04-CONTEXT.md)

### Locked Decisions
- Migrate pick/unproject/ray and gizmo drag math through a shared project-owned `cglm` helper boundary; do not keep divergent per-call-site logic in `src/app.c`.
- Keep parity-first behavior for interaction feel and accuracy during migration.
- Migrate gizmo axis/plane drag and vertex-mode world/local delta math through shared helpers.
- Expand helper surface in this phase to include project/unproject, ray-axis/plane, and quaternion-capable coverage.
- Keep helper additions thin; do not introduce vendor type aliases or a compatibility facade.
- Retire equivalent `src/math3d.h` helpers only in migrated interaction slices, not repo-wide.

### Required IDs This Phase Must Close
- `HOT-03`: pick/unproject/ray + translation gizmo drag math runs on new foundation without visible regressions (macOS gate).
- `EXP-01`: expose quaternion and broader helper capability for future work.
- `EXP-02`: migrated subsystems retire equivalent `src/math3d.h` helpers.

## Summary

Plan Phase 4 as three bounded slices aligned to roadmap plans:

1. **04-01 (HOT-03 pick path):** move screen-ray/unproject and pick MVP interaction math into one `src/math/` helper boundary and route `src/app.c` + `src/gpu/pick_buffer.h` through it.
2. **04-02 (HOT-03 gizmo path):** move gizmo axis/plane intersection and vertex world/local delta conversion to the same helper boundary and remove direct dependence on `math3d` ray helpers in gizmo interaction code.
3. **04-03 (EXP-01 + EXP-02):** add quaternion-capable helpers under `src/math/`, retire equivalent `math3d` helpers in migrated slices, and extend harness compare/bench coverage to interaction math.

Primary planning principle: **migrate compute paths first, keep storage stable where possible (`vec3_t`/`mat4_t`), and keep bridge points explicit**.

## Standard Stack

### Core
| Library / Tool | Version | Purpose in Phase 4 | Why Standard |
|----------------|---------|--------------------|--------------|
| `cglm` via `src/math/cglm_entry.h` | `0.9.6` | Unproject/project, matrix inverse/mul, ray math, quaternion helpers | Locked backend and contract entrypoint |
| `cglm` struct API (`glms_*`) | `0.9.6` | Value-style interaction helpers (screen ray/unproject, ray-plane/axis) | Fits app/gizmo interaction call shape |
| `cglm` array API (`glm_*`) | `0.9.6` | Raw buffer operations in pick math and matrix/vector transforms | Fits existing `mat4_t` storage bridges |
| `mdcad_math_harness` | repo-local | Compare + bench gates for migrated interaction math | Existing migration gate already used in Phases 2-3 |

### Supporting
| Tool / Module | Purpose | Planning Note |
|---------------|---------|---------------|
| `src/math/cglm_entry.h` | Enforce RH_ZO clip-control contract | Include in all new interaction helper headers |
| `src/math_harness.c` | Existing `screen-ray-unproject` parity + bench scaffold | Extend with pick/gizmo-specific cases |
| `src/app.c` | Current interaction orchestrator and duplicate ray construction sites | Route both drag begin/update through one helper call |
| `src/gpu/pick_buffer.h` | Current pick MVP math (`mat4_identity` + `mat4_mul`) | Move projection-windowing math behind shared helper |

## Architecture Patterns

### Pattern 1: Shared Interaction Math Boundary Under `src/math/`
Create one thin helper module (or two focused headers) in `src/math/` for interaction math:
- `screen -> ray` (viewport coords + view/proj to world ray)
- pick-window projection composition
- ray-axis closest point
- ray-plane intersection
- world-delta to local-delta conversion for vertex mode

This matches Decision `D-01/D-04/D-07` and removes duplicated logic currently split across `src/app.c`, `src/gizmo/gizmo.h`, and `src/gizmo/gizmo_vertex_mode.h`.

### Pattern 2: One Ray Construction Path in `src/app.c`
`src/app.c` currently computes `inv_vp + ray_from_screen(...)` in both drag begin and drag update blocks. Replace with one helper path and reuse it in both sites so interaction parity changes are isolated to one implementation.

### Pattern 3: Keep `mat4_t`/`vec3_t` Runtime Storage Stable During This Slice
Like Phase 3, prefer compute migration over storage migration:
- Keep subsystem-facing types stable where already used (`pick_params_t`, gizmo state, ECS transform storage).
- Convert at helper boundaries via explicit bridge copies, not scattered ad hoc conversions.

### Pattern 4: Quaternion Expansion as Helper Surface, Not Subsystem Rewrite
To satisfy `EXP-01`, add quaternion-capable helper API in `src/math/` that is ready for future use, but do not force gizmo/camera storage rewrites in this phase. This keeps scope bounded while delivering the requirement.

## Don't Hand-Roll

| Problem | Do Not Build | Use Instead | Why |
|---------|--------------|-------------|-----|
| Screen-ray generation | New per-call-site `ndc + inverse-vp` code | Shared helper wrapping `cglm` unproject/project math | Prevents drift between drag begin/update and future callsites |
| Ray-axis/plane drag math | New local re-implementations in gizmo files | Shared helper called by `gizmo_begin_drag`/`gizmo_update_drag` | Keeps interaction behavior consistent |
| Quaternion support | Custom quaternion type/facade | Thin helper functions over `cglm` quaternion ops | Meets `EXP-01` without wrapper bloat |
| Legacy retirement | Repo-wide purge of `math3d.h` in Phase 4 | Scoped retirement in migrated interaction slices only | Matches staged migration boundary and avoids blast radius |

## Common Pitfalls

### Pitfall 1: Clip-Depth / Unproject Convention Drift
Current migration contract is RH + ZO. Any helper that silently mixes legacy clip assumptions will break selection feel. Keep all new interaction helpers anchored to `cglm_entry.h` and validate behavior with harness cases, not raw matrix equality.

### Pitfall 2: Duplicated Pick-MVP Logic
`pick_buffer_compute_mvp(...)` is used for both culling population and render. If planning splits or duplicates this math, frustum culling and rendered pick IDs can diverge.

### Pitfall 3: Degenerate Intersection Handling Regressions
`ray_axis_closest_t` and `ray_plane_intersect` currently return safe defaults on parallel/degenerate cases. Preserve this behavior contract (no NaN propagation, stable "no move" behavior).

### Pitfall 4: Vertex Delta Conversion Drift
`gizmo_vertex_mode_apply_delta(...)` currently depends on world-matrix inverse and manual direction transform. Migration must preserve local-space delta semantics for non-uniform scale and rotated transforms.

### Pitfall 5: Scope Expansion Past Phase Boundary
Do not couple Phase 4 to broad `math3d.h` cleanup or global type rewrites. Only retire helpers proven redundant inside migrated interaction slices (`app`, `pick_buffer`, `gizmo` interaction paths, and corresponding harness baselines).

## Implementation Map (Planning Inputs)

### Existing Hotspots to Refactor
- `src/app.c`
  - Replace drag begin/update `inv_vp + ray_from_screen(...)` blocks with shared helper call.
- `src/gpu/pick_buffer.h`
  - Route `pick_buffer_compute_mvp(...)` math through shared interaction helper (or helper called by this function).
- `src/gizmo/gizmo.h`
  - Replace direct `ray_axis_closest_t` / `ray_plane_intersect` usage with shared helper path.
- `src/gizmo/gizmo_vertex_mode.h`
  - Replace direct `mat4_inverse` + manual delta transform with shared helper.

### New/Expanded Helper Surface Under `src/math/`
- Recommended modules:
  - `src/math/math_interaction.h` (screen-ray, pick MVP, ray-axis/plane, world/local delta helpers)
  - `src/math/math_quat.h` (quaternion-capable expansion helpers for `EXP-01`)
- Keep helpers thin and explicit; no new vendor typedef aliases.

### Legacy Retirement Targets (`EXP-02`) for This Phase
Retire equivalent `math3d` helper dependence in migrated slices:
- `ray_from_screen`
- `ray_axis_closest_t`
- `ray_plane_intersect`
- interaction-path `mat4_inverse` usage where replaced by new helper surface

## Validation Architecture

Automated gate remains harness-first, then manual viewport smoke.

### Harness additions to plan
- Compare cases:
  - pick MVP parity (legacy baseline vs migrated helper behavior-level checks)
  - screen ray construction parity from viewport sample points
  - axis drag delta progression parity
  - plane drag delta progression parity
  - vertex world->local delta conversion parity
  - quaternion helper sanity (normalization/rotation invariants)
- Bench cases:
  - migrated interaction ray path
  - migrated axis/plane intersection path
  - migrated world/local delta conversion path

### Manual macOS parity checks to include in plan
- Hover + click selection accuracy near thin lines/points and dense geometry
- Gizmo axis drag smoothness and no jump at drag start
- Gizmo plane drag behavior across camera angles
- Geometry mode vertex drag correctness (including rotated/scaled entity transforms)
- Undo/redo correctness after drag operations

## Code Examples

### Example: Single shared screen-ray helper usage in app interaction path
```c
ray_t mouse_ray = mdcad_interaction_screen_ray_from_viewport(
    vp_x, vp_y,
    vp_width, vp_height,
    view_legacy, proj_legacy);
```

### Example: Shared gizmo axis/plane drag helpers
```c
float t_axis = mdcad_interaction_ray_axis_closest_t(mouse_ray, drag_origin, axis_dir);
bool hit = mdcad_interaction_ray_plane_intersect(mouse_ray, drag_origin, plane_n, &hit_point);
```

### Example: Quaternion-capable helper surface (EXP-01)
```c
versors q = mdcad_quat_from_axis_angle(axis, radians);
vec3_t rotated = mdcad_quat_rotate_vec3(q, input_vec);
```

## Plan-Readiness Checklist

- [x] Migration seams and files are identified.
- [x] `HOT-03`, `EXP-01`, and `EXP-02` are mapped to concrete deliverables.
- [x] Validation strategy includes both compare and bench gates plus manual smoke.
- [x] Scope boundaries match Phase 4 context decisions.

---
*Research completed: 2026-03-25*
*Ready for planning: yes*
