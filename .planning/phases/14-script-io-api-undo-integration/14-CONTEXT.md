# Phase 14: Script IO + API/Undo Integration - Context

**Gathered:** 2026-04-01
**Status:** Ready for planning

<domain>
## Phase Boundary

Deliver safe script-side scene updates and script IO parameter interaction for the existing Phase 13 script workflow, while expanding scene-level API entrypoints and enforcing transactional undo/redo behavior for script-driven mutations.

This phase does **not** add new scripting language/runtime capabilities beyond the current Lua 5.4 declarative model, and does **not** include non-numeric IO widget types.

</domain>

<decisions>
## Implementation Decisions

### Script-side apply safety
- **D-01:** Phase 14 uses **strict transactional apply** semantics: any parse/validation/apply failure aborts the operation and restores the full pre-apply scene state.
- **D-02:** Safety rules apply consistently to both explicit `Apply Script` actions and script-IO-driven edits.
- **D-03:** Failure paths must surface explicit diagnostics; silent no-op failure behavior is not acceptable.

### Script IO schema (Phase 14 scope)
- **D-04:** IO schema for Phase 14 is **numeric-only** and split into `inputs` and `outputs`.
- **D-05:** Numeric IO supports optional `min/max/step` metadata; when present, UI must expose both numeric input and slider semantics.
- **D-06:** Non-numeric/multi-type IO (`int/bool/string` as separate widget families) is deferred out of Phase 14 scope.

### Script IO frontend behavior
- **D-07:** Script IO is shown in a **dedicated ImGui window** (not embedded in Script Editor), opened via a toggle button next to `Open Script Editor` in SketchManager.
- **D-08:** IO window shows editable inputs and read-only outputs with immediate bidirectional sync between UI values and script state.
- **D-09:** IO input edits auto-apply live, but each edit must run through the same transactional pipeline (validate -> atomic apply -> rollback on failure).

### Scene API integration shape
- **D-10:** Scene API exposure remains **scene-level façade first** (workflow-family entrypoints), with UI as a thin caller and no direct low-level ECS mutation flow from UI panels.
- **D-11:** Script workflow APIs, sketch/geometry manager APIs, and constraint manager APIs should align behind coherent scene-level contracts to satisfy `API-01`.

### Undo/redo transaction contract
- **D-12:** One successful `Apply Script` operation equals **one atomic undo entry**.
- **D-13:** Rolling back a script-driven mutation must restore full pre-apply state (no partial restores), including solver-impacting state and related sketch entities/constraints.
- **D-14:** IO auto-apply edits use the same atomicity guarantees as manual apply and must not poison subsequent valid edits.

### the agent's Discretion
- Exact C struct layout for numeric IO declarations as long as `inputs/outputs` + optional `min/max/step` behavior is preserved.
- Exact dedicated IO window layout/styling details and docking behavior, while keeping current ImGui conventions.
- Exact internal undo payload representation for script transactions, provided user-observed behavior remains one atomic undo/redo unit per committed apply.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Milestone and requirement contract
- `.planning/ROADMAP.md` — Phase 14 goal, dependencies, and required truths (`SCRP-04`, `SCRP-05`, `API-01`, `API-02`).
- `.planning/REQUIREMENTS.md` — requirement definitions and traceability status for scripting + API/undo integration.
- `.planning/PROJECT.md` — v1.2 scope constraints, platform priorities, and guardrails.
- `.planning/STATE.md` — continuity and prior phase decisions that must be preserved.

### Product behavior specification
- `docs/feature-proposal/Sketches, Constraints, Scripting.md` — canonical intent for script IO variables, generated controls, and bidirectional script/frontend behavior.

