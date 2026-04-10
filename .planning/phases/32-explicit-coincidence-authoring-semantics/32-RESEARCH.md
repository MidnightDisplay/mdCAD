# Phase 32 Research: Explicit Coincidence Authoring Semantics

## Objective
Research implementation approach for explicit coincidence authoring semantics so planning can produce executable, deterministic Phase 32 work.

## Inputs Read
- .planning/phases/32-explicit-coincidence-authoring-semantics/32-CONTEXT.md
- .planning/REQUIREMENTS.md
- .planning/STATE.md
- .planning/ROADMAP.md
- docs/improvements/solver-user-workflow-robustness.md
- src/app.c
- src/constraints/constraint_types.h
- src/ecs/ecs_scene.h
- src/scripting/sketch_script_apply.h
- src/tests/scene_solver_contract_test.c
- src/tests/scene_solver_diagnostics_test.c
- src/tests/script_roundtrip_tests.c

## Phase Description
### Phase 32: Explicit Coincidence Authoring Semantics **Goal**: Users get explicit, durable coincidence semantics when authoring composite ArcAxisLine and line-end/arc-end tangency relations. **Depends on**: Phase 31 **Requirements**: COIN-01, COIN-02 **Success Criteria** (what must be TRUE):   1. User authoring ArcAxisLine sees required center/axis coincidence represented explicitly rather than relying on implicit coupling.   2. User authoring line-end/arc-end tangency gets explicit endpoint coincidence semantics that remain intact after subsequent edits.   3. User can continue editing sketches containing these composite relations without hidden coupling drift or surprise relation breakage. **Plans**: TBD

## Requirement Coverage Focus
- COIN-01: Explicit center/axis coincidence for ArcAxisLine authoring.
- COIN-02: Explicit endpoint coincidence for line-end/arc-end tangency authoring, durable through edits.

## Existing Architecture Findings

### Authoring surface
- Constraint authoring is initiated in mdcad_draw_constraint_context_menu() (src/app.c) with participant descriptors from mdcad_collect_constraint_context(...).
- Menu currently creates only the chosen constraint via scene_add_constraint_to_sketch_with_descriptors(...); no composite authoring transaction exists yet.

### Legality contract
- constraint_type_is_selection_legal(...) (src/constraints/constraint_types.h) defines admissible signatures for CONSTRAINT_ARC_AXIS_LINE and CONSTRAINT_LINE_ARC_ENDPOINT_TANGENCY.
- Both are already endpoint/role-aware for tangency and entity-level for ArcAxisLine.

### Solver behavior
- src/ecs/ecs_scene.h currently solves ArcAxisLine and line-arc endpoint tangency directly with implicit coupling assumptions inside dedicated branches.
- Tangency branch already performs coincidence-first adjustments, then tangent reconciliation; however, this is internal and not represented as explicit authored coincidence constraints.

### Script apply/emit pipeline
- Script apply path (src/scripting/sketch_script_apply.h) creates constraints from descriptors but has no explicit pair-link semantics for owner constraint ↔ auto-created coincidence.
- Script roundtrip tests exist and can be extended for explicit pairing persistence requirements.

## Gray Areas Resolved from CONTEXT
Locked by discuss-phase and must be honored:
1. ArcAxisLine creation auto-adds explicit COINCIDENT(arc.center, nearest line endpoint).
2. Chosen endpoint binding is stable after creation; no auto-switch.
3. ArcAxisLine creation fails transactionally when explicit coincidence cannot be established.
4. Tangency creation always adds explicit endpoint coincidence.
5. On infeasible edits, coincidence stays authoritative while tangency may become unsatisfied with explicit diagnostics.
6. Lifecycle: deleting owner keeps paired coincidence; deleting coincidence keeps owner unsatisfied (no implicit fallback).
7. Script emit/reapply preserves explicit pairing metadata deterministically (no heuristic recomputation).

## Recommended Implementation Strategy

### 1) Introduce explicit pair-link metadata contract
Add lightweight metadata tying a composite owner constraint (ArcAxisLine/Tangency) to its authored coincidence companion. This should support:
- owner -> paired coincidence lookup
- paired coincidence -> owner lookup (if needed for diagnostics/lifecycle)
- deterministic serialization identity for script roundtrip

Likely touchpoints:
- constraint component metadata surface (src/components/constraint_comp.h or neighboring scene-side metadata registry)
- script emit/apply structures

### 2) Add transactional composite authoring helper in scene layer
Create a scene-level helper in src/ecs/ecs_scene.h used by UI authoring that:
- validates selection signature
- deterministically chooses coincidence participants (nearest endpoint rule for ArcAxisLine)
- creates explicit coincidence + owner constraint in one transaction
- rolls back both on any failure
- stores pair-link metadata

Rationale: keeps UI thin and preserves existing scene-owned mutation policy.

### 3) Update UI authoring callsite to use composite helper
In src/app.c constraint menu flow:
- for ArcAxisLine and line-arc endpoint tangency types, route through new composite helper
- for all other constraints, keep existing path

### 4) Solver policy integration
Use pair-link metadata during solve/diagnostics so behavior matches locked decisions:
- no implicit fallback when paired coincidence is removed
- owner remains and reports explicit unsatisfied diagnostics
- deterministic message family for coincidence-missing vs geometric-unsatisfied states

### 5) Script emit/apply persistence
Extend script schema/model to persist pair-link identity deterministically, ensuring repeat apply parity:
- emit explicit pair metadata
- apply recreates exact linkage
- if linkage invalid/missing, fail explicitly (or mark unsatisfied per locked policy)

## Validation Architecture

### Deterministic test surfaces to update
- src/tests/scene_solver_contract_test.c
  - creation semantics for ArcAxisLine composite authoring
  - creation semantics for tangency composite authoring
  - nearest-endpoint binding stability
  - owner-remains-unsatisfied behavior when paired coincidence removed
- src/tests/scene_solver_diagnostics_test.c
  - explicit diagnostics for missing-pair / unsatisfied tangency under preserved coincidence
- src/tests/script_roundtrip_tests.c
  - emit/apply preserves explicit pair metadata and deterministic identity
  - repeated apply parity for pair linkage

### Minimum gate recommendation
Use targeted plus canonical gate:
1. ctest --test-dir build-vulkan -C Release -R "scene_solver_contract|scene_solver_diagnostics|script_roundtrip_tests" --output-on-failure
2. Canonical 7-suite rerun parity gate used in previous closures.

## Risks and Mitigations

1. Risk: introducing pair metadata can break script compatibility.
   - Mitigation: versioned/default-safe parser behavior and explicit diagnostics for malformed pair refs.

2. Risk: UI + scene dual logic drift.
   - Mitigation: composite authoring only in scene helper; UI calls helper.

3. Risk: lifecycle semantics create orphan coincidence clutter.
   - Mitigation: explicit UX/diagnostic messaging and deterministic behavior tests; defer cleanup tooling to later phase if needed.

## Planning Guidance (for gsd-planner)
- Split plans into: (A) authoring + metadata plumbing, (B) solver/diagnostic behavior + tests + deterministic gate evidence.
- Each plan must directly map to COIN-01/COIN-02 and include deterministic rerun acceptance criteria.
- Ensure no requirements spill into Phase 33 large-jump/parity scope.

## Research Verdict
## RESEARCH COMPLETE

Phase 32 is ready for planning. Existing architecture already has the right extension points (UI selection descriptors, scene transactional mutation authority, deterministic test harnesses); work should focus on explicit pair authoring semantics and deterministic persistence/diagnostics rather than broad solver redesign.
