# Phase 18: Add undo steps for endpoint moves - Research

**Researched:** 2026-04-04  
**Domain:** Endpoint move undo/redo semantics in ECS + gizmo + inspector flows  
**Confidence:** HIGH

## User Constraints

- No `18-CONTEXT.md` exists yet (no locked phase-specific decision document to copy verbatim).
- Constraints inferred from request and active roadmap/state:
  - Use Phase 17 decisions and existing undo patterns.
  - Preserve sketch-only endpoint behavior.
  - Avoid changing non-sketch semantics.
  - Investigate: gizmo drag path, inspector edit path, scene sync path, undo command creation.

## Summary

Current endpoint movement works live, but undo is not endpoint-aware in the key transform path. In `app.c`, transform-mode gizmo drag applies endpoint selection via `scene_apply_transform_delta_for_selection(...)`, which correctly routes endpoint points through owner geometry updates (`scene_apply_endpoint_point_world_delta(...)`). However, drag-end undo recording still uses `undo_cmd_set_position(...)` against selected entities, and endpoint entities do not move through `TransformComp.position` in this path. Result: endpoint drag can become a no-op undo record.

Inspector editing has a similar coupling issue: endpoint point edits can be recorded as `CMD_SET_POINT_POSITION`, but undo/redo handlers for `CMD_SET_POINT_POSITION` and `CMD_SET_LINE_ENDPOINTS` currently mutate raw geometry only and do not run endpoint-owner sync + sketch side effects. That risks stale endpoint/owner state and missing solver/script refresh after undo/redo.

**Primary recommendation:** Add endpoint-aware undo commands that replay through scene sync helpers (not raw component writes), coalesce at drag/session boundaries, and keep non-sketch transform/script IO behavior unchanged.

## Standard Stack

### Core
| Library/Module | Version | Purpose | Why Standard |
|---|---:|---|---|
| Header-only scene/undo modules (`ecs_scene.h`, `undo_redo.h`, `undo_redo_exec.h`) | in-repo | Command recording + apply/unapply | Existing architecture is static-inline command pattern; Phase 18 should extend this path, not add side systems. |
| ECS world/components (`ecs_world.h`, `EndPointsComp`) | in-repo | Endpoint-owner metadata and sketch guards | Already authoritative for endpoint sketch scoping and bidirectional sync. |
| CTest native executables (`src/tests/*.c`) | CMake/CTest 4.3.0 installed | Regression gates | Existing phase gates already use focused C executables and `ctest -R ...`. |

### Supporting
| Library/Module | Version | Purpose | When to Use |
|---|---:|---|---|
| `scene_apply_transform_delta_for_selection(...)` | in-repo | Central selection delta router | Always for transform-mode drag apply; preserves endpoint/non-endpoint branching. |
| `scene_sync_owner_geometry_from_endpoint_entity(...)` + `scene_sync_endpoint_entities_for_owner(...)` | in-repo | Bidirectional endpoint-owner sync | Use from undo apply/unapply for endpoint-related commands. |
| `scene_solver_request_auto(...)` + `scene_script_reemit_for_sketch(...)` | in-repo | Sketch side effects after geometry mutation | Required for sketch endpoint edits (including undo/redo), not non-sketch entities. |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|---|---|---|
| New endpoint-aware undo command(s) | Keep using `CMD_SET_POSITION` / `CMD_SET_POINT_POSITION` | Fails to capture endpoint drag intent correctly and misses owner/sync side effects. |
| Scene-helper-driven undo apply/unapply | Raw geometry writes in undo executor | Easier initially, but causes stale endpoint metadata and behavior drift from live edit path. |

## Architecture Patterns

### Recommended Project Structure
```text
src/
├── undo_redo.h            # command types + record helpers
├── undo_redo_exec.h       # apply/unapply execution semantics
├── ecs/ecs_scene.h        # endpoint-owner sync + sketch side effects
├── app.c                  # gizmo drag start/update/end boundaries
└── tests/                 # endpoint + solver + script regression executables
```

### Pattern 1: Endpoint-aware command semantics (owner+role, not transform-only)
**What:** Represent endpoint moves using participant identity (owner entity + role/sub-index, old/new local point), then apply/unapply via scene helper path.  
**When to use:** Any endpoint point move from gizmo transform mode or inspector point edits.  
**Why:** Endpoint drag currently mutates owner geometry via scene helper, not endpoint transform.

### Pattern 2: Coalesce live drag to one commit boundary
**What:** Keep per-frame live updates without undo pushes; record undo only on drag end (existing gizmo pattern).  
**When to use:** Transform and geometry drag flows.  
**Why:** Matches existing command granularity patterns (also used by script IO slider session coalescing).

### Pattern 3: Sketch-gated side effects
**What:** After endpoint-affecting undo/redo apply/unapply, run endpoint sync + solver/script updates only when owner belongs to a sketch.  
**When to use:** Endpoint command apply/unapply in undo executor.  
**Why:** Preserves non-sketch semantics (explicitly required).

