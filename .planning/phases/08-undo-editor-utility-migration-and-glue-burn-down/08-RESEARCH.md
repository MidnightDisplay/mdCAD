# Phase 8: Undo/Editor Utility Migration and Glue Burn-Down - Research

**Researched:** 2026-03-27  
**Domain:** Undo/redo + editor utility math migration to cglm-backed helpers; temporary glue reduction  
**Confidence:** HIGH

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
### Transform migration policy
- **D-01:** Phase 8 is correctness-first for undo/editor utility math: prefer mathematically correct behavior even when this changes prior behavior.
- **D-02:** User-visible behavior shifts are allowed when mathematically justified and explicitly documented in validation evidence.
- **D-03:** Correctness-driven changes must remain scoped to undo/editor utility migration surfaces.

### Glue burn-down scope
- **D-04:** Use an aggressive glue-reduction posture in Phase 8: remove most temporary migration glue unless an immediate blocker appears.
- **D-05:** If a glue removal causes regressions that are not safely resolved in-phase, carry-over to Phase 9 is allowed when explicitly documented with rationale and impact.
- **D-06:** Glue burn-down must still respect runtime safety; removals must be traceable to proven non-consumers or replaced by migrated cglm-backed paths.

### Validation depth and evidence
- **D-07:** Validation gate for Phase 8 remains light: compile gate plus focused undo/editor parity workflows.
- **D-08:** Mandatory focused workflows are:
  1. Undo/redo transform edits
  2. Gizmo vertex edit workflow
  3. Inspector edit workflow
- **D-09:** Validation artifacts must call out correctness-driven behavior deltas and any explicitly deferred follow-up.

### Scope control
- **D-10:** No new editor/undo feature capabilities are in scope for Phase 8.

### the agent's Discretion
- Exact helper API names/signatures and placement under `src/math/` for undo/editor migration.
- Exact glue inventory and removal sequencing in plans (`08-01`..`08-03`), as long as locked decisions above are respected.
- Exact shape of focused validation evidence format, provided mandatory workflows and behavior-delta notes are covered.

### Deferred Ideas (OUT OF SCOPE)
- Detailed editor UX contract tuning (beyond migration parity/correctness behavior) was not expanded in this discussion.
- Any unresolved regressions from aggressive glue removal may be explicitly deferred to Phase 9 with evidence, instead of blocking Phase 8 close by default.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| TAIL-03 | User can perform undo/redo and editor utility transform workflows with cglm-backed math and behavior parity to the pre-migration user experience. | Defines in-scope touchpoints (`undo_redo_exec`, inspector, gizmo vertex mode), prescriptive migration cutline, and mandatory parity workflows + delta evidence. |
| TRED-01 | Runtime-critical migrated paths no longer rely on removable temporary thin-entrypoint migration glue. | Provides glue inventory, removal priority, and proof requirements for runtime consumer safety before deletion/carry-over. |
</phase_requirements>

## Summary

Phase 8 should be planned as a **surgical migration + burn-down** phase, not a feature phase. Runtime undo/edit behavior is currently centralized in `src/undo_redo_exec.h` (record/apply/unapply), inspector command recording in `src/ui/ui_entity_inspector.h`, hierarchy undo/redo UI wiring in `src/ui/ui_scene_hierarchy.h`, and geometry-mode vertex interaction math in `src/gizmo/gizmo_vertex_mode.h`.

The highest-risk migration touchpoint is **vertex edit math flow** (`gizmo_vertex_mode_get_center`, `gizmo_vertex_mode_apply_delta`) because it still relies on legacy `math3d` primitives (`mat4_mul_point`, `vec3_add`) while interacting with already-migrated interaction helpers (`mdcad_interaction_world_delta_to_local`). This is the most concrete Phase 8 glue seam to close.

Primary planning posture: preserve command semantics and dirty-flag invariants, migrate scoped math touchpoints to cglm-backed helpers, and aggressively remove safe glue only where runtime non-consumers are proven. Keep validation light but mandatory around the three workflows in D-08, with explicit correctness-delta notes.

