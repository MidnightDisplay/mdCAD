# Architecture Patterns

**Domain:** mdCAD v1.6 — Observable Flat JSONL Import for Large Geometry Dumps  
**Researched:** 2026-04-27  
**Confidence:** HIGH (repo-code verified)

## Recommended Architecture

Add **optional observer-driven refresh** to the existing flat JSONL import path **without changing default import behavior**.

### Entry Points

- Existing menu/flow stays primary:
  - `src/ui/ui_scene_hierarchy.h`
    - `Import JSONL Geometry Log...`
    - `jsonl_import_job_start(...)`
- Add one option in the existing JSONL import popup:
  - `Attach observer link for refresh` (default **OFF** to preserve behavior)

### Existing Modules/Files Involved (Integration Points)

1. **Import pipeline**
   - `src/jsonl_loader.h` (parse, quick scan)
   - `src/jsonl_import_job.h` (chunked parse/create/parent)
   - `src/ui/ui_scene_hierarchy.h` (import options + progress)
2. **Scene/runtime**
   - `src/ecs/ecs_scene.h` (`scene_add_anchor`, batch parenting, `ImportPending` behavior)
   - `src/app.c` (per-frame tick location, currently ticks sketch observer)
3. **Observer patterns to reuse**
   - `src/components/jsonl_observer_comp.h`
   - `src/jsonl_observer_system.h`
   - `src/jsonl_sketch_import_job.h` (transactional reparse pattern)
4. **Persistence**
   - `src/ecs/ecs_world.h` (component registration/get/set)
   - `src/scene_serializer.h` (component save/load)

## New Components/State vs Existing Flow Changes

## New (recommended)

### 1) New component for flat imports
Use a **new component** (e.g. `JsonlFlatObserverComp`) attached to the imported flat root anchor entity, not `SketchComp`.

Store:
- source path
- observe toggle / interval / retry state
- transform/import settings needed for replay (scale/rotation/shift, color mode)
- source stamp (size/mtime/hash)
- minimal message history

Why new component: keeps sketch observer untouched and avoids regressions in Phase 35 behavior.

### 2) New flat observer system
Add `jsonl_flat_observer_system.h`:
- query: entities with flat observer component
- source change detection + retry/debounce policy
- schedule/drive refresh pipeline

### 3) New transactional flat refresh pipeline
Add a refresh path that preserves root entity identity:
- parse changed JSONL
- stage new subtree (entry anchors + geometry)
- commit only on success:
  - delete old subtree
  - reparent staged subtree under same root
- rollback on failure:
  - delete staged subtree only
  - keep last-good live subtree

This mirrors sketch transactional reparse semantics but for flat ECS trees.

## Modify existing flow (minimal, safe)

- `ui_scene_hierarchy.h`:
  - add observer opt-in checkbox in existing JSONL popup
  - on successful import, attach flat observer component to `job->root_entity`
- `app.c`:
  - call `jsonl_flat_observer_system_tick(...)` near existing `jsonl_observer_system_tick(...)`
- `scene_serializer.h` + `ecs_world.h`:
  - register/persist new flat observer component (new JSON key; do not alter existing `jsonl_observer` contract)

## Data Flow (Flat Import + Optional Observer Refresh)

1. User imports JSONL (existing flat flow).
2. `jsonl_import_job` creates root + entries + geometry as today.
3. If observer opt-in:
   - attach flat observer component to root entity.
4. Frame tick:
   - observer system checks source stamp.
   - unchanged -> no-op.
   - changed -> run transactional flat refresh job.
5. Refresh success:
   - subtree atomically replaced under same root.
6. Refresh failure:
   - last-good subtree preserved, message pushed, retry policy applied.

## Anti-Patterns to Avoid

- Reusing sketch observer component/system directly for non-sketch roots.
- In-place mutation of old flat subtree during parse (non-transactional partial corruption risk).
- Changing current JSONL import defaults (must remain unchanged when observer not enabled).
- Coupling observer runtime state to UI-only structs.

## Suggested Build Order (Low-Risk)

1. **Phase A — Persistence scaffolding + component only**
   - Add `JsonlFlatObserverComp` registration + serializer read/write.
   - No runtime behavior yet.
2. **Phase B — Hook import completion metadata**
   - Wire optional checkbox in existing JSONL popup.
   - Attach new component to imported root on success.
   - Keep default OFF.
3. **Phase C — Transactional refresh engine (manual only)**
   - Implement flat refresh transaction API:
     - `jsonl_flat_reparse_transactional(scene, root, observer)`
   - Trigger via manual button in inspector for selected root.
4. **Phase D — Automatic observer tick**
   - Add `jsonl_flat_observer_system_tick` in `app.c`.
   - Enable debounce/retry/auto-disable policy.
5. **Phase E — Scale/perf hardening + tests**
   - Large fixture tests for high entity counts.
   - Validate stable totals/no leaks/no duplicate accumulation.
   - Keep existing sketch observer and plain import tests green.

## Maintainability/Compatibility Contract

- Existing `Import JSONL Geometry Log...` behavior remains identical unless observer opt-in is enabled.
- Existing sketch observer (`JsonlObserverComp` + `jsonl_observer_system_tick`) remains isolated and unchanged.
- New flat observer architecture is additive and root-scoped, minimizing regression surface.
