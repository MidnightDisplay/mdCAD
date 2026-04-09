---
phase: 30-deterministic-closure-gate-windows-vulkan-solver-docs
verified: 2026-04-09T14:28:00Z
status: passed
scope: windows-vulkan-only
---

# Phase 30: Deterministic Closure Gate (Windows Vulkan) Verification

This report captures closure evidence for `V14-01` and `V14-02` using the locked deterministic Windows Vulkan sequence:

1. Build (`cmake --build build-vulkan --config Release`)
2. Baseline canonical 7-test gate
3. Immediate rerun canonical 7-test gate

Deterministic policy: **any flakiness is treated as failure**.

## Canonical Command (Locked)

`ctest --test-dir build-vulkan -C Release -R "scene_solver_contract|scene_solver_pass_policy|scene_solver_diagnostics|scene_solver_trigger|scene_solver_drag|endpoint_pick|script_roundtrip_tests" --output-on-failure`

## Evidence Block 1 — Build (Windows Vulkan)

**Command**

`cmake --build build-vulkan --config Release`

**Result summary**

- Status: **PASS**
- Toolchain output confirms successful Release artifacts for:
  - `mdCAD`
  - `script_roundtrip_tests`
  - `scene_solver_contract`
  - `scene_solver_pass_policy`
  - `scene_solver_diagnostics`
  - `scene_solver_trigger`
  - `scene_solver_drag`
  - `endpoint_pick`
- No build failure observed in command output.

## Evidence Block 2 — Baseline Canonical Gate

**Label:** Baseline

**Command**

`ctest --test-dir build-vulkan -C Release -R "scene_solver_contract|scene_solver_pass_policy|scene_solver_diagnostics|scene_solver_trigger|scene_solver_drag|endpoint_pick|script_roundtrip_tests" --output-on-failure`

**Result summary**

- Status: **PASS**
- Outcome: `100% tests passed, 0 tests failed out of 7`
- Total time (real): `5.10 sec`
- Covered tests:
  - `script_roundtrip_tests`
  - `scene_solver_contract`
  - `scene_solver_drag`
  - `endpoint_pick`
  - `scene_solver_diagnostics`
  - `scene_solver_trigger`
  - `scene_solver_pass_policy`

## Evidence Block 3 — Immediate Rerun Canonical Gate

**Label:** Immediate rerun

**Command**

`ctest --test-dir build-vulkan -C Release -R "scene_solver_contract|scene_solver_pass_policy|scene_solver_diagnostics|scene_solver_trigger|scene_solver_drag|endpoint_pick|script_roundtrip_tests" --output-on-failure`

**Result summary**

- Status: **PASS**
- Outcome: `100% tests passed, 0 tests failed out of 7`
- Total time (real): `0.41 sec`
- Covered tests:
  - `script_roundtrip_tests`
  - `scene_solver_contract`
  - `scene_solver_drag`
  - `endpoint_pick`
  - `scene_solver_diagnostics`
  - `scene_solver_trigger`
  - `scene_solver_pass_policy`

## Deterministic Policy Outcome

- Baseline and immediate rerun both passed with identical test scope and command string.
- No flakiness observed in closure sequence.
- Per policy, any future flakiness (baseline/rerun mismatch or intermittent failures) must be treated as closure failure until stabilized.

## Scope Confirmation

- This sign-off is scoped to the **Windows Vulkan** path only.
- Cross-platform closure expansion remains out of scope for Phase 30.
