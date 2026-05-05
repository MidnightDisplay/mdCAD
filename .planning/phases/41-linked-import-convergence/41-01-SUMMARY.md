---
phase: 41-linked-import-convergence
plan: 01
subsystem: observer-runtime
tags: [jsonl, observer, linked-import, regression]
requires:
  - phase: 40-large-dump-stability-interaction-coherence
    provides: "Large-dump flat refresh stability guardrails and bounded observer behavior."
provides:
  - "Initial linked flat imports stamp their observer baseline at commit time."
  - "Baseline-arm failure keeps committed geometry and source metadata while observe disables safely."
  - "Deterministic regressions protect no-self-refresh startup and fallback behavior."
affects: [41-02 manual acceptance, phase-42 refresh-teardown stability]
tech-stack:
  added: []
  patterns: ["import-time baseline arm", "safe observe-disable fallback", "linked-import idle regression"]
key-files:
  modified:
    - src/jsonl_import_job.h
    - src/tests/jsonl_flat_observer_auto_safety_test.c
    - src/tests/jsonl_flat_observer_manual_refresh_test.c
requirements-completed: [FIMP-05, FIMP-06]
completed: 2026-05-05
---

# Phase 41 Plan 01 Summary

**Initial linked flat imports now stamp their observer baseline at commit time and stay idle until a real source change.**

## Accomplishments

1. Reused `jsonl_observer_stamp_source_state(...)` during flat import completion so linked imports commit with a valid source baseline instead of appearing immediately changed.
2. Added a safe fallback that preserves the imported root, geometry, and source path while disabling observe with explicit warning/status text when baseline arming fails.
3. Added deterministic regressions for:
   - `test_flat_linked_import_stamps_baseline_and_stays_idle`
   - `test_flat_linked_import_baseline_failure_keeps_geometry_and_disables_observe`

## Task Commits

1. `a74d99c` — `test(41-01): add failing linked-import convergence regressions`
2. `6168330` — `fix(41-01): arm linked import observer baseline`

## Verification

- `ctest --test-dir build-vulkan -C Release -R "jsonl_flat_observer_(auto_safety_test|manual_refresh_test|inspector_contract_test)" --output-on-failure` passed.

## Outcome

Plan 41-01 completed with initial linked-import convergence locked in automation and ready for the manual `lamp_11.jsonl` acceptance checklist in 41-02.
