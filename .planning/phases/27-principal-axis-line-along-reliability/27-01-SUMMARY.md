---
phase: 27-principal-axis-line-along-reliability
plan: 01
subsystem: testing
tags: [solver, constraints, along, determinism, transactional]
requires:
  - phase: 26-line-line-constraint-coverage
    provides: line-line legality/runtime parity and deterministic canonical participant behavior
provides:
  - reliable ALONG X/Y/Z solving for legacy single-line ENTITY signatures via endpoint descriptor normalization
  - deterministic ALONG participant normalization and deduplication in runtime solve path
  - transactional unsatisfied fixed ALONG behavior preserved for line-entity legacy signatures
affects: [27-02-mixed-along-determinism, solver-diagnostics, endpoint-legality]
tech-stack:
  added: []
  patterns: [descriptor-authoritative runtime normalization, deterministic descriptor dedupe, transactional fixed-anchor failure]
key-files:
  created: []
  modified:
    - src/tests/scene_solver_contract_test.c
    - src/tests/endpoint_pick_test.c
    - src/ecs/ecs_scene.h
key-decisions:
  - "ALONG runtime now expands line ENTITY participants into POINT_A and POINT_B descriptors before candidate resolution."
  - "Single-line ALONG constraints are valid when normalization yields two endpoint candidates, preserving legality/runtime parity."
patterns-established:
  - "ALONG participant normalization uses deterministic descriptor ordering and duplicate collapse before candidate lookup."
  - "ALONG fixed unsatisfied paths continue returning explicit family reasons with no geometry mutation."
requirements-completed: [ALIN-01, ALIN-02, ALIN-03]
duration: 9 min
completed: 2026-04-09
---

# Phase 27 Plan 01: Principal-axis ALONG reliability Summary

**ALONG X/Y/Z now solves legal legacy single-line entity signatures by deterministic endpoint-descriptor normalization while preserving transactional fixed-failure behavior.**

## Performance

- **Duration:** 9 min
- **Started:** 2026-04-09T09:19:59Z
- **Completed:** 2026-04-09T09:29:58Z
- **Tasks:** 3
- **Files modified:** 3

## Accomplishments
- Added RED regression coverage for ALIN-01/02/03 using legacy single-line ENTITY ALONG signatures and fixed unsatisfied transactional assertions.
- Implemented ALONG solver normalization that expands line ENTITY participants to endpoint descriptors and deduplicates deterministically before candidate resolution.
- Closed deterministic gate with two consecutive targeted runs of `scene_solver_contract` + `endpoint_pick`.

## Task Commits

1. **Task 1: Add failing ALONG X/Y/Z line reliability tests for legacy line signatures** - `ac15812` (test)
2. **Task 2: Implement deterministic ALONG participant normalization in solver runtime** - `1872274` (feat)
3. **Task 3: Run deterministic and transactional gate for ALONG line reliability slice** - `39ee587` (test)

## Files Created/Modified
- `src/tests/scene_solver_contract_test.c` - Added ALIN single-line legacy line reliability tests for ALONG X/Y/Z and fixed unsatisfied transactional assertion.
- `src/tests/endpoint_pick_test.c` - Added explicit legality parity regression for legacy single-line ALONG line signatures.
- `src/ecs/ecs_scene.h` - Normalized ALONG participant descriptors at solve-time (ENTITY line -> POINT_A/POINT_B), deterministic dedupe, and preserved axis-lock semantics.

## Decisions Made
- Kept legality acceptance for legacy single-line line-ENTITY ALONG signatures and restored runtime parity by solve-time descriptor expansion instead of changing legality contracts.
- Preserved existing orthogonal-axis lock semantics and transactional fixed-anchor behavior; only participant normalization/dedup path was changed.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Single-line ALONG branch rejected before normalization**
- **Found during:** Task 2 verification
- **Issue:** Existing ALONG branch rejected constraints with `< 2` raw participants before line ENTITY expansion, causing legacy single-line ALONG to fail despite legal signature.
- **Fix:** Removed the pre-normalization participant-count failure gate so ALONG validates using normalized participant count.
- **Files modified:** `src/ecs/ecs_scene.h`
- **Verification:** `ctest --test-dir build -C Release -R "scene_solver_contract|endpoint_pick" --output-on-failure`
- **Committed in:** `1872274`

---

**Total deviations:** 1 auto-fixed (1 bug)
**Impact on plan:** Fix was required to satisfy ALIN-01/02/03 correctness without scope expansion.

## Issues Encountered
- Initial RED verify passed because stale binaries were executed; resolved by rebuilding `scene_solver_contract` and `endpoint_pick` before running ctest.

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- ALIN-01/02/03 are now covered by passing runtime and legality parity regressions.
- Phase 27 Plan 02 can focus on ALIN-04 mixed-constraint determinism and diagnostics closure.

## Known Stubs
None.

## Self-Check: PASSED
