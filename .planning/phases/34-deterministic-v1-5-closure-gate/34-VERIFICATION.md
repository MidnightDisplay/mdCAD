---
phase: 34-deterministic-v1-5-closure-gate
verified: 2026-04-11T21:10:00Z
status: failed
score: 2/3 closure truths verified
---

# Phase 34: Deterministic v1.5 Closure Gate Verification Evidence

**Phase Goal:** Developers and users can trust v1.5 reliability claims through reproducible deterministic baseline and immediate rerun evidence.  
**Requirement:** `DIAG-03`  
**Run scope:** Windows Vulkan canonical 7-test closure gate  
**Result:** **FAILED** (deterministic parity on failure observed, but baseline was not a pass)

## Executed Commands

1. Build:
   - `cmake --build build-vulkan --config Release`
2. Baseline:
   - `ctest --test-dir build-vulkan -C Release -R "scene_solver_contract|scene_solver_pass_policy|scene_solver_diagnostics|scene_solver_trigger|scene_solver_drag|endpoint_pick|script_roundtrip_tests" --output-on-failure`
3. Immediate rerun:
   - Same canonical command as baseline, executed immediately.

## Result Summaries

### Build
- Outcome: **Pass**
- Summary: Release targets built successfully in `build-vulkan`.

### Baseline (canonical 7-test gate)
- Outcome: **Fail**
- Summary: `71% tests passed, 2 tests failed out of 7`
- Failed tests:
  - `scene_solver_contract`
    - `FAILED: test_parallel_pair_along_interaction_switches_authority_without_lock_in`
  - `scene_solver_drag`
    - `FAILED: test_drag_parallel_pair_ab_moves_cd_follows`

### Immediate rerun (same canonical command)
- Outcome: **Fail**
- Summary: `71% tests passed, 2 tests failed out of 7`
- Failed tests:
  - `scene_solver_contract`
    - `FAILED: test_parallel_pair_along_interaction_switches_authority_without_lock_in`
  - `scene_solver_drag`
    - `FAILED: test_drag_parallel_pair_ab_moves_cd_follows`

## Outcome Parity

- Baseline outcome class: **Fail (2/7 failed)**
- Immediate rerun outcome class: **Fail (2/7 failed)**
- Parity: **Matched** (deterministic repeat of identical failure set)

## DIAG-03 Verdict

`DIAG-03` is **NOT SATISFIED** for closure because the requirement demands a deterministic baseline **pass** plus immediate rerun parity.  
While parity was deterministic, baseline did not pass.

## Closure Policy Application

- Hard-fail divergence/flakiness policy remained enforced.
- No command variants were used.
- Windows Vulkan-only scope was preserved.
- Stabilization is required before sign-off can proceed.

## Historical UAT Warning Markers (D-09)

Historical warning markers are treated as non-blocking metadata **only after DIAG-03 parity-on-pass is satisfied**.  
Because DIAG-03 is currently unsatisfied, this non-blocking interpretation is not yet sufficient to close the phase.

## Next Action

Create gap-closure work for the failing canonical gate tests (`scene_solver_contract`, `scene_solver_drag`), then rerun the exact baseline + immediate rerun closure sequence.
