# Phase 25: Regression and Reliability Closure - Context

**Gathered:** 2026-04-08
**Status:** Ready for planning

<domain>
## Phase Boundary

Lock a stable, repeatable automated regression closure gate for the v1.3 sketch solver milestone, proving trigger integrity, iterative pass policy behavior, and legality/solve determinism for delivered constraint families.

This phase is regression-and-reliability closure only. It does not add new constraint features.

</domain>

<decisions>
## Implementation Decisions

### Regression matrix scope
- **D-01:** Baseline closure gate uses the current targeted solver+script suite (do not expand to full `ctest` for this phase).
- **D-02:** Baseline gate remains exactly these 7 tests:
  - `script_roundtrip_tests`
  - `scene_solver_contract`
  - `scene_solver_pass_policy`
  - `scene_solver_diagnostics`
  - `scene_solver_trigger`
  - `scene_solver_drag`
  - `endpoint_pick`
- **D-03:** Do not add extra sentinels (for example `mdCAD` build target) in Phase 25 scope unless needed to fix a discovered regression.

### Failure policy
- **D-04:** Strict pass/fail closure: any failing test blocks Phase 25 closure.
- **D-05:** If baseline gate fails, fix within Phase 25 before closure (no defer-as-known-fail policy).
- **D-06:** Flaky behavior is treated as failure; stabilize or rewrite to deterministic behavior before closure.
- **D-07:** Explicit per-family diagnostics behavior remains mandatory closure contract, not optional metadata.

### Evidence and provenance expectations
- **D-08:** Mandatory evidence is command + result summary in verification artifacts (raw output files are not required by default).
- **D-09:** Closure requires a fresh rerun at closure time, even if earlier phase runs were green.
- **D-10:** Verification posture is automation-first; manual checks are only required when a discovered regression specifically needs manual confirmation.

### Scope guardrails
- **D-11:** No new constraint features in Phase 25; regression closure only.
- **D-12:** New feature ideas discovered during Phase 25 are recorded as deferred/backlog items, not folded into execution scope.
- **D-13:** Closure remains on established Windows Vulkan gate scope for this phase (no cross-platform expansion in Phase 25).

### the agent's Discretion
- Exact test orchestration mechanics (single command vs staged command blocks), as long as D-01..D-13 remain satisfied.
- Exact wording/format for summary evidence in verification artifacts, as long as command + result is explicit and reproducible.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Milestone and phase contracts
- `.planning/ROADMAP.md` — Phase 25 goal, success criteria, dependency on completed Phase 24.
- `.planning/REQUIREMENTS.md` — `V13-01` requirement contract and v1.3 regression closure target.
- `.planning/PROJECT.md` — milestone intent and reliability-first constraints.
- `.planning/STATE.md` — current continuity, focus, and sequencing.

### Upstream phase decisions to preserve
- `.planning/phases/22-solver-trigger-recalculate-determinism/22-CONTEXT.md` — trigger/pass-policy and explicit diagnostic contracts.
- `.planning/phases/23-principal-direction-constraint-expansion/23-CONTEXT.md` — directional legality/solver coexistence decisions.
- `.planning/phases/24-advanced-arc-line-arc-constraint-expansion/24-CONTEXT.md` — ARCI legality/runtime/diagnostics decisions.
- `.planning/phases/24-advanced-arc-line-arc-constraint-expansion/24-VERIFICATION.md` — latest closure-level ARCI verification baseline.

### Regression gate code anchors
- `src/CMakeLists.txt` — canonical registration of the 7 targeted tests used by closure gate.
- `src/tests/scene_solver_contract_test.c` — deterministic transactional solver behavior contracts.
- `src/tests/scene_solver_pass_policy_test.c` — pass-cap/tolerance deterministic policy contracts.
- `src/tests/scene_solver_diagnostics_test.c` — explicit diagnostics assertions.
- `src/tests/scene_solver_trigger_test.c` — auto/manual trigger queue behavior contracts.
- `src/tests/scene_solver_drag_test.c` — drag + solver integration contracts.
- `src/tests/endpoint_pick_test.c` — legality and endpoint participant selection contracts.
- `src/tests/script_roundtrip_tests.c` — script apply/emit/runtime stability anchor.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- Existing focused CTest target set already mirrors v1.3 solver/script reliability surface (`src/CMakeLists.txt`, 7 named tests).
- Contract tests already encode transactional no-partial-mutation expectations and deterministic repeated recalc behavior.
- Diagnostics test harness already validates explicit message contracts, suitable for strict closure policy.

### Established Patterns
- Solver/runtime authority is scene-owned (`scene_solver_*`), with UI/app as thin callers.
- Failure semantics are explicit and testable (diagnostic + implication), not silent.
- Phase closures in this repo use targeted Windows Vulkan gates with documented verification artifacts.

### Integration Points
- Phase 25 planning should wire verification/validation artifacts directly to existing test binaries and current CTest naming.
- Any fix work triggered by failures should land in owning solver/legality/test files without widening feature scope.
- Closure docs should reference command-level reproducibility and fresh gate rerun evidence.

</code_context>

<specifics>
## Specific Ideas

- Keep Phase 25 tight: "lock what we already built" rather than introducing any new capability.
- Treat nondeterminism/flakiness as a core reliability defect, not a tooling inconvenience.
- Preserve explicit diagnostics quality as part of reliability, not just correctness.

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope.

</deferred>

---

*Phase: 25-regression-and-reliability-closure*
*Context gathered: 2026-04-08*

