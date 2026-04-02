---
phase: 14-script-io-api-undo-integration
plan: 01
subsystem: api
tags: [scripting, undo-redo, transactional-apply, scene-façade, regression-tests]
requires:
  - phase: 13-script-round-trip-baseline
    provides: deterministic script emit/parse/apply façade and script editor flow
provides:
  - Atomic script-apply undo command payload and execution path
  - Scene-level apply façade wiring to push one undo entry per successful apply
  - Regression tests for single-step undo/redo and failed-apply state preservation
affects: [phase-14-script-io-window, script-io-live-apply, api-contracts]
tech-stack:
  added: []
  patterns: [scene-level script apply owns undo transaction creation, undo execution reuses scene façade with suppression guard]
key-files:
  created: []
  modified:
    - src/tests/script_roundtrip_tests.c
    - src/undo_redo.h
    - src/undo_redo_exec.h
    - src/ecs/ecs_scene.h
    - src/app.c
key-decisions:
  - "Represent script apply history as CMD_SCRIPT_APPLY_TRANSACTION containing before/after emitted scripts."
  - "Keep UI thin: app binds undo stack once and still invokes scene_script_apply_commit only."
patterns-established:
  - "Script transaction undo/redo executes via scene_script_apply_commit with internal suppression to avoid recursive history."
requirements-completed: [SCRP-04, API-02]
duration: 1m 23s
completed: 2026-04-02
---

# Phase 14 Plan 01: Script apply transaction undo integration Summary

**Atomic script apply commits now map to exactly one undo/redo transaction while preserving last-valid sketch state on failed applies.**

## Performance

- **Duration:** 1m 23s
- **Started:** 2026-04-02T09:20:01Z
- **Completed:** 2026-04-02T09:21:24Z
- **Tasks:** 2
- **Files modified:** 5

## Accomplishments
- Added explicit regression coverage for one-step script apply undo/redo and failure-state preservation.
- Added `CMD_SCRIPT_APPLY_TRANSACTION` command type with owned before/after script payload lifecycle.
- Wired `scene_script_apply_commit` to record one undo command per successful apply and rollback if transaction recording fails.

## Task Commits

1. **Task 1: Add failing transaction tests for script apply atomic undo/redo** - `007283e` (test)
2. **Task 2: Implement script-transaction undo command and wire Apply Script to single-step undo** - `843e2ba` (feat)

## Files Created/Modified
- `src/tests/script_roundtrip_tests.c` - added transaction undo/redo and failure-preservation tests.
- `src/undo_redo.h` - added script transaction command enum, payload, cleanup, and naming.
- `src/undo_redo_exec.h` - added script transaction command execution for apply/unapply paths.
- `src/ecs/ecs_scene.h` - added scene->undo binding and transactional recording inside script apply façade.
- `src/app.c` - bound scene script subsystem to app undo stack at initialization.

## Decisions Made
- Use emitted script snapshots (`before_script` / `after_script`) as undo payload to restore full sketch subtree state consistently.
- Protect undo/redo replay from recursive history growth using `script_apply_undo_suppressed` guard in scene.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Added missing scene/undo integration symbol required by new regression tests**
- **Found during:** Task 1 (TDD RED compile)
- **Issue:** New tests referenced `scene_script_bind_undo_redo`, which did not exist, causing unresolved symbol at link time.
- **Fix:** Implemented scene-level undo binding plus transaction recording path and command handlers (Task 2 implementation scope).
- **Files modified:** `src/ecs/ecs_scene.h`, `src/undo_redo.h`, `src/undo_redo_exec.h`, `src/app.c`
- **Verification:** `ctest --test-dir build-vulkan -C Release -R script_roundtrip_tests --output-on-failure`
- **Committed in:** `843e2ba`

---

**Total deviations:** 1 auto-fixed (Rule 3 blocking)
**Impact on plan:** Required for plan completion; no scope expansion beyond planned transactional undo integration.

## Issues Encountered
- Initial targeted ctest run did not rebuild the updated test binary; explicit target build was run to exercise true RED behavior before GREEN implementation.

## Known Stubs
None.

## Next Phase Readiness
- Script apply now has atomic undo semantics at scene API level, ready for script IO live-edit flows to consume the same contract.
- No blockers identified for subsequent Phase 14 plans.

## Self-Check: PENDING

## Self-Check: PASSED

