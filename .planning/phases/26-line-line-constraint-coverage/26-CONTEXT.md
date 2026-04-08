# Phase 26: Line-Line Constraint Coverage - Context

**Gathered:** 2026-04-08
**Status:** Ready for planning

<domain>
## Phase Boundary

Deliver deterministic, legality-validated `PARALLEL` and `PERPENDICULAR` constraints for sketch lines, including pair and supported group authoring, without breaking transactional solver guarantees.

This phase is limited to line-line constraint coverage and associated diagnostics/ordering behavior. It does not include ALONG-line fixes, tangency robustness, gizmo semantics changes, or solver architecture docs (handled in later v1.4 phases).

</domain>

<decisions>
## Implementation Decisions

### Perpendicular group semantics
- **D-01:** For `PERPENDICULAR` with 3+ selected lines, use an anchor-line contract: one canonical anchor line and every other participant must be perpendicular to that anchor.
- **D-02:** `PARALLEL` remains supported for pair and group selections, with all participating lines satisfying one shared parallel relation set.

### Deterministic participant canonicalization
- **D-03:** Canonical anchor selection is based on stable entity-id ordering, not click order.
- **D-04:** Participant normalization must make outcomes selection-order invariant for both pair and group authoring.

### Diagnostics and failure contract
- **D-05:** Invalid selections must be rejected at legality time with explicit type-specific messaging for line-line constraints.
- **D-06:** Unsatisfied solve diagnostics must explicitly name the constraint family (`PARALLEL` or `PERPENDICULAR`) and implicated entities.
- **D-07:** No silent failure path for rejected/unsatisfied line-line constraints.

### Fixed/free movement policy
- **D-08:** Fixed participants are hard anchors and must not be moved by solve adjustments.
- **D-09:** If all participants are fixed and unsatisfied, solver must fail transactionally with no geometry mutation.
- **D-10:** Movement is resolved through non-fixed participants while preserving deterministic anchor semantics.

### the agent's Discretion
- Exact diagnostic wording text and formatting in UI/log output, as long as family/implication clarity is preserved.
- Exact internal solve math/projection formulation for enforcing parallel/perpendicular under the locked behavioral contracts.
- Exact test fixture split across existing solver test binaries.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Phase and milestone contracts
- `.planning/ROADMAP.md` — Phase 26 goal, requirement mapping, and success criteria.
- `.planning/REQUIREMENTS.md` — `LCON-01..05` acceptance contracts.
- `.planning/PROJECT.md` — v1.4 robustness-first milestone constraints and scope boundaries.
- `.planning/STATE.md` — current continuity and active phase status.

### User-reported issue context
- `.plans/phase-26-issues-and-observations.md` — source issue list driving v1.4 and this phase.

### Upstream solver decisions to preserve
- `.planning/phases/23-principal-direction-constraint-expansion/23-CONTEXT.md` — deterministic directional legality/solver pattern and descriptor usage.
- `.planning/phases/24-advanced-arc-line-arc-constraint-expansion/24-CONTEXT.md` — transactional diagnostics posture for advanced constraints.
- `.planning/phases/25-regression-and-reliability-closure/25-CONTEXT.md` — strict deterministic regression/diagnostic closure posture.
- `.planning/phases/26-line-line-constraint-coverage/26-RESEARCH.md` — phase-specific implementation and risk analysis.

### Code and test anchors
- `src/constraints/constraint_types.h` — legality matrix for line-line constraints and participant role rules.
- `src/components/constraint_comp.h` — constraint type enum and participant descriptor storage.
- `src/ecs/ecs_scene.h` — constraint creation path and transactional solver recalculate loop.
- `src/app.c` — constraint authoring flow and legality-driven menu behavior.
- `src/tests/endpoint_pick_test.c` — legality contract tests around selection signatures.
- `src/tests/scene_solver_contract_test.c` — transactional and deterministic solve contracts.
- `src/tests/scene_solver_diagnostics_test.c` — diagnostics and implication behavior checks.
- `src/tests/scene_solver_pass_policy_test.c` — deterministic pass-policy/regression anchors.
- `src/CMakeLists.txt` — test target registration and closure gate wiring.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `constraint_type_is_selection_legal(...)` already centralizes legality decisions and is shared by UI authoring and script apply.
- `scene_add_constraint_to_sketch_with_descriptors(...)` already normalizes participant descriptors and is the right choke point for canonicalization.
- `scene_solver_request_recalculate(...)` already enforces transactional candidate staging and explicit failure implication lifecycle.

### Established Patterns
- Solver authority remains in scene-level solver APIs (`scene_solver_*`), with app/UI acting as thin callers.
- Deterministic behavior and explicit diagnostics are treated as contractual, not optional.
- Regression closure uses targeted CTest slices and strict pass/fail expectations.

### Integration Points
- Extend legality rules for `PERPENDICULAR` group semantics in `src/constraints/constraint_types.h`.
- Add participant canonicalization rules in the descriptor-based constraint creation path in `src/ecs/ecs_scene.h`.
- Add/extend runtime solve branches for `PARALLEL` and `PERPENDICULAR` in `scene_solver_request_recalculate(...)`.
- Expand targeted test coverage in solver legality/contract/diagnostics suites.

</code_context>

<specifics>
## Specific Ideas

- Lock anchor-line semantics for perpendicular groups to prevent ambiguous multi-line behavior.
- Make deterministic canonical ordering entity-id based to eliminate selection-order drift.
- Keep diagnostics family-specific and implication-rich so users can recover quickly from invalid/unsatisfied setups.

</specifics>

<deferred>
## Deferred Ideas

- Line `ALONG X/Y/Z` robustness corrections — Phase 27.
- Arc-line tangency drag hardening — Phase 28.
- Active-sketch line gizmo endpoint authority and midpoint behavior — Phase 29.
- Solver architecture documentation and closure rerun gate — Phase 30.

</deferred>

---

*Phase: 26-line-line-constraint-coverage*
*Context gathered: 2026-04-08*
