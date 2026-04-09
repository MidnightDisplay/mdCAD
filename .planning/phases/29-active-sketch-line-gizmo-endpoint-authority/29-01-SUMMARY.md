---
phase: 29-active-sketch-line-gizmo-endpoint-authority
plan: 01
subsystem: gizmo, solver, interaction
tags: [gizmo, midpoint, endpoint-authority, mixed-selection, solver]
provides:
  - Midpoint-anchored gizmo center behavior for active-sketch line selections.
  - Endpoint-authoritative rigid line translation for eligible active-sketch lines.
  - Mixed-selection routing preserving legacy behavior for non-eligible entities.
key-files:
  modified:
    - src/gizmo/gizmo.h
    - src/ecs/ecs_scene.h
    - src/app.c
    - src/tests/endpoint_pick_test.c
    - src/tests/scene_solver_drag_test.c
requirements-completed: [GZM-01, GZM-02, GZM-03]
completed: 2026-04-09
---

# Phase 29 Plan 01 Summary

Implemented active-sketch line midpoint anchoring and endpoint-authority drag routing.

- Gizmo center now uses:
  - midpoint for single active-sketch line
  - average of selected active-sketch line midpoints for multi-line selections
- Added `scene_apply_active_sketch_line_world_delta(...)` for rigid endpoint translation (`A`/`B`) with endpoint sync + solver/script refresh.
- Drag path in `app.c` now partitions selections into eligible active-sketch lines vs fallback entities and applies one projected delta to both subsets while preserving legacy fallback semantics.
- Added regression coverage for midpoint behavior, rigid endpoint translation, and mixed-selection guardrails.

Verification:

- `ctest --test-dir build-vulkan -C Release --output-on-failure -R "endpoint_pick|scene_solver_drag"` → pass

