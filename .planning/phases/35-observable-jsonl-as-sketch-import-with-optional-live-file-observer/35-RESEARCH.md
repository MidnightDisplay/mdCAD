# Phase 35: observable-jsonl-as-sketch-import-with-optional-live-file-observer - Research

**Researched:** 2026-04-12  
**Domain:** mdCAD JSONL import pipeline + sketch/script lifecycle + optional file observation  
**Confidence:** HIGH

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- **D-01:** JSONL-as-sketch import creates the observer link with `Observe file` ON by default.
- **D-02:** If observer-link setup fails during import, import fails transactionally (no partially-created sketch left behind).
- **D-03:** Observer settings persist per linked sketch across save/load, including observe toggle and rate-limit value.
- **D-04:** Linked re-parse is authoritative: source JSONL fully replaces current linked sketch content.
- **D-05:** Re-parse removes user-added geometry/constraints as part of that authoritative replacement.
- **D-06:** On parse/read failure, keep the last good sketch state and surface the failure in observer messages.
- **D-07:** Place observer controls on the sketch entity in Entity Inspector when sketch is inactive.
- **D-08:** When sketch is active, move controls to the top of Active Sketch Workspace and hide editable duplicates from Entity Inspector.
- **D-09:** Observer warning/error area shows the two most recent messages.
- **D-10:** Use hardcoded max retries = 50 for locked-file reparse attempts.
- **D-11:** Retry debounce uses the same observer rate-limit interval slider value.
- **D-12:** After retry exhaustion, auto-toggle observer OFF and require manual re-enable for continuous observation.
- **D-13:** Manual `Re-parse now` remains available even when observer is auto-disabled by retry exhaustion.
- **D-14:** Script is authoritative between re-parse events; on successful re-parse, sketch is fully recreated from JSONL and script follows the recreated sketch state.
- **D-15:** Show explicit warning near Script Editor/observer controls that script edits can be overwritten by next successful re-parse while observer linkage is active.
- **D-16:** Sketch label metadata contract is strict:
  - `Label.name` = JSONL filename (without extension)
  - `Label.description` = full JSONL path
  - relinking to another file updates both fields accordingly
- **D-17:** `Line3D` -> sketch line, `Arc3D` -> sketch arc, `Circle3D` -> full-circle sketch arc, `Point3D` -> sketch point via sketch APIs.
- **D-18:** `PolyLine3D` and `Polygon3D` are converted into separate sketch line segments.
- **D-19:** Mesh entries are ignored completely in JSONL-as-sketch import path.
- **D-20:** Imported sketch geometry remains unconstrained (no automatic constraint synthesis).

### the agent's Discretion
- Exact ECS shape of `JsonlObserverComponent` and where link metadata is physically stored, as long as D-01..D-03 and D-10..D-13 behavior is preserved.
- Exact warning text format/layout for overwrite messaging, provided D-15 remains explicit and visible.
- Exact implementation details for sketch replacement transaction internals, provided D-04..D-06 deterministic behavior is preserved.

### Deferred Ideas (OUT OF SCOPE)
None — discussion stayed within phase scope.
</user_constraints>

## Summary

This phase should be implemented as a **new JSONL-to-sketch import pipeline**, not as an extension of the existing JSONL geometry-log root import. Current code already has the right primitives: geometry parsing in `jsonl_loader.h`, sketch creation APIs in `ecs_scene.h` (`scene_add_sketch`, `scene_add_*_to_sketch`), script regeneration hooks (`scene_script_reemit_for_sketch`), and active/inactive control relocation patterns in `ui_entity_inspector.h` (solver controls move between inspector and Active Sketch Workspace). The safest plan is to reuse these primitives and add one sketch-scoped observer component persisted through `scene_serializer.h`.

The current JSONL importer (`jsonl_import_job.h`) is incompatible with D-17..D-20 because it creates top-level geometry entities and supports mesh import. Therefore Phase 35 needs a **new sketch-specific import job/state machine** (or a mode split with strict branching) that: flattens all entries to one sketch, converts polyline/polygon into line segments, ignores mesh, preserves per-geometry color, and performs transactional rollback if observer-link setup fails (D-02). Existing script-apply transactional patterns in `sketch_script_apply.h` are a good implementation reference for “destroy-and-recreate on success only” behavior.

Persistence is the highest-risk integration point: `SketchComp` itself is currently partially serialized (status/color/name indices only), so observer settings should be serialized as a dedicated component via `scene_serializer.h` (recommended) or by extending sketch serialization parser/writer in lockstep. Also note label limits (`LABEL_NAME_MAX=128`, `LABEL_DESC_MAX=128`) can truncate full paths; D-16 requires exact path semantics, so this likely requires increasing `LABEL_DESC_MAX` or storing canonical path in observer metadata and reflecting truncated display safely.

