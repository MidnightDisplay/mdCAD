---
phase: 38-observable-link-manual-transactional-refresh
plan: 01
subsystem: observer-metadata
tags: [jsonl, flat-import, observer, serializer, contracts]
requires:
  - phase: 37-anchor-scoped-flat-ingest
    provides: "Stable flat root/entry hierarchy and import contract baseline."
provides:
  - "Flat root observer metadata capture at import completion (including replay settings)."
  - "jsonl_observer serialization/load support even when linked=false."
  - "Phase 38 contract tests for metadata durability and inspector surface guardrails."
affects: [phase 38 plan 02 runtime refresh path]
tech-stack:
  added: []
  patterns: ["Contract-first tests", "Observer metadata persistence independent of linked flag"]
key-files:
  created:
    - src/tests/jsonl_flat_observer_manual_refresh_test.c
    - src/tests/jsonl_flat_observer_inspector_contract_test.c
  modified:
    - src/components/jsonl_observer_comp.h
    - src/jsonl_import_job.h
    - src/scene_serializer.h
    - src/CMakeLists.txt
requirements-completed: [OBSF-01, OBSF-02]
completed: 2026-04-27
---

# Phase 38 Plan 01 Summary

**Implemented durable flat-root observer metadata and contract tests so flat imports retain refresh replay settings and survive save/load even when link is OFF.**

## Accomplishments
- Added `use_jsonl_colours` and `mesh_import_mode` to `JsonlObserverComp` defaults.
- Wired flat import completion to always attach observer metadata to the created root anchor from import options + observer contract.
- Updated scene serialization/load so `jsonl_observer` is persisted for observer-bearing anchors even when `linked=false`.
- Added Phase 38 tests:
  - `jsonl_flat_observer_manual_refresh_test` (metadata durability, roundtrip persistence, transactional behavior hooks).
  - `jsonl_flat_observer_inspector_contract_test` (minimal inspector surface and no global-status coupling).
- Registered both new tests in `src/CMakeLists.txt`.

## Verification
- `cmake --build build-vulkan --config Release --target jsonl_flat_observer_manual_refresh_test jsonl_flat_observer_inspector_contract_test` — **PASS**
- `ctest --test-dir build-vulkan -C Release -R "jsonl_flat_observer_manual_refresh_test|jsonl_flat_observer_inspector_contract_test|jsonl_flat_import_options_contract_test" --output-on-failure` — **PASS**

## Notes
- `jsonl_import_job_set_observer_contract(...)` now captures source path even when link is disabled; `linked` remains controlled by explicit contract flag.