**Primary recommendation:** Execute Phase 8 in three passes: (1) migrate undo/editor utility math touchpoints and helper boundaries, (2) remove proven-safe glue with evidence-backed cutlines, (3) run compile + focused workflow validation and document deltas/deferred carry-over.

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| cglm (vendored) | 0.9.6 | Math backend for migrated helper surfaces | Locked in `.planning/STATE.md` and enforced by `src/math/cglm_entry.h` |
| math entrypoint (`src/math/cglm_entry.h`) | in-repo | Contract gate (RH_ZO clip control/layout assertions) | Established migration choke point across prior phases |
| Undo command system (`src/undo_redo.h` + `src/undo_redo_exec.h`) | in-repo | Canonical undo/redo command model and apply/unapply logic | Runtime-critical behavior path; must be migrated in place |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| Interaction helpers (`src/math/math_interaction.h`) | in-repo | Existing cglm-backed world/screen ray + world→local delta bridge | Reuse as pattern/reference for Phase 8 helper design |
| CImGui UI path (`src/ui/ui_entity_inspector.h`, `src/ui/ui_scene_hierarchy.h`) | in-repo | Undo command recording + user-triggered undo/redo entrypoints | Required boundaries for mandatory D-08 workflows |
| Geometry edit mode (`src/gizmo/gizmo_vertex_mode.h`) | in-repo | Vertex transform math and local/world conversion in editor utility flow | Mandatory workflow surface for gizmo vertex validation |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| In-place migration of current command/UI/gizmo boundaries | New undo subsystem or editor utility rewrite | Out of scope (D-10), high regression risk, unnecessary for TAIL-03/TRED-01 |
| Thin cglm-backed helper additions under `src/math/` | Broad compatibility facade wrapping all legacy math | Contradicts thin-boundary posture from prior phases and increases glue debt |

**Installation:** no new external dependencies required; all phase libraries are vendored/in-repo.

**Version verification:** `cglm 0.9.6` verified from project state and current vendored contract (`src/math/cglm_entry.h` + `.planning/STATE.md`).

## Migration Cutlines (What Changes, What Must Not)

### In Scope (must change)
- `src/gizmo/gizmo_vertex_mode.h`
  - Replace legacy point transform/addition call paths (`mat4_mul_point`, `vec3_add`) with cglm-backed helper calls.
  - Keep behavior of selection/center/update semantics unchanged unless correctness requires change (then document).
- `src/undo_redo_exec.h`
  - Keep apply/unapply semantics, but migrate any scoped math-touch helper usage introduced for correctness.
  - Preserve `t->dirty`, `ecs_world_mark_descendants_dirty`, `r->instance_dirty` invariants across command types.
- `src/ui/ui_entity_inspector.h`
  - Maintain drag-start/drag-end undo capture semantics while updating any scoped helper calls.
- `src/ui/ui_scene_hierarchy.h`
  - Maintain undo/redo menu/shortcut wiring; only migration-related updates.

### Out of Scope (must not expand)
- New undo command types, new editor interaction modes, or broader UX redesign (D-10).
- Broad repo-wide `math3d.h` retirement.
- Phase 9 validation/perf expansion tasks.

## Architecture Patterns

### Recommended Project Structure
```text
src/
├── math/
│  ├── cglm_entry.h                 # locked backend contract
│  ├── math_interaction.h           # existing migrated interaction helpers
│  └── [phase-8 helper].h           # thin undo/editor utility helper(s), if added
├── undo_redo.h                     # command payload/types
├── undo_redo_exec.h                # apply/unapply + snapshot execution
├── gizmo/gizmo_vertex_mode.h       # geometry edit utility touchpoints
└── ui/
   ├── ui_entity_inspector.h        # inspector edit capture -> undo command recording
   └── ui_scene_hierarchy.h         # edit menu undo/redo invocation
```

### Pattern 1: Centralized command apply/unapply with invariant dirty propagation
**What:** Keep all mutation behavior centralized in `undo_apply_command`/`undo_unapply_command`, with explicit transform and render dirty updates.  
**When to use:** Every undo command path touched by migration.  
**Example:**
```c
// Source: src/undo_redo_exec.h
t->position = cmd->data.set_vec3.new_value;
t->dirty = true;
ecs_world_mark_descendants_dirty(w, e);
if (r) r->instance_dirty = true;
```