**Primary recommendation:** Implement a dedicated `JsonlObserverComponent` + sketch-only import/reparse transaction path, reuse existing sketch/script APIs, and serialize observer state explicitly in `scene_serializer.h`.

## Standard Stack

### Core
| Library / Module | Version | Purpose | Why Standard |
|---|---:|---|---|
| flecs (vendored) | 4.1.4 | ECS components/tags/queries for sketch + observer state | Already central world model (`ecs_world.h`) |
| cJSON (vendored) | 1.7.19 | JSONL parsing | Existing parser + quick scan already built on it |
| `src/jsonl_loader.h` | repo current | Typed JSONL geometry parsing | Canonical parser used by current import |
| `src/ecs/ecs_scene.h` | repo current | Sketch creation + geometry attach + script re-emit | Required to satisfy “real sketch with script linkage” |
| `src/scene_serializer.h` | repo current | Save/load component persistence | Required for D-03 observer state persistence |

### Supporting
| Library / Module | Version | Purpose | When to Use |
|---|---:|---|---|
| `src/jsonl_import_job.h` | repo current | Reference import state machine pattern | Reuse structure/chunking ideas, not geometry mapping logic |
| `src/ui/ui_scene_hierarchy.h` | repo current | File menu + import popup/progress flow | Add new menu action + import options |
| `src/ui/ui_entity_inspector.h` | repo current | Inactive/active sketch control placement pattern | Implement D-07/D-08 control relocation |
| `src/scripting/sketch_script_apply.h` | repo current | Transaction + rollback pattern for sketch replacement | Model reparse overwrite/rollback semantics |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|---|---|---|
| Polling file mtime + retry in frame tick | OS-native watchers (`ReadDirectoryChangesW`, etc.) | Native watcher is more complex/cross-platform risk; polling matches existing frame-driven architecture and D-10/D-11 retry semantics |
| Dedicated observer component | Extending `SketchComp` directly | Extending `SketchComp` increases serializer coupling; dedicated component keeps concern isolated |
| New sketch import job | Reusing existing JSONL job with many conditionals | Existing job assumes root entities + mesh handling; mode branching becomes brittle and high regression risk |

## Architecture Patterns

### Recommended Project Structure
```text
src/
├── components/
│   └── jsonl_observer_comp.h      # New persisted sketch-link state + last messages
├── jsonl_sketch_import_job.h      # New sketch-specific import/reparse transaction flow
├── jsonl_observer_system.h        # Tick/poll/retry/reparse orchestration
├── ui/
│   ├── ui_scene_hierarchy.h       # New File action + import options
│   └── ui_entity_inspector.h      # D-07/D-08 controls + messages
└── scene_serializer.h             # Serialize/parse observer component
```

### Pattern 1: Transactional sketch reparse (authoritative replace)
**What:** On successful parse, remove existing script-owned sketch children, recreate from JSONL, refresh metadata, re-emit script; on failure keep previous sketch unchanged.  
**When to use:** Import initialization and every observer/manual reparse (D-04..D-06, D-14).  
**Example:**
```c
// Source: src/scripting/sketch_script_apply.h (commit model pattern)
int previous_count = sketch_script_snapshot_script_children(scene, sketch, previous_children, 1024);
sketch_script_remove_children(scene, previous_children, previous_count);
if (!sketch_script_apply_model_on_sketch(scene, sketch, &model, &previous_labels, out_error)) {
    int created_count = sketch_script_snapshot_script_children(scene, sketch, created_after, 1024);
    sketch_script_remove_children(scene, created_after, created_count);
    // restore previous model/state
}
```

### Pattern 2: Sketch-native geometry creation only
**What:** Use `scene_add_point_to_sketch`, `scene_add_line_to_sketch`, `scene_add_arc_to_sketch` only.  
**When to use:** All JSONL-as-sketch conversions (D-17..D-20).  
**Example:**
```c
// Source: src/ecs/ecs_scene.h
ecs_entity_t e = scene_add_line_to_sketch(scene, sketch, a, b, color, 0.03f);
scene_refresh_sketch_metadata(scene, sketch);
scene_script_reemit_for_sketch(scene, sketch);
```

### Pattern 3: Control relocation by active sketch context
**What:** Mirror existing solver-control policy: editable controls in inspector when inactive; moved to Active Sketch Workspace when active.  
**When to use:** Observer controls/messages UI (D-07/D-08).  
**Example:**
```c
// Source: src/ui/ui_entity_inspector.h
bool suppress_solver_in_inspector = (state->active_sketch_workspace_open && state->active_sketch == e);
if (!suppress_solver_in_inspector) { ui_entity_inspector_draw_solver_section(...); }
else { igTextDisabled("Solver controls moved to Active Sketch Workspace."); }
```

