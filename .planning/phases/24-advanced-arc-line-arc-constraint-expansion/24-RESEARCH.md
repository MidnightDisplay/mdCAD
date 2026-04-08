# Phase 24: Advanced Arc + Line-Arc Constraint Expansion - Research

**Researched:** 2026-04-08  
**Domain:** mdCAD sketch constraint legality + transactional solver expansion (arc/line-arc families)  
**Confidence:** HIGH

## User Constraints (from CONTEXT.md)

### Locked Decisions
### Arc-center axis vs line (ARCI-01)
- **D-01:** Legal authoring shape is exactly one line entity + one arc entity (entity roles only; no sub-entity endpoint roles for this constraint type).
- **D-02:** Solve contract is axis-based: arc normal axis must be parallel or anti-parallel to the selected line direction.
- **D-03:** When both participants are editable, the arc is the primary moving/reorienting geometry and the line is treated as reference.
- **D-04:** If fixed-state constraints make the relation unsatisfiable, solver fails transactionally (no geometry mutation), emits explicit diagnostic, and sets implication highlighting.

### Line-end to arc-end tangency (ARCI-02)
- **D-05:** Legal authoring shape is exactly two endpoint participants: one line endpoint role and one arc endpoint role.
- **D-06:** Solve order is deterministic: enforce endpoint coincidence first (shared point), then enforce tangency at that shared point.
- **D-07:** Fixed/free policy: fixed participant anchors; free participant absorbs required solve delta.
- **D-08:** If both sides are fixed and unsatisfied, solver fails transactionally with explicit tangency diagnostic (no silent downgrade/fallback).

### Arc endpoint-angle constraint (ARCI-03)
- **D-09:** Authoring is endpoint-pair on the same arc (not single-arc entity mode). Selection order is semantic: first selected endpoint is the anchored reference.
- **D-10:** Value domain is normalized sweep magnitude in `[0, π]` (UI edits in degrees, solver stores/evaluates radians).
- **D-11:** Solve behavior honors anchored-reference contract: keep first-selected endpoint fixed as reference and move the second endpoint role to satisfy target angle.

### Diagnostics and deterministic recalculate (ARCI-04)
- **D-12:** Keep existing solver failure policy unchanged for ARCI constraints: transactional rollback on unsatisfied solve, explicit diagnostics row, implication highlighting, deterministic repeated recalculate.
- **D-13:** Add dedicated explicit unsatisfied messages per new family (arc-axis relation, line-arc endpoint tangency, arc endpoint-angle) instead of generic catch-all text.

### the agent's Discretion
- Exact low-level solve math implementation (projection/rotation formulation) as long as D-02/D-06/D-11 contracts and deterministic behavior hold.
- Exact diagnostic wording text, provided each ARCI family remains explicitly distinguishable.
- Exact fixture split across existing solver test binaries.

### Deferred Ideas (OUT OF SCOPE)
None — discussion stayed within phase scope.

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| ARCI-01 | User can constrain an arc-center axis relative to a line so the arc orientation follows the perpendicular-to-line authoring contract. | Descriptor legality extension in `constraint_type_is_selection_legal`, menu filtering via `mdcad_collect_constraint_context`, solver branch in `scene_solver_request_recalculate` using arc normal/line direction and transactional failure path. |
| ARCI-02 | User can constrain line-end and arc-end tangency at a shared point, with solver preserving tangency by moving geometric positions as needed. | Existing endpoint-role plumbing (`EndPointsComp`, descriptor roles, endpoint pick mapping) enables exact endpoint participants; implement deterministic coincidence-then-tangent solve branch with fixed/free policy and explicit unsat diagnostics. |
| ARCI-03 | User can apply and edit an angle constraint between a single arc's start and end points. | Existing dimensional pipeline already handles degree↔radian conversion (`app.c`, `ui_entity_inspector.h`); add same-arc endpoint-pair legality and solver branch anchoring first-selected endpoint, clamping value to `[0,π]`. |
| ARCI-04 | Advanced arc/line-arc constraints solve deterministically and surface explicit diagnostics when not satisfiable. | Existing transactional pass-loop, diagnostic ring, and failure implication APIs in `ecs_scene.h` are reusable; add ARCI-specific failure reasons and deterministic recalc/idempotence tests in existing solver test targets. |

