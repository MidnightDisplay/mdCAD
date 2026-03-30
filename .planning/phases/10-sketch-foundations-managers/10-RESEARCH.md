# Phase 10: Sketch Foundations & Managers - Research

**Researched:** 2026-03-30  
**Domain:** ECS sketch foundations, manager UX, and undo-safe geometry workflows in mdCAD  
**Confidence:** HIGH

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- **D-01:** Sketch creation entrypoint is `Add Entity` menu -> `Create Sketch` (aligned with current app menu-first creation flow).
- **D-02:** Core geometry attachment to active sketch is supported in **both** places: `Add Entity` menu and GeometryManager panel add controls.
- **D-03:** GeometryManager uses a flat list (not grouped/tree) with geometry type + name + fixed/loose status visible per row.
- **D-04:** GeometryManager supports multiselect actions in Phase 10 for fix/unfix/delete.
- **D-05:** Multiselect action execution should refresh list state immediately and be recorded as one combined undo entry per action.
- **D-06:** Phase 10 fixed-state behavior is visual/manager-state oriented now; strict transform/gizmo enforcement is deferred to solver/constrained interaction phases.
- **D-07:** Fixed geometry should still be clearly labeled as fixed in manager and sketch status surfaces, even before full solver enforcement lands.
- **D-08:** Reuse target status labels now (`solved`, `loose`, `fixed`, `error`) with placeholder/pre-solver logic in Phase 10.
- **D-09:** Implement full sketch color propagation/override semantics in Phase 10, including proposal rule: sketch color applies by default; geometry with explicit non-black RGB color overrides inherited sketch color.

### the agent's Discretion
- Exact placeholder status derivation rules before solver backend integration, as long as visible labels match final taxonomy and are internally consistent.
- Exact UI layout details inside Entity Inspector/GeometryManager (spacing, iconography, row adornments) while preserving existing panel style.
- Exact combined undo command implementation strategy for multiselect actions, as long as user-observed behavior is one action -> one undo step.

### Deferred Ideas (OUT OF SCOPE)
- Strict fixed-state enforcement in transforms/gizmo is deferred to solver/constrained interaction phases (Phase 12 scope).
- Constraint authoring menu/glyph workflows remain Phase 11 scope.
- Solver backend behavior and diagnostic depth remain Phase 12+ scope.
- Script editor/round-trip behavior remains Phase 13+ scope.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| SKCH-01 | User can create a sketch entity and attach point, line, and arc/circle geometry to that sketch. | Add `SketchComp` ownership model + `scene_add_sketch_*` helpers; expose creation in Add Entity + GeometryManager add controls; reuse existing `scene_add_point/line/arc` and parent-child APIs. |
| SKCH-02 | User can view per-sketch solve status, color policy, geometry count, and constraint count in the Entity Inspector. | Extend inspector single-entity path with sketch sections (`SketchManager`, `GeometryManager summary`), placeholder status taxonomy (`solved/loose/fixed/error`), inherited color policy, and count derivation from sketch-owned children + future constraint count field. |
| SKCH-03 | User can fix, unfix, and delete sketch geometries from GeometryManager using single-select and multi-select workflows. | Implement flat GeometryManager list with internal selection buffer and bulk commands (`fix/unfix/delete`) using single action -> single undo entry behavior; integrate with existing `selection.h` patterns and undo stack extensions. |
</phase_requirements>

## Summary

Phase 10 should be planned as an incremental extension of existing mdCAD patterns, not a subsystem rewrite. The current architecture already has nearly all structural primitives needed: ECS entities and parent-child ownership (`EcsChildOf`), scene-level creation APIs (`scene_add_point`, `scene_add_line`, `scene_add_arc`), panel-based ImGui UX (`ui_scene_hierarchy.h`, `ui_entity_inspector.h`), and undo infrastructure with single and bulk commands (`undo_redo_exec.h`).  

The key planning decision is to introduce **minimal new sketch-specific components** while reusing existing geometry components and scene ownership. A sketch should be a normal ECS entity (with label/transform/renderable/selectable) that owns geometry children by parent link. Sketch metadata (status, color policy, manager-state fields) should live in dedicated components under `src/components/`, registered in `ecs_world_init()`, and surfaced in Entity Inspector. This keeps Phase 10 compatible with later phases (constraints/solver/scripting) and avoids dead-end scaffolding.

The highest-risk integration points are undo atomicity for multi-select actions, color inheritance semantics, and input conflicts around `Tab` (currently gizmo mode toggle in `app.c`). Phase 10 should **not** bind new `Tab` behavior. Constraint menu hotkey behavior remains Phase 11.

