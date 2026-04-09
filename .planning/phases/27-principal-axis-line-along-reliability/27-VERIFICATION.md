---
phase: 27-principal-axis-line-along-reliability
verified: 2026-04-09T09:53:27Z
status: passed
score: 6/6 must-haves verified
---

# Phase 27: Principal-Axis Line ALONG Reliability Verification Report

**Phase Goal:** Users can constrain lines to principal axes (`ALONG X/Y/Z`) without immediate solver failure and with deterministic behavior in mixed constraints.  
**Verified:** 2026-04-09T09:53:27Z  
**Status:** passed  
**Re-verification:** No — initial verification

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
| --- | --- | --- | --- |
| 1 | User can apply ALONG X to a sketch line and solve does not fail immediately in feasible setups. | ✓ VERIFIED | `scene_solver_contract_test.c:1356-1382` (`test_recalculate_along_x_single_legacy_line_entity_solves_alin01`) passes; asserts solved + Y/Z lock. |
| 2 | User can apply ALONG Y and ALONG Z to sketch lines with the same no-immediate-failure behavior. | ✓ VERIFIED | `scene_solver_contract_test.c:1384-1438` has dedicated ALONG Y/Z passing solve tests (`alin02`, `alin03`). |
| 3 | Legacy legal line ENTITY ALONG authoring is resolved via descriptor-authoritative endpoint behavior, not runtime rejection. | ✓ VERIFIED | Runtime expands line ENTITY to endpoints in `ecs_scene.h:3061-3075`; normalized descriptors feed candidate lookup in `ecs_scene.h:3116-3125`; legality parity tests in `endpoint_pick_test.c:396-417`. |
| 4 | User can combine ALONG with LENGTH, ANGLE, and connectivity constraints and get deterministic solved outcomes in feasible cases. | ✓ VERIFIED | Mixed-stack deterministic checks in `scene_solver_contract_test.c:1480-1534` and `scene_solver_pass_policy_test.c:287-349`. |
| 5 | Equivalent mixed-constraint setups rerun to identical outcomes without selection-order drift. | ✓ VERIFIED | Equivalent-ordering exact endpoint equality test in `scene_solver_contract_test.c:1536-1624`; ANGLE canonicalization by entity id in `ecs_scene.h:3515-3521`. |
| 6 | Infeasible fixed mixed ALONG cases fail transactionally with explicit ALONG family diagnostics. | ✓ VERIFIED | Transactional unsat for ALONG X/Z in `scene_solver_contract_test.c:1440-1478, 1626-1688`; family-specific diagnostics in `scene_solver_diagnostics_test.c:337-434`; failure reason wired to implication + diagnostics in `ecs_scene.h:4276-4284`. |

**Score:** 6/6 truths verified

### Required Artifacts

| Artifact | Expected | Status | Details |
| --- | --- | --- | --- |
| `src/ecs/ecs_scene.h` | ALONG normalization + mixed determinism runtime handling | ✓ VERIFIED | Exists, substantive solver logic for ALONG/ANGLE branches (`3027-3204`, `3506-3605`), wired through recalc and diagnostic pathways. |
| `src/tests/scene_solver_contract_test.c` | ALIN-01..04 contract coverage (feasible, deterministic, transactional unsat) | ✓ VERIFIED | ALIN tests present and registered in test list (`2137-2150`), includes X/Y/Z + mixed-stack checks. |
| `src/tests/endpoint_pick_test.c` | Legality/runtime parity assertions including legacy line ENTITY ALONG acceptance | ✓ VERIFIED | Directional legality cases for ALONG signatures at `317-417`; test registered in suite (`1887-1888`). |
| `src/tests/scene_solver_pass_policy_test.c` | Deterministic pass-policy coverage for mixed ALONG stack | ✓ VERIFIED | `test_alin04_pass_policy_mixed_along_x_length_angle_connectivity_rerun_deterministic` (`287-349`), registered (`365-366`). |
| `src/tests/scene_solver_diagnostics_test.c` | Explicit ALONG X/Y/Z unsatisfied diagnostic assertions | ✓ VERIFIED | Family-specific ALONG unsat message assertions at `337-434`; registered (`453-458`). |

### Key Link Verification

