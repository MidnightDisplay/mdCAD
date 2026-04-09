# Phase 27: Principal-Axis Line ALONG Reliability - Research

**Researched:** 2026-04-09  
**Domain:** Sketch solver ALONG X/Y/Z line semantics, mixed-constraint determinism, transactional reliability  
**Confidence:** HIGH

## User Constraints (from CONTEXT.md)

### Locked Decisions
- **D-01:** For line `ALONG X`, the line axis must be constrained parallel to world X (the 3D equivalent of "horizontal" in this model), not perpendicular-to-X lockstep behavior.
- **D-02:** For line `ALONG Y` and `ALONG Z`, apply the same axis-parallel contract relative to Y and Z respectively.
- **D-03:** Preserve descriptor-based line endpoint participants (`POINT_A`, `POINT_B`) as the authoritative line representation for `ALONG` solving; avoid reintroducing raw-entity-only semantics.
- **D-04:** Line `ALONG` legality and runtime behavior must remain synchronized through shared legality gates (`constraint_type_is_selection_legal`) used by both UI authoring and script apply.
- **D-05:** Invalid `ALONG` signatures must continue explicit rejection; no silent acceptance path that later fails ambiguously in recalc.
- **D-06:** `ALONG` solve branches must remain transactional: unsatisfied all-fixed setups fail without geometry mutation.
- **D-07:** Mixed `ALONG` + `LENGTH` + `ANGLE` + connectivity setups must be deterministic under immediate reruns (selection-order invariant where applicable).
- **D-08:** Fixed participants remain hard anchors; movement must be resolved through non-fixed participants only.

### the agent's Discretion
- Exact projection math implementation for axis-parallel line enforcement, as long as D-01..D-08 and ALIN-01..04 are satisfied.
- Exact diagnostic string wording/format, provided family/type clarity and implication visibility remain explicit.
- Exact split of regression fixtures across existing solver legality/contract/diagnostics suites.

### Deferred Ideas (OUT OF SCOPE)
- Tangency drag robustness corrections — Phase 28.
- Active-sketch line gizmo endpoint authority/midpoint anchor behavior — Phase 29.
- Solver architecture documentation + milestone closure rerun/sign-off — Phase 30.

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| ALIN-01 | User can apply `ALONG X` to sketch lines without immediate unsatisfied-driving solver failure. | Replace legacy single-line raw-entity runtime handling with descriptor-expansion to line endpoints in ALONG solve path. |
| ALIN-02 | User can apply `ALONG Y` to sketch lines without immediate unsatisfied-driving solver failure. | Same endpoint-expansion + axis-locked solve pattern parameterized by free axis = Y. |
| ALIN-03 | User can apply `ALONG Z` to sketch lines without immediate unsatisfied-driving solver failure. | Same endpoint-expansion + axis-locked solve pattern parameterized by free axis = Z. |
| ALIN-04 | User can combine line `ALONG` with `LENGTH`, `ANGLE`, and connectivity constraints and still get deterministic solve outcomes in feasible cases. | Preserve transactional candidate staging, fixed-anchor policy, deterministic participant normalization, and add mixed-constraint deterministic rerun tests. |
</phase_requirements>

## Summary

The immediate reliability defect is reproducible from current solver behavior: `scene_add_constraint_to_sketch()` still allows the legacy single-line `ALONG_*` signature (line entity role `ENTITY`) via `constraint_type_is_selection_legal`, but the ALONG solve branch in `scene_solver_request_recalculate()` resolves participants only through `scene_solver_ensure_point_candidate()`, which rejects line `ENTITY` roles and requires `POINT_A`/`POINT_B`. This mismatch causes immediate unsatisfied failure for legal authoring paths (ALIN-01..03 regression).

