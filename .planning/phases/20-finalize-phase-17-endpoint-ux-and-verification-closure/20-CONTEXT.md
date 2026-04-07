# Phase 20: Finalize Phase 17 Endpoint UX and Verification Closure - Context

**Gathered:** 2026-04-07
**Status:** Ready for planning

<domain>
## Phase Boundary

Close Phase 17 endpoint UX/manual-verification debt and publish final Phase 17 verification evidence for `D-01..D-12`, with explicit closure of endpoint legality/menu flow concerns called out by the v1.2 audit.

This phase is verification and closure work first. It is not a net-new feature expansion phase.

</domain>

<decisions>
## Implementation Decisions

### Evidence strategy
- **D-01:** Default policy is to primarily reuse existing approved Phase 17 evidence (`17-UAT.md`, prior command evidence, prior summaries), with only minimal reruns where needed.
- **D-02:** Every reused requirement row must include explicit source citations (artifact path + specific section/row reference).
- **D-03:** If any row remains ambiguous after reuse, escalate that row to fresh targeted rerun and/or fresh manual recheck before marking `passed`.

### Verification artifact structure
- **D-04:** `17-VERIFICATION.md` is the authoritative closure artifact for Phase 17.
- **D-05:** `17-VALIDATION.md` remains a supporting strategy/evidence log and must be aligned with final closure state.
- **D-06:** `17-UAT.md` remains the executed manual proof artifact and should be preserved as canonical manual evidence.
- **D-07:** `17-VERIFICATION.md` must explicitly represent all `D-01..D-12` as row-level entries with evidence links.

### Scope and execution guardrails
- **D-08:** Use docs-first closure. Apply code changes only if a required closure check fails and the failure is reproducible.
- **D-09:** Any such code fix must be minimal and directly tied to unblocking a required closure row.

### Manual sign-off policy
- **D-10:** Reuse `17-UAT.md` tests 1-7 as the mandatory manual sign-off set.
- **D-11:** Run fresh manual/rerun evidence only for rows that become ambiguous or fail under current validation.

### the agent's Discretion
- Define the minimal rerun command set used to resolve ambiguous rows, as long as it remains auditable and requirement-linked.
- Choose exact verification table formatting and citation layout, while preserving per-row traceability.
- Decide whether a failing row needs command rerun, manual rerun, or both, based on reproducibility and evidence sufficiency.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Phase and milestone contracts
- `.planning/ROADMAP.md` — Phase 20 goal/scope/dependency and closure target.
- `.planning/REQUIREMENTS.md` — `D-01..D-12` requirement definitions and traceability expectations.
- `.planning/v1.2-MILESTONE-AUDIT.md` — authoritative gap source describing missing Phase 17 verification closure.
- `.planning/PROJECT.md` — milestone constraints and no-scope-creep posture for closure phases.
- `.planning/STATE.md` — continuity and prior decision trail affecting closure strategy.

### Upstream Phase 17 closure artifacts
- `.planning/phases/17-constraint-driven-geometry-solving/17-CONTEXT.md` — locked Phase 17 behavior decisions.
- `.planning/phases/17-constraint-driven-geometry-solving/17-VALIDATION.md` — existing validation strategy, addendum history, and currently open closure notes.
- `.planning/phases/17-constraint-driven-geometry-solving/17-UAT.md` — manual verification checklist/results (tests 1-7 baseline).
- `.planning/phases/17-constraint-driven-geometry-solving/17-05-SUMMARY.md` — latest remediation/closure narrative and evidence anchors.
- `.planning/phases/17-constraint-driven-geometry-solving/17-04-SUMMARY.md` — native endpoint entity baseline and scope guardrails.
- `.planning/phases/17-constraint-driven-geometry-solving/17-05-PLAN.md` — prior checkpoint contract and acceptance criteria context.

### Product and continuity references
- `docs/feature-proposal/Sketches, Constraints, Scripting.md` — product intent for constraint/endpoint authoring semantics.
- `CHECKPOINT.md` — continuity anchor for user-observed endpoint UX outcomes and closure history.

### Code and test anchors
- `src/app.c` — endpoint participant selection/signature assembly and constraint-menu trigger flow.
- `src/constraints/constraint_types.h` — legality filtering for participant-role context and point/endpoint semantics.
- `src/tests/endpoint_pick_test.c` — automated endpoint context legality, selection, and sync regression coverage.
- `src/ui/ui_viewport.h` — viewport interaction capture path used by endpoint selection flow.
- `src/ecs/ecs_scene.h` — scene-level endpoint-owner sync and selection transform application helpers.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `endpoint_pick_test` already covers key closure behaviors: point-context legality filtering, endpoint-to-owner mapping, overlay precedence, and sync regressions.
- `constraint_type_is_selection_legal(...)` in `src/constraints/constraint_types.h` centralizes legality semantics and is the stable anchor for point-vs-line option gating.
- Endpoint selection payload assembly in `src/app.c` already maps endpoint point entities to owner+role descriptors used by context/authoring paths.

### Established Patterns
- Verification closure is requirement-first and evidence-linked; status upgrades require explicit rationale and source links.
- Constraint participation semantics are centralized and reused across manager/menu/glyph flows rather than duplicated ad hoc.
- Existing phase closure practice prefers targeted reruns over broad suite reruns when scope is narrow and traceability is explicit.

### Integration Points
- Create/update `.planning/phases/17-constraint-driven-geometry-solving/17-VERIFICATION.md` as authoritative closure for `D-01..D-12`.
- Align `.planning/phases/17-constraint-driven-geometry-solving/17-VALIDATION.md` with final closure disposition and evidence map.
- Preserve and cite `.planning/phases/17-constraint-driven-geometry-solving/17-UAT.md` as manual proof baseline; rerun only ambiguous rows.
- If closure checks fail, apply minimal targeted fixes in existing endpoint legality/sync paths and immediately re-verify affected rows.

</code_context>

<specifics>
## Specific Ideas

- Make Phase 20 the audit-grade reconciliation point for Phase 17 by converting prior remediation evidence into one explicit authoritative verification matrix.
- Treat existing `17-UAT` 7/7 pass results as baseline truth, but require row-level citations so auditors can trace every final status claim.
- Keep closure focused: verification completeness and endpoint legality/menu-flow confidence, not additional UX expansion.

</specifics>

<deferred>
## Deferred Ideas

- Opportunistic endpoint UX polish beyond failing closure checks (future scope).
- Broader residual traceability cleanup outside `D-01..D-12` (handled by Phase 21).

</deferred>

---

*Phase: 20-finalize-phase-17-endpoint-ux-and-verification-closure*
*Context gathered: 2026-04-07*
