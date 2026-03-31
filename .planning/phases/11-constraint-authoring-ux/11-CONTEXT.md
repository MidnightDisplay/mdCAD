# Phase 11: Constraint Authoring UX - Context

**Gathered:** 2026-03-31
**Status:** Ready for planning

<domain>
## Phase Boundary

Deliver constraint authoring UX: users can apply the full v1.2 constraint set to legally-typed sketch geometry via the C-key in-context menu, inspect and manage constraints through a ConstraintManager inspector section, interact with constant-screen-size viewport glyphs (hover/select), create and edit LENGTH/ANGLE dimensions via glyph double-click, and see participating geometry highlighted on constraint selection.

This phase does **not** implement solver execution (Phase 12), scripting (Phase 13+), or multi-sketch cross-references (deferred).

</domain>

<decisions>
## Implementation Decisions

### Constraint menu trigger
- **D-01:** Constraint menu trigger key = `C` (not `Tab`). Tab remains unchanged as gizmo mode toggle (no conflict). The `C` key shows the in-context constraint menu only when a sketch entity or sketch geometry is selected.

### Constraint menu behavior
- **D-06:** Constraint menu appears at the mouse cursor position when `C` is pressed. Lists only applicable constraints for the current selection (filtered by geometry types in selection). Applies one constraint and auto-hides immediately.

### Constraint data model
- **D-02:** Each constraint is its own ECS entity parented to the sketch (not to any individual geometry entity). Two components are introduced:
  - `ConstraintComp` on each constraint entity: stores constraint type, value (for dimensional types), and the list of participating geometry entity IDs.
  - `ConstraintParticipantComp` on each geometry entity: stores back-reference list of constraint entity IDs that apply to this geometry.
  - Constraints and geometry entities are siblings under the sketch (both ECS-parented to the sketch entity).

### Auto-naming convention
- **D-04 (naming):** All sketch geometry, sketch constraints, and sketch entities themselves have `LabelComp` pre-filled at creation time:
  - Sketch: `Sketch_N` where N is a per-scene sequential counter.
  - Geometry: `[type]_N` (e.g., `Point_1`, `Line_2`) where N is per-sketch sequential per geometry type.
  - Constraints: `[type]_N` (e.g., `Coincident_1`) where N is per-sketch sequential per constraint type.
  - Description for geometry/constraints: pre-filled with the parent sketch name.
  - All labels editable. ECS entity IDs are the true unique identifiers.
  - Phase 11 should update `scene_add_*_to_sketch` helpers to pre-fill LabelComp per this convention.

### Glyph rendering
- **D-03:** All constraint glyphs use the pick buffer overlay system (same as gizmo), with a dedicated reserved pick ID range for constraints.
  - Symbol glyphs (COINCIDENT, PARALLEL, PERPENDICULAR, etc.): rendered flat to the screen (screen-space constant-size).
  - Dimension glyphs (LENGTH, ANGLE): 3D-rotatable at constant screen size, gizmo-style -- anchored to geometry in 3D space.

### ConstraintManager inspector section
- **D-04:** ConstraintManager is an inspector section after GeometryManager (order: Label -> Transform -> SketchManager -> GeometryManager -> ConstraintManager).
  - Flat list of constraints (constraint type + value for dimensional + participant geometry names).
  - Filter: dropdown by constraint type + substring search matching name/description.
  - Select a constraint row -> highlights all participating geometry entities.
  - Phase 11 action: Delete only.

### LENGTH/ANGLE editing workflow
- **D-05:** Double-clicking a LENGTH or ANGLE glyph opens a floating ImGui popup anchored near the glyph.
  - Editable InputFloat with current value.
  - Accept/Cancel buttons. Dismisses on Accept, Cancel, or Escape.
  - Value mirrored bidirectionally with ConstraintManager row when constraint is selected.

