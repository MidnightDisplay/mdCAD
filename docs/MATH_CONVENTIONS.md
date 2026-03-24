# Math Conventions

## Contract

`src/math/math_conventions.h` is the single code-owned source of truth for the mdCAD math migration contract. It replaces `src/math3d.h` as the internal baseline for migrated subsystems and applies across supported backends.

| Decision | Value | Notes |
|----------|-------|-------|
| Matrix layout | column-major | Matches the project-wide matrix contract for migrated code |
| Handedness | right-handed | Preserves the chosen global coordinate convention |
| Clip depth | 0..1 | The internal clip-depth contract for the migration |
| Alignment policy | performance-first | Data-layout refactors are allowed when needed to keep the stronger native path |
| Native gate behavior | block on policy break | A migration slice should stop rather than silently falling back to a weaker mode |
| Visible behavior | normalize during rollout | Camera, picking, and gizmo interactions must stay stable while internals evolve |

## Policy Notes

- One global alignment policy is mandatory. mdCAD should not let compiler defaults or per-subsystem choices create silent drift.
- Data-layout refactors are acceptable when they are needed to satisfy the performance-first alignment policy.
- Backend glue may adapt to platform-specific API details, but semantics may not fork per backend.
- Linked `cglm` remains deferred; this phase stays on the header-only integration path unless later measurements justify a change.

## Rollout-Sensitive Hotspots

These files should be treated as the first places where policy drift would become user-visible:

- `src/orbit_camera.h`
- `src/app.c`
- `src/ecs/ecs_scene.h`
- `src/gpu/pick_buffer.h`
- `src/gizmo/gizmo.h`

## Visible Behavior Normalization During Rollout

Visible behavior normalization during rollout means the user should not feel a regression in camera navigation, picking accuracy, or gizmo manipulation while the internal convention moves to `0..1` clip depth. The migration may change internal math layout and alignment strategy, but viewport behavior in `src/orbit_camera.h`, view/projection composition in `src/app.c`, pick-ray behavior in `src/gpu/pick_buffer.h`, and drag handling in `src/gizmo/gizmo.h` should remain stable and trustworthy throughout staged adoption.

## Implementation Guidance

- Include `src/math/math_conventions.h` before vendor `cglm` headers so convention policy is visible at every migration entrypoint.
- Treat `src/math/cglm_entry.h` as an entrypoint only, not as a wrapper API that redefines vendor math types.
- When a native build gate cannot satisfy the contract cleanly, stop the slice and resolve the policy break before expanding the rollout.
