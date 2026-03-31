# Phase 11: Constraint Authoring UX - Research

**Researched:** 2026-03-31  
**Domain:** Native C (Dear ImGui + Flecs ECS + Sokol pick-buffer) constraint authoring/inspection UX  
**Confidence:** HIGH

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
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

### Deferred Ideas (OUT OF SCOPE)
- Solver execution backend (Phase 12).
- Auto-solve/solver diagnostics (Phase 12).
- Constraint-respecting gizmo transform (Phase 12).
- Script editor and sketch round-trip (Phase 13+).
- CONS-05 driven constraint field modeled in Phase 11 ConstraintComp but solver enforcement deferred to Phase 12.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| SKCH-04 | User can select a constraint and see all participating geometry entities/sub-entities highlighted. | Use bidirectional links (`ConstraintComp.participants[]` + `ConstraintParticipantComp.constraints[]`) and route selection through existing ECS selected/hover tags and highlight path in `ecs_scene_update/render`. |
| CONS-01 | User can apply initial v1.2 constraint set to legal geometry combos. | Implement legality matrix + filtered apply menu; create constraint entities under sketch; maintain participant backrefs. |
| CONS-02 | User can open in-context menu showing applicable constraints and auto-hide after one apply. | Integrate key handling in `app.c` keyboard routing near existing Tab binding; popup anchored at mouse and one-shot apply. (Locked decision updates key to `C`.) |
| CONS-03 | User can hover/select constraints through constant-size viewport glyphs. | Reuse pick-buffer overlay API and gizmo overlay pattern (`pick_buffer_add_overlay_*`, `gizmo_populate_pick_buffer`) with reserved constraint pick range. |
| CONS-04 | User can create/view/edit LENGTH/ANGLE from viewport and manager with mirrored values. | Add double-click glyph popup + manager row value editors sharing same source of truth in `ConstraintComp.value`. |
| CONS-05 | User can mark LENGTH/ANGLE constraints as driven (visible, non-driving). | Add `driven` field to `ConstraintComp`, expose toggle in manager and popup, persist in data model now; solver semantics deferred to Phase 12. |
</phase_requirements>

## Project Constraints (from copilot-instructions.md)

`./copilot-instructions.md` was not found in repository root at research time, so no additional project-specific directives were extracted from that file.

## Summary

Phase 11 should be implemented as a strict extension of existing patterns already used in Phase 10 and gizmo picking, not as a new subsystem architecture. The codebase already has: (1) keyboard shortcut routing in `app.c`, (2) pick-buffer overlay infrastructure for constant-on-screen interactive handles, (3) ECS parent/child ownership for sketch-local entities, and (4) inspector section composition with modal/popups and selection/bulk actions. Constraint authoring UX fits directly into these existing seams.

The most important planning detail is consistency between contracts: roadmap/requirements still mention a Tab-triggered menu, but locked Phase 11 decisions explicitly changed this to `C` to avoid conflict with the existing Tab gizmo toggle in `app.c`. Planner tasks should treat `C` as canonical for implementation and ensure verification criteria align accordingly.

Primary implementation risk is state coherence rather than rendering: every create/delete/edit action must keep `ConstraintComp`, `ConstraintParticipantComp`, sketch metadata (`constraint_count`), UI list state, pick IDs/ranges, and undo/redo behavior synchronized. Plan tasks should explicitly enforce these invariants.

**Primary recommendation:** Implement constraints as first-class sketch-child ECS entities using existing pick-buffer + inspector patterns, and add explicit invariant-update helpers so all entry points maintain bidirectional links and counts safely.

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| Dear ImGui via cimgui | 1.92.5dock (repo stack doc) | Inspector UI, popups, in-context menu, value editing | Already used throughout editor UI; popup/double-click patterns already present in codebase |
| Flecs ECS | Vendored in repo (version not declared in read files) | Constraint/geometry entities + components + parenting | Existing ownership, selection tags, and component lifecycle are all Flecs-based |
| Sokol + project pick buffer | Repo-integrated | GPU picking for hover/select and overlay hit-testing | Existing gizmo/vertex-handle solution already solves constant-screen-size interactive overlays |
| Existing undo/redo command stack | In-repo custom (`undo_redo.h`) | Transactional user actions | All manager mutations already route through this system; needed for predictable constraint workflows |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| cglm | 0.9.6 (locked backend in STATE.md) | View/projection and world-space placement math | Glyph anchor transforms, dimension orientation, screen-size scaling |
| Existing UI manager patterns (`ui_entity_inspector.h`) | In-repo | Flat list/filter/row actions | ConstraintManager row/filter/delete mirrors GeometryManager structure |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Pick-buffer overlay glyph picking | CPU ray tests over procedural glyphs | More custom math/edge cases; duplicates existing proven GPU picking path |
| Constraint data as detached registry/map | Constraint ECS entities parented to sketch (chosen) | Registry is quicker initially but breaks scene ownership/select/delete patterns |

