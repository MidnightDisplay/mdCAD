# Phase 36: Flat Import Entry & Configuration - Research

**Researched:** 2026-04-27  
**Domain:** Flat JSONL import entry path and options contract  
**Confidence:** HIGH

## Summary

Phase 36 should be implemented as a **new UI entry flow** that reuses existing flat JSONL import infrastructure (`jsonl_import_job`) and existing popup/progress patterns in `ui_scene_hierarchy.h`.  
This minimizes risk and fully satisfies `FIMP-01` and `FIMP-02` without changing existing JSONL geometry-log or JSONL-as-sketch behavior.

## Locked Decisions Carried from Context

- Add third explicit menu action: `Import JSONL (Flat Large Dump)...`.
- Keep existing two JSONL actions unchanged.
- Mirror current JSONL options: units, colours/default colour, shift-to-CoM, XYZ rotation, mesh mode.
- Show `Link file for refresh (optional)` default OFF.
- Keep existing sync-fast-path + async progress-popup behavior.

## Implementation Anchors

### UI flow
- `src/ui/ui_scene_hierarchy.h`
  - Existing JSONL browser + quick scan + options popup flow around the `Import JSONL Options` popup.
  - Existing JSONL progress modal and sync/async branch.
  - Existing JSONL-as-sketch popup used as secondary reference for observer-related wording.

### Import execution
- `src/jsonl_import_job.h`
  - `jsonl_import_job_start(...)`
  - `jsonl_import_job_should_sync(...)`
  - `jsonl_import_job_tick(...)`
  - `jsonl_import_job_set_mesh_mode(...)`

### Parse metadata for popup
- `src/jsonl_loader.h`
  - `jsonl_quick_scan_mesh(...)` for entry/element/mesh summary before showing popup.

### Forward-compatible observer metadata shape
- `src/components/jsonl_observer_comp.h`
- `src/scene_serializer.h`

## Requirement Support

| Requirement | Research-backed approach |
|-------------|---------------------------|
| FIMP-01 | Add dedicated third action in File menu and dedicated browser/popup state for flat-large path. |
| FIMP-02 | Mirror existing option controls and pass selected values through existing job-start mapping before import execution. |

## Recommended Architecture Pattern

1. Add dedicated flat-large browser/popup state fields in `ui_scene_hierarchy_state_t`.
2. Reuse `jsonl_quick_scan_mesh(...)` and option controls from existing JSONL popup.
3. On submit, map options to `jsonl_import_job_start(...)` and mesh mode setter.
4. Keep current sync threshold and async progress popup behavior unchanged.
5. Capture observer opt-in (default OFF) in a persistable forward-compatible shape for later phases.

## Risks / Pitfalls

1. **State collision** between old JSONL popup and new flat-large popup if state fields are shared.
2. **Behavior drift** if new flow accidentally replaces existing JSONL action semantics.
3. **Contract mismatch** if mesh mode or transform options are omitted from the new popup.
4. **UX inconsistency** if async popup path is not triggered for non-trivial files.

## Open Questions

1. Where exactly observer opt-in metadata is attached in Phase 36 (root anchor vs interim storage) before Phase 38 refresh engine.
2. Whether progress modal title text should explicitly mention flat-large path while preserving behavior.

## Validation Architecture

### Existing test harness
- CTest via top-level and `src/CMakeLists.txt`.
- Existing related tests:
  - `jsonl_loader_limits_test`
  - `jsonl_sketch_import_test`
  - `jsonl_observer_state_test`

### Phase 36 gaps (Wave 0)
- Add test for menu/entry contract of flat-large action.
- Add test for option-to-job-start mapping contract.
- Register both tests in `src/CMakeLists.txt`.

### Suggested commands
- Targeted:
  - `ctest --output-on-failure -R "jsonl_loader_limits_test|jsonl_sketch_import_test|jsonl_observer_state_test"`
- Full:
  - `ctest --output-on-failure`

## Sources

- `.planning/phases/36-flat-import-entry-configuration/36-CONTEXT.md`
- `.planning/REQUIREMENTS.md`
- `.planning/ROADMAP.md`
- `src/ui/ui_scene_hierarchy.h`
- `src/jsonl_import_job.h`
- `src/jsonl_loader.h`
- `src/components/jsonl_observer_comp.h`
- `src/scene_serializer.h`
- `src/CMakeLists.txt`
