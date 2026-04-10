# Phase 31: Script Reapply Fidelity Foundation - Research

**Researched:** 2026-04-10  
**Domain:** Script parse/apply/emit fidelity, solver contract parity, deterministic replay  
**Confidence:** HIGH

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
### Participant descriptor contract (roles/sub-index)
- **D-01:** Script re-apply uses a strict descriptor contract: participant role/sub-index intent must be preserved for script-managed constraints.
- **D-02:** Missing/ambiguous descriptor intent is an explicit apply failure, not a silent downgrade to entity-only participants.
- **D-03:** Legacy entity-only participant fallback is not allowed for this phase.

### Color preservation semantics
- **D-04:** Script color is canonical metadata for script-managed entities and must round-trip exactly on re-apply.
- **D-05:** Color preservation applies to script-managed points, lines, and arcs.
- **D-06:** Non-script entities remain untouched by script re-apply color restoration.

### Reapply/remap transaction behavior
- **D-07:** Re-apply remains all-or-nothing: unresolved script id, illegal participant signature, or metadata mismatch triggers full rollback.
- **D-08:** No partial scene mutations are allowed on failure.
- **D-09:** Failures must use deterministic, explicit error taxonomy/messages suitable for debugging and regression assertions.

### Determinism verification contract
- **D-10:** Phase verification must include repeated apply in a single process and parity checks across fresh process reruns.
- **D-11:** Determinism parity checks must include emitted script stability and key solver diagnostics stability for identical inputs.
- **D-12:** Weak "no crash/no error only" verification is not sufficient for this phase.

### Shared capability registry (script + solver parity)
- **D-13:** Script contract validation and solver legality checks must consume a shared capability registry as a single source of truth for supported entity types, constraint types, participant roles, and legal signatures.
- **D-14:** Manual dual-maintenance of separate script/solver allowlists is not acceptable in this phase.
- **D-15:** When script input references unsupported or out-of-contract combinations, errors must be explicit, deterministic, and derived from the shared registry contract.

### the agent's Discretion
- Exact internal representation for descriptor persistence in script parse/apply/emit pipeline, as long as D-01..D-03 behavior is enforced.
- Exact deterministic failure code/message structuring, provided family-level diagnostics remain explicit and stable.
- Exact registry factoring shape (header ownership/module boundaries), as long as D-13..D-15 single-source behavior is enforced.
- Exact test fixture composition and helper wiring, provided D-10..D-15 parity guarantees are covered.

### Deferred Ideas (OUT OF SCOPE)
None — discussion stayed within phase scope.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| SCRI-01 | Re-applying scripts preserves participant role/sub-index semantics; no unsupported-participant failures for valid cases. | Introduce descriptor-aware participant schema through parse/apply/emit, enforce shared registry legality checks, remove entity-only fallback in script path. |
| SCRI-02 | Re-applying scripts preserves entity colors (no default-white reset). | Parse/store/emit script color and use script color at entity creation in apply path for script-managed point/line/arc entities. |
| SCRI-03 | Re-applying same script repeatedly yields deterministic stable outcomes. | Add repeated-apply + rerun parity tests for emitted script and diagnostics; enforce deterministic failure taxonomy/messages and rollback invariants. |
</phase_requirements>

## Project Constraints (from copilot-instructions.md)

Source read: `.github/copilot-instructions.md` (repo-level conventions file).

- Read `CHECKPOINT.md` at session start for continuity.
- Keep project documentation continuity (`README.md` should stay up to date as development continues).
- C-first architecture and conventions (header-heavy modules, `snake_case`, explicit status/error returns).
- Existing workflow expectation: plan artifacts are stored in `.plans/PLAN_*.md` when planning mode is used.
- Preserve lightweight CMake + CTest workflow (no recommendation that adds heavy framework churn for this phase).

## Summary

Phase 31 is blocked by three concrete fidelity gaps in the current script pipeline, all now directly code-anchored. First, parser and emitter currently treat constraint participants as plain ID strings (`participants = {"geometry_1", ...}`), while apply converts every participant to `ROLE_ENTITY` (`sketch_script_apply.h`:181-183). That destroys endpoint/center intent and causes legal scripted constraints to degrade into invalid runtime signatures, surfacing errors like `Unsupported coincident participants...` (`ecs_scene.h`:2967) despite originally valid descriptor intent.