</phase_requirements>

## Summary

Phase 24 should be implemented as an extension of the existing descriptor-based constraint architecture, not as a new subsystem. The core enabling patterns already exist: participant roles (`ENTITY`, `POINT_A`, `POINT_B`, `CENTER`), sketch-scoped endpoint entities, transactional candidate solving, deterministic pass-bounded recalculate, and explicit diagnostics/implication payloads.

The biggest planning risk is not math complexity alone; it is preserving current solver contracts while introducing new constraint families. In this codebase, legality must remain centralized (`constraint_type_is_selection_legal`), authoring must be signature-gated from selection context (`mdcad_collect_constraint_context` + constraint menu), and solver failures must remain transactional with explicit family-specific diagnostic messages.

**Primary recommendation:** implement ARCI-01/02/03 as new typed branches inside existing legality + `scene_solver_request_recalculate` flow, backed by Wave-0 tests that lock participant-shape legality, fixed/free policy, deterministic recalc, and family-specific diagnostics.

## Project Constraints (from copilot-instructions.md)

`copilot-instructions.md` not found at repository root; no additional file-specific directives extracted.

## Standard Stack

### Core
| Library/Module | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| `src/constraints/constraint_types.h` | repo current | Constraint type/legality gate | Single source of truth for interactive + script legality; already used by scene/script paths. |
| `src/ecs/ecs_scene.h` | repo current | Constraint creation + solver + diagnostics | Owns transactional recalc and implication behavior; required by Phase 24 contracts. |
| `constraint_participant_descriptor_t` (`constraint_comp.h`) | repo current | Participant role encoding | Existing mechanism for endpoint/center semantics without ad-hoc side tables. |
| `EndPointsComp` (`endpoints_comp.h`) | repo current | Native endpoint ownership mapping | Enables line/arc endpoint selection and role-preserving solver participants. |

### Supporting
| Library/Module | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| `src/app.c` authoring flow | repo current | Selection → descriptor signature and context menu | When exposing ARCI options only for legal signatures. |
| `src/ui/ui_entity_inspector.h` | repo current | Dimensional edit UI (deg/rad, decimals) | For ARCI-03 value editing/display parity. |
| `src/scripting/sketch_script_apply.h` | repo current | Script-time legality validation | When extending legality so script apply and UI behave identically. |
| solver test binaries (`scene_solver_contract`, `scene_solver_pass_policy`, `endpoint_pick`) | repo current | Regression contracts | For deterministic behavior/failure policy lock-in. |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Descriptor-based participants | Separate ARCI-specific participant structs | Would fork architecture and break existing legality + script reuse patterns. |
| Reusing existing pass loop | Dedicated ARCI mini-solver | Higher risk of nondeterministic divergence and duplicated rollback/diagnostic logic. |

**Installation:** N/A (existing in-repo C modules; no new package dependency identified for Phase 24 planning).

## Architecture Patterns

### Recommended Project Structure
```text
src/
├── constraints/constraint_types.h      # legality signatures + min participants + display names
├── ecs/ecs_scene.h                     # constraint creation + recalculate solve branches + diagnostics
├── app.c                               # selection→descriptor context and C-menu authoring path
├── ui/ui_entity_inspector.h            # dimensional editing and constraint manager behaviors
└── tests/                              # executable solver/legality/pick contract tests
```

### Pattern 1: Centralized legality gate (UI + script parity)
**What:** All ARCI participant-shape rules must be encoded in `constraint_type_is_selection_legal`.  
**When to use:** Every new constraint family/signature.  
**Example (existing pattern):**
```c
if (!constraint_type_is_selection_legal(&signature, type)) return 0;
```
Source: `src/ecs/ecs_scene.h` (constraint creation path), `src/scripting/sketch_script_apply.h` (script validation path)

