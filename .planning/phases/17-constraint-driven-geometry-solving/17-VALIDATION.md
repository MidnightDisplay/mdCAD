---
phase: 17
slug: constraint-driven-geometry-solving
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-04-02
---

# Phase 17 — Validation Strategy

## Test Infrastructure

| Property | Value |
|---|---|
| Framework | CTest + C test executables |
| Config file | `src/CMakeLists.txt` |
| Quick run command | `ctest -R "solver|script_roundtrip" --test-dir build-vulkan -C Release --output-on-failure` |
| Full suite command | `ctest --test-dir build-vulkan -C Release --output-on-failure` |
| Estimated runtime | ~10-60s (environment dependent) |

## Sampling Rate

- After every task commit: run quick command.
- After every plan wave: run full suite command.
- Before `/gsd-verify-work`: full suite must be green.

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---|---|---|---|---|---|---|---|
| 17-01-01 | 01 | 1 | D-01,D-02,D-05 | contract | `ctest -R scene_solver_contract --test-dir build-vulkan -C Release --output-on-failure` | ❌ W0 | ⬜ pending |
| 17-02-01 | 02 | 1 | D-03,D-04,D-07 | integration | `ctest -R scene_solver_drag --test-dir build-vulkan -C Release --output-on-failure` | ❌ W0 | ⬜ pending |
| 17-03-01 | 03 | 2 | D-09,D-10,D-11,D-12 | integration | `ctest -R endpoint_pick --test-dir build-vulkan -C Release --output-on-failure` | ❌ W0 | ⬜ pending |
| 17-03-02 | 03 | 2 | D-06,D-08 | unit/fixture | `ctest -R scene_solver_diagnostics --test-dir build-vulkan -C Release --output-on-failure` | ❌ W0 | ⬜ pending |

## Wave 0 Requirements

- [ ] `src/tests/scene_solver_contract_test.c` (or equivalent) — deterministic solve fixture contract tests.
- [ ] `src/tests/scene_solver_drag_test.c` (or equivalent) — live drag + last-valid-state semantics.
- [ ] `src/tests/endpoint_pick_test.c` (or equivalent) — endpoint selection/pick layering checks.
- [ ] `src/tests/scene_solver_diagnostics_test.c` (or equivalent) — dedupe + deterministic diagnostics ordering checks.
- [ ] CMake test registration entries for new tests in `src/CMakeLists.txt`.

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|---|---|---|---|
| Coincident endpoint authoring for open chains/closed loops feels usable in viewport | D-09..D-12 | UX and interaction feel | In app, create multiple lines/arcs, select endpoints directly (no Tab vertex mode), apply Coincident, verify chain/loop authoring is reliable. |
| Unsat drag feedback clarity in viewport | D-04,D-05 | Visual feedback quality | Force unsat movement and verify no partial mutation + immediate understandable feedback. |

## Validation Sign-Off

- [ ] All planned tasks have verify steps or Wave 0 dependencies.
- [ ] Deterministic fixture coverage exists for D-01..D-12.
- [ ] No unresolved missing test references.
- [ ] `nyquist_compliant: true` set when checks are fully wired.

Approval: pending
