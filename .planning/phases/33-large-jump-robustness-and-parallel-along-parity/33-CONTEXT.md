# Phase 33: Large-Jump Robustness and PARALLEL/ALONG Parity - Context

**Gathered:** 2026-04-10
**Status:** Ready for planning

<domain>
## Phase Boundary

Stabilize large-jump editing in mixed arc/line constrained sketches and enforce parity across geometrically equivalent `PARALLEL` and `ALONG` arrangements, with deterministic and actionable failure diagnostics.

This phase is scoped to `SROB-01..03`, `DIAG-01..02`, and `PARI-01..02`. It does not include milestone closure rerun sign-off (`DIAG-03`, Phase 34) or broader solver-family expansion beyond the targeted robustness/parity surfaces.

</domain>

<decisions>
## Implementation Decisions

### Carry-forward constraints from prior phases
- **D-01:** Solver behavior remains transactional: unsatisfied edits must not partially mutate scene geometry.
- **D-02:** Fixed participants remain hard anchors; movement resolution must occur through non-fixed participants.
- **D-03:** Explicit family diagnostics and deterministic implication ordering remain mandatory.
- **D-04:** No implicit coincidence fallback is permitted (preserve Phase 32 explicit coincidence posture).

### Large-jump solve strategy
- **D-05:** Implement a two-stage solve approach for large-jump operations: (1) large-jump preprojection/staging, then (2) normal iterative constraint passes.
- **D-06:** The large-jump strategy must remove user-observed "wiggle to latch" behavior in feasible quarter-arc closed-loop arrangements.

### Failure contract for unsatisfied large-jump edits
- **D-07:** If two-stage solve still cannot satisfy constraints, rollback hard transactionally to the last valid geometry state.
- **D-08:** On rollback, publish explicit failure implication and diagnostic reason(s) for recovery.
- **D-09:** After an infeasible attempt, immediate follow-up edits must remain responsive (no stale-failure lock/deadlock behavior).

### PARALLEL / ALONG parity policy
- **D-10:** Phase 33 enforces geometric-equivalence parity: equivalent `PARALLEL` and `ALONG` setups should map to the same feasibility/outcome class.
- **D-11:** Mirrored and participant-order variants of equivalent setups must preserve parity and determinism.

### Deterministic diagnostics taxonomy
- **D-12:** Lock a deterministic family+reason diagnostic taxonomy for this phase, including large-jump unsatisfied, parity-class mismatch, and max-passes classes.
- **D-13:** Diagnostic and implication ordering must remain stable for identical operation sequences.

### the agent's Discretion
- Exact low-level preprojection math and pass handoff mechanics for the two-stage solve, provided D-05..D-09 hold.
- Exact diagnostic text wording/format, provided family+reason taxonomy and deterministic ordering are preserved.
- Exact fixture split across existing solver contract/drag/pass-policy/diagnostics suites.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Phase and requirement contracts
- `.planning/ROADMAP.md` — Phase 33 goal, dependency, requirement mapping, and success criteria.
- `.planning/REQUIREMENTS.md` — `SROB-01..03`, `DIAG-01..02`, and `PARI-01..02` requirement contracts.
- `.planning/PROJECT.md` — v1.5 robustness scope and constraints.
- `.planning/STATE.md` — current continuity and sequencing context.

### User workflow evidence and problem framing
- `docs/improvements/solver-user-workflow-robustness.md` — source user-reported large-jump stall, parity mismatch, and diagnostics pain points.

### Upstream phase decisions to preserve
- `.planning/phases/26-line-line-constraint-coverage/26-CONTEXT.md` — deterministic canonicalization and transactional line-line solve posture.
- `.planning/phases/27-principal-axis-line-along-reliability/27-CONTEXT.md` — `ALONG` semantics and deterministic mixed-constraint expectations.
- `.planning/phases/28-tangency-drag-robustness/28-CONTEXT.md` — dragged-participant authority and transactional infeasible rollback behavior.
- `.planning/phases/29-active-sketch-line-gizmo-endpoint-authority/29-CONTEXT.md` — active sketch drag authority and grouped interaction expectations.
- `.planning/phases/32-explicit-coincidence-authoring-semantics/32-CONTEXT.md` — explicit coincidence lifecycle and no implicit fallback policy.

### Code and test anchors
- `src/ecs/ecs_scene.h` — solver recalc loop, drag feasibility precheck (`scene_solver_can_apply_drag`), failure implication and diagnostics.
- `src/constraints/constraint_types.h` — legality and constraint family contracts (`PARALLEL`, `ALONG_*`).
- `src/components/constraint_comp.h` — constraint representation and participant descriptors.
- `src/app.c` — interactive drag/authoring call paths that trigger solver behavior.
- `src/tests/scene_solver_contract_test.c` — mixed-constraint contract and transactional outcome assertions.
- `src/tests/scene_solver_drag_test.c` — large-delta drag and rollback responsiveness behavior.
- `src/tests/scene_solver_pass_policy_test.c` — mirrored/order parity and deterministic pass-policy regressions.
- `src/tests/scene_solver_diagnostics_test.c` — deterministic diagnostics ordering and family-specific error assertions.
- `src/tests/endpoint_pick_test.c` — legality and participant-role signature coverage.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `scene_solver_can_apply_drag(...)` in `src/ecs/ecs_scene.h` already supports bounded projection-style feasibility checks and can seed stage-1 large-jump behavior.
- `scene_solver_set_drag_anchor(...)` / `scene_solver_clear_drag_anchor(...)` provide existing drag-authority intent plumbing.
- Existing deterministic implication/diagnostic infrastructure (`scene_solver_set_failure_implication`, ordered implication participants) is available for taxonomy hardening.
- Existing parity-oriented tests in `scene_solver_pass_policy_test.c` (mirrored + ordering variants) provide a direct regression scaffold for PARI requirements.

### Established Patterns
- Scene-level solver APIs remain authority; UI/app surfaces act as thin orchestrators.
- Recalculate behavior is transactional with explicit unsatisfied diagnostics instead of silent fallback.
- Deterministic behavior is validated with repeated reruns and ordering-invariant test fixtures.

### Integration Points
- Extend large-jump handling in `scene_solver_request_recalculate(...)` and adjacent drag-feasibility paths in `src/ecs/ecs_scene.h`.
- Normalize parity outcome classification across `PARALLEL` and `ALONG` branches for equivalent geometry classes.
- Expand deterministic diagnostic reason mapping and ordering assertions in `scene_solver_diagnostics_test.c`.
- Add/extend targeted deterministic and parity cases in contract/drag/pass-policy suites.

</code_context>

<specifics>
## Specific Ideas

- Use a two-stage large-jump path (preprojection + normal passes) as the primary anti-stall strategy.
- Keep failure behavior strict and reversible: rollback to last valid state with explicit implication and diagnostics.
- Enforce geometric-equivalence parity between `PARALLEL` and `ALONG` outcome classes, including mirrored/order variants.
- Introduce deterministic reason taxonomy without weakening existing family-specific diagnostic clarity.

</specifics>

<deferred>
## Deferred Ideas

- Final closure rerun sign-off (`DIAG-03`) remains in Phase 34.
- Broad solver-family optimization outside SROB/DIAG/PARI scope is deferred.

</deferred>

---

*Phase: 33-large-jump-robustness-and-parallel-along-parity*
*Context gathered: 2026-04-10*
