---
phase: 18-add-undo-steps-for-endpoint-moves
plan: 03
subsystem: testing
tags: [undo, endpoints, regression, script-io, validation]
requires:
  - phase: 18-add-undo-steps-for-endpoint-moves
    provides: endpoint-aware undo record/replay contract from 18-01 and 18-02
provides:
  - Regression gates for non-sketch undo invariants and endpoint drag-end coalescing boundaries.
  - Script transaction undo invariants for `CMD_SCRIPT_APPLY_TRANSACTION`, including noop guard behavior.
  - Updated phase validation evidence with PH18-01..PH18-03 automated command results.
affects: [undo-redo, endpoint-interaction, non-sketch-transform, script-transactions, phase-validation]
tech-stack:
  added: []
  patterns:
    - Shared drag-end undo recorder helper routes endpoint entities to endpoint command path and non-endpoint entities to legacy position path.
    - Script apply transaction recording skips noop before/after payload pairs to preserve undo granularity.
key-files:
  created:
    - .planning/phases/18-add-undo-steps-for-endpoint-moves/18-03-SUMMARY.md
  modified:
    - src/tests/endpoint_pick_test.c
    - src/app.c
    - src/undo_redo_exec.h
    - src/tests/script_roundtrip_tests.c
    - .planning/phases/18-add-undo-steps-for-endpoint-moves/18-VALIDATION.md
key-decisions:
  - "Centralize transform drag-end undo path selection in record_drag_end_move_for_entity for parity between app runtime and regression tests."
  - "Preserve script transaction semantics by rejecting noop script transaction pushes in undo_cmd_script_apply_transaction."
patterns-established:
  - "Endpoint drag emits one undo entry at release boundary while mid-drag frames remain undo-silent."
  - "Non-sketch transform drag-end remains on CMD_SET_POSITION path."
requirements-completed: [PH18-03]
duration: 6m
completed: 2026-04-04
---

# Phase 18 Plan 03: Endpoint regression hardening and validation closure Summary

**Regression coverage now explicitly guards endpoint drag-end coalescing, non-sketch legacy undo routing, and script transaction atomic undo semantics while phase validation evidence is updated for PH18-03 automation gates.**

## Performance

- **Duration:** 6m
- **Started:** 2026-04-04T08:40:05Z
- **Completed:** 2026-04-04T08:46:08Z
- **Tasks:** 3 completed (including human checkpoint approval)
- **Files modified:** 6

## Accomplishments
- Added endpoint regression tests for non-sketch drag-end legacy undo behavior and endpoint drag release-only coalescing boundaries.
- Added script transaction regression checks for command type integrity and noop transaction undo suppression.
- Updated `18-VALIDATION.md` to mark PH18-01..PH18-03 automated evidence green and set `nyquist_compliant: true`.
- Added arc endpoint undo branch-continuity fix plus regression coverage to prevent undo/redo inversion flips across ±π wrapping.
- Fixed GeometryManager delete crash when viewport-selected sketch entities are also selected in manager rows.
- Added viewport → GeometryManager selection highlight synchronization (including endpoint-point to owner-row mapping) for single/multi-select parity with ConstraintManager.
- Completed Task 3 human verification checkpoint with explicit approval after follow-up fixes; endpoint drag undo/redo granularity and mixed selection/delete behavior were re-checked and accepted.

## Task Commits

Each completed task was committed atomically:

1. **Task 1: Add regression gates for non-sketch invariants and undo coalescing boundary** - `f512a20` (test), `6d64f89` (feat)
2. **Task 2: Lock script transaction invariants and update phase validation evidence** - `6ae049d` (test), `8389199` (feat)
3. **Task 3 follow-up (from checkpoint repro): Fix endpoint drag undo pre-drag snapshot regression** - `6e2b611` (fix)
4. **Task 3 follow-up (new checkpoint repro): Fix arc endpoint undo inversion reliability** - `4a1bce4` (fix)
5. **Task 3 follow-up (new checkpoint repro): Fix GeometryManager delete crash + selection highlight sync** - `559f43d` (fix)
6. **Task 3: Human verify endpoint drag undo feel and single-step granularity** - Approved (user response: `approved`)

## Files Created/Modified
- `src/tests/endpoint_pick_test.c` - Added non-sketch drag-end legacy undo routing and endpoint release-boundary coalescing regressions.
- `src/app.c` - Switched transform drag-end undo commit loop to shared recorder helper.
- `src/undo_redo_exec.h` - Added `record_drag_end_move_for_entity` helper and noop guard for script transaction pushes.
- `src/ecs/ecs_scene.h` - Preserved arc endpoint angle branch continuity when applying participant local points to avoid ±π wrap flips.
- `src/tests/script_roundtrip_tests.c` - Added script apply transaction command-type and noop-push regression tests.
- `.planning/phases/18-add-undo-steps-for-endpoint-moves/18-VALIDATION.md` - Marked automated evidence green for PH18-01..PH18-03 and set compliance fields.
- `src/tests/endpoint_pick_test.c` - Added regression that validates endpoint drag undo restores the true pre-drag endpoint local point instead of origin.
- `src/tests/endpoint_pick_test.c` - Added regression that reproduces arc endpoint undo branch inversion and locks deterministic undo/redo branch continuity.
- `src/selection.h` - Added `selection_prune_dead` helper to invalidate stale selection entries after scene deletions.
- `src/ui/ui_entity_inspector.h` - Synced GeometryManager row selection with viewport selection, mapped endpoint-point picks to owner rows, and pruned selection around manager-driven deletes.
- `src/tests/endpoint_pick_test.c` - Added regression coverage for viewport→GeometryManager selection mapping and dead-selection pruning after entity delete.

