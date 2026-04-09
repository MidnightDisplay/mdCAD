# Phase 29: Active-Sketch Line Gizmo Endpoint Authority - Research

**Researched:** 2026-04-09  
**Domain:** mdCAD gizmo interaction + sketch endpoint authority + undo/redo command model  
**Confidence:** HIGH

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
### Scope boundary and eligibility
- **D-01:** Endpoint-authority behavior applies to active-sketch `GEOM_LINE` entities and is not limited to single-line selection.
- **D-02:** Multi-selection of active-sketch lines is supported; selected lines translate as rigid geometry by moving their endpoints `A`/`B` together.
- **D-03:** For mixed selections, apply endpoint-authority only to eligible active-sketch lines while non-line/non-active entities continue existing semantics in the same drag interaction.
- **D-04:** Non-active sketch lines and non-line geometry must preserve current behavior (`GZM-03` guardrail).

### Mode behavior
- **D-05:** Endpoint-authority can activate in `GIZMO_TRANSFORM_MODE`.
- **D-06:** Endpoint-authority also activates in `GIZMO_GEOMETRY_MODE` when vertex mode is inactive.
- **D-07:** Vertex-mode active editing remains vertex-authoritative and is not replaced by line endpoint-authority.

### Midpoint anchoring behavior
- **D-08:** For a single active-sketch line, gizmo center is anchored to the line midpoint.
- **D-09:** For multi-line active-sketch selection, use one gizmo anchored at the average of selected line midpoints.

### Undo/redo interaction contract
- **D-10:** One completed mouse drag is recorded as one grouped undo/redo interaction across all affected active-sketch line endpoints.
- **D-11:** Undo/redo must restore exact endpoint geometry for the full drag interaction (`GZM-04`), not fragmented per-endpoint history entries.

### the agent's Discretion
- Exact internal data-path mechanics for hybrid mixed-selection application (eligible line subset + existing fallback path), provided D-01..D-11 remain true.
- Exact helper boundaries and naming for midpoint-center computation and grouped undo packaging.
- Exact regression test split across existing test binaries.

### Deferred Ideas (OUT OF SCOPE)
None — discussion stayed within phase scope.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| GZM-01 | Gizmo anchors at midpoint for selected active-sketch line | Add active-sketch-line center path in `gizmo_update(...)` using line endpoint geometry midpoint(s). |
| GZM-02 | Active-sketch line gizmo drag updates endpoints A/B as rigid translation | Add endpoint-authority drag apply path for eligible line subset; apply delta to both endpoint participants. |
| GZM-03 | Endpoint-driven behavior only for eligible active-sketch lines | Add explicit eligibility guardrails (active sketch + `GEOM_LINE` + mode gating), preserve fallback transform/vertex behavior for others. |
| GZM-04 | Undo/redo coherent single interaction restoring endpoint geometry | Introduce single grouped undo command for whole drag (bulk endpoint or bulk line endpoints), replay with endpoint sync + auto-solve/script reemit. |
</phase_requirements>

## Summary

Phase 29 should be implemented by adding a **targeted active-sketch line path** on top of existing gizmo flow, not by rewriting scene drag architecture. The current behavior already has robust drag lifecycle staging in `app.c` (start/update/end), endpoint application APIs in `ecs_scene.h`, and release-boundary undo recording in `undo_redo_exec.h`.

The main gap is that selected sketch lines in transform-style gizmo interactions are currently moved through transform position changes (`scene_apply_transform_delta_for_selection(...)`) rather than line endpoint geometry movement. Gizmo center is also computed from entity world centers (often transform-based), so line midpoint anchoring is not guaranteed for active-sketch lines.

**Primary recommendation:** implement a new eligibility-driven active-sketch-line path (center + drag apply + grouped undo command) and keep all non-eligible selections on existing paths unchanged.

## Project Constraints (from copilot-instructions.md)

