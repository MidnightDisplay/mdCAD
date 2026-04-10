---
phase: 31-script-reapply-fidelity-foundation
plan: 01
subsystem: scripting, solver-contract
tags: [script, registry, contract, deterministic-errors, parity]
requires: []
provides:
  - "Single-source script+solver capability registry for supported entity/constraint signatures."
  - "Deterministic contract-failure taxonomy shared across script validation and solver legality."
  - "Parity regression coverage preventing script/solver allowlist drift."
key-files:
  created:
    - src/scripting/sketch_script_capability_registry.h
  modified:
    - src/scripting/sketch_script_contract.h
    - src/ecs/ecs_scene.h
    - src/tests/scene_solver_contract_test.c
    - src/tests/script_roundtrip_tests.c
requirements-completed: [SCRI-01]
completed: 2026-04-10
---

# Phase 31 Plan 01 Summary

Implemented shared capability-contract wiring so script validation and solver legality now use one registry and fail deterministically on unsupported signatures.

- Added `sketch_script_capability_registry.h` as single-source capability authority.
- Removed duplicated script-contract allowlist logic in favor of registry helpers.
- Wired solver constraint admission (`scene_add_constraint_to_sketch_with_descriptors`) to the same registry validators.
- Added parity-focused tests in script/solver suites for deterministic contract-failure taxonomy.

Verification:

- `cmake --build build-vulkan --config Release` → pass
- `ctest --test-dir build-vulkan -C Release -R "script_roundtrip_tests|scene_solver_contract|scene_solver_diagnostics" --output-on-failure` → pass

