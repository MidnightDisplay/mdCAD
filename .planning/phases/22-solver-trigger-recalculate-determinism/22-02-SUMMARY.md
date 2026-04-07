---
phase: 22-solver-trigger-recalculate-determinism
plan: 02
subsystem: solver
tags: [solver, deterministic, debounce, recalculate, constraints]
requires:
  - phase: 22-01
    provides: Baseline solver contract tests for deterministic recalc and D-09/D-10 anchors
provides:
  - Scene-owned debounced auto-solve queue processor with 50ms default coalescing semantics
  - Manual recalculate override that clears pending queue metadata before immediate solve
  - Bounded recalc loop using tolerance + max-pass policy with explicit max-pass diagnostics
  - Driving LENGTH/ANGLE deterministic handling with explicit unsat implication path
affects: [phase-22-verification, solver-ui, constraint-runtime]
tech-stack:
  added: []
  patterns: [scene-owned solver authority, per-sketch runtime solve policy]
key-files:
  created: []
  modified:
    - src/components/sketch_comp.h
    - src/ecs/ecs_scene.h
    - src/app.c
    - src/tests/scene_solver_contract_test.c
    - src/tests/scene_solver_trigger_test.c
    - src/tests/scene_solver_pass_policy_test.c
key-decisions:
  - "Keep solver orchestration in scene_solver_* APIs; app frame only calls queue processor."
  - "Use per-sketch debounce/tolerance/max-pass settings with defaults (50ms, 1e-4, 10)."
  - "Emit explicit 'max passes reached' diagnostic and implication when bounded loop does not converge."
patterns-established:
  - "Scene-level queue processing pattern: request APIs enqueue/coalesce, frame loop flushes via scene API."
  - "Transactional recalc pattern: pending queue metadata is cleared before manual authoritative solve."
requirements-completed: [SRLV-01, SRLV-02, SRLV-03, SRLV-05]
duration: 8m
completed: 2026-04-07
---

# Phase 22 Plan 02: Solver Trigger + Recalculate Determinism Summary

**Debounced scene-owned auto-solve with deterministic bounded recalc and explicit max-pass/unsat diagnostics for driving constraints.**

## Performance

- **Duration:** 8 min
- **Started:** 2026-04-07T16:47:49+01:00
- **Completed:** 2026-04-07T16:55:21Z
- **Tasks:** 2
- **Files modified:** 6

## Accomplishments
- Added per-sketch solver runtime config/state fields for debounce, tolerances, and max-pass policy with required defaults.
- Implemented `scene_solver_process_auto_queue(...)` and wired a single per-frame call in `app.c` without moving solver authority out of scene APIs.
- Refactored recalc flow to bounded deterministic pass behavior with explicit `"max passes reached"` diagnostic and implication handling.
- Preserved success implication clear behavior and diagnostics ring dedupe/cap/explicit-clear behavior while extending LENGTH/ANGLE driving paths.

## Task Commits

1. **Task 1: Add sketch solver runtime config for debounce/tolerances/max-pass defaults**
   - `9add50c` (test): failing test for defaults + manual recalc queue clear
   - `3e93dbb` (feat): sketch/runtime fields + manual recalc queue metadata clear contract
2. **Task 2: Implement scene-owned debounce processing and deterministic bounded recalc behavior**
   - `5a71c9f` (test): failing debounce/queue and pass policy tests
   - `8377f41` (test): stabilized max-pass policy test setup
   - `ec95c52` (feat): queue processor API, frame call, bounded recalc, explicit diagnostics

## Files Created/Modified
- `src/components/sketch_comp.h` - Added runtime debounce/tolerance/max-pass fields and defaults.
- `src/ecs/ecs_scene.h` - Added scene queue processor, bounded pass loop, max-pass diagnostics, and driving LENGTH/ANGLE handling updates.
- `src/app.c` - Added per-frame `scene_solver_process_auto_queue(&state.ecs_scene)` call.
- `src/tests/scene_solver_contract_test.c` - Added default/config and manual recalc queue-clear contract test.
- `src/tests/scene_solver_trigger_test.c` - Reworked to assert real scene queue coalescing/debounce and manual recalc cancellation.
- `src/tests/scene_solver_pass_policy_test.c` - Reworked to assert tolerance stop and max-pass explicit diagnostic behavior.

## Decisions Made
- Kept all orchestration semantics in `scene_solver_*` APIs and used app loop as thin caller only (D-12).
- Used per-sketch settings defaults from component constants for deterministic behavior across queue and recalc policy.
- Kept implication clear path centralized in `scene_solver_apply_status(... SKETCH_STATUS_SOLVED ...)`.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Fixed unstable max-pass test setup that hit immediate unsat path**
- **Found during:** Task 2
- **Issue:** Original max-pass test setup used fixed coincident points, producing immediate unsatisfied failure instead of bounded-pass behavior.
- **Fix:** Reworked test to use chained coincident constraints requiring multi-pass convergence pressure, so max-pass contract is exercised correctly.
- **Files modified:** `src/tests/scene_solver_pass_policy_test.c`
- **Verification:** Targeted solver gate passed with `scene_solver_pass_policy` green.
- **Committed in:** `8377f41`

---

**Total deviations:** 1 auto-fixed (Rule 1)
**Impact on plan:** No scope creep; fix was necessary to validate the intended bounded-pass contract accurately.

## Issues Encountered
- Initial Task 2 test run failed because `scene_solver_process_auto_queue` API did not yet exist (expected TDD RED state).
- First pass-policy assertion design validated unsat behavior instead of max-pass behavior; corrected via Rule 1 test adjustment.

## User Setup Required
None - no external service configuration required.

## Known Stubs
None.

## Next Phase Readiness
- Phase 22 solver orchestration behavior is now deterministic and test-anchored for debounce/manual override/pass-cap paths.
- Ready for remaining Phase 22 follow-up plan(s) and verification artifact updates.

## Self-Check: PASSED

