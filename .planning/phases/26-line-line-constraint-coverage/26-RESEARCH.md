# Phase 26: Line-Line Constraint Coverage - Research

**Researched:** 2026-04-08  
**Domain:** Sketch constraint legality/runtime/serialization for line-line relations  
**Confidence:** HIGH

## User Constraints

No `*-CONTEXT.md` exists for Phase 26 (`has_context: false` from init), so there are no additional locked user decisions beyond ROADMAP/REQUIREMENTS.

## Project Constraints (from copilot-instructions.md)

`./copilot-instructions.md` was not found in repository root, so no additional project-specific directives were discovered from that file.

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| LCON-01 | Apply `PARALLEL` between two sketch lines and solve when feasible | Add solver branch for `CONSTRAINT_PARALLEL` in `scene_solver_request_recalculate` with pair support and transactional unsat handling |
| LCON-02 | Apply `PARALLEL` to multi-line groups and all remain parallel after solve | Add N-ary `PARALLEL` semantics (deterministic anchor + align all others) and legality parity |
| LCON-03 | Apply `PERPENDICULAR` between two sketch lines and solve when feasible | Add solver branch for `CONSTRAINT_PERPENDICULAR` pair solve |
| LCON-04 | Apply `PERPENDICULAR` to supported multi-line selections with deterministic behavior | Support deterministic multi-line policy (recommended: canonical anchor + all others perpendicular) and selection-order invariant canonicalization |
| LCON-05 | Explicit legality feedback for invalid line-line selections | UI-level invalid-selection reasoning in constraint menu + preserve legality gate in `constraint_type_is_selection_legal` and script preview errors |

</phase_requirements>

## Summary

Phase 26 is mostly an internal parity gap: line-line `PARALLEL`/`PERPENDICULAR` are legal in portions of current selection rules (`constraint_types.h`) but are not implemented in transactional solve (`ecs_scene.h`). Today, unsupported types fall through to the generic failure `"Constraint type not yet solved in transactional recalc."`, which violates the phase goal and produces ambiguous UX.

The safest implementation is to keep the current architecture: legality gate -> create constraint -> transactional candidate solve -> commit on convergence -> deterministic diagnostics/failure implication. The codebase already has this pattern for `ALONG`, `ANGLE`, ARCI constraints and already has deterministic utility functions (`scene_solver_sort_entities_unique`, sorted implication lists, deterministic script emit ordering). Phase 26 should extend those patterns, not introduce new solver subsystems.

Primary recommendation: **implement line-line solve in `scene_solver_request_recalculate` with canonicalized participant ordering for symmetric relations, and add focused CTest coverage for legality, solve success, unsat transactional rollback, and selection-order determinism.**

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| cglm | 0.9.6 | Vector math for solver operations | Already project-locked in state and used throughout solver math |
| flecs | 4.1.4 | ECS storage/query for sketch entities and constraints | Current architecture depends on ECS queries/components |
| cJSON | 1.7.19 | Scene JSON serialization/deserialization | Existing scene persistence path already implemented on cJSON |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| CTest (CMake) | 4.3.0 (env) | Targeted deterministic regression execution | Per-task and phase-gate solver checks |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Extend current transactional solver | New external solver backend | High integration risk; explicitly out of scope for v1.4 |

**Build/Test Commands:**
```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure -R "scene_solver_contract|scene_solver_diagnostics|endpoint_pick|script_roundtrip"
```

**Version verification (repo/vendor + environment):**
- `vendors/cglm/include/cglm/version.h` -> 0.9.6
- `vendors/flecs/flecs.h` -> 4.1.4
- `vendors/cjson/cJSON.h` -> 1.7.19
- `cmake --version` / `ctest --version` -> 4.3.0 available in environment

## Architecture Patterns

