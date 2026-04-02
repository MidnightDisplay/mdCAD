---
phase: 17-constraint-driven-geometry-solving
plan: 01
subsystem: testing
tags: [solver, constraints, ctest, ecs]
requires:
  - phase: 16-constraint-ux-closure-and-verification
    provides: Constraint participant highlighting and solver UX baseline contracts
provides:
  - Wave-0 deterministic CTest targets for solver contract, drag, endpoint pick, and diagnostics
  - Transactional scene solver recalc behavior for point coincident constraints
  - Constraint participant descriptors with entity role/sub-entity index metadata
affects: [17-02, 17-03, viewport endpoint authoring, solver diagnostics]
tech-stack:
  added: []
  patterns: [header-only scene solver transactional staging, deterministic implication ordering]
key-files:
  created:
    - src/tests/scene_solver_contract_test.c
    - src/tests/scene_solver_drag_test.c
    - src/tests/endpoint_pick_test.c
    - src/tests/scene_solver_diagnostics_test.c
  modified:
    - src/CMakeLists.txt
    - src/components/constraint_comp.h
    - src/ecs/ecs_scene.h
key-decisions:
  - "Phase 17 Wave-0 tests are native C executables registered as first-class CTest targets."
  - "Recalculate now performs transactional candidate staging and only commits geometry on full success."
patterns-established:
  - "Constraint participants now carry descriptor metadata (entity + role + sub-index) alongside legacy IDs."
  - "Unsatisfied transactional solves return false, keep geometry unchanged, and emit stable implication payloads."
requirements-completed: [D-01, D-02, D-05, D-08, D-09, D-10, D-11]
duration: 11 min
completed: 2026-04-02
---

# Phase 17 Plan 01: Transactional solve contracts Summary

**Transactional point-coincident solve-apply behavior with deterministic no-mutation failure and Wave-0 CTest gates for solver/drag/pick/diagnostics.**

## Performance

- **Duration:** 11 min
- **Started:** 2026-04-02T17:58:00Z
- **Completed:** 2026-04-02T18:09:30Z
- **Tasks:** 2
- **Files modified:** 7

## Accomplishments
- Added four Phase 17 Wave-0 C test executables and registered matching CTest targets.
- Introduced participant descriptors that encode entity, role, and sub-entity index contracts.
- Implemented transactional recalc for point coincident constraints with deterministic failure implication behavior.

## Task Commits

1. **Task 1: Create Wave-0 deterministic test scaffolds and CTest registration** - `21baa32` (test)
2. **Task 2: Refactor participant and solve contracts for endpoint-aware transactional solve** - `53201ca` (feat)

## Files Created/Modified
- `src/tests/scene_solver_contract_test.c` - deterministic solve success/failure transactional contract tests.
- `src/tests/scene_solver_drag_test.c` - drag unsat/diagnostic baseline tests.
- `src/tests/endpoint_pick_test.c` - endpoint priority contract baseline test target.
- `src/tests/scene_solver_diagnostics_test.c` - diagnostics baseline ordering/dedupe scaffolding.
- `src/CMakeLists.txt` - adds 4 test executables and `add_test(...)` registrations.
- `src/components/constraint_comp.h` - adds participant descriptor schema with role/sub-index support.
- `src/ecs/ecs_scene.h` - transactional solve staging/commit logic and deterministic failure implication ordering.

## Decisions Made
- Kept Wave-0 endpoint/diagnostic tests minimal but executable to satisfy deterministic target wiring before broader viewport integration.
- Scoped transactional solver implementation to point-point coincident in this plan to honor deferred multi-constraint backend scope from `17-CONTEXT.md`.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] New test executables were not produced by mdCAD target build**
- **Found during:** Task 1 verification
- **Issue:** `ctest` could not find new executables after building only `mdCAD`.
- **Fix:** Built explicit test targets (`scene_solver_contract`, `scene_solver_drag`, `endpoint_pick`, `scene_solver_diagnostics`) before running CTest.
- **Files modified:** none
- **Verification:** `ctest -R "scene_solver_contract|scene_solver_drag|endpoint_pick|scene_solver_diagnostics" --test-dir build-vulkan -C Release --output-on-failure`
- **Committed in:** `21baa32`

**2. [Rule 3 - Blocking] New test sources missed required include stack used by header-only scene modules**
- **Found during:** Task 1/2 build verification
- **Issue:** unresolved static inline declarations while compiling new tests.
- **Fix:** Added the established include block used by existing native tests.
- **Files modified:** `src/tests/scene_solver_contract_test.c`, `src/tests/scene_solver_drag_test.c`, `src/tests/endpoint_pick_test.c`, `src/tests/scene_solver_diagnostics_test.c`
- **Verification:** targeted build + CTest passes
- **Committed in:** `21baa32`, `53201ca`

---

**Total deviations:** 2 auto-fixed (2 blocking)
**Impact on plan:** Both fixes were required to make planned verification executable; no scope expansion beyond plan goals.

## Issues Encountered
- None

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- Wave-0 deterministic test entry points are available for Phase 17 follow-on implementation.
- Transactional solver contract and participant descriptor scaffolding are in place for endpoint authoring and drag hardening work.


## Self-Check: PASSED
