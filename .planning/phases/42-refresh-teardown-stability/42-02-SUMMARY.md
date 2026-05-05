---
phase: 42-refresh-teardown-stability
plan: 02
subsystem: observer-runtime
tags: [jsonl, observer, delete, undo, teardown]
requires:
  - phase: 42-refresh-teardown-stability
    plan: 01
    provides: "Exact settled refresh occupancy and cleanup anomaly fallback."
provides:
  - "Linked-root delete cancels active flat refresh work before entity teardown."
  - "Keyboard and hierarchy delete use the same undo-first cancel/delete ordering."
  - "Deterministic regressions protect delete-after-refresh, active-refresh delete, and undo-last-committed-only behavior."
affects: [42-03 manual acceptance, linked flat import teardown]
tech-stack:
  added: []
  patterns: ["cancel-before-delete", "shared delete helper", "undo committed-only restore"]
key-files:
  modified:
    - src/jsonl_observer_system.h
    - src/ui/ui_scene_hierarchy.h
    - src/app.c
    - src/tests/jsonl_flat_observer_manual_refresh_test.c
requirements-completed: [OBSF-08, PERF-04]
completed: 2026-05-05
---

# Phase 42 Plan 02 Summary

**Linked flat import deletion now cancels active refresh work first, clears staged leftovers, and restores only committed content on undo.**

## Accomplishments

1. Added deterministic regressions for delete-after-refresh cleanup and delete-during-refresh undo behavior, including explicit `slot_to_entity` ownership scans.
2. Added `jsonl_observer_cancel_flat_refresh_for_root(...)` plus in-flight staged-entity teardown so deleting a linked root retires active refresh work instead of leaving orphaned stage geometry behind.
3. Routed hierarchy context-menu delete and keyboard Delete/Backspace through the same undo-first, cancel-before-delete ordering.
4. Kept refresh locks on drag/reparent actions while making delete available during refresh with explicit UI copy: `Delete will cancel the running flat refresh.`

## Task Commits

1. `6d2ccfd` — `test(42-02): add delete teardown regressions`
2. `5ed8aa2` — `feat(42-02): cancel refresh before linked delete`

## Verification

- `ctest --test-dir build-vulkan -C Release -R "jsonl_flat_observer_(manual_refresh_test|auto_safety_test|inspector_contract_test)" --output-on-failure` passed.
- `cmake --build build-vulkan --config Release --target mdCAD` passed.

## Outcome

Plan 42-02 completed with cancel-before-delete teardown semantics locked in automation and ready for the real-file `lamp_11.jsonl` manual acceptance pass in 42-03.
