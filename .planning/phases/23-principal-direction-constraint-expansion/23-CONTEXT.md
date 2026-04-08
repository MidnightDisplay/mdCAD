# Phase 23: Principal-Direction Constraint Expansion - Context

**Gathered:** 2026-04-08
**Status:** Ready for planning

<domain>
## Phase Boundary

Deliver principal-direction constraint support (`ALONG X`, `ALONG Y`, `ALONG Z`) across pairs/groups of point participants, including standalone points and sub-entity landmark points (line endpoints and arc landmark points), while keeping recalculate deterministic when directional constraints coexist with `ANGLE`.

This phase expands existing directional-constraint capability. It does **not** include advanced arc/line-arc semantic constraints (`ARCI-*`) reserved for Phase 24.

</domain>

<decisions>
## Implementation Decisions

### Directional authoring scope
- **D-01:** `ALONG X`, `ALONG Y`, and `ALONG Z` must support participant pairs and larger groups (within existing participant limits).
- **D-02:** Supported participant sources must include:
  - standalone point entities
  - line endpoint landmark points (`POINT_A`, `POINT_B`)
  - arc landmark points (`POINT_A`, `POINT_B`, `CENTER`)
- **D-03:** Existing single-line directional constraint behavior remains backward-compatible where currently legal.

### Legality and participant model
- **D-04:** Directional legality must move from single-line-only to point-participant signatures aligned with AXIS-01..04.
- **D-05:** Sub-entity landmark roles must be legal for directional constraints (line/arc endpoint descriptors are first-class participants).
- **D-06:** Keep deterministic selection legality contracts shared between UI authoring and script apply validation.

### Solver/runtime behavior
- **D-07:** Recalculate must remain functional when directional constraints coexist with `ANGLE` constraints (`SRLV-04`).
- **D-08:** Directional solving must preserve deterministic transactional behavior: explicit failure path and no partial-mutation leaks on unsatisfied solves.
- **D-09:** Solver authority remains in `scene_solver_*` APIs; UI and app loop stay thin callers.

### Validation posture
- **D-10:** Phase 23 requires executable regression coverage for directional legality and solve behavior across standalone + landmark participants.
- **D-11:** Include targeted coexistence tests (`ALONG*` + `ANGLE`) to prevent reintroducing non-functional recalc pathways.

### the agent's Discretion
- Exact participant-count bounds enforcement strategy within existing `CONSTRAINT_MAX_PARTICIPANTS`.
- Exact directional solve implementation details (projection strategy/order) as long as deterministic contracts hold.
- Exact split of test fixtures across existing solver test binaries vs new phase-specific test target(s).

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Milestone and phase contracts
- `.planning/ROADMAP.md` — Phase 23 goal, requirements, success criteria, and dependency boundary.
- `.planning/REQUIREMENTS.md` — `AXIS-01`, `AXIS-02`, `AXIS-03`, `AXIS-04`, `SRLV-04` acceptance contract.
- `.planning/PROJECT.md` — v1.3 milestone constraints and solver reliability posture.
- `.planning/STATE.md` — continuity and current phase position.

### Upstream baseline from Phase 22
- `.planning/phases/22-solver-trigger-recalculate-determinism/22-CONTEXT.md`
- `.planning/phases/22-solver-trigger-recalculate-determinism/22-RESEARCH.md`
- `.planning/phases/22-solver-trigger-recalculate-determinism/22-VALIDATION.md`
- `.planning/phases/22-solver-trigger-recalculate-determinism/22-VERIFICATION.md`

### Code and test anchors
- `src/constraints/constraint_types.h` — legality signatures and directional type rules.
- `src/components/constraint_comp.h` — constraint type enum and participant descriptor storage.
- `src/components/endpoints_comp.h` — endpoint/landmark participant model (`POINT_A`, `POINT_B`, `CENTER`).
- `src/ecs/ecs_scene.h` — constraint creation + recalculate orchestration + scene solver authority.
- `src/scripting/sketch_script_apply.h` — script-side legality validation path sharing selection rules.
- `src/ui/ui_entity_inspector.h` — constraint authoring and selection-based legality surface.
- `src/tests/scene_solver_contract_test.c` — transactional solve contracts.
- `src/tests/scene_solver_pass_policy_test.c` — deterministic pass policy behaviors.
- `src/tests/endpoint_pick_test.c` — endpoint role legality/pick interaction anchors.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- Directional constraint types already exist in `constraint_type_t` (`CONSTRAINT_ALONG_X/Y/Z`) and label/name infrastructure.
- Participant descriptors and endpoint role metadata are already represented in constraint components and endpoint components.
- Solver transactional and diagnostics contracts are already hardened in Phase 22 and should be reused.

### Established Patterns
- Constraint legality is centralized through `constraint_type_is_selection_legal(...)` and reused across authoring flows.
- Script apply path reuses legality checks, so legality changes must remain synchronized and deterministic.
- Scene solver orchestrates all runtime decision-making; UI and frame loop call scene APIs.

### Current Gaps to Close
- Directional legality currently rejects sub-entity point roles and only accepts single-line signatures.
- Directional solve behavior for point groups/landmark points is not yet implemented to satisfy AXIS-01..04.
- Coexistence path (`ALONG*` + `ANGLE`) needs explicit solver/runtime and regression coverage to satisfy SRLV-04.

</code_context>

<specifics>
## Specific Ideas

- Treat directional constraints as axis-equality constraints across participant points (group-consistent coordinate projection by axis).
- Keep endpoint/landmark participants first-class by resolving descriptors to concrete point candidates before solve.
- Prefer expanding existing solver contract tests with focused directional fixtures before adding new suites, unless isolation is necessary.

</specifics>

<deferred>
## Deferred Ideas

- Arc-center-axis relation and tangency semantics (`ARCI-01..04`) remain Phase 24.
- Broad regression-packaging closure (`V13-01`) remains Phase 25.

</deferred>

---

*Phase: 23-principal-direction-constraint-expansion*
*Context gathered: 2026-04-08*

