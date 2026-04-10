---
phase: 33-large-jump-robustness-and-parallel-along-parity
plan: 02
subsystem: solver
tags: [solver, constraints, diagnostics, parity, parallel, along, drag]
requires:
  - phase: 33-large-jump-robustness-and-parallel-along-parity
    provides: large-jump staging baseline and deterministic implication ordering
provides:
  - PARALLEL pair canonical ordering aligned with ALONG parity policy
  - explicit PARALLEL↔ALONG parity matrix coverage across pass-policy/contract/drag suites
  - deterministic family+reason diagnostics assertions for parity unsatisfied variants
affects: [phase-34-closure-gate, solver diagnostics taxonomy, parity regression coverage]
tech-stack:
  added: []
  patterns: [equivalent-geometry parity matrix fixtures, mirrored/reordered parity assertions, family+reason diagnostics taxonomy checks]
key-files:
  created: []
  modified:
    - src/ecs/ecs_scene.h
    - src/tests/scene_solver_pass_policy_test.c
    - src/tests/scene_solver_contract_test.c
    - src/tests/scene_solver_drag_test.c
    - src/tests/scene_solver_diagnostics_test.c
key-decisions:
  - "Canonicalize PARALLEL pair participant ordering by entity ID in recalc to match ALONG equivalent-class determinism."
  - "Assert PARALLEL↔ALONG parity by outcome class (feasible/unsat) and mirrored/reordered invariants across static+drag workflows."
  - "Lock parity diagnostics to family+reason classes and explicitly reject participant-type fallback wording in targeted failures."
patterns-established:
  - "Parity matrix pattern: base + mirrored + reordered variants across pass-policy, contract, and drag suites."
requirements-completed: [PARI-01, PARI-02, DIAG-01, DIAG-02]
duration: 11min
completed: 2026-04-10
---

# Phase 33 Plan 02: PARALLEL/ALONG Parity and Diagnostics Summary

**Equivalent PARALLEL and ALONG setups now share deterministic outcome-class behavior and family-specific unsatisfied diagnostics across recalc and drag workflows.**

## Performance

- **Duration:** 11 min
- **Started:** 2026-04-10T18:25:35Z
- **Completed:** 2026-04-10T18:36:05Z
- **Tasks:** 3
- **Files modified:** 5

## Accomplishments
- Aligned solver core pairwise PARALLEL branch ordering with ALONG deterministic normalization for equivalent-geometry parity outcomes.
- Added explicit PARALLEL↔ALONG parity matrix regressions in pass-policy/contract suites plus mirrored/reordered drag feasibility parity coverage.
- Hardened diagnostics coverage so targeted parity failures stay family+reason specific and deterministic across immediate reruns.

## Task Commits

1. **Task 1: Align PARALLEL and ALONG equivalent-geometry outcome-class logic in solver core (TDD)**  
   - `db3b5a1` test(33-02): add failing parallel-along parity matrix tests  
   - `fd48a56` feat(33-02): align parallel pair ordering with along parity policy
2. **Task 2: Add explicit PARALLEL↔ALONG parity matrix regressions across pass-policy/contract/drag suites (TDD)**  
   - `86ad3e8` feat(33-02): add drag parity matrix for parallel-along equivalents
3. **Task 3: Harden DIAG family+reason taxonomy and deterministic ordering for parity and mixed-constraint failures (TDD)**  
   - `5d14946` feat(33-02): harden parity diagnostics taxonomy and rerun determinism

## Files Created/Modified
- `src/ecs/ecs_scene.h` - Canonicalized two-participant PARALLEL recalc ordering for parity with ALONG normalization behavior.
- `src/tests/scene_solver_pass_policy_test.c` - Added equivalent PARALLEL/ALONG base+mirrored+reordered pass-policy parity fixture/assertions.
- `src/tests/scene_solver_contract_test.c` - Added contract-level parity/transactional unsatisfied equivalence and family-reason checks.
- `src/tests/scene_solver_drag_test.c` - Added mirrored+reordered linked-vertex drag parity fixture with projected-delta parity assertions.
- `src/tests/scene_solver_diagnostics_test.c` - Added parity diagnostics family+reason and immediate-rerun deterministic ordering assertions.

## Decisions Made
- Used entity-ID canonical ordering in pairwise PARALLEL recalc to remove order-dependent parity drift relative to ALONG.
- Kept parity assertions centered on deterministic outcome class + mirrored/reordered consistency rather than strict full-geometry identity.
- Expanded diagnostics tests to explicitly reject participant-text fallback in parity-targeted unsatisfied paths.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Corrected incorrect CMake target names during RED/GREEN rebuild flow**
- **Found during:** Task 1 verification
- **Issue:** `cmake --build` attempted non-existent `scene_solver_pass_policy_tests` target, causing MSBuild project lookup failure.
- **Fix:** Switched to actual registered targets (`scene_solver_pass_policy`, `scene_solver_contract`, etc.) before running CTest.
- **Files modified:** none (execution command fix)
- **Verification:** Subsequent targeted builds succeeded and CTest slices executed as intended.
- **Committed in:** task commits (verification workflow)

---

**Total deviations:** 1 auto-fixed (1 blocking issue)  
**Impact on plan:** No scope creep; deviation only corrected execution plumbing for planned verification.

## Issues Encountered
- Initial build command used stale `_tests` target suffixes; corrected to repo target names defined in `src/CMakeLists.txt`.

## Known Stubs
- `src/ecs/ecs_scene.h:5205` — existing placeholder comment (`// Placeholder derivation for Phase 10 (D-06/D-08):`) is pre-existing and out of scope for this plan.

## Next Phase Readiness
- Phase 34 deterministic closure gate can consume parity matrix and diagnostics rerun assertions as baseline evidence.
- No active blockers for phase continuation.

## Self-Check: PASSED
- Summary file exists: `.planning/phases/33-large-jump-robustness-and-parallel-along-parity/33-02-SUMMARY.md`
- Task commits found: `db3b5a1`, `fd48a56`, `86ad3e8`, `5d14946`

