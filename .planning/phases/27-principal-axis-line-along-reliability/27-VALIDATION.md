---
phase: 27
slug: principal-axis-line-along-reliability
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-04-09
---

# Phase 27 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | CTest + C unit executables |
| **Config file** | `src/CMakeLists.txt` (`include(CTest)` + `add_test(...)`) |
| **Quick run command** | `ctest --test-dir build -C Release -R "scene_solver_contract|endpoint_pick|scene_solver_diagnostics" --output-on-failure` |
| **Full suite command** | `ctest --test-dir build -C Release -R "scene_solver_contract|scene_solver_diagnostics|scene_solver_pass_policy|endpoint_pick|script_roundtrip_tests|scene_solver_trigger|scene_solver_drag" --output-on-failure` |
| **Estimated runtime** | ~180 seconds |

---

## Sampling Rate

- **After every task commit:** Run `ctest --test-dir build -C Release -R "scene_solver_contract|endpoint_pick|scene_solver_diagnostics" --output-on-failure`
- **After every plan wave:** Run `ctest --test-dir build -C Release -R "scene_solver_contract|scene_solver_diagnostics|scene_solver_pass_policy|endpoint_pick|script_roundtrip_tests|scene_solver_trigger|scene_solver_drag" --output-on-failure`
- **Before `/gsd-verify-work`:** Full suite must be green
- **Max feedback latency:** 180 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 27-01-01 | 01 | 1 | ALIN-01 | contract | `ctest --test-dir build -C Release -R scene_solver_contract --output-on-failure` | ✅ (extend) | ⬜ pending |
| 27-01-02 | 01 | 1 | ALIN-02 | contract | `ctest --test-dir build -C Release -R scene_solver_contract --output-on-failure` | ✅ (extend) | ⬜ pending |
| 27-01-03 | 01 | 1 | ALIN-03 | contract | `ctest --test-dir build -C Release -R scene_solver_contract --output-on-failure` | ✅ (extend) | ⬜ pending |
| 27-02-01 | 02 | 2 | ALIN-04 | contract + determinism | `ctest --test-dir build -C Release -R "scene_solver_contract|scene_solver_pass_policy" --output-on-failure` | ✅ (extend) | ⬜ pending |
| 27-02-02 | 02 | 2 | ALIN-04 | diagnostics | `ctest --test-dir build -C Release -R "scene_solver_diagnostics|endpoint_pick" --output-on-failure` | ✅ (extend) | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] `src/tests/scene_solver_contract_test.c` — add ALIN-01/02/03 single-line ALONG line feasibility tests
- [ ] `src/tests/scene_solver_contract_test.c` — add ALIN-04 mixed `ALONG` + `LENGTH` + `ANGLE` + connectivity determinism tests
- [ ] `src/tests/scene_solver_diagnostics_test.c` — add ALONG X/Y/Z family-specific unsatisfied diagnostics assertions
- [ ] Ensure local `build` tree has required test targets before execution

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Constraint menu clarity for invalid ALONG signatures | ALIN-01..03 | Messaging readability is best judged visually | In active sketch, perform invalid directional selections; verify explicit legality feedback and no silent no-op |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 180s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending