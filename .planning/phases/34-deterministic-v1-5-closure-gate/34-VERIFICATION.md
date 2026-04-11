---
phase: 34-deterministic-v1-5-closure-gate
verified: 2026-04-11T23:59:00Z
status: passed
score: 3/3 must-haves verified
re_verification:
  previous_status: gaps_found
  previous_score: 2/3 must-haves verified
  gaps_closed:
    - "Developer can run the canonical Windows Vulkan 7-test gate and get a 7/7 baseline pass."
  gaps_remaining: []
  regressions: []
---

# Phase 34: Deterministic v1.5 Closure Gate Verification Report

**Phase Goal:** Developers and users can trust v1.5 reliability claims through reproducible deterministic baseline and immediate rerun evidence.  
**Verified:** 2026-04-11T23:59:00Z  
**Status:** passed  
**Re-verification:** Yes — after gap closure

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
| --- | --- | --- | --- |
| 1 | Developer can run the v1.5 targeted regression gate and obtain a deterministic baseline pass result. | ✓ VERIFIED | Canonical command returned `100% tests passed, 0 tests failed out of 7` (baseline exit code `0`). |
| 2 | Developer can immediately rerun the identical gate command and observe parity with baseline. | ✓ VERIFIED | Immediate rerun returned `100% tests passed, 0 tests failed out of 7` (rerun exit code `0`), with explicit parity marker `PARITY=PASS`. |
| 3 | Milestone closure evidence demonstrates deterministic sign-off for the v1.5 targeted workflow set. | ✓ VERIFIED | Verification artifact contains explicit closure evidence lines: baseline 7/7 pass, immediate rerun 7/7 pass, `Outcome parity`, and `DIAG-03 SATISFIED`. |

**Score:** 3/3 truths verified

### Required Artifacts

| Artifact | Expected | Status | Details |
| --- | --- | --- | --- |
| `src/ecs/ecs_scene.h` | Parallel/ALONG drag-authority behavior no longer regresses canonical contract+drag tests | ✓ VERIFIED | Substantive helper exists (`scene_solver_apply_parallel_drag_authority_translation`) and is applied in PARALLEL solve path (`3905-3923`, `3946-3947`). |
| `.planning/phases/34-deterministic-v1-5-closure-gate/34-VERIFICATION.md` | Baseline+rereun closure evidence with explicit DIAG-03 satisfied verdict | ✓ VERIFIED | Contains baseline, immediate rerun, parity, and explicit `DIAG-03 SATISFIED` decision text. |

### Key Link Verification

| From | To | Via | Status | Details |
| --- | --- | --- | --- | --- |
| `src/ecs/ecs_scene.h` | `src/tests/scene_solver_contract_test.c` | `test_parallel_pair_along_interaction_switches_authority_without_lock_in` / `scene_solver_contract` | ✓ WIRED | Target test exists and is registered (`2545`, `2730-2731`); canonical `scene_solver_contract` passes in baseline+rereun. |
| `src/ecs/ecs_scene.h` | `src/tests/scene_solver_drag_test.c` | `test_drag_parallel_pair_ab_moves_cd_follows` / `scene_solver_drag` | ✓ WIRED | Target test exists and is registered (`573`, `762-763`); canonical `scene_solver_drag` passes in baseline+rereun. |
| `.planning/.../34-VERIFICATION.md` | DIAG-03 closure decision | Baseline + Immediate rerun + Outcome parity + DIAG decision | ✓ WIRED | Evidence and decision are explicitly present in this artifact. |

### Data-Flow Trace (Level 4)

| Artifact | Data Variable | Source | Produces Real Data | Status |
| --- | --- | --- | --- | --- |
| `src/ecs/ecs_scene.h` | Solver candidate points and drag-anchor authority | Runtime solver state in `scene_solver_request_recalculate` | Yes (tested by executable CTest runs, not static constants) | ✓ FLOWING |
| `.planning/.../34-VERIFICATION.md` | Closure evidence statements | Executed CTest outputs and exit codes | Yes (command output captured in artifact) | ✓ FLOWING |

### Behavioral Spot-Checks

| Behavior | Command | Result | Status |
| --- | --- | --- | --- |
| Canonical gate baseline passes | `ctest --test-dir build-vulkan -C Release -R "scene_solver_contract|scene_solver_pass_policy|scene_solver_diagnostics|scene_solver_trigger|scene_solver_drag|endpoint_pick|script_roundtrip_tests" --output-on-failure` | `100% tests passed, 0 tests failed out of 7` | ✓ PASS |
| Immediate rerun parity holds | Same command executed immediately twice in sequence | `BASELINE_EXIT=0`, `RERUN_EXIT=0`, `PARITY=PASS` | ✓ PASS |

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
| --- | --- | --- | --- | --- |
| DIAG-03 | `34-01-PLAN.md`, `34-02-PLAN.md` | Developer can verify deterministic baseline + immediate rerun parity on the v1.5 targeted regression gate | ✓ SATISFIED | Baseline 7/7 pass and immediate rerun 7/7 pass are both evidenced, with explicit parity and closure verdict. |

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
| --- | --- | --- | --- | --- |
| `src/ecs/ecs_scene.h` | 5417 | `Placeholder` comment | ℹ️ Info | Legacy note in unrelated section; does not affect Phase 34 closure behavior or canonical gate evidence. |

### Human Verification Required

None required for DIAG-03 closure. Automated deterministic gate evidence is sufficient.

### DIAG-03 Decision

**DIAG-03 SATISFIED**

Canonical evidence is explicit and consistent:
- **Baseline:** 7/7 pass
- **Immediate rerun:** 7/7 pass
- **Outcome parity:** explicit and passing

### Gaps Summary

No remaining gaps. Previously failed baseline truth is now closed with reproducible baseline+rereun pass evidence and parity.

---

_Verified: 2026-04-11T23:59:00Z_  
_Verifier: the agent (gsd-verifier)_