`./copilot-instructions.md` not found in repository root at research time; no additional project-specific directives discovered from that file.

## Standard Stack

### Core
| Library/Module | Version | Purpose | Why Standard Here |
|---|---:|---|---|
| C + header-inline architecture | repo-native | Interaction + ECS + solver | Current codebase pattern; phase should extend existing inline APIs. |
| `src/app.c` gizmo lifecycle | repo-native | Drag begin/update/end orchestration | Existing single source of truth for interaction transitions. |
| `src/ecs/ecs_scene.h` endpoint/solver APIs | repo-native | Authoritative sketch geometry edits | Already handles endpoint-owner sync, sketch scope checks, solver queueing. |
| `src/undo_redo.h` + `src/undo_redo_exec.h` | repo-native | Undo command model + replay | Existing command stack and replay hooks; extend instead of ad-hoc history. |

### Supporting
| Library/Module | Purpose | When to Use |
|---|---|---|
| `src/gizmo/gizmo.h` | Gizmo center + handle geometry + drag delta generation | For midpoint anchor logic and mode-aware center policy. |
| `src/tests/endpoint_pick_test.c` | Endpoint/owner sync and undo behavior contracts | Add most new Phase 29 regression coverage here. |
| `src/tests/scene_solver_drag_test.c` | Solver drag feasibility contracts | Keep solver bounded projection/unsat behavior guardrails. |

## Codebase Findings

1. **Current center logic is transform-centric** (**HIGH**)  
   - `gizmo_update(...)` averages `gizmo_entity_world_center(...)` in non-vertex mode (`src/gizmo/gizmo.h:259-266`).  
   - `gizmo_entity_world_center(...)` uses transform and only special-cases `GEOM_POINT`, not line midpoint (`src/gizmo/gizmo.h:112-125`).

2. **Active-sketch eligibility pattern already exists and is reusable** (**HIGH**)  
   - `mdcad_is_active_sketch_standalone_point_entity(...)` and `mdcad_selection_is_active_sketch_standalone_points(...)` show explicit active sketch + geometry checks (`src/app.c:161-184`).

3. **Drag apply path currently routes transform mode through transform deltas** (**HIGH**)  
   - Update loop applies transform path via `scene_apply_transform_delta_for_selection(...)` unless in direct point mode or vertex mode (`src/app.c:1852-1869`).

4. **Endpoint-authoritative scene APIs are already available** (**HIGH**)  
   - `scene_apply_endpoint_point_world_delta(...)` updates owner geometry, syncs endpoint entities, sets drag anchor for A/B, requests auto solve, and re-emits script (`src/ecs/ecs_scene.h:801-851`).

5. **Undo stack has no built-in grouping/composite transaction command** (**HIGH**)  
   - `undo_redo_push(...)` stores one command per push; no begin/end group API (`src/undo_redo.h:540-556`).

6. **Existing line-endpoints undo command does not currently perform endpoint sync/solver re-trigger on replay** (**HIGH**)  
   - `CMD_SET_LINE_ENDPOINTS` apply/unapply sets `g->data.line.{a,b}` and dirty renderable only (`src/undo_redo_exec.h:1308-1317`, `1579-1588`).  
   - `CMD_MOVE_ENDPOINT_PARTICIPANT` replay does endpoint sync + solver request + script reemit (`src/undo_redo_exec.h:1173-1197`, `1437-1439`).

## Recommended Implementation Strategy

### 1) Add explicit Phase-29 eligibility helpers in `app.c`
- Introduce helpers parallel to existing point eligibility:
  - `mdcad_is_active_sketch_line_entity(entity, active_sketch)`
  - `mdcad_collect_active_sketch_line_drag_sets(...)` → split selection into:
    - eligible active-sketch lines
    - fallback entities (everything else)
- Mode gate per decisions:
  - eligible when `(edit_mode == GIZMO_TRANSFORM_MODE) || (edit_mode == GIZMO_GEOMETRY_MODE && !vertex_mode.active)`.

