# Phase 13: Script Round-Trip Baseline - Research

**Researched:** 2026-04-01  
**Domain:** Sketch scripting round-trip architecture (Lua-backed declarative reconstruction + deterministic emission)  
**Confidence:** MEDIUM

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
### Script contract and scope
- **D-01:** Script format is a **declarative Lua table model** (not imperative command stream) as the Phase 13 canonical contract.
- **D-02:** Phase 13 parser/reconstructor supports **exact current v1.2 sketch scope only**: points, lines, arcs/circles, and current constraint set.
- **D-03:** Lua runtime baseline is locked to **Lua 5.4.x** for Phase 13 deliverables and verification evidence.

### Editor interaction model
- **D-04:** Script editor is a standalone ImGui window opened from SketchManager (not embedded in Entity Inspector body).
- **D-05:** Editing flow uses **both** auto-apply preview and explicit Apply commit.
- **D-06:** Auto-apply preview parse/validation failure must show diagnostics while preserving last committed valid scene state.
- **D-07:** Explicit Apply commit is **atomic all-or-nothing**: commit only when script is fully valid and all references resolve.

### Round-trip identity and reconstruction
- **D-08:** Script entities/constraints use **script-local stable IDs** persisted via components/serialization, not labels and not raw ECS IDs.
- **D-09:** Script reconstruction uses a **two-pass link strategy**: create all entities first, then resolve all references/participants atomically.

### Deterministic UI->script emission
- **D-10:** Deterministic output ordering is stable sort by script-local IDs within type groups.
- **D-11:** Numeric output format is fixed-decimal with trimming and no scientific notation.

### the agent's Discretion
- Exact Lua embedding wiring and allocation boundaries in C, as long as API/runtime behavior is Lua 5.4.x-compliant.
- Exact editor panel layout details (button placement, diagnostics widget composition) while preserving current ImGui conventions.
- Exact internal storage schema for script-local IDs, provided IDs remain stable across save/load and round-trip edits.
- Exact deterministic grouping order precedence among type groups, provided ordering is documented and stable.

### Deferred Ideas (OUT OF SCOPE)
- Script-side safe recovery/last-valid-state guarantees during full scene apply lifecycle are Phase 14 scope.
- Dynamic script IO controls (`min/max/step`, generated sliders/readouts) are Phase 14 scope.
- Cross-platform scripting parity evidence for macOS/web/iOS validation is Phase 15+ scope.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| SCRP-01 | User can open a standalone sketch script editor window from SketchManager. | Existing SketchManager + standalone workspace window pattern supports adding a second standalone script window trigger/path. |
| SCRP-02 | User can reconstruct the full sketch sub-scene (entities, constraints, values, and links) from script parse output. | Existing scene loader already uses first-pass create + second-pass parent/link resolution; same strategy should be mirrored for script apply. |
| SCRP-03 | UI-side sketch/geometry/constraint edits update script output deterministically. | Centralized scene mutation APIs and existing metadata refresh points can trigger canonical script re-emit after each mutation. |
| SCRP-06 | v1.2 scripting runtime is Lua 5.4.x. | Local environment has Lua 5.4.8 available; Phase must lock runtime behavior to Lua 5.4 semantics and include explicit runtime/version check in verification. |
</phase_requirements>

## Summary

Phase 13 should be planned as a **scene-layer scripting baseline**, not a UI-only feature. The UI already has a strong pattern for standalone sketch-focused windows (`Active Sketch Workspace`) and SketchManager controls, so SCRP-01 is mostly an integration task: add script window launch controls in SketchManager and maintain window lifecycle in app/UI state.

For SCRP-02/SCRP-03, the strongest existing architecture anchor is `scene_serializer.h` + `ecs_scene.h`: scene load already demonstrates a two-pass reconstruction model (create entities first, resolve links afterward), while scene mutation helpers centralize geometry/constraint changes and metadata/solver refresh behavior. Reuse this approach for script parse/apply and deterministic emit to avoid fragmented logic in UI paths.

Critical planning risk is identity and determinism: current persisted structures use ECS IDs and `%g` float formatting, both non-canonical for script round-tripping. Phase 13 must introduce script-local stable IDs plus a canonical formatter/order policy, then route all relevant UI-driven mutations through one “emit canonical script” path so output stays stable.

**Primary recommendation:** Implement a dedicated scripting module at scene layer (`parse -> validate -> preview/apply -> canonical emit`) and keep UI as thin orchestration only.

## Project Constraints (from copilot-instructions.md)

`./copilot-instructions.md` not found in repository root at research time; no additional project-specific directives extracted.

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| Lua (PUC-Rio runtime) | 5.4.x (local: 5.4.8) | Script runtime baseline for v1.2 sketch scripts | Locked by D-03/SCRP-06; lightweight embeddable C runtime aligned with proposal constraints |
| Dear ImGui via cimgui | Existing project dependency | Standalone script editor window UI | Already used throughout inspector/workspace flows; lowest-risk integration path |
| flecs + project scene APIs | Existing project dependency | Authoritative sketch/geometry/constraint graph mutation | Existing mutation contracts already preserve metadata/solver semantics |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| Existing `scene_serializer.h` patterns | In-repo | Two-pass reconstruction and participant remap precedent | Use as blueprint for script reconstruction architecture |
| Existing constraint formatting helpers | In-repo | Decimal precision normalization for dimensional values | Use in canonical script numeric formatting path |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Declarative Lua table contract | Imperative command script stream | Rejected by locked decision D-01 |
| Script-local stable IDs | Labels or ECS IDs | Rejected by D-08; labels collide, ECS IDs unstable across sessions |

