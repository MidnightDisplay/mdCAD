# Phase 12: solver-control-and-constrained-interaction - Research

**Researched:** 2026-04-01  
**Domain:** Sketch solver control UX + constrained gizmo interaction in mdCAD ECS/ImGui runtime  
**Confidence:** MEDIUM

## User Constraints

### Locked Decisions
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

### Deferred Ideas (OUT OF SCOPE)
- Multiple selectable solver backends and backend-switch UX (explicitly out of v1.2 scope).
- Cross-sketch/global solve graph behavior.
- Script-driven solver control or diagnostics export formats (Phase 13/14+ concerns).

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| SOLV-01 | User can toggle auto-solve per sketch and manually trigger solve recalculation from Solver controls. | Add per-sketch solver policy fields in `SketchComp` and scene-level trigger APIs; surface controls in SketchManager panel. |
| SOLV-02 | User can see sketch solve states (`solved`, `loose`, `fixed`, `error`) and timestamped solver diagnostics with `INFO`, `WARNING`, and `ERROR` levels. | Replace Phase-10 placeholder derivation with solver-fed status updates + bounded diagnostics ring buffer (100 entries). |
| SOLV-03 | The system uses one solver backend type for v1.2 and exposes that active type in sketch solver controls. | Add read-only backend identifier/name in sketch solver panel; keep backend fixed in config/codepath. |
| SOLV-04 | User can identify implicated constraints/geometries when a solve fails or is invalid. | Reuse `constraint_selection_apply_participants(...)`, store implicated constraint IDs in diagnostics entries, persist highlight until next success. |
| API-03 | User manipulation/gizmo transforms respect active constraints during interaction. | Insert constraint projection/blocking in active drag path (`app.c` `gizmo_update_drag` application block) with immediate UX feedback + log. |

</phase_requirements>

## Summary

Phase 12 is an integration phase, not a greenfield subsystem. The repo already has core sketch/constraint scaffolding (SketchComp status taxonomy, constraint entities, participant back-references, glyph picking, dimensional editing, shared participant-highlighting helper), but **no actual solver execution backend and no diagnostics pipeline** yet. The primary planning risk is wiring solver semantics across existing mutation paths (constraint creation/editing, geometry edits, gizmo drag updates, undo/redo) without introducing inconsistent states or breaking Phase 16 highlight parity.

Current architecture strongly favors a scene-centric approach: keep UI thin, add solver orchestration to `src/ecs/ecs_scene.h`, and invoke that orchestration from existing mutation chokepoints. For constrained interaction (`API-03`), the active drag loop in `src/app.c` is the single canonical insertion point for real-time constrained projection or movement blocking. This is where unsatisfiable motion can be rejected and surfaced as both viewport toast and diagnostics event.

Validation should remain Nyquist-style (combined build + ctest companion command with explicit manual interaction rows), because no first-party automated unit test framework is presently configured in-repo. A robust plan should prioritize deterministic state transitions, bounded diagnostic storage, and explicit requirement-to-check mapping in manual verification artifacts.

**Primary recommendation:** Implement a per-sketch solver runtime contract in `ecs_scene.h` first (status, auto/manual trigger, diagnostics log, implication payload), then thread all UI and gizmo behavior through that contract instead of adding direct ad hoc logic in inspector/app code.

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| Flecs | vendored amalgamation (exact semver not declared in vendor CMake) | ECS entity/component graph for sketches, geometry, constraints | Existing scene ownership, parent-child sketch model, and component access patterns already rely on Flecs. |
| cimgui / Dear ImGui | cimgui `1.92.5dock` (from `vendors/libcimgui/CMakeLists.txt`) | Immediate-mode UI for SketchManager/ConstraintManager and solver controls | Existing inspector/workspace UX is ImGui-first; solver controls should extend same panel model. |
| libsokol | fetched from `master` (from `vendors/libsokol/CMakeLists.txt`) | App shell, rendering, viewport integration, input flow | Existing frame/event loop and pick-buffer pipeline run through Sokol. |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| cglm | `0.9.6` (`vendors/cglm/VERSION.txt`) | Math and transforms used by interaction/projection codepaths | For drag projection math, constraint-space calculations, and stable transform updates. |
| cJSON | `1.7.19` (from `vendors/cjson/cJSON.h`) | Structured diagnostic serialization (future-ready) | Only if diagnostics need persistence/export in later phases. |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Scene-level solver orchestration in `ecs_scene.h` | UI-local solve logic in `ui_entity_inspector.h` / `app.c` | UI-local logic causes duplicated trigger rules and fragile consistency; scene-level orchestration keeps one source of truth. |
| Reusing shared participant helper | New custom highlight path for failures | Regresses Phase 16 parity guarantees and duplicates participant traversal logic. |