### the agent Discretion
- Exact glyph geometry shapes per constraint type.
- Exact pick ID range for constraints (must not overlap gizmo or entity ranges).
- Per-sketch label sequence storage approach.
- ConstraintManager row layout details.
- Whether ConstraintParticipantComp is separate or merged into a shared geometry metadata component.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Milestone and phase contract
- `.planning/ROADMAP.md` -- Phase 11 goal, dependencies, and success criteria.
- `.planning/REQUIREMENTS.md` -- SKCH-04, CONS-01..05 requirement contract.
- `.planning/PROJECT.md` -- v1.2 milestone intent and platform priorities.
- `.planning/STATE.md` -- current milestone continuity.

### Proposal-level behavior requirements
- `docs/feature-proposal/Sketches, Constraints, Scripting.md` -- canonical feature intent for constraints, glyphs, ConstraintManager, and dimension editing.

### Phase 10 foundations (directly depended upon)
- `.planning/phases/10-sketch-foundations-managers/10-CONTEXT.md` -- locked Phase 10 decisions.
- `src/components/sketch_comp.h` -- SketchComp with constraint count placeholder.
- `src/components/sketch_geometry_state_comp.h` -- geometry fixed/loose state.
- `src/ecs/ecs_scene.h` -- sketch creation/attach helpers and ownership model.
- `src/ecs/ecs_world.h` -- component registration and entity lifecycle patterns.

### Existing UI and interaction architecture
- `src/ui/ui_entity_inspector.h` -- inspector section composition style.
- `src/app.c` -- key event routing (Tab=gizmo toggle unchanged; C is unused).
- `src/gizmo/gizmo.h` -- pick buffer overlay registration/render pattern.
- `src/gpu/pick_buffer.h` -- overlay rendering API and pick ID range boundaries.

### Supporting codebase guidance
- `.planning/codebase/CONVENTIONS.md` -- naming/style constraints.
- `.planning/codebase/ARCHITECTURE.md` -- subsystem boundaries.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `pick_buffer.h`: pick_buffer_add_overlay_line/point -- constant-screen-size overlay used by gizmo. Constraint glyphs reuse these.
- `gizmo.h`: Reserved pick ID range + overlay populate/hover pattern -- direct template.
- `ui_entity_inspector.h`: GeometryManager flat-list/filter/selection pattern -- ConstraintManager mirrors this.
- `ecs_world.h`: ecs_world_alloc_pick_id, component registration -- same pattern for new components.
- `ecs_scene.h`: scene_add_*_to_sketch helpers -- Phase 11 updates these for LabelComp pre-fill.

### Established Patterns
- Header-only static inline module style, snake_case naming.
- Entity lifecycle: create -> register selectable with pick_id -> parent to sketch via EcsChildOf.
- Inspector sections as igCollapsingHeader_TreeNodeFlags blocks in ui_entity_inspector_draw_single.
- Tab key: globally bound to gizmo mode toggle -- must NOT be remapped.

### Integration Points
- Add ConstraintComp and ConstraintParticipantComp in src/components/ and register in ecs_world_init.
- Add scene_add_constraint and constraint scene helpers in src/ecs/ecs_scene.h.
- Extend src/ui/ui_entity_inspector.h with ConstraintManager section.
- Add constraint glyph populate/handle functions (analogous to gizmo_populate_pick_buffer/gizmo_handle_hover).
- Add C key handler in src/app.c event routing.
- Add dimension popup rendering path in viewport frame logic.

</code_context>

<specifics>
## Specific Ideas

- Glyph symbol design: simple geometric shapes recognizable at ~12-16px. Reference CAD conventions (perpendicular=small square, parallel=two short lines, coincident=circle).
- Dimension glyphs: CAD-style leader lines with arrows + value label, rotatable in 3D.
- Auto-naming makes ConstraintManager list readable without manual renaming.
- Bidirectional constraint-geometry reference enables O(1) highlight queries.

</specifics>

<deferred>
## Deferred Ideas

- Solver execution backend (Phase 12).
- Auto-solve/solver diagnostics (Phase 12).
- Constraint-respecting gizmo transform (Phase 12).
- Script editor and sketch round-trip (Phase 13+).
- CONS-05 driven constraint field modeled in Phase 11 ConstraintComp but solver enforcement deferred to Phase 12.

</deferred>

---

*Phase: 11-constraint-authoring-ux*
*Context gathered: 2026-03-31*
