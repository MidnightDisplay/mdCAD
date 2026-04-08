---
phase: 23-principal-direction-constraint-expansion
verified: 2026-04-08T15:29:37.2170339+01:00
status: passed
score: 6/6 must-haves verified
---

# Phase 23: Principal-Direction Constraint Expansion Verification Report

**Phase Goal:** Deliver deterministic ALONG X/Y/Z legality and solve behavior across supported point participant sources, including coexistence with ANGLE constraints, and close UX gaps for interactive sketch manipulation workflows.  
**Verified:** 2026-04-08T15:29:37.2170339+01:00  
**Status:** passed

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
| --- | --- | --- | --- |
| 1 | ALONG X/Y/Z legality supports point-pair/group signatures beyond single-line-only legacy behavior. | ✓ VERIFIED | `src/constraints/constraint_types.h` legality expansion; `src/tests/endpoint_pick_test.c` directional legality matrix coverage. |
| 2 | Directional legality includes standalone points, line endpoint roles, and arc landmark roles (including center). | ✓ VERIFIED | Descriptor-role handling in `src/constraints/constraint_types.h` + scene candidate support in `src/ecs/ecs_scene.h`; legality/runtime tests in `endpoint_pick_test` and `scene_solver_contract_test`. |
| 3 | Recalculate with ALONG + ANGLE constraints remains functional and deterministic under unchanged state. | ✓ VERIFIED | `src/tests/scene_solver_contract_test.c` coexistence contracts; targeted closure gate includes `scene_solver_contract` + `scene_solver_pass_policy` + `scene_solver_drag`. |
| 4 | Unsatisfied directional scenarios stay transactional with explicit diagnostics and no partial mutation leaks. | ✓ VERIFIED | Transactional failure path in `src/ecs/ecs_scene.h`; pass-policy and diagnostics checks in `scene_solver_pass_policy_test` / `scene_solver_diagnostics_test`. |
| 5 | Active-sketch standalone points can be manipulated by gizmo without tabbing into geometry mode, mutating geometry-local point coordinates. | ✓ VERIFIED | Drag routing and direct-point mode in `src/app.c`; scene helper `scene_apply_standalone_sketch_point_world_delta` in `src/ecs/ecs_scene.h`; user checkpoint approval. |
| 6 | Untabbed standalone-point drag records geometry-vertex undo contracts and replays deterministically. | ✓ VERIFIED | `src/app.c` direct-point undo uses `undo_cmd_set_geometry_vertices`; regression in `src/tests/endpoint_pick_test.c` (`test_standalone_sketch_point_drag_records_geometry_vertex_undo`). |

**Score:** 6/6 truths verified

### Behavioral Spot-Checks

| Behavior | Command | Result | Status |
| --- | --- | --- | --- |
| Phase 23 targeted closure gate | `ctest -R "endpoint_pick|scene_solver_contract|scene_solver_pass_policy|scene_solver_trigger|scene_solver_diagnostics|scene_solver_drag" --test-dir build-vulkan -C Release --output-on-failure` | 6/6 passed | ✓ PASS |

### Requirements Coverage

| Requirement | Status | Evidence |
| --- | --- | --- |
| AXIS-01 | ✓ SATISFIED | ALONG X legality + solve behavior covered by `endpoint_pick_test` and `scene_solver_contract_test` plus manual checkpoint outcomes. |
| AXIS-02 | ✓ SATISFIED | ALONG Y legality + solve behavior covered by same directional test matrix and manual checkpoint outcomes. |
| AXIS-03 | ✓ SATISFIED | ALONG Z legality + solve behavior covered by same directional test matrix and manual checkpoint outcomes. |
| AXIS-04 | ✓ SATISFIED | Mixed participant sources (standalone points, line endpoints, arc landmarks) covered in legality/runtime tests and approved manual checks. |
| SRLV-04 | ✓ SATISFIED | ALONG+ANGLE deterministic recalc coexistence covered by solver contract/pass-policy tests and approved manual checks. |

### Gaps Summary

No blocking gaps remain within Phase 23 scope. Directional legality/runtime goals and the requested standalone sketch-point gizmo UX closure are complete.

---

_Verified: 2026-04-08T15:29:37.2170339+01:00_  
_Verifier: the agent (phase execution closure pass)_

