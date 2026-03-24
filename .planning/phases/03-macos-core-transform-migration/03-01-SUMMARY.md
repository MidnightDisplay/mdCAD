---
phase: 03-macos-core-transform-migration
plan: 01
subsystem: camera
tags: [cglm, camera, viewport, parity]
requires:
  - phase: 02-direct-adoption-tooling-and-validation-harness
    provides: Standalone harness workflow and the thin cglm math entrypoint
provides:
  - cglm-backed orbit camera eye and view helpers with stable legacy-returning wrappers
  - cglm-backed app-side view, projection, VP, and MVP construction with explicit legacy bridge copies
  - Frozen legacy orbit/projection baselines so the harness still validates the migrated production path
affects: [phase-03-02, phase-03-03, macos-parity]
tech-stack:
  added: [cglm-struct-camera-helpers, explicit-matrix-bridge-copies]
  patterns: [cglm-first-camera-path, frozen-legacy-harness-baselines]
key-files:
  created: []
  modified:
    - src/orbit_camera.h
    - src/app.c
    - src/math_harness.c
key-decisions:
  - "Orbit camera eye/view math now originates from cglm helpers while legacy vec3_t and mat4_t return paths remain as explicit temporary bridges."
  - "The app assembles view/projection/VP/MVP once through cglm and then copies those matrices into legacy storage for unchanged pick, gizmo, and render consumers."
patterns-established:
  - "Migrate user-visible hot paths by computing in cglm first and bridging outward at subsystem boundaries that still require mat4_t."
  - "Freeze pre-migration formulas inside the harness before replacing production helpers so parity checks do not compare migrated code against itself."
requirements-completed: [HOT-01]
duration: 1 min
completed: 2026-03-24
---

# Phase 03 Plan 01: Orbit Camera and Viewport Matrix Construction Summary

**The live macOS camera and viewport matrix path now runs through cglm, while legacy bridge matrices stay explicit and the harness still compares against frozen pre-migration formulas**

## Performance

- **Duration:** 1 min
- **Started:** 2026-03-24T17:32:11Z
- **Completed:** 2026-03-24T17:33:42Z
- **Tasks:** 2
- **Files modified:** 3

## Accomplishments

- Added `orbit_camera_get_eye_position_cglm(...)` and `orbit_camera_get_view_matrix_cglm(...)` in `src/orbit_camera.h`, while keeping the existing `vec3_t` and `mat4_t` helpers as thin bridge wrappers.
- Replaced the app-side view/projection/VP/MVP construction in `src/app.c` with `glms_*` calls and explicit `mat4_t` bridge copies for unchanged pick, gizmo, and triangle-render consumers.
- Froze the old orbit camera and projection formulas in `src/math_harness.c` so `orbit-camera-view` and `view-projection-roundtrip` continue validating the migrated production path honestly.

## Task Commits

Each task was committed atomically:

1. **Task 1: Add cglm-backed orbit camera helpers while keeping source-compatible bridges** - `47bc5eb` (feat)
2. **Task 2: Migrate app-side view/projection/MVP construction and freeze legacy parity baselines in the harness** - `ede0e61` (feat)

**Plan metadata:** recorded in the summary/state/roadmap completion commit

## Files Created/Modified

- `src/orbit_camera.h` - Added cglm-native eye and view helpers and kept the legacy-returning API as an explicit compatibility bridge.
- `src/app.c` - Swapped the live camera/view/projection/MVP assembly to cglm and copied those matrices into local `mat4_t` bridge values for unchanged downstream consumers.
- `src/math_harness.c` - Added frozen legacy orbit-view helpers and kept the compare cases pointed at live migrated production helpers on the candidate side.

## Decisions Made

- Kept the public `orbit_camera_t` state and legacy helper signatures stable in Phase 3 so camera/UI code can migrate incrementally without a wider storage rewrite.
- Chose one app-side cglm assembly point with explicit bridge copies instead of allowing downstream subsystems to recompute view/projection with `math3d.h`.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None

## User Setup Required

None - the existing macOS/Ninja workflow remains the execution path.

## Next Phase Readiness

- `03-02` can now move `TransformComp` and ECS world-point application onto cglm without reopening the camera path.
- The harness already has frozen camera/projection baselines in place for the remaining Phase 3 parity work.

---
*Phase: 03-macos-core-transform-migration*
*Completed: 2026-03-24*