**Installation:** N/A (phase uses current vendored/native stack; no new npm packages identified)

## Architecture Patterns

### Recommended Project Structure
```text
src/
├── components/
│   ├── constraint_comp.h                # constraint type/value/driven/participants
│   └── constraint_participant_comp.h    # geometry -> constraint backrefs
├── ecs/
│   ├── ecs_world.h                      # register new components + getters/setters
│   └── ecs_scene.h                      # scene_add_constraint + helper invariants
├── ui/
│   └── ui_entity_inspector.h            # ConstraintManager section
├── constraints/
│   ├── constraint_types.h               # enum + names + legality checks
│   └── constraint_glyphs.h              # populate/hover/select + popup anchors
└── app.c                                # C-key menu trigger + event routing
```

### Pattern 1: Reuse overlay pick-buffer pipeline for glyph interactivity
**What:** Build constraint hover/select on existing overlay pick primitives.  
**When to use:** Any constant-screen-size glyph needing reliable viewport pick.  
**Example:**
```c
// Source: src/gpu/pick_buffer.h + src/gizmo/gizmo.h
pick_buffer_begin_frame(&state.pick_buffer);
ecs_scene_populate_pick_buffer(&state.ecs_scene, &state.pick_buffer, pick_mvp);
gizmo_populate_pick_buffer(&state.gizmo, &state.pick_buffer, &state.ecs_scene);
constraint_glyphs_populate_pick_buffer(&state.constraints, &state.pick_buffer, &state.ecs_scene);
pick_buffer_render(&state.pick_buffer, view, proj);
```

### Pattern 2: Sketch-local ownership through ECS parent/child
**What:** Constraint entities are siblings of geometry under sketch parent.  
**When to use:** Creating/deleting constraints and computing sketch metadata.  
**Example:**
```c
// Source: src/ecs/ecs_scene.h
scene_set_parent(scene, constraint_entity, sketch_entity);   // sibling of geometry
scene_refresh_sketch_metadata(scene, sketch_entity);         // include constraint_count update path
```

### Pattern 3: Inspector manager mirrors GeometryManager interaction model
**What:** Flat rows + filter + selection + destructive confirm popup.  
**When to use:** ConstraintManager row operations and delete action.  
**Example:**
```c
// Source: src/ui/ui_entity_inspector.h (GeometryManager baseline)
if (igCollapsingHeader_TreeNodeFlags("ConstraintManager", ImGuiTreeNodeFlags_DefaultOpen)) {
    // filter controls
    // selectable rows
    // delete confirmation popup
}
```

### Anti-Patterns to Avoid
- **Creating constraint state outside ECS:** breaks parent ownership, selection, serialization, and undo consistency.
- **Using Tab for constraint menu:** conflicts with existing locked Tab gizmo mode toggle in `app.c`.
- **One-way participant links only:** makes SKCH-04 highlighting expensive/error-prone after delete/update operations.
- **Ad-hoc pick IDs:** risks collisions with reserved gizmo range and undefined selection behavior.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Hover/select hit-testing for glyphs | Custom CPU ray intersection for each glyph type | Existing GPU pick-buffer overlay system | Already production path for gizmo/vertex handles; handles depth/top-layer behavior |
| Manager UI interaction framework | New bespoke list/table system | Existing GeometryManager inspector pattern | Consistent UX, less code, proven selection+modal workflows |
| Global ID naming service outside scene | Separate singleton naming registry | Per-sketch counters derived/stored in sketch-related state | Better locality and easier undo/serialization control |
| Transaction history for edits | New mini-undo stack for constraints | Existing `undo_redo` command framework | Avoid divergent undo semantics across editor features |

**Key insight:** Most complexity is already solved in the current architecture; custom parallel systems would increase integration risk and verification cost.

## Common Pitfalls

### Pitfall 1: Requirement text drift vs locked decisions
**What goes wrong:** Team implements Tab menu because REQUIREMENTS/ROADMAP text says Tab.  
**Why it happens:** Decision update (C-key) exists only in Phase 11 context.  
**How to avoid:** Treat `11-CONTEXT.md` locked decisions as implementation authority and add verification step proving Tab behavior remains gizmo toggle.  
**Warning signs:** C does nothing; Tab opens constraint menu; gizmo mode switching regresses.

