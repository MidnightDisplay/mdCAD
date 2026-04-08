---
phase: 23-principal-direction-constraint-expansion
plan: 03
subsystem: gizmo-and-ux-closure
tags: [constraints, along-axis, sketch-points, gizmo, undo, verification]
requires:
  - phase: 23-01
    provides: ALONG legality expansion for descriptor-based point participants
  - phase: 23-02
    provides: transactional ALONG runtime solve path and coexistence coverage
provides:
  - Active-sketch standalone point gizmo drag without geometry-tab dependency
  - Correct direct-geometry mutation path for standalone sketch points during untabbed drag
  - Geometry-vertex undo recording for standalone sketch point drag sessions
affects: [phase-23-closure, sketch-ux, gizmo-drag, undo-replay]
tech-stack:
  added: []
  patterns:
    - scene-owned solver authority with app-level mode routing
    - direct GEOM_POINT local-coordinate mutation for active sketch workspace drags
key-files:
  created:
    - .planning/phases/23-principal-direction-constraint-expansion/23-03-SUMMARY.md
  modified:
    - src/app.c
    - src/ecs/ecs_scene.h
    - src/tests/endpoint_pick_test.c
key-decisions:
  - "Untabbed drag for standalone points in active sketch must mutate GeometryComp point coordinates, not TransformComp position."
  - "Undo for direct-point drag must record CMD_SET_GEOMETRY_VERTICES payloads keyed to GEOM_POINT vertex 0."
patterns-established:
  - "Transform-mode gizmo drag can route to direct geometry path when selection is active-sketch standalone points."
  - "Scene helper scene_apply_standalone_sketch_point_world_delta centralizes solver/script side-effects for standalone sketch point drags."
requirements-completed: [AXIS-01, AXIS-02, AXIS-03, AXIS-04, SRLV-04]
duration: multi-session
completed: 2026-04-08
---

# Phase 23 Plan 03: UX Alignment + Final Verification Summary

**Phase 23 is closed with user-verified directional behavior and a delivered UX improvement: standalone sketch points in the active sketch now drag correctly without `Tab`, updating geometry coordinates directly and preserving deterministic undo behavior.**

## Performance

- **Duration:** multi-session
- **Completed:** 2026-04-08T15:29:37.2170339+01:00
- **Tasks:** 2 completed (including human checkpoint)

## Accomplishments

- Finalized directional UX/solver closure loop with approved manual verification outcomes for SKCH directional checks and ALONG+ANGLE coexistence.
- Implemented direct untabbed gizmo manipulation for standalone points in active sketch workspace:
  - Drag path now routes to geometry-local point mutation for eligible selections.
  - Solver auto-request and script re-emit remain scene-owned side-effects.
  - Undo records geometry vertex edits (`CMD_SET_GEOMETRY_VERTICES`) rather than transform position changes.
- Added regression coverage for standalone sketch point geometry-drag + undo replay behavior in `endpoint_pick_test`.

## Verification

- Targeted closure gate (Windows Vulkan) passed:
  - `endpoint_pick`
  - `scene_solver_contract`
  - `scene_solver_pass_policy`
  - `scene_solver_trigger`
  - `scene_solver_diagnostics`
  - `scene_solver_drag`
- Command:
  - `ctest -R "endpoint_pick|scene_solver_contract|scene_solver_pass_policy|scene_solver_trigger|scene_solver_diagnostics|scene_solver_drag" --test-dir build-vulkan -C Release --output-on-failure`
  - Result: **6/6 passed**

## Human Checkpoint Outcome

- Manual verification for plan 23-03 accepted.
- Follow-up UX request was implemented and re-verified by user:
  - "standalone sketch points move with gizmo without tab"
  - Result: **pass** after routing fix (geometry coordinates mutate directly).

## Next Phase Readiness

- Phase 23 is complete (3/3 plans).
- Roadmap continuity advances to Phase 24 (advanced arc + line-arc constraints).

## Self-Check: PASSED

