# Phase 27: Principal-Axis Line ALONG Reliability - Context

**Gathered:** 2026-04-09
**Status:** Ready for planning

<domain>
## Phase Boundary

Deliver deterministic, legality-validated `ALONG X`, `ALONG Y`, and `ALONG Z` behavior for sketch lines so authoring no longer immediately fails in feasible setups, including mixed-constraint combinations (`LENGTH`, `ANGLE`, connectivity) without breaking transactional solver guarantees.

This phase is limited to line `ALONG` reliability and deterministic mixed-constraint solve behavior. It does not include tangency drag hardening, active-sketch line gizmo semantics, or solver documentation closure work (handled in Phases 28-30).

</domain>

<decisions>
## Implementation Decisions

### Principal-axis semantics contract
- **D-01:** For line `ALONG X`, the line axis must be constrained parallel to world X (the 3D equivalent of "horizontal" in this model), not perpendicular-to-X lockstep behavior.
- **D-02:** For line `ALONG Y` and `ALONG Z`, apply the same axis-parallel contract relative to Y and Z respectively.
- **D-03:** Preserve descriptor-based line endpoint participants (`POINT_A`, `POINT_B`) as the authoritative line representation for `ALONG` solving; avoid reintroducing raw-entity-only semantics.

### Legality and authoring parity
- **D-04:** Line `ALONG` legality and runtime behavior must remain synchronized through shared legality gates (`constraint_type_is_selection_legal`) used by both UI authoring and script apply.
- **D-05:** Invalid `ALONG` signatures must continue explicit rejection; no silent acceptance path that later fails ambiguously in recalc.

### Solver behavior, determinism, and fixed-state policy
- **D-06:** `ALONG` solve branches must remain transactional: unsatisfied all-fixed setups fail without geometry mutation.
- **D-07:** Mixed `ALONG` + `LENGTH` + `ANGLE` + connectivity setups must be deterministic under immediate reruns (selection-order invariant where applicable).
- **D-08:** Fixed participants remain hard anchors; movement must be resolved through non-fixed participants only.

### the agent's Discretion
- Exact projection math implementation for axis-parallel line enforcement, as long as D-01..D-08 and ALIN-01..04 are satisfied.
- Exact diagnostic string wording/format, provided family/type clarity and implication visibility remain explicit.
- Exact split of regression fixtures across existing solver legality/contract/diagnostics suites.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Phase and milestone contracts
- `.planning/ROADMAP.md` — Phase 27 goal, dependency, requirement mapping, and success criteria.
- `.planning/REQUIREMENTS.md` — `ALIN-01..04` acceptance contracts and milestone traceability.
- `.planning/PROJECT.md` — v1.4 robustness-first scope boundaries and current milestone priorities.
- `.planning/STATE.md` — continuity, sequencing, and current phase position.

### User-reported issue context
- `.plans/phase-26-issues-and-observations.md` — source report for ALONG line failure and directional interpretation mismatch.

### Upstream decisions to preserve
- `.planning/phases/23-principal-direction-constraint-expansion/23-CONTEXT.md` — directional legality/descriptor model and deterministic coexistence baseline.
- `.planning/phases/24-advanced-arc-line-arc-constraint-expansion/24-CONTEXT.md` — transactional diagnostics/failure posture to preserve.
- `.planning/phases/25-regression-and-reliability-closure/25-CONTEXT.md` — strict deterministic closure policy and explicit diagnostics expectations.
- `.planning/phases/26-line-line-constraint-coverage/26-CONTEXT.md` — canonical ordering, fixed-anchor policy, and legality/runtime parity model now expected across solver families.

### Code and test anchors
- `src/constraints/constraint_types.h` — directional legality contracts (`CONSTRAINT_ALONG_X/Y/Z`).
- `src/ecs/ecs_scene.h` — transactional recalculate loop and current `ALONG` solve branch.
- `src/app.c` — constraint menu/authoring legality surface.
- `src/tests/scene_solver_contract_test.c` — directional transactional and determinism contracts.
- `src/tests/endpoint_pick_test.c` — legality signature coverage around directional constraints.
- `src/tests/scene_solver_diagnostics_test.c` — explicit failure-message regression anchors.
- `src/tests/scene_solver_pass_policy_test.c` — deterministic pass-policy/regression guard rails.
- `src/CMakeLists.txt` — target registration for closure-gate suites.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `constraint_type_is_selection_legal(...)` already centralizes `ALONG` legality and is shared across authoring/script paths.
- `scene_add_constraint_to_sketch_with_descriptors(...)` already stores descriptor-level participants and enforces legality parity before creation.
- `scene_solver_request_recalculate(...)` already has transactional candidate staging, implication tracking, and explicit unsatisfied reasons for `ALONG`.

### Established Patterns
- Solver authority remains in scene-level APIs; app/UI should stay thin legality/authoring callers.
- Deterministic behavior and explicit diagnostics are contractual quality gates, not optional polish.
- Fixed-state policy is hard-anchor + transactional failure when all-fixed and unsatisfied.

### Integration Points
- Update `ALONG` runtime branch in `src/ecs/ecs_scene.h` to enforce axis-parallel line semantics and mixed-constraint determinism.
- Keep/update legality contracts in `src/constraints/constraint_types.h` only where needed for runtime parity.
- Extend/adjust targeted tests in `scene_solver_contract`, `endpoint_pick`, and `scene_solver_diagnostics` to lock ALIN requirements and prevent directional regressions.

</code_context>

<specifics>
## Specific Ideas

- Preserve the user-intended semantics: "Along axis" means line direction is constrained to that axis, not that endpoints are forced into opposite/perpendicular behavior.
- Prioritize visible interactive reliability in active sketch editing for line constraints under mixed constraint loads.

</specifics>

<deferred>
## Deferred Ideas

- Tangency drag robustness corrections — Phase 28.
- Active-sketch line gizmo endpoint authority/midpoint anchor behavior — Phase 29.
- Solver architecture documentation + milestone closure rerun/sign-off — Phase 30.

</deferred>

---

*Phase: 27-principal-axis-line-along-reliability*
*Context gathered: 2026-04-09*