### Anti-Patterns to Avoid
- **Using `CMD_SET_POSITION` for endpoint drag undo:** endpoint transform position is not the moved state in this path.
- **Undo executor raw writes without sync:** leaves owner/endpoints inconsistent and skips sketch solve/script refresh.
- **Pushing undo per frame during drag:** creates undo spam and breaks established user interaction model.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---|---|---|---|
| Endpoint-to-owner mapping | Ad hoc lookup tables in app layer | Existing `EndPointsComp` metadata + scene helpers | Already canonical and sketch-scoped. |
| Drag coalescing lifecycle | New timer/debounce logic | Existing gizmo begin/update/end + ImGui activation/deactivation patterns | Proven in current app/inspector/script IO flows. |
| Sketch side-effect fanout | Duplicated solve/reemit logic in app/undo layer | `scene_solver_request_auto(...)` + `scene_script_reemit_for_sketch(...)` guarded by sketch checks | Avoids drift and regressions. |

**Key insight:** Reuse scene authority and undo command pattern; the bug is semantic mismatch, not missing infrastructure.

## Common Pitfalls

### Pitfall 1: Recording wrong state space
**What goes wrong:** Undo captures endpoint `TransformComp.position` while movement happened in owner geometry local points.  
**Why it happens:** Endpoint drag is routed through `scene_apply_endpoint_point_world_delta(...)`, not transform writes.  
**How to avoid:** Store owner participant old/new local point (or endpoint geometry old/new plus enforced sync on replay).  
**Warning signs:** Endpoint visibly moves, Ctrl+Z does nothing or partially reverts.

### Pitfall 2: Undo replay skips endpoint-owner sync
**What goes wrong:** Undo updates point/line geometry but counterpart endpoint entities/owners remain stale.  
**Why it happens:** `CMD_SET_POINT_POSITION` / `CMD_SET_LINE_ENDPOINTS` handlers currently set raw geometry and mark renderable dirty only.  
**How to avoid:** In undo apply/unapply, call sync helpers and sketch side effects for endpoint-related commands.  
**Warning signs:** Endpoint dots and line/arc endpoints diverge after undo/redo.

### Pitfall 3: Breaking non-sketch transform behavior
**What goes wrong:** Endpoint-specific logic leaks into non-sketch entities.  
**Why it happens:** Missing sketch guards in replay path.  
**How to avoid:** Keep scene sketch checks (`scene_find_parent_sketch` + `scene_is_sketch`) before endpoint sync/solver/script calls.  
**Warning signs:** Non-sketch line transform path changes from established behavior.

### Pitfall 4: Regressing script IO undo semantics
**What goes wrong:** Endpoint undo changes interfere with script transaction suppression/coalescing.  
**Why it happens:** Touching shared undo stack behavior without preserving command boundaries.  
**How to avoid:** Keep `CMD_SCRIPT_APPLY_TRANSACTION` behavior untouched and add dedicated endpoint command path.  
**Warning signs:** Script slider drags produce many undo entries or fail to revert atomically.

## Code Examples

### Endpoint drag is applied through scene helper (not transform write)
```c
// Source: src/ecs/ecs_scene.h:803-810, 639-685
if (is_endpoint_point) {
    if (scene_apply_endpoint_point_world_delta(scene, e, world_delta)) {
        applied_any = true;
    }
    continue;
}
```

### Current drag undo boundary is at mouse release (correct coalescing point)
```c
// Source: src/app.c:1772-1785
if (!io->MouseDown[0]) {
    vec3_t total_delta = gizmo_end_drag(&state.gizmo);
    if (vec3_length(total_delta) > 1e-7f) {
        for (int i = 0; i < state.gizmo_drag_entity_count; i++) {
            undo_cmd_set_position(&state.undo_redo, e, old_pos, new_pos);
        }
    }
}
```

### Inspector endpoint edits already route through sync helpers (pattern to mirror in undo replay)
```c
// Source: src/ui/ui_entity_inspector.h:1958-1967
if (endpoint_meta && endpoint_meta->is_endpoint_point) {
    scene_sync_owner_geometry_from_endpoint_entity(scene, e);
} else {
    scene_sync_endpoint_entities_for_owner(scene, e);
    scene_solver_request_auto(scene, parent);
    scene_script_reemit_for_sketch(scene, parent);
}
```

## Recommended Plan Split (with verification gates)

### Plan 18-01 — Endpoint undo command contract + recorder wiring
**Scope**
- Add endpoint-aware undo command type(s) (owner/role/sub-index + old/new local points).
- Wire recording at:
  - gizmo transform drag end for selected endpoint entities,
  - inspector endpoint point edit deactivation.
- Keep existing non-endpoint command paths unchanged.

**Verification gate**
- `ctest -R endpoint_pick --test-dir build-vulkan -C Release --output-on-failure`
- Add/extend tests to assert endpoint drag produces meaningful undo delta (not transform no-op).

### Plan 18-02 — Undo executor replay semantics + sketch side effects
**Scope**
- Implement apply/unapply for endpoint command via scene helper flow:
  - owner geometry mutation through participant mapping,
  - endpoint-owner resync,
  - sketch-only solver auto-request + script re-emit.
