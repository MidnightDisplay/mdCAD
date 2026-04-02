# Phase 15: Validation & Acceptance Closure - Context

**Gathered:** 2026-04-02
**Status:** Ready for planning

<domain>
## Phase Boundary

Package v1.2 acceptance as verifiable evidence: ship example sketch/script case studies and close Windows MSVC + Vulkan acceptance gates with explicit recorded proof.

This phase is evidence-and-closure oriented, not feature-expansion work.

</domain>

<decisions>
## Implementation Decisions

### Validation artifact contract
- **D-01:** Phase closure evidence bundle must include `15-VALIDATION.md`, `15-03-SUMMARY.md`, and an updated `CHECKPOINT.md`.
- **D-02:** Validation rows must reference concrete command outputs and artifact locations; acceptance claims without traceable evidence are not acceptable.

### Windows acceptance gate strictness
- **D-03:** Windows acceptance requires a successful Release build of `mdCAD` in `build-vulkan`.
- **D-04:** Windows acceptance also requires a passing full test gate: `ctest --test-dir build-vulkan -C Release --output-on-failure`.
- **D-05:** Phase acceptance is blocked unless both the build and full CTest gate pass.

### Case-study deliverables (`VAL-01`)
- **D-06:** Evidence must include **two representative sketches** (constraint-focused) with script artifacts suitable for development/debugging.
- **D-07:** Evidence must include **one Script IO scenario** covering parse/apply/reset behavior and diagnostics visibility.
- **D-08:** Case studies should be selected to exercise realistic constraint workflows rather than synthetic minimal-only examples.

### macOS parity handling (`VAL-03`)
- **D-09:** macOS parity execution is intentionally **out of scope for Phase 15** by user decision in discuss-phase.
- **D-10:** `VAL-03` evidence is deferred; downstream planning should record this explicitly instead of silently treating it as complete.

### the agent's Discretion
- Exact sketch/sample naming and where example artifacts live, as long as they are easy to run and reference in validation docs.
- Exact structure of evidence subsections in `15-VALIDATION.md` and summary narrative in `15-03-SUMMARY.md`.
- Exact command ordering for evidence capture, provided the required Windows gates are clearly proven.

</decisions>

<specifics>
## Specific Ideas

- Discuss-phase covered all major gray areas in sequence: evidence format, Windows gate strictness, macOS scope, then case-study depth.
- Preferred closure style is direct and auditable: explicit commands, explicit outputs, explicit pass/fail statements.

</specifics>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Milestone and requirement contract
- `.planning/ROADMAP.md` — Phase 15 goal/success criteria and dependency on Phase 14.
- `.planning/REQUIREMENTS.md` — `VAL-01`, `VAL-02`, `VAL-03` definitions and current traceability status.
- `.planning/STATE.md` — current milestone position, active focus, and Windows-first development gate posture.
- `.planning/PROJECT.md` — milestone constraints and platform priorities.

### Product intent and acceptance framing
- `docs/feature-proposal/Sketches, Constraints, Scripting.md` — script IO behavior intent, case-study motivation, and platform direction notes.
- `CHECKPOINT.md` — continuity artifact that must be updated as part of the closure bundle.

### Upstream phase evidence and behavior contracts
- `.planning/phases/14-script-io-api-undo-integration/14-03-SUMMARY.md` — shipped Script IO behavior, UAT stabilization outcomes, and prior Windows evidence style.
- `.planning/phases/14-script-io-api-undo-integration/14-VALIDATION.md` — validation evidence format precedent and gate reporting style.
- `.planning/phases/14-script-io-api-undo-integration/14-CONTEXT.md` — locked Script IO/API/undo decisions that validation must preserve.

### Implementation anchors in code
- `src/app.c` — Script Editor + Script IO UI behavior and diagnostics surfaces used by case studies.
- `src/ecs/ecs_scene.h` — scene-level scripting façade (`scene_script_preview_parse`, `scene_script_apply_commit`, `scene_script_io_*`) used in validation scenarios.
- `src/tests/script_roundtrip_tests.c` — regression and round-trip coverage relevant to Script IO parse/apply/reset expectations.
- `docs/QUICKSTART.md` — canonical Windows Vulkan build invocation patterns.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- Existing Script IO and Script Editor runtime behavior in `src/app.c` can be used directly for acceptance walkthroughs.
- `scene_script_io_*` and script apply/preview APIs in `src/ecs/ecs_scene.h` define the transactional behavior that case studies should validate.
- `script_roundtrip_tests` already covers critical parser/apply regressions and provides a baseline for expected scripted behavior.

### Established Patterns
- Acceptance claims are expected to be backed by explicit command evidence in planning artifacts.
- Script mutation behavior follows transactional apply semantics with diagnostic surfacing (no silent partial state commits).
- Windows MSVC + Vulkan is the active development gate for this milestone.

### Integration Points
- Phase 15 planning/execution must produce and wire evidence into `15-VALIDATION.md` and `15-03-SUMMARY.md`.
- Closure must update `CHECKPOINT.md` with concise outcomes and gate status.
- Case-study artifacts and run steps must be referenced clearly from validation docs.

</code_context>

<deferred>
## Deferred Ideas

- macOS parity execution evidence (`VAL-03`) is deferred out of Phase 15 scope per user decision and should be scheduled in a follow-up closure activity.
- Any expansion of case studies beyond the required two sketches + one Script IO scenario is optional and out of required scope.

</deferred>

---

*Phase: 15-validation-and-acceptance-closure*
*Context gathered: 2026-04-02*