### 2) Midpoint anchor in gizmo center computation (GZM-01)
- Extend `gizmo_update(...)` signature to accept active sketch + optional line-center policy callback/context, or add a new helper in app before calling update.
- Compute center contributions as:
  - eligible line: midpoint `(a+b)/2` from `GeometryComp` line endpoints.
  - fallback entity: existing `gizmo_entity_world_center(...)`.
- For all-eligible multi-line selection, center = average of selected line midpoints (D-09).

### 3) Endpoint-authoritative drag apply path (GZM-02, GZM-03)
- In active drag update (`app.c` around current delta apply branch):
  - keep requested/projected delta acquisition unchanged.
  - apply projected delta to eligible line subset by moving both endpoint participants (`POINT_A`, `POINT_B`) as rigid translation.
  - apply same delta to fallback entities via existing transform/vertex semantics.
- Prefer a new scene helper for maintainability:
  - `scene_apply_active_sketch_line_world_delta(scene, line_entity, delta)` that:
    - verifies line + active-sketch eligibility,
    - resolves endpoint bindings from owner endpoints metadata,
    - applies delta to A and B via participant-local update path,
    - performs one endpoint sync + solver/script trigger.

### 4) Grouped undo/redo coherence (GZM-04)
- Implement one new command type for drag-level coherence:
  - Recommended: `CMD_BULK_MOVE_ENDPOINT_PARTICIPANTS` (array of owner/role/sub-index old/new).
  - Alternative: `CMD_BULK_SET_LINE_ENDPOINTS` (array of line old/new A/B) **plus** replay-time endpoint sync + solver request.
- Record once at mouse release:
  - start snapshot: for each eligible line capture A/B local positions at drag start.
  - release snapshot: capture final A/B.
  - emit single bulk command containing all changed endpoints/lines.
- Replay contract:
  - apply and unapply must restore exact local endpoint geometry.
  - after each replay: `scene_sync_endpoint_entities_for_owner(...)`, `scene_solver_request_auto(...)`, `scene_script_reemit_for_sketch(...)`.

## Architecture Patterns

### Pattern 1: Orchestration in app, authority in scene
**What:** `app.c` decides when drag starts/ends; scene APIs perform geometry-authoritative mutation.  
**Use:** Keep UI interaction logic separate from geometry mutation internals.  
**Anchor:** `src/app.c:1693-1950`, `src/ecs/ecs_scene.h:801-851`.

### Pattern 2: Release-boundary undo capture
**What:** Drag updates do not push commands per-frame; undo recorded on mouse-up.  
**Use:** Preserve coherent user interaction semantics and avoid history spam.  
**Anchor:** `src/app.c:1883-1940`.

### Pattern 3: Sketch scope guardrails before endpoint behavior
**What:** Endpoint owner and parent sketch are validated before mutation.  
**Use:** Prevent non-sketch/non-eligible entities from incorrectly entering endpoint-authority path.  
**Anchor:** `src/ecs/ecs_scene.h:824-826`, `2279-2283`.

### Anti-Patterns to Avoid
- **Per-frame undo pushes during drag:** bloats history and violates GZM-04 coherence.
- **Replacing existing transform semantics globally:** breaks GZM-03 guardrails.
- **Using `CMD_SET_LINE_ENDPOINTS` as-is for sketch-line drag undo:** currently misses endpoint/sync solver replay side effects.

## Don’t Hand-Roll

| Problem | Don’t Build | Use Instead | Why |
|---|---|---|---|
| Endpoint-owner consistency | Custom one-off endpoint entity update code in app | `scene_sync_endpoint_entities_for_owner(...)` | Centralized endpoint binding/sync logic already enforces sketch guardrails. |
| Solver drag diagnostics | Ad-hoc unsat messaging | `scene_solver_can_apply_drag(...)` + `scene_solver_drag_make_rejected_diagnostic(...)` | Keeps drag feasibility behavior aligned with existing solver diagnostics pipeline. |
| Drag snapshot mapping | New ad-hoc per-entity state format | Existing release-boundary snapshot pattern in `app.c` and helper style in `undo_redo_exec.h` | Reduces risk of mismatch between start/end capture and replay. |

