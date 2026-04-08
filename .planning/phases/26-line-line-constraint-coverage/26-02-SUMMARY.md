---
phase: 26-line-line-constraint-coverage
plan: 02
subsystem: testing
tags: [constraints, solver, parallel, perpendicular, determinism, script-roundtrip]
requires:
  - phase: 26-line-line-constraint-coverage
    provides: pair line-line transactional legality/runtime parity and descriptor canonicalization
provides:
  - deterministic group PARALLEL runtime behavior for 3+ lines
  - deterministic anchor-based group PERPENDICULAR runtime behavior for 3+ lines
  - explicit line-line legality feedback in the constraint context menu
  - script roundtrip coverage for group Parallel/Perpendicular constraints
affects: [27-along-reliability, solver-diagnostics, script-roundtrip]
tech-stack:
  added: []
  patterns: [entity-id canonical anchor selection, transactional fixed-anchor line solving, deterministic rerun gate]
key-files:
  created: []
  modified:
    - src/constraints/constraint_types.h
    - src/ecs/ecs_scene.h
    - src/app.c
    - src/tests/scene_solver_contract_test.c
    - src/tests/endpoint_pick_test.c
    - src/tests/scene_solver_diagnostics_test.c
    - src/tests/script_roundtrip_tests.c
key-decisions:
  - "Group PERPENDICULAR uses the first canonical descriptor as anchor and enforces perpendicularity for each other line."
  - "Line-line group legality/runtime/message behavior is explicit and deterministic at legality, solve, and diagnostics layers."
patterns-established:
  - "Group line-line constraints follow canonical participant ordering to keep selection-order invariance."
  - "Phase closure requires deterministic rerun evidence using the targeted five-test suite twice back-to-back."
requirements-completed: [LCON-02, LCON-04, LCON-05]
duration: 45 min
completed: 2026-04-09
---

# Phase 26 Plan 02: Group line-line canonical semantics and deterministic closure Summary

**Group line-line PARALLEL/PERPENDICULAR constraints now solve with deterministic canonical-anchor behavior, explicit legality messaging, and green double-rerun closure evidence.**

## Performance

- **Duration:** 45 min
- **Started:** 2026-04-09T00:00:00Z
- **Completed:** 2026-04-09T00:45:00Z
- **Tasks:** 3
- **Files modified:** 7

## Accomplishments
- Added RED tests for group PARALLEL solve, group PERPENDICULAR anchor semantics, selection-order invariance, legality checks, diagnostics, and script roundtrip.
- Implemented group legality/runtime behavior for line-line constraints (including 3+ PERPENDICULAR) with deterministic canonical anchor semantics and transactional fixed handling.
- Added explicit line-line invalid-selection message in constraint menu and closed deterministic rerun gate with two consecutive targeted suite passes.

## Task Commits

1. **Task 1: Add failing deterministic group tests for PARALLEL and anchor-based PERPENDICULAR (LCON-02, LCON-04)** - `fa548fe` (test)
2. **Task 2: Implement group canonicalization, anchor semantics, and explicit legality/UI diagnostics (LCON-02, LCON-04, LCON-05)** - `b6465d9` (feat)
3. **Task 3: Run deterministic rerun gate for Phase 26 target suite and lock regression evidence (LCON-02, LCON-04, LCON-05)** - `ae3b89a` (fix)

## Files Created/Modified
- `src/constraints/constraint_types.h` - enabled 3+ line legality for `CONSTRAINT_PERPENDICULAR`.
- `src/ecs/ecs_scene.h` - implemented 3+ participant solve branch for line-line constraints using canonical anchor semantics.
- `src/app.c` - added explicit invalid line-line legality message when no line-line signature is applicable.
- `src/tests/scene_solver_contract_test.c` - added group solve contracts and deterministic rerun guard.
- `src/tests/endpoint_pick_test.c` - added multi-line legality acceptance/rejection coverage.
- `src/tests/scene_solver_diagnostics_test.c` - added group PERPENDICULAR family-specific unsatisfied diagnostic test.
- `src/tests/script_roundtrip_tests.c` - added script apply/emit/reapply roundtrip test for group Parallel/Perpendicular.

## Decisions Made
- Kept all group line-line behavior in existing legality + transactional solver flow (no new solver subsystem).
- Enforced deterministic anchor policy via canonical descriptor ordering already present in constraint creation path.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Windows ctest multi-config invocation missing `-C Release`**
- **Found during:** Task 1 verification
- **Issue:** CTest reported tests as Not Run in Windows multi-config build without explicit configuration.
- **Fix:** Switched all verification invocations to include `-C Release`.
- **Files modified:** none (command-level correction)
- **Verification:** Targeted CTest suite executed and produced real pass/fail outcomes.
- **Committed in:** `fa548fe` (task context)

**2. [Rule 1 - Bug] Script roundtrip fixture mixed incompatible group constraints**
- **Found during:** Task 2 verification
- **Issue:** Initial script-roundtrip fixture reused the same lines for both group parallel and group perpendicular constraints, making the fixture unsatisfiable.
- **Fix:** Split perpendicular participants to a separate compatible line set in test fixture.
- **Files modified:** `src/tests/script_roundtrip_tests.c`
- **Verification:** `ctest --test-dir build -C Release -R "scene_solver_contract|endpoint_pick|scene_solver_diagnostics|script_roundtrip" --output-on-failure`
- **Committed in:** `b6465d9`

---

**Total deviations:** 2 auto-fixed (1 blocking, 1 bug)
**Impact on plan:** Both fixes were required to obtain valid deterministic verification signal; no scope expansion beyond line-line group behavior.

## Issues Encountered
- Initial build/test command overlap produced one temporary file-lock run; resolved by rerunning build then tests sequentially.

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- Phase 26 line-line pair+group requirements are now fully implemented and regression-covered.
- Ready for Phase 27 ALONG-line reliability work without additional carry-over blockers.

## Known Stubs
None.

## Self-Check: PASSED
