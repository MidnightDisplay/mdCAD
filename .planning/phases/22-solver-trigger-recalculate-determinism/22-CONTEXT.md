# Phase 22: Solver Trigger + Recalculate Determinism - Context

**Gathered:** 2026-04-07
**Status:** Ready for planning

<domain>
## Phase Boundary

Deliver deterministic sketch solver trigger behavior and bounded recalculate semantics so committed sketch edits autosolve reliably, manual recalc is authoritative, and driving LENGTH/ANGLE constraints behave predictably.

This phase hardens existing solver behavior. It does **not** add new grouped axis constraints (Phase 23) or advanced arc/line-arc constraint families (Phase 24).

</domain>

<decisions>
## Implementation Decisions

### Auto-solve trigger semantics
- **D-01:** Auto-solve should use a **short debounce queue** rather than immediate per-mutation solve.
- **D-02:** Debounce default is **50ms**.
- **D-03:** If new mutations arrive while auto-solve is pending, requests are **coalesced** into the pending solve (no request fan-out).
- **D-04:** Manual **Recalculate Sketch** runs immediately and **clears pending auto-solve queue**.

### Recalculate pass policy
- **D-05:** Phase 22 must expose **position + angle tolerances in UI** (not internal-only hidden tolerances).
- **D-06:** Default max pass count is **10**.
- **D-07:** If pass-cap is hit without convergence, solver status becomes **Error** with explicit `"max passes reached"` diagnostic.
- **D-08:** On successful convergence, prior failure implication highlighting clears **immediately** (preserve existing successful-solve clear contract).

### Driving LENGTH/ANGLE contract
- **D-09:** For unsatisfiable driving LENGTH/ANGLE constraints: **no geometry mutation**, explicit failure diagnostic, and implicated constraints highlighted.
- **D-10:** For satisfiable driving LENGTH/ANGLE constraints: apply geometry update **atomically in one successful solve commit** (no gradual multi-click convergence UX).

### Migration safety guardrails
- **D-11:** Preserve diagnostics ring behavior unchanged: identical-consecutive dedupe, cap 100, explicit clear action only.
- **D-12:** Preserve solver authority boundary as hard requirement: runtime decisions stay in `scene_solver_*`; UI/app remain thin callers.

### the agent's Discretion
- Exact field names and storage location for the new tolerance UI-exposed values.
- Exact queue flush implementation details, provided 50ms debounce + coalescing + manual override semantics remain intact.
- Exact diagnostic message text for max-pass failures, as long as explicit reason is present and testable.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Milestone and phase contracts
- `.planning/ROADMAP.md` — Phase 22 goal, requirements, success criteria, and dependency boundary.
- `.planning/REQUIREMENTS.md` — `SRLV-01`, `SRLV-02`, `SRLV-03`, `SRLV-05` acceptance contract for this phase.
- `.planning/PROJECT.md` — v1.3 milestone intent and solver-reliability-first scope.
- `.planning/STATE.md` — active milestone continuity and current focus.

### Upstream solver/constraint decisions that must be preserved
- `.planning/phases/12-solver-control-and-constrained-interaction/12-CONTEXT.md` — prior solve trigger policy, diagnostics lifecycle, and implication behavior contracts.
- `.planning/phases/17-constraint-driven-geometry-solving/17-CONTEXT.md` — deterministic solver semantics and unsat/no-mutation guardrails.
- `.planning/phases/20-finalize-phase-17-endpoint-ux-and-verification-closure/20-CONTEXT.md` — closure posture for explicit diagnostics and deterministic evidence-first behavior.
- `.planning/phases/21-traceability-closure-and-re-audit-readiness/21-CONTEXT.md` — audit-grade truthfulness and closure discipline.

### Code and test anchors
- `src/ecs/ecs_scene.h` — `scene_solver_request_auto`, `scene_solver_request_recalculate`, status/diagnostic/implication contracts.
- `src/ui/ui_entity_inspector.h` — Auto-solve toggle, recalc button, and diagnostics presentation surface.
- `src/app.c` — constrained interaction paths that already call solver APIs and must remain thin.
- `src/constraints/constraint_types.h` — current legality model for LENGTH/ANGLE/ALONG constraints used as baseline.
- `src/components/sketch_comp.h` — solver counters, status fields, and diagnostics ring storage.
- `src/tests/scene_solver_contract_test.c` — existing deterministic/transactional recalc tests to extend.
- `src/tests/scene_solver_diagnostics_test.c` — diagnostics behavior regression anchors.
- `src/tests/scene_solver_drag_test.c` — drag + solver interaction contract anchors.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `scene_solver_request_auto(...)` already supports pending/coalesced request semantics and serial tracking; Phase 22 extends this with explicit debounce-flush policy.
- `scene_solver_request_recalculate(...)` already enforces transactional failure/no-mutation for unsat pathways and provides a direct place to add bounded pass-loop behavior.
- Inspector solver panel already includes Auto-solve and Recalculate controls, making it the natural host for tolerance and pass-cap surfaces.

### Established Patterns
- Solver behavior authority belongs in `scene_solver_*` APIs; UI/app call into scene APIs rather than re-implementing solve decisions.
- Diagnostics are explicit, deterministic, and user-clearable; this behavior is already anchored in prior phases and tests.
- Regression expectations favor deterministic/idempotent outcomes with explicit failure implication payloads.

### Integration Points
- Add debounce queue flush behavior in scene-level solver orchestration without moving logic into UI/app callsites.
- Extend sketch solver config/state to include UI-exposed tolerance values and default pass-cap policy.
- Expand solver contract tests to cover debounce/coalescing/manual override, pass-cap stop reason, and LENGTH/ANGLE atomic success/failure behavior.

</code_context>

<specifics>
## Specific Ideas

- “Recalculate should be authoritative” — one click should run immediate solve attempt with deterministic outcome and clear reason on failure.
- Avoid user-visible “gradual convergence requiring repeated clicks”; successful solves should commit atomically per attempt.
- Keep diagnostics trustworthy and concise by preserving existing dedupe and explicit clear model.

</specifics>

<deferred>
## Deferred Ideas

- Grouped `ALONG X/Y/Z` constraints across mixed point participant sets (Phase 23).
- Advanced arc/line-arc constraints (arc-center axis relation, endpoint tangency, arc endpoint angle) (Phase 24).

</deferred>

---

*Phase: 22-solver-trigger-recalculate-determinism*
*Context gathered: 2026-04-07*
