# Phase 23: Principal-Direction Constraint Expansion - Research

**Researched:** 2026-04-08  
**Domain:** Sketch constraint legality + solver expansion for `ALONG X/Y/Z` over point and landmark participants in deterministic transactional recalculate flow  
**Confidence:** HIGH

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
### Directional authoring scope
- **D-01:** `ALONG X`, `ALONG Y`, and `ALONG Z` must support participant pairs and larger groups (within existing participant limits).
- **D-02:** Supported participant sources must include:
  - standalone point entities
  - line endpoint landmark points (`POINT_A`, `POINT_B`)
  - arc landmark points (`POINT_A`, `POINT_B`, `CENTER`)
- **D-03:** Existing single-line directional constraint behavior remains backward-compatible where currently legal.

### Legality and participant model
- **D-04:** Directional legality must move from single-line-only to point-participant signatures aligned with AXIS-01..04.
- **D-05:** Sub-entity landmark roles must be legal for directional constraints (line/arc endpoint descriptors are first-class participants).
- **D-06:** Keep deterministic selection legality contracts shared between UI authoring and script apply validation.

### Solver/runtime behavior
- **D-07:** Recalculate must remain functional when directional constraints coexist with `ANGLE` constraints (`SRLV-04`).
- **D-08:** Directional solving must preserve deterministic transactional behavior: explicit failure path and no partial-mutation leaks on unsatisfied solves.
- **D-09:** Solver authority remains in `scene_solver_*` APIs; UI and app loop stay thin callers.

### Validation posture
- **D-10:** Phase 23 requires executable regression coverage for directional legality and solve behavior across standalone + landmark participants.
- **D-11:** Include targeted coexistence tests (`ALONG*` + `ANGLE`) to prevent reintroducing non-functional recalc pathways.

### the agent's Discretion
- Exact participant-count bounds enforcement strategy within existing `CONSTRAINT_MAX_PARTICIPANTS`.
- Exact directional solve implementation details (projection strategy/order) as long as deterministic contracts hold.
- Exact split of test fixtures across existing solver test binaries vs new phase-specific test target(s).

### Deferred Ideas (OUT OF SCOPE)
- Arc-center-axis relation and tangency semantics (`ARCI-01..04`) remain Phase 24.
- Broad regression-packaging closure (`V13-01`) remains Phase 25.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| AXIS-01 | User can apply `ALONG X` to pairs and groups of point participants to constrain them along principal X direction. | Legality update to point-participant signatures + axis projection solve pass for participant groups. |
| AXIS-02 | User can apply `ALONG Y` to pairs and groups of point participants to constrain them along principal Y direction. | Same solve architecture as AXIS-01 with Y-axis coordinate equality residual and deterministic pass behavior. |
| AXIS-03 | User can apply `ALONG Z` to pairs and groups of point participants to constrain them along principal Z direction. | Same solve architecture as AXIS-01 with Z-axis coordinate equality residual and deterministic pass behavior. |
| AXIS-04 | Directional group constraints support standalone points, line endpoints, and arc landmark points. | Descriptor-first participant resolution via existing `scene_add_constraint_to_sketch_with_descriptors(...)`, endpoint role handling, and CENTER support extension in candidate resolution. |
| SRLV-04 | Recalculate remains functional when `ALONG X`, `ALONG Y`, `ALONG Z`, and `ANGLE` constraints are present. | New ALONG solver branch in transactional recalc loop; coexistence contract tests with ANGLE and deterministic pass outcomes. |
</phase_requirements>

## Summary

Phase 23 is an in-place expansion of existing architecture, not a rewrite. The required foundations already exist: descriptor-based constraint creation (`scene_add_constraint_to_sketch_with_descriptors`), deterministic solver orchestration and transactional commit/no-mutation failure (`scene_solver_request_recalculate`), bounded pass policy with `max_passes`, and diagnostics/implication contracts hardened in Phase 22.

Current blockers are explicit in code: directional legality currently rejects sub-entity roles and only permits a single line participant (`constraint_type_is_selection_legal` in `src/constraints/constraint_types.h`), and the solver has no `CONSTRAINT_ALONG_X/Y/Z` branch, so recalc fails with `"Constraint type not yet solved in transactional recalc."` when ALONG constraints are present. This directly blocks AXIS-01..04 and SRLV-04.

**Primary recommendation:** Implement Phase 23 as a focused three-layer extension: (1) legality model update to point-participant signatures including landmark roles, (2) participant candidate resolution update to include arc center role, and (3) deterministic ALONG axis-equality solve branch integrated into the existing pass loop and failure diagnostics contracts.

## Standard Stack

