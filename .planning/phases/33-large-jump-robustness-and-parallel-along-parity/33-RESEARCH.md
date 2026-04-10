# Phase 33: Large-Jump Robustness and PARALLEL/ALONG Parity - Research

**Researched:** 2026-04-10  
**Domain:** mdCAD sketch solver robustness, deterministic diagnostics, PARALLEL/ALONG parity  
**Confidence:** HIGH

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
### Carry-forward constraints from prior phases
- **D-01:** Solver behavior remains transactional: unsatisfied edits must not partially mutate scene geometry.
- **D-02:** Fixed participants remain hard anchors; movement resolution must occur through non-fixed participants.
- **D-03:** Explicit family diagnostics and deterministic implication ordering remain mandatory.
- **D-04:** No implicit coincidence fallback is permitted (preserve Phase 32 explicit coincidence posture).

### Large-jump solve strategy
- **D-05:** Implement a two-stage solve approach for large-jump operations: (1) large-jump preprojection/staging, then (2) normal iterative constraint passes.
- **D-06:** The large-jump strategy must remove user-observed "wiggle to latch" behavior in feasible quarter-arc closed-loop arrangements.

### Failure contract for unsatisfied large-jump edits
- **D-07:** If two-stage solve still cannot satisfy constraints, rollback hard transactionally to the last valid geometry state.
- **D-08:** On rollback, publish explicit failure implication and diagnostic reason(s) for recovery.
- **D-09:** After an infeasible attempt, immediate follow-up edits must remain responsive (no stale-failure lock/deadlock behavior).

### PARALLEL / ALONG parity policy
- **D-10:** Phase 33 enforces geometric-equivalence parity: equivalent `PARALLEL` and `ALONG` setups should map to the same feasibility/outcome class.
- **D-11:** Mirrored and participant-order variants of equivalent setups must preserve parity and determinism.

### Deterministic diagnostics taxonomy
- **D-12:** Lock a deterministic family+reason diagnostic taxonomy for this phase, including large-jump unsatisfied, parity-class mismatch, and max-passes classes.
- **D-13:** Diagnostic and implication ordering must remain stable for identical operation sequences.

### the agent's Discretion
- Exact low-level preprojection math and pass handoff mechanics for the two-stage solve, provided D-05..D-09 hold.
- Exact diagnostic text wording/format, provided family+reason taxonomy and deterministic ordering are preserved.
- Exact fixture split across existing solver contract/drag/pass-policy/diagnostics suites.

### Deferred Ideas (OUT OF SCOPE)
- Final closure rerun sign-off (`DIAG-03`) remains in Phase 34.
- Broad solver-family optimization outside SROB/DIAG/PARI scope is deferred.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|---|---|---|
| SROB-01 | Large-jump edits in quarter-arc closed loops should not require wiggle-to-latch | Two-stage solve recommendation centered on `scene_solver_can_apply_drag(...)` preprojection + `scene_solver_request_recalculate(...)` staged pass handoff; add quarter-arc fixtures to drag/contract suites |
| SROB-02 | Large coupled changes must either fully solve or rollback transactionally | Preserve current transactional commit point in `scene_solver_request_recalculate(...)`; assert no writeback on failure paths (`solve_failed`/`!converged`) |
| SROB-03 | Immediate follow-up edits after infeasible attempt remain responsive | Preserve/verify drag-anchor/failure reset behavior; extend `test_drag_tangency_post_failure_followup_feasible_is_responsive` pattern |
| DIAG-01 | Deterministic outcomes and deterministic failure-class diagnostics | Keep deterministic sorting in `scene_solver_set_failure_implication(...)`; add repeat-run parity tests for large-jump + parity fixtures |
| DIAG-02 | Actionable diagnostics without misleading participant-type errors | Use family-specific failure reasons (existing ALONG/PARALLEL messages); add large-jump-specific reason class and deterministic ordering checks |
| PARI-01 | Equivalent PARALLEL vs ALONG setups behave equivalently | Add paired fixtures (same geometry, different constraint encoding) and assert same outcome class + mirrored invariants |
| PARI-02 | Drag equivalent mirrored/linked PARALLEL and ALONG vertices with consistent feasibility | Extend pass-policy mirrored/order tests to include explicit PARALLEL↔ALONG parity matrix and feasibility class checks |
</phase_requirements>

## Summary