### Upstream phase contracts to preserve
- `.planning/phases/13-script-round-trip-baseline/13-CONTEXT.md` — locked script model/runtime/editor decisions (`D-01..D-11`) that Phase 14 extends.
- `.planning/phases/13-script-round-trip-baseline/13-03-SUMMARY.md` — accepted Script Editor behavior and post-checkpoint hardening outcomes.
- `.planning/phases/12-solver-control-and-constrained-interaction/12-CONTEXT.md` — solver implication/diagnostic semantics that script-driven changes must not regress.
- `.planning/phases/11-constraint-authoring-ux/11-CONTEXT.md` — constraint legality and participant model contracts used by script apply/validation.
- `.planning/phases/10-sketch-foundations-managers/10-CONTEXT.md` — sketch ownership and manager interaction foundations.

### Implementation anchors in code
- `src/ecs/ecs_scene.h` — scene-level scripting façade (`scene_script_preview_parse`, `scene_script_apply_commit`, `scene_script_reemit_for_sketch`) and mutation integration points.
- `src/scripting/sketch_script_apply.h` — parse/validate/apply pipeline and current rollback/error behavior.
- `src/scripting/sketch_script_parse.h` — declarative parser rules and token/structure validation boundaries.
- `src/scripting/sketch_script_emit.h` — deterministic scene->script output contract and numeric formatting baseline.
- `src/app.c` — Script Editor window orchestration, preview/apply flow, and error-state UI behavior.
- `src/ui/ui_entity_inspector.h` — SketchManager controls and launch/toggle entrypoints for Script Editor (and new IO window trigger).
- `src/undo_redo.h` — undo command model and transaction boundaries.
- `src/undo_redo_exec.h` — undo snapshot/restore execution path used to guarantee atomic restores.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `scene_script_preview_parse(...)` and `scene_script_apply_commit(...)` in `src/ecs/ecs_scene.h` already provide a scene façade boundary suitable for extending safe apply semantics.
- `sketch_script_apply_commit_model(...)` in `src/scripting/sketch_script_apply.h` already snapshots/removes/rebuilds sketch children and is the direct place to harden atomic rollback guarantees.
- Script Editor lifecycle/state in `src/app.c` already supports preview diagnostics, apply failure persistence, and reset/discard flows to integrate IO-driven updates safely.
- `ui_entity_inspector_request_script_editor(...)` and SketchManager button wiring in `src/ui/ui_entity_inspector.h` provide the existing UI launch pattern for adding IO-window toggles.
- Undo/redo snapshot infrastructure in `src/undo_redo_exec.h` is available for transactional script apply integration.

### Established Patterns
- Header-only `static inline` subsystem style with scene-owned mutation authority.
- UI panels are thin callers into scene APIs; business logic is centralized in scene/scripting modules.
- Deterministic script emission and script-local ID contracts are already established in Phase 13 and should remain unchanged.
- Failure handling favors explicit status/diagnostics over hidden fallback behavior.

### Integration Points
- Extend scripting model/schema and parser for numeric `inputs/outputs` plus optional `min/max/step` metadata.
- Add scene-level façade methods for IO update/apply flows so both Script Editor and IO window use the same transactional pipeline.
- Add dedicated IO window state/draw lifecycle in `app.c`, with SketchManager toggle entrypoint in `ui_entity_inspector.h`.
- Wire script-driven transaction commits into undo/redo as one atomic command boundary.

</code_context>

<specifics>
## Specific Ideas

- Keep the IO panel discoverable from SketchManager via a dedicated toggle next to `Open Script Editor`.
- Preserve immediate feel (live input edits) while still treating each edit as an atomic safe apply.
- Keep outputs explicitly read-only in the IO panel to avoid ambiguity between derived values and editable parameters.

</specifics>

<deferred>
## Deferred Ideas

- Non-numeric/multi-type IO widgets (`bool`, `string`, richer type families) are deferred to a future phase.
- Broader scripting capability expansion beyond deterministic sketch-parametric workflows remains outside Phase 14.

</deferred>

---

*Phase: 14-script-io-api-undo-integration*
*Context gathered: 2026-04-01*