### Core
| Library/Module | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| `src/constraints/constraint_types.h` | repo-current | Central legality matrix for constraint type + participant signature | Shared source of truth for menu legality and script validation contracts. |
| `src/ecs/ecs_scene.h` (`scene_solver_*`) | repo-current | Constraint creation, descriptor storage, recalculate solver authority, diagnostics | Existing hard authority boundary from prior phases; deterministic behavior already enforced here. |
| `ConstraintComp` + descriptor model (`src/components/constraint_comp.h`) | repo-current | Stores participant entities and landmark roles (`participant_descriptors`) | Enables mixed participant semantics without UI-owned solver logic. |
| CTest native C binaries (`src/CMakeLists.txt`) | repo-current | Regression gates for solver/legality behavior | Existing suite already includes `scene_solver_contract`, `scene_solver_pass_policy`, `scene_solver_trigger`, `endpoint_pick`, `scene_solver_diagnostics`. |

### Supporting
| Library/Module | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| `src/components/endpoints_comp.h` | repo-current | Landmark role model (`POINT_A`, `POINT_B`, `CENTER`) and owner/endpoint mapping | Required for AXIS-04 mixed participant support. |
| `src/app.c` constraint context menu | repo-current | Descriptor-aware constraint creation path | Constraint authoring already passes descriptors into scene API. |
| `src/scripting/sketch_script_apply.h` | repo-current | Script-side legality validation | Must remain synchronized with legality contract (D-06). |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Scene-owned ALONG solve in `scene_solver_request_recalculate` | UI/app-time axis projection mutations | Violates D-09 solver authority and breaks transactional determinism. |
| Descriptor-first landmark model | Endpoint-entity-specific ad hoc logic in each caller | Duplicates role logic and risks legality/solver drift. |

**Installation:** Not applicable (in-repo C/CMake stack only).  
**Version verification:** Not applicable for npm packages in this phase.

## Architecture Patterns

### Recommended Project Structure
```text
src/
├── constraints/constraint_types.h         # Legality signatures for ALONG expansion
├── ecs/ecs_scene.h                        # Descriptor resolution + transactional ALONG solve pass
├── scripting/sketch_script_apply.h        # Shared legality validation (must remain deterministic)
└── tests/
   ├── endpoint_pick_test.c                # Legality signatures for endpoint/arc landmark roles
   ├── scene_solver_contract_test.c        # ALONG + ANGLE coexistence and transactional behavior
   └── scene_solver_pass_policy_test.c     # Deterministic pass-bound behavior with ALONG groups
```

### Pattern 1: Descriptor-first participant legality
**What:** Evaluate legality using `geometry_type + participant_role` signature, not entity-only assumptions.  
**When to use:** Any ALONG authoring/validation path (menu + script apply).  
**Example:**
```c
// Source: src/ecs/ecs_scene.h
constraint_selection_signature_t signature = {0};
signature.geometry_types[i] = g->type;
signature.roles[i] = role;
if (!constraint_type_is_selection_legal(&signature, type)) return 0;
```

### Pattern 2: Transactional pass-loop extension (no partial mutation leak)
**What:** Add ALONG solve branch inside existing candidate-stage loop, commit only after convergence, preserve explicit failure path.  
**When to use:** All AXIS constraints and ALONG+ANGLE coexistence in recalc.  
**Example:**
```c
// Source: src/ecs/ecs_scene.h
for (uint32_t pass = 0; pass < max_passes && !solve_failed; pass++) {
    // ... per-constraint adjustments into candidates[]
}
if (solve_failed || !converged) { /* implication + diagnostic + return false */ }
for (int i = 0; i < candidate_count; i++) { scene_apply_local_point_to_participant(...); }
```

### Pattern 3: Deterministic diagnostics + implication lifecycle
**What:** Use existing deterministic implication sorting and diagnostics dedupe contracts for new ALONG failure modes.  
**When to use:** Unsupported role combos, impossible fixed-point axis equality, pass-cap failures.  
**Example:**
```c
// Source: src/ecs/ecs_scene.h
scene_solver_sort_entities_unique(implicated_constraints, &implicated_constraint_count);
scene_solver_set_failure_implication(scene, sketch, implicated_constraints, implicated_constraint_count, reason);
scene_solver_add_diagnostic(scene, sketch, SKETCH_SOLVER_DIAG_ERROR, "solve", reason, implicated_constraints[0]);
```

### Anti-Patterns to Avoid
- **UI-owned ALONG math:** breaks scene-owned solver authority (D-09).
- **Special-casing endpoint entities instead of participant descriptors:** causes legality drift.
- **Relaxing deterministic ordering for group participants:** introduces flaky recalc outcomes.
- **Auto-clearing diagnostics history on success:** violates established diagnostics contract.

