---
phase: 38-observable-link-manual-transactional-refresh
plan: 02
subsystem: manual-transactional-refresh
tags: [jsonl, observer, refresh, transactional, inspector]
requires:
  - phase: 38-observable-link-manual-transactional-refresh
    plan: 01
    provides: "Observer metadata durability and test harness for flat roots."
provides:
  - "Background flat refresh request/tick runtime with staged import and safe subtree swap."
  - "Manual inspector controls for source, link toggle, and re-import."
  - "Frame-loop integration for non-blocking flat observer refreshes."
affects: [phase 39 automatic-observer-safety-loop]
tech-stack:
  added: []
  patterns: ["Transactional staged swap", "Last-good subtree preservation on failure", "Selection fallback to anchor"]
key-files:
  modified:
    - src/jsonl_observer_system.h
    - src/ui/ui_entity_inspector.h
    - src/app.c
    - src/tests/jsonl_flat_observer_manual_refresh_test.c
requirements-completed: [OBSF-03, OBSF-04, OBSF-05]
completed: 2026-04-27
---

# Phase 38 Plan 02 Summary

**Implemented non-sketch flat observer manual refresh as a staged, transactional, same-anchor replacement flow with minimal inspector controls.**

## Accomplishments
- Added new runtime APIs in `jsonl_observer_system.h`:
  - `jsonl_observer_request_flat_refresh(...)`
  - `jsonl_observer_tick_flat_refreshes(...)`
  - `jsonl_observer_is_flat_refresh_running(...)`
- Implemented staged flat import and subtree replacement:
  - On success: replace old imported subtree under same anchor.
  - On failure: retain last-good subtree and surface error state.
  - Added optional selection fallback to anchor when selected descendants are replaced.
- Added/updated inspector controls on flat roots:
  - Link toggle (opt-out supported).
  - Source path display/edit + choose file flow.
  - Manual “Re-import” trigger and last-message feedback.
- Integrated refresh ticking in `app.c` frame update loop so refreshes run in background without sketch reparse coupling.

## Verification
- `cmake --build build-vulkan --config Release --target mdCAD` — **PASS**
- `ctest --test-dir build-vulkan -C Release -R "jsonl_reparse_transaction_test|jsonl_flat_anchor_scoped_ingest_test|jsonl_flat_observer_manual_refresh_test|jsonl_flat_observer_inspector_contract_test" --output-on-failure` — **PASS**

## Notes
- Flat refresh slot bookkeeping is isolated per ECS world pointer to avoid cross-world entity-id collisions in test/runtime lifecycles.
