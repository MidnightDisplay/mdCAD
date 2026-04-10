---
phase: 33
slug: large-jump-robustness-and-parallel-along-parity
status: draft
nyquist_compliant: true
wave_0_complete: false
created: 2026-04-10
---

# Phase 33 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | CTest + native C executable tests (repo-native) |
| **Config file** | `CMakeLists.txt`, `src/CMakeLists.txt` |
| **Fast smoke command (<30s target)** | `ctest --test-dir build-vulkan -C Release -R "scene_solver_drag|scene_solver_contract" --output-on-failure` |
| **Quick run command** | `ctest --test-dir build-vulkan -C Release -R "scene_solver_contract|scene_solver_drag|scene_solver_pass_policy|scene_solver_diagnostics" --output-on-failure` |
| **Full suite command** | `ctest --test-dir build-vulkan -C Release -R "scene_solver_contract|scene_solver_pass_policy|scene_solver_diagnostics|scene_solver_trigger|scene_solver_drag|endpoint_pick|script_roundtrip_tests" --output-on-failure` |
| **Estimated fast smoke runtime** | <30 seconds target |
| **Estimated quick/full runtime** | ~60 seconds |

---

## Sampling Rate

- **After every task commit:** Run `ctest --test-dir build-vulkan -C Release -R "scene_solver_drag|scene_solver_contract" --output-on-failure` (fast smoke)
- **After every plan wave:** Run `ctest --test-dir build-vulkan -C Release -R "scene_solver_contract|scene_solver_drag|scene_solver_pass_policy|scene_solver_diagnostics" --output-on-failure` (quick)
- **Before `/gsd-verify-work`:** Full suite must be green
- **Max feedback latency (per-task smoke):** <30 seconds target
- **Wave/phase gate latency budget:** ~60 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 33-01-01 | 01 | 1 | SROB-01, SROB-02 | integration | `ctest --test-dir build-vulkan -C Release -R "scene_solver_drag|scene_solver_contract" --output-on-failure` | ✅ | ⬜ pending |
| 33-01-02 | 01 | 1 | SROB-03, DIAG-01 | integration | `ctest --test-dir build-vulkan -C Release -R "scene_solver_drag|scene_solver_diagnostics" --output-on-failure` | ✅ | ⬜ pending |
| 33-02-01 | 02 | 2 | PARI-01, PARI-02 | integration | `ctest --test-dir build-vulkan -C Release -R "scene_solver_pass_policy|scene_solver_contract" --output-on-failure` | ✅ | ⬜ pending |
| 33-02-02 | 02 | 2 | DIAG-01, DIAG-02 | diagnostics | `ctest --test-dir build-vulkan -C Release -R "scene_solver_diagnostics|scene_solver_pass_policy" --output-on-failure` | ✅ | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] `src/tests/scene_solver_drag_test.c` — quarter-arc large-jump fixture covering "wiggle-to-latch" regression (`SROB-01`).
- [ ] `src/tests/scene_solver_contract_test.c` — transactional rollback/no-partial-mutation fixture for unsatisfied large-jump updates (`SROB-02`).
- [ ] `src/tests/scene_solver_pass_policy_test.c` — explicit PARALLEL↔ALONG parity matrix fixtures across base/mirrored/order variants (`PARI-01`, `PARI-02`).
- [ ] `src/tests/scene_solver_diagnostics_test.c` — deterministic large-jump/parity reason-taxonomy ordering assertions (`DIAG-01`, `DIAG-02`).

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Live sketch interaction confirms immediate post-failure edit responsiveness without stale lock in viewport drag sessions | SROB-03 | Requires interactive gizmo workflows across user-edited scene states | Reproduce infeasible large-jump in a mixed line/arc sketch; immediately drag a related endpoint and confirm solver response, geometry stability, and diagnostic refresh. |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Per-task fast smoke feedback latency target < 30s
- [x] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