### Pattern 2: Drag lifecycle capture for undo recording
**What:** Capture start state on `igIsItemActivated()` and record command once on `igIsItemDeactivatedAfterEdit()`.  
**When to use:** Inspector and gizmo workflows to avoid per-frame command spam.  
**Example:**
```c
// Source: src/ui/ui_entity_inspector.h
if (igIsItemActivated()) { state->drag_start_position = t->position; }
if (igIsItemDeactivatedAfterEdit()) {
    undo_cmd_set_position(state->undo_redo, e, state->drag_start_position, t->position);
}
```

### Pattern 3: Thin helper bridging (cglm internals, legacy outward types)
**What:** Use cglm internally but keep existing external `vec3_t`/`mat4_t` call contracts in runtime modules.  
**When to use:** Phase 8 helper additions under `src/math/`.  
**Example:**
```c
// Source pattern: src/math/math_interaction.h
vec3s s = mdcad_interaction_vec3s_from_legacy(v);
// ... glms_* ops ...
return vec3_make(out.raw[0], out.raw[1], out.raw[2]);
```

### Anti-Patterns to Avoid
- **Scattered ad hoc math rewrites in UI files:** move shared math to `src/math/` helper boundary.
- **Changing undo command semantics during migration:** migration is correctness-first math, not command-model redesign.
- **Dropping dirty-flag side effects:** this silently breaks render/transform propagation and appears as stale scene bugs.

## Implementation Approach (Prescriptive)

1. **Inventory and patch math touchpoints first** in `gizmo_vertex_mode.h`, `undo_redo_exec.h`, and inspector transform edit paths.
2. **Adopt/extend thin cglm-backed helper(s)** under `src/math/` only where repeated math exists.
3. **Burn down glue aggressively** where runtime non-consumers are proven:
   - prioritize deprecated interaction helpers in `math3d.h` with no runtime consumers,
   - keep harness-only legacy comparators if still needed for Phase 9.
4. **Add explicit carry-over notes** for any glue that cannot be removed safely this phase (D-05).
5. **Run focused validation workflows** and record any mathematically justified behavior deltas.

## Plan Guidance for 08-01 .. 08-03

### 08-01: Migrate undo/editor utility math touchpoints to cglm-backed helpers
- Touchpoints: `gizmo_vertex_mode.h` (center + delta application), scoped helper usage in `undo_redo_exec.h` and inspector edit flows.
- Proof of completion:
  - No legacy `mat4_mul_point`/direct legacy-only math in migrated Phase 8 touchpoints.
  - Compile gate green.

### 08-02: Remove safe thin-entrypoint glue and update call-site boundaries
- Removal candidates:
  - deprecated interaction helper call paths in `math3d.h` with no runtime consumers,
  - local math glue now superseded by `src/math/` helper usage.
- Safety proof:
  - grep evidence for runtime non-consumption,
  - if harness still requires legacy path, keep as explicit carry-over to Phase 9 with rationale.

### 08-03: Validate edit/undo parity and document migration deltas
- Mandatory workflows (D-08):
  1. Undo/redo transform edits
  2. Gizmo vertex edit workflow
  3. Inspector edit workflow
- Required outputs:
  - checklist/report artifact,
  - explicit “correctness delta vs prior behavior” section,
  - deferred carry-over section (if any).

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| World/local transform math for editor drags | New custom inverse/unproject math in UI/gizmo files | Existing/extended cglm-backed `src/math/` helpers | Avoids divergent math semantics and repeated numeric edge-case bugs |
| Undo stack semantics | New parallel undo history path | Existing `undo_redo_t` command model | Existing model already integrated across scene hierarchy + inspector |
| Dirty-flag propagation logic | New per-call-site partial flag updates | Existing apply/unapply invariant pattern in `undo_redo_exec.h` | Prevents stale transforms/render instances after undo/redo |

**Key insight:** In this phase, the risk is not missing algorithms; it is **duplicating or diverging** established undo/editor invariants while migrating math.

## Runtime State Inventory

| Category | Items Found | Action Required |
|----------|-------------|------------------|
| Stored data | None — verified by phase scope/docs + code inspection: undo history is in-memory (`undo_redo_t`), not persisted datastore state in-scope. | None (code edit only) |
| Live service config | None — no external service/UI-hosted config touched by Phase 8 runtime files. | None |
| OS-registered state | None — no task/service registration changes implied by undo/editor math migration. | None |
| Secrets/env vars | None — no phase touchpoints read/write secret keys or env-var names for undo/editor math behavior. | None |
| Build artifacts | Existing build directories (`build-vulkan`) may require rebuild after code edits; no rename/install artifact migration identified. | Rebuild targets during validation |

