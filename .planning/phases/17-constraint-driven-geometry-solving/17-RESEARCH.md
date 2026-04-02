# Phase 17: Constraint-driven geometry solving - Research

**Researched:** 2026-04-02  
**Confidence:** High

## Core Findings

1. Keep solver authority centralized in `scene_solver_*` (`src/ecs/ecs_scene.h`).  
   Existing app/UI already depend on this contract, so Phase 17 should deepen solver internals instead of creating side paths.

2. Current behavior is mostly status/guardrail oriented, not full geometric solve application.  
   `scene_solver_request_recalculate(...)` updates serials/timestamp/status, and drag logic blocks fixed-geometry unsat cases, but does not yet enforce full v1.2 constraints as solving equations.

3. Endpoint authoring gap is real and structural.  
   Coincident chain/loop workflows need first-class endpoint/sub-entity pick targets in normal viewport flow, independent of gizmo Tab vertex mode.

4. Pick layering already has a suitable mechanism.  
   `pick_buffer` overlay points/lines can be used to keep endpoint targets visible/pickable above continuous primitives.

5. Determinism must be treated as a contract.  
   Deterministic failure/no-mutation, stable implicated-constraint reporting, and diagnostic dedupe should be locked with automated fixtures.

## Recommended Architecture Direction

### A) Transactional solve apply
- Build candidate solved state in scene-owned scratch buffers.
- Validate all constraints/tolerances.
- Commit geometry atomically only on success.
- On failure: no mutation + implication payload + diagnostic event.

### B) Live constrained drag
- Keep existing app drag contract (`scene_solver_can_apply_drag` + projected delta).
- Extend implementation so feasible drags run bounded live solve and project to valid motion.
- Unsat drag must preserve last valid solved state and emit immediate feedback.

### C) Endpoint/sub-entity selection model
- Add explicit pickable endpoint targets for line/arc endpoints.
- Route endpoint picks through normal viewport selection/constraint authoring.
- Do not depend on gizmo vertex mode for authoring Coincident endpoint constraints.

### D) Diagnostics quality
- Keep append-only history with explicit clear action.
- Add consecutive-duplicate suppression at scene level.
- Keep deterministic ordering for implicated constraints/entities.

## Integration Anchors (Code)

- `src/ecs/ecs_scene.h` — solver core contracts and status/drag APIs.
- `src/app.c` — drag update loop and failure feedback wiring.
- `src/ui/ui_entity_inspector.h` — solver controls + diagnostics panel.
- `src/components/geometry_comp.h` — geometry value surfaces updated by solver.
- `src/gpu/pick_buffer.h` — overlay pick/render behavior for endpoint targets.
- `src/constraints/constraint_selection.h` — implication participant highlight path.

## Risks and Mitigations

- **Risk:** performance regression during live solve.  
  **Mitigation:** bounded per-frame solve budget + graceful degrade path.

- **Risk:** flaky behavior from tolerance drift/order instability.  
  **Mitigation:** deterministic fixture corpus with stable ordering and explicit tolerances.

- **Risk:** endpoint picking conflicts with existing primitive picks.  
  **Mitigation:** dedicated endpoint pick IDs + overlay priority contract.

## Validation Architecture

### Test Infrastructure

| Property | Value |
|---|---|
| Framework | CTest + C test executables |
| Config file | `src/CMakeLists.txt` |
| Quick run | `ctest -R "solver|script_roundtrip" --test-dir build-vulkan -C Release --output-on-failure` |
| Full run | `ctest --test-dir build-vulkan -C Release --output-on-failure` |

### Wave 0 test requirements

- Add deterministic solver fixture tests (success + unsat + tolerance edges).
- Add endpoint-pick contract tests (endpoint precedence over continuous geometry).
- Add diagnostics dedupe tests (identical consecutive messages suppressed).

### Acceptance gate intent

- Every locked decision D-01..D-12 from `17-CONTEXT.md` maps to at least one automated check or explicit manual verification row.
- Phase close requires deterministic fixture pass on target build gate.

## Open Decisions for Planning (agent discretion areas)

- Per-frame solve budget defaults and degrade trigger thresholds.
- Exact tolerance constants by constraint type.
- Internal representation for endpoint participant identity (entity + sub-entity role/index).

## RESEARCH COMPLETE

Phase 17 should be planned as a scene-solver deepening phase with three primary deliverables:
1) true solve-and-apply geometry behavior,  
2) first-class endpoint/sub-entity authoring picks, and  
3) deterministic regression/validation gates.