Phase 33 should be planned as a **solver behavior hardening phase**, not a new feature phase. The core mechanisms already exist: bounded drag preprojection (`scene_solver_can_apply_drag`), transactional recalc/failure handling (`scene_solver_request_recalculate`), deterministic failure implication ordering (`scene_solver_set_failure_implication`), and diagnostics dedupe/order behavior (`scene_solver_add_diagnostic` + diagnostics tests). The planning focus is to wire these into a **two-stage large-jump path** and then lock parity rules between equivalent PARALLEL and ALONG geometries.

Current tests already prove many invariants you need to preserve: transactional rollback, deterministic reruns, family-specific diagnostics, mirrored/order stability, and post-failure responsiveness. The gap is not missing infrastructure; the gap is targeted fixtures for quarter-arc large-jump and explicit PARALLEL↔ALONG equivalence classes under mirrored/order variants.

**Primary recommendation:** Implement large-jump preprojection as explicit stage-1 state feeding the existing iterative solver, then prove behavior with deterministic parity-first tests before broad refactors.

## Requirement-to-Surface Mapping (Implementation Anchors)

| Req ID | Primary code surface | Supporting tests to extend | Notes |
|---|---|---|---|
| SROB-01 | `scene_solver_can_apply_drag(...)` (~5000), `scene_solver_request_recalculate(...)` (~3218) in `src/ecs/ecs_scene.h` | `scene_solver_drag_test.c`, `scene_solver_contract_test.c` | Add quarter-arc closed-loop large-delta fixtures |
| SROB-02 | Recalc epilogue failure branches (~4738, ~4769) in `ecs_scene.h` | `scene_solver_contract_test.c`, `scene_solver_drag_test.c` | Verify no writeback when `solve_failed` or `!converged` |
| SROB-03 | Failure implication clear/apply (`scene_solver_apply_status`, `scene_solver_clear_failure_implication`) + app drag feedback path | `scene_solver_drag_test.c`, `scene_solver_diagnostics_test.c` | Preserve immediate follow-up solve responsiveness |
| DIAG-01 | `scene_solver_set_failure_implication(...)` (~4909), diagnostics append/dedupe (~4816) | `scene_solver_diagnostics_test.c`, `scene_solver_pass_policy_test.c` | Repeat-run deterministic ordering for identical operations |
| DIAG-02 | Family-specific reason assignment in ALONG/PARALLEL/ARCI branches | `scene_solver_diagnostics_test.c` | Avoid generic participant-type fallback on targeted paths |
| PARI-01 | ALONG branch (~3421+) and PARALLEL branch (~3600+) in `ecs_scene.h` | `scene_solver_contract_test.c`, `scene_solver_pass_policy_test.c` | Equivalent geometry must map to same outcome class |
| PARI-02 | Drag path in `src/app.c` (gizmo update block ~1863+) + solver drag feasibility | `scene_solver_pass_policy_test.c`, `scene_solver_drag_test.c` | Mirrored/order variants must keep feasibility parity |

## Standard Stack

### Core
| Library/Module | Version | Purpose | Why Standard |
|---|---|---|---|
| `src/ecs/ecs_scene.h` solver core | In-repo | Constraint solve loop, convergence policy, failure implication, diagnostics | Already enforces transactional + deterministic behavior expected by v1.5 |
| `ConstraintComp` descriptors (`src/components/constraint_comp.h`) | In-repo | Participant role/sub-index semantics | Required for ALONG and mixed sub-entity legality and parity |
| Constraint legality helpers (`src/constraints/constraint_types.h`) | In-repo | Selection legality parity between authoring/runtime | Prevents script/runtime signature drift |

### Supporting
| Library/Module | Version | Purpose | When to Use |
|---|---|---|---|
| CTest targets in `src/CMakeLists.txt` | CMake/CTest 4.3.1 detected | Deterministic regression gate | Per-commit quick gate + rerun parity |
| `src/app.c` drag orchestration | In-repo | Feasibility precheck + user feedback path | Any change to drag staging must preserve UX contract |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|---|---|---|
| Deterministic staged solve in existing loop | Non-deterministic jitter/retry hacks | Faster to prototype but violates deterministic v1.5 closure goals |
| Existing failure implication taxonomy | Generic error bucket | Loses DIAG-02 actionable diagnostics and breaks existing tests |