**Primary recommendation:** Implement sketch as ECS-owned container + manager metadata components, wire UI into existing inspector/hierarchy patterns, and add explicit bulk undo commands for fix/unfix/delete to guarantee one action = one undo step.

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| Flecs | 4.1.4 (vendored; `FLECS_VERSION_MAJOR 4`, `MINOR 1`, `PATCH 4`) | ECS entities, components, parent-child ownership | Already core scene model; sketch ownership cleanly maps to `EcsChildOf` and existing helper APIs. |
| cimgui / Dear ImGui | `1.92.5dock` tag (vendored via FetchContent) | Entity Inspector + manager panel UI | Existing UI layer is fully cimgui-based; extends naturally with SketchManager/GeometryManager sections. |
| Sokol | `master` (floating FetchContent tag) | Rendering/app loop/platform abstraction | Existing runtime entry and render loop rely on sokol; no reason to replace for this phase. |
| cglm | 0.9.6 (locked in project state) | Math conventions and transform helpers | Already locked baseline for runtime math and kept via `src/math/cglm_entry.h`. |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| mdCAD undo/redo (`undo_redo.h`, `undo_redo_exec.h`) | in-repo | command capture and apply/unapply | All sketch mutations (create, attach, fix/unfix/delete) must be undoable. |
| mdCAD selection (`selection.h`) | in-repo | single/multi-select behavior | GeometryManager list selection behavior should mirror existing ctrl/shift patterns. |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Parent-child ownership for sketch geometries | Separate adjacency map/table | More bookkeeping and easier to desync from ECS lifecycle; parent-child is already implemented and battle-tested in codebase. |
| Inspector-integrated manager sections | New standalone window first | Slower path to SKCH-02 and inconsistent with canonical proposal/UI pattern for Phase 10. |

**Installation/build baseline (existing project):**
```powershell
cmake -B build-vulkan -G "Visual Studio 18" -DUSE_VULKAN=ON
cmake --build build-vulkan --config Release --target mdCAD
```

## Architecture Patterns

### Recommended Project Structure
```text
src/
├── components/
│   ├── sketch_comp.h              # Sketch status/color-policy/manager metadata
│   └── sketch_geometry_state_comp.h # Fixed/loose flags per geometry entity
├── ecs/
│   └── ecs_scene.h                # sketch creation/attach helpers
└── ui/
    ├── ui_scene_hierarchy.h       # Add Entity -> Create Sketch + add geometry routing
    └── ui_entity_inspector.h      # SketchManager + GeometryManager surfaces
```

### Pattern 1: Sketch as ECS container with owned geometry children
**What:** Create sketch entity once; attach geometry entities by setting parent to sketch entity.  
**When to use:** Always for SKCH-01 ownership and count derivation.  
**Example:**
```c
// Source: src/ecs/ecs_scene.h
ecs_entity_t sketch = scene_add_anchor(scene, "Sketch", "");
ecs_entity_t g = scene_add_line(scene, a, b, color, width);
scene_set_parent(scene, g, sketch);
```

### Pattern 2: Undo capture at interaction end, bulk command for multi-select
**What:** Follow existing `igIsItemDeactivatedAfterEdit()` style for edits and `undo_cmd_bulk_*` for multi-target operations.  
**When to use:** Fix/unfix/delete actions from GeometryManager.  
**Example:**
```c
// Source: src/undo_redo_exec.h
undo_cmd_bulk_set_visible(ur, entities, old_visible, count, true);
```

### Pattern 3: Manager UI embedded in Entity Inspector
**What:** Add sketch sections only when selected entity has sketch component, preserving existing single/multi inspector flow.  
**When to use:** SKCH-02 and SKCH-03 surfaces.

### Anti-Patterns to Avoid
- **Do not introduce Tab hotkey in Phase 10:** `app.c` already uses Tab to toggle gizmo edit mode.
- **Do not hand-wire ad hoc arrays outside ECS ownership:** use parent-child queries (`scene_get_children`, `scene_count_children`).
- **Do not implement solver enforcement now:** fixed-state enforcement in transforms/gizmo is explicitly deferred.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Geometry ownership graph | Custom child registry | Existing ECS `EcsChildOf` + `scene_set_parent/get_children/count_children` | Prevents lifecycle drift and uses existing recursive delete behavior. |
| Multi-select semantics | New bespoke selection system | `selection.h` patterns (ctrl toggle, shift add, single replace) | Already consistent across hierarchy/inspector workflows. |
| Undo stack storage | Manual stack implementation in manager UI | `undo_redo_t` + new `undo_cmd_*` entries | Existing apply/unapply pipeline already integrated with scene mutations. |
| Color propagation rendering | Duplicate color state cache per frame | Derive effective color from sketch color + geometry override rule | Avoids stale cache and aligns with D-09 semantics. |