The safest implementation is to keep legality/runtime parity and descriptor authority by normalizing ALONG participants at solve-time: when an ALONG participant is a line with role `ENTITY`, expand to its endpoint participants (`POINT_A`,`POINT_B`) before axis solve. Then enforce axis-parallel semantics by locking the two orthogonal coordinates (existing ALONG math already does this for point-like participants) while preserving fixed anchors and transactional commit semantics.

For ALIN-04, the current solver architecture already has the correct reliability backbone (candidate staging, failure implication, no partial mutation on failure, deterministic rerun checks). Phase 27 should add ALONG+LENGTH+ANGLE+connectivity mixed-contract tests and ALONG-specific diagnostics tests, without broad solver rewrites.

**Primary recommendation:** Implement a deterministic ALONG participant normalization step that expands legacy line-entity inputs to endpoint descriptors before solve, then enforce axis-parallel constraints using existing transactional candidate updates.

## Project Constraints (from copilot-instructions.md)

`copilot-instructions.md` not found at repository root; no additional project-specific directives were discovered from that file.

## Standard Stack

### Core
| Library/Module | Version | Purpose | Why Standard |
|---|---|---|---|
| Native C solver in `src/ecs/ecs_scene.h` | repo-local | Constraint solve pass loop and transactional commit | Existing canonical solver path for all constraint families |
| `constraint_types.h` legality gates | repo-local | Authoring/runtime signature legality | Shared by UI/script APIs; parity contract already established |
| ECS world/components (`ecs_world.h`, `ConstraintComp`, geometry comps) | repo-local | State storage and candidate materialization | Existing deterministic participant/geometry model |

### Supporting
| Library/Module | Version | Purpose | When to Use |
|---|---|---|---|
| `scene_solver_contract_test` | repo-local | Transactional + deterministic behavior contracts | Every ALONG semantic/runtime change |
| `endpoint_pick_test` | repo-local | Selection legality signatures | Any legality rule update |
| `scene_solver_diagnostics_test` | repo-local | Explicit diagnostic wording/family checks | Any new/changed ALONG failure messages |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|---|---|---|
| Solve-time participant normalization | Changing legality to reject legacy single-line ALONG | Breaks existing authoring path and contradicts current acceptance contract |
| Endpoint-descriptor authority | Reintroducing raw line-entity solve semantics | Recreates mismatch risk and weakens deterministic participant model |

**Installation (existing stack):**
```bash
cmake -S . -B build
cmake --build build --config Debug --target scene_solver_contract endpoint_pick scene_solver_diagnostics
```

## Architecture Patterns

### Recommended Project Structure (Phase 27 touch points)
```text
src/
├── constraints/constraint_types.h      # legality contracts (parity guard)
├── ecs/ecs_scene.h                     # ALONG runtime solve branch (primary fix)
└── tests/
    ├── scene_solver_contract_test.c    # ALIN-01..04 behavior + determinism
    ├── endpoint_pick_test.c            # legality signature coverage
    └── scene_solver_diagnostics_test.c # explicit ALONG failure diagnostics
```

### Pattern 1: ALONG participant normalization before solve
**What:** Convert ALONG participants into point-candidate participants deterministically.  
**When to use:** Every ALONG constraint solve pass before residual/update logic.

**Implementation strategy (prescriptive):**
1. In ALONG branch, for each participant descriptor:
   - `GEOM_POINT` + `ENTITY` → keep as-is.
   - `GEOM_LINE` + `POINT_A/POINT_B` → keep as-is.
   - `GEOM_LINE` + `ENTITY` (legacy accepted path) → expand to two candidate roles (`POINT_A`, `POINT_B`).
   - `GEOM_ARC` + `POINT_A/POINT_B/CENTER` → keep as-is.
   - Any other signature → fail with explicit ALONG unsatisfied reason.
2. Deduplicate normalized entries by `(entity, role, sub_index)` deterministically.
3. Feed normalized entries into existing coordinate-locking solve math.