### Pattern 2: Descriptor-first constraint authoring
**What:** Always create constraints with descriptor roles, not raw entity arrays, for endpoint/center semantics.  
**When to use:** ARCI-02 and ARCI-03 require ordered endpoint roles; ARCI-01 requires entity roles only.  
**Example:**
```c
ecs_entity_t created = scene_add_constraint_to_sketch_with_descriptors(
    &state.ecs_scene, state.constraint_menu_sketch, type,
    state.constraint_menu_participants, state.constraint_menu_participant_count,
    initial_value, false);
```
Source: `src/app.c` (constraint menu apply)

### Pattern 3: Transactional pass-loop solving with explicit unsat reason
**What:** Extend `scene_solver_request_recalculate` branches; never mutate live geometry until pass-loop success.  
**When to use:** ARCI solve implementation and unsat diagnostics.  
**Example (existing behavior contract):**
```c
if (solve_failed) {
    failure_reason = "Unsatisfied driving ANGLE constraint.";
    implicated_constraints[implicated_constraint_count++] = child;
}
```
Source: `src/ecs/ecs_scene.h` (current ANGLE/ALONG patterns)

### Anti-Patterns to Avoid
- **Bypassing legality in UI only:** breaks script apply parity and creates hidden invalid states.
- **Adding non-transactional geometry writes in solver:** violates D-12 and existing contract tests.
- **Generic “constraint failed” messages:** violates D-13 explicit family diagnostics.
- **Ignoring participant ordering for endpoint-angle:** violates D-09/D-11 anchored-reference semantics.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Endpoint role resolution | Custom pick-id parsing for ARCI only | Existing endpoint entity + descriptor role path (`EndPointsComp`, `mdcad_collect_constraint_context`) | Already sketch-scoped and regression-tested; avoids mismatch bugs. |
| Failure highlighting | New ad-hoc highlight path | `scene_solver_set_failure_implication` + diagnostic ring | Existing deterministic implication sorting and participant dedupe. |
| Dimensional unit conversion | New ARCI angle UI converter | Existing degree/radian helpers in `app.c` and inspector helpers | Keeps editing behavior consistent with existing ANGLE constraints. |
| Participant legality checks | Inline checks in menu/solver | `constraint_type_is_selection_legal` | Single canonical gate used by UI and scripting. |

**Key insight:** Phase 24 is an incremental extension problem; custom side-channels would increase correctness risk more than they reduce implementation effort.

## Common Pitfalls

### Pitfall 1: Signature drift between app menu and script apply
**What goes wrong:** ARCI constraints can be created from UI but rejected by script apply (or vice versa).  
**Why it happens:** Legality encoded outside `constraint_type_is_selection_legal`.  
**How to avoid:** Add ARCI signatures only in legality helper and ensure both paths consume it.  
**Warning signs:** UI creates constraint but script import errors “Constraint participants are not legal for type.”

### Pitfall 2: Endpoint-order semantics lost for ARCI-03
**What goes wrong:** Arc endpoint-angle solve moves wrong endpoint, causing nondeterministic edits.  
**Why it happens:** Participant order not preserved from selection descriptors.  
**How to avoid:** Preserve descriptor order from `mdcad_collect_constraint_context`; implement anchored-first solve branch.  
**Warning signs:** Recalculate toggles between equivalent arc states across runs.

### Pitfall 3: Unsat ARCI silently mutates geometry before failing
**What goes wrong:** Partial geometry changes remain after failure.  
**Why it happens:** Writing directly to geometry instead of candidate staging/commit model.  
**How to avoid:** Keep all ARCI math in candidate loop and rely on existing transactional commit boundary.  
**Warning signs:** fixed/fixed unsat case changes endpoint/arc center values.

