---
phase: 23-principal-direction-constraint-expansion
plan: 02
subsystem: solver
tags: [constraints, along-axis, transactional-solve, deterministic, ctest]
requires:
  - phase: 23-01
    provides: ALONG X/Y/Z legality for descriptor-based point participants
provides:
  - Scene-owned transactional ALONG X/Y/Z runtime solve path for descriptor participants
  - Arc CENTER candidate resolution support in recalculate participant staging
  - Regression coverage for ALONG group solve semantics and ALONG+ANGLE coexistence
affects: [23-03, solver-runtime, recalculate, diagnostics]
tech-stack:
  added: []
  patterns: [scene_solver authority, deterministic axis-equality projection, transactional no-mutation failure]
key-files:
  created: [.planning/phases/23-principal-direction-constraint-expansion/23-02-SUMMARY.md]
  modified:
    - src/ecs/ecs_scene.h
    - src/tests/scene_solver_contract_test.c
    - src/tests/scene_solver_pass_policy_test.c
key-decisions:
  - "Implement ALONG X/Y/Z as deterministic axis-equality projection using participant-coordinate mean per axis."
  - "Keep ALONG unsatisfied cases on the existing transactional failure path with explicit per-axis diagnostics and implication wiring."
patterns-established:
  - "ALONG constraints resolve participants via scene_solver_ensure_point_candidate using descriptor roles (including arc CENTER) before solving."
  - "ALONG and ANGLE coexistence remains pass-policy bounded and idempotent under repeated recalc on unchanged state."
requirements-completed: [SRLV-04, AXIS-01, AXIS-02, AXIS-03, AXIS-04]
duration: 8m
completed: 2026-04-08
---

# Phase 23 Plan 02: Principal-direction runtime solve expansion Summary

**Scene-owned recalculate now solves ALONG X/Y/Z across standalone and landmark point participants with deterministic transactional behavior and explicit unsatisfied diagnostics, including ALONG+ANGLE coexistence coverage.**

## Performance

- **Duration:** 8m
- **Started:** 2026-04-08T11:41:52Z
- **Completed:** 2026-04-08T11:49:28Z
- **Tasks:** 2
- **Files modified:** 3

## Accomplishments
- Added a dedicated transactional ALONG X/Y/Z solver branch in `scene_solver_request_recalculate(...)` before unsupported-type fallback.
- Extended solver participant resolution to support `CONSTRAINT_PARTICIPANT_ROLE_CENTER` for arc participants in candidate staging.
- Locked runtime behavior with executable contracts for ALONG group solves, mixed landmark descriptors, ALONG+ANGLE coexistence, pass-policy tolerance stop, and max-pass diagnostics.

## Task Commits

1. **Task 1 (TDD RED): Extend scene solver participant resolution and ALONG X/Y/Z transactional solve branch** - `7484efb` (test)
2. **Task 1 (TDD GREEN): Extend scene solver participant resolution and ALONG X/Y/Z transactional solve branch** - `273a638` (feat)
3. **Task 2: Add regression coverage for ALONG solve semantics and ALONG+ANGLE coexistence** - `4cc8d83` (test)

## Files Created/Modified
- `src/ecs/ecs_scene.h` - Added arc-center candidate support and deterministic transactional ALONG X/Y/Z solve handling with explicit unsatisfied diagnostics.
- `src/tests/scene_solver_contract_test.c` - Added ALONG runtime contracts for mixed descriptors, group semantics, unsatisfied fixed no-mutation behavior, and ALONG+ANGLE deterministic coexistence.
- `src/tests/scene_solver_pass_policy_test.c` - Added ALONG-inclusive tolerance-stop and max-pass explicit diagnostic assertions.

## Decisions Made
- Used deterministic mean-coordinate axis projection for ALONG participant groups to preserve idempotent outcomes across repeated recalc.
- Preserved solver authority and failure contracts in `scene_solver_*` APIs only; UI/app remains a thin caller path.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered
- None.

## Known Stubs
None.

## Next Phase Readiness
- Plan 23-03 can focus on UX alignment + final verification with runtime ALONG solve behavior and coexistence contracts already locked.
- SRLV-04 runtime closure is now backed by solver contract/pass-policy coverage and phase gate command output.

## Self-Check: PASSED

