---
phase: 17
slug: constraint-driven-geometry-solving
status: compliant
nyquist_compliant: true
wave_0_complete: true
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
| 17-01-01 | 01 | 1 | D-01,D-02,D-05 | contract | `ctest -R scene_solver_contract --test-dir build-vulkan -C Release --output-on-failure` | ✅ present | ✅ pass |
| 17-02-01 | 02 | 1 | D-03,D-04,D-07 | integration | `ctest -R scene_solver_drag --test-dir build-vulkan -C Release --output-on-failure` | ✅ present | ✅ pass |
| 17-03-01 | 03 | 2 | D-09,D-10,D-11,D-12 | integration | `ctest -R endpoint_pick --test-dir build-vulkan -C Release --output-on-failure` | ✅ present | ✅ pass |
| 17-03-02 | 03 | 2 | D-06,D-08 | unit/fixture | `ctest -R scene_solver_diagnostics --test-dir build-vulkan -C Release --output-on-failure` | ✅ present | ✅ pass |
| 17-03-03 | 03 | 2 | D-01..D-12 regression gate | suite | `ctest -R "scene_solver_contract|scene_solver_drag|endpoint_pick|scene_solver_diagnostics" --test-dir build-vulkan -C Release --output-on-failure` | ✅ present | ✅ pass |

## Wave 0 Requirements

- [x] `src/tests/scene_solver_contract_test.c` (or equivalent) — deterministic solve fixture contract tests.
- [x] `src/tests/scene_solver_drag_test.c` (or equivalent) — live drag + last-valid-state semantics.
- [x] `src/tests/endpoint_pick_test.c` (or equivalent) — endpoint selection/pick layering checks.
- [x] `src/tests/scene_solver_diagnostics_test.c` (or equivalent) — dedupe + deterministic diagnostics ordering checks.
- [x] CMake test registration entries for new tests in `src/CMakeLists.txt`.

## Decision-to-Evidence Traceability (D-01..D-12)

| Decision | Validation Type | Evidence |
|---|---|---|
| D-01 full v1.2 constraint enforcement scope | Automated | `scene_solver_contract` transactional solve tests + full suite run |
| D-02 solved geometry applies immediately | Automated | `scene_solver_contract` commit-on-success assertions |
| D-03 live constrained drag with projection | Automated | `scene_solver_drag` feasible drag projection assertions |
| D-04 unsat drag preserves last valid solved state | Automated + Manual | `scene_solver_drag` unsat no-mutation assertion; viewport unsat interaction check |
| D-05 deterministic failure implication, no mutation | Automated + Manual | `scene_solver_contract`/`scene_solver_diagnostics` deterministic implication ordering + viewport failure clarity check |
| D-06 diagnostics append + consecutive dedupe + clear | Automated + Manual | `scene_solver_diagnostics` dedupe test; inspector explicit clear action UX |
| D-07 bounded per-frame solve budget degrade path | Automated | `scene_solver_drag` bounded projection expectation (`projected_delta` clamp) |
| D-08 deterministic regression fixtures/tolerances | Automated | `scene_solver_contract|scene_solver_drag|endpoint_pick|scene_solver_diagnostics` aggregate CTest gate |
| D-09 endpoint selection in normal viewport flow | Automated + Manual | `endpoint_pick` test + direct endpoint selection workflow check |
| D-10 endpoint controls first-class pickable sub-elements | Automated + Manual | `endpoint_pick` pick priority checks + manual hover/select confirmation |
| D-11 coincident endpoint-to-endpoint authoring path | Automated + Manual | `endpoint_pick` + `scene_solver_contract` participant/constraint path checks; chain/loop authoring UAT |
| D-12 endpoint points rendered/picked above primitives | Automated + Manual | `endpoint_pick` layering assertions + viewport pick reliability check |

## Executed Evidence Commands (Wave 0 + Plan 03)

- `ctest -R scene_solver_drag --test-dir build-vulkan -C Release --output-on-failure`
- `ctest -R scene_solver_diagnostics --test-dir build-vulkan -C Release --output-on-failure`
- `ctest -R "scene_solver_contract|scene_solver_drag|endpoint_pick|scene_solver_diagnostics" --test-dir build-vulkan -C Release --output-on-failure`
- `ctest -R "solver|script_roundtrip" --test-dir build-vulkan -C Release --output-on-failure`
- `ctest --test-dir build-vulkan -C Release --output-on-failure`

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|---|---|---|---|
| Coincident endpoint authoring for open chains/closed loops feels usable in viewport | D-09..D-12 | UX and interaction feel | In app, create multiple lines/arcs, select endpoints directly (no Tab vertex mode), apply Coincident, verify chain/loop authoring is reliable. |
| Unsat drag feedback clarity in viewport | D-04,D-05 | Visual feedback quality | Force unsat movement and verify no partial mutation + immediate understandable feedback. |

## Checkpoint Failure Addendum (2026-04-03, Plan 17-02 Task 3)

**Result:** ❌ Human verification failed (automated `endpoint_pick` remained green).

**Observed failures (repro from user verification):**
- Endpoint points are not visibly rendered in normal viewport flow; discoverable only via pick IDs or Tab gizmo vertex mode.
- Left-click near endpoint unexpectedly opens constraint context menu as if point-selected + C-triggered.
- Single endpoint selection exposes invalid line-oriented options for point participants (e.g., Fixed/Along X/Y/Z/Length context mismatch).

**Disposition:** Keep D-12 and full D-09..D-12 UX row open pending redesign/reimplementation.

**Planning handoff recommendation:** `/gsd-plan-phase 17 --gaps` with scope to transition from synthetic endpoint tail-range picks to sketch-scoped native point entities synchronized from line/arc notable vertices.

## Gap-Closure Remediation Evidence (2026-04-03, Plan 17-05 follow-up)

Automated verification after crash/sync remediation and endpoint geometry-edit sync wiring:

- `ctest -R endpoint_pick --test-dir build-vulkan -C Release --output-on-failure` ✅ pass
- `ctest -R "scene_solver_contract|scene_solver_drag|scene_solver_diagnostics" --test-dir build-vulkan -C Release --output-on-failure` ✅ pass

Additional regression coverage added:

- `test_endpoint_direct_geometry_edit_syncs_owner_and_entities` in `src/tests/endpoint_pick_test.c`

Manual checkpoint status (historical, superseded by final closure):

- This Plan 17-05 note captured an intermediate state before final Phase 20 closure reconciliation.
- Canonical manual proof remains `17-UAT.md` (`7/7` pass), and authoritative final requirement disposition is now tracked in `17-VERIFICATION.md`.

## Final Closure Alignment (2026-04-07, Plan 20-03)

- `17-VERIFICATION.md` is authoritative for final `D-01..D-12` closure state.
- `17-UAT.md` remains the canonical manual evidence artifact (Tests 1-7 pass).
- Final targeted closure gate command for audit reproducibility:
  - `ctest -R "scene_solver_contract|scene_solver_drag|endpoint_pick|scene_solver_diagnostics" --test-dir build-vulkan -C Release --output-on-failure`

## Validation Sign-Off

- [x] All planned tasks have verify steps or Wave 0 dependencies.
- [x] Deterministic fixture coverage exists for D-01..D-12.
- [x] No unresolved missing test references.
- [x] Human verification closure for D-09..D-12 endpoint UX is satisfied via canonical `17-UAT.md` baseline and finalized in `17-VERIFICATION.md`.
- [x] `nyquist_compliant: true` set when checks are fully wired.

Approval: final — supporting validation artifact is aligned with authoritative `17-VERIFICATION.md` closure for `D-01..D-12`.