**Key insight:** Most Phase 10 complexity is integration, not algorithms; reusing established ECS/UI/undo primitives is safer and faster than introducing new framework layers.

## Common Pitfalls

### Pitfall 1: Breaking future Phase 11 input by reusing Tab
**What goes wrong:** New sketch shortcut collides with current gizmo mode toggle.  
**Why it happens:** `Tab` is globally handled in `app.c`.  
**How to avoid:** Keep Phase 10 on buttons/menu entries only; reserve Tab changes for Phase 11 with explicit conflict resolution.  
**Warning signs:** Tab no longer toggles gizmo mode in manual smoke.

### Pitfall 2: Multi-select actions create multiple undo steps
**What goes wrong:** One UI click requires N undos.  
**Why it happens:** Recording per-entity commands in loops without grouping strategy.  
**How to avoid:** Add dedicated bulk command(s) for sketch fixed-state and delete actions, or equivalent grouped command abstraction.  
**Warning signs:** Undo label count increments by selection size, not by user action.

### Pitfall 3: Sketch delete leaves orphan geometry or stale list entries
**What goes wrong:** Manager list shows dead entities or render artifacts.  
**Why it happens:** Child deletion not routed through recursive scene removal and cache dirtying.  
**How to avoid:** Use `scene_remove_entity()` on sketch root; always refresh manager list cache after structural changes.  
**Warning signs:** `ecs_is_alive` false entries still listed; geometry remains visible after delete.

### Pitfall 4: Incorrect color inheritance logic
**What goes wrong:** Geometry colors unexpectedly override or fail to inherit sketch color.  
**Why it happens:** Missing explicit “non-black RGB overrides” policy branch from D-09.  
**How to avoid:** Centralize effective-color rule and apply in one place in scene update/render path.  
**Warning signs:** Black geometry no longer inherits sketch color, or explicitly colored geometry changes with sketch color unexpectedly.

## Code Examples

Verified patterns from current codebase:

### Add Entity creation flow + undo capture
```c
// Source: src/ui/ui_scene_hierarchy.h
if (igBeginMenu("Add Entity", true)) {
    ecs_entity_t new_entity = 0;
    if (igMenuItem_Bool("Point", NULL, false, true)) {
        new_entity = scene_add_point(state->scene, vec3_make(0,0,0), color, 0.06f);
    }
    if (new_entity != 0 && state->undo_redo) {
        undo_cmd_create_entity(state->undo_redo, new_entity);
    }
}
```

### Parent-child ownership and recursive removal
```c
// Source: src/ecs/ecs_scene.h
scene_set_parent(scene, child, parent);
int child_count = scene_count_children(scene, parent);
scene_remove_entity(scene, parent); // recursively removes children
```

