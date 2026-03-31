---
phase: 11-constraint-authoring-ux
plan: 01
subsystem: ecs
tags: [constraints, ecs, sketch, naming, pick-id]
requires:
  - phase: 10-sketch-foundations-managers
    provides: sketch ownership model, geometry manager baseline
provides:
  - Constraint ECS component contracts and registration
  - Sketch-owned constraint lifecycle helpers with bidirectional links
  - D-04 auto-naming for sketches, sketch geometry, and constraints
affects: [CONS-01, CONS-05, SKCH-04, phase-11-02, phase-11-03]
tech-stack:
  added: []
  patterns: [header-only components, scene-centric constraint mutations, ECS child ownership]
key-files:
  created:
    - src/components/constraint_comp.h
    - src/components/constraint_participant_comp.h
  modified:
    - src/components/selectable_comp.h
    - src/components/sketch_comp.h
    - src/ecs/ecs_world.h
    - src/ecs/ecs_scene.h
key-decisions:
  - "Constraint entities are first-class sketch children with ConstraintComp; geometry backrefs live in ConstraintParticipantComp."
  - "Constraint glyph pick IDs use a dedicated reserved range below gizmo IDs."
  - "Default labels follow D-04 via scene helper ownership paths."
patterns-established:
  - "All constraint mutations go through scene_add_constraint_to_sketch/scene_remove_constraint helpers."
  - "Sketch metadata refresh derives constraint_count from real sketch children."
requirements-completed: [CONS-01, CONS-05]
completed: 2026-03-31
---

# Phase 11 Plan 01: Constraint ECS foundation summary

**Phase 11 ECS foundations are now in place: constraints are modeled as sketch-owned entities with bidirectional geometry links, dedicated pick ranges, and default naming contracts required by downstream UI and viewport flows.**

## Accomplishments

- Added `ConstraintComp` with full v1.2 type set, dimensional value fields, `driven` flag (CONS-05 model), and participant references.
- Added `ConstraintParticipantComp` for geometry-side back-reference tracking.
- Registered both components in ECS world init and exposed typed world getters/setters.
- Reserved a non-overlapping constraint glyph pick range in `selectable_comp.h`.
- Extended `SketchComp` with per-type naming counters for geometry/constraints.
- Added scene-level constraint lifecycle helpers:
  - create (`scene_add_constraint_to_sketch`)
  - remove (`scene_remove_constraint`)
  - dimensional updates (`scene_constraint_set_dimensional_value`)
  - bidirectional link maintenance on create/delete
- Implemented D-04 naming defaults:
  - `Sketch_N`
  - `[GeometryType]_N`
  - `[ConstraintType]_N`
  - description seeded from parent sketch name.
- Updated sketch metadata refresh to derive `constraint_count` from actual sketch constraint children.

## Validation

- `cmake --build build-vulkan --config Release --target mdCAD` (pass)