**Installation:**  
No new package installation required for planning baseline; project uses vendored/fetchcontent dependencies in CMake.

## Architecture Patterns

### Recommended Project Structure
```text
src/
├── components/
│   ├── sketch_comp.h                 # extend with auto-solve + backend + diagnostics metadata refs
│   └── (optional) sketch_solver_comp.h  # dedicated solver runtime state if SketchComp should stay compact
├── ecs/
│   └── ecs_scene.h                   # scene_solver_* APIs, trigger routing, implication extraction
├── constraints/
│   ├── constraint_selection.h        # reuse for failure implication highlighting
│   └── (optional) constraint_solver_bridge.h
├── ui/
│   └── ui_entity_inspector.h         # SketchManager solver controls + diagnostics panel
└── app.c                             # constrained drag integration + toast lifecycle
```

### Pattern 1: Scene-Orchestrated Solver Triggering
**What:** All solve requests (auto and manual) route through `scene_solver_request_*` APIs.  
**When to use:** Any mutation that can affect constraint validity (geometry edit, constraint add/remove/edit, driven toggle, gizmo drag step/finalize).  
**Example:**
```c
// Source: src/ecs/ecs_scene.h + src/app.c existing mutation patterns
scene_geometry_apply_delta(scene, entity, delta);
scene_solver_request(scene, sketch_entity, SCENE_SOLVE_REASON_GEOMETRY_EDIT);
```

### Pattern 2: Shared Highlight Path for Failure Implications
**What:** Failure/invalid outcomes highlight implicated constraints + participants via existing helper.  
**When to use:** On any solver result with status `error` or invalid constraints.  
**Example:**
```c
// Source: src/constraints/constraint_selection.h, used in app+inspector
state.selected_constraint_entity = first_implicated_constraint;
constraint_selection_apply_participants(&state.selection, &state.ecs_world, first_implicated_constraint);
```

### Pattern 3: Real-Time Constrained Drag Gate
**What:** In drag update loop, run projected delta through solver feasibility check before mutating ECS transform/vertices.  
**When to use:** `state.gizmo_drag_active` path in `src/app.c` for both transform and geometry edit modes.  
**Example:**
```c
// Source anchor: src/app.c lines ~1114-1150 (current unconstrained delta apply)
vec3_t delta = gizmo_update_drag(&state.gizmo, mouse_ray);
if (scene_solver_can_apply_drag(scene, sketch, selection, delta, &projected_delta)) {
    scene_apply_drag_delta(scene, selection, projected_delta);
} else {
    // block movement + toast + diagnostic entry + implication highlight
}
```

### Anti-Patterns to Avoid
- **Duplicated solve triggers in UI + app + scene:** causes non-deterministic behavior and missed triggers.
- **Clearing diagnostics on each solve:** violates locked D-05.
- **Transient-only failure highlight:** violates locked D-08 persistence requirement.
- **Allowing temporary invalid drag states then snapping back:** violates locked D-10.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Participant highlighting logic | New custom iteration over constraint participants | `constraint_selection_apply_participants(...)` | Already verified in Phase 16 parity closure. |
| Constraint legality matrix | Ad hoc per-UI checks | `constraint_type_is_selection_legal(...)` in `constraint_types.h` | Single legal-source contract already used by context menu. |
| Undo coupling for geometry/constraint side effects | Separate ad hoc undo records per micro-action | Existing undo command patterns (`undo_cmd_*`) with bulk snapshots | Existing system already captures linked constraints and hierarchy restoration. |
| Drag projection math basis | Parallel custom ray/axis/plane interaction module | Existing gizmo interaction math (`gizmo_update_drag`, interaction helpers) | Reduces interaction divergence and rendering mismatch risk. |

**Key insight:** The repo already has reliable primitives for selection, legality, and undo coupling. Phase 12 should compose them into solver behavior, not replace them.

