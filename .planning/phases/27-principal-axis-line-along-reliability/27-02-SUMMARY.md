---
phase: 27-principal-axis-line-along-reliability
plan: 02
subsystem: testing
tags: [solver, constraints, along, determinism, diagnostics, transactional]
requires:
  - phase: 27-principal-axis-line-along-reliability
    provides: ALONG legacy line normalization/runtime legality parity from plan 01
provides:
  - deterministic mixed ALONG+LENGTH+ANGLE+connectivity rerun behavior for ALIN-04
  - family-specific ALONG X/Y/Z unsatisfied diagnostics coverage for transactional failures
  - selection-order invariant ANGLE participant handling in mixed ALONG stacks
affects: [phase-28-tangency-robustness, phase-30-closure-gate, solver-diagnostics]
tech-stack:
  added: []
  patterns:
    - deterministic participant canonicalization for binary angle constraints
    - mixed-constraint rerun exact-equality gating for ALONG closure
key-files:
  created: []
  modified:
    - src/tests/scene_solver_contract_test.c
    - src/tests/scene_solver_pass_policy_test.c
    - src/tests/scene_solver_diagnostics_test.c
    - src/ecs/ecs_scene.h
key-decisions:
  - "Canonicalize ANGLE participant line ordering by entity id before candidate resolution to preserve selection-order invariance in mixed ALONG stacks."
  - "Use explicit ALONG X/Y/Z unsatisfied diagnostic assertions to keep mixed transactional failures family-specific and non-ambiguous."
patterns-established:
  - "Mixed ALONG deterministic closure requires both immediate rerun exact-equality checks and equivalent ordering variant checks."
requirements-completed: [ALIN-04]
duration: 13 min
completed: 2026-04-09
---

# Phase 27 Plan 02: Mixed ALONG determinism and diagnostics Summary

**ALIN-04 is closed with deterministic mixed ALONG reruns, transactional fixed-anchor failures, and explicit ALONG X/Y/Z unsatisfied diagnostics.**

## Performance

- **Duration:** 13 min
- **Started:** 2026-04-09T09:33:43Z
- **Completed:** 2026-04-09T09:47:14Z
- **Tasks:** 3
- **Files modified:** 4

## Accomplishments
- Added ALIN-04 mixed-stack contract/pass-policy RED coverage for feasible rerun determinism, ordering equivalence, and transactional fixed unsatisfied behavior across X/Y/Z families.
- Stabilized solver determinism by canonicalizing ANGLE binary participant ordering in runtime recalc, preserving transactional staged candidates and fixed-anchor behavior.
- Added diagnostics regressions asserting explicit `Unsatisfied driving ALONG X|Y|Z constraint.` messages and closed the targeted closure gate with two consecutive green reruns.

## Task Commits

1. **Task 1: Add failing mixed ALONG+LENGTH+ANGLE+connectivity deterministic tests** - `b69e541` (test)
2. **Task 2: Stabilize mixed ALONG runtime ordering and add family-specific diagnostics assertions** - `cd5ed43` (feat)
3. **Task 3: Execute full Phase 27 deterministic transactional closure gate** - `f25344f` (test)

## Files Created/Modified
- `src/tests/scene_solver_contract_test.c` - Added ALIN-04 mixed deterministic/transactional coverage including equivalent ordering endpoint equality and fixed unsatisfied no-mutation assertions.
- `src/tests/scene_solver_pass_policy_test.c` - Added mixed ALONG pass-policy rerun determinism coverage.
- `src/tests/scene_solver_diagnostics_test.c` - Added ALONG X/Y/Z family-specific unsatisfied diagnostic message assertions.
- `src/ecs/ecs_scene.h` - Canonicalized ANGLE participant ordering by entity id in recalc runtime to eliminate selection-order drift in mixed ALONG stacks.

## Decisions Made
- Canonicalized ANGLE participant ordering in runtime instead of relaxing test determinism expectations, preserving deterministic contracts in mixed ALONG compositions.
- Kept all changes scoped to ALONG mixed-constraint determinism/diagnostics hardening with no tangency, gizmo, or documentation modifications.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered
- Initial RED fixtures for mixed feasible ALONG X stack could converge to `max passes reached`; adjusted fixture geometry to a stable feasible baseline so failures isolate ALIN-04 determinism expectations.

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- Phase 27 now has ALIN-01..04 covered and closure-gate evidence captured with deterministic back-to-back reruns.
- Ready to proceed to Phase 28 tangency robustness without carrying ALONG mixed-stack reliability debt.

## Known Stubs
- `src/ecs/ecs_scene.h:4710` — Existing comment marker includes "Placeholder derivation for Phase 10"; pre-existing, non-functional comment, and not blocking ALIN-04 outcomes.

## Self-Check: PASSED

