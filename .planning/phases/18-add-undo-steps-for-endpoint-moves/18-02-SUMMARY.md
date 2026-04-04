---
phase: 18-add-undo-steps-for-endpoint-moves
plan: 02
subsystem: undo
tags: [undo, sketch, endpoints, solver, scripting, testing]
requires:
  - phase: 18-add-undo-steps-for-endpoint-moves
    provides: endpoint-aware undo command contract and recorder wiring from 18-01
provides:
  - Endpoint undo replay now applies through scene-authoritative owner/endpoint sync helpers.
  - Sketch-owned endpoint replay triggers solver auto-request and script re-emit side effects.
  - Regression coverage for endpoint replay sync and sketch/non-sketch side-effect boundaries.
affects: [undo-redo, sketch-editing, endpoint-interaction, script-transactions]
tech-stack:
  added: []
  patterns:
    - Endpoint replay uses shared undo helper path that resyncs owner and endpoint entities.
    - Sketch-only side effects are gated by parent sketch detection during endpoint replay.
key-files:
  created: []
  modified:
    - src/undo_redo_exec.h
    - src/tests/endpoint_pick_test.c
    - src/tests/scene_solver_contract_test.c
key-decisions:
  - "Centralize endpoint replay behavior in undo_replay_endpoint_participant_move for apply/unapply parity."
  - "Invoke scene_solver_request_auto and scene_script_reemit_for_sketch only when endpoint owner resolves to a sketch."
patterns-established:
  - "Undo/redo endpoint replay updates owner geometry, marks renderable dirty, then resyncs endpoint entities before sketch side effects."
  - "Scene-side effect regressions are validated with dedicated sketch and non-sketch replay tests."
requirements-completed: [PH18-02]
duration: 9m
completed: 2026-04-04
---

# Phase 18 Plan 02: Endpoint replay now preserves sketch-side contracts Summary

**Endpoint undo/redo replay now follows scene-authoritative sync and sketch-gated solver/script side effects, matching live endpoint mutation semantics.**

## Performance

- **Duration:** 9m
- **Started:** 2026-04-04T08:33:59Z
- **Completed:** 2026-04-04T08:38:39Z
- **Tasks:** 2
- **Files modified:** 3

## Accomplishments
- Added endpoint replay helper in undo executor and routed apply/unapply to owner+endpoint resync flow.
- Enforced sketch-only replay side effects by invoking solver auto-request and script re-emit only for sketch-owned endpoint owners.
- Added replay regression coverage for undo/redo endpoint-owner synchronization and sketch/non-sketch side-effect expectations.

## Task Commits

Each task was committed atomically:

1. **Task 1: Add endpoint command apply/unapply path via scene sync helpers** - `f12ed1c` (test), `b938556` (feat)
2. **Task 2: Enforce sketch-only side effects in endpoint replay without touching script transaction semantics** - `301b1a3` (test), `95e6091` (feat)

_Note: TDD tasks produced RED and GREEN commits._

## Files Created/Modified
- `src/undo_redo_exec.h` - Added `undo_replay_endpoint_participant_move` helper and sketch-gated side-effect replay path for `CMD_MOVE_ENDPOINT_PARTICIPANT`.
- `src/tests/endpoint_pick_test.c` - Added undo/redo replay assertions that endpoint entities stay synchronized with owner geometry.
- `src/tests/scene_solver_contract_test.c` - Added sketch replay side-effect checks and non-sketch invariance checks.

## Decisions Made
- Consolidated endpoint replay logic into one helper to keep apply/unapply behavior symmetric and reduce drift.
- Preserved script transaction semantics by only changing endpoint command replay branch; `CMD_SCRIPT_APPLY_TRANSACTION` behavior remained untouched.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered
- Initial RED run appeared green due to stale test binary; rebuilding the explicit target produced expected failures and validated TDD flow.

## Known Stubs
None.

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- Plan 18-02 truths are satisfied: endpoint replay sync is deterministic and sketch side effects are replayed correctly.
- Ready for 18-03 regression hardening and validation evidence closure.

## Self-Check: PASSED
- FOUND: .planning/phases/18-add-undo-steps-for-endpoint-moves/18-02-SUMMARY.md
- FOUND: f12ed1c
- FOUND: b938556
- FOUND: 301b1a3
- FOUND: 95e6091