## Decisions Made
- Reused shared helper logic for drag-end command path selection to avoid drift between runtime behavior and test expectations.
- Treated noop script transaction pushes as correctness issue for undo granularity and fixed inline.
- Capture endpoint drag-start snapshots from endpoint local geometry for endpoint entities; preserve transform snapshot path for non-endpoints.
- Keep arc endpoint replay angles on the nearest equivalent branch relative to current angle to eliminate wrap-induced inversion.
- Treat GeometryManager row highlight state as a pure projection of canonical scene selection to prevent stale local row selections and crash-prone delete paths.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Test/build path could not call app-local drag-end recorder**
- **Found during:** Task 1 (GREEN)
- **Issue:** RED tests referenced `record_drag_end_move_for_entity`, but the helper existed only as app-local behavior, causing unresolved symbol in test target.
- **Fix:** Added shared `record_drag_end_move_for_entity` helper in `undo_redo_exec.h` and routed app transform drag-end commit loop through it.
- **Files modified:** `src/undo_redo_exec.h`, `src/app.c`
- **Verification:** `ctest -R "endpoint_pick|scene_solver_drag" --test-dir build-vulkan -C Release --output-on-failure`
- **Committed in:** `6d64f89`

**2. [Rule 1 - Bug] Endpoint gizmo drag undo captured endpoint transform origin instead of endpoint local point**
- **Found during:** Task 3 human verify (UAT repro)
- **Issue:** Endpoint-point transforms remain at `(0,0,0)` while the moved endpoint position is stored in `GeometryComp.point`; using transform position as drag-start caused undo to snap endpoint geometry to origin.
- **Fix:** Added `record_drag_start_position_for_entity` helper and used it for gizmo transform drag-start snapshots. Endpoint entities now snapshot from endpoint geometry local point; non-endpoints continue to snapshot transform position. Added regression test for undo-to-pre-drag local point behavior.
- **Files modified:** `src/undo_redo_exec.h`, `src/app.c`, `src/tests/endpoint_pick_test.c`
- **Verification:** `ctest -R endpoint_pick --test-dir build-vulkan -C Release --output-on-failure`; `ctest -R "endpoint_pick|scene_solver_contract|scene_solver_drag|scene_solver_diagnostics|script_roundtrip_tests" --test-dir build-vulkan -C Release --output-on-failure`
- **Committed in:** `6e2b611`

**3. [Rule 1 - Bug] Arc endpoint undo could flip to inverse branch after undo/redo**
- **Found during:** Task 3 human verify follow-up (new repro)
- **Issue:** Replaying arc endpoint local points computed angle with `atan2f`, which can switch between equivalent branches at ±π and invert arc orientation/span after undo.
- **Fix:** In `scene_apply_local_point_to_participant` (arc path), added angular continuity normalization to keep new angle on the nearest branch to existing start/end angle before commit. Added endpoint regression exercising near-π branch wrap and asserting stable undo/redo behavior.
- **Files modified:** `src/ecs/ecs_scene.h`, `src/tests/endpoint_pick_test.c`
- **Verification:** `ctest -R endpoint_pick --test-dir build-vulkan -C Release --output-on-failure`; `ctest -R "endpoint_pick|scene_solver_contract|scene_solver_drag|scene_solver_diagnostics|script_roundtrip_tests" --test-dir build-vulkan -C Release --output-on-failure`
- **Committed in:** `4a1bce4`

**4. [Rule 1 - Bug] GeometryManager delete could crash when rows inherited stale viewport-selected entities**
- **Found during:** Task 3 human verify follow-up (new repro)
- **Issue:** GeometryManager maintained static row selection independent of canonical scene selection; when a row was selected after viewport pick state changes, delete-confirm path could iterate stale/dead selection entries and hit invalid entity lifetime assumptions.
- **Fix:** Added `selection_prune_dead` guard in selection subsystem, removed to-be-deleted entities from canonical selection before delete loop, pruned dead entries post-delete, and made GeometryManager row highlight a live projection of viewport/scene selection (including endpoint-point → owner-geometry mapping) for single/multi-select sync parity.
- **Files modified:** `src/selection.h`, `src/ui/ui_entity_inspector.h`, `src/tests/endpoint_pick_test.c`
- **Verification:** `ctest -R "endpoint_pick|scene_solver_contract|scene_solver_drag|scene_solver_diagnostics|script_roundtrip_tests" --test-dir build-vulkan -C Release --output-on-failure`; `cmake --build build-vulkan --config Release --target mdCAD`
- **Committed in:** `559f43d`

---

**Total deviations:** 4 auto-fixed (4 bugs)
**Impact on plan:** Fixes were required to make regression gates executable and preserve endpoint undo correctness (line and arc paths) with no scope creep.

## Known Stubs
None.

## Next Phase Readiness
- Plan 18-03 execution is fully complete, including checkpoint closure.
- Automated verifies for plan 18-03 are green, including endpoint drag-start snapshot regression coverage.
- Human checkpoint validated endpoint drag undo UX feel, GeometryManager delete stability after mixed viewport/manager selection, and viewport↔GeometryManager row highlight synchronization.

## Self-Check: PASSED
- FOUND: .planning/phases/18-add-undo-steps-for-endpoint-moves/18-03-SUMMARY.md
- FOUND: f512a20
- FOUND: 6d64f89
- FOUND: 6ae049d
- FOUND: 8389199
- FOUND: 6e2b611
- FOUND: 4a1bce4
- FOUND: 559f43d
