---
phase: 17-constraint-driven-geometry-solving
plan: 05
subsystem: ui
tags: [endpoint-pick, checkpoint-remediation, crash-fix, sketch-sync]
requires:
  - phase: 17-constraint-driven-geometry-solving
    provides: Native sketch endpoint entities and point-context legality baseline
provides:
  - Crash-safe transform gizmo routing for non-sketch geometry and endpoint point entities
  - Bidirectional endpoint/owner synchronization for sketch line/arc notable vertices
affects: [D-10, D-11, D-12]
tech-stack:
  added: []
  patterns: [selection transform routing helper, endpoint metadata center-role support, endpoint sync regression tests]
key-files:
  created:
    - .planning/phases/17-constraint-driven-geometry-solving/17-05-SUMMARY.md
  modified:
    - src/app.c
    - src/components/endpoints_comp.h
    - src/ecs/ecs_scene.h
    - src/tests/endpoint_pick_test.c
    - src/ui/ui_entity_inspector.h
    - src/gpu/geometry_batch.h
    - src/ui/ui_visibility.h
key-decisions:
  - "Transform-mode gizmo deltas now route through scene-level selection delta application so endpoint point entities update owner geometry instead of mutating transform-only paths."
  - "Arc endpoint metadata now includes center-role endpoint binding to keep center/start/end endpoint entities synchronized in both directions."
patterns-established:
  - "Use scene_apply_transform_delta_for_selection for transform drag application to preserve sketch endpoint semantics and non-sketch baseline behavior."
requirements-completed: []
duration: in-progress
completed: 2026-04-03
---

# Phase 17 Plan 05: Endpoint checkpoint remediation summary (in progress)

Implemented remediation requested after failed human verification:

- Fixed transform-gizmo crash path for bare non-sketch entities by consolidating transform delta application into a scene helper that safely handles endpoint vs non-endpoint entities.
- Restored endpoint ↔ parent bidirectional synchronization for sketch line/arc:
  - Endpoint point drag updates owner line/arc geometry.
  - Owner geometry edits/resolves resync endpoint point entities.
  - Arc center is now represented and synchronized as endpoint metadata alongside start/end.
- Added regression coverage for:
  - non-sketch transform drag path remains stable and non-crashing,
  - line endpoint-to-owner sync both directions,
  - arc center/start endpoint-to-owner sync both directions.

## Remediation Verification Evidence

- `ctest -R endpoint_pick --test-dir build-vulkan -C Release --output-on-failure` ✅
- `ctest -R "scene_solver_contract|scene_solver_drag|scene_solver_diagnostics" --test-dir build-vulkan -C Release --output-on-failure` ✅

## Checkpoint-Failure Remediation Notes

- Addressed failure #1: bare non-sketch line/arc transform path now avoids endpoint-only mutation assumptions and preserves baseline transform behavior.
- Addressed failure #2: endpoint point entities and owner line/arc notable vertices now synchronize in both directions, including arc center support.
- Informational checkpoint note retained: coincident-visible morph behavior remains governed by current phase solver contract and is not expanded in this remediation pass.

## Follow-up remediation pass (after user re-check)

- Hardened sketch guards in `scene_is_sketch` to prevent invalid sketch checks on `0`/dead entities during transform drag paths.
- Added explicit endpoint-point fast-path handling in `scene_apply_transform_delta_for_selection` so endpoint and non-endpoint entities no longer share unsafe assumptions.
- Added direct endpoint-geometry synchronization API (`scene_sync_owner_geometry_from_endpoint_entity`) and wired it into inspector editing flow so endpoint point edits propagate to parent line/arc geometry immediately.
- Wired geometry edit paths to re-sync endpoint entities and re-request sketch solve/script re-emit when parent is a sketch.
- Added transform-gizmo geometry-mode hook to keep endpoint entities synchronized while editing geometry in-viewport.
- Included user-requested visual tuning tweaks:
  - point render size increased in `src/gpu/geometry_batch.h`,
  - minimum point-size slider lowered in `src/ui/ui_visibility.h`.
- Added new regression test `test_endpoint_direct_geometry_edit_syncs_owner_and_entities`.