Second, script apply hardcodes white for all recreated script-managed entities (`sketch_script_apply.h`:133,135,140 via `vec4_make(1,1,1,1)`), while parser/emitter currently have no color field handling for entities (`sketch_script_parse.h`:338-557 and `sketch_script_emit.h`:253-295). This is the direct root cause of the reported color reset on reapply (SCRI-02).

Third, script-contract validation is split across separate logic islands: script allowlists in `sketch_script_contract.h` vs solver legality in `constraint_types.h` + `scene_add_constraint_to_sketch_with_descriptors` (`ecs_scene.h`:2548-2647). D-13..D-15 requires single-source capability rules; current dual-maintenance is a drift risk.

**Primary recommendation:** Implement a shared script+solver capability registry, propagate descriptor+color fidelity end-to-end through parse/apply/emit, and gate with deterministic repeat-apply/rerun parity tests mapped to SCRI-01..03.

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| mdCAD script parser/apply/emit headers (`sketch_script_parse.h`, `sketch_script_apply.h`, `sketch_script_emit.h`) | repo-current | Script model parsing, transactional apply, deterministic emission | Existing production path; phase is fidelity hardening, not stack replacement |
| Constraint legality core (`constraint_types.h` + `scene_add_constraint_to_sketch_with_descriptors`) | repo-current | Canonical role/type legality and runtime acceptance | Already authoritative at runtime; must become shared source for script validation too |
| CTest + C test executables (`script_roundtrip_tests`, `scene_solver_*`) | CMake/CTest via repo | Deterministic regression verification | Already integrated in build + milestone closure process |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| Scene serializer (`scene_serializer.h`) | repo-current | Participant descriptor persistence precedent | Mirror descriptor persistence patterns for script model fidelity |
| Undo transaction pipeline (`undo_redo_exec.h`, scene script apply transaction in `ecs_scene.h`) | repo-current | Atomic rollback and deterministic replay | Preserve all-or-nothing semantics and undo integrity during reapply failures |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Shared internal capability registry | Keep script and solver allowlists separate | Rejected by D-13..D-15; high drift risk and nondeterministic contract mismatch |
| Descriptor-preserving schema | Entity-only participant fallback | Rejected by D-01..D-03; reintroduces unsupported participant failures |

**Installation:**  
No new external package required for Phase 31.

**Version verification:**  
Not applicable (no npm package additions for this phase).

## Architecture Patterns

### Recommended Project Structure
```text
src/
├── scripting/
│   ├── sketch_script_capability_registry.h   # NEW shared script+solver contract source
│   ├── sketch_script_parse.h                 # extend participant + color schema
│   ├── sketch_script_apply.h                 # descriptor/color-aware transactional apply
│   └── sketch_script_emit.h                  # emit descriptor + color canonical form
├── constraints/
│   └── constraint_types.h                    # consume shared registry where legal checks are centralized
├── ecs/
│   └── ecs_scene.h                           # scene facade unchanged semantically; keep transaction wrapper
└── tests/
    ├── script_roundtrip_tests.c              # SCRI-focused fidelity/determinism tests
    └── scene_solver_diagnostics_test.c       # deterministic diagnostic parity assertions
```

### Pattern 1: Descriptor-first participant pipeline
**What:** Parse participants into `{id, role, sub_index}` records, validate against shared registry, apply using `constraint_participant_descriptor_t` without role loss, emit same descriptor schema.  
**When to use:** All script-managed constraint participants in parse/preview/commit/emit.
**Example:**
```c
// Source: src/scripting/sketch_script_apply.h:174-187 (current collapse point)
participants[p] = constraint_participant_descriptor_make(
    (uint64_t)participant_entity,
    CONSTRAINT_PARTICIPANT_ROLE_ENTITY, // <-- must be replaced by parsed role/sub_index
    0);
```

### Pattern 2: Script metadata is canonical for script-managed entities
**What:** Entity color must be parsed/stored and applied directly when creating script-managed point/line/arc entities.  
**When to use:** `sketch_script_apply_create_entities(...)` path only; do not touch non-script entities.
**Example:**
```c
// Source: src/scripting/sketch_script_apply.h:133-140 (current forced white)
entity = scene_add_point_to_sketch(scene, sketch, src->point, vec4_make(1, 1, 1, 1), 0.01f);
```