### Pattern 2: Axis-parallel line semantics via orthogonal coordinate locks
**What:** ALONG X/Y/Z means direction parallel to that axis; equivalent to equal orthogonal coordinates across involved landmarks.  
**When to use:** ALONG solve updates.

**Rule map:**
- ALONG X → lock `Y` and `Z`, free `X`
- ALONG Y → lock `X` and `Z`, free `Y`
- ALONG Z → lock `X` and `Y`, free `Z`

For a single line participant (expanded to A/B), this yields endpoint orthogonal equality and therefore line-axis parallelism.

### Pattern 3: Transactional commit only after convergence
**What:** Keep all candidate edits in local arrays; apply to ECS geometry only after successful convergence.  
**When to use:** Always; do not mutate geometry in-branch on failure paths.

### Anti-Patterns to Avoid
- **Legality/runtime drift:** Allowing signatures through legality that ALONG runtime cannot resolve.
- **Partial mutation on unsatisfied ALONG:** Any geometry write before failure return breaks transactional contract.
- **Order-sensitive participant handling:** Non-canonical normalization/dedup introduces nondeterministic outcomes.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---|---|---|---|
| Participant signature validation | Ad hoc ALONG-only validator | `constraint_type_is_selection_legal` + signature model | Keeps UI/script/runtime legality parity |
| Point extraction from geometry roles | Manual endpoint math per branch | `scene_solver_ensure_point_candidate` + `scene_entity_participant_subpoint` | Centralizes role semantics and fixed-state metadata |
| Failure reporting | Custom logging path | `scene_solver_set_failure_implication` + `scene_solver_add_diagnostic` | Deterministic, test-covered diagnostics pipeline |

**Key insight:** Phase 27 is a parity/normalization fix inside existing solver architecture, not a new solver architecture.

## Common Pitfalls

### Pitfall 1: Treating ALONG as “perpendicular to axis” or “freeze axis”
**What goes wrong:** Endpoint updates satisfy wrong geometric relation.  
**Why it happens:** Misinterpreting free vs locked axes.  
**How to avoid:** Use explicit axis map above and add per-axis line-direction assertions in tests.  
**Warning signs:** ALONG X lines show varying Y/Z across endpoints after solve.

### Pitfall 2: Legacy line-entity participant not expanded
**What goes wrong:** Immediate unsatisfied failure for legal single-line ALONG authoring.  
**Why it happens:** `scene_solver_ensure_point_candidate` rejects line `ENTITY` role.  
**How to avoid:** Normalize line ENTITY → POINT_A/POINT_B in ALONG branch before candidate lookup.  
**Warning signs:** Failure reason `Unsatisfied driving ALONG * constraint.` on first solve with simple single-line ALONG.

### Pitfall 3: Hidden nondeterminism in mixed constraints
**What goes wrong:** Re-run produces different feasible geometry under ALONG+ANGLE+LENGTH+connectivity.  
**Why it happens:** Order-dependent participant insertion/dedupe or inconsistent tie-breaking.  
**How to avoid:** Deterministic normalization + rerun exact-equality tests (`vec3_exact_eq`) for feasible cases.  
**Warning signs:** First and second recalc produce coordinate drift without user edits.

## Code Examples

### Example: Current ALONG branch anchor (needs normalization fix)
```c
// Source: src/ecs/ecs_scene.h (around lines 3027+)
if (constraint->type == CONSTRAINT_ALONG_X ||
    constraint->type == CONSTRAINT_ALONG_Y ||
    constraint->type == CONSTRAINT_ALONG_Z) {
    // current logic gathers participant descriptors directly into point candidates
    // and locks orthogonal axes to averaged coordinates
}
```

### Example: Role restriction causing legacy mismatch
```c
// Source: src/ecs/ecs_scene.h (scene_solver_ensure_point_candidate)
if (g->type == GEOM_LINE) {
    if (role != CONSTRAINT_PARTICIPANT_ROLE_POINT_A &&
        role != CONSTRAINT_PARTICIPANT_ROLE_POINT_B) {
        return -1; // line ENTITY rejected at runtime
    }
}
```

