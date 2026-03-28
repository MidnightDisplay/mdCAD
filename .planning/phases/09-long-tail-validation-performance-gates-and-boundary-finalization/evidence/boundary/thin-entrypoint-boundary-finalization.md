# Thin Entrypoint Boundary Finalization (TRED-02)

## Purpose

Finalize the Phase 9 `TRED-02` boundary contract by explicitly documenting the remaining `src/math3d.h` interaction surface as intentionally minimal, with consumer-traceable rationale and deferred-removal posture.

## Scope

- In scope: retained legacy interaction symbols and their concrete consumers.
- In scope: removable-vs-deferred classification with rationale.
- Out of scope: broad repo-wide `math3d.h` removal and platform-expansion changes.

## Symbol inventory proof commands

```powershell
rg -n "ray_from_screen\(|ray_axis_closest_t\(|ray_plane_intersect\(" src
rg -n "mdcad_interaction_screen_ray_from_viewport|mdcad_interaction_ray_axis_closest_t|mdcad_interaction_ray_plane_intersect" src
```

## Retained boundary (intentional minimal surface)

| Symbol | Location | Current consumer proof | Rationale |
|---|---|---|---|
| `ray_from_screen` | `src/math3d.h` | `src/math_harness.c:545,1133` | Retained for legacy-vs-candidate compare harness coverage and parity evidence continuity. |
| `ray_axis_closest_t` | `src/math3d.h` | `src/math_harness.c:633,634,637,976,977` | Retained for strict compare case parity against `mdcad_interaction_ray_axis_closest_t`. |
| `ray_plane_intersect` | `src/math3d.h` | `src/math_harness.c:685,686,687,1012,1019` | Retained for strict compare case parity against `mdcad_interaction_ray_plane_intersect`. |

### Retained-surface alignment to project-owned boundary

- Runtime/editor interaction consumers are on project-owned candidate APIs:
  - `mdcad_interaction_screen_ray_from_viewport` (e.g., `src/app.c`)
  - `mdcad_interaction_ray_axis_closest_t` and `mdcad_interaction_ray_plane_intersect` (e.g., `src/gizmo/gizmo.h`)
- Legacy symbols remain to support evidence continuity in `src/math_harness.c`, not as preferred runtime interfaces.

## Removable remnants

| Symbol/File | Status | Consumer proof | Removal note |
|---|---|---|---|
| None identified for immediate safe removal in this plan slice | N/A | `rg` inventory shows all three deferred symbols still consumed by harness compare cases | Removing now would invalidate existing parity compare coverage and exceeds this plan’s documentation-finalization scope. |

## Deferred removals

| Symbol | Why deferred | Required follow-up |
|---|---|---|
| `ray_from_screen` | Compare harness still exercises legacy parity path. | Introduce harness strategy that no longer depends on legacy implementation while preserving parity guarantees. |
| `ray_axis_closest_t` | Compare case set uses legacy/candidate paired assertions. | Replace legacy-reference assertions with stable golden/reference vectors under project-owned boundary. |
| `ray_plane_intersect` | Compare case set uses legacy/candidate paired assertions. | Same as above; migrate harness assertions to non-legacy reference strategy before symbol removal. |

## Closure statement

`TRED-02` is satisfied for Phase 9 by making the retained thin-entrypoint interaction surface explicit, minimal (three deprecated symbols), and consumer-traceable. The retained set is intentional and bounded to compare-harness compatibility, while runtime/editor paths are aligned to project-owned `mdcad_interaction_*` boundaries.