## Common Pitfalls

### Pitfall 1: Apply/unapply asymmetry
**What goes wrong:** Redo path is migrated but undo path keeps legacy math/side effects (or vice versa).  
**Why it happens:** `undo_apply_command` and `undo_unapply_command` are long switch blocks and easy to edit unevenly.  
**How to avoid:** Any command touched in one function must be reviewed in the paired function.  
**Warning signs:** Undo restores wrong values or leaves render/transform stale.

### Pitfall 2: Dirty-flag regression masked as math regression
**What goes wrong:** Behavior appears “off” after migration because flags were missed, not because math is wrong.  
**Why it happens:** Transform and render invalidation is manual (`t->dirty`, descendants dirty, `instance_dirty`).  
**How to avoid:** Treat flag updates as non-optional invariants in each edited command path.  
**Warning signs:** Values in inspector change but viewport does not update until another action.

### Pitfall 3: Vertex workflow drift due mixed helper usage
**What goes wrong:** Gizmo vertex drags accumulate subtle offsets or inconsistent center/pick behavior.  
**Why it happens:** Mixing legacy `mat4_mul_point` paths with cglm-backed world/local conversion can introduce inconsistencies.  
**How to avoid:** Move center/delta math in vertex mode to one helper family.  
**Warning signs:** Vertex drag feels slightly “off-axis” or undo restores unexpected local coordinates.

### Pitfall 4: Over-aggressive glue deletion without consumer proof
**What goes wrong:** Removing helper paths breaks harness/runtime paths that still consume them indirectly.  
**Why it happens:** “Unused” is assumed from local scope without repo-wide call-site check.  
**How to avoid:** Require grep-based consumer proof before deletion; carry to Phase 9 when uncertain (D-05).  
**Warning signs:** Build errors in non-primary targets or validation harness regressions after cleanup.

## Code Examples

Verified in-repo patterns for this phase:

### Undo command recording at drag end
```c
// Source: src/app.c
undo_cmd_set_position(&state.undo_redo, e,
    state.gizmo_drag_start_positions[i], new_pos);
```

### Geometry vertex undo capture with old/new arrays
```c
// Source: src/app.c
undo_cmd_set_geometry_vertices(&state.undo_redo, entity,
    state.gizmo_drag_vertex_indices,
    state.gizmo_drag_start_vertices,
    new_positions, count);
```