### Pitfall 4: Ambiguous diagnostics
**What goes wrong:** Users see generic failure text and cannot identify failing family.  
**Why it happens:** Reusing generic fallback messages.  
**How to avoid:** Add dedicated strings per ARCI family per D-13.  
**Warning signs:** diagnostic message equals generic “Constraint type not yet solved…” or unrelated family text.

## Code Examples

### Selection context preserves role + order (required for ARCI-02/03)
```c
out_participants[*out_participant_count] =
    constraint_participant_descriptor_make((uint64_t)participant_entity,
                                           (uint8_t)participant_role,
                                           participant_sub_index);
out_sig->geometry_types[*out_participant_count] = g->type;
out_sig->roles[*out_participant_count] = participant_role;
(*out_participant_count)++;
```
Source: `src/app.c` (`mdcad_collect_constraint_context`)

### Constraint creation validates descriptors against centralized legality
```c
if (!constraint_type_is_selection_legal(&signature, type)) return 0;
```
Source: `src/ecs/ecs_scene.h` (`scene_add_constraint_to_sketch_with_descriptors`)

### Dimensional editing already supports ANGLE in degrees with solver radians
```c
float value_for_solver = is_angle_dimension
    ? mdcad_deg_to_rad(state.constraint_dimension_popup_value)
    : state.constraint_dimension_popup_value;
scene_constraint_set_dimensional_value(&state.ecs_scene,
    state.constraint_dimension_popup_constraint, value_for_solver, constraint->driven);
```
Source: `src/app.c` (`mdcad_draw_constraint_dimension_popup`)

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Entity-only participant assumptions | Descriptor roles (`ENTITY`, `POINT_A`, `POINT_B`, `CENTER`) | v1.2-v1.3 evolution | Enables endpoint/center-aware legality and solving. |
| Non-specific or sparse solver diagnostics | Explicit per-family unsat reasons + implication payload | Phase 22/23 baseline | Supports ARCI-04 explicit diagnostics requirement. |
| Single-constraint simplistic solve paths | Pass-bounded deterministic recalculate with tolerance + max passes | Phase 22 baseline | ARCI can integrate without new solve engine. |

**Deprecated/outdated for this phase:**
- Raw participant arrays for endpoint-sensitive constraints as primary authoring path.
- Any design that bypasses transactional recalculate and writes live geometry mid-pass.

## Open Questions

1. **ARCI-01 geometric interpretation details (perpendicular authoring vs axis parallelism)**
   - What we know: D-02 locks solve relation to arc normal axis parallel/anti-parallel to line direction.
   - What's unclear: exact UI wording and whether any legacy “perpendicular” label must be retained.
   - Recommendation: lock user-facing label in planning, but keep solver math per D-02.

2. **ARCI-03 same-arc endpoint legality edge cases**
   - What we know: Must be endpoint-pair on same arc with selection-order semantics.
   - What's unclear: whether mixed endpoint+center selection should be silently filtered or explicitly warned.
   - Recommendation: reject in legality (menu won’t show), add targeted legality tests for clarity.

3. **ARCI diagnostics copy text**
   - What we know: family-specific explicit messages required (D-13).
   - What's unclear: final wording standard for future localization/consistency.
   - Recommendation: define canonical string constants in planning tasks and lock them via tests.

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| cmake | Configure/build solver test binaries | ✓ | 4.3.0 | — |
| ctest | Running solver regression targets | ✓ | 4.3.0 | — |
| C compiler toolchain (`cl`/MSBuild/Ninja) | Building updated tests/targets | ✗ (not detected in current shell) | — | Use existing preconfigured IDE shell/CI runner |
| node | GSD tooling + phase init scripts | ✓ | v25.9.0 | — |
| python | auxiliary scripting | ✓ | 3.13.12 | — |

**Missing dependencies with no fallback:**
- Local compile/test execution in this shell is blocked until a compiler toolchain shell is used.