## Common Pitfalls

### Pitfall 1: Status Drift Between Placeholder and Solver Output
**What goes wrong:** `scene_derive_sketch_status(...)` keeps deriving status from fixed/geometry counts while solver writes different truth.  
**Why it happens:** Phase 10 placeholder logic still active in `ecs_scene.h`.  
**How to avoid:** Introduce explicit solver-authoritative status path and gate/retire placeholder derivation after solver integration.  
**Warning signs:** Status flickers back to `solved/loose/fixed` after solver sets `error`.

### Pitfall 2: Missing Solve Triggers on Some Mutation Paths
**What goes wrong:** Some edits auto-solve, others silently skip solve.  
**Why it happens:** Multiple mutation entrypoints (`ui_entity_inspector`, `app.c`, scene helpers).  
**How to avoid:** Centralize triggering in scene mutation APIs and call them from all UI/interaction paths.  
**Warning signs:** Manual recalc changes outcome after operations that should already have solved.

### Pitfall 3: Constraint Highlight Regressions
**What goes wrong:** Failure implication highlighting differs from glyph/manager selection behavior.  
**Why it happens:** New failure-specific selection code bypasses shared helper.  
**How to avoid:** Always route failure highlight through `constraint_selection_apply_participants(...)`.  
**Warning signs:** Different participant set for same constraint depending on entrypoint.

### Pitfall 4: Undo Fragmentation During Drag
**What goes wrong:** Drag interactions emit too many undo entries or lose constrained projection outcome.  
**Why it happens:** Recording every frame update instead of interaction boundary snapshots.  
**How to avoid:** Keep existing drag-start snapshot / drag-end commit pattern and record one undo action per drag completion.  
**Warning signs:** Ctrl+Z steps through each mouse move.

## Code Examples

Verified in-repo integration anchors:

### Existing drag mutation chokepoint (to constrain)
```c
// Source: src/app.c:1119-1150
vec3_t delta = gizmo_update_drag(&state.gizmo, mouse_ray);
if (vec3_length(delta) > 1e-7f) {
    // currently applies delta directly to transforms/vertices
}
```

### Existing shared participant highlighting path
```c
// Source: src/constraints/constraint_selection.h:14-45
constraint_selection_apply_participants(&state.selection, &state.ecs_world, clicked_constraint);
```

