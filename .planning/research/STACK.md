# Technology Stack

**Project:** mdCAD milestone v1.6 — Observable Flat JSONL Import for Large Geometry Dumps  
**Researched:** 2026-04-27  
**Scope:** Stack changes only for flat (non-sketch) JSONL import + large-file observer refresh

## Recommended Stack

### Core Framework

| Technology | Version | Purpose | Why |
|------------|---------|---------|-----|
| C (C11), header-inline modules | Keep current | Implement flat import + refresh pipeline | Matches current architecture; zero stack churn |
| Flecs | 4.1.4 (vendored) | Entity graph + import anchor parenting | Already central; flat mode fits existing `scene_add_anchor` + `EcsChildOf` model |
| cJSON | 1.7.19 (vendored) | JSONL line parse | Already integrated in `jsonl_loader.h`; avoid parser migration risk |
| Existing math/import helpers (`math_import.h`) | Keep current | Unit scale / rotation / shift transforms | Reuse proven transform path from current JSONL import |

### Infrastructure / Runtime Behavior

| Technology | Version | Purpose | Why |
|------------|---------|---------|-----|
| Existing polling observer (metadata + optional hash) | Keep, but optimize policy | File change detection for flat import anchor | Cross-platform, dependency-free, already integrated (`jsonl_observer_system.h`) |
| OS file metadata APIs (`GetFileAttributesExA` / `stat`) | Existing | Fast change pre-check | Already implemented and stable on Windows/macOS path |

### Required Internal Additions (no new external deps)

| Module | Purpose | Recommendation |
|--------|---------|----------------|
| `jsonl_flat_import_job.h` (new) | Large-file flat import path | Add streaming/chunked **flat** importer that creates geometry under one import anchor (no per-entry anchors) |
| `jsonl_flat_observer_system.h` (new or merged) | Flat refresh semantics | Add transactional "replace children under one anchor" semantics similar to sketch reparse contract, but without script/constraint work |
| `jsonl_loader.h` extension | Reduce huge-file overhead | Add parse mode that avoids full in-memory `jsonl_data_t` accumulation for flat import (line->apply pipeline) |
| Observer component split or mode flag | Link metadata for flat anchors | Add either `JsonlFlatObserverComp` or `import_mode` field to existing observer component |

## Concrete Stack Changes for v1.6

1. **Do not add new third-party libraries.**
   - Keep parser as cJSON for this milestone.
   - Keep observer as polling-based.
2. **Add a flat import execution path optimized for size:**
   - Avoid current multi-pass overhead (`quick_scan_mesh` + `count_lines` + full parse accumulation).
   - Prefer: lightweight pre-scan optional, then streaming create in chunks.
3. **Use single-anchor hierarchy for flat mode:**
   - Root anchor only (`scene_add_anchor`), geometry children directly parented to root.
   - Skip entry-anchor allocations to reduce entity count and parenting cost.
4. **Observer semantics for flat mode:**
   - Reuse current transactional pattern (stage new, commit swap, rollback on failure).
   - Keep interval/retry controls; preserve metadata stamping logic.

## Explicit "Do NOT Add" Notes

| Do NOT add | Why | Use instead |
|------------|-----|-------------|
| libuv/fswatch/FSEvents wrapper deps | Extra complexity + platform matrix risk | Existing polling observer + metadata debounce |
| New JSON parser (RapidJSON/simdjson/etc.) for v1.6 | Integration + memory model churn; not required for milestone | Optimize current cJSON pipeline and passes first |
| Background threaded importer in this milestone | ECS/thread-safety risk and larger test surface | Keep main-thread chunked job state machine |
| Per-entry anchor tree in flat mode | Increases entity/parenting overhead on huge logs | Single import anchor + direct children |

## Integration Touchpoints in Current Codebase

- `src/jsonl_import_job.h`
  - Add flat mode branch or extract new `jsonl_flat_import_job.h`.
  - Remove dependency on entry anchor arrays for flat path.
- `src/jsonl_loader.h`
  - Add streaming/low-retention parse APIs for huge files.
  - Avoid mandatory full line-count pass for flat import progress.
- `src/jsonl_observer_system.h` + `src/components/jsonl_observer_comp.h`
  - Add flat-anchor observer support (or mode flag).
- `src/ui/ui_scene_hierarchy.h`
  - Add "Import JSONL (Flat Large)" option or mode toggle in JSONL import dialog.
  - Add observer opt-in + refresh controls for flat anchor.
- `src/ui/ui_entity_inspector.h`
  - Observer controls for flat import anchor entities (not only sketch entities).
- `src/scene_serializer.h`
  - Persist new flat observer/link metadata.
- `src/ecs/ecs_world.h`
  - Register any new flat observer component (if split approach used).
- `src/app.c`
  - Tick flat observer system each frame alongside existing observer tick.

## Alternatives Considered

| Category | Recommended | Alternative | Why Not |
|----------|-------------|-------------|---------|
| File change detection | Polling observer | Native watcher APIs per OS | Higher platform complexity; unnecessary for v1.6 |
| Parsing strategy | cJSON line parse + streaming apply | Replace parser library | Too risky for milestone scope |
| Import hierarchy | Single anchor flat tree | Entry-per-line subtree | Too heavy for large dumps |

## Installation

```bash
# No new external packages required for v1.6 stack
# Keep current CMake + vendored deps
```

## Sources

- `src/jsonl_import_job.h` (current multi-phase import, entry anchors, parent batching)
- `src/jsonl_loader.h` (line counting + full parse accumulation + quick scan mesh)
- `src/jsonl_sketch_import_job.h` (transactional reparse semantics pattern)
- `src/jsonl_observer_system.h` and `src/components/jsonl_observer_comp.h` (observer policy and metadata model)
- `src/ui/ui_scene_hierarchy.h` and `src/ui/ui_entity_inspector.h` (current JSONL import/observer UX integration)
- `src/ecs/ecs_scene.h`, `src/ecs/ecs_world.h`, `src/scene_serializer.h`, `src/app.c` (anchor creation, component registration, serialization, tick loop)