### Existing cglm-backed world->local conversion utility
```c
// Source: src/math/math_interaction.h
vec3_t local_delta = mdcad_interaction_world_delta_to_local(world_matrix, world_delta);
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Legacy `math3d` interaction helpers (`ray_from_screen`, `ray_axis_closest_t`, `ray_plane_intersect`) in runtime paths | cglm-backed interaction helpers in `src/math/math_interaction.h` | Phase 4 | Runtime interaction already migrated; Phase 8 should finish adjacent editor/undo utility seams |
| Staged migration with heavy hotspot gates only | Long-tail light gate + targeted evidence | Phases 6-7 | Phase 8 should keep compile + focused workflow evidence posture |

**Deprecated/outdated:**
- Introducing new editor capabilities in migration phase.
- Keeping temporary glue “just in case” without proving active consumers.

## Open Questions

1. **Exact Phase 8 helper API shape under `src/math/`**
   - What we know: thin helper boundary is required; facade-style expansion is not.
   - What's unclear: whether one dedicated `math_undo_editor.h` helper is needed versus reusing `math_interaction.h` only.
   - Recommendation: start by reusing existing helper(s); add a narrow new helper only if duplication appears in >1 file.

2. **How much deprecated `math3d` interaction glue can be removed now**
   - What we know: runtime consumers in Phase 8 scope are already on `mdcad_interaction_*` for ray/plane/axis operations.
   - What's unclear: whether harness/Phase 9 compare flows still need legacy symbols.
   - Recommendation: remove runtime usage now; keep harness-only legacy paths if required, documenting explicit Phase 9 carry-over.

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| cmake | Compile/validation targets | ✓ | 4.3.0 | — |
| python | Optional checklist/evidence tooling | ✓ | 3.13.12 | Manual checklist/report update |
| node | Existing scripts/tooling ecosystem | ✓ | v25.8.1 | — |
| ninja | Optional native build path | ✗ | — | Use existing `build-vulkan` via `cmake --build` |
| build-vulkan directory | Practical compile gate in this workspace | ✓ | configured | Reconfigure if missing |
| Vulkan SDK env (`VULKAN_SDK`) | Windows Vulkan build context | ✓ | 1.4.341.1 (path env) | Existing preconfigured build dir |

**Missing dependencies with no fallback:**
- None identified for Phase 8 execution in this environment.

**Missing dependencies with fallback:**
- `ninja` (fallback: `cmake --build build-vulkan --config Release ...`).

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | Native build/target validation + focused manual workflow checks |
| Config file | `src/CMakeLists.txt` |
| Quick run command | `cmake --build build-vulkan --config Release --target mdcad_math_harness` |
| Full suite command | `cmake --build build-vulkan --config Release --target math-validation` |

### Phase Requirements → Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| TAIL-03 | Undo/redo + editor utility transform workflows operate with migrated cglm-backed math and parity-safe behavior | compile + targeted manual integration | `cmake --build build-vulkan --config Release --target mdcad_math_harness` | ⚠️ Partial (compile exists; Phase 8 targeted checklist/report needs update/addition) |
| TRED-01 | Runtime-critical migrated paths no longer rely on removable temporary glue | static cutline audit + compile validation | `cmake --build build-vulkan --config Release --target mdcad_math_harness` | ⚠️ Partial (needs explicit glue inventory + non-consumer proof artifact) |

### Sampling Rate
- **Per task commit:** run quick compile command.
- **Per wave merge:** run `math-validation` and execute mandatory D-08 workflow checklist.
- **Phase gate:** compile green + mandatory workflow evidence + documented correctness deltas/carry-over.

### Wave 0 Gaps
- [ ] Add Phase 8 focused checklist/report artifact under `.planning/phases/08-undo-editor-utility-migration-and-glue-burn-down/evidence/`.
- [ ] Add explicit glue inventory artifact (removed / retained / deferred with rationale).
- [ ] Add/extend QUICKSTART section for Phase 8 workflow commands and mandatory manual steps.

## Sources

### Primary (HIGH confidence)
- `.planning/phases/08-undo-editor-utility-migration-and-glue-burn-down/08-CONTEXT.md` — locked decisions, scope, validation workflow mandates.
- `.planning/REQUIREMENTS.md` — `TAIL-03`, `TRED-01` contract text.
- `.planning/ROADMAP.md` — Phase 8 success criteria and plan slots.
- `.planning/PROJECT.md`, `.planning/STATE.md` — milestone constraints and active routing.
- Runtime code: `src/undo_redo.h`, `src/undo_redo_exec.h`, `src/ui/ui_entity_inspector.h`, `src/ui/ui_scene_hierarchy.h`, `src/gizmo/gizmo_vertex_mode.h`, `src/math/cglm_entry.h`, `src/math/math_interaction.h`, `src/math3d.h`, `src/app.c`.
- Validation context: `.planning/codebase/TESTING.md`, `.planning/codebase/CONCERNS.md`, `docs/QUICKSTART.md`, `.planning/config.json`.

### Secondary (MEDIUM confidence)
- `.planning/phases/04-interaction-math-and-api-expansion/04-CONTEXT.md` — prior helper-boundary and legacy-retirement precedent.
- `.planning/phases/06-serializer-and-save-load-long-tail-migration/06-CONTEXT.md` and `.planning/phases/07-import-pipeline-long-tail-migration/07-CONTEXT.md` — long-tail migration and light-gate evidence precedent.
- `.planning/phases/07-import-pipeline-long-tail-migration/07-RESEARCH.md` — section and process pattern for v1.1 long-tail phases.

### Tertiary (LOW confidence)
- None.

## Metadata

**Confidence breakdown:**
- Standard stack: **HIGH** — all recommendations align with locked in-repo stack and current code.
- Architecture: **HIGH** — based on concrete call-site and invariant inspection in runtime files.
- Pitfalls: **HIGH** — derived from current undo/gizmo/UI execution flow and known testing fragility.

**Research date:** 2026-03-27  
**Valid until:** 2026-04-26 (30 days; repo-local migration scope, stable stack)