### Recommended Project Structure
```text
src/
├── constraints/constraint_types.h        # Legality matrix for selection signatures
├── ecs/ecs_scene.h                       # Constraint creation + transactional solve
├── app.c                                 # Constraint-menu authoring and user feedback
├── scene_serializer.h                    # JSON persistence of constraints/descriptors
├── scripting/sketch_script_*.h           # Script emit/parse/apply constraint roundtrip
└── tests/*.c                             # CTest executables for contract/diagnostic/policy
```

### Pattern 1: Legality/runtime parity at creation boundary
**What:** Keep `constraint_type_is_selection_legal(...)` and `scene_add_constraint_to_sketch_with_descriptors(...)` as the single legality gate before runtime solve.  
**When to use:** Every constraint type addition or participant-shape expansion.  
**Example (existing gate):**
```c
// src/ecs/ecs_scene.h
if (!constraint_type_is_selection_legal(&signature, type)) return 0;
```

### Pattern 2: Transactional solve with deferred commit
**What:** Solve into candidate buffers and only write geometry back on convergence; on failure, preserve prior geometry and emit deterministic diagnostics/failure implication.  
**When to use:** All new line-line solve branches (`PARALLEL`, `PERPENDICULAR`).  

### Pattern 3: Deterministic canonicalization for symmetric constraints
**What:** Canonicalize line participants (sort by stable key/entity ID, dedupe exact duplicates) for symmetric relations so selection order does not alter result.  
**When to use:** Pair/group `PARALLEL` and group-enabled `PERPENDICULAR`.

### Anti-Patterns to Avoid
- **Order-dependent participant semantics for symmetric constraints:** leads to different outcomes for equivalent selections (violates LCON-04 determinism intent).
- **Adding legality in UI only:** legality must remain enforced in core (`constraint_types.h` + add-constraint gate), not just menu filtering.
- **Generic unsupported failure reason for supported types:** must emit family-specific unsat reasons for diagnostics quality.

## Current Code Touchpoints (Legality, Runtime, Serialization)

| Concern | File | Current State | Phase 26 Action |
|---|---|---|---|
| Selection legality matrix | `src/constraints/constraint_types.h` | `PARALLEL` legal for 2+ lines; `PERPENDICULAR` legal only 2 lines | Decide and encode multi-line `PERPENDICULAR` support contract to satisfy LCON-04 |
| Constraint authoring gate | `src/ecs/ecs_scene.h` (`scene_add_constraint_to_sketch_with_descriptors`) | Builds signature + enforces legality before creation | Add canonicalization for symmetric line-line participant lists before storing |
| UI legality exposure | `src/app.c` (`mdcad_draw_constraint_context_menu`) | Shows only legal constraints; invalid cases show generic “No applicable constraints…” | Add explicit invalid-selection reasons for line-line failures (LCON-05) |
| Runtime transactional solve | `src/ecs/ecs_scene.h` (`scene_solver_request_recalculate`) | No branches for `PARALLEL`/`PERPENDICULAR`; falls to generic unsupported error | Implement deterministic pair/group solve branches + explicit unsat reason strings |
| JSON persistence | `src/scene_serializer.h` | Stores type, participants, participant_descriptors; remaps entities on load | No format changes required; add regression tests for new types |
| Script emit/parse/apply | `src/scripting/sketch_script_emit.h`, `parse.h`, `apply.h` | Type names are generic; participants are entity IDs (entity-role only) | Works for line-line constraints; add roundtrip tests for `Parallel`/`Perpendicular` |

## Deterministic Ordering / Canonicalization Strategy

1. **Canonical participant list for symmetric line-line types (`PARALLEL`, `PERPENDICULAR`)**
   - Sort by `(entity, role, sub_index)` ascending.
   - Remove exact duplicates before creation.
   - Reject effective participant count below minimum after dedupe.

2. **Deterministic anchor policy**
   - For N-ary constraints: first canonical participant is anchor/reference line.
   - Pair constraints still use canonical order for deterministic target/adjustment policy when both lines are editable.

3. **Deterministic direction sign policy**
   - Preserve each line’s current orientation by choosing the target direction with maximal dot-product to current line direction.
   - This avoids spontaneous flips between equivalent runs/selections.