## Explicit Decisions (Research Recommendations)

- **D-12 (Legality model):** `CONSTRAINT_ALONG_X/Y/Z` legality must accept **2..CONSTRAINT_MAX_PARTICIPANTS** point participants resolved from: standalone points (`ENTITY` on `GEOM_POINT`), line `POINT_A/POINT_B`, arc `POINT_A/POINT_B/CENTER`; reject raw line/entity and arc/entity ALONG signatures.
- **D-13 (Participant resolution model):** `scene_solver_ensure_point_candidate(...)` must accept `CONSTRAINT_PARTICIPANT_ROLE_CENTER` for `GEOM_ARC` and preserve existing line endpoint and point-entity behaviors; this becomes the single runtime projection entrypoint.
- **D-14 (Solver behavior with ALONG + ANGLE):** ALONG applies axis-equality projection across all non-fixed participants each pass (target coordinate = deterministic mean of movable+fixed participants on that axis); ANGLE processing remains in existing branch; convergence is unchanged (`position_tolerance`, `angle_tolerance`, `max_passes`).
- **D-15 (Diagnostics behavior):** ALONG unsatisfied due to all participants fixed and residual > tolerance yields explicit error diagnostic (`"Unsatisfied driving ALONG <axis> constraint."`) with implication list containing the failing ALONG constraint only; pass-cap retains `"max passes reached"` contract.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Mixed participant legality | Separate UI legality tables per view | Existing `constraint_type_is_selection_legal(...)` signature model | Prevents menu/script inconsistency; D-06 requires shared legality contract. |
| ALONG group participant storage | New custom participant schema | Existing `participant_descriptors[]` in `ConstraintComp` | Already integrated with scene add path and role metadata. |
| Solver failure highlighting | New UI-specific error marker path | `scene_solver_set_failure_implication(...)` and diagnostics ring | Deterministic ordering + existing tests already validate behavior. |
| Recalculate orchestration | Additional recalc path outside `scene_solver_request_recalculate` | Extend current transactional pass loop | Preserves Phase 22 deterministic and no-partial-mutation guarantees. |

**Key insight:** AXIS expansion should be implemented as a narrow extension to existing descriptor + transactional solver infrastructure; most complexity is in legality and role resolution, not new architecture.

## Common Pitfalls

### Pitfall 1: Legality/UI mismatch between app menu and script apply
**What goes wrong:** Selection menu allows ALONG signature that script apply rejects (or vice versa).  
**Why it happens:** Divergent legality logic outside shared `constraint_type_is_selection_legal`.  
**How to avoid:** Keep all ALONG signature rules in `constraint_types.h`; script apply continues consuming the same helper.  
**Warning signs:** same participant set succeeds in UI but fails with `"Constraint participants are not legal for type."` in script apply.

### Pitfall 2: Arc center role supported in data model but not in candidate resolver
**What goes wrong:** Constraint creation succeeds, recalc fails with unsupported participant diagnostic.  
**Why it happens:** `scene_solver_ensure_point_candidate` currently only permits line/arc `POINT_A|POINT_B`.  
**How to avoid:** Extend candidate resolver to accept `CENTER` for `GEOM_ARC` (D-13).  
**Warning signs:** ALONG with arc center immediately errors without mutation.

### Pitfall 3: Non-deterministic group solve due to unstable target coordinate selection
**What goes wrong:** repeated recalc on unchanged sketch produces different placements.  
**Why it happens:** unordered participant traversal or branch-dependent anchor choice.  
**How to avoid:** deterministic aggregation (stable participant order + deterministic mean rule).  
**Warning signs:** idempotence assertions fail intermittently.

### Pitfall 4: SRLV-04 regression via unsupported ALONG branch
**What goes wrong:** sketches containing ALONG + ANGLE fail on recalc even when satisfiable.  
**Why it happens:** solver default fallback `"Constraint type not yet solved..."` still catches ALONG.  
**How to avoid:** explicit ALONG branch before unsupported fallback plus coexistence tests.  
**Warning signs:** any ALONG constraint immediately sets `SKETCH_STATUS_ERROR`.

## Code Examples

### Descriptor-based constraint creation already used by authoring path
```c
// Source: src/app.c
ecs_entity_t created = scene_add_constraint_to_sketch_with_descriptors(
    &state.ecs_scene,
    state.constraint_menu_sketch,
    type,
    state.constraint_menu_participants,
    state.constraint_menu_participant_count,
    initial_value,
    false);
```

