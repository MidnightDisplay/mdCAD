---
phase: 18
slug: add-undo-steps-for-endpoint-moves
status: in_review
nyquist_compliant: true
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
| 18-01-01 | 01 | 1 | PH18-01 | unit/integration | `ctest -R endpoint_pick --test-dir build-vulkan -C Release --output-on-failure` | ✅ | ✅ green |
| 18-01-02 | 01 | 1 | PH18-01 | unit/integration | `ctest -R endpoint_pick --test-dir build-vulkan -C Release --output-on-failure` | ✅ | ✅ green |
| 18-02-01 | 02 | 2 | PH18-02 | unit/integration | `ctest -R "endpoint_pick\|scene_solver_contract\|scene_solver_diagnostics" --test-dir build-vulkan -C Release --output-on-failure` | ✅ | ✅ green |
| 18-02-02 | 02 | 2 | PH18-02 | unit/integration | `ctest -R "endpoint_pick\|scene_solver_contract\|scene_solver_diagnostics" --test-dir build-vulkan -C Release --output-on-failure` | ✅ | ✅ green |
| 18-03-01 | 03 | 3 | PH18-03 | regression | `ctest -R "endpoint_pick\|scene_solver_contract\|scene_solver_drag\|scene_solver_diagnostics\|script_roundtrip_tests" --test-dir build-vulkan -C Release --output-on-failure` | ✅ | ✅ green |
| 18-03-02 | 03 | 3 | PH18-03 | regression | `ctest -R "endpoint_pick\|script_roundtrip_tests" --test-dir build-vulkan -C Release --output-on-failure` | ✅ | ✅ green |
| 18-03-03 | 03 | 3 | PH18-03 | manual checkpoint | `ctest -R endpoint_pick --test-dir build-vulkan -C Release --output-on-failure` | ✅ | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- Existing infrastructure covers all phase requirements.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Endpoint drag + Ctrl+Z/Redo feel and granularity in viewport | PH18-03 | UX-level granularity/coalescing is best judged interactively | Move one endpoint across multiple frames, release drag, press undo once; verify full drag reverts in one step and redo restores it. |
| Non-sketch line move undo/redo parity check | PH18-03 | Confirms no endpoint-only replay leakage to normal transform entities | Move a non-sketch line, undo once, redo once; verify behavior matches baseline transform undo semantics. |

---

## Requirement Evidence (PH18-01..PH18-03)

| Requirement | Evidence | Status |
|-------------|----------|--------|
| PH18-01 | Endpoint command contract + drag/inspector endpoint commit boundary tests in `18-01-SUMMARY.md`; `endpoint_pick` remains green in latest reruns. | ✅ |
| PH18-02 | Endpoint replay sync + sketch-gated side-effect coverage in `18-02-SUMMARY.md`; `endpoint_pick`, `scene_solver_contract`, `scene_solver_diagnostics` green in latest reruns. | ✅ |
| PH18-03 | New non-sketch drag-end legacy-path + endpoint drag release-only coalescing regressions and script transaction invariants; command evidence captured below. | ✅ (automated) |

## Command Evidence (2026-04-04)

1. `ctest -R "endpoint_pick|scene_solver_drag" --test-dir build-vulkan -C Release --output-on-failure`  
   Result: **Passed** (2/2)
2. `ctest -R "endpoint_pick|script_roundtrip_tests" --test-dir build-vulkan -C Release --output-on-failure`  
   Result: **Passed** (2/2)
3. `ctest -R "endpoint_pick|scene_solver_contract|scene_solver_drag|scene_solver_diagnostics|script_roundtrip_tests" --test-dir build-vulkan -C Release --output-on-failure`  
   Result: **Passed** (5/5)

Checkpoint prerequisite status: ✅ Automated verifies complete for 18-03 Task 3 handoff.

---

## Validation Sign-Off

- [x] All tasks have `<automated>` verify or Wave 0 dependencies
- [x] Sampling continuity: no 3 consecutive tasks without automated verify
- [x] Wave 0 covers all MISSING references
- [x] No watch-mode flags
- [x] Feedback latency < 120s
- [x] `nyquist_compliant: true` set in frontmatter

**Approval:** pending human checkpoint (18-03-03)
