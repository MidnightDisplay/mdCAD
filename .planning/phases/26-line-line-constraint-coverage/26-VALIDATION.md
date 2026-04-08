---
phase: 26
slug: line-line-constraint-coverage
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-04-08
---

# Phase 26 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | CTest + C unit executables |
| **Config file** | `src/CMakeLists.txt` (`include(CTest)` + `add_test(...)`) |
| **Quick run command** | `ctest --test-dir build -R "scene_solver_contract|endpoint_pick" --output-on-failure` |
| **Full suite command** | `ctest --test-dir build -R "scene_solver_contract|scene_solver_diagnostics|scene_solver_pass_policy|endpoint_pick|script_roundtrip" --output-on-failure` |
| **Estimated runtime** | ~120 seconds |

---

## Sampling Rate

- **After every task commit:** Run `ctest --test-dir build -R "scene_solver_contract|endpoint_pick" --output-on-failure`
- **After every plan wave:** Run `ctest --test-dir build -R "scene_solver_contract|scene_solver_diagnostics|scene_solver_pass_policy|endpoint_pick|script_roundtrip" --output-on-failure`
- **Before `/gsd-verify-work`:** Full suite must be green
- **Max feedback latency:** 120 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 26-01-01 | 01 | 1 | LCON-01 | contract | `ctest --test-dir build -R scene_solver_contract --output-on-failure` | ✅ (extend) | ⬜ pending |
| 26-01-02 | 01 | 1 | LCON-03 | contract | `ctest --test-dir build -R scene_solver_contract --output-on-failure` | ✅ (extend) | ⬜ pending |
| 26-01-03 | 01 | 1 | LCON-05 | legality/diagnostics | `ctest --test-dir build -R "endpoint_pick|scene_solver_diagnostics" --output-on-failure` | ✅ (extend) | ⬜ pending |
| 26-02-01 | 02 | 2 | LCON-02 | contract | `ctest --test-dir build -R scene_solver_contract --output-on-failure` | ✅ (extend) | ⬜ pending |
| 26-02-02 | 02 | 2 | LCON-04 | contract + determinism rerun | `ctest --test-dir build -R scene_solver_contract --output-on-failure` (twice) | ✅ (extend) | ⬜ pending |
| 26-02-03 | 02 | 2 | LCON-05 | script roundtrip | `ctest --test-dir build -R script_roundtrip --output-on-failure` | ✅ (extend) | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] `src/tests/scene_solver_contract_test.c` — add LCON parallel/perpendicular success + unsat transactional coverage
- [ ] `src/tests/scene_solver_diagnostics_test.c` — add family-specific unsat diagnostics assertions for `PARALLEL`/`PERPENDICULAR`
- [ ] `src/tests/endpoint_pick_test.c` — add multi-line perpendicular legality and explicit invalid-case coverage
- [ ] `src/tests/script_roundtrip_tests.c` — add `Parallel`/`Perpendicular` roundtrip coverage

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Constraint menu messaging clarity for invalid line-line selections | LCON-05 | Message readability and UI wording quality are best judged visually | In active sketch, make invalid mixed selection; open constraint menu; confirm explicit line-line legality reason shown (not generic/no-op) |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 120s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
