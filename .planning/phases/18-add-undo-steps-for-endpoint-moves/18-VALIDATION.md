---
phase: 18
slug: add-undo-steps-for-endpoint-moves
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-04-04
---

# Phase 18 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | CTest + native C test executables |
| **Config file** | `src/CMakeLists.txt` |
| **Quick run command** | `ctest -R endpoint_pick --test-dir build-vulkan -C Release --output-on-failure` |
| **Full suite command** | `ctest -R "endpoint_pick\|scene_solver_contract\|scene_solver_drag\|scene_solver_diagnostics\|script_roundtrip_tests" --test-dir build-vulkan -C Release --output-on-failure` |
| **Estimated runtime** | ~20 seconds (quick), ~120 seconds (full) |

---

## Sampling Rate

- **After every task commit:** Run `ctest -R "endpoint_pick|scene_solver_drag" --test-dir build-vulkan -C Release --output-on-failure`
- **After every plan wave:** Run `ctest -R "endpoint_pick\|scene_solver_contract\|scene_solver_drag\|scene_solver_diagnostics\|script_roundtrip_tests" --test-dir build-vulkan -C Release --output-on-failure`
- **Before `/gsd-verify-work`:** Full suite must be green
- **Max feedback latency:** 20 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 18-01-01 | 01 | 1 | PH18-01 | unit/integration | `ctest -R endpoint_pick --test-dir build-vulkan -C Release --output-on-failure` | ✅ | ⬜ pending |
| 18-01-02 | 01 | 1 | PH18-01 | unit/integration | `ctest -R endpoint_pick --test-dir build-vulkan -C Release --output-on-failure` | ✅ | ⬜ pending |
| 18-02-01 | 02 | 2 | PH18-02 | unit/integration | `ctest -R "endpoint_pick\|scene_solver_contract\|scene_solver_diagnostics" --test-dir build-vulkan -C Release --output-on-failure` | ✅ | ⬜ pending |
| 18-02-02 | 02 | 2 | PH18-02 | unit/integration | `ctest -R "endpoint_pick\|scene_solver_contract\|scene_solver_diagnostics" --test-dir build-vulkan -C Release --output-on-failure` | ✅ | ⬜ pending |
| 18-03-01 | 03 | 3 | PH18-03 | regression | `ctest -R "endpoint_pick\|scene_solver_contract\|scene_solver_drag\|scene_solver_diagnostics\|script_roundtrip_tests" --test-dir build-vulkan -C Release --output-on-failure` | ✅ | ⬜ pending |
| 18-03-02 | 03 | 3 | PH18-03 | regression | `ctest -R "endpoint_pick\|script_roundtrip_tests" --test-dir build-vulkan -C Release --output-on-failure` | ✅ | ⬜ pending |
| 18-03-03 | 03 | 3 | PH18-03 | manual checkpoint | `ctest -R endpoint_pick --test-dir build-vulkan -C Release --output-on-failure` | ✅ | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- Existing infrastructure covers all phase requirements.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Endpoint drag + Ctrl+Z/Redo feel and granularity in viewport | PH18-01 | UX-level granularity/coalescing is best judged interactively | Move one endpoint across multiple frames, release drag, press undo once; verify full drag reverts in one step and redo restores it. |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 120s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
