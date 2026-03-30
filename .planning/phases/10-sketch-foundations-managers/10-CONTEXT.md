# Phase 10: Sketch Foundations & Managers - Context

**Gathered:** 2026-03-30
**Status:** Ready for planning

<domain>
## Phase Boundary

Deliver sketch foundation workflows: create sketch containers, attach point/line/arc(circle) geometry, expose sketch-level status/count/color in Entity Inspector, and support GeometryManager single/multi-select fix/unfix/delete actions.

This phase does **not** add new constraint authoring behavior (that begins in Phase 11).

</domain>

<decisions>
## Implementation Decisions

### Sketch creation and geometry attachment flow
- **D-01:** Sketch creation entrypoint is `Add Entity` menu -> `Create Sketch` (aligned with current app menu-first creation flow).
- **D-02:** Core geometry attachment to active sketch is supported in **both** places: `Add Entity` menu and GeometryManager panel add controls.

### GeometryManager organization and actions
- **D-03:** GeometryManager uses a flat list (not grouped/tree) with geometry type + name + fixed/loose status visible per row.
- **D-04:** GeometryManager supports multiselect actions in Phase 10 for fix/unfix/delete.
- **D-05:** Multiselect action execution should refresh list state immediately and be recorded as one combined undo entry per action.

### Fixed-state semantics for Phase 10
- **D-06:** Phase 10 fixed-state behavior is visual/manager-state oriented now; strict transform/gizmo enforcement is deferred to solver/constrained interaction phases.
- **D-07:** Fixed geometry should still be clearly labeled as fixed in manager and sketch status surfaces, even before full solver enforcement lands.

### Sketch status and color behavior
- **D-08:** Reuse target status labels now (`solved`, `loose`, `fixed`, `error`) with placeholder/pre-solver logic in Phase 10.
- **D-09:** Implement full sketch color propagation/override semantics in Phase 10, including proposal rule: sketch color applies by default; geometry with explicit non-black RGB color overrides inherited sketch color.

### the agent's Discretion
- Exact placeholder status derivation rules before solver backend integration, as long as visible labels match final taxonomy and are internally consistent.
- Exact UI layout details inside Entity Inspector/GeometryManager (spacing, iconography, row adornments) while preserving existing panel style.
- Exact combined undo command implementation strategy for multiselect actions, as long as user-observed behavior is one action -> one undo step.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Milestone and phase contract
- `.planning/ROADMAP.md` — Phase 10 goal, dependencies, and success criteria.
- `.planning/REQUIREMENTS.md` — `SKCH-01`, `SKCH-02`, `SKCH-03` requirement contract and v1.2 scope boundaries.
- `.planning/PROJECT.md` — v1.2 milestone intent, constraints, and platform priorities.
- `.planning/STATE.md` — current milestone continuity and next-step routing.

### Proposal-level behavior requirements
- `docs/feature-proposal/Sketches, Constraints, Scripting.md` — canonical feature intent for sketch managers, color propagation, geometry scope, and future phase interactions.

### Existing runtime/UI architecture references
- `src/ui/ui_scene_hierarchy.h` — established Add Entity/menu workflows, import job UX patterns, and list interaction conventions.
- `src/ui/ui_entity_inspector.h` — existing inspector composition/edit patterns and undo capture style.
- `src/ecs/ecs_world.h` — ECS component registration/tag patterns and entity lifecycle helpers.
- `src/ecs/ecs_scene.h` — scene-level creation/update APIs and geometry ownership patterns.
- `src/components/geometry_comp.h` — geometry type model and current point/line/arc representations.
- `src/app.c` — keybinding/event routing baseline (`Tab` currently toggles gizmo mode; conflict awareness for later phases).

### Supporting codebase guidance
- `.planning/codebase/CONVENTIONS.md` — naming/style constraints for new modules/components.
- `.planning/codebase/ARCHITECTURE.md` — subsystem boundaries and frame/update integration points.
- `.planning/codebase/STRUCTURE.md` — file placement and module organization norms.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `ui_scene_hierarchy.h`: existing add-entity patterns, list caching, multiselect-like list operations, and status text surfaces that can inform Sketch/Geometry manager UX.
- `ui_entity_inspector.h`: established inspector section structure and undo capture patterns for editable fields.
- `ecs_world.h`: component/tag registration and entity helper utilities suitable for new sketch-related components.
- `ecs_scene.h` + `geometry_comp.h`: current point/line/arc(circles via arc model) geometry creation pathways to reuse for sketch-owned geometry.

### Established Patterns
- Header-only module style with `static inline` APIs and `snake_case` function naming.
- Explicit, narrow component responsibilities with helper factories/defaults.
- Undo recording on interaction end (`igIsItemDeactivatedAfterEdit`) and scene-hierarchy dirty-marking patterns for UI coherence.
- Menu-first entity creation UX in hierarchy panel, consistent with user-selected sketch creation path.

### Integration Points
- Add new sketch-related components in `src/components/` and register them in `ecs_world_init`.
- Extend hierarchy and/or inspector UI modules for SketchManager + GeometryManager display/action controls.
- Wire sketch creation and geometry attachment through scene/world helper APIs so future phases (constraints/solver/script) can build on same ownership model.
- Maintain compatibility with existing gizmo/selection/undo loops in `app.c` and avoid breaking current input routing before Phase 11/12.

</code_context>

<specifics>
## Specific Ideas

- Keep creation UX consistent with current app patterns: `Add Entity` menu remains a primary source of truth.
- Geometry attachment should be ergonomically accessible from both global menu and local manager panel.
- Status vocabulary should stabilize early by using final labels immediately, even when underlying solver logic is still placeholder-level.
- Multiselect operations should feel atomic to users (single action, single undo).

</specifics>

<deferred>
## Deferred Ideas

- Strict fixed-state enforcement in transforms/gizmo is deferred to solver/constrained interaction phases (Phase 12 scope).
- Constraint authoring menu/glyph workflows remain Phase 11 scope.
- Solver backend behavior and diagnostic depth remain Phase 12+ scope.
- Script editor/round-trip behavior remains Phase 13+ scope.

</deferred>

---

*Phase: 10-sketch-foundations-managers*
*Context gathered: 2026-03-30*