### Pattern 3: Scene façade owns transaction and rollback policy
**What:** Keep all-or-nothing behavior in `scene_script_apply_commit(...)`; script model commit remains side-effect block called by the scene transaction wrapper.  
**When to use:** Any failure in parse/validation/apply/emit/undo recording.
**Example:**
```c
// Source: src/ecs/ecs_scene.h:6035-6079
// capture before script -> apply -> rollback to before_script on any downstream failure
```

### Anti-Patterns to Avoid
- **Entity-only script participants:** Causes legal signature drift and solver rejection for endpoint/center constraints.
- **Hardcoded default-white in script apply:** Violates SCRI-02 and breaks user-authored metadata trust.
- **Dual contract sources (script vs solver):** Produces non-deterministic error surfaces and hidden incompatibilities.

## Proposed File-Level Change Map

- **`src/scripting/sketch_script_capability_registry.h` (new)**
  - Define canonical capability entries (entity types, constraint types, allowed participant role signatures).
  - Expose lookup helpers used by both script contract validation and solver legality adapters.
- **`src/scripting/sketch_script_contract.h`**
  - Remove hardcoded `point/line/arc/circle` and ad-hoc constraint allowlist checks.
  - Delegate to shared registry for type support and participant schema requirements.
- **`src/scripting/sketch_script_parse.h`**
  - Extend constraint participant model from string IDs to descriptor objects (`id`, `role`, `sub_index`).
  - Add parse support for entity `color = {r,g,b,a}` for point/line/arc.
  - Add deterministic parse errors for missing/ambiguous descriptor intent (D-02).
- **`src/scripting/sketch_script_apply.h`**
  - Preserve parsed participant role/sub_index when building `constraint_participant_descriptor_t`.
  - Replace white defaults with parsed script color for script-managed entities.
  - Keep strict rollback behavior and explicit deterministic failure messages (D-07..D-09).
- **`src/scripting/sketch_script_emit.h`**
  - Emit participants in descriptor form (role/sub_index preserved).
  - Emit color for script-managed entities in stable deterministic ordering/format.
- **`src/constraints/constraint_types.h` and/or `src/ecs/ecs_scene.h`**
  - Wire legality checks to shared capability registry (single source for script + solver contract).
- **`src/tests/script_roundtrip_tests.c`**
  - Add SCRI-01 descriptor roundtrip + strict failure taxonomy + SCRI-02 color roundtrip + SCRI-03 repeat-apply determinism tests.
- **`src/tests/scene_solver_diagnostics_test.c`**
  - Add deterministic parity assertions for diagnostics under repeated script reapply conditions.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Constraint legality divergence | Separate script-specific legality engine | Existing runtime legality path (`constraint_type_is_selection_legal`, descriptor validation in `scene_add_constraint_to_sketch_with_descriptors`) behind shared registry | Prevents script/runtime drift and duplicate bug surfaces |
| Transaction semantics | Custom partial rollback logic in parser/apply internals | Existing scene transaction wrapper + rollback-to-before-script (`scene_script_apply_commit`) | Already proven by current atomic tests; safer and deterministic |
| Determinism checking | Manual ad hoc diff workflow | Existing CTest executable suite + explicit parity assertions in tests | Reproducible and CI/planner-friendly |

**Key insight:** The phase is contract unification and fidelity transport, not solver algorithm replacement.

## Common Pitfalls

### Pitfall 1: Parsing descriptors but dropping them at apply
**What goes wrong:** Model carries role info but `apply` still writes `ROLE_ENTITY`, recreating current bug in new syntax clothes.  
**Why it happens:** `sketch_script_apply_create_constraints` currently hardcodes role/sub-index.  
**How to avoid:** Make apply consume descriptor struct directly from parsed constraint model; add test that endpoint-role constraints reapply without downgrade.  
**Warning signs:** Reapply succeeds for simple constraints but fails with endpoint/center families.

### Pitfall 2: Registry exists but script contract still uses legacy allowlists
**What goes wrong:** D-13..D-15 violated silently; legality mismatch remains.  
**Why it happens:** `sketch_script_contract.h` currently has local hardcoded `geometry_type_allowed` and `constraint_type_allowed`.  
**How to avoid:** Replace those functions with registry-backed checks only.  
**Warning signs:** Solver accepts combo that preview rejects (or inverse).

