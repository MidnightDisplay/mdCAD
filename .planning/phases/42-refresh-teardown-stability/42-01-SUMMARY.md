---
phase: 42-refresh-teardown-stability
plan: 01
subsystem: observer-runtime
tags: [jsonl, observer, refresh, slot-compaction, safety]
requires:
  - phase: 41-linked-import-convergence
    provides: "Linked flat imports commit full geometry and stamp observer baseline safely."
provides:
  - "Flat refresh settle compacts line and point slot buffers back to exact live occupancy."
  - "Cleanup/commit anomalies keep last-good content, disable observe, and preserve manual refresh."
  - "Selection fallback to the linked root survives descendant teardown during refresh commit."
affects: [42-02 teardown work, 42-03 manual acceptance]
tech-stack:
  added: []
  patterns: ["settle-time slot compaction", "request-time selection fallback latch", "safe anomaly fallback"]
key-files:
  modified:
    - src/jsonl_observer_system.h
    - src/tests/jsonl_flat_observer_auto_safety_test.c
    - src/tests/jsonl_flat_observer_manual_refresh_test.c
requirements-completed: [OBSF-07, OBSF-08, PERF-05]
completed: 2026-05-05
---

# Phase 42 Plan 01 Summary

**Linked flat refresh now settles back to exact live slot occupancy and fails safe when cleanup invariants break.**

## Accomplishments

1. Added exact-footprint regressions for observer-driven refresh, repeated manual refresh, and forced cleanup anomalies in the flat observer test suite.
2. Added settle-time compaction for flat line/point instance buffers, including renderable slot rebinding so committed geometry owns the compacted slots consistently.
3. Added a one-shot cleanup-failure seam plus anomaly fallback that keeps last-good content, disables automatic observe, and preserves manual `Re-import now`.
4. Fixed refresh selection fallback so linked roots reclaim selection even when the previously selected descendant dies before commit finishes.

## Task Commits

1. `fde5852` — `test(42-01): add failing refresh footprint regressions`
2. `fe30f8e` — `feat(42-01): stabilize flat refresh cleanup`

## Verification

- `ctest --test-dir build-vulkan -C Release -R "jsonl_flat_observer_(auto_safety_test|manual_refresh_test|inspector_contract_test)" --output-on-failure` passed.

## Outcome

Plan 42-01 completed with exact settled refresh occupancy, no-late-slack regressions, and cleanup anomaly fallback ready for the delete/teardown work in 42-02.