## Risks/Mitigations

### Risk 1: Mixed-selection delta divergence
- **Risk:** Eligible lines and fallback entities may receive different delta logic.  
- **Mitigation:** Compute one resolved drag delta (post solver projection), then apply that same delta to both subsets through their respective apply functions.

### Risk 2: Undo replay does not refresh endpoint visuals/solver state
- **Risk:** Geometry restored but endpoint entities or solver status stale.  
- **Mitigation:** In new grouped command replay, explicitly call endpoint sync + solver auto request + script reemit per touched owner.

### Risk 3: Geometry-mode (vertex inactive) accidental regression
- **Risk:** New line-authority logic may interfere with vertex-active workflow.  
- **Mitigation:** Keep strict mode guard: only when geometry mode and `!vertex_mode.active` (D-06/D-07).

### Risk 4: Non-active/non-line semantics drift
- **Risk:** Endpoint-authority leaks into ineligible entities.  
- **Mitigation:** Centralize eligibility checks in helper(s), add negative tests for non-sketch line and mixed selection guardrails.

## Common Pitfalls

### Pitfall 1: Using transform position for sketch-line authority
**What goes wrong:** line appears to move visually without endpoint geometry authority.  
**Avoid:** always mutate line participant points A/B for eligible active-sketch lines.

### Pitfall 2: Fragmented undo stack for one drag
**What goes wrong:** one mouse drag requires multiple undo keypresses.  
**Avoid:** single bulk command for full interaction.

### Pitfall 3: Midpoint computed from owner transform instead of line data
**What goes wrong:** gizmo anchor drifts from actual line geometry.  
**Avoid:** midpoint from `GeometryComp.line.a/b`.

## Test Strategy

Primary test target: `endpoint_pick` (`src/CMakeLists.txt:134-144`) with focused additions; keep solver projection checks in `scene_solver_drag`.

### New tests to add (recommended)
1. **Single active-sketch line midpoint anchor** (GZM-01)  
   - assert gizmo center equals `(a+b)/2` for active sketch line.
2. **Multi active-sketch lines midpoint averaging** (GZM-01 + D-09)  
   - assert center equals average of selected line midpoints.
3. **Eligible line drag moves both endpoints rigidly** (GZM-02)  
   - after delta, `a' = a + d`, `b' = b + d`.
4. **Mixed selection guardrail** (GZM-03)  
   - active sketch line endpoints move by authority path; non-active/non-line entity keeps legacy transform/vertex semantics.
5. **Single grouped undo for multi-line drag** (GZM-04)  
   - one drag pushes one undo command; undo/redo restores exact endpoint geometry for all touched lines.
6. **Vertex-mode active unaffected** (GZM-03, D-07)  
   - ensure geometry vertex mode path remains authoritative when active.

### Requirement Traceability Matrix
| Requirement | Implementation Hook | Verification |
|---|---|---|
| GZM-01 | midpoint center path in gizmo update | midpoint/average tests |
| GZM-02 | eligible line endpoint A/B world-delta apply | rigid translation endpoint assertions |
| GZM-03 | eligibility guards + mixed subset fallback | mixed selection + vertex-mode negative tests |
| GZM-04 | single bulk undo command + replay sync | undo count/name + exact A/B restore on undo/redo |

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|---|---|---|---|---|
| cmake | configure/build test binaries | ✓ | 4.3.1 | — |
| ctest | targeted test runs | ✓ | 4.3.1 | run binaries directly from build output |
| gcc | C compilation (non-MSVC path) | ✓ | 15.2.0 | — |
| ninja | optional generator | ✗ | — | use default generator / existing build dirs |
| cl (MSVC) | optional Windows toolchain path | ✗ (in current shell PATH) | — | GCC/MinGW build path present |

