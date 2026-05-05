# Phase 42 Discussion Log

**Date:** 2026-05-05
**Mode:** Interactive discuss-phase (`/gsd-discuss-phase 42`)

## Outcome

- Phase 42 discussion completed before planning.
- The phase boundary was confirmed from `.planning/ROADMAP.md` and `.planning/REQUIREMENTS.md`: refresh and delete linked large flat imports without geometry collapse, late churn, or orphaned render state.
- The user locked the refresh, teardown, safety, and acceptance decisions below for downstream planning.

## Decisions Captured

### Refresh convergence contract
- Settled linked auto-reloads and settled manual `Re-import now` runs must both return to exact 1:1 slot-buffer occupancy for the live entity footprint.
- Temporary overlap during an active refresh is acceptable.
- Cumulative slot-buffer growth across successive refreshes is not acceptable.

### Teardown behavior
- Deleting a linked flat root after prior refresh activity must fully remove its visible geometry and slot-buffer occupancy in the same action.
- Deleting while refresh is active must cancel in-flight or staged refresh work and still complete the delete immediately with no leftovers.
- Undo of such a delete restores only the last committed import state, never canceled staged refresh data.

### Cleanup anomaly safety
- If refresh commit/cleanup invariants fail late, keep the last committed import content and discard the staged refresh result.
- Disable automatic observe and surface a warning, but keep manual `Re-import now` available for explicit retry.

### Acceptance gate
- Phase 42 must prove four scenarios before closeout: linked auto-reload, repeated manual `Re-import now`, delete after settled refresh history, and delete while refresh is active.
- Closeout requires both deterministic automated regressions and a real-file `lamp_11.jsonl` manual pass with viewport, Scene Hierarchy, and slot-buffer evidence.