### Existing deterministic pass policy and transactional commit contract
```c
// Source: src/ecs/ecs_scene.h
if (!converged) {
    scene_solver_set_failure_implication(..., "max passes reached");
    scene_solver_add_diagnostic(..., "max passes reached", ...);
    return false;
}
for (int i = 0; i < candidate_count; i++) {
    scene_apply_local_point_to_participant(g, candidates[i].role, candidates[i].point);
}
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| ALONG legal only as single line, no landmark roles | Descriptor-aware constraint creation exists but legality gate still line-only | v1.2 → v1.3 transition | Phase 23 can unlock mixed point/landmark ALONG by changing one legality authority. |
| Recalc had no ALONG solver branch | Transactional multi-pass solver supports Coincident/Length/Angle and deterministic failure diagnostics | Phase 22 | ALONG can be added as another branch without architecture churn. |
| Point candidate resolver only endpoint roles for non-points | Candidate infrastructure already handles entity+role and fixed states | Phase 17/22 | Minimal targeted extension needed for arc CENTER role. |

**Deprecated/outdated (for Phase 23 scope):**
- `CONSTRAINT_ALONG_*` as single-line-only semantic in legality.
- Treating ALONG as “declared type only” without transactional recalc support.

## Open Questions

1. **Should ALONG group target coordinate be mean or anchor-based when all participants are movable?**
   - What we know: deterministic rule is required; mean is deterministic and symmetric.
   - What's unclear: UX preference for preserving first participant as anchor.
   - Recommendation: adopt deterministic mean in Phase 23 for stability; defer UX anchoring variants unless explicitly requested.

2. **Script grammar support for landmark-role participants**
   - What we know: script apply currently builds signatures with `ROLE_ENTITY` only.
   - What's unclear: whether Phase 23 requires script syntax changes or just UI/runtime support.
   - Recommendation: keep script grammar unchanged in this phase (out of explicit requirements) but ensure existing entity-only script constraints remain valid.

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| cmake | Configure/build tests | ✓ | 4.3.0 | — |
| ctest | Run phase validation targets | ✓ | 4.3.0 | — |
| gcc | Native C test compilation | ✓ | 15.2.0 | — |
| git | Workflow tooling/history checks | ✓ | 2.51.1.windows.1 | — |
| node | GSD orchestration scripts | ✓ | v25.9.0 | — |
| python | Auxiliary scripting if needed | ✓ | 3.13.12 | — |

**Missing dependencies with no fallback:**
- None identified for Phase 23 execution in this environment.

**Missing dependencies with fallback:**
- None.

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | CTest + native C executable tests |
| Config file | `CMakeLists.txt`, `src/CMakeLists.txt` |
| Quick run command | `ctest --output-on-failure -R "endpoint_pick|scene_solver_contract|scene_solver_pass_policy"` |
| Full suite command | `ctest --output-on-failure` |

### Phase Requirements → Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| AXIS-01 | ALONG X legal + solves for point pair/group | unit/contract | `ctest --output-on-failure -R "endpoint_pick|scene_solver_contract"` | ⚠️ Partial (needs extension) |
| AXIS-02 | ALONG Y legal + solves for point pair/group | unit/contract | `ctest --output-on-failure -R "endpoint_pick|scene_solver_contract"` | ⚠️ Partial (needs extension) |
| AXIS-03 | ALONG Z legal + solves for point pair/group | unit/contract | `ctest --output-on-failure -R "endpoint_pick|scene_solver_contract"` | ⚠️ Partial (needs extension) |
| AXIS-04 | Mixed participant sources: point + line endpoint + arc landmark (incl center) | unit/contract | `ctest --output-on-failure -R "endpoint_pick|scene_solver_contract"` | ⚠️ Partial (needs extension) |
| SRLV-04 | Recalculate works with ALONG + ANGLE constraints | unit/contract | `ctest --output-on-failure -R "scene_solver_contract|scene_solver_pass_policy"` | ⚠️ Partial (needs extension) |

### Sampling Rate
- **Per task commit:** `ctest --output-on-failure -R "endpoint_pick|scene_solver_contract|scene_solver_pass_policy"`
- **Per wave merge:** `ctest --output-on-failure -R "scene_solver_(contract|pass_policy|trigger|diagnostics)|endpoint_pick"`
- **Phase gate:** Full suite green before `/gsd-verify-work`

### Wave 0 Gaps
- [ ] `src/tests/endpoint_pick_test.c` — extend legality tests: ALONG X/Y/Z legal for role signatures (`POINT_A/POINT_B/CENTER`) and illegal for raw line/entity ALONG forms.
- [ ] `src/tests/scene_solver_contract_test.c` — add ALONG X/Y/Z pair/group satisfiable solve tests for standalone points.
- [ ] `src/tests/scene_solver_contract_test.c` — add mixed participant solve tests (`point + line endpoint`, `point + arc endpoint`, `point + arc center`) for AXIS-04.
- [ ] `src/tests/scene_solver_contract_test.c` — add ALONG + ANGLE coexistence recalc tests to lock SRLV-04.
- [ ] `src/tests/scene_solver_pass_policy_test.c` — add ALONG group pass-cap/tolerance determinism tests (including max-pass diagnostic preservation).

## Proposed Test Architecture (Wave Plan)

- **Wave 0A (legality contract):** `endpoint_pick_test.c` only, fast static legality matrix expansion for ALONG role signatures.
- **Wave 0B (solver contract):** `scene_solver_contract_test.c` ALONG projection and coexistence behavior; includes deterministic repeated recalc checks.
- **Wave 0C (pass policy):** `scene_solver_pass_policy_test.c` verifies ALONG participates correctly in convergence/`max_passes reached` behavior.
- **Wave 1 (implementation):** legality + candidate resolution + solver branch implementation.
- **Wave 2 (stabilization):** rerun targeted suite + full suite and update verification artifacts.

## Risks, Fallback Strategy, Out-of-Scope

### Risks
1. **Legality broadening accidentally enables unsupported non-point ALONG signatures**  
   Mitigation: explicit negative tests for line/entity and arc/entity ALONG forms.
2. **Arc CENTER role support destabilizes existing endpoint sync behavior**  
   Mitigation: keep role handling centralized in candidate resolver and reuse existing endpoint sync path.
3. **ALONG group projection introduces convergence oscillation with ANGLE**  
   Mitigation: deterministic target-coordinate rule + pass-cap diagnostics contract already in place.

### Fallback Strategy
- If full mixed-role ALONG solve cannot be stabilized in one pass, deliver in deterministic slices while preserving legal authoring boundaries:
  1. Standalone point + line endpoint roles first,
  2. Arc endpoint roles next,
  3. Arc center role last.
- Any temporarily unsupported but selectable signature must fail explicitly with deterministic diagnostic and no geometry mutation.

### Out-of-Scope Boundaries
- Arc-line advanced semantic constraints (`ARCI-01..04`) remain Phase 24.
- Broad milestone regression closure (`V13-01`) remains Phase 25.
- Changing solver authority boundary (scene-owned) is out of scope.
- Reworking script grammar for explicit landmark-role participant syntax is out of scope for Phase 23 unless newly mandated.

## Sources

### Primary (HIGH confidence)
- `.planning/phases/23-principal-direction-constraint-expansion/23-CONTEXT.md` — locked phase decisions and boundaries.
- `.planning/ROADMAP.md` — Phase 23 goal, requirements, success criteria.
- `.planning/REQUIREMENTS.md` — AXIS-01..04 and SRLV-04 requirement contracts.
- `src/constraints/constraint_types.h` — current legality behavior (`ALONG*` currently line-only; sub-entity role rejection).
- `src/ecs/ecs_scene.h` — descriptor add path, candidate resolver constraints, transactional solver loop, diagnostics/implication behavior.
- `src/components/constraint_comp.h` — participant descriptor storage model.
- `src/components/endpoints_comp.h` — endpoint/landmark role metadata, includes `CENTER`.
- `src/scripting/sketch_script_apply.h` — script legality validation path through shared legality helper.
- `src/app.c` — descriptor-based authoring callsite in constraint menu.
- `src/tests/scene_solver_contract_test.c` — existing deterministic transactional and dimensional contracts.
- `src/tests/scene_solver_pass_policy_test.c` — current pass/tolerance diagnostics behavior.
- `src/tests/endpoint_pick_test.c` — endpoint role legality tests and existing ALONG illegality baseline for endpoint roles.
- `src/tests/scene_solver_trigger_test.c` + `src/tests/scene_solver_diagnostics_test.c` — Phase 22 determinism/diagnostics guardrails.
- `src/CMakeLists.txt` — active CTest target map.

### Secondary (MEDIUM confidence)
- None.

### Tertiary (LOW confidence)
- None.

## Metadata

**Confidence breakdown:**
- Standard stack: **HIGH** — pure in-repo architecture continuity; no external dependency uncertainty.
- Architecture: **HIGH** — solver and legality extension points are explicit in current code.
- Pitfalls: **HIGH** — derived from existing failure behavior and tested contracts.

**Research date:** 2026-04-08  
**Valid until:** 2026-05-08 (revalidate if solver legality or descriptor schema changes before implementation)