### Existing sketch status metadata refresh hook
```c
// Source: src/ecs/ecs_scene.h:1581-1589
sk->geometry_count = scene_count_sketch_geometry(scene, sketch);
sk->fixed_geometry_count = scene_count_sketch_fixed_geometry(scene, sketch);
sk->constraint_count = scene_count_sketch_constraints(scene, sketch);
sk->status = scene_derive_sketch_status(scene, sketch);
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Placeholder sketch status derived from fixed-count heuristics | Solver-authoritative status expected by Phase 12 contract | Phase 12 | Must replace/augment derivation logic to avoid semantic mismatch. |
| Constraint selection paths diverged (manager vs glyph) | Unified via shared participant helper | Phase 16 | Phase 12 must preserve this parity for failure highlighting. |

**Deprecated/outdated for this phase:**
- Placeholder-only status derivation as final truth for solver outcomes.
- Any assumption that fixed state is visual-only (Phase 10 deferral ended by Phase 12 scope).

## Open Questions

1. **Solver backend concrete implementation for v1.2**
   - What we know: Requirement demands one backend type visible in UI (`SOLV-03`), but no backend is currently wired in repo.
   - What's unclear: Exact backend library/module chosen for this milestone.
   - Recommendation: Plan Wave 0 task to lock backend identifier and adapter API before UI work.

2. **Debounce interval and queue coalescing strategy**
   - What we know: Debounce/coalescing is discretionary; auto-solve required after sketch-affecting edits.
   - What's unclear: Frame-based vs timestamp-based queue and cancellation semantics.
   - Recommendation: Pick deterministic frame-time window (e.g., 1-2 frames) and test with rapid slider edits + drag.

3. **Constraint implication extraction contract**
   - What we know: Failure must identify implicated constraints/geometries and auto-focus first implicated constraint.
   - What's unclear: Backend result payload format and ordering.
   - Recommendation: Define stable implication payload struct in scene layer and keep UI agnostic.

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| cmake | Build + validation gates | ✓ | 4.3.0 | — |
| ctest | Validation gate command | ✓ | 4.3.0 | Manual-only checks (lower confidence) |
| Vulkan SDK (`VULKAN_SDK`) | Windows Vulkan path | ✓ | `C:\VulkanSDK\1.4.341.1` | D3D11 build path (not milestone gate) |
| glslc | Shader toolchain support | ✓ | shaderc v2026.1 | Existing precompiled assets |
| git | Workflow + evidence capture | ✓ | 2.51.1.windows.1 | — |
| ninja | Optional local generator | ✗ | — | MSBuild/CMake generator in `build-vulkan` |

**Missing dependencies with no fallback:**
- None identified for current Windows MSVC + Vulkan gate workflow.

**Missing dependencies with fallback:**
- `ninja` absent; use existing `build-vulkan` MSBuild/CMake path.

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | CMake build gate + CTest harness + manual interactive verification |
| Config file | `CMakeLists.txt`, `src/CMakeLists.txt` |
| Quick run command | `cmake --build build-vulkan --config Release --target mdCAD` |
| Full suite command | `ctest --test-dir build-vulkan -C Release --output-on-failure` |

### Phase Requirements → Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| SOLV-01 | Auto-solve toggle + manual recalc | Manual integration + gate | `cmake --build build-vulkan --config Release --target mdCAD && ctest --test-dir build-vulkan -C Release --output-on-failure` | ✅ |
| SOLV-02 | Status + timestamped INFO/WARNING/ERROR diagnostics | Manual UI + gate | same as above | ✅ |
| SOLV-03 | Single backend displayed | Manual UI + gate | same as above | ✅ |
| SOLV-04 | Failure implication visibility + focus | Manual UI + gate | same as above | ✅ |
| API-03 | Constrained drag blocking/projection behavior | Manual interaction + gate | same as above | ✅ |

### Sampling Rate
- **Per task commit:** `cmake --build build-vulkan --config Release --target mdCAD`
- **Per wave merge:** `ctest --test-dir build-vulkan -C Release --output-on-failure`
- **Phase gate:** Combined command must pass before `/gsd-verify-work`

### Wave 0 Gaps
- [ ] `12-VALIDATION.md` initial Nyquist-compliant map for SOLV-01..04 + API-03 (manual rows with companion command).
- [ ] Explicit interactive checklist for constrained drag edge cases (unsatisfiable move block, implication persistence, recalc behavior).
- [ ] Optional: first-party targeted smoke test harness (if introduced) for non-UI solver state transitions.

## Sources

### Primary (HIGH confidence)
- `.planning/phases/12-solver-control-and-constrained-interaction/12-CONTEXT.md` — locked decisions and scope.
- `.planning/REQUIREMENTS.md` — SOLV/API requirement contracts.
- `src/app.c` — key routing, glyph handling, drag lifecycle, mutation chokepoints.
- `src/ecs/ecs_scene.h` — sketch metadata/status derivation and scene mutation helpers.
- `src/ui/ui_entity_inspector.h` — SketchManager/GeometryManager/ConstraintManager integration surface.
- `src/constraints/constraint_selection.h` — canonical participant highlight helper.
- `.planning/phases/16-constraint-ux-closure-and-verification/16-VERIFICATION.md` — parity behavior already validated and must not regress.

### Secondary (MEDIUM confidence)
- `.planning/codebase/ARCHITECTURE.md` — subsystem boundary guidance.
- `.planning/codebase/CONVENTIONS.md` — coding conventions for planning fit.
- `.planning/codebase/TESTING.md` — current testing baseline and gaps.
- `docs/feature-proposal/Sketches, Constraints, Scripting.md` — product intent details.

### Tertiary (LOW confidence)
- None (no unverified web-only claims used).

## Metadata

**Confidence breakdown:**
- Standard stack: MEDIUM — versions are partly pinned (cglm/cjson/cimgui tag), but some deps track floating references (libsokol master, flecs amalgamation without explicit semver in local metadata).
- Architecture: HIGH — directly validated against current code integration points.
- Pitfalls: MEDIUM — based on code and prior phase artifacts; runtime behavior still requires interactive verification.

**Research date:** 2026-04-01  
**Valid until:** 2026-04-08 (fast-moving active feature phase)

