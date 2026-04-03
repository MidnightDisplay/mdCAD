---
phase: 17-constraint-driven-geometry-solving
plan: 04
subsystem: ui
tags: [endpoint-pick, sketch-endpoints, ecs, constraint-authoring]
requires:
  - phase: 17-constraint-driven-geometry-solving
    provides: Transactional solve/drag and endpoint pick baseline coverage from 17-01 and 17-03
provides:
  - Sketch-scoped EndPoints native endpoint entity contract
  - Sketch line/arc endpoint child entity spawn and sync lifecycle
  - Native endpoint point pick-path routing without synthetic tail-range decode dependency
affects: [17-05, endpoint UX, D-09, D-10, D-12]
tech-stack:
  added: []
  patterns: [owner+endpoint metadata component, sketch-only endpoint point spawn/sync, overlay-point native endpoint precedence]
key-files:
  created:
    - src/components/endpoints_comp.h
    - .planning/phases/17-constraint-driven-geometry-solving/17-04-SUMMARY.md
  modified:
    - src/ecs/ecs_world.h
    - src/ecs/ecs_scene.h
    - src/tests/endpoint_pick_test.c
key-decisions:
  - "Endpoint picks now resolve through native point entities carrying EndPoints metadata instead of synthetic tail-range decoding."
  - "Endpoint entity lifecycle is explicitly sketch-scoped; bare non-sketch lines/arcs do not receive endpoint entities."
patterns-established:
  - "Line/arc notable vertices in sketches are represented as child point entities with owner role/index metadata."
requirements-completed: [D-09, D-10, D-12]
duration: 22 min
completed: 2026-04-03
---

# Phase 17 Plan 04: Native sketch endpoint entities Summary

**Sketch lines/arcs now own visible native endpoint point entities with deterministic overlay pick precedence in normal viewport flow, replacing synthetic endpoint pick decoding for the main endpoint authoring path.**

## Performance

- **Duration:** 22 min
- **Started:** 2026-04-03T12:24:00Z
- **Completed:** 2026-04-03T12:45:37Z
- **Tasks:** 2/2 complete
- **Files modified:** 5

## Accomplishments
- Added `EndPointsComp` with owner/endpoint contracts that map endpoint point entity ↔ owner geometry role/index.
- Registered `EndPointsComp` in ECS world state with get/set accessors.
- Implemented sketch-only endpoint point spawn/sync in scene lifecycle and routed participant pick resolution through native endpoint entities.
- Removed synthetic line/arc endpoint tail-range overlay generation from normal pick population path.
- Expanded `endpoint_pick` tests to assert native endpoint entity resolution and non-sketch scope guard behavior.

## Task Commits

1. **Task 1: Define sketch-scoped EndPoints component contract and registration** - `e45c725` (feat)
2. **Task 2: Spawn/sync native endpoint point entities for sketch lines/arcs and lock pick precedence** - `3b63adf` (feat)

## Files Created/Modified
- `src/components/endpoints_comp.h` - EndPoints owner/point metadata contract and binding helpers.
- `src/ecs/ecs_world.h` - EndPoints component registration and ECS get/set helpers.
- `src/ecs/ecs_scene.h` - sketch-only endpoint lifecycle sync, native endpoint pick resolution, and overlay-point pick routing.
- `src/tests/endpoint_pick_test.c` - native endpoint resolution + non-sketch guardrail tests.

## Decisions Made
- Endpoint participant picks are now resolved by inspecting native endpoint point entities (via `EndPointsComp`) and returning the owning line/arc with role metadata.
- Endpoint entities are only created when owner geometry is under a sketch ancestor; non-sketch line/arc behavior is preserved.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Full solution build included unrelated target/link failures**
- **Found during:** Task 2 verification
- **Issue:** Building all targets surfaced unrelated issues (`mdCAD.exe` file lock and separate script emit symbol path in unrelated target), which blocked full-build completion but not plan-required tests.
- **Fix:** Switched to building only plan-required test targets (`endpoint_pick`, `scene_solver_contract`, `scene_solver_drag`, `scene_solver_diagnostics`) before running required ctest commands.
- **Verification:** All required ctest commands passed.
- **Committed in:** `3b63adf` (task commit)

---

**Total deviations:** 1 auto-fixed (Rule 3)
**Impact on plan:** No scope creep; change was limited to unblock required plan verification.

## Issues Encountered
- Initial full `cmake --build` run failed due to unrelated binary lock and non-plan target issue; resolved by scoped target build for required tests.

## Next Phase Readiness
- Ready for Plan 17-05 point-context semantics and final endpoint UX closure verification.
- Native endpoint entity contract and sketch-only lifecycle foundation are now in place.

## Self-Check: PASSED
- Confirmed summary file exists at `.planning/phases/17-constraint-driven-geometry-solving/17-04-SUMMARY.md`.
- Confirmed task commits exist in git history (`e45c725`, `3b63adf`).

