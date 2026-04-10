---
phase: 31-script-reapply-fidelity-foundation
plan: 02
subsystem: scripting, roundtrip, diagnostics
tags: [script, descriptor-fidelity, color-fidelity, deterministic-reapply]
requires:
  - 31-01
provides:
  - "Descriptor participant fidelity (`id`, `role`, `sub_index`) across parse/apply/emit/reapply."
  - "Script-managed color fidelity for point/line/arc across roundtrip/reapply."
  - "Deterministic repeat-apply and immediate-rerun parity evidence for script and diagnostics suites."
key-files:
  modified:
    - src/scripting/sketch_script_parse.h
    - src/scripting/sketch_script_apply.h
    - src/scripting/sketch_script_emit.h
    - src/tests/script_roundtrip_tests.c
    - src/tests/scene_solver_diagnostics_test.c
    - .planning/phases/31-script-reapply-fidelity-foundation/31-VALIDATION.md
requirements-completed: [SCRI-02, SCRI-03]
completed: 2026-04-10
---

# Phase 31 Plan 02 Summary

Completed end-to-end script reapply fidelity for descriptors and colors, then locked deterministic parity through repeated targeted/full rerun gates.

- Parser now enforces descriptor-object participants and deterministic schema failures.
- Apply path preserves participant role/sub-index and validates role legality by geometry.
- Emitter now outputs canonical descriptor participants and canonical color fields.
- Apply commit now replaces only script-managed children, preserving non-script geometry/colors.
- Updated legacy roundtrip tests to descriptor syntax and added new Phase 31 regression cases:
  - registry parity deterministic error
  - descriptor roundtrip role/sub-index preservation
  - deterministic atomic reject behavior
  - color roundtrip + non-script color untouched
  - repeat apply deterministic parity
  - diagnostics reapply parity

Verification:

- `ctest --test-dir build-vulkan -C Release -R "script_roundtrip_tests|scene_solver_contract|scene_solver_diagnostics" --output-on-failure` → pass
- Immediate rerun of same command → pass
- `ctest --test-dir build-vulkan -C Release -R "scene_solver_contract|scene_solver_pass_policy|scene_solver_diagnostics|scene_solver_trigger|scene_solver_drag|endpoint_pick|script_roundtrip_tests" --output-on-failure` → pass
- Immediate rerun of same full command → pass

