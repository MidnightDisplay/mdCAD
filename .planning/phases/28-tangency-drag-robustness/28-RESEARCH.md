# Phase 28: tangency-drag-robustness - Research

**Researched:** 2026-04-09  
**Domain:** Arc-line endpoint tangency solve/drag robustness in transactional sketch solver  
**Confidence:** High

## Summary

Phase 28 should be implemented as targeted hardening in existing solver paths, not a solver rewrite. The primary runtime surface is `CONSTRAINT_LINE_ARC_ENDPOINT_TANGENCY` in `src/ecs/ecs_scene.h`, which already includes drag-anchor awareness, coincidence+tangent enforcement, transactional recalc staging, and explicit family-specific failure messaging.

The main gaps are coverage and deterministic behavior closure for:

- Shared-point vs adjacent-handle drag behavior in feasible setups
- Immediate post-failure responsiveness after infeasible edits
- Mirrored interaction feasibility parity under mixed constraints

## Locked Decisions Carry-Through

From `28-CONTEXT.md`, implementation must preserve:

- Dragged participant authority within active DoF constraints
- Both shared and adjacent tangency handle drags feasible when geometry allows
- Mirrored interaction pass/fail parity and deterministic reruns
- Strict transactional rollback and explicit tangency-family diagnostics
- No cross-family drag-policy generalization in this phase

## Code Anchors

- `src/ecs/ecs_scene.h`
  - `scene_solver_set_drag_anchor(...)`
  - `scene_solver_clear_drag_anchor(...)`
  - `scene_solver_request_recalculate(...)`
  - tangency branch for `CONSTRAINT_LINE_ARC_ENDPOINT_TANGENCY`
- `src/constraints/constraint_types.h`
  - legality signatures for line/arc endpoint-role tangency participants
- `src/tests/scene_solver_contract_test.c`
  - tangency correctness + transactional baseline
- `src/tests/scene_solver_drag_test.c`
  - drag behavior contracts (needs tangency matrix expansion)
- `src/tests/scene_solver_diagnostics_test.c`
  - family-specific diagnostic assertions
- `src/tests/scene_solver_pass_policy_test.c`
  - repeated-recalc determinism checks

## Recommended Implementation Strategy

1. Add RED tests first for TRDG-01/02/03 matrix:
   - feasible shared drag
   - feasible adjacent drag
   - infeasible rollback + family diagnostic
   - immediate follow-up feasible edit succeeds

2. Tighten tangency solve/anchor precedence in `ecs_scene.h`:
   - dragged handle remains authoritative where feasible
   - connected geometry absorbs adjustment to restore coincidence+tangency
   - keep transactional fail behavior unchanged

3. Add RED mirrored parity tests for TRDG-04:
   - mirrored feasible/infeasible paired fixtures
   - rerun + ordering stability assertions

4. Run targeted closure gate twice and record deterministic evidence.

## Risks and Mitigations

- **Risk:** Hidden ordering bias breaks mirrored parity.  
  **Mitigation:** Add explicit mirrored paired fixtures and equality of outcome class.

- **Risk:** Partial mutation leaks on unsat path.  
  **Mitigation:** assert unchanged geometry snapshots in unsat tests.

- **Risk:** drag interaction regresses after failure.  
  **Mitigation:** add post-failure immediate-success regression case.

## Validation Architecture

### Test Infrastructure

| Property | Value |
|----------|-------|
| Framework | Native C tests via CMake + CTest |
| Config file | `CMakeLists.txt`, `src/CMakeLists.txt` |
| Quick run command | `ctest --test-dir build -C Release -R "scene_solver_contract\|scene_solver_drag\|scene_solver_diagnostics\|scene_solver_pass_policy" --output-on-failure` |
| Full suite command | `ctest --test-dir build -C Release --output-on-failure` |
| Estimated runtime | ~20-60 seconds for targeted slice |

### Requirement Mapping

| Requirement | Coverage approach |
|-------------|-------------------|
| TRDG-01 | Contract tests for feasible line-end/arc-end tangency authoring and solved geometry stability |
| TRDG-02 | Drag matrix tests for shared and adjacent handle drags in feasible setups |
| TRDG-03 | Transactional unsat rollback + explicit diagnostics + immediate next-edit responsiveness |
| TRDG-04 | Mirrored parity fixtures and deterministic rerun/order invariance assertions |

### Wave 0 Gap Targets

- Add tangency-specific shared/adjacent matrix cases in `scene_solver_drag_test.c`
- Add mirrored feasibility parity fixtures in `scene_solver_contract_test.c`
- Add mirrored rerun/order determinism checks in `scene_solver_pass_policy_test.c`
- Add explicit post-failure responsiveness assertion (contract or drag suite)

## Open Questions

1. Whether current no-anchor fallback ordering causes mirrored asymmetry in complex mixed stacks.
2. Best fixture split between contract and drag suites for fast, stable signal.

---

## RESEARCH COMPLETE

Phase 28 planning can proceed with existing architecture and targeted test-first hardening.

