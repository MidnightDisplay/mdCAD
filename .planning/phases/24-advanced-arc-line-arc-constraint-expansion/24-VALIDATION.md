---
phase: 24
slug: advanced-arc-line-arc-constraint-expansion
status: complete
nyquist_compliant: true
wave_0_complete: true
created: 2026-04-08
---

# Phase 24 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | CTest + native C executable tests |
| **Config file** | `CMakeLists.txt`, `src/CMakeLists.txt` |
| **Quick run command** | `ctest -R "scene_solver_contract|endpoint_pick" --test-dir build-vulkan -C Release --output-on-failure` |
| **Full suite command** | `ctest -R "scene_solver_contract|scene_solver_pass_policy|scene_solver_diagnostics|scene_solver_trigger|scene_solver_drag|endpoint_pick" --test-dir build-vulkan -C Release --output-on-failure` |
| **Estimated runtime** | quick <180s, full targeted gate variable |

---

## Sampling Rate

- **After every task commit:** Run `ctest -R "scene_solver_contract|endpoint_pick" --test-dir build-vulkan -C Release --output-on-failure`
- **After every plan wave:** Run `ctest -R "scene_solver_contract|scene_solver_pass_policy|scene_solver_diagnostics|scene_solver_trigger|scene_solver_drag|endpoint_pick" --test-dir build-vulkan -C Release --output-on-failure`
- **Before `/gsd-verify-work`:** Full targeted gate must be green
- **Max feedback latency:** 180 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 24-01-01 | 01 | 1 | ARCI-01 | legality + transactional solve | `ctest -R "scene_solver_contract|endpoint_pick" --test-dir build-vulkan -C Release --output-on-failure` | ✅ | ✅ green |
| 24-01-02 | 01 | 1 | ARCI-02 | endpoint-role legality + solve behavior | `ctest -R "scene_solver_contract|endpoint_pick" --test-dir build-vulkan -C Release --output-on-failure` | ✅ | ✅ green |
| 24-02-01 | 02 | 2 | ARCI-03 | dimensional edit + ordered endpoint semantics | `ctest -R "scene_solver_contract|scene_solver_pass_policy" --test-dir build-vulkan -C Release --output-on-failure` | ✅ | ✅ green |
| 24-02-02 | 02 | 2 | ARCI-04 | deterministic unsat diagnostics/implication | `ctest -R "scene_solver_contract|scene_solver_pass_policy|scene_solver_diagnostics|scene_solver_trigger|scene_solver_drag" --test-dir build-vulkan -C Release --output-on-failure` | ✅ | ✅ green |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [x] `src/tests/endpoint_pick_test.c` — add ARCI legality matrix tests for required participant signatures and same-arc endpoint-pair constraints.
- [x] `src/tests/scene_solver_contract_test.c` — add ARCI success + fixed/fixed unsat transactional rollback tests.
- [x] `src/tests/scene_solver_pass_policy_test.c` or `src/tests/scene_solver_diagnostics_test.c` — lock explicit per-family unsatisfied diagnostics.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Constraint menu exposes new ARCI constraint options only for legal selections | ARCI-01, ARCI-02, ARCI-03 | Interactive selection/context-menu behavior is user-flow dependent | In active sketch, validate line+arc entity selection, endpoint-pair selection, and invalid signatures; confirm only legal ARCI options appear. |
| Unsatisfied ARCI constraints visibly highlight implicated participants/constraints in editor flow | ARCI-04 | Requires visual implication state confirmation during interaction | Author deliberate unsatisfied ARCI setup, run recalc, verify explicit diagnostics and highlighted implicated geometry/constraints. |

---

## Validation Sign-Off

- [x] All tasks have `<automated>` verification or explicit Wave 0 dependencies
- [x] Sampling continuity: no 3 consecutive tasks without automated verify
- [x] Wave 0 covers missing ARCI legality/solver/diagnostic test gaps
- [x] No watch-mode flags
- [x] Feedback latency < 180s
- [x] `nyquist_compliant: true` set in frontmatter

**Approval:** complete (manual checkpoint passed: Step 3 drag-anchor UX confirmed by user; full targeted gate green)