- Do not alter `CMD_SCRIPT_APPLY_TRANSACTION` behavior.

**Verification gate**
- `ctest -R "endpoint_pick|scene_solver_contract|scene_solver_diagnostics" --test-dir build-vulkan -C Release --output-on-failure`
- Add replay-specific regression tests (undo then redo retains endpoint-owner consistency).

### Plan 18-03 — Regression hardening for non-sketch + script IO invariants
**Scope**
- Add tests proving:
  - non-sketch line transform semantics unchanged,
  - script roundtrip/script-IO undo flow still transactional/coalesced,
  - endpoint undo does not inject extra entries during live drag.
- Update phase validation evidence with explicit command outputs.

**Verification gate**
- `ctest -R "endpoint_pick|scene_solver_contract|scene_solver_drag|scene_solver_diagnostics|script_roundtrip_tests" --test-dir build-vulkan -C Release --output-on-failure`

## State of the Art

| Old Approach | Current Approach | Impact for Phase 18 |
|---|---|---|
| Synthetic endpoint pick ranges | Native endpoint entities (`EndPointsComp`) in sketch scope | Undo must now respect endpoint-owner metadata contracts. |
| Transform-only undo assumption | Mixed transform + geometry-participant mutation paths | Endpoint moves need dedicated undo semantics. |

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|---|---|---|---|---|
| cmake | Build test targets | ✓ | 4.3.0 | — |
| ctest | Run verification gates | ✓ | 4.3.0 | — |

**Missing dependencies with no fallback:** None.  
**Missing dependencies with fallback:** None.

## Validation Architecture

### Test Framework
| Property | Value |
|---|---|
| Framework | CTest + native C test executables |
| Config file | `src/CMakeLists.txt` |
| Quick run command | `ctest -R endpoint_pick --test-dir build-vulkan -C Release --output-on-failure` |
| Full suite command | `ctest -R "endpoint_pick|scene_solver_contract|scene_solver_drag|scene_solver_diagnostics|script_roundtrip_tests" --test-dir build-vulkan -C Release --output-on-failure` |

### Phase Requirements → Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|---|---|---|---|---|
| PH18-01 | Endpoint gizmo drag creates valid undo step | unit/integration | `ctest -R endpoint_pick --test-dir build-vulkan -C Release --output-on-failure` | ⚠️ extend existing |
| PH18-02 | Undo/redo keeps endpoint-owner sync and sketch side effects | unit/integration | `ctest -R "endpoint_pick|scene_solver_contract" --test-dir build-vulkan -C Release --output-on-failure` | ⚠️ add cases |
| PH18-03 | Non-sketch behavior unchanged | regression | `ctest -R endpoint_pick --test-dir build-vulkan -C Release --output-on-failure` | ✅ baseline + extend |
| PH18-04 | Script IO transaction undo unaffected | regression | `ctest -R script_roundtrip_tests --test-dir build-vulkan -C Release --output-on-failure` | ✅ |

### Sampling Rate
- **Per task commit:** `ctest -R endpoint_pick --test-dir build-vulkan -C Release --output-on-failure`
- **Per wave merge:** `ctest -R "endpoint_pick|scene_solver_contract|scene_solver_diagnostics" --test-dir build-vulkan -C Release --output-on-failure`
- **Phase gate:** `ctest -R "endpoint_pick|scene_solver_contract|scene_solver_drag|scene_solver_diagnostics|script_roundtrip_tests" --test-dir build-vulkan -C Release --output-on-failure`

### Wave 0 Gaps
- [ ] Add endpoint undo replay tests (new or expanded `src/tests/endpoint_pick_test.c`) for apply/unapply sync behavior.
- [ ] Add explicit non-sketch + endpoint mixed-selection undo regression case.
- [ ] Add script-roundtrip smoke assertion in phase verification checklist.

## Sources

### Primary (HIGH confidence)
- `src/app.c` (gizmo drag update + undo commit boundaries, script IO coalescing patterns)
- `src/ecs/ecs_scene.h` (endpoint apply/sync helpers, sketch guards, solver/script side effects)
- `src/undo_redo.h` + `src/undo_redo_exec.h` (command model and apply/unapply behavior)
- `src/ui/ui_entity_inspector.h` (inspector edit + endpoint sync path)
- `src/tests/endpoint_pick_test.c`, `src/tests/scene_solver_*.c`, `src/tests/script_roundtrip_tests.c`
- `src/CMakeLists.txt` (test target registration)
- `.planning/STATE.md`, `.planning/ROADMAP.md`, `.planning/phases/17-*/17-0*-SUMMARY.md` (Phase 17 architectural decisions)

### Secondary (MEDIUM confidence)
- None required; research grounded in current repository implementation.

### Tertiary (LOW confidence)
- None.

## Metadata

**Confidence breakdown:**
- Standard stack: **HIGH** — direct codebase and build config inspection.
- Architecture: **HIGH** — traced live edit, sync, undo recording, and undo replay call paths.
- Pitfalls: **HIGH** — directly evidenced by current command semantics and scene behavior.

**Research date:** 2026-04-04  
**Valid until:** 2026-05-04