## State of the Art

| Old Approach (current defect path) | Current Recommended Approach | When Changed | Impact |
|---|---|---|---|
| Legacy single-line ALONG accepted by legality but unresolved by runtime | Deterministic solve-time expansion of line ENTITY to endpoint participants | Phase 27 | Removes immediate ALONG line failure while preserving descriptor authority |

**Deprecated/outdated (for this phase):**
- Relying on raw line-entity ALONG solve semantics without endpoint normalization.

## Implementation Strategy

1. **Patch ALONG solve normalization in `scene_solver_request_recalculate`**
   - Add local helper-like block in ALONG branch to expand/collect normalized point participants deterministically.
   - Preserve existing all-fixed unsatisfied handling and ALONG family-specific failure reasons.

2. **Keep legality/runtime parity**
   - Keep `constraint_type_is_selection_legal` behavior consistent with runtime capability.
   - If signature handling changes, update endpoint legality tests in `endpoint_pick_test.c` in same patch.

3. **Add/extend regression tests (strictly Phase 27 scope)**
   - `scene_solver_contract_test.c`:
     - ALIN-01..03: single-line legacy ALONG X/Y/Z should solve when feasible.
     - ALIN-04: mixed ALONG+LENGTH+ANGLE+COINCIDENT deterministic rerun (`vec3_exact_eq`) and fixed-anchor transactional unsat cases.
   - `scene_solver_diagnostics_test.c`:
     - Add ALONG fixed-unsat family message checks for X/Y/Z.

4. **Do not change unrelated families**
   - No tangency drag behavior changes (Phase 28), no gizmo behavior (Phase 29), no docs closure (Phase 30).

## Risk Analysis

| Risk | Likelihood | Impact | Mitigation |
|---|---|---|---|
| Normalization introduces duplicate participant weighting bug | MEDIUM | HIGH | Deduplicate by normalized `(entity, role, sub_index)` before averaging |
| Mixed constraint oscillation / max-pass failures increase | MEDIUM | MEDIUM | Add ALIN-04 feasible deterministic rerun and infeasible transactional tests |
| Diagnostic regressions from message changes | LOW | MEDIUM | Add explicit ALONG diagnostics tests in `scene_solver_diagnostics_test.c` |
| Accidental scope creep into tangency/gizmo behavior | LOW | HIGH | Restrict edits to ALONG branch + ALONG-related tests only |

## Open Questions

1. **Should ALONG diagnostics in `scene_solver_diagnostics_test.c` use “driving” wording or family wording?**
   - What we know: Contract tests currently assert `Unsatisfied driving ALONG X constraint.` for unsatisfied ALONG X.
   - What's unclear: Whether diagnostics suite should mirror exact “driving” prefix for all ALONG axes.
   - Recommendation: Reuse exact existing ALONG wording to avoid drift and keep deterministic assertions.

2. **Do we need ALONG participant canonical sorting (like PARALLEL/PERPENDICULAR)?**
   - What we know: ALONG currently computes symmetric averages and dedupes candidates, reducing order sensitivity.
   - What's unclear: Whether future non-symmetric ALONG heuristics could break this.
   - Recommendation: For Phase 27, deterministic normalization + rerun tests are sufficient; defer broader canonicalization unless failing evidence appears.

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|---|---|---|---|---|
| cmake | Configure/build tests | ✓ | 4.3.0 | — |
| ctest | Running registered tests | ✓ | 4.3.0 | Run binaries directly |
| gcc | Native compilation (MSYS2 path) | ✓ | 15.2.0 | — |
| ninja | Optional generator | ✗ | — | Use default generator |
| MSVC `cl` | Visual Studio builds | ✗ (in current shell) | — | Use gcc toolchain shell |

**Missing dependencies with no fallback:**  
None for Phase 27 research/planning.

