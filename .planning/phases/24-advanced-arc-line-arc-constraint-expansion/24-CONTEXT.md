# Phase 24: Advanced Arc + Line-Arc Constraint Expansion - Context

**Gathered:** 2026-04-08
**Status:** Ready for planning

<domain>
## Phase Boundary

Deliver advanced arc/line-arc constraints for the sketch solver so users can author:
- arc-center axis relation against a line (perpendicular-to-line contract),
- line-end to arc-end tangency at a shared point,
- arc endpoint-angle control,
with deterministic solve behavior and explicit failure diagnostics when unsatisfiable.

This phase extends constraint legality + solve behavior inside the existing single-sketch transactional solver boundary. It does not add new constraint families outside ARCI-01..04.

</domain>

<decisions>
## Implementation Decisions

### Arc-center axis vs line (ARCI-01)
- **D-01:** Legal authoring shape is exactly one line entity + one arc entity (entity roles only; no sub-entity endpoint roles for this constraint type).
- **D-02:** Solve contract is axis-based: arc normal axis must be parallel or anti-parallel to the selected line direction.
- **D-03:** When both participants are editable, the arc is the primary moving/reorienting geometry and the line is treated as reference.
- **D-04:** If fixed-state constraints make the relation unsatisfiable, solver fails transactionally (no geometry mutation), emits explicit diagnostic, and sets implication highlighting.

### Line-end to arc-end tangency (ARCI-02)
- **D-05:** Legal authoring shape is exactly two endpoint participants: one line endpoint role and one arc endpoint role.
- **D-06:** Solve order is deterministic: enforce endpoint coincidence first (shared point), then enforce tangency at that shared point.
- **D-07:** Fixed/free policy: fixed participant anchors; free participant absorbs required solve delta.
- **D-08:** If both sides are fixed and unsatisfied, solver fails transactionally with explicit tangency diagnostic (no silent downgrade/fallback).

### Arc endpoint-angle constraint (ARCI-03)
- **D-09:** Authoring is endpoint-pair on the same arc (not single-arc entity mode). Selection order is semantic: first selected endpoint is the anchored reference.
- **D-10:** Value domain is normalized sweep magnitude in `[0, π]` (UI edits in degrees, solver stores/evaluates radians).
- **D-11:** Solve behavior honors anchored-reference contract: keep first-selected endpoint fixed as reference and move the second endpoint role to satisfy target angle.

### Diagnostics and deterministic recalculate (ARCI-04)
- **D-12:** Keep existing solver failure policy unchanged for ARCI constraints: transactional rollback on unsatisfied solve, explicit diagnostics row, implication highlighting, deterministic repeated recalculate.
- **D-13:** Add dedicated explicit unsatisfied messages per new family (arc-axis relation, line-arc endpoint tangency, arc endpoint-angle) instead of generic catch-all text.

### the agent's Discretion
- Exact low-level solve math implementation (projection/rotation formulation) as long as D-02/D-06/D-11 contracts and deterministic behavior hold.
- Exact diagnostic wording text, provided each ARCI family remains explicitly distinguishable.
- Exact fixture split across existing solver test binaries.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Milestone and phase contracts
- `.planning/ROADMAP.md` — Phase 24 goal, requirement mapping, success criteria.
- `.planning/REQUIREMENTS.md` — ARCI-01..04 acceptance contracts and v1.3 scope boundaries.
- `.planning/PROJECT.md` — v1.3 milestone intent and solver reliability posture.
- `.planning/STATE.md` — current milestone continuity and active phase focus.

### Upstream context to preserve
- `.planning/phases/22-solver-trigger-recalculate-determinism/22-CONTEXT.md` — autosolve/recalc/pass-policy and explicit diagnostics contracts.
- `.planning/phases/23-principal-direction-constraint-expansion/23-CONTEXT.md` — participant-descriptor legality posture and deterministic ALONG coexistence baseline.

### Code and behavior anchors
- `src/constraints/constraint_types.h` — legality signatures, participant role policy, and dimensional constraints baseline.
- `src/components/constraint_comp.h` — `constraint_type_t`, participant descriptor storage, dimensional metadata.
- `src/components/endpoints_comp.h` — endpoint ownership model and endpoint role semantics (`POINT_A`, `POINT_B`, `CENTER`).
- `src/app.c` — selection-to-descriptor mapping (`mdcad_collect_constraint_context`) and constraint menu authoring flow.
- `src/ecs/ecs_scene.h` — scene-owned constraint creation, solver request/recalculate, transactional failure + implication contracts.
- `src/ui/ui_entity_inspector.h` — constraint manager and dimensional editing surface.
- `src/scripting/sketch_script_apply.h` — script-side legality validation path (`constraint_type_is_selection_legal`) coupling.

### Regression anchors
- `src/tests/endpoint_pick_test.c` — participant-role legality and endpoint selection contracts.
- `src/tests/scene_solver_contract_test.c` — transactional deterministic solver contracts.
- `src/tests/scene_solver_pass_policy_test.c` — pass-cap, tolerance, and deterministic recalc behavior.
- `src/CMakeLists.txt` — canonical solver test targets (`scene_solver_contract`, `scene_solver_drag`, `scene_solver_trigger`, `scene_solver_pass_policy`, `scene_solver_diagnostics`, `endpoint_pick`).

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `constraint_participant_descriptor_t` and role model already support entity + endpoint landmarks; can encode ordered endpoint pairs directly.
- `mdcad_collect_constraint_context(...)` in `src/app.c` already captures endpoint descriptors from selection and preserves participant ordering.
- Scene solver in `src/ecs/ecs_scene.h` already has transactional candidate staging, multi-pass tolerance checks, and implication/diagnostic plumbing.
- Existing dimensional editing path (ConstraintManager + dimension popup) already handles degree/radian display conversion for `CONSTRAINT_ANGLE`.

### Established Patterns
- Solver authority remains scene-owned (`scene_solver_*`); UI/app are thin callers.
- Unsatisfied constraints must not partially mutate geometry; failure path is explicit and deterministic.
- Legality is centralized (`constraint_type_is_selection_legal`) and reused by both interactive authoring and script apply.

### Integration Points
- Extend `constraint_type_t`/legality tables for ARCI authoring signatures.
- Extend selection-context + constraint menu authoring to expose new ARCI types only when signatures are legal.
- Extend `scene_solver_request_recalculate(...)` pass loop with ARCI solve branches honoring fixed-state and ordered-endpoint semantics.
- Extend diagnostics + implication payload mapping for ARCI-specific failure reasons.
- Add/extend executable tests in existing solver/endpoint targets.

</code_context>

<specifics>
## Specific Ideas

- Predictability rule explicitly requested: for endpoint-pair angle authoring, first selected endpoint is the anchored reference and subsequent solve updates must respect that ordering.
- Tangency contract should be geometrically robust: shared-point coincidence is part of the tangency solve contract, not an optional side effect.

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope.

</deferred>

---

*Phase: 24-advanced-arc-line-arc-constraint-expansion*
*Context gathered: 2026-04-08*
