# Phase 28: Tangency Drag Robustness - Context

**Gathered:** 2026-04-09
**Status:** Ready for planning

<domain>
## Phase Boundary

Harden line-end/arc-end tangency authoring and editing so feasible drags stay solvable, mirrored interactions behave consistently, and infeasible edits fail transactionally with explicit diagnostics while preserving immediate solver responsiveness for subsequent edits.

This phase is limited to tangency drag robustness (`TRDG-01..04`) inside the existing transactional sketch solver. It does not include active-sketch line gizmo authority fixes (Phase 29) or solver architecture documentation/closure gates (Phase 30).

</domain>

<decisions>
## Implementation Decisions

### Drag authority and anchor precedence
- **D-01:** The directly dragged participant is authoritative for that interaction and should remain user-draggable (subject to its active degrees-of-freedom limits from existing constraints).
- **D-02:** Tangency solve adjustments should propagate from the dragged participant to connected geometry, instead of overriding or freezing the dragged handle in feasible cases.
- **D-03:** Keep this interaction contract scoped to tangency drag robustness in Phase 28; broader cross-constraint drag policy unification is a future consideration.

### Shared-point and adjacent-handle mobility
- **D-04:** In feasible tangency setups, both the shared tangency point and adjacent non-shared handles must remain draggable.
- **D-05:** For each drag operation, solver reconciliation should be centered around the handle the user actually dragged (shared or adjacent), then restore tangency/coincidence as required.
- **D-06:** Avoid the prior “aggressively pinned shared point” behavior that prevents practical editing in fillet-like corner workflows.

### Mirrored interaction determinism
- **D-07:** Equivalent mirrored interactions (left/right, orientation-mirrored setup with same constraint set) must produce consistent feasibility outcomes.
- **D-08:** Determinism requirements remain selection-order and rerun stable for tangency-related mixed constraints in feasible/infeasible parity scenarios.

### Infeasible edit failure policy
- **D-09:** Infeasible tangency edits must rollback transactionally with no partial geometry mutation.
- **D-10:** Failure feedback must remain explicit and family-specific (line-arc endpoint tangency), not generic or silent.
- **D-11:** After an infeasible edit, solver must remain responsive for immediate follow-up edits (no deadlock/stuck state).

### the agent's Discretion
- Exact low-level tangency solve ordering/heuristics within transactional boundaries, as long as D-01..D-11 and `TRDG-01..04` are preserved.
- Exact fixture distribution across existing solver contract/drag/diagnostics suites.
- Exact wording polish for diagnostics, provided family specificity is retained.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Phase and milestone contracts
- `.planning/ROADMAP.md` — Phase 28 goal, dependencies, requirements mapping, and success criteria.
- `.planning/REQUIREMENTS.md` — `TRDG-01..04` acceptance contracts and v1.4 scope boundaries.
- `.planning/PROJECT.md` — v1.4 robustness-first posture and milestone constraints.
- `.planning/STATE.md` — continuity and current sequencing context.

### Upstream decisions to preserve
- `.planning/phases/24-advanced-arc-line-arc-constraint-expansion/24-CONTEXT.md` — ARCI tangency legality/runtime baseline and transactional semantics.
- `.planning/phases/25-regression-and-reliability-closure/25-CONTEXT.md` — strict deterministic reliability/diagnostics closure policy.
- `.planning/phases/26-line-line-constraint-coverage/26-CONTEXT.md` — canonical ordering, fixed-anchor posture, and explicit unsatisfied diagnostics model.
- `.planning/phases/27-principal-axis-line-along-reliability/27-CONTEXT.md` — legality/runtime parity and mixed-constraint determinism expectations.

### Code and behavior anchors
- `src/constraints/constraint_types.h` — legality signatures and endpoint role contracts for line/arc tangency participants.
- `src/ecs/ecs_scene.h` — drag anchor lifecycle, transactional recalc path, and tangency branch failure semantics.
- `src/tests/scene_solver_contract_test.c` — tangency coincidence+tangent correctness, transactional unsat, and drag-anchor behavior regression anchors.
- `src/tests/scene_solver_diagnostics_test.c` — family-specific unsatisfied diagnostic assertions for tangency failures.
- `src/tests/scene_solver_pass_policy_test.c` — deterministic pass-policy guardrails for mixed-constraint recalc stability.
- `src/tests/scene_solver_drag_test.c` — drag interaction contracts and solver integration behavior.
- `src/CMakeLists.txt` — targeted solver test target registration used by reliability gates.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `scene_solver_set_drag_anchor(...)` / `scene_solver_clear_drag_anchor(...)` in `src/ecs/ecs_scene.h` already encode dragged-participant intent and can remain the authority handshake for drag-centric solving.
- `scene_solver_request_recalculate(...)` already stages candidate updates transactionally and supports family-specific failure reasons.
- Existing tangency branch (`CONSTRAINT_LINE_ARC_ENDPOINT_TANGENCY`) already enforces coincidence+tangent checks and explicit unsatisfied reasons; this is the primary hardening surface.

### Established Patterns
- Scene-owned solver authority (`scene_solver_*`) with UI/app as thin callers.
- Transactional rollback on unsatisfied constraints with implication/diagnostic surfacing.
- Deterministic regression posture enforced through focused solver test suites.

### Integration Points
- Adjust tangency drag-anchor precedence and participant movement resolution in `src/ecs/ecs_scene.h` tangency handling.
- Add/extend mirrored and shared-vs-adjacent drag cases in `scene_solver_contract_test` and `scene_solver_drag_test`.
- Preserve/expand explicit tangency family diagnostics assertions in `scene_solver_diagnostics_test`.

</code_context>

<specifics>
## Specific Ideas

- User intent: direct manipulation should feel authoritative — “the selected point/line should be draggable freely within its current DoF constraints, and connected entities should adjust to satisfy constraints.”
- User noted this may be a broader UX principle beyond tangency and should be researched carefully before generalization.

</specifics>

<deferred>
## Deferred Ideas

- Generalized “dragged participant authority” policy across all constraint families (beyond tangency) — candidate future phase after dedicated research.

</deferred>

---

*Phase: 28-tangency-drag-robustness*
*Context gathered: 2026-04-09*