4. **Stable failure implication**
   - Reuse existing sorted implication machinery (`scene_solver_sort_entities_unique`) so unsat reports remain stable across runs.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Ad-hoc selection validation in multiple call sites | Separate UI-only legality trees | `constraint_type_is_selection_legal` + descriptor signature path | Prevent legality/runtime drift |
| Custom ad-hoc rollback mechanism | Partial direct writes during solve | Existing candidate-buffer transactional recalc pattern | Already proven by existing tests; lower regression risk |
| New serializer format for line-line constraints | Special-case schema branch | Existing constraint JSON/script fields | Type + participant arrays already sufficient |

**Key insight:** The architecture already has robust patterns for deterministic, transactional constraints; Phase 26 should plug into those patterns rather than introducing new abstractions.

## Common Pitfalls

### Pitfall 1: Selection-order dependence in symmetric constraints
- **What goes wrong:** Same set of lines solves differently by click order.
- **Why it happens:** Storing/solving participants in raw selection order.
- **How to avoid:** Canonicalize participant descriptors before persistence/solve.
- **Warning signs:** Determinism tests fail only when participant order is permuted.

### Pitfall 2: Legality/runtime mismatch
- **What goes wrong:** Constraint is creatable but always solver-fails (or vice versa).
- **Why it happens:** `constraint_types.h` updated without solver branch (current state for line-line).
- **How to avoid:** Pair legality edits with explicit runtime branch and tests for both success + unsat.
- **Warning signs:** Generic `"Constraint type not yet solved..."` diagnostics for “supported” constraints.

### Pitfall 3: Breaking transactional guarantees
- **What goes wrong:** Unsatisfied solve mutates geometry.
- **Why it happens:** Direct geometry writes before convergence/failure outcome.
- **How to avoid:** Keep all updates in candidate buffers until success path commit.
- **Warning signs:** Pre/post geometry differs on expected-unsat test.

### Pitfall 4: Ambiguous multi-line `PERPENDICULAR` semantics
- **What goes wrong:** Implementation “works” but behaves unpredictably for >2 lines.
- **Why it happens:** No explicit deterministic contract for supported group behavior.
- **How to avoid:** Document and enforce one policy (recommended anchor-line policy) in legality + solver + tests.
- **Warning signs:** Different outcomes across mirrored/grouped selection permutations.

## Likely Failure Modes + Guardrails (Transactional Solve)

| Failure Mode | Guardrail |
|---|---|
| Unsupported branch falls to generic solver failure | Add explicit `CONSTRAINT_PARALLEL` and `CONSTRAINT_PERPENDICULAR` branches with family-specific unsat reasons |
| Duplicate/same-line participants create degenerate vectors | Canonical dedupe + reject constraints with <2 unique lines |
| All relevant endpoints fixed but residual > tolerance | Return unsat with deterministic implication and no geometry commit |
| Multi-constraint feedback loop fails convergence | Keep max-pass diagnostics (`"max passes reached"`) and ensure implication lists stay sorted |
| Selection-order nondeterminism | Canonical participant ordering at creation + deterministic anchor/sign policy |

## Code Examples

### Legality + creation boundary (existing contract to preserve)
```c
// src/ecs/ecs_scene.h
if (!constraint_type_is_selection_legal(&signature, type)) return 0;
ecs_entity_t constraint_e = scene_add_anchor(scene, "", "");
```

### Deterministic implication sorting (reuse)
```c
// src/ecs/ecs_scene.h
scene_solver_sort_entities_unique(sorted_constraints, &sorted_constraint_count);
scene_solver_sort_entities_unique(imp->participants, &imp->participant_count);
```

## State of the Art (within this codebase)

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Direct/implicit behavior for some constraints | Explicit descriptor-based participant roles + transactional solver | v1.3/v1.4 groundwork | Enables deterministic, testable constraint behavior |
| Generic selection paths | Signature-driven legality checks | Existing v1.2+ | Makes feature expansion safer if parity maintained |
| Weak deterministic diagnostics | Sorted implications + diagnostic dedupe | Existing tests in `scene_solver_diagnostics_test.c` | Stable failure UX and reproducible debugging |