**Installation:** N/A (phase is in-repo C/CMake work; no new package dependency required).

## Architecture Patterns

### Recommended Project Structure
```text
src/
├── ecs/ecs_scene.h                  # Two-stage solve staging + transactional recalc + diagnostics
├── app.c                            # Drag request projection + rejection UX path
├── constraints/constraint_types.h   # Legality parity for PARALLEL/ALONG signatures
├── components/constraint_comp.h     # Descriptor semantics (role/sub-index)
└── tests/
    ├── scene_solver_contract_test.c
    ├── scene_solver_drag_test.c
    ├── scene_solver_pass_policy_test.c
    ├── scene_solver_diagnostics_test.c
    └── endpoint_pick_test.c
```

### Pattern 1: Two-Stage Large-Jump Solve (recommended)
**What:** Stage 1 computes bounded/projection-assisted candidate movement for large deltas; Stage 2 runs existing iterative constraints with unchanged transactional commit rules.  
**When to use:** Any drag/edit where requested delta exceeds normal convergence comfort and user currently sees “wiggle to latch.”  
**Example:**
```c
// Source: src/ecs/ecs_scene.h + src/app.c (existing pattern)
scene_solver_drag_decision_t decision = {0};
scene_solver_can_apply_drag(scene, sketch, drag_entities, drag_count, requested_delta, &decision);
if (decision.result == SCENE_SOLVER_DRAG_FEASIBLE) {
    vec3_t staged = decision.projected_delta;   // stage-1 bounded delta
    // apply staged delta to entities/endpoints
    // then call scene_solver_request_recalculate(...) for stage-2 iterative pass
}
```

### Pattern 2: Transactional Failure Epilogue
**What:** On `solve_failed` or `!converged`, set deterministic implication + diagnostic and return without committing candidate points.  
**When to use:** Every unsatisfied class introduced in this phase (large-jump, parity mismatch classification).  

### Anti-Patterns to Avoid
- **No partial writes before success:** Do not mutate geometry outside the final writeback loops in `scene_solver_request_recalculate(...)`.
- **No random retry/jitter:** Violates deterministic rerun requirements and DIAG-01.
- **No family message collapse:** Keep “Unsatisfied parallel constraint.” vs “Unsatisfied driving ALONG X/Y/Z constraint.” family specificity.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---|---|---|---|
| Large-jump feasibility gate | New ad-hoc drag filters in UI | `scene_solver_can_apply_drag(...)` + staged handoff | Already integrated with solver rejection contract and diagnostics |
| Failure participant ordering | Custom sorting in each branch | `scene_solver_set_failure_implication(...)` deterministic sort/unique | Existing deterministic invariant and tests already cover ordering |
| Diagnostics queue logic | New side-channel buffers | `scene_solver_add_diagnostic(...)` dedupe/ring behavior | Prevents duplicate spam while keeping deterministic logs |

**Key insight:** The phase should compose existing solver primitives, not replace them.

## Common Pitfalls

### Pitfall 1: Breaking transactional semantics during staging
**What goes wrong:** Stage-1 mutates geometry directly and failed stage-2 leaves partial corruption.  
**How to avoid:** Keep stage-1 as candidate/projection data only; commit only in final successful writeback loop.

### Pitfall 2: PARALLEL vs ALONG branch drift
**What goes wrong:** Equivalent setups diverge because ALONG normalizes/averages points while PARALLEL rotates line directions independently.  
**How to avoid:** Define parity outcome classes (feasible/unsat + mirrored invariants) and test both encodings with identical fixtures.

### Pitfall 3: Diagnostic nondeterminism after parity fixes
**What goes wrong:** Constraint implication order changes run-to-run or by participant order.  
**How to avoid:** Preserve canonical descriptor sorting and implication sort/unique path; add immediate rerun assertions.

## Implementation Risks + Mitigations (Phase 33 specific)

| Risk | Likely Touch Point(s) | Mitigation |
|---|---|---|
| Stage handoff regresses existing convergent small-delta drags | `scene_solver_can_apply_drag`, `scene_solver_request_recalculate` in `ecs_scene.h` | Gate staged behavior by “large delta” threshold; keep small-delta path untouched |
| Parity patch changes ALONG legacy single-line behavior | ALONG normalization branch in `ecs_scene.h`; legality in `constraint_types.h` | Re-run ALIN legacy tests (`test_recalculate_along_*_single_legacy_line_entity_*`) |
| New failure reasons break diagnostics tests | Failure reason assignment in ALONG/PARALLEL/epilogue blocks | Extend diagnostics taxonomy tests first; assert exact message strings |
| UI feedback mismatch on unsat drag | `src/app.c` drag loop + `mdcad_apply_solver_failure_feedback` | Keep current “Drag rejected...” event path and implication attachment |

