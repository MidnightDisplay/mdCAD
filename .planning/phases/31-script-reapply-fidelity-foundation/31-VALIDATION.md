---
phase: 31
slug: script-reapply-fidelity-foundation
status: draft
nyquist_compliant: true
wave_0_complete: false
created: 2026-04-10
---

# Phase 31 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | CTest + C executable tests (repo-native) |
| **Config file** | `CMakeLists.txt`, `src/CMakeLists.txt` |
| **Fast smoke command (<30s target)** | `ctest --test-dir build-vulkan -C Release -R "script_roundtrip_tests" --output-on-failure` |
| **Quick run command** | `ctest --test-dir build-vulkan -C Release -R "script_roundtrip_tests\|scene_solver_diagnostics" --output-on-failure` |
| **Full suite command** | `ctest --test-dir build-vulkan -C Release -R "scene_solver_contract\|scene_solver_pass_policy\|scene_solver_diagnostics\|scene_solver_trigger\|scene_solver_drag\|endpoint_pick\|script_roundtrip_tests" --output-on-failure` |
| **Estimated fast smoke runtime** | <30 seconds target |
| **Estimated quick/full runtime** | ~60 seconds |

---

## Sampling Rate

- **After every task commit:** Run `ctest --test-dir build-vulkan -C Release -R "script_roundtrip_tests" --output-on-failure` (fast smoke)
- **After every plan wave:** Run `ctest --test-dir build-vulkan -C Release -R "scene_solver_contract\|scene_solver_pass_policy\|scene_solver_diagnostics\|scene_solver_trigger\|scene_solver_drag\|endpoint_pick\|script_roundtrip_tests" --output-on-failure`
- **Before `/gsd-verify-work`:** Full suite must be green
- **Max feedback latency (per-task smoke):** <30 seconds target
- **Wave/phase gate latency budget:** ~60 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 31-01-01 | 01 | 1 | SCRI-01 | unit/integration | `ctest --test-dir build-vulkan -C Release -R "script_roundtrip_tests" --output-on-failure` | ✅ | ⬜ pending |
| 31-01-02 | 01 | 1 | SCRI-02 | unit/integration | `ctest --test-dir build-vulkan -C Release -R "script_roundtrip_tests" --output-on-failure` | ✅ | ⬜ pending |
| 31-02-01 | 02 | 2 | SCRI-03 | integration | `ctest --test-dir build-vulkan -C Release -R "script_roundtrip_tests" --output-on-failure` | ✅ | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] `src/tests/script_roundtrip_tests.c` — add descriptor-role/sub-index roundtrip coverage and color roundtrip coverage.
- [ ] `src/tests/scene_solver_diagnostics_test.c` — add deterministic diagnostics parity assertions under repeated script reapply.
- [ ] Existing infrastructure covers framework/tooling; no test framework installation required.

---

## Manual-Only Verifications

All phase behaviors have automated verification.

---

## Validation Sign-Off

- [x] All tasks have `<automated>` verify or Wave 0 dependencies
- [x] Sampling continuity: no 3 consecutive tasks without automated verify
- [x] Wave 0 covers all MISSING references
- [x] No watch-mode flags
- [x] Per-task fast smoke feedback latency target < 30s
- [x] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