**Deprecated/outdated for this phase:**
- Relying on generic unsupported solver message for line-line constraints.

## Test Strategy (Targeted CTest Additions/Updates)

### Recommended test files
- **Update** `src/tests/scene_solver_contract_test.c`
  - Add line-line solve success + unsat transactional tests.
- **Update** `src/tests/scene_solver_diagnostics_test.c`
  - Add family-specific diagnostic assertions for parallel/perpendicular unsat.
- **Update** `src/tests/endpoint_pick_test.c`
  - Add legality coverage for valid/invalid multi-line perpendicular signatures.
- **Update** `src/tests/script_roundtrip_tests.c`
  - Add parse/apply/emit roundtrip with `Parallel` and `Perpendicular`.

### Requirement-to-test mapping
| Req ID | Behavior to prove | Test Type | Command |
|---|---|---|---|
| LCON-01 | Pair `PARALLEL` solves feasible lines | unit/contract | `ctest --test-dir build -R scene_solver_contract --output-on-failure` |
| LCON-02 | Group `PARALLEL` keeps all participants parallel | unit/contract | `ctest --test-dir build -R scene_solver_contract --output-on-failure` |
| LCON-03 | Pair `PERPENDICULAR` solves feasible lines | unit/contract | `ctest --test-dir build -R scene_solver_contract --output-on-failure` |
| LCON-04 | Multi-line `PERPENDICULAR` deterministic supported behavior | unit/contract + determinism rerun | `ctest --test-dir build -R scene_solver_contract --output-on-failure` (run twice) |
| LCON-05 | Invalid line-line selections produce explicit feedback | legality + diagnostics + script preview | `ctest --test-dir build -R \"endpoint_pick|scene_solver_diagnostics|script_roundtrip\" --output-on-failure` |

## Recommended Implementation Sequence (Low Regression Risk)

1. **Define behavior contract first**
   - Finalize multi-line `PERPENDICULAR` semantics and canonicalization rules.
2. **Legality parity updates**
   - Update `constraint_type_is_selection_legal` and context-menu feedback reasons.
3. **Canonicalization in constraint creation**
   - Normalize symmetric line-line participants in `scene_add_constraint_to_sketch_with_descriptors`.
4. **Runtime solver branches**
   - Implement `PARALLEL` and `PERPENDICULAR` in `scene_solver_request_recalculate`.
5. **Diagnostics and implication polish**
   - Add family-specific unsat messages and confirm sorted implications.
6. **Serialization/script regression pass**
   - Add roundtrip tests (no format changes expected).
7. **Determinism rerun gate**
   - Repeat targeted CTest runs to verify stable outcomes.

## Open Questions

1. **Exact multi-line `PERPENDICULAR` semantics**
   - What we know: Requirement asks “supported multi-line selections” and deterministic runtime.
   - Unclear: Whether support means N-ary single constraint or deterministic expansion into pair constraints.
   - Recommendation: Lock one policy before coding; implement tests that encode that policy explicitly.

2. **UI legality feedback granularity**
   - What we know: Current menu gives generic “No applicable constraints...” message.
   - Unclear: Desired user-facing reason text granularity.
   - Recommendation: Add minimally explicit reasons for line-line invalid cases (non-line participant, cross-sketch mix, duplicate entity set).

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| cmake | Configure/build tests | ✓ | 4.3.0 | — |
| ctest | Targeted validation runs | ✓ | 4.3.0 | — |
| gcc | Native C compilation path in this env | ✓ | 15.2.0 | — |
| clang | Optional compiler matrix | ✗ | — | Use gcc |
| MSVC `cl` | Optional Windows compiler matrix | ✗ | — | Use gcc toolchain in this environment |

