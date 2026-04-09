# Phase 30: Deterministic Closure Gate (Windows Vulkan) + Solver Docs - Context

**Gathered:** 2026-04-09
**Status:** Ready for planning

<domain>
## Phase Boundary

Close v1.4 with a deterministic Windows Vulkan reliability gate and solver architecture documentation that is practical for future debugging and iteration.

This phase is limited to closure validation (`V14-01`, `V14-02`) and solver docs (`SDOC-01..03`). It does not add new solver capabilities, constraints, or interaction features.

</domain>

<decisions>
## Implementation Decisions

### Regression closure gate contract
- **D-01:** Use the exact canonical 7-test targeted suite already established in prior closure work:
  - `script_roundtrip_tests`
  - `scene_solver_contract`
  - `scene_solver_pass_policy`
  - `scene_solver_diagnostics`
  - `scene_solver_trigger`
  - `scene_solver_drag`
  - `endpoint_pick`
- **D-02:** Lock the canonical closure command regex string in Phase 30 docs/verification artifacts (no variant command patterns).
- **D-03:** Deterministic sign-off requires both a baseline pass and a mandatory immediate rerun pass using the same canonical 7-test command.
- **D-04:** Windows Vulkan closure run must include build + gate + rerun in this phase (`cmake --build build-vulkan --config Release` followed by canonical gate command and mandatory rerun).

### Determinism and failure policy
- **D-05:** Any flakiness during closure reruns is treated as failure and must be stabilized before Phase 30 closure.
- **D-06:** Verification evidence must include command plus result summaries; raw logs are optional unless needed for debugging.
- **D-07:** Closure sign-off scope remains the established Windows Vulkan path only for this phase (no cross-platform expansion).

### Solver documentation contract
- **D-08:** Create dedicated solver doc at `docs/solver/SOLVER_ARCHITECTURE.md`.
- **D-09:** Doc structure is locked as: Overview -> Solve pipeline -> Diagnostics flow -> Code anchors -> TL;DR debug primer.
- **D-10:** SDOC-02 mapping must include stage-by-stage file and key function anchors for authoring, recalc, diagnostics, and UI feedback.

### Literature and debug-primer depth
- **D-11:** Include a concise curated set of 3-6 high-signal references, each with one-line practical rationale.
- **D-12:** Prioritize practical geometric-constraint and numerical-robustness references over theory-only depth.
- **D-13:** TL;DR must include explicit "first place to look" debug entry points for failure families: unsatisfied constraints, drag rollback behavior, and pass-policy convergence stalls.

### the agent's Discretion
- Exact wording/formatting of the canonical command examples, as long as D-01..D-04 remain explicit and unambiguous.
- Exact section naming/details inside `docs/solver/SOLVER_ARCHITECTURE.md`, as long as D-08..D-13 and `SDOC-01..03` are fully covered.
- Exact verification artifact layout and summary style, as long as deterministic provenance is reproducible.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Phase and milestone contracts
- `.planning/ROADMAP.md` - Phase 30 goal, dependency, requirements mapping, and success criteria.
- `.planning/REQUIREMENTS.md` - `SDOC-01..03` and `V14-01..02` requirement contracts.
- `.planning/PROJECT.md` - v1.4 milestone scope and current closure intent.
- `.planning/STATE.md` - continuity position and current phase sequencing.

### Upstream reliability decisions to preserve
- `.planning/phases/25-regression-and-reliability-closure/25-CONTEXT.md` - strict deterministic closure policy and evidence expectations.
- `.planning/phases/27-principal-axis-line-along-reliability/27-CONTEXT.md` - deterministic mixed-constraint reliability expectations.
- `.planning/phases/28-tangency-drag-robustness/28-CONTEXT.md` - transactional drag behavior and explicit diagnostics posture.
- `.planning/phases/29-active-sketch-line-gizmo-endpoint-authority/29-CONTEXT.md` - endpoint-authority interaction and grouped undo/redo contracts.

### Runtime/build/docs anchors for this phase
- `src/CMakeLists.txt` - canonical registration of the 7 targeted tests used in closure gate.
- `src/ecs/ecs_scene.h` - solve/recalculate paths, pass policy, drag-anchor behavior, and diagnostic integration points.
- `src/constraints/constraint_types.h` - legality signatures and constraint family definitions.
- `src/app.c` - authoring/UI flow, solver trigger usage, and user-facing feedback integration.
- `src/tests/scene_solver_contract_test.c` - transactional and deterministic constraint behavior coverage.
- `src/tests/scene_solver_pass_policy_test.c` - pass-policy determinism and convergence behavior.
- `src/tests/scene_solver_diagnostics_test.c` - explicit unsatisfied-diagnostics coverage.
- `src/tests/scene_solver_trigger_test.c` - trigger/recalculate behavior contracts.
- `src/tests/scene_solver_drag_test.c` - drag + solver integration behavior contracts.
- `src/tests/endpoint_pick_test.c` - endpoint and active-sketch interaction behavior coverage.
- `src/tests/script_roundtrip_tests.c` - script import/export/runtime consistency coverage.
- `docs/VULKAN_WINDOWS.md` - Windows Vulkan build/runtime specifics for this repository.
- `docs/QUICKSTART.md` - canonical command surfaces and platform run patterns.
- `docs/feature-proposal/Sketches, Constraints, Scripting.md` - feature-domain intent backdrop for solver architecture framing.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- Existing 7-test CTest suite in `src/CMakeLists.txt` already maps directly to `V14-01` coverage domains.
- Solver behavior choke points already centralized in `scene_solver_*` flow inside `src/ecs/ecs_scene.h`.
- Existing docs (`docs/QUICKSTART.md`, `docs/VULKAN_WINDOWS.md`) can be referenced from the new solver architecture document to avoid duplicate command guidance.

### Established Patterns
- Closure policy in prior phases is strict pass/fail with deterministic rerun evidence and explicit diagnostics.
- Header-first subsystem style with app-level orchestration and scene-owned solver authority should be preserved in docs/code mapping.
- Verification artifacts in `.planning/phases/*` use command/result summaries as the durable evidence baseline.

### Integration Points
- Add `docs/solver/SOLVER_ARCHITECTURE.md` and cross-link it from existing docs/planning artifacts.
- Keep Phase 30 verification artifacts keyed to the locked canonical closure command and rerun pair.
- Map each solver-doc section to concrete files/functions used by current runtime and test coverage.

</code_context>

<specifics>
## Specific Ideas

- Keep the solver architecture doc practical and debug-oriented rather than theoretical.
- Make deterministic closure reproducible by locking command patterns and rerun policy explicitly.

</specifics>

<deferred>
## Deferred Ideas

- Cross-platform closure expansion (macOS/Linux co-equal sign-off) remains a future milestone concern.
- Any solver capability additions discovered during docs/closure work remain out of scope for Phase 30.

</deferred>

---

*Phase: 30-deterministic-closure-gate-windows-vulkan-solver-docs*
*Context gathered: 2026-04-09*
