---
phase: 26-line-line-constraint-coverage
plan: 01
subsystem: testing
tags: [constraints, solver, parallel, perpendicular, determinism]
requires:
  - phase: 25-regression-and-reliability-closure
    provides: transactional solver diagnostics and deterministic failure implication patterns
provides:
  - pair line-line PARALLEL/PERPENDICULAR transactional solver branches
  - explicit line-line legality parity for endpoint-role rejection
  - deterministic participant canonicalization for pair line-line creation
affects: [26-02, solver-diagnostics, endpoint-legality]
tech-stack:
  added: []
  patterns: [transactional candidate solve, entity-id canonical ordering, family-specific unsat diagnostics]
key-files:
  created: []
  modified:
    - src/constraints/constraint_types.h
    - src/ecs/ecs_scene.h
    - src/tests/scene_solver_contract_test.c
    - src/tests/endpoint_pick_test.c
    - src/tests/scene_solver_diagnostics_test.c
key-decisions:
  - "Pair PARALLEL/PERPENDICULAR solve branches follow fixed-line hard-anchor policy and fail transactionally when both lines are fixed and unsatisfied."
  - "Line-line symmetric constraints use canonical participant descriptor ordering to make pair authoring selection-order invariant."
patterns-established:
  - "Line-line legality/runtime parity: legality rejection and solver behavior evolve together with explicit family diagnostics."
  - "Determinism guard: canonicalize descriptors before storage, then assert order invariance in contract tests."
requirements-completed: [LCON-01, LCON-03, LCON-05]
duration: 12 min
completed: 2026-04-08
---

# Phase 26 Plan 01: Pair line-line legality/runtime parity Summary

**Pair line-line PARALLEL/PERPENDICULAR constraints now solve transactionally with explicit unsatisfied diagnostics and deterministic participant ordering.**

## Performance

- **Duration:** 12 min
- **Started:** 2026-04-08T21:58:22Z
- **Completed:** 2026-04-08T22:10:38Z
- **Tasks:** 3
- **Files modified:** 5

## Accomplishments
- Added RED tests for pair line-line solve success, legality rejection, and family-specific unsatisfied diagnostics.
- Implemented pair PARALLEL/PERPENDICULAR solver branches with fixed-participant transactional behavior and explicit unsat reasons.
- Added canonical descriptor ordering/deduping for symmetric line-line constraints and validated selection-order invariance.

## Task Commits

1. **Task 1: Add failing pair line-line contract and legality tests (LCON-01, LCON-03, LCON-05)** - `cc5abaf` (test)
2. **Task 2: Implement pair legality/runtime parity with fixed-anchor transactional behavior (LCON-01, LCON-03)** - `54ea26f` (feat)
3. **Task 3: Close deterministic rerun proof for pair line-line slice (LCON-05 determinism guard)** - `420bc90` (fix)

## Files Created/Modified
- `src/tests/scene_solver_contract_test.c` - added pair PARALLEL/PERPENDICULAR success, unsat transactional, and order-invariance contracts.
- `src/tests/endpoint_pick_test.c` - added explicit legality coverage for valid/invalid pair line-line signatures.
- `src/tests/scene_solver_diagnostics_test.c` - added family-specific unsatisfied diagnostic assertions for pair line-line constraints.
- `src/constraints/constraint_types.h` - enforced endpoint-role rejection for PARALLEL legality parity with PERPENDICULAR.
- `src/ecs/ecs_scene.h` - implemented pair line-line solve branches and deterministic descriptor canonicalization.

## Decisions Made
- Kept solver authority fully inside `scene_solver_request_recalculate(...)` and reused existing transactional candidate commit pattern.
- Used explicit family unsat strings (`Unsatisfied parallel constraint.`, `Unsatisfied perpendicular constraint.`) for deterministic diagnostics.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Selection-order drift risk for symmetric pair line-line constraints**
- **Found during:** Task 3 (determinism closure)
- **Issue:** Pair constraints could be stored in raw selection order, risking order-dependent behavior as phase expands.
- **Fix:** Canonicalized and deduped line-line participant descriptors by stable `(entity, role, sub_index)` ordering in constraint creation.
- **Files modified:** `src/ecs/ecs_scene.h`, `src/tests/scene_solver_contract_test.c`
- **Verification:** `ctest --test-dir build -C Release -R "scene_solver_contract|scene_solver_diagnostics" --output-on-failure` (twice)
- **Committed in:** `420bc90`

---

**Total deviations:** 1 auto-fixed (1 bug)
**Impact on plan:** Reinforced D-03/D-04 deterministic contract without broadening scope beyond pair line-line behavior.

## Issues Encountered
- Initial verification command failed because `build/` did not exist. Resolved by configuring and building (`cmake -S . -B build`, `cmake --build build --config Release`).

## Known Stubs
None.

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- Pair line-line slice is green and deterministic; ready for Phase 26 plan 02 group semantics expansion.
- No active blockers.

## Self-Check: PASSED