### Anti-Patterns to Avoid
- **Reusing root JSONL importer path for sketch mode:** It parents entities under anchor roots and supports mesh import, conflicting with D-17..D-19.
- **Partial in-place patch of existing sketch geometry on reparse:** Violates authoritative replacement and creates stale constraints/entities.
- **Storing observer runtime state only in UI structs:** Breaks D-03 persistence.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---|---|---|---|
| Sketch/script synchronization | Custom script text mutator | `scene_script_reemit_for_sketch` flow | Existing deterministic script pipeline already integrated |
| Geometry attach semantics | Manual ECS parent/endpoint wiring everywhere | `scene_add_*_to_sketch` APIs | Ensures endpoint/scripting/solver hooks run correctly |
| Transaction rollback scaffolding | Ad-hoc delete/recreate logic | `sketch_script_apply` transaction pattern | Proven rollback semantics in repo |
| Scene persistence format | Separate sidecar observer file | `scene_serializer.h` component serialization | Keeps save/load single-source-of-truth |

**Key insight:** most failure risk is not parsing JSONL; it is preserving mdCAD’s sketch/script transactional invariants during reparse.

## Common Pitfalls

### Pitfall 1: Label description truncation breaks strict path contract
**What goes wrong:** Full JSONL path is cut at 127 chars (`LABEL_DESC_MAX`).  
**Why it happens:** `LabelComp.description` fixed-size buffer.  
**How to avoid:** Increase `LABEL_DESC_MAX` or persist canonical full path in observer component and mirror safely to label/UI.  
**Warning signs:** Imported sketch relink path mismatches displayed or persisted path.

### Pitfall 2: Mesh elements accidentally imported
**What goes wrong:** Existing job creates mesh entities in mode 0/1.  
**Why it happens:** Shared importer branch still calls mesh handling path.  
**How to avoid:** Hard gate: JSONL-as-sketch path skips `JSONL_GEOM_MESH` entirely (D-19).  
**Warning signs:** New sketch contains triangles/mesh entities after import.

### Pitfall 3: Observer retry loop never disables
**What goes wrong:** Locked file causes endless retries/spam.  
**Why it happens:** Missing hard stop at 50 attempts.  
**How to avoid:** Persist retry counter and force `observe=false` at exhaustion (D-12), keep manual reparse active (D-13).  
**Warning signs:** Continuous error churn while observe toggle appears ON.

### Pitfall 4: Script edits silently lost without warning
**What goes wrong:** User edits script then observer reparse overwrites unexpectedly.  
**Why it happens:** No visible D-15 warning near script/observer controls.  
**How to avoid:** Always show explicit overwrite warning when sketch has active JSONL link.  
**Warning signs:** User reports “Apply worked, then reverted on file update.”

## Code Examples

### Add sketch and sketch geometry with existing APIs
```c
// Source: src/ecs/ecs_scene.h
ecs_entity_t sketch = scene_add_sketch(scene, name, full_path, vec4_make(1,1,1,1));
scene_add_point_to_sketch(scene, sketch, p, color, 0.06f);
scene_add_line_to_sketch(scene, sketch, a, b, color, 0.03f);
scene_add_arc_to_sketch(scene, sketch, center, radius, start_ang, end_ang, normal, color, 0.03f);
scene_refresh_sketch_metadata(scene, sketch);
scene_script_reemit_for_sketch(scene, sketch);
```

