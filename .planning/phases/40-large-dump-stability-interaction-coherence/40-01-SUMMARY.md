---
phase: 40-large-dump-stability-interaction-coherence
plan: 01
subsystem: observer-runtime
tags: [jsonl, observer, coalescing, perf-evidence]
requires:
  - phase: 39-automatic-observer-safety-loop
    provides: "Flat observer auto safety baseline and transactional refresh path."
provides:
  - "Bounded single-flight + one coalesced pending rerun runtime policy."
  - "Advisory timing evidence surfaced in flat inspector status."
  - "Deterministic burst/tier stability regressions for PERF-01."
affects: [40-02 coherence work, phase-40 verification]
tech-stack:
  added: []
  patterns: ["bounded coalescing", "advisory perf evidence", "tiered deterministic fixtures"]
key-files:
  modified:
    - src/jsonl_observer_system.h
    - src/ui/ui_entity_inspector.h
    - src/tests/jsonl_flat_observer_auto_safety_test.c
    - src/tests/jsonl_flat_observer_inspector_contract_test.c
requirements-completed: [PERF-01]
completed: 2026-04-27
---

# Phase 40 Plan 01 Summary

**Implemented bounded burst-refresh coalescing and advisory timing evidence for large flat refresh stability.**

## Accomplishments

1. Added `pending_rerun` semantics for flat refresh slots so burst updates coalesce to one follow-up run.
2. Preserved bounded runtime behavior (one active + one pending rerun max) without introducing queue growth.
3. Added elapsed-time capture in flat refresh completion messages and explicit advisory (non-hard-gate) timing copy in inspector.
4. Expanded auto safety tests with:
   - burst coalescing coverage,
   - deterministic small/medium/large fixture stability checks.

## Task Commits

1. `a16a805` — `feat(observer): add coalesced rerun and advisory timing`
2. `a3afcf0` — `test(observer): add burst coalescing and tiered stability checks`

## Verification

- `ctest --test-dir build-vulkan -C Release -R "jsonl_flat_observer_(auto_safety_test|inspector_contract_test|manual_refresh_test)" --output-on-failure` passed.

## Outcome

Plan 40-01 completed with bounded coalescing behavior and PERF-01-focused stability evidence ready for phase-40 wave-2 coherence work.