## Code Examples

### Deterministic implication ordering pattern
```c
// Source: src/ecs/ecs_scene.h
scene_solver_sort_entities_unique(sorted_constraints, &sorted_constraint_count);
scene_solver_sort_entities_unique(imp->participants, &imp->participant_count);
```

### Convergence gate pattern
```c
// Source: src/ecs/ecs_scene.h
if (pass_max_position_delta <= position_tolerance &&
    pass_max_angle_delta <= angle_tolerance &&
    pass_max_residual <= position_tolerance) {
    converged = true;
    break;
}
```

### Unsatisfied drag diagnostic payload pattern
```c
// Source: src/ecs/ecs_scene.h
if (decision->result == SCENE_SOLVER_DRAG_UNSATISFIABLE) {
    out_event->severity = SKETCH_SOLVER_DIAG_WARNING;
    snprintf(out_event->message, sizeof(out_event->message),
             "Drag rejected: active constraints make this move invalid.");
}
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|---|---|---|---|
| Implicit fallback and ad-hoc behavior | Explicit participant descriptors + deterministic implication ordering | Phases 26-32 | Enables parity and deterministic failure taxonomy work |
| Generic constraint failure messaging | Family-specific unsatisfied diagnostics (ALONG/PARALLEL/ARCI) | Phase 27+ and later hardening | Actionable diagnosis and stable test assertions |
| One-shot drag application intuition | Bounded drag feasibility projection exists | Phase 28+ | Direct base for stage-1 large-jump strategy |

## Candidate Plan Slices (minimal recommended decomposition)

1. **Slice A — Stage-1 large-jump staging contract (SROB-01/02)**  
   Implement two-stage handoff in `ecs_scene.h` without changing final transactional commit behavior.
2. **Slice B — Failure/recovery contract lock (SROB-03 + DIAG-01/02)**  
   Harden failure reason taxonomy, deterministic implication ordering, and immediate follow-up responsiveness.
3. **Slice C — PARALLEL/ALONG parity matrix (PARI-01/02)**  
   Add equivalent fixture matrix (base/mirrored/reordered) and align outcome classes.
4. **Slice D — Deterministic rerun evidence pass**  
   Baseline + immediate rerun with identical CTest command; record parity of outcomes for all targeted tests.

## Open Questions

1. **Exact large-jump threshold definition**
   - What we know: Bounded projection exists already in drag feasibility.
   - What’s unclear: Best threshold policy for “enter stage-1 mode” vs normal path.
   - Recommendation: Start with component/max-delta threshold + keep test-driven tuning in same phase.

2. **Parity class granularity**
   - What we know: Feasible vs unsat parity is mandatory.
   - What’s unclear: Whether to also enforce exact numeric endpoint equality or mirrored-equivalence tolerances only.
   - Recommendation: Require outcome-class parity + mirrored invariant tolerances; keep exact equality only for deterministic rerun on same fixture.

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|---|---|---|---|---|
| cmake | Configure/build tests | ✓ | 4.3.1 | — |
| ctest | Run solver test gate | ✓ | 4.3.1 | direct test exe runs |
| ninja | Optional generator | ✗ | — | Use Visual Studio generator / existing build dir |
| python | Utility scripts (optional) | ✓ | 3.13.12 | — |
| node | gsd tooling/workflow scripts | ✓ | v25.9.0 | — |

**Missing dependencies with no fallback:** None identified.  
**Missing dependencies with fallback:** `ninja` missing; CMake/CTest can run via existing generator/build.

## Validation Architecture

### Test Framework
| Property | Value |
|---|---|
| Framework | CTest + native C test executables |
| Config file | `src/CMakeLists.txt` test target registrations (`add_test`) |
| Quick run command | `ctest --test-dir build -R "scene_solver_contract|scene_solver_drag|scene_solver_pass_policy|scene_solver_diagnostics|endpoint_pick" --output-on-failure` |
| Full suite command | `ctest --test-dir build --output-on-failure` |

### Phase Requirements → Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|---|---|---|---|---|
| SROB-01 | Large-jump quarter-arc solve without wiggle | integration | `ctest --test-dir build -R "scene_solver_drag|scene_solver_contract" --output-on-failure` | ✅ |
| SROB-02 | Large-jump unsat rollback transactional | integration | `ctest --test-dir build -R "scene_solver_drag|scene_solver_contract" --output-on-failure` | ✅ |
| SROB-03 | Post-failure immediate follow-up responsiveness | integration | `ctest --test-dir build -R "scene_solver_drag|scene_solver_diagnostics" --output-on-failure` | ✅ |
| DIAG-01 | Deterministic outcomes + diagnostics ordering | integration | `ctest --test-dir build -R "scene_solver_pass_policy|scene_solver_diagnostics|scene_solver_contract" --output-on-failure` | ✅ |
| DIAG-02 | Actionable family-specific diagnostics | integration | `ctest --test-dir build -R "scene_solver_diagnostics|scene_solver_contract" --output-on-failure` | ✅ |
| PARI-01 | PARALLEL/ALONG equivalent setup parity | integration | `ctest --test-dir build -R "scene_solver_pass_policy|scene_solver_contract" --output-on-failure` | ✅ |
| PARI-02 | Mirrored/linked drag feasibility parity | integration | `ctest --test-dir build -R "scene_solver_pass_policy|scene_solver_drag" --output-on-failure` | ✅ |

### Deterministic rerun evidence approach (explicit)
1. Run the exact quick command once (baseline).  
2. Immediately rerun the **exact same command string** unchanged.  
3. Accept only if pass/fail set is identical and no test is flaky across the two runs.  
4. For parity-critical tests, include repeat-run assertions in-test (already patterned in pass-policy/contract suites).

### Sampling Rate
- **Per task commit:** quick run command above
- **Per wave merge:** quick run + deterministic immediate rerun
- **Phase gate:** full suite command + immediate rerun of targeted quick gate

### Wave 0 Gaps
- [ ] Add quarter-arc large-jump fixture covering explicit “wiggle-to-latch” regression (SROB-01)
- [ ] Add explicit PARALLEL vs ALONG equivalence matrix fixtures (base/mirror/order variants) (PARI-01/02)
- [ ] Add deterministic reason taxonomy assertions for new large-jump/parity-class diagnostics (DIAG-01/02)

## Sources

### Primary (HIGH confidence)
- `.planning/phases/33-large-jump-robustness-and-parallel-along-parity/33-CONTEXT.md` — locked decisions and scope
- `.planning/REQUIREMENTS.md` — SROB/DIAG/PARI requirement contracts
- `.planning/ROADMAP.md` — phase goal, dependencies, success criteria
- `.planning/STATE.md` — milestone continuity, locked stack context
- `docs/improvements/solver-user-workflow-robustness.md` — user pain cases motivating phase
- `src/ecs/ecs_scene.h` — solver recalc loop, ALONG/PARALLEL branches, implication/diagnostics, drag feasibility
- `src/app.c` — constrained drag orchestration and rejection feedback
- `src/constraints/constraint_types.h` — legality/selection contracts for PARALLEL/ALONG and descriptors
- `src/components/constraint_comp.h` — constraint type enum and participant descriptor layout
- `src/tests/scene_solver_contract_test.c` — transactional/deterministic and ALONG/PARALLEL baselines
- `src/tests/scene_solver_drag_test.c` — large-delta projection and rollback responsiveness
- `src/tests/scene_solver_pass_policy_test.c` — mirrored/order deterministic parity patterns
- `src/tests/scene_solver_diagnostics_test.c` — deterministic diagnostic ordering and family-specific reasons
- `src/tests/endpoint_pick_test.c` — endpoint/descriptor anchoring support surface
- `src/CMakeLists.txt` — test target + CTest registration

### Secondary (MEDIUM confidence)
- None used.

### Tertiary (LOW confidence)
- None used.

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — all recommendations derived from current in-repo architecture and test harness.
- Architecture: HIGH — based on direct code-path inspection of solver, drag orchestration, and deterministic implication flow.
- Pitfalls: HIGH — grounded in user-reported workflow failures and existing regression test patterns.

**Research date:** 2026-04-10  
**Valid until:** 2026-05-10
