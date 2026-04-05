---
status: complete
phase: 18-add-undo-steps-for-endpoint-moves
source:
  - 18-01-SUMMARY.md
  - 18-02-SUMMARY.md
  - 18-03-SUMMARY.md
started: 2026-04-05T00:19:36.1970658+01:00
updated: 2026-04-05T13:14:40.573Z
---

## Current Test

[testing complete]

## Tests

### 1. Endpoint gizmo drag undo/redo remains one-step and geometry-driven
expected: Drag/release endpoint creates one undo step; Undo restores exact pre-drag geometry position; Redo reapplies full drag.
result: pass

### 2. Arc endpoint undo/redo preserves orientation (no branch inversion flip)
expected: Repeated start/end endpoint drags near +-pi undo/redo without arc inversion or inside-out flips.
result: pass

### 3. GeometryManager Delete is stable with mixed viewport + manager selection
expected: Selecting geometry in viewport, then selecting same row in GeometryManager and deleting via UI does not crash.
result: pass

### 4. Viewport selection and multi-selection highlight GeometryManager rows
expected: Single and multi-pick in viewport (including endpoint-point picks mapped to owners) reflect as GeometryManager row highlights.
result: pass

### 5. Delete then Undo restores endpoint-owner sync for lines and arcs
expected: After UI delete then undo, restored line/arc endpoints stay bound to owner geometry landmarks and gizmo edits drive geometry, not independent transform drift.
result: pass

### 6. Non-sketch manipulation and script transaction semantics remain unchanged
expected: Non-sketch line move undo/redo behaves as baseline, and script transaction undo/redo behavior remains stable.
result: pass

## Summary

total: 6
passed: 6
issues: 0
pending: 0
skipped: 0
blocked: 0

## Gaps

none.