**Missing dependencies with fallback:**
- Build/test can run in CI or a Developer Command Prompt with MSVC configured.

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | Native C executable tests wired through CTest |
| Config file | `src/CMakeLists.txt` test target registration |
| Quick run command | `ctest -R "scene_solver_contract|endpoint_pick" --output-on-failure` |
| Full suite command | `ctest --output-on-failure` |

### Phase Requirements → Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| ARCI-01 | Arc-line axis contract legality + deterministic solve | unit/integration | `ctest -R scene_solver_contract --output-on-failure` | ✅ (extend existing file) |
| ARCI-02 | Line-end/arc-end tangency with deterministic coincidence-first policy | unit/integration | `ctest -R "scene_solver_contract|endpoint_pick" --output-on-failure` | ✅ (extend existing files) |
| ARCI-03 | Same-arc endpoint angle apply/edit (deg UI, rad solver, anchored order) | unit/integration | `ctest -R scene_solver_contract --output-on-failure` | ✅ (extend existing file) |
| ARCI-04 | Deterministic unsat failure with explicit family diagnostics | unit/integration | `ctest -R "scene_solver_contract|scene_solver_pass_policy|scene_solver_diagnostics" --output-on-failure` | ✅ (extend existing files) |

### Sampling Rate
- **Per task commit:** `ctest -R "scene_solver_contract|endpoint_pick" --output-on-failure`
- **Per wave merge:** `ctest -R "scene_solver_contract|scene_solver_pass_policy|scene_solver_diagnostics|endpoint_pick" --output-on-failure`
- **Phase gate:** Full suite green before `/gsd-verify-work`

### Wave 0 Gaps
- [ ] Add ARCI legality tests in `src/tests/endpoint_pick_test.c` (or new focused legality executable) for required participant shapes.
- [ ] Add ARCI solver branch contract tests in `src/tests/scene_solver_contract_test.c` (success + fixed/fixed unsat transactional rollback).
- [ ] Add ARCI explicit unsat message assertions in `src/tests/scene_solver_pass_policy_test.c` or `scene_solver_diagnostics_test.c`.

## Sources

### Primary (HIGH confidence)
- `src/constraints/constraint_types.h` — current legality model, participant role semantics, dimensional constraints.
- `src/components/constraint_comp.h` — `constraint_type_t`, descriptor storage model.
- `src/components/endpoints_comp.h` — endpoint role ownership/binding (`POINT_A`, `POINT_B`, `CENTER`).
- `src/app.c` — selection-to-descriptor flow and dimensional popup conversion behavior.
- `src/ecs/ecs_scene.h` — descriptor validation, transactional recalc loop, diagnostics and implication APIs.
- `src/ui/ui_entity_inspector.h` — ConstraintManager editing path and angle conversion helpers.
- `src/scripting/sketch_script_apply.h` — script-time legality coupling.
- `src/tests/scene_solver_contract_test.c`, `src/tests/scene_solver_pass_policy_test.c`, `src/tests/endpoint_pick_test.c` — regression/test architecture baseline.
- `.planning/phases/24-advanced-arc-line-arc-constraint-expansion/24-CONTEXT.md` — locked decisions D-01..D-13.
- `.planning/REQUIREMENTS.md`, `.planning/ROADMAP.md`, `.planning/STATE.md` — requirement traceability and active milestone scope.

### Secondary (MEDIUM confidence)
- `AGENTS.md` — repo-level workflow conventions and platform/build notes (used for planner alignment).

### Tertiary (LOW confidence)
- None.

## Metadata

**Confidence breakdown:**
- Standard stack: **HIGH** — derived directly from current repository implementation seams.
- Architecture: **HIGH** — validated by existing solver, legality, and authoring flows in code.
- Pitfalls: **HIGH** — inferred from explicit existing contracts/tests and known failure paths.

**Research date:** 2026-04-08  
**Valid until:** 2026-05-08 (stable internal architecture; re-check if solver core refactors)

