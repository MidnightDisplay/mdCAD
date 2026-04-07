---
phase: 22
slug: solver-trigger-recalculate-determinism
status: draft
nyquist_compliant: true
wave_0_complete: false
created: 2026-04-07
---

# Phase 22 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | CTest + native C test executables |
| **Config file** | `CMakeLists.txt`, `src/CMakeLists.txt` |
| **Quick run command** | `ctest -R "scene_solver_contract|scene_solver_diagnostics|scene_solver_drag" --test-dir build-vulkan -C Release --output-on-failure` |
| **Full suite command** | `ctest --test-dir build-vulkan -C Release --output-on-failure` |
| **Estimated runtime** | quick <180s, full suite variable |

---

## Sampling Rate

- **After every task commit:** Run `ctest -R "scene_solver_contract|scene_solver_diagnostics|scene_solver_drag" --test-dir build-vulkan -C Release --output-on-failure`
- **After every plan wave:** Run `ctest -R "scene_solver_.*|endpoint_pick|script_roundtrip_tests" --test-dir build-vulkan -C Release --output-on-failure`
- **Before `/gsd-verify-work`:** Full suite must be green.
- **Max feedback latency:** 180 seconds.

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 22-01-01 | 01 | 1 | SRLV-01 | unit/contract | `ctest -R scene_solver_trigger --test-dir build-vulkan -C Release --output-on-failure` | ❌ W0 | ⬜ pending |
| 22-01-02 | 01 | 1 | SRLV-01, SRLV-02 | integration/behavior | `ctest -R "scene_solver_trigger|scene_solver_drag" --test-dir build-vulkan -C Release --output-on-failure` | ❌ W0 (trigger), ✅ (drag) | ⬜ pending |
| 22-02-01 | 02 | 1 | SRLV-03 | unit/contract | `ctest -R scene_solver_pass_policy --test-dir build-vulkan -C Release --output-on-failure` | ❌ W0 | ⬜ pending |
| 22-02-02 | 02 | 1 | SRLV-03, SRLV-02 | unit/regression | `ctest -R "scene_solver_pass_policy|scene_solver_contract" --test-dir build-vulkan -C Release --output-on-failure` | ❌ W0 (pass_policy), ✅ (contract) | ⬜ pending |
| 22-03-01 | 03 | 2 | SRLV-05 | unit/contract | `ctest -R scene_solver_contract --test-dir build-vulkan -C Release --output-on-failure` | ✅ (needs extension) | ⬜ pending |
| 22-03-02 | 03 | 2 | SRLV-01, SRLV-02, SRLV-03, SRLV-05 | targeted gate | `ctest -R "scene_solver_trigger|scene_solver_pass_policy|scene_solver_contract|scene_solver_diagnostics|scene_solver_drag" --test-dir build-vulkan -C Release --output-on-failure` | ❌ W0 (new tests) | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] `src/tests/scene_solver_trigger_test.c` — debounce 50ms, coalescing, manual queue-clear semantics for SRLV-01.
- [ ] `src/tests/scene_solver_pass_policy_test.c` — tolerance stop vs max-pass stop, default pass cap 10, explicit max-pass diagnostics for SRLV-03.
- [ ] Extend `src/tests/scene_solver_contract_test.c` for deterministic repeated recalc and LENGTH/ANGLE atomic-success/no-mutation-failure assertions (SRLV-02, SRLV-05).
- [ ] Register new test binaries in `src/CMakeLists.txt` with `add_executable(...)` and `add_test(...)`.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Inspector exposes position tolerance, angle tolerance, and max pass controls with expected defaults | SRLV-03 | UI control visibility/usability confirmation is interaction-level | In Entity Inspector solver section, verify all three controls are present, max pass defaults to `10`, and edits persist in sketch solver config for recalc runs. |
| Manual Recalculate is authoritative and clears pending auto queue | SRLV-01, SRLV-02 | Requires timing-sensitive interactive flow validation | Queue rapid geometry edits with Auto-solve enabled, click Recalculate once, verify one immediate solve result and no stale delayed solve effect afterward. |
| Unsatisfiable driving LENGTH/ANGLE reports explicit error and implication without geometry mutation | SRLV-05 | Failure implication visibility and no-visual-drift check are user-facing | Create a contradictory LENGTH/ANGLE setup, run recalc, verify explicit diagnostic text and implication highlighting while measured geometry remains unchanged. |

---

## Validation Sign-Off

- [x] All tasks have `<automated>` verify or explicit Wave 0 dependencies
- [x] Sampling continuity: no 3 consecutive tasks without automated verify
- [x] Wave 0 captures all currently missing test infrastructure
- [x] No watch-mode flags
- [x] Feedback latency < 180s in targeted runs
- [x] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
