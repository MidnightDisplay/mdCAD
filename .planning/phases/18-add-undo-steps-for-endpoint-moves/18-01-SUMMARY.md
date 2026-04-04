---
phase: 18-add-undo-steps-for-endpoint-moves
plan: 01
subsystem: undo
tags: [undo, sketch, endpoints, gizmo, inspector, testing]
requires:
  - phase: 17-constraint-driven-geometry-solving
    provides: native endpoint point entities and sketch-scoped endpoint sync behavior
provides:
  - Endpoint-aware undo command contract keyed by owner participant metadata
  - Drag-end and inspector endpoint commit routing to endpoint-aware undo recording
  - Regression tests covering endpoint undo command contract and commit boundaries
affects: [undo-redo, sketch-editing, endpoint-interaction]
tech-stack:
  added: []
  patterns:
    - Endpoint move undo records owner-local participant points instead of transform position
    - Endpoint undo command emission remains coalesced at interaction/session boundaries
key-files:
  created: []
  modified:
    - src/undo_redo.h
    - src/undo_redo_exec.h
    - src/app.c
    - src/ui/ui_entity_inspector.h
    - src/tests/endpoint_pick_test.c
key-decisions:
  - "Use a dedicated CMD_MOVE_ENDPOINT_PARTICIPANT payload with owner entity, role, sub-index, and old/new local point."
  - "Keep legacy CMD_SET_POSITION and CMD_SET_POINT_POSITION recording behavior unchanged for non-endpoint paths."
patterns-established:
  - "Gizmo transform drag-end checks endpoint metadata and records endpoint-aware undo for endpoint entities only."
  - "Inspector endpoint point edits record endpoint-aware undo only on ImGui deactivation boundary."
requirements-completed: [PH18-01]
duration: 64m
completed: 2026-04-04
---

# Phase 18 Plan 01: Add undo steps for endpoint moves Summary

**Endpoint drag/edit undo now records owner-participant local geometry deltas so a single undo step reverts endpoint movement semantics instead of transform no-ops.**

## Performance

- **Duration:** 64m
- **Started:** 2026-04-04T08:25:39Z
- **Completed:** 2026-04-04T09:29:47Z
- **Tasks:** 2
- **Files modified:** 5

## Accomplishments
- Added endpoint-aware undo command contract (`CMD_MOVE_ENDPOINT_PARTICIPANT`) with owner/role/sub-index and old/new local endpoint points.
- Added endpoint-aware record helper and wired endpoint command routing in gizmo drag-end and inspector endpoint edit commit boundaries.
- Extended `endpoint_pick` regression coverage for endpoint undo contract, endpoint drag/edit boundary semantics, and legacy command path stability.

## Task Commits

Each task was committed atomically:

1. **Task 1: Add endpoint-aware undo command contract and helpers** - `c36de86` (test), `9180e0b` (feat)
2. **Task 2: Wire drag-end and inspector commit to endpoint-aware recorder** - `3218959` (test), `8f7face` (feat)

_Note: TDD tasks produced RED and GREEN commits._

## Files Created/Modified
- `src/undo_redo.h` - Added endpoint move command type/payload and command name mapping.
- `src/undo_redo_exec.h` - Added endpoint move record helper, endpoint command recorder, and apply/unapply execution handling.
- `src/app.c` - Routed transform-mode drag-end endpoint selections to endpoint-aware undo recording.
- `src/ui/ui_entity_inspector.h` - Routed endpoint point edit deactivation commits to endpoint-aware undo recording.
- `src/tests/endpoint_pick_test.c` - Added contract and interaction-boundary undo regressions plus legacy-path stability checks.

## Decisions Made
- Use endpoint owner-local participant point payload as undo source of truth for endpoint movement semantics.
- Keep existing non-endpoint transform/point undo commands untouched and route only endpoint-point entities to new command path.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered
- Endpoint RED test additions initially passed because build cache still used the previous test binary; rebuilding `endpoint_pick` produced the expected compile/link failures for missing symbols before GREEN implementation.

## Known Stubs
None.

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- Plan 18-01 objective is complete with endpoint-aware command contract and recorder wiring in both targeted interaction paths.
- Ready for downstream endpoint undo replay/validation hardening in subsequent Phase 18 plans.

## Self-Check: PASSED
- FOUND: .planning/phases/18-add-undo-steps-for-endpoint-moves/18-01-SUMMARY.md
- FOUND: c36de86
- FOUND: 9180e0b
- FOUND: 3218959
- FOUND: 8f7face

