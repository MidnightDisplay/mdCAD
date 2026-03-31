---
phase: 11-constraint-authoring-ux
plan: 03
subsystem: app
tags: [constraints, viewport, picking, popup, hotkey]
requires:
  - phase: 11-constraint-authoring-ux
    provides: plan 01/02 ECS contracts + ConstraintManager workflows
provides:
  - C-key cursor-anchored context menu for applicable constraints
  - Constraint glyph overlay picking with dedicated ID range routing
  - LENGTH/ANGLE double-click popup editing with Accept/Cancel/Escape
affects: [CONS-02, CONS-03, CONS-04, D-01, D-03, D-05, D-06]
tech-stack:
  added: []
  patterns: [pick-buffer overlay routing, popup-at-cursor workflow, scene helper mutation path]
key-files:
  created:
    - src/constraints/constraint_glyphs.h
  modified:
    - src/app.c
key-decisions:
  - "C key opens a lightweight context menu window at cursor; Tab gizmo toggle remains unchanged."
  - "Constraint glyphs use dedicated reserved pick IDs and are routed before entity pick handling."
  - "Dimension popup edits flow through scene_constraint_set_dimensional_value for manager/popup mirroring."
patterns-established:
  - "Constraint menu and popup are frame-driven app UI surfaces keyed off explicit request/open state."
requirements-completed: [CONS-02, CONS-03, CONS-04]
completed: 2026-03-31
---

# Phase 11 Plan 03: Constraint interaction UX summary

**Phase 11 interaction wiring is implemented: `C` now opens an applicable-only constraint menu at cursor, constraint glyphs participate in overlay pick routing, and dimensional constraints support double-click popup editing with mirrored source-of-truth updates.**

## Accomplishments

- Added `src/constraints/constraint_glyphs.h`:
  - pick-ID classification helpers for constraint glyph range
  - glyph population into pick overlay primitives
  - constraint↔pick-id mapping
  - screen-anchor projection support for popup placement
  - zoom-stable glyph hit target sizing based on camera distance/FOV.
- Extended `app.c` with:
  - `C` key context collection and menu open request (D-01)
  - cursor-anchored one-shot constraint menu rendering/filtering/apply (D-06)
  - constraint glyph pick-buffer population and hover routing (D-03)
  - glyph click selection/highlight of participants
  - double-click LENGTH/ANGLE glyph popup with `InputFloat`, Accept/Cancel/Escape (D-05)
  - scene helper mutation path for dimensional value changes.
- Kept existing `Tab` gizmo mode toggle behavior unchanged.

## Validation

- `cmake --build build-vulkan --config Release --target mdCAD` (pass)

## Human Checkpoint

- Pending manual UAT from `11-03-PLAN.md` Task 3 checklist.

