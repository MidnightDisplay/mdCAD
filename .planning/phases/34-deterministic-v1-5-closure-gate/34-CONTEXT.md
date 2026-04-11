# Phase 34: Deterministic v1.5 Closure Gate - Context

**Gathered:** 2026-04-11
**Status:** Ready for planning

<domain>
## Phase Boundary

Lock deterministic v1.5 milestone closure evidence so reliability claims are reproducible and auditable.

This phase is scoped to `DIAG-03` closure-gate policy and evidence capture only. It does not add solver capabilities or broaden platform gate scope.

</domain>

<decisions>
## Implementation Decisions

### Closure gate command contract and platform scope
- **D-01:** Reuse Phase 30 closure contract exactly for v1.5 sign-off.
- **D-02:** Windows Vulkan remains the only required closure platform scope for this phase.
- **D-03:** Use one canonical 7-test gate command string (no command-format variants for closure evidence).
- **D-04:** Deterministic sign-off requires baseline pass plus immediate rerun pass using the identical canonical command.

### Evidence strictness policy
- **D-05:** Verification artifacts must include the exact command and baseline/rerun result summaries.
- **D-06:** Raw logs are not mandatory artifacts for closure; include only when needed for debugging.

### Divergence and flake handling
- **D-07:** If baseline and immediate rerun diverge for any reason (including flake), closure is a hard fail.
- **D-08:** Stabilization is required before sign-off can proceed.

### Historical warning-marker handling
- **D-09:** Historical human-UAT warning markers in lifecycle docs are treated as non-blocking metadata once `DIAG-03` deterministic gate requirements are satisfied.

### the agent's Discretion
- Exact wording/layout of verification summaries, while preserving D-05 and D-06.
- Exact location/shape of optional debug log excerpts when closure investigation is needed.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Phase and requirement contracts
- `.planning/ROADMAP.md` — Phase 34 goal, dependency, and success criteria.
- `.planning/REQUIREMENTS.md` — `DIAG-03` closure requirement contract and traceability.
- `.planning/PROJECT.md` — v1.5 closure-stage framing and deterministic-signoff posture.
- `.planning/STATE.md` — continuity and current milestone sequencing state.

### Upstream closure policy to preserve
- `.planning/phases/30-deterministic-closure-gate-windows-vulkan-solver-docs/30-CONTEXT.md` — canonical closure-gate contract baseline (Windows Vulkan + canonical command + baseline/rerun policy).
- `.planning/phases/33-large-jump-robustness-and-parallel-along-parity/33-CONTEXT.md` — prior-phase deterministic robustness posture feeding into v1.5 closure.
- `.planning/phases/33-large-jump-robustness-and-parallel-along-parity/33-VERIFICATION.md` — latest verification posture and closure handoff context.
- `.planning/phases/33-large-jump-robustness-and-parallel-along-parity/33-HUMAN-UAT.md` — historical UAT warnings context to be treated per D-09.

### User workflow evidence and closure framing
- `docs/improvements/solver-user-workflow-robustness.md` — source user scenarios that motivated deterministic closure confidence.

### Build/test anchors for deterministic gate
- `src/CMakeLists.txt` — canonical test registration and targeted suite anchors.
- `docs/VULKAN_WINDOWS.md` — Windows Vulkan environment/build guidance for closure runs.
- `src/tests/script_roundtrip_tests.c` — targeted gate member.
- `src/tests/scene_solver_contract_test.c` — targeted gate member.
- `src/tests/scene_solver_pass_policy_test.c` — targeted gate member.
- `src/tests/scene_solver_diagnostics_test.c` — targeted gate member.
- `src/tests/scene_solver_trigger_test.c` — targeted gate member.
- `src/tests/scene_solver_drag_test.c` — targeted gate member.
- `src/tests/endpoint_pick_test.c` — targeted gate member.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- Existing canonical 7-test deterministic gate composition is already established in project test registration and prior verification artifacts.
- Existing phase verification artifacts already follow command-plus-summary evidence style suitable for D-05.

### Established Patterns
- Closure policy is strict pass/fail with deterministic immediate rerun parity.
- Windows Vulkan remains the authoritative closure platform for milestone sign-off in current workflow.
- Deterministic evidence is treated as release-gating rather than informational.

### Integration Points
- Phase 34 planning/execution should wire deterministic closure evidence updates into the phase verification artifact.
- Lifecycle closure updates should preserve historical warning context while applying D-09 non-blocking interpretation once DIAG-03 is satisfied.

</code_context>

<specifics>
## Specific Ideas

- Preserve the exact Phase 30 closure-gate contract to avoid ambiguity and keep closure decisions consistent across milestones.
- Keep artifact requirements light (command + baseline/rerun summaries) while retaining strict hard-fail behavior on divergence.

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope.

</deferred>

---

*Phase: 34-deterministic-v1-5-closure-gate*
*Context gathered: 2026-04-11*
