---
phase: 25
slug: regression-and-reliability-closure
status: complete
nyquist_compliant: true
wave_0_complete: true
created: 2026-04-08
---

# Phase 25 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | CTest + native C test executables |
| **Config file** | `src/CMakeLists.txt` (CTest targets) |
| **Quick run command** | `ctest --test-dir build-vulkan -C Release -R "scene_solver_trigger|scene_solver_pass_policy|scene_solver_diagnostics" --output-on-failure` |
| **Full suite command** | `ctest --test-dir build-vulkan -C Release -R "scene_solver_contract|scene_solver_pass_policy|scene_solver_diagnostics|scene_solver_trigger|scene_solver_drag|endpoint_pick|script_roundtrip_tests" --output-on-failure` |
| **Estimated runtime** | quick <180s, full targeted gate variable |

---

## Sampling Rate

- **After every task commit:** Run quick run command
- **After every plan wave:** Run full suite command
- **Before `/gsd-verify-work`:** Full suite must be green
- **Max feedback latency:** 180 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 25-01-01 | 01 | 1 | V13-01 | trigger reliability gate | `ctest --test-dir build-vulkan -C Release -R "scene_solver_trigger" --output-on-failure` | ✅ | ✅ green |
| 25-01-02 | 01 | 1 | V13-01 | pass-policy reliability gate | `ctest --test-dir build-vulkan -C Release -R "scene_solver_pass_policy" --output-on-failure` | ✅ | ✅ green |
| 25-02-01 | 02 | 2 | V13-01 | legality/determinism/diagnostics gate | `ctest --test-dir build-vulkan -C Release -R "scene_solver_contract|scene_solver_diagnostics|endpoint_pick|scene_solver_drag|script_roundtrip_tests" --output-on-failure` | ✅ | ✅ green |
| 25-02-02 | 02 | 2 | V13-01 | closure full gate (fresh rerun) | `ctest --test-dir build-vulkan -C Release -R "scene_solver_contract|scene_solver_pass_policy|scene_solver_diagnostics|scene_solver_trigger|scene_solver_drag|endpoint_pick|script_roundtrip_tests" --output-on-failure` | ✅ | ✅ green |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [x] Existing infrastructure covers all phase requirements.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Regression-specific interaction confirmation when automated failure is ambiguous | V13-01 | Needed only if a discovered regression cannot be confidently resolved from automation output alone | Run focused interactive repro only for the failing behavior, then rerun automated gate to confirm closure. |

---

## Validation Sign-Off

- [x] All tasks have `<automated>` verify or Wave 0 dependencies
- [x] Sampling continuity: no 3 consecutive tasks without automated verify
- [x] Wave 0 covers all MISSING references
- [x] No watch-mode flags
- [x] Feedback latency < 180s
- [x] `nyquist_compliant: true` set in frontmatter

**Approval:** complete (focused trigger/pass-policy rerun + full 7-test baseline and fresh rerun all green)

