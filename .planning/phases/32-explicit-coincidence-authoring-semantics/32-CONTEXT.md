# Phase 32: Explicit Coincidence Authoring Semantics - Context

**Gathered:** 2026-04-10
**Status:** Ready for planning

<domain>
## Phase Boundary

Make ArcAxisLine and line-end/arc-end tangency authoring explicitly coincedence-driven so users no longer depend on hidden coupling.

This phase is scoped to explicit coincidence authoring semantics (`COIN-01`, `COIN-02`) and edit-time durability for those semantics. It does not broaden solver family scope or absorb large-jump parity hardening work from Phase 33.

</domain>

<decisions>
## Implementation Decisions

### ArcAxisLine explicit coincidence authoring
- **D-01:** Creating `ArcAxisLine` must also create an explicit `COINCIDENT(arc.center, line.endpoint)` relation (no implicit-only center/axis coupling).
- **D-02:** The line endpoint for `COINCIDENT` is chosen automatically at author time as the nearest line endpoint to the arc center.
- **D-03:** After creation, the chosen endpoint binding is stable and must not auto-switch (`POINT_A`/`POINT_B`) during subsequent edits.
- **D-04:** If the explicit ArcAxisLine coincidence cannot be established, creation fails transactionally with an explicit ArcAxisLine coincidence diagnostic.

### Line-end/arc-end tangency explicit coincidence authoring
- **D-05:** Creating line-end/arc-end tangency must always create an explicit endpoint `COINCIDENT` relation alongside tangency.
- **D-06:** For edit outcomes where both cannot be simultaneously satisfied, explicit coincidence remains authoritative while tangency may become unsatisfied with explicit diagnostics.
- **D-07:** Hidden implicit coincidence behavior is not acceptable as fallback for this phase.

### Lifecycle semantics for auto-created coincidence relations
- **D-08:** Deleting the owning ArcAxisLine/tangency does **not** cascade-delete the paired auto-created `COINCIDENT`; the coincidence remains unless explicitly deleted by the user.
- **D-09:** If a paired explicit `COINCIDENT` is deleted while owner constraint remains, owner remains present and is reported unsatisfied with explicit diagnostics (no silent fallback).
- **D-10:** Script emit/reapply must preserve explicit pairings deterministically (stable IDs + pairing relationship metadata), not recompute pairings heuristically per apply.

### the agent's Discretion
- Exact internal metadata model for pairing ownership/linkage (field shape, storage location), provided D-08..D-10 hold.
- Exact UI wording and affordances used to communicate pair creation and unsatisfied-owner diagnostics.
- Exact test fixture decomposition across contract/diagnostics/drag/script suites, provided `COIN-01` and `COIN-02` are covered with deterministic rerun evidence.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Phase and requirement contracts
- `.planning/ROADMAP.md` — Phase 32 goal, requirement mapping (`COIN-01`, `COIN-02`), and success criteria.
- `.planning/REQUIREMENTS.md` — explicit coincidence requirement definitions and traceability.
- `.planning/PROJECT.md` — v1.5 robustness scope and non-negotiables.
- `.planning/STATE.md` — continuity and current milestone sequencing.

### User-reported workflow intent
- `docs/improvements/solver-user-workflow-robustness.md` — explicit request to replace implicit ArcAxisLine/tangency coincidence coupling with explicit coincidence authoring.

### Upstream phase decisions to preserve
- `.planning/phases/24-advanced-arc-line-arc-constraint-expansion/24-CONTEXT.md` — ARCI legality and deterministic ordering contracts.
- `.planning/phases/28-tangency-drag-robustness/28-CONTEXT.md` — drag authority and tangency robustness semantics.
- `.planning/phases/31-script-reapply-fidelity-foundation/31-CONTEXT.md` — deterministic script remap and descriptor fidelity posture.

### Implementation anchors
- `src/constraints/constraint_types.h` — legal participant signatures for ArcAxisLine and line-arc endpoint tangency.
- `src/app.c` — constraint context capture and menu-driven authoring flow (`mdcad_collect_constraint_context`, `mdcad_draw_constraint_context_menu`).
- `src/ecs/ecs_scene.h` — descriptor-based constraint creation and solver enforcement branches for ArcAxisLine and line-arc endpoint tangency.
- `src/scripting/sketch_script_apply.h` — script-side descriptor apply path for paired-constraint persistence.
- `src/tests/scene_solver_contract_test.c` — semantic contract regression anchors.
- `src/tests/scene_solver_diagnostics_test.c` — explicit unsatisfied-family diagnostic assertions.
- `src/tests/script_roundtrip_tests.c` — deterministic emit/reapply integrity for explicit pairing metadata.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `mdcad_collect_constraint_context(...)` in `src/app.c` already captures endpoint roles and selection signatures needed for explicit coincidence authoring.
- `scene_add_constraint_to_sketch_with_descriptors(...)` in `src/ecs/ecs_scene.h` already supports role-aware participant descriptors for deterministic explicit pairing.
- Existing `CONSTRAINT_COINCIDENT` creation path is already used in startup seed helpers (`mdcad_seed_add_coincident_line_endpoints`), showing known-good explicit coincidence authoring.

### Established Patterns
- Constraint legality is centralized in `constraint_type_is_selection_legal(...)` and reused across authoring and script apply.
- Solver failure policy is transactional + explicit diagnostics instead of permissive fallback.
- Deterministic behavior contracts are validated through targeted C tests and repeated rerun gates.

### Integration Points
- Extend `src/app.c` authoring path so ArcAxisLine/tangency creation emits explicit paired coincidence constraints with deterministic participant-role binding.
- Extend `src/ecs/ecs_scene.h` and script roundtrip surfaces to persist, reapply, and diagnose pair linkage deterministically.
- Add regression coverage for pair lifecycle operations (owner delete, coincidence delete, reapply) in solver/script tests.

</code_context>

<specifics>
## Specific Ideas

- ArcAxisLine explicit coincidence should pick nearest line endpoint at creation time, then keep that endpoint assignment stable.
- Tangency should always be authored with explicit endpoint coincidence, but on infeasible edits coincidence may remain while tangency reports explicit unsatisfied status.
- Pair linkage must survive script emit/reapply as authored, without heuristic reassignment.

</specifics>

<deferred>
## Deferred Ideas

- Legacy implicit-only sketch migration policy (automatic backfill vs opt-in migration tooling) was not locked in this discussion and can be decided during planning if needed.

</deferred>

---

*Phase: 32-explicit-coincidence-authoring-semantics*
*Context gathered: 2026-04-10*
