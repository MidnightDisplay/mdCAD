# Phase 37: Anchor-Scoped Flat Ingest - Research

**Researched:** 2026-04-27  
**Domain:** JSONL flat import hierarchy construction in ECS scene graph  
**Confidence:** HIGH

## User Constraints (from CONTEXT.md)

### Locked Decisions
- **D-01:** Keep Phase 36 behavior constraints intact: the dedicated `Import JSONL (Flat Large Dump)...` path remains distinct, and existing `Import JSONL Geometry Log...` / `Import JSONL as Sketch` flows are not repurposed.
- **D-02:** Flat ingest uses a two-level hierarchy: `root anchor -> per-entry anchors -> geometry entities`.
- **D-03:** Do not create per-entry anchors when an entry yields zero importable geometry.
- **D-04:** Root anchor label contract:
  - `Label.name` = JSONL filename stem (no extension)
  - `Label.description` = full source path
- **D-05:** Entry anchor label contract:
  - `Label.name` = entry `Name`
  - `Label.description` = entry `Description`
- **D-06:** Re-importing the same source file keeps previous anchors and creates a fresh root anchor per run.
- **D-07:** Root-name collisions are resolved with numeric suffixes (`name`, `name (2)`, `name (3)`).
- **D-08:** Successful flat import keeps the current selection unchanged (no auto-select of new root or geometry).

## Phase Requirements

| ID | Description | Research Support |
|---|---|---|
| FIMP-03 | Flat import creates a single root anchor and imports entries as plain non-sketch scene entities under that anchor. | Enforce root/entry anchor contracts in `jsonl_import_job_tick` parse-complete + create + parent stages; keep plain `scene_add_*` geometry path; add rerun root-name suffixing. |

## Summary

Current implementation already uses the correct core pipeline for flat import (`jsonl_import_job.h`): parse -> create entities -> parent via `EcsChildOf`. It already creates root + entry anchors and plain geometry (non-sketch), but it violates key Phase 37 contracts: root naming/description, zero-geometry entry anchors, and rerun name collision suffixing.

Best minimal-scope implementation is to modify only flat ingest internals in `jsonl_import_job.h` (plus focused tests), while leaving Phase 36 UI contract in `ui_scene_hierarchy.h` unchanged. This preserves the dedicated menu flow and sync/async behavior already verified in Phase 36.

## Reusable Implementation Assets & Integration Points

- Root/entry anchor creation exists in `jsonl_import_job.h` parse-complete stage.
- Plain non-sketch geometry path already exists via `scene_add_point/line/arc/polyline/polygon/mesh/triangle`.
- Batch parenting already exists in deferred parent stage using `ecs_add_pair(..., EcsChildOf, ...)`.
- Flat flow UI contract already exists and is stable in `ui_scene_hierarchy.h`.
- Anchor API is stable: `scene_add_anchor(scene, name, desc)`.

## Architecture Pattern

### Recommended minimal patch surface
1. `jsonl_import_job.h`
   - Root stem/suffix + path label contract helper
   - Lazy per-entry anchor creation only when geometry is actually created
   - Parent only existing entry anchors to root
2. `src/tests/jsonl_flat_anchor_scoped_ingest_test.c` (+ CMake wiring)
3. Keep Phase 36 UI behavior unchanged

### Risks / failure modes
1. Empty entry anchors still created if anchor creation remains eager.
2. Root label contract incorrect if filename extension/static description are kept.
3. Rerun naming unstable without deterministic suffixing.
4. Label length truncation edge cases (`LabelComp` limits) need explicit handling.

## Validation Architecture

### Test Infrastructure
| Property | Value |
|---|---|
| Framework | CTest (CMake) |
| Config file | `CMakeLists.txt`, `src/CMakeLists.txt` |
| Quick run command | `ctest --test-dir build-vulkan -C Release -R "jsonl_flat_import_ui_contract_test\|jsonl_flat_import_options_contract_test\|jsonl_flat_anchor_scoped_ingest_test" --output-on-failure` |
| Full suite command | `ctest --test-dir build-vulkan -C Release --output-on-failure` |

### Phase Requirements -> Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|---|---|---|---|---|
| FIMP-03 | Single root per run, plain non-sketch children, entry anchors only for non-empty entries, rerun suffix naming | integration | `ctest --test-dir build-vulkan -C Release -R "jsonl_flat_anchor_scoped_ingest_test" --output-on-failure` | No |

### Wave 0 Gaps
- [ ] `src/tests/jsonl_flat_anchor_scoped_ingest_test.c` — assert D-02/D-03/D-04/D-07/D-08 contracts.
- [ ] `src/CMakeLists.txt` wiring for the new test target.

## Sources

- `.planning/phases/37-anchor-scoped-flat-ingest/37-CONTEXT.md`
- `.planning/REQUIREMENTS.md`
- `src/jsonl_import_job.h`
- `src/ui/ui_scene_hierarchy.h`
- `src/ecs/ecs_scene.h`
- `src/ecs/ecs_world.h`
- `src/components/label_comp.h`