**Missing dependencies with fallback:**  
- `ninja`, `cl` unavailable in current shell; fallback to available generator/toolchain (`cmake` + `gcc` environment).

## Validation Architecture

### Test Framework
| Property | Value |
|---|---|
| Framework | C test executables registered through CMake/CTest |
| Config file | `CMakeLists.txt` + `src/CMakeLists.txt` |
| Quick run command | `cmake --build build --config Debug --target scene_solver_contract endpoint_pick scene_solver_diagnostics && ctest --test-dir build -R "scene_solver_contract|endpoint_pick|scene_solver_diagnostics" --output-on-failure` |
| Full suite command | `ctest --test-dir build --output-on-failure` |

### Phase Requirements → Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|---|---|---|---|---|
| ALIN-01 | Single-line ALONG X does not fail immediately and solves feasible case | unit/contract | `ctest --test-dir build -R scene_solver_contract --output-on-failure` | ✅ |
| ALIN-02 | Single-line ALONG Y same reliability behavior | unit/contract | `ctest --test-dir build -R scene_solver_contract --output-on-failure` | ✅ |
| ALIN-03 | Single-line ALONG Z same reliability behavior | unit/contract | `ctest --test-dir build -R scene_solver_contract --output-on-failure` | ✅ |
| ALIN-04 | ALONG + LENGTH + ANGLE + connectivity deterministic feasible solve + transactional unsat | unit/contract | `ctest --test-dir build -R scene_solver_contract --output-on-failure` | ✅ (add cases) |

### Sampling Rate
- **Per task commit:** `ctest --test-dir build -R "scene_solver_contract|endpoint_pick|scene_solver_diagnostics" --output-on-failure`
- **Per wave merge:** same targeted trio + any newly touched test binary
- **Phase gate:** full ctest suite green before `/gsd-verify-work`

### Wave 0 Gaps
- [ ] `src/tests/scene_solver_contract_test.c` — add explicit ALIN-01/02/03 single-line legacy ALONG feasibility tests.
- [ ] `src/tests/scene_solver_contract_test.c` — add ALIN-04 mixed ALONG+LENGTH+ANGLE+COINCIDENT deterministic rerun tests.
- [ ] `src/tests/scene_solver_diagnostics_test.c` — add ALONG X/Y/Z unsatisfied family diagnostics checks.
- [ ] Ensure local build tree has generated test binaries (`scene_solver_contract`, `endpoint_pick`, `scene_solver_diagnostics`) before execution.

## Sources

### Primary (HIGH confidence)
- `.planning/phases/27-principal-axis-line-along-reliability/27-CONTEXT.md` — locked decisions D-01..D-08 and phase scope.
- `.planning/REQUIREMENTS.md` — ALIN-01..ALIN-04 acceptance requirements.
- `src/constraints/constraint_types.h` — legality contracts for ALONG signatures.
- `src/ecs/ecs_scene.h` — ALONG runtime branch, candidate extraction rules, transactional commit/failure flow.
- `src/tests/scene_solver_contract_test.c` — existing ALONG transactional/deterministic test baselines.
- `src/tests/endpoint_pick_test.c` — legality signature tests and directional selection coverage.
- `src/tests/scene_solver_diagnostics_test.c` — diagnostics family-specific pattern baseline.
- `src/CMakeLists.txt` — test target registration and CTest integration.
- `.planning/config.json` — Nyquist validation enabled (`workflow.nyquist_validation: true`).

### Secondary (MEDIUM confidence)
- Environment probe from local shell (`cmake`, `ctest`, `gcc`, `ninja`, `cl`) for dependency availability.

### Tertiary (LOW confidence)
- None.

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — entirely repository-anchored and already in use.
- Architecture: HIGH — based on direct solver branch/source analysis.
- Pitfalls: HIGH — directly derived from current legality/runtime mismatch and existing test contracts.

**Research date:** 2026-04-09  
**Valid until:** 2026-05-09
