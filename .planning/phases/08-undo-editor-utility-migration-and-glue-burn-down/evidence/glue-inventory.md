# Phase 8 Glue Inventory

## Removed

| Symbol/File | Scope | Runtime consumer proof (`rg -n`) | Rationale / Impact |
|---|---|---|---|
| `#include "../math3d.h"` in `src/gizmo/gizmo_vertex_mode.h` | Phase 8 runtime editor touchpoint | `rg -n "math3d\\.h|mat4_mul_point|\\bvec3_add\\b|ray_from_screen|ray_plane_intersect|ray_axis_closest_t" src/gizmo/gizmo_vertex_mode.h` (no legacy symbol matches after migration) | Safe removal after migrating center/delta/world-point paths to `mdcad_undo_editor_*`; reduces direct legacy dependency in mandatory gizmo workflow. |

## Retained

| Symbol/File | Scope | Runtime consumer proof (`rg -n`) | Rationale / Impact |
|---|---|---|---|
| `vec3_add`, `mat4_mul_point` in `src/math3d.h` | Core legacy math helpers still used by non-Phase-8 runtime paths | `rg -n "vec3_add\\(|mat4_mul_point\\(" src --glob "**/*.{h,c}"` | Not temporary interaction glue; broad consumers remain (e.g. `src/app.c`, `src/gizmo/gizmo.h`, `src/ecs/ecs_scene.h`). Keeping avoids out-of-scope refactor during Phase 8. |
| `ray_from_screen`, `ray_axis_closest_t`, `ray_plane_intersect` in `src/math3d.h` | Legacy interaction helper surface | `rg -n "ray_from_screen\\(|ray_axis_closest_t\\(|ray_plane_intersect\\(" src --glob "**/*.{h,c}"` | Retained for compare harness and residual runtime users outside Phase 8 boundaries; marked with Phase8 deferred glue comments for explicit carry-over. |

## Deferred-to-Phase-9

| Symbol/File | Why deferred | Impact if retained now | Phase 9 carry-over note |
|---|---|---|---|
| `ray_from_screen`, `ray_axis_closest_t`, `ray_plane_intersect` in `src/math3d.h` | Removal now would force cross-scope updates in harness parity surfaces and additional runtime files beyond Phase 8 target boundaries. | Minimal runtime risk; symbols are explicitly deprecated for migrated slices and guarded by evidence checks. | Close under `TRED-02` boundary-finalization by migrating remaining consumers and/or isolating harness-only compatibility paths. |