### Pitfall 2: Broken bidirectional links after delete/undo
**What goes wrong:** Geometry remains referencing deleted constraints (or vice versa).  
**Why it happens:** Mutations update only one component (`ConstraintComp` or `ConstraintParticipantComp`).  
**How to avoid:** Centralize add/remove helpers that always update both sides + sketch count + undo record.  
**Warning signs:** highlight selects dead entities, stale rows, crashes during iteration.

### Pitfall 3: Pick ID collisions with gizmo reserved range
**What goes wrong:** Hover/select routes to wrong subsystem.  
**Why it happens:** Constraint glyph pick IDs overlap `GIZMO_PICK_RESERVED_START` region.  
**How to avoid:** Reserve explicit non-overlapping constraint range and route hover handling by range check before entity lookup.  
**Warning signs:** clicking glyph manipulates gizmo or selects unrelated entity.

### Pitfall 4: ImGui input flag misuse for numeric editors
**What goes wrong:** LENGTH/ANGLE editing behaves inconsistently.  
**Why it happens:** Applying text-input-only flags to scalar editors (known codebase gotcha).  
**How to avoid:** Keep `InputFloat` usage clean; use explicit Accept/Cancel buttons and Escape handling.  
**Warning signs:** Enter key handling glitches or ignored edits in dimension popup.

## Code Examples

Verified patterns from repository sources:

### Keyboard shortcut routing without conflicting Tab behavior
```c
// Source: src/app.c
if (!io->WantCaptureKeyboard) {
    if (igIsKeyPressed_Bool(ImGuiKey_Tab, false)) {
        gizmo_edit_mode_t new_mode = (state.gizmo.edit_mode == GIZMO_TRANSFORM_MODE)
            ? GIZMO_GEOMETRY_MODE : GIZMO_TRANSFORM_MODE;
        gizmo_set_edit_mode(&state.gizmo, new_mode, &state.ecs_scene, &state.selection);
    }
    // Add C-key constraint menu trigger here
}
```

### Overlay pick rendering path template
```c
// Source: src/gpu/pick_buffer.h + src/app.c
pick_buffer_begin_frame(&state.pick_buffer);
ecs_scene_populate_pick_buffer(&state.ecs_scene, &state.pick_buffer, pick_mvp);
gizmo_populate_pick_buffer(&state.gizmo, &state.pick_buffer, &state.ecs_scene);
// constraint_glyphs_populate_pick_buffer(...)
pick_buffer_render(&state.pick_buffer, view_legacy, proj_legacy);
pick_buffer_readback(&state.pick_buffer);
pick_buffer_update_hover(&state.pick_buffer);
```

