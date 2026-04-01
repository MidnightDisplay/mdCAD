# Phase 12: Solver Control & Constrained Interaction - Context

**Gathered:** 2026-04-01
**Status:** Ready for planning

<domain>
## Phase Boundary

Deliver solver control and constrained interaction behavior for sketch workflows already implemented in Phases 10, 11, and 16. This phase adds auto/manual solve control, solver status + diagnostics visibility, failure implication highlighting, and constraint-respecting gizmo manipulation behavior.

This phase does **not** add scripting features (Phases 13/14), validation packaging (Phase 15), or multi-backend solver selection UX.

</domain>

<decisions>
## Implementation Decisions

### Solve trigger policy
- **D-01:** New sketches default to **auto-solve ON** and also expose a manual **Recalculate** action.
- **D-02:** With auto-solve ON, solve runs after every sketch-affecting edit (geometry, constraints, dimensional values, driven toggles) with lightweight debounce.

### Diagnostics behavior
- **D-03:** Diagnostics are tracked per sketch as a rolling history log.
- **D-04:** The per-sketch diagnostics log keeps the latest **100** entries.
- **D-05:** Diagnostics history is only cleared via explicit user action, not automatically on each new solve.

### Failure implication visibility
- **D-06:** On solve failure/invalid state, highlight both implicated constraints and their participant geometries.
- **D-07:** On failure, auto-select the first implicated constraint as the primary focus target.
- **D-08:** Failure implication highlighting persists until the next successful solve.

### Gizmo behavior under constraints
- **D-09:** During drag, apply constrained projection in real time where feasible.
- **D-10:** If movement is unsatisfiable under active constraints, block movement instead of allowing temporary invalid placement.
- **D-11:** Unsatisfiable movement surfaces immediate UX feedback through viewport toast + diagnostics log entry + implication highlight.

### the agent's Discretion
- Exact debounce interval and solve queue/coalescing implementation strategy.
- Exact toast wording and on-screen placement, following existing viewport overlay style.
- Exact data structure for diagnostics entries, provided it supports timestamp + level + implicated entities.
- Internal solver-state representation details as long as UI contract remains consistent with `SOLV-01..04`.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Milestone and requirement contract
- `.planning/ROADMAP.md` - Phase 12 goal, success criteria, dependencies, and milestone sequencing.
- `.planning/REQUIREMENTS.md` - `SOLV-01`, `SOLV-02`, `SOLV-03`, `SOLV-04`, `API-03` acceptance contract.
- `.planning/PROJECT.md` - v1.2 scope guardrails, platform priorities, and non-negotiables.
- `.planning/STATE.md` - latest continuity and milestone status context.

### Upstream phase decisions that must be preserved
- `.planning/phases/10-sketch-foundations-managers/10-CONTEXT.md` - SketchManager/GeometryManager foundations and sketch status baseline assumptions.
- `.planning/phases/11-constraint-authoring-ux/11-CONTEXT.md` - constraint menu/glyph/manager interaction contracts and keybinding conventions.
- `.planning/phases/16-constraint-ux-closure-and-verification/16-VERIFICATION.md` - verified participant-highlighting parity that Phase 12 must not regress.

### Product intent reference
- `docs/feature-proposal/Sketches, Constraints, Scripting.md` - canonical solver control and constrained manipulation intent for v1.2.

### Implementation anchors in current code
- `src/components/sketch_comp.h` - sketch status model (`solved`, `loose`, `fixed`, `error`) and sketch metadata fields.
- `src/ecs/ecs_scene.h` - sketch metadata refresh and placeholder status derivation currently used by SketchManager.
- `src/ui/ui_entity_inspector.h` - existing SketchManager/GeometryManager/ConstraintManager UI surfaces to extend with solver controls and diagnostics.
- `src/app.c` - frame/event flow, key routing, glyph selection, and interaction update loop where constrained gizmo behavior integrates.
- `src/gizmo/gizmo.h` - transform/geometry edit mode mechanics and drag lifecycle.
- `src/undo_redo_exec.h` - atomic undo/redo behavior for geometry/constraint coupled mutations that solver-impacting edits must preserve.
- `src/constraints/constraint_selection.h` - shared participant selection helper used for constraint implication highlighting.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `scene_refresh_sketch_metadata(...)` and `scene_derive_sketch_status(...)` in `src/ecs/ecs_scene.h` provide the current sketch status update path and are natural extension points for solver-driven statuses.
- `ui_entity_inspector.h` already hosts sketch-local control surfaces (SketchManager, GeometryManager, ConstraintManager), making it the right place for solver toggles, recalc action, backend display, and diagnostics panel.
- `constraint_selection_apply_participants(...)` in `src/constraints/constraint_selection.h` is reusable for implication-highlighting consistency on solver failures.
- Existing viewport popup/overlay behavior in `src/app.c` (constraint menu and dimension popup) gives patterns for non-modal solver toasts/feedback.

### Established Patterns
- Header-only module style with `static inline` functions and explicit data flow.
- Scene mutation paths route through `ecs_scene.h` helpers and preserve undo recording at interaction boundaries.
- Selection/highlight semantics already converged between viewport glyph and manager interactions (Phase 16); solver implication highlighting should reuse this shared path.
- Gizmo drag lifecycle is centralized in `app.c` + `gizmo.h`; constrained interaction should hook this single path instead of introducing a parallel manipulation system.

### Integration Points
- Extend `SketchComp` and/or adjacent sketch-related components with auto-solve toggle, solver backend id/name, and diagnostics/log state.
- Add scene-level solver entrypoints (auto-trigger + manual recalc + implication extraction) in `src/ecs/ecs_scene.h` to keep UI and interaction layers thin.
- Add solver controls/diagnostics rendering in SketchManager section of `src/ui/ui_entity_inspector.h`.
- Integrate constrained projection + unsatisfiable-block flow in gizmo drag update path within `src/app.c`, backed by solver feedback calls.
- Route failure implication outputs through shared constraint/geometry selection/highlight helpers to preserve established UX parity.

</code_context>

<specifics>
## Specific Ideas

- Solver controls should feel local to each sketch and remain visible where sketch status/counts already live.
- Failure feedback should be immediate and non-modal; users should not lose interaction flow due to blocking dialogs.
- Diagnostics should remain useful for iterative edits, so short-lived transient messages are insufficient without rolling history.

</specifics>

<deferred>
## Deferred Ideas

- Multiple selectable solver backends and backend-switch UX (explicitly out of v1.2 scope).
- Cross-sketch/global solve graph behavior.
- Script-driven solver control or diagnostics export formats (Phase 13/14+ concerns).

</deferred>

---

*Phase: 12-solver-control-and-constrained-interaction*
*Context gathered: 2026-04-01*
