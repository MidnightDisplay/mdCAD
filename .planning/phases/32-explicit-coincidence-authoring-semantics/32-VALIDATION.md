---
phase: 32
slug: explicit-coincidence-authoring-semantics
status: draft
nyquist_compliant: true
wave_0_complete: true
created: 2026-04-10
---

# Phase 32 — Validation Strategy

> Per-phase validation contract for explicit coincidence authoring semantics (`COIN-01`, `COIN-02`).

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | CTest + native C executable tests (repo-native) |
| **Config file** | `CMakeLists.txt`, `src/CMakeLists.txt` |
| **Fast smoke command (<30s target)** | `ctest --test-dir build-vulkan -C Release -R "scene_solver_contract" --output-on-failure` |
| **Quick run command** | `ctest --test-dir build-vulkan -C Release -R "scene_solver_contract|scene_solver_diagnostics|script_roundtrip_tests" --output-on-failure` |
| **Full suite command** | `ctest --test-dir build-vulkan -C Release -R "scene_solver_contract|scene_solver_pass_policy|scene_solver_diagnostics|scene_solver_trigger|scene_solver_drag|endpoint_pick|script_roundtrip_tests" --output-on-failure` |
| **Estimated fast smoke runtime** | <30 seconds target |
| **Estimated quick/full runtime** | ~60 seconds |

---

## Sampling Rate

- **After every task commit:** Run `ctest --test-dir build-vulkan -C Release -R "scene_solver_contract" --output-on-failure` (fast smoke)
- **After every plan wave:** Run `ctest --test-dir build-vulkan -C Release -R "scene_solver_contract|scene_solver_diagnostics|script_roundtrip_tests" --output-on-failure` (quick)
- **Before `/gsd-verify-work`:** Full suite must be green
- **Max feedback latency (per-task smoke):** <30 seconds target
- **Wave/phase gate latency budget:** ~60 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 32-01-01 | 01 | 1 | COIN-01 | unit/integration | `ctest --test-dir build-vulkan -C Release -R "scene_solver_contract" --output-on-failure` | ✅ | ⬜ pending |
| 32-01-02 | 01 | 1 | COIN-01 | diagnostics | `ctest --test-dir build-vulkan -C Release -R "scene_solver_diagnostics" --output-on-failure` | ✅ | ⬜ pending |
| 32-02-01 | 02 | 2 | COIN-02 | unit/integration | `ctest --test-dir build-vulkan -C Release -R "scene_solver_contract|scene_solver_drag" --output-on-failure` | ✅ | ⬜ pending |
| 32-02-02 | 02 | 2 | COIN-01, COIN-02 | script roundtrip | `ctest --test-dir build-vulkan -C Release -R "script_roundtrip_tests" --output-on-failure` | ✅ | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [x] Existing CTest solver/script harnesses cover phase requirement surfaces.
- [x] No new test framework/tool installation required.
- [x] Existing infrastructure covers all phase requirements.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Constraint menu authoring UX confirms explicit pair semantics visible and stable across edits | COIN-01, COIN-02 | Requires interactive viewport + menu interaction not fully represented in headless tests | Author ArcAxisLine and line-end/arc-end tangency from UI, confirm explicit coincidence appears in Script Editor and remains stable through edits/deletions per context decisions. |

---

## Validation Sign-Off

- [x] All tasks have `<automated>` verify or Wave 0 dependencies
- [x] Sampling continuity: no 3 consecutive tasks without automated verify
- [x] Wave 0 covers all MISSING references
- [x] No watch-mode flags
- [x] Per-task fast smoke feedback latency target < 30s
- [x] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