### Pitfall 3: Color parse added but emitter omits color
**What goes wrong:** First reapply may restore color, next emit/reapply cycle drops color again.  
**Why it happens:** `sketch_script_emit_for_sketch` currently emits no color fields for point/line/arc entities.  
**How to avoid:** Require color emission in canonical entity output and assert emit/apply/emit parity.  
**Warning signs:** Colors survive one apply but drift on subsequent reapply cycles.

### Pitfall 4: Determinism checks only assert success/failure, not output parity
**What goes wrong:** Hidden instability in emitted script or diagnostics escapes verification.  
**Why it happens:** Weak gates focus on crash-free behavior only.  
**How to avoid:** Compare emitted script strings and key diagnostic payloads across repeated runs and fresh reruns (D-10..D-12).  
**Warning signs:** Intermittent test pass with non-identical scripts/messages.

## Code Examples

Verified in-repo patterns to reuse:

### Transactional rollback pattern
```c
// Source: src/ecs/ecs_scene.h:6035-6045
bool capture_before = scene_script_emit_for_sketch(scene, sketch, before_script, sizeof(before_script), out_error);
if (!capture_before) return false;
if (!sketch_script_apply_commit_model(scene, sketch, script_text, out_error)) return false;
if (!sketch_script_parse_model(script_text, &model, out_error)) {
    (void)sketch_script_apply_commit_model(scene, sketch, before_script, &rollback_error);
    return false;
}
```

### Runtime legality gateway
```c
// Source: src/ecs/ecs_scene.h:2645-2647
if (participant_count < constraint_type_min_participants(type)) return 0;
if (!constraint_type_is_selection_legal(&signature, type)) return 0;
```