**Missing dependencies with no fallback:** None identified for Phase 26 implementation/testing.  
**Missing dependencies with fallback:** clang, cl (fallback: gcc).

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | CTest + C unit executables |
| Config file | `src/CMakeLists.txt` (`include(CTest)` + `add_test(...)`) |
| Quick run command | `ctest --test-dir build -R "scene_solver_contract|endpoint_pick" --output-on-failure` |
| Full targeted suite command | `ctest --test-dir build -R "scene_solver_contract|scene_solver_diagnostics|scene_solver_pass_policy|endpoint_pick|script_roundtrip" --output-on-failure` |

### Phase Requirements → Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| LCON-01 | Pair parallel solve success | unit/contract | `ctest --test-dir build -R scene_solver_contract --output-on-failure` | ✅ (extend) |
| LCON-02 | Group parallel solve success | unit/contract | `ctest --test-dir build -R scene_solver_contract --output-on-failure` | ✅ (extend) |
| LCON-03 | Pair perpendicular solve success | unit/contract | `ctest --test-dir build -R scene_solver_contract --output-on-failure` | ✅ (extend) |
| LCON-04 | Multi-line perpendicular deterministic behavior | unit/contract | `ctest --test-dir build -R scene_solver_contract --output-on-failure` | ✅ (extend) |
| LCON-05 | Explicit legality feedback for invalid selection | legality/diagnostics | `ctest --test-dir build -R "endpoint_pick|scene_solver_diagnostics|script_roundtrip" --output-on-failure` | ✅ (extend) |

### Sampling Rate
- **Per task commit:** `ctest --test-dir build -R "scene_solver_contract|endpoint_pick" --output-on-failure`
- **Per wave merge:** `ctest --test-dir build -R "scene_solver_contract|scene_solver_diagnostics|scene_solver_pass_policy|endpoint_pick|script_roundtrip" --output-on-failure`
- **Phase gate:** Repeat full targeted suite twice to verify deterministic rerun stability.

### Wave 0 Gaps
- [ ] Add LCON-specific `PARALLEL`/`PERPENDICULAR` tests to `scene_solver_contract_test.c`
- [ ] Add LCON family-specific unsat diagnostic assertions to `scene_solver_diagnostics_test.c`
- [ ] Add multi-line perpendicular legality + explicit invalid cases in `endpoint_pick_test.c`
- [ ] Add script roundtrip test coverage for `Parallel`/`Perpendicular` in `script_roundtrip_tests.c`

## Sources

### Primary (HIGH confidence)
- `src/constraints/constraint_types.h` — legality matrix for line-line and directional constraints
- `src/ecs/ecs_scene.h` — constraint creation gate, transactional solver flow, deterministic implication sorting
- `src/app.c` — constraint context collection and menu legality exposure
- `src/scene_serializer.h` — JSON persistence for constraints/participant descriptors
- `src/scripting/sketch_script_emit.h`, `parse.h`, `apply.h` — script persistence and legality validation path
- `src/tests/scene_solver_contract_test.c`, `scene_solver_diagnostics_test.c`, `endpoint_pick_test.c`, `script_roundtrip_tests.c` — current test coverage and gaps
- `src/CMakeLists.txt` — CTest executable/test registration
- `.planning/ROADMAP.md`, `.planning/REQUIREMENTS.md`, `.planning/research/SUMMARY.md`, `.planning/research/ARCHITECTURE.md`, `.plans/phase-26-issues-and-observations.md`

### Secondary (MEDIUM confidence)
- `.planning/STATE.md` milestone constraints and locked backend context
- `.planning/config.json` (`workflow.nyquist_validation: true`)

### Tertiary (LOW confidence)
- None

## Metadata

**Confidence breakdown:**
- Standard stack: **HIGH** — directly verified from vendored headers and CMake/CTest config
- Architecture: **HIGH** — based on current in-repo solver/legality/serialization implementation
- Pitfalls: **HIGH** — derived from existing failure paths and current test/doc evidence

**Research date:** 2026-04-08  
**Valid until:** 2026-05-08 (stable internal architecture, re-verify if solver core changes)
