# Phase 31: Script Reapply Fidelity Foundation - Context

**Gathered:** 2026-04-10
**Status:** Ready for planning

<domain>
## Phase Boundary

Users can re-apply scripts without losing constraint participant intent or visual metadata, and replay remains stable and deterministic across repeated runs.

This phase is limited to script re-apply integrity (`SCRI-01`, `SCRI-02`, `SCRI-03`). It does not add new solver capability families or broader constraint authoring UX beyond preserving existing valid scripted intent.

</domain>

<decisions>
## Implementation Decisions

### Participant descriptor contract (roles/sub-index)
- **D-01:** Script re-apply uses a strict descriptor contract: participant role/sub-index intent must be preserved for script-managed constraints.
- **D-02:** Missing/ambiguous descriptor intent is an explicit apply failure, not a silent downgrade to entity-only participants.
- **D-03:** Legacy entity-only participant fallback is not allowed for this phase.

### Color preservation semantics
- **D-04:** Script color is canonical metadata for script-managed entities and must round-trip exactly on re-apply.
- **D-05:** Color preservation applies to script-managed points, lines, and arcs.
- **D-06:** Non-script entities remain untouched by script re-apply color restoration.

### Reapply/remap transaction behavior
- **D-07:** Re-apply remains all-or-nothing: unresolved script id, illegal participant signature, or metadata mismatch triggers full rollback.
- **D-08:** No partial scene mutations are allowed on failure.
- **D-09:** Failures must use deterministic, explicit error taxonomy/messages suitable for debugging and regression assertions.

### Determinism verification contract
- **D-10:** Phase verification must include repeated apply in a single process and parity checks across fresh process reruns.
- **D-11:** Determinism parity checks must include emitted script stability and key solver diagnostics stability for identical inputs.
- **D-12:** Weak "no crash/no error only" verification is not sufficient for this phase.

### the agent's Discretion
- Exact internal representation for descriptor persistence in script parse/apply/emit pipeline, as long as D-01..D-03 behavior is enforced.
- Exact deterministic failure code/message structuring, provided family-level diagnostics remain explicit and stable.
- Exact test fixture composition and helper wiring, provided D-10..D-12 parity guarantees are covered.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Phase and milestone contracts
- `.planning/ROADMAP.md` — Phase 31 goal, requirement mapping (`SCRI-01..03`), and success criteria.
- `.planning/REQUIREMENTS.md` — Script re-apply integrity requirement definitions and traceability.
- `.planning/PROJECT.md` — v1.5 milestone framing and robustness/stability constraints.
- `.planning/STATE.md` — active continuity state for v1.5 and current phase handoff.

### User scenario and intended behavior
- `docs/improvements/solver-user-workflow-robustness.md` — user-reported script re-apply regressions (unsupported participant failure and color reset) that this phase must eliminate.

### Upstream script and transactional contracts
- `.planning/phases/13-script-round-trip-baseline/13-CONTEXT.md` — script-local ID contract, deterministic emit ordering, two-pass reconstruction.
- `.planning/phases/14-script-io-api-undo-integration/14-CONTEXT.md` — strict transactional apply/rollback and explicit diagnostics posture.

### Code anchors for implementation
- `src/scripting/sketch_script_apply.h` — parse/validate/apply transaction flow, participant construction, rollback behavior, label restoration.
- `src/scripting/sketch_script_parse.h` — current script constraint participant schema and parse boundaries.
- `src/scripting/sketch_script_emit.h` — deterministic emitter behavior and current participant/color emission contract.
- `src/ecs/ecs_scene.h` — scene-level script apply/preview/reemit facade and undo transaction integration.
- `src/components/script_identity_comp.h` — script-local identity persistence boundary.
- `src/tests/script_roundtrip_tests.c` — existing roundtrip regression surface to extend for SCRI coverage.
- `src/tests/scene_solver_diagnostics_test.c` — diagnostic determinism assertions relevant to repeated re-apply parity.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `sketch_script_apply_commit_model(...)` in `src/scripting/sketch_script_apply.h`: existing transactional apply skeleton with rollback path.
- `scene_script_apply_commit(...)` in `src/ecs/ecs_scene.h`: scene façade that already captures pre-state and integrates undo transaction boundaries.
- `scene_script_emit_for_sketch(...)` in `src/scripting/sketch_script_emit.h`: deterministic emit baseline for parity assertions.
- `ScriptIdentityComp` in `src/components/script_identity_comp.h`: canonical script-local identity anchor for remap.

### Established Patterns
- Scene-level façade owns mutation policy; UI remains a thin caller.
- Failure handling favors explicit diagnostics and deterministic behavior over permissive fallbacks.
- Script apply is expected to be transactional from earlier phases (rollback on failed apply path).
- Deterministic ordering/formatting is already treated as a first-class scripting contract.

### Integration Points
- Extend parse model + emitter + apply model so participant descriptors carry role/sub-index intent through roundtrip.
- Replace hardcoded default-white geometry creation in script apply path with script-provided color restoration for script-managed entities.
- Harden apply failure taxonomy and ensure rollback leaves no partial children/constraint artifacts.
- Add deterministic repeated-apply tests (same process + fresh rerun parity) over script output and diagnostics.

</code_context>

<specifics>
## Specific Ideas

- Treat descriptor fidelity as mandatory correctness, not compatibility best-effort.
- Treat script color fields as canonical for script-managed geometry during re-apply.
- Keep this phase stabilization-focused; no expansion into new constraint capability families.

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope.

</deferred>

---

*Phase: 31-script-reapply-fidelity-foundation*
*Context gathered: 2026-04-10*
