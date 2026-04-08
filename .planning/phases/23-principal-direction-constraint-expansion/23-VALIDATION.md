---
phase: 23
slug: principal-direction-constraint-expansion
status: complete
nyquist_compliant: true
wave_0_complete: true
created: 2026-04-08
---

# Phase 23 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | CTest + native C test executables |
| **Config file** | `CMakeLists.txt`, `src/CMakeLists.txt` |
| **Quick run command** | `ctest -R "endpoint_pick|scene_solver_contract|scene_solver_pass_policy" --test-dir build-vulkan -C Release --output-on-failure` |
| **Full suite command** | `ctest -R "scene_solver_.*|endpoint_pick|script_roundtrip_tests" --test-dir build-vulkan -C Release --output-on-failure` |
| **Estimated runtime** | quick <180s, full suite variable |

---

## Sampling Rate

- **After every task commit:** Run `ctest -R "endpoint_pick|scene_solver_contract|scene_solver_pass_policy" --test-dir build-vulkan -C Release --output-on-failure`
- **After every plan wave:** Run `ctest -R "scene_solver_.*|endpoint_pick|script_roundtrip_tests" --test-dir build-vulkan -C Release --output-on-failure`
- **Before `/gsd-verify-work`:** Full targeted suite must be green
- **Max feedback latency:** 180 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 23-01-01 | 01 | 1 | AXIS-01, AXIS-02, AXIS-03, AXIS-04 | legality/contract | `ctest -R endpoint_pick --test-dir build-vulkan -C Release --output-on-failure` | ✅ | ✅ green |
| 23-01-02 | 01 | 1 | AXIS-01, AXIS-02, AXIS-03 | solver/contract | `ctest -R scene_solver_contract --test-dir build-vulkan -C Release --output-on-failure` | ✅ | ✅ green |
| 23-02-01 | 02 | 2 | SRLV-04 | integration/behavior | `ctest -R "scene_solver_contract|scene_solver_pass_policy" --test-dir build-vulkan -C Release --output-on-failure` | ✅ | ✅ green |
| 23-02-02 | 02 | 2 | SRLV-04, AXIS-01, AXIS-02, AXIS-03, AXIS-04 | targeted gate | `ctest -R "endpoint_pick|scene_solver_contract|scene_solver_pass_policy|scene_solver_trigger|scene_solver_diagnostics|scene_solver_drag" --test-dir build-vulkan -C Release --output-on-failure` | ✅ | ✅ green |
| 23-03-01 | 03 | 3 | AXIS-01, AXIS-02, AXIS-03, AXIS-04, SRLV-04 | regression + UX acceptance gate | `ctest -R "endpoint_pick|scene_solver_contract|scene_solver_pass_policy|scene_solver_trigger|scene_solver_diagnostics|scene_solver_drag" --test-dir build-vulkan -C Release --output-on-failure` | ✅ | ✅ green |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [x] `src/tests/endpoint_pick_test.c` — legality matrix expansion for `ALONG X/Y/Z` over standalone points + endpoint/center roles.
- [x] `src/tests/scene_solver_contract_test.c` — deterministic ALONG axis projection behavior and ALONG+ANGLE coexistence assertions.
- [x] `src/tests/scene_solver_pass_policy_test.c` — bounded pass behavior when ALONG constraints are active.
- [x] `src/constraints/constraint_types.h` legality changes are covered by executable tests before runtime plan completion.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Constraint menu exposes ALONG X/Y/Z for valid point-pair/group signatures including line endpoints and arc landmarks | AXIS-01, AXIS-02, AXIS-03, AXIS-04 | Selection legality + menu visibility over mixed interactive picks is UX-level behavior | In sketch mode, select valid point pairs/groups (standalone points, line endpoints, arc endpoints/center) and confirm ALONG X/Y/Z options appear only for legal signatures. |
| Recalculate remains functional when ALONG and ANGLE constraints coexist in one sketch | SRLV-04 | Requires user-observed interactive solve outcomes and diagnostics visibility | Author a sketch combining ALONG and ANGLE constraints, run recalculate repeatedly, verify deterministic outcomes and explicit diagnostics on unsatisfied setups. |

---

## Validation Sign-Off

- [x] All tasks have `<automated>` verify or explicit Wave 0 dependencies
- [x] Sampling continuity: no 3 consecutive tasks without automated verify
- [x] Wave 0 captures directional legality + solver coexistence gaps
- [x] No watch-mode flags
- [x] Feedback latency < 180s in targeted runs
- [x] `nyquist_compliant: true` set in frontmatter

**Approval:** complete (user accepted final manual checks and standalone-point untabbed gizmo behavior fix)