**Missing dependencies with no fallback:** None identified for this phase in current environment.  
**Missing dependencies with fallback:** `ninja`, `cl` (fallback to available CMake+GCC path and/or existing build directories).

## Validation Architecture

### Test Framework
| Property | Value |
|---|---|
| Framework | CTest + native C test executables |
| Config file | `CMakeLists.txt` + `src/CMakeLists.txt` |
| Quick run command | `ctest --test-dir build --output-on-failure -R "endpoint_pick|scene_solver_drag"` |
| Full suite command | `ctest --test-dir build --output-on-failure` |

### Phase Requirements → Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|---|---|---|---|---|
| GZM-01 | Active-sketch line midpoint anchor (single + multi midpoint avg) | unit/integration (native C) | `ctest --test-dir build --output-on-failure -R endpoint_pick` | ✅ (extend `src/tests/endpoint_pick_test.c`) |
| GZM-02 | Eligible line drag translates A/B rigidly | unit/integration (native C) | `ctest --test-dir build --output-on-failure -R endpoint_pick` | ✅ (extend `src/tests/endpoint_pick_test.c`) |
| GZM-03 | Guardrails for non-eligible selections; vertex-mode unaffected | unit/integration (native C) | `ctest --test-dir build --output-on-failure -R endpoint_pick` | ✅ (extend `src/tests/endpoint_pick_test.c`) |
| GZM-04 | One drag → one coherent undo/redo restoring exact endpoints | unit/integration (native C) | `ctest --test-dir build --output-on-failure -R endpoint_pick` | ✅ (extend `src/tests/endpoint_pick_test.c`) |

### Sampling Rate
- **Per task commit:** `ctest --test-dir build --output-on-failure -R "endpoint_pick|scene_solver_drag"`
- **Per wave merge:** `ctest --test-dir build --output-on-failure`
- **Phase gate:** Full suite green before `/gsd-verify-work`

### Wave 0 Gaps
- [ ] Add new Phase-29-focused test cases into `src/tests/endpoint_pick_test.c` for GZM-01..GZM-04 coverage details above.
- [ ] If grouped undo command type is introduced, add direct command-contract roundtrip test (similar style to existing endpoint command contract tests).

## Sources

### Primary (HIGH confidence)
- `src/app.c` — gizmo drag lifecycle, mode gates, release-boundary undo capture.
- `src/gizmo/gizmo.h` — current center computation and mode behavior.
- `src/ecs/ecs_scene.h` — endpoint apply/sync logic, sketch scope guards, solver drag feasibility.
- `src/undo_redo.h` — command model and stack behavior.
- `src/undo_redo_exec.h` — command replay semantics and drag record helpers.
- `src/tests/endpoint_pick_test.c` — endpoint/undo contracts and regression style.
- `src/tests/scene_solver_drag_test.c` — solver drag feasibility and rollback tests.
- `.planning/phases/29-active-sketch-line-gizmo-endpoint-authority/29-CONTEXT.md` — locked decisions D-01..D-11.
- `.planning/REQUIREMENTS.md` — GZM-01..GZM-04 requirement contracts.
- `.planning/ROADMAP.md` — Phase 29 scope and boundary.
- `.planning/STATE.md` — milestone/phase continuity.
- `.planning/config.json` — `workflow.nyquist_validation=true` (Validation Architecture required).

## Metadata

**Confidence breakdown:**
- Standard stack: **HIGH** — based on in-repo architecture and active build/test wiring.
- Architecture: **HIGH** — direct evidence from current drag and endpoint code paths.
- Pitfalls: **HIGH** — derived from concrete replay/center/apply behavior in source.

**Research date:** 2026-04-09  
**Valid until:** 2026-05-09