| From | To | Via | Status | Details |
| --- | --- | --- | --- | --- |
| `src/ecs/ecs_scene.h` | `scene_solver_ensure_point_candidate` in `src/ecs/ecs_scene.h` | normalized participant descriptor expansion before candidate lookup | ✓ WIRED | ALONG branch expands descriptors (`3061-3075`) then resolves candidates (`3120-3125`). |
| `src/tests/endpoint_pick_test.c` | `constraint_type_is_selection_legal` in `src/constraints/constraint_types.h` | ALONG signature legality contracts | ✓ WIRED | Multiple direct legality calls for ALONG at `325-327`, `338-340`, `354-356`, `400-402`, `413-415`. |
| `src/tests/scene_solver_contract_test.c` | `scene_solver_request_recalculate` in `src/ecs/ecs_scene.h` | mixed constraint fixture solve + immediate rerun equality checks | ✓ WIRED | Mixed ALIN-04 test invokes recalc twice (`1514`, `1523`) and compares exact endpoints. |
| `src/tests/scene_solver_diagnostics_test.c` | `scene_solver_set_failure_implication` in `src/ecs/ecs_scene.h` | family-specific unsatisfied reason assertions | ✓ WIRED | Diagnostics tests assert ALONG reason strings (`365`, `398`, `431`); solver routes failure reason into implication + diagnostic (`4278-4284`). |

### Data-Flow Trace (Level 4)

| Artifact | Data Variable | Source | Produces Real Data | Status |
| --- | --- | --- | --- | --- |
| `src/ecs/ecs_scene.h` (ALONG branch) | `normalized_desc[]`, `along_indices[]`, candidate points | Constraint entities discovered from live sketch children (`2903-2908`, `3047-3053`) | Yes — computed from runtime ECS geometry/constraint components, not static literals | ✓ FLOWING |
| `src/ecs/ecs_scene.h` (failure diagnostics) | `failure_reason` | Set by actual branch failures (e.g., ALONG unsat at `3181-3184`) | Yes — propagated to failure implication + diagnostics (`4278-4284`) | ✓ FLOWING |

### Behavioral Spot-Checks

| Behavior | Command | Result | Status |
| --- | --- | --- | --- |
| Targeted ALONG closure suite run 1 | `ctest --test-dir C:\dev\mdCAD\build -C Release -R "scene_solver_contract\|scene_solver_pass_policy\|scene_solver_diagnostics\|endpoint_pick" --output-on-failure` | 4/4 tests passed | ✓ PASS |
| Targeted ALONG closure suite run 2 (determinism rerun) | same command | 4/4 tests passed again | ✓ PASS |

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
| --- | --- | --- | --- | --- |
| ALIN-01 | 27-01-PLAN | User can apply `ALONG X` without immediate unsatisfied-driving failure. | ✓ SATISFIED | Contract test `test_recalculate_along_x_single_legacy_line_entity_solves_alin01` (`1356-1382`); targeted ctest pass. |
| ALIN-02 | 27-01-PLAN | User can apply `ALONG Y` without immediate unsatisfied-driving failure. | ✓ SATISFIED | Contract test `test_recalculate_along_y_single_legacy_line_entity_solves_alin02` (`1384-1410`). |
| ALIN-03 | 27-01-PLAN | User can apply `ALONG Z` without immediate unsatisfied-driving failure. | ✓ SATISFIED | Contract test `test_recalculate_along_z_single_legacy_line_entity_solves_alin03` (`1412-1438`). |
| ALIN-04 | 27-02-PLAN | User can combine ALONG with LENGTH/ANGLE/connectivity and keep deterministic feasible outcomes. | ✓ SATISFIED | Mixed deterministic and ordering tests (`1480-1624`), transactional unsat test (`1626-1688`), pass-policy and diagnostics suites, plus two green reruns. |

**Orphaned requirements check:** No orphaned Phase 27 requirements found in `.planning/REQUIREMENTS.md` (ALIN-01..ALIN-04 all declared in phase plans).

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
| --- | --- | --- | --- | --- |
| `src/ecs/ecs_scene.h` | 4710 | Comment contains “Placeholder derivation for Phase 10” | ℹ️ Info | Non-executable comment only; no behavioral impact on ALIN coverage. |

### Human Verification Required

None required for phase-goal acceptance. Core goal criteria are solver/runtime determinism and transactional behavior, covered by targeted automated tests and direct code-path verification.

### Gaps Summary

No blocking gaps found. Must-haves for ALIN-01..ALIN-04 are present, substantive, wired, and behaviorally validated with deterministic reruns.

---

_Verified: 2026-04-09T09:53:27Z_  
_Verifier: the agent (gsd-verifier)_