**Installation:**
```bash
# Runtime check used for phase baseline evidence
lua -v
```

**Version verification:**  
Environment probe shows `lua -v` => `Lua 5.4.8` (2026-04-01 research session).  

## Architecture Patterns

### Recommended Project Structure
```text
src/
├── scripting/
│   ├── sketch_script_contract.h      # Lua table schema + validation errors
│   ├── sketch_script_parse.h         # Lua parse -> intermediate model
│   ├── sketch_script_apply.h         # preview/apply, two-pass reconstruction
│   └── sketch_script_emit.h          # deterministic canonical emission
├── ecs/
│   └── ecs_scene.h                   # façade entrypoints called by UI/app
└── ui/
    └── ui_entity_inspector.h         # SketchManager launch + script window
```

### Pattern 1: Thin UI, scene-owned mutations
**What:** UI triggers operations; scene/scripting layer performs mutation and consistency updates.  
**When to use:** Any script parse preview, apply commit, or UI-driven edit that must re-emit script.  
**Example:**
```c
// Source: src/ui/ui_entity_inspector.h + src/ecs/ecs_scene.h
if (igInputFloat(value_id, &edit_value, 0.1f, 1.0f, value_fmt, ImGuiInputTextFlags_CharsDecimal)) {
    scene_constraint_set_dimensional_value(scene, c_e, edit_value, constraint->driven);
}
```

### Pattern 2: Two-pass reconstruction for links
**What:** First create entities, second resolve parent/participant references after all IDs are known.  
**When to use:** Script parse/apply for constraints referencing geometry by stable IDs.  
**Example:**
```c
// Source: src/scene_serializer.h
// pass 1: create entities
entities[i].new_entity = scene_create_from_loaded(scene, &entities[i]);
// pass 2: resolve parents and participants after creation
scene_set_parents_batch(scene, load_children, load_parents, parent_count);
```

### Anti-Patterns to Avoid
- **Script logic in UI widgets:** causes drift between editor behavior and API behavior.
- **Single-pass reference resolution:** breaks forward references and all-or-nothing constraints.
- **Using ECS IDs as script identity:** non-deterministic across load/session boundaries.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Constraint legality validation | Ad-hoc script parser-only checks | Existing `constraint_type_*` legality + scene helpers | Prevents mismatch with current UI/solver legality rules |
| Parent/participant graph linking | One-off custom graph mutator in UI | Scene two-pass apply with participant back-reference rebuild pattern | Already proven for scene load, lower bug risk |
| Numeric canonicalization | New float formatting rules in multiple places | Single shared canonical emitter helper | Determinism requires one formatter and one policy |

**Key insight:** Deterministic round-trip fails mainly from duplicated mutation/formatting logic; centralization is the non-negotiable design choice.

## Common Pitfalls

### Pitfall 1: “Preview” accidentally mutates committed state
**What goes wrong:** parse failures leave partially changed sketch state.  
**Why it happens:** preview path shares direct mutators with commit path without transaction boundary.  
**How to avoid:** keep preview in isolated intermediate model or temp scene branch; commit only on full validation.  
**Warning signs:** constraint counts/participants change after syntax error.

### Pitfall 2: Non-deterministic output from unstable iteration order
**What goes wrong:** scripts differ across identical edits/saves.  
**Why it happens:** ECS query/child traversal order used directly for emit.  
**How to avoid:** explicit type-group ordering + stable sort by script-local IDs (D-10).  
**Warning signs:** no-op UI actions still alter emitted script diff.

### Pitfall 3: Float representation churn
**What goes wrong:** value text oscillates (`1`, `1.0`, `0.9999999`, scientific notation).  
**Why it happens:** mixed `%g`/`%f` formatting and inconsistent precision inference.  
**How to avoid:** one fixed-decimal trim formatter with scientific notation disabled (D-11).  
**Warning signs:** repeated parse/emit cycles change only number text.

## Code Examples

Verified patterns from current codebase:

### Standalone sketch-focused window orchestration
```c
// Source: src/ui/ui_entity_inspector.h
if (state->active_sketch != 0 && state->active_sketch_workspace_open) {
    if (igBegin("Active Sketch Workspace", &open, ImGuiWindowFlags_None)) {
        // draw sketch-focused managers
    }
    igEnd();
}
```

