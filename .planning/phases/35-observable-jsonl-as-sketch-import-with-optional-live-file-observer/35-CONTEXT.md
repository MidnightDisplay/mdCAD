# Phase 35: Observable JSONL as sketch import with optional live file observer - Context

**Gathered:** 2026-04-12
**Status:** Ready for planning

<domain>
## Phase Boundary

Add a new JSONL import flow that creates a real sketch (with script linkage) instead of top-level dump geometry, and optionally keeps that sketch linked to the source file for live re-parse.

This phase is scoped to:
- JSONL geometry-to-sketch conversion (`Point3D`, `Line3D`, `Arc3D`, `Circle3D`, `PolyLine3D`, `Polygon3D`) using sketch creation APIs
- Flattening geometry-log entries into one sketch
- Color carry-over for imported sketch geometry
- Mesh ignore behavior for this sketch-import path
- Optional per-sketch live observer controls and retry/debounce behavior

This phase does not add constraint inference from JSONL and keeps imported geometry unconstrained.

</domain>

<decisions>
## Implementation Decisions

### Observer lifecycle defaults
- **D-01:** JSONL-as-sketch import creates the observer link with `Observe file` ON by default.
- **D-02:** If observer-link setup fails during import, import fails transactionally (no partially-created sketch left behind).
- **D-03:** Observer settings persist per linked sketch across save/load, including observe toggle and rate-limit value.

### Re-parse overwrite semantics
- **D-04:** Linked re-parse is authoritative: source JSONL fully replaces current linked sketch content.
- **D-05:** Re-parse removes user-added geometry/constraints as part of that authoritative replacement.
- **D-06:** On parse/read failure, keep the last good sketch state and surface the failure in observer messages.

### UI placement and operator ergonomics
- **D-07:** Place observer controls on the sketch entity in Entity Inspector when sketch is inactive.
- **D-08:** When sketch is active, move controls to the top of Active Sketch Workspace and hide editable duplicates from Entity Inspector.
- **D-09:** Observer warning/error area shows the two most recent messages.

### File-lock retry behavior
- **D-10:** Use hardcoded max retries = 50 for locked-file reparse attempts.
- **D-11:** Retry debounce uses the same observer rate-limit interval slider value.
- **D-12:** After retry exhaustion, auto-toggle observer OFF and require manual re-enable for continuous observation.
- **D-13:** Manual `Re-parse now` remains available even when observer is auto-disabled by retry exhaustion.

### Script and metadata contract
- **D-14:** Script is authoritative between re-parse events; on successful re-parse, sketch is fully recreated from JSONL and script follows the recreated sketch state.
- **D-15:** Show explicit warning near Script Editor/observer controls that script edits can be overwritten by next successful re-parse while observer linkage is active.
- **D-16:** Sketch label metadata contract is strict:
  - `Label.name` = JSONL filename (without extension)
  - `Label.description` = full JSONL path
  - relinking to another file updates both fields accordingly

### Import conversion semantics
- **D-17:** `Line3D` -> sketch line, `Arc3D` -> sketch arc, `Circle3D` -> full-circle sketch arc, `Point3D` -> sketch point via sketch APIs.
- **D-18:** `PolyLine3D` and `Polygon3D` are converted into separate sketch line segments.
- **D-19:** Mesh entries are ignored completely in JSONL-as-sketch import path.
- **D-20:** Imported sketch geometry remains unconstrained (no automatic constraint synthesis).

### the agent's Discretion
- Exact ECS shape of `JsonlObserverComponent` and where link metadata is physically stored, as long as D-01..D-03 and D-10..D-13 behavior is preserved.
- Exact warning text format/layout for overwrite messaging, provided D-15 remains explicit and visible.
- Exact implementation details for sketch replacement transaction internals, provided D-04..D-06 deterministic behavior is preserved.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Phase and milestone contracts
- `.planning/ROADMAP.md` — Phase 35 slot and dependency sequencing after Phase 34.
- `.planning/REQUIREMENTS.md` — current milestone constraints and out-of-scope posture (no broad scope expansion).
- `.planning/PROJECT.md` — milestone framing, C-first constraints, and stability priorities.
- `.planning/STATE.md` — current continuity and handoff state.

### Feature specification (primary)
- `docs/feature-proposal/observable-jsonl-as-sketch.md` — canonical user feature contract for this phase.

### Upstream behavioral contracts to preserve
- `.planning/phases/31-script-reapply-fidelity-foundation/31-CONTEXT.md` — transactional script apply/reapply and deterministic script integrity posture.
- `.planning/phases/32-explicit-coincidence-authoring-semantics/32-CONTEXT.md` — explicit behavior over hidden fallback policy style.
- `.planning/phases/33-large-jump-robustness-and-parallel-along-parity/33-CONTEXT.md` — transactional rollback + explicit diagnostics expectations.

### Code anchors for implementation
- `src/ui/ui_scene_hierarchy.h` — current File menu JSONL import entry and import options/progress flow.
- `src/jsonl_loader.h` — JSONL geometry type parsing (`Point3D`, `Line3D`, `Arc3D`, `PolyLine3D`, `Polygon3D`, `MeshBody`).
- `src/jsonl_import_job.h` — import state machine, transforms, geometry creation mapping, and current mesh handling.
- `src/ecs/ecs_scene.h` — sketch creation and sketch geometry add APIs (`scene_add_sketch`, `scene_add_*_to_sketch`).
- `src/ui/ui_entity_inspector.h` — sketch inspector/workspace surfaces for adding observer controls.
- `src/app.c` — Script Editor and Script IO windows; warning placement integration point.
- `src/components/label_comp.h` — sketch label name/description storage for filename/path contract.
- `src/components/sketch_comp.h` — sketch-level persistent metadata patterns relevant to observer state persistence.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- Existing JSONL import UI flow already supports file picking, quick scan, options, and progress (`ui_scene_hierarchy.h`).
- Existing JSONL loader/import job already parses required geometry classes and maps to scene geometry creation (`jsonl_loader.h`, `jsonl_import_job.h`).
- Existing sketch APIs already create points/lines/arcs/circles inside sketch parentage (`ecs_scene.h`, `ui_entity_inspector.h`).
- Existing Script Editor + Script IO surfaces already react to sketch/script revision changes and can host overwrite warnings (`app.c`).

### Established Patterns
- Header-only modules with explicit status/state structs and deterministic early-return error handling.
- Transactional behavior and explicit diagnostics are preferred over silent fallback.
- Persistent per-entity user-facing metadata is represented by ECS components and serialized with scene state.

### Integration Points
- Add new File menu action for JSONL-as-sketch import while keeping existing JSONL geometry-log import path available.
- Add observer ECS component lifecycle + persistence and connect it to sketch entity UI panels.
- Hook observer-driven reparse into sketch recreation pipeline and script regeneration/revision updates.
- Wire label metadata updates at import/relink boundaries.

</code_context>

<specifics>
## Specific Ideas

- When a linked sketch is active, observer controls should be top-priority and colocated in Active Sketch Workspace for operator visibility.
- Observer failure mode is explicit and safe: stop auto-observe after lock exhaustion, but preserve manual reparse path.
- Treat source JSONL as authoritative on reparse boundaries while still allowing short-lived script edits between reparses.

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope.

</deferred>

---

*Phase: 35-observable-jsonl-as-sketch-import-with-optional-live-file-observer*
*Context gathered: 2026-04-12*