### Current drift anchor (must change)
```c
// Source: src/scripting/sketch_script_apply.h:181-183
participants[p] = constraint_participant_descriptor_make(
    (uint64_t)participant_entity,
    CONSTRAINT_PARTICIPANT_ROLE_ENTITY,
    0);
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Script participants as ID strings only | Runtime supports descriptor roles/sub-index (`constraint_participant_descriptor_t`) | Introduced before v1.5 (present in current `constraint_comp.h` and solver tests) | Script pipeline is now behind runtime capability and must catch up |
| Scene serializer persisted only participant entity IDs | Scene serializer persists `participant_descriptors` with role/sub_index (`scene_serializer.h`:523-535,1725-1805) | Already implemented | Proven persistence pattern exists; script pipeline should mirror |

**Deprecated/outdated:**
- Script-side local allowlists in `sketch_script_contract.h` for supported types (outdated under D-13..D-15).

## Open Questions

1. **Color “exact roundtrip” precision definition (textual vs float-equivalent)**
   - What we know: D-04 requires exact roundtrip; current emitter formats numbers to 6 decimals.
   - What's unclear: Whether exact means byte-for-byte script text parity or semantic RGBA equality after parse/apply/emit.
   - Recommendation: Lock expectation in first SCRI-02 test (prefer semantic RGBA exact in component values + deterministic emitted canonical format).

2. **Descriptor script syntax shape**
   - What we know: Existing parser accepts only string participant IDs.
   - What's unclear: Final canonical textual form for participants with role/sub-index.
   - Recommendation: Choose one canonical object form and enforce via emitter (single canonical output to avoid syntactic variance).

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| cmake | build/test invocation | ✓ | 4.3.1 | — |
| ctest | deterministic test gates | ✓ | 4.3.1 | run test binaries directly (less ideal) |
| `build-vulkan` directory/artifacts | project’s standard deterministic closure path | ✓ | present | local rebuild if stale |

**Missing dependencies with no fallback:**
- None detected for research/planning scope.

**Missing dependencies with fallback:**
- None.

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | CTest + C executable tests (repo-native) |
| Config file | `CMakeLists.txt`, `src/CMakeLists.txt` |
| Quick run command | `ctest --test-dir build-vulkan -C Release -R "script_roundtrip_tests|scene_solver_diagnostics" --output-on-failure` |
| Full suite command | `ctest --test-dir build-vulkan -C Release -R "scene_solver_contract|scene_solver_pass_policy|scene_solver_diagnostics|scene_solver_trigger|scene_solver_drag|endpoint_pick|script_roundtrip_tests" --output-on-failure` |

### Phase Requirements → Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| SCRI-01 | Descriptor role/sub-index preserved; no entity fallback | unit/integration | `ctest --test-dir build-vulkan -C Release -R "script_roundtrip_tests" --output-on-failure` | ✅ |
| SCRI-02 | Script-managed point/line/arc color roundtrip preserved | unit/integration | `ctest --test-dir build-vulkan -C Release -R "script_roundtrip_tests" --output-on-failure` | ✅ |
| SCRI-03 | Repeat apply + rerun parity for emitted script + diagnostics | integration | `ctest --test-dir build-vulkan -C Release -R "script_roundtrip_tests|scene_solver_diagnostics" --output-on-failure` | ✅ |

### Sampling Rate
- **Per task commit:** `ctest --test-dir build-vulkan -C Release -R "script_roundtrip_tests|scene_solver_diagnostics" --output-on-failure`
- **Per wave merge:** `ctest --test-dir build-vulkan -C Release -R "scene_solver_contract|scene_solver_pass_policy|scene_solver_diagnostics|scene_solver_trigger|scene_solver_drag|endpoint_pick|script_roundtrip_tests" --output-on-failure`
- **Phase gate:** Full suite green, then immediate rerun of identical command for parity evidence.

### Wave 0 Gaps
- [ ] Add descriptor-schema roundtrip tests in `src/tests/script_roundtrip_tests.c` (role/sub-index emit/apply parity).
- [ ] Add script color canonical roundtrip tests in `src/tests/script_roundtrip_tests.c` for point/line/arc.
- [ ] Add deterministic repeat-apply and fresh-rerun parity assertions (script string + key diagnostics stability).

## Acceptance Probes (planner-ready)

1. **Descriptor fidelity probe (SCRI-01)**
   - Apply script containing endpoint/center participant descriptors for ARCI families.
   - Re-emit + reapply emitted script.
   - Assert: constraint participant descriptors (entity/role/sub_index) are unchanged across cycle and solve does not fail with unsupported-participant taxonomy.

2. **Strict failure taxonomy probe (D-02/D-09)**
   - Apply script missing required participant descriptor intent for a descriptor-required constraint.
   - Assert: apply fails atomically with deterministic explicit contract error code/message; no scene mutation.

3. **Color canonicality probe (SCRI-02)**
   - Apply script with non-white colors on point/line/arc.
   - Reapply same script multiple times.
   - Assert: geometry component `color` remains exact expected values for script-managed entities; non-script entities unchanged.

4. **Repeat-apply determinism probe (SCRI-03)**
   - In one process: apply same script N times (e.g., 5).
   - Assert: emitted script text identical each iteration; key diagnostics stream/count/messages stable.

5. **Fresh rerun parity probe (D-10..D-12)**
   - Run deterministic CTest gate command twice back-to-back in fresh invocations.
   - Assert: pass/pass and no differences in targeted test outputs.

## Sources

### Primary (HIGH confidence)
- `src/scripting/sketch_script_apply.h` — current participant-role collapse and white-color reset points.
- `src/scripting/sketch_script_parse.h` — current script model schema limitations (no descriptor/color fields).
- `src/scripting/sketch_script_emit.h` — current emission schema limitations (no descriptor/color emission).
- `src/ecs/ecs_scene.h` — runtime legality gateway, solver failure taxonomy anchor, script transaction wrapper.
- `src/constraints/constraint_types.h` — canonical solver-side signature legality and participant role semantics.
- `src/scene_serializer.h` — participant descriptor persistence pattern already implemented for scene JSON.
- `src/tests/script_roundtrip_tests.c`, `src/tests/scene_solver_diagnostics_test.c`, `src/tests/scene_solver_contract_test.c` — existing deterministic/transactional test baseline.
- `.planning/phases/31-script-reapply-fidelity-foundation/31-CONTEXT.md` — locked decisions D-01..D-15.
- `.planning/REQUIREMENTS.md`, `.planning/ROADMAP.md`, `.planning/STATE.md` — requirement mapping and phase scope.
- `docs/improvements/solver-user-workflow-robustness.md` — user-observed repro/failure motivation.

### Secondary (MEDIUM confidence)
- None (no external-doc claim required for this repo-local phase).

### Tertiary (LOW confidence)
- None.

## Metadata

**Confidence breakdown:**
- Standard stack: **HIGH** — entirely repo-native and already in use.
- Architecture: **HIGH** — drift points are directly code-anchored with explicit call sites.
- Pitfalls: **HIGH** — derived from current implementation plus existing failing user scenario.

**Research date:** 2026-04-10  
**Valid until:** 2026-05-10

