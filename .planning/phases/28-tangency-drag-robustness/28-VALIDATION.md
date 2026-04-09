---
phase: 28
slug: tangency-drag-robustness
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-04-09
---

# Phase 28 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Native C tests via CMake + CTest |
| **Config file** | `CMakeLists.txt`, `src/CMakeLists.txt` |
| **Quick run command** | `ctest --test-dir build -C Release -R "scene_solver_contract\|scene_solver_drag\|scene_solver_diagnostics\|scene_solver_pass_policy" --output-on-failure` |
| **Full suite command** | `ctest --test-dir build -C Release --output-on-failure` |
| **Estimated runtime** | ~20-60 seconds |

---

## Sampling Rate

- **After every task commit:** Run quick targeted solver slice
- **After every plan wave:** Run targeted slice twice back-to-back
- **Before `/gsd-verify-work`:** Full targeted Phase 28 closure slice green
- **Max feedback latency:** <120 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 28-01-01 | 01 | 1 | TRDG-01, TRDG-02, TRDG-03 | contract/drag/diagnostic | `ctest --test-dir build -C Release -R "scene_solver_contract\|scene_solver_drag\|scene_solver_diagnostics" --output-on-failure` | ✅ | ⬜ pending |
| 28-01-02 | 01 | 1 | TRDG-01, TRDG-02, TRDG-03 | contract/drag/diagnostic | `ctest --test-dir build -C Release -R "scene_solver_contract\|scene_solver_drag\|scene_solver_diagnostics" --output-on-failure` | ✅ | ⬜ pending |
| 28-01-03 | 01 | 1 | TRDG-01, TRDG-02, TRDG-03 | deterministic rerun | `ctest --test-dir build -C Release -R "scene_solver_contract\|scene_solver_drag\|scene_solver_diagnostics\|scene_solver_pass_policy" --output-on-failure && ctest --test-dir build -C Release -R "scene_solver_contract\|scene_solver_drag\|scene_solver_diagnostics\|scene_solver_pass_policy" --output-on-failure` | ✅ | ⬜ pending |
| 28-02-01 | 02 | 2 | TRDG-04 | contract/pass-policy | `ctest --test-dir build -C Release -R "scene_solver_contract\|scene_solver_pass_policy" --output-on-failure` | ✅ | ⬜ pending |
| 28-02-02 | 02 | 2 | TRDG-04 | contract/pass-policy/drag/diagnostic | `ctest --test-dir build -C Release -R "scene_solver_contract\|scene_solver_pass_policy\|scene_solver_drag\|scene_solver_diagnostics" --output-on-failure` | ✅ | ⬜ pending |
| 28-02-03 | 02 | 2 | TRDG-04 | deterministic closure evidence | `ctest --test-dir build -C Release -R "scene_solver_contract\|scene_solver_pass_policy\|scene_solver_drag\|scene_solver_diagnostics" --output-on-failure && ctest --test-dir build -C Release -R "scene_solver_contract\|scene_solver_pass_policy\|scene_solver_drag\|scene_solver_diagnostics" --output-on-failure` | ✅ | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠ flaky*

---

## Wave 0 Requirements

- [ ] `src/tests/scene_solver_drag_test.c` — tangency shared/adjacent drag matrix fixtures.
- [ ] `src/tests/scene_solver_contract_test.c` — mirrored tangency parity fixtures.
- [ ] `src/tests/scene_solver_pass_policy_test.c` — mirrored rerun/order determinism assertions.
- [ ] `src/tests/scene_solver_diagnostics_test.c` — explicit tangency family diagnostics + post-failure responsiveness check.

---

## Manual-Only Verifications

All Phase 28 behaviors are expected to have automated verification through targeted solver suites.

---

## Validation Sign-Off

- [ ] All tasks have automated verify commands
- [ ] Sampling continuity preserved
- [ ] Wave 0 gaps resolved by test additions
- [ ] No watch-mode flags
- [ ] Feedback latency within target
- [ ] `nyquist_compliant: true` set in frontmatter before phase closure

**Approval:** pending