### Serialize a new sketch-scoped component
```c
// Source pattern: src/scene_serializer.h
if (jsonl_observer) {
  json_builder_append(b, "\"jsonl_observer\": { ... }");
}
// parse side:
else if (strcmp(comp_key, "jsonl_observer") == 0) {
  if (!json_parse_jsonl_observer(p, ent)) return false;
}
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|---|---|---|---|
| JSONL imports as top-level geometry log tree | JSONL-as-sketch with script linkage + optional observer | Phase 35 | Enables deterministic overwrite/reparse workflows |
| Inspector-only controls | Contextual relocation between inspector and active workspace | Existing solver UI pattern (already in repo) | Reuse for observer controls to reduce UX inconsistency |
| Non-persistent transient import options | ECS + scene serialization persistence | Existing scene architecture | Required for D-03 |

**Deprecated/outdated:**
- Treating JSONL sketch import as equivalent to root geometry import is outdated for Phase 35 requirements.

## Open Questions

1. **Where to store full canonical file path if `Label.description` remains 128 chars?**
   - What we know: D-16 requires strict full path metadata behavior.
   - What's unclear: whether path-length increase is acceptable across serializer/tests.
   - Recommendation: store canonical path in `JsonlObserverComponent`; keep label for UX display.

2. **How should observer timing source be implemented?**
   - What we know: solver/import systems already use `sokol_time` (`stm_now`, `stm_ms`).
   - What's unclear: preferred global tick owner for observer polling (scene vs UI).
   - Recommendation: update observer from main frame path (same cadence as other incremental jobs), avoid background thread.

## Environment Availability

No external runtime/service dependency identified for this phase (code/config-only + local file IO).  
**Step 2.6: SKIPPED (no external dependencies identified).**

## Validation Architecture

### Test Framework
| Property | Value |
|---|---|
| Framework | CTest + native C executables |
| Config file | `CMakeLists.txt` (root + `src/CMakeLists.txt`) |
| Quick run command | `ctest --output-on-failure -R "script_roundtrip_tests|scene_solver_contract"` |
| Full suite command | `ctest --output-on-failure` |

### Phase Requirements → Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|---|---|---|---|---|
| P35-01 | New File action imports JSONL into a sketch (not root dump) | integration | `ctest -R jsonl_sketch_import_test --output-on-failure` | ❌ Wave 0 |
| P35-02 | Type mapping + flattening + mesh ignore + unconstrained output | unit/integration | `ctest -R jsonl_sketch_mapping_test --output-on-failure` | ❌ Wave 0 |
| P35-03 | Observer defaults/persistence/retry behavior (D-01..D-03, D-10..D-13) | unit | `ctest -R jsonl_observer_state_test --output-on-failure` | ❌ Wave 0 |
| P35-04 | Authoritative reparse replacement + failure keeps last good state | integration | `ctest -R jsonl_reparse_transaction_test --output-on-failure` | ❌ Wave 0 |
| P35-05 | Script overwrite warning + control relocation behavior | manual/UI + smoke | `ctest -R script_roundtrip_tests --output-on-failure` | ✅ (partial) |
| P35-06 | Label contract (filename/path) on import and relink | unit | `ctest -R jsonl_label_contract_test --output-on-failure` | ❌ Wave 0 |

### Sampling Rate
- **Per task commit:** `ctest --output-on-failure -R "script_roundtrip_tests|scene_solver_contract"`
- **Per wave merge:** `ctest --output-on-failure`
- **Phase gate:** Full suite green before `/gsd-verify-work`

### Wave 0 Gaps
- [ ] `src/tests/jsonl_sketch_import_test.c` — core import flow
- [ ] `src/tests/jsonl_observer_state_test.c` — persistence + retry/auto-disable
- [ ] `src/tests/jsonl_reparse_transaction_test.c` — rollback semantics
- [ ] `src/tests/jsonl_label_contract_test.c` — filename/path contract and truncation handling
- [ ] `src/CMakeLists.txt` additions for new tests + `add_test(...)`

## Sources

### Primary (HIGH confidence)
- `.planning/phases/35-observable-jsonl-as-sketch-import-with-optional-live-file-observer/35-CONTEXT.md` — locked decisions D-01..D-20
- `docs/feature-proposal/observable-jsonl-as-sketch.md` — canonical feature behavior
- `src/jsonl_loader.h` — geometry parsing capabilities and type support
- `src/jsonl_import_job.h` — existing importer behavior and mesh handling
- `src/ecs/ecs_scene.h` — sketch APIs and script re-emit hooks
- `src/ui/ui_scene_hierarchy.h` — current JSONL import UI flow
- `src/ui/ui_entity_inspector.h` — active/inactive control relocation pattern
- `src/scene_serializer.h` — persistence contract and component parse/write patterns
- `src/ecs/ecs_world.h` — component registration/access model
- `vendors/flecs/flecs.h`, `vendors/cjson/cJSON.h`, `vendors/cglm/include/cglm/version.h` — vendored dependency versions

### Secondary (MEDIUM confidence)
- `src/app.c` — Script Editor placement and warning insertion points
- `src/scripting/sketch_script_apply.h` — transaction/rollback implementation pattern

### Tertiary (LOW confidence)
- None.

## Metadata

**Confidence breakdown:**
- Standard stack: **HIGH** — all recommendations derive from current repo architecture and vendored dependencies.
- Architecture: **HIGH** — directly anchored to existing mdCAD patterns and code paths.
- Pitfalls: **MEDIUM-HIGH** — mostly code-verified, with one policy uncertainty around full-path storage/truncation strategy.

**Research date:** 2026-04-12  
**Valid until:** 2026-05-12