### Inspector edit -> deferred undo recording
```c
// Source: src/ui/ui_entity_inspector.h
if (igIsItemActivated()) {
    state->drag_start_color = g->color;
}
if (igIsItemDeactivatedAfterEdit() && state->undo_redo) {
    undo_cmd_set_color(state->undo_redo, e, state->drag_start_color, g->color);
}
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Monolithic per-entity editing only | Mixed single + multi workflows in inspector/hierarchy | Existing pre-v1.2 baseline | Phase 10 should follow established multi-select patterns, not re-invent UI semantics. |
| Standalone geometry edits without sketch ownership | Planned sketch-owned geometry containers | v1.2 roadmap kickoff | Enables later constraints/solver/script phases without ownership migration churn. |

**Deprecated/outdated for this phase:**
- Implementing constraint authoring UI now (`Tab` menu/glyph authoring): deferred to Phase 11.
- Implementing strict fixed transform enforcement now: deferred to Phase 12.

## Open Questions

1. **How should “constraint count” be represented before Phase 11 authoring exists?**
   - What we know: SKCH-02 requires showing constraint count in inspector.
   - What's unclear: whether to derive from placeholder component or always show `0`.
   - Recommendation: Add explicit `constraint_count` field in sketch manager component, default `0`, so UI/API contract is stable now.

2. **Best undo strategy for bulk delete in GeometryManager?**
   - What we know: Existing delete records per-entity commands; requirement wants one combined undo entry per action.
   - What's unclear: whether to add command grouping or a dedicated bulk-delete command.
   - Recommendation: Implement dedicated `CMD_BULK_DELETE_ENTITIES` snapshot command to satisfy D-05 deterministically.

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| cmake | Configure/build verification | ✓ | 4.3.0 | — |
| git | workflow/tooling scripts | ✓ | 2.51.1.windows.1 | — |
| node | script-based tooling/tests | ✓ | v25.8.1 | — |
| npm | puppeteer script setup | ✓ | 11.11.0 | — |
| python | utility scripts/evaluators | ✓ | 3.13.12 | — |
| Vulkan SDK env (`VULKAN_SDK`) | Windows Vulkan build path | ✓ (SDK dir detected) | — | Use D3D11 build if Vulkan toolchain unavailable |
| MSVC `cl` | Windows MSVC gate builds | ✗ (not on PATH in this shell) | — | Build via VS Developer Prompt/IDE or use existing generated build artifacts |
| ninja | Ninja workflow variant | ✗ | — | Use Visual Studio generator build commands |

**Missing dependencies with no fallback:**
- None blocking for planning. (Implementation execution on this shell may need MSVC Developer environment for full Windows gate.)

**Missing dependencies with fallback:**
- `cl` not on PATH → use Visual Studio developer shell or IDE-driven build.
- `ninja` missing → use Visual Studio generator path already documented in QUICKSTART.

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | Build-gate + manual UI smoke (no dedicated unit-test framework detected) |
| Config file | none — existing validation is command/checklist based |
| Quick run command | `cmake --build build-vulkan --config Release --target mdCAD` |
| Full suite command | `cmake --build build-vulkan --config Release --target math-validation` |

### Phase Requirements → Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| SKCH-01 | Create sketch + attach point/line/arc/circle via intended entrypoints | smoke/manual | `cmake --build build-vulkan --config Release --target mdCAD` then in-app checklist | ✅ |
| SKCH-02 | Inspector shows status/color policy/geometry count/constraint count | smoke/manual | `cmake --build build-vulkan --config Release --target mdCAD` then in-app checklist | ✅ |
| SKCH-03 | GeometryManager single/multi-select fix/unfix/delete with atomic undo | smoke/manual | `cmake --build build-vulkan --config Release --target mdCAD` then in-app checklist | ✅ |

### Sampling Rate
- **Per task commit:** `cmake --build build-vulkan --config Release --target mdCAD`
- **Per wave merge:** `cmake --build build-vulkan --config Release --target math-validation`
- **Phase gate:** Full suite green + targeted manual sketch checklist before `/gsd-verify-work`

### Wave 0 Gaps
- [ ] Add phase-specific manual smoke checklist artifact for SKCH-01..03 under `.planning/phases/10-sketch-foundations-managers/evidence/`
- [ ] Add automated harness/CLI smoke hooks for sketch manager operations (currently manual-only)
- [ ] Add undo stack behavior check script for multi-select atomicity

## Sources

### Primary (HIGH confidence)
- `.planning/phases/10-sketch-foundations-managers/10-CONTEXT.md` - locked decisions/scope/discretion/deferred boundaries.
- `.planning/ROADMAP.md` - Phase 10 goals and success criteria.
- `.planning/REQUIREMENTS.md` - SKCH-01, SKCH-02, SKCH-03 contracts.
- `docs/feature-proposal/Sketches, Constraints, Scripting.md` - canonical feature intent and phase boundaries.
- `src/ui/ui_scene_hierarchy.h` - Add Entity flow, selection patterns, undo integration.
- `src/ui/ui_entity_inspector.h` - inspector composition and undo capture patterns.
- `src/ecs/ecs_world.h` and `src/ecs/ecs_scene.h` - component registration, scene API, parent-child and deletion semantics.
- `src/undo_redo.h` and `src/undo_redo_exec.h` - available command types and bulk command patterns.
- `src/app.c` - current global Tab keybinding and input routing.
- `vendors/flecs/flecs.h` - Flecs pinned version macros.
- `vendors/libcimgui/CMakeLists.txt`, `vendors/libsokol/CMakeLists.txt`, `vendors/CMakeLists.txt` - dependency sourcing/pinning behavior.
- `.planning/config.json` - nyquist validation enabled.

### Secondary (MEDIUM confidence)
- `README.md`, `.planning/codebase/ARCHITECTURE.md`, `.planning/codebase/CONVENTIONS.md`, `.planning/codebase/STRUCTURE.md` - repository architecture and coding pattern references.

### Tertiary (LOW confidence)
- None.

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - versions and dependency sourcing verified from repo files.
- Architecture: HIGH - derived from current implementation files and existing patterns.
- Pitfalls: MEDIUM - inferred from current integration points and phase boundaries; should be validated during implementation smoke.

**Research date:** 2026-03-30  
**Valid until:** 2026-04-29
