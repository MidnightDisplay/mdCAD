# Architecture Research: v1.4 Solver Robustness + Sketch Gizmo Corrections

**Domain:** Integration strategy for v1.4 solver/gizmo/doc scope  
**Researched:** 2026-04-08  
**Confidence:** High

## Integration Points

- Keep authoring flow: selection -> legality check -> scene constraint add -> solver recalc -> diagnostics.
- Keep drag flow mediated through solver feasibility (`scene_solver_can_apply_drag(...)`) with transactional commit semantics.
- Keep failure implication lifecycle as authoritative UX feedback contract.

## Module Touchpoints

### Primary

- `src/ecs/ecs_scene.h` (solver runtime branches + transactional hardening)
- `src/constraints/constraint_types.h` (legality/runtime parity)
- `src/app.c` (active-sketch line gizmo routing to geometry endpoints)
- `src/gizmo/gizmo.h` (midpoint anchor behavior)

### Tests

- `src/tests/scene_solver_contract_test.c`
- `src/tests/scene_solver_pass_policy_test.c`
- `src/tests/scene_solver_drag_test.c`
- `src/tests/scene_solver_diagnostics_test.c`
- `src/tests/endpoint_pick_test.c`

### Documentation

- Add `docs/SOLVER_ARCHITECTURE.md`.

## Build Order (recommended)

1. Legality/runtime parity for new line-line constraints.
2. Runtime solver implementation for parallel/perpendicular.
3. ALONG line semantics fix + mixed-constraint determinism.
4. Tangency drag hardening + transactional rollback guarantees.
5. Active-sketch line gizmo midpoint/endpoints behavior fix.
6. Solver architecture docs + TL;DR primer.

## Verification Hooks

- Use targeted strict CTest gates and mandatory fresh reruns for determinism.
- Include manual checkpoint for active-sketch line midpoint/endpoints UX behavior.