### Manager confirmation popup pattern
```c
// Source: src/ui/ui_entity_inspector.h
igOpenPopup_Str("Delete Constraint##constraint_manager_delete_popup", 0);
if (igBeginPopupModal("Delete Constraint##constraint_manager_delete_popup", NULL,
                      ImGuiWindowFlags_AlwaysAutoResize)) {
    if (igButton("Delete##constraint_manager_confirm_delete", (ImVec2){120.0f, 0.0f})) {
        // delete + undo command + metadata refresh
    }
    igEndPopup();
}
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Tab-triggered constraint menu (requirements text) | C-triggered context menu (locked decision D-01) | 2026-03-31 discuss/context update | Prevents collision with Tab gizmo mode; implementation must follow C |
| Constraint count as placeholder only | Constraint entities + real per-sketch constraint count | Phase 11 implementation target | Enables trustworthy SketchManager status and manager workflows |
| Geometry-only manager operations | Dual manager model (GeometryManager + ConstraintManager) | Phase 11 target | Separates geometry edit workflow from constraint lifecycle |

**Deprecated/outdated:**
- Treating sketch constraints as future-only placeholder fields is outdated for Phase 11 scope.

## Open Questions

1. **Where should per-sketch naming counters live?**
   - What we know: naming convention is locked; storage method is discretionary.
   - What's unclear: whether counters are stored in `SketchComp`, derived by scan, or hybrid cached.
   - Recommendation: store counters in sketch-owned component state for O(1) creation, with optional rebuild helper for recovery/import.

2. **How to represent sub-entity references for line endpoints/arc center in participants?**
   - What we know: requirements mention geometry/sub-entity combinations.
   - What's unclear: participant schema beyond plain entity IDs.
   - Recommendation: include optional sub-index/role metadata in participant entries now to avoid remodel in Phase 12.

3. **Undo granularity for value edits (live typing vs Accept-only)**
   - What we know: popup has Accept/Cancel contract.
   - What's unclear: whether manager inline edit should commit per-keystroke.
   - Recommendation: commit on Accept/Enter/loss-confirm only (single undo step), not per character.

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| cmake | Building phase changes | ✓ | 4.3.0 | — |
| ctest | Validation architecture command surface | ✓ | 4.3.0 | Manual checklist if no tests wired |
| ninja | Optional fast builds | ✗ | — | Use Visual Studio/MSBuild generator path |
| python | Existing tooling scripts (optional) | ✓ | 3.13.12 | — |
| node/npm | Existing scripts (optional) | ✓ | v25.8.1 / 11.11.0 | — |

**Missing dependencies with no fallback:**
- None identified for Phase 11 implementation itself.

**Missing dependencies with fallback:**
- `ninja` missing; use existing Windows Visual Studio generator workflow.

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | CMake/CTest harness present; no dedicated Phase 11 automated UI test suite detected |
| Config file | CMakeLists.txt / src/CMakeLists.txt (no standalone test config discovered) |
| Quick run command | `cmake --build build-vulkan --config Release --target mdCAD` |
| Full suite command | `ctest --test-dir build-vulkan -C Release --output-on-failure` |

### Phase Requirements → Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| SKCH-04 | Constraint row/glyph selection highlights participants | manual UI smoke | `cmake --build build-vulkan --config Release --target mdCAD` | ❌ Wave 0 |
| CONS-01 | Applicable legal constraints can be applied | manual UI matrix smoke | `cmake --build build-vulkan --config Release --target mdCAD` | ❌ Wave 0 |
| CONS-02 | In-context menu appears at cursor, filtered, one-shot apply | manual UI interaction | `cmake --build build-vulkan --config Release --target mdCAD` | ❌ Wave 0 |
| CONS-03 | Hover/select via constant-size glyphs | manual viewport interaction | `cmake --build build-vulkan --config Release --target mdCAD` | ❌ Wave 0 |
| CONS-04 | LENGTH/ANGLE create/view/edit mirrored | manual popup + manager validation | `cmake --build build-vulkan --config Release --target mdCAD` | ❌ Wave 0 |
| CONS-05 | Driven toggle persists and surfaces in UI | manual state/UI validation | `cmake --build build-vulkan --config Release --target mdCAD` | ❌ Wave 0 |

### Sampling Rate
- **Per task commit:** `cmake --build build-vulkan --config Release --target mdCAD`
- **Per wave merge:** `ctest --test-dir build-vulkan -C Release --output-on-failure`
- **Phase gate:** Full suite green + explicit manual UX checklist for SKCH-04/CONS-01..05 before `/gsd-verify-work`

### Wave 0 Gaps
- [ ] `.planning/phases/11-constraint-authoring-ux/11-VALIDATION.md` checklist entries for SKCH-04, CONS-01..05 UI behaviors
- [ ] Automated smoke harness for input routing and pick ID range collision checks (currently absent)
- [ ] Regression checks for undo/redo around constraint create/delete/value edits (currently manual-only)

## Sources

### Primary (HIGH confidence)
- `.planning/phases/11-constraint-authoring-ux/11-CONTEXT.md` — locked decisions, scope boundaries, integration points
- `.planning/phases/11-constraint-authoring-ux/11-UI-SPEC.md` — UI behavior/copy/spacing/color contract
- `.planning/REQUIREMENTS.md` — SKCH-04 + CONS-01..05 requirement definitions
- `.planning/ROADMAP.md` — Phase 11 goal and success criteria
- `src/app.c` — keyboard routing, pick pass orchestration, Tab binding
- `src/gpu/pick_buffer.h` — overlay pick API and lifecycle
- `src/gizmo/gizmo.h` — overlay populate/hover patterns
- `src/ui/ui_entity_inspector.h` — manager section/list/filter/popup patterns
- `src/ecs/ecs_world.h` — component registration + pick ID allocator constraints
- `src/ecs/ecs_scene.h` — sketch ownership helpers and metadata refresh paths
- `src/components/selectable_comp.h` — reserved pick ID range definitions

### Secondary (MEDIUM confidence)
- `AGENTS.md` (project stack snapshot) — cimgui version and toolchain posture

### Tertiary (LOW confidence)
- None (no external web/doc claims were required for core phase planning)

## Metadata

**Confidence breakdown:**
- Standard stack: **HIGH** - directly derived from in-repo architecture and active modules
- Architecture: **HIGH** - concrete integration points verified in source files
- Pitfalls: **MEDIUM** - mainly inferred from current patterns/contracts and one documented gotcha

**Research date:** 2026-03-31  
**Valid until:** 2026-04-30 (stable internal architecture, recheck if core input/picking architecture changes)