### Two-pass participant remap precedent
```c
// Source: src/scene_serializer.h
for (uint32_t p_idx = 0; p_idx < constraint->participant_count; p_idx++) {
    ecs_entity_t participant = scene_find_new_entity(entities, entity_count, constraint->participants[p_idx]);
    constraint->participants[p_idx] = (uint64_t)participant;
}
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Scene persistence around raw ECS IDs and `%g` emission | Deterministic script contract with stable script IDs + canonical formatting (required for this phase) | Phase 13 target | Enables reproducible round-trip diffs and stable reconstruction |

**Deprecated/outdated:**
- Script identity via labels/raw ECS IDs for round-trip linkage in Phase 13 scope.

## Open Questions

1. **Where should script-local IDs live?**
   - What we know: must persist via components/serialization (D-08).
   - What's unclear: whether IDs belong in new dedicated components vs extension fields on existing sketch/constraint/geometry components.
   - Recommendation: pick dedicated lightweight components to minimize coupling and migration risk.

2. **Preview isolation mechanism choice**
   - What we know: preview failures must preserve last committed valid state (D-06).
   - What's unclear: temp-scene clone vs intermediate model validation without scene mutation.
   - Recommendation: for Phase 13 baseline, prefer intermediate model validation + explicit apply; reserve full transactional scene branching for Phase 14.

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| Lua CLI/runtime | SCRP-06 baseline verification | ✓ | 5.4.8 | — |
| CMake | Build/integration checks | ✓ | 4.3.0 | — |
| CTest | Validation command path | ✓ | 4.3.0 | If no scripted tests yet, run target-level smoke build |
| Python | Utility scripts/evidence tooling | ✓ | 3.13.12 | — |
| Node/npm | Existing project scripts/tooling | ✓ | Node 25.8.1 / npm 11.11.0 | — |
| Ninja | optional generator | ✗ | — | Use Visual Studio generator/build-vulkan (already used in repo) |

**Missing dependencies with no fallback:**
- None identified.

**Missing dependencies with fallback:**
- Ninja (fallback: existing Visual Studio/MSVC build workflow).

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | CMake/CTest targets (project-level), plus command smoke validation |
| Config file | `src/CMakeLists.txt` (no dedicated Phase 13 script test config yet) |
| Quick run command | `cmake --build build-vulkan --config Release --target mdcad` |
| Full suite command | `ctest --test-dir build-vulkan -C Release --output-on-failure` |

### Phase Requirements → Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| SCRP-01 | Standalone script editor can be opened from SketchManager | integration/UI smoke | `ctest --test-dir build-vulkan -C Release --output-on-failure` | ❌ Wave 0 |
| SCRP-02 | Script parse reconstructs supported sub-scene with links/values | unit + integration | `ctest --test-dir build-vulkan -C Release --output-on-failure` | ❌ Wave 0 |
| SCRP-03 | UI edits produce deterministic script output | unit (canonical emit) + integration | `ctest --test-dir build-vulkan -C Release --output-on-failure` | ❌ Wave 0 |
| SCRP-06 | Runtime baseline locked to Lua 5.4.x | smoke/env check | `lua -v` | ❌ Wave 0 |

### Sampling Rate
- **Per task commit:** `cmake --build build-vulkan --config Release --target mdcad`
- **Per wave merge:** `ctest --test-dir build-vulkan -C Release --output-on-failure`
- **Phase gate:** Full suite green + explicit Lua 5.4.x version evidence before `/gsd-verify-work`

### Wave 0 Gaps
- [ ] Add dedicated script round-trip tests (parser/apply/emitter determinism) under source-controlled test target.
- [ ] Add Lua runtime/version assertion test or startup guard for SCRP-06.
- [ ] Add deterministic golden-file style script emit checks for no-op edit stability.

## Sources

### Primary (HIGH confidence)
- `.planning/phases/13-script-round-trip-baseline/13-CONTEXT.md` - locked decisions and phase boundaries.
- `.planning/REQUIREMENTS.md` - SCRP-01/02/03/06 contract.
- `.planning/ROADMAP.md` - Phase 13 success criteria.
- `docs/feature-proposal/Sketches, Constraints, Scripting.md` - product-level scripting behavior.
- `src/ui/ui_entity_inspector.h` - SketchManager/workspace standalone UI patterns.
- `src/ecs/ecs_scene.h` - authoritative mutation contracts for sketch/constraint graph.
- `src/scene_serializer.h` - two-pass reconstruction and participant remap precedent.
- Local environment probes (`lua -v`, `cmake --version`, `ctest --version`) - dependency availability.

### Secondary (MEDIUM confidence)
- `src/components/sketch_comp.h`, `src/components/constraint_comp.h` - current metadata/value contracts to extend for stable IDs.
- `src/CMakeLists.txt` + existing `build-vulkan/CTestTestfile.cmake` observations - current test infrastructure shape.

### Tertiary (LOW confidence)
- None.

## Metadata

**Confidence breakdown:**
- Standard stack: **MEDIUM** - Lua baseline is locked and locally verified; embedding strategy details remain implementation-specific.
- Architecture: **HIGH** - strong direct precedent in existing scene/UI serializer code paths.
- Pitfalls: **MEDIUM** - grounded in current code patterns, but script module does not yet exist and needs validation once implemented.

**Research date:** 2026-04-01  
**Valid until:** 2026-04-30

