---
phase: 29-active-sketch-line-gizmo-endpoint-authority
plan: 02
subsystem: undo-redo, interaction
tags: [undo, redo, grouped-interaction, endpoint-sync]
requires:
  - 29-01
provides:
  - One completed active-sketch line drag recorded as one grouped undo interaction.
  - Exact endpoint restoration on undo/redo for grouped active-sketch line drags.
  - Replay side effects (endpoint sync + solver/script refresh) for grouped command path.
key-files:
  modified:
    - src/undo_redo.h
    - src/undo_redo_exec.h
    - src/app.c
    - src/tests/endpoint_pick_test.c
requirements-completed: [GZM-04]
completed: 2026-04-09
---

# Phase 29 Plan 02 Summary

Implemented grouped undo/redo for active-sketch line endpoint-authority drags.

- Added grouped command type `CMD_BULK_LINE_ENDPOINTS`.
- Added grouped command recording helper `undo_cmd_push_bulk_line_endpoints(...)`.
- Added grouped replay path in undo/redo apply + unapply with explicit endpoint sync and per-sketch solver/script refresh.
- Updated drag-end recording in `app.c` to emit one grouped line-endpoint command per completed interaction for all changed eligible active-sketch lines.
- Kept fallback non-eligible undo semantics unchanged.
- Added grouped undo/redo regression tests including exact endpoint and endpoint-entity sync assertions.

Determinism gate:

- `ctest --test-dir build-vulkan -C Release --output-on-failure -R "endpoint_pick|scene_solver_drag" && ctest --test-dir build-vulkan -C Release --output-on-failure -R "endpoint_pick|scene_solver_drag"` → pass/pass

