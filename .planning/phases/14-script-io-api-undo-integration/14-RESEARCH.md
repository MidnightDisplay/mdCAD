# Phase 14: Script IO + API/Undo Integration - Research

**Researched:** 2026-04-01  
**Domain:** Script-driven sketch mutation safety, dynamic numeric IO UI, scene API façade coherence, transactional undo/redo  
**Confidence:** MEDIUM

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
### Script-side apply safety
- **D-01:** Phase 14 uses **strict transactional apply** semantics: any parse/validation/apply failure aborts the operation and restores the full pre-apply scene state.
- **D-02:** Safety rules apply consistently to both explicit `Apply Script` actions and script-IO-driven edits.
- **D-03:** Failure paths must surface explicit diagnostics; silent no-op failure behavior is not acceptable.

### Script IO schema (Phase 14 scope)
- **D-04:** IO schema for Phase 14 is **numeric-only** and split into `inputs` and `outputs`.
- **D-05:** Numeric IO supports optional `min/max/step` metadata; when present, UI must expose both numeric input and slider semantics.
- **D-06:** Non-numeric/multi-type IO (`int/bool/string` as separate widget families) is deferred out of Phase 14 scope.

### Script IO frontend behavior
- **D-07:** Script IO is shown in a **dedicated ImGui window** (not embedded in Script Editor), opened via a toggle button next to `Open Script Editor` in SketchManager.
- **D-08:** IO window shows editable inputs and read-only outputs with immediate bidirectional sync between UI values and script state.
- **D-09:** IO input edits auto-apply live, but each edit must run through the same transactional pipeline (validate -> atomic apply -> rollback on failure).

### Scene API integration shape
- **D-10:** Scene API exposure remains **scene-level façade first** (workflow-family entrypoints), with UI as a thin caller and no direct low-level ECS mutation flow from UI panels.
- **D-11:** Script workflow APIs, sketch/geometry manager APIs, and constraint manager APIs should align behind coherent scene-level contracts to satisfy `API-01`.

### Undo/redo transaction contract
- **D-12:** One successful `Apply Script` operation equals **one atomic undo entry**.
- **D-13:** Rolling back a script-driven mutation must restore full pre-apply state (no partial restores), including solver-impacting state and related sketch entities/constraints.
- **D-14:** IO auto-apply edits use the same atomicity guarantees as manual apply and must not poison subsequent valid edits.

### the agent's Discretion
- Exact C struct layout for numeric IO declarations as long as `inputs/outputs` + optional `min/max/step` behavior is preserved.
- Exact dedicated IO window layout/styling details and docking behavior, while keeping current ImGui conventions.
- Exact internal undo payload representation for script transactions, provided user-observed behavior remains one atomic undo/redo unit per committed apply.

### Deferred Ideas (OUT OF SCOPE)
- Non-numeric/multi-type IO widgets (`bool`, `string`, richer type families) are deferred to a future phase.
- Broader scripting capability expansion beyond deterministic sketch-parametric workflows remains outside Phase 14.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| SCRP-04 | Script-side edits update scene safely while preserving last-valid sketch state on parse/apply errors | Existing `scene_script_preview_parse` + `scene_script_apply_commit` façade and atomic tests are present; Phase 14 must harden rollback fidelity and add undo transaction wrapping for commit path. |
| SCRP-05 | Script input/output variables with generated numeric controls/readouts + optional min/max/step | Add parse/emit schema extension for `inputs`/`outputs`, dedicated IO window in app/inspector flow, and scene-level IO apply API that reuses script commit safety pipeline. |
| API-01 | Scene API entrypoints for sketch/geometry-manager/constraint-manager/script workflows | Current scene façade already exists for core sketch/script/constraint operations; Phase 14 should add explicit script-IO façade methods and route UI through them (no direct ECS mutations from IO UI). |
| API-02 | Transactional undo/redo for sketch/solver-impacting/script-driven mutations without partial restore | Existing undo system has no script-transaction command type; add single command encapsulating full sketch subtree snapshot/restore around script applies so one apply = one undo step. |
</phase_requirements>

## Summary

Phase 14 should be planned as an integration-hardening phase, not a greenfield scripting rewrite. The codebase already has a deterministic script parse/apply/emit pipeline (`sketch_script_parse.h`, `sketch_script_apply.h`, `sketch_script_emit.h`) and a scene façade (`scene_script_preview_parse`, `scene_script_apply_commit`) that UI already calls from Script Editor. The main gap is not basic parsing—it is transactional boundaries across UI-driven script apply, dynamic IO auto-apply, and undo/redo semantics.

`SCRP-04` is partially in place today: current tests confirm parse failure and unresolved references do not mutate committed sketch state. However, `API-02` is not yet met for script operations because successful script apply is not represented as one undo command. Existing undo commands are granular (create/delete/property/bulk) and there is no script-specific atomic transaction command. This is the highest-risk integration gap.

`SCRP-05` and `API-01` should be implemented by extending the existing scene-first pattern: add script IO declarations to parser/emitter model, add dedicated scene API entrypoints for IO apply/readback, and keep app/inspector as thin callers. The IO window must be a sibling to Script Editor (not embedded), launched from SketchManager next to `Open Script Editor`, and every input edit must run through the same commit pipeline with explicit diagnostics.

**Primary recommendation:** Plan Phase 14 around one shared transactional script mutation pipeline (manual apply + IO auto-apply) plus one new atomic undo command type for script apply.

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| cimgui / Dear ImGui | `1.92.5dock` (from `vendors/libcimgui/CMakeLists.txt` `GIT_TAG`) | Script editor + new Script IO immediate-mode panels | Already integrated; all sketch tooling UX uses ImGui patterns |
| Flecs | vendored single-header/static (`vendors/flecs/flecs.c`) | ECS world/entity/component ownership for sketch graph | Existing scene architecture is Flecs-first; required for consistent parent/child + component restoration |
| Sokol | `master` (from `vendors/libsokol/CMakeLists.txt` `GIT_TAG`) | Runtime platform/render integration used by app shell | App lifecycle/input/render hooks are already sokol-based |
| cJSON | vendored static (`vendors/cjson`) | Existing scene serialization support used by tests/snapshots | Reused in scene persistence and test fixtures |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| CTest (via CMake) | CMake 4.3.0 toolchain present | Run native regression tests | For phase gate and per-commit script/undo regressions |
| Lua runtime contract layer | Lua 5.4 baseline enforced by tests/contracts | Script compatibility guardrails | Keep strict with existing `SCRP-06` baseline |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Header-only scene/script façade pattern | Split into `.c` translation units now | Larger refactor risk in integration phase; not needed to hit Phase 14 requirements |
| Reusing existing undo stack | Separate ad-hoc rollback manager for script UI | Duplicates state authority and risks inconsistent undo semantics |

**Installation:** existing in-repo CMake build; no new package manager required for this phase.

## Architecture Patterns

### Recommended Project Structure
```text
src/
├── scripting/
│   ├── sketch_script_parse.h      # extend model for inputs/outputs metadata
│   ├── sketch_script_emit.h       # deterministic emit incl. io declarations
│   └── sketch_script_apply.h      # commit pipeline reused by IO auto-apply
├── ecs/
│   └── ecs_scene.h                # scene-level script/io façade entrypoints
├── ui/
│   └── ui_entity_inspector.h      # SketchManager toggle entrypoint for IO window
├── app.c                          # dedicated IO window state + draw loop
└── undo_redo.{h,exec.h}           # new script-transaction command + apply/unapply
```

### Pattern 1: Scene façade owns mutation authority
**What:** UI asks scene APIs to mutate; UI does not directly mutate low-level ECS for business flows.  
**When to use:** All script, IO auto-apply, geometry-manager, and constraint-manager pathways.  
**Example:**
```c
// Source: src/app.c + src/ecs/ecs_scene.h
bool ok = scene_script_apply_commit(&state.ecs_scene, sketch, script_text, &err);
```

### Pattern 2: Two-stage script flow (preview then commit)
**What:** validate script before commit; commit path performs full model apply.  
**When to use:** Manual Apply Script button and every IO input edit.  
**Example:**
```c
// Source: src/ecs/ecs_scene.h
scene_script_preview_parse(scene, sketch, text, &preview_err);
scene_script_apply_commit(scene, sketch, text, &apply_err);
```

### Pattern 3: Deterministic re-emit revision as UI sync trigger
**What:** scene mutators call `scene_script_reemit_for_sketch`; UI observes `scene_script_emit_revision`.  
**When to use:** after successful script apply and IO-driven mutation to keep editor/IO panels coherent.  
**Example:**
```c
// Source: src/scripting/sketch_script_emit.h
uint64_t rev = scene_script_emit_revision(&state.ecs_scene);
```

### Anti-Patterns to Avoid
- **Direct ECS writes in IO window:** bypasses API-01 façade contract and fractures behavior across panels.
- **Separate IO apply path from Apply Script path:** violates D-02/D-09 and will drift error/rollback behavior.
- **Multi-command undo push for one script apply:** violates D-12 and leads to partial state restores.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Script mutation transaction history | New bespoke history stack | Existing `undo_redo_t` + new script transaction command type | Keeps all editor undo semantics in one system |
| Constraint legality checks | Custom ad-hoc participant validators in UI | Existing `constraint_type_is_selection_legal` / script apply validation path | Already encodes legal combos and prevents divergence |
| Script synchronization heuristics | String diff patches between editor and scene | Existing deterministic emit + revision counter | Deterministic order/format already proven by tests |

**Key insight:** Most complexity is already solved in scene/scripting primitives; Phase 14 should compose them, not replace them.

## Common Pitfalls

### Pitfall 1: “Atomic apply” implemented without undo atomicity
**What goes wrong:** Apply succeeds, but undo requires many steps or restores partially.  
**Why it happens:** No script-level undo command exists today.  
**How to avoid:** Introduce one command representing full sketch pre/post snapshot.  
**Warning signs:** Undo name shows generic granular actions after Apply Script.

### Pitfall 2: IO auto-apply bypasses preview/commit validation
**What goes wrong:** IO edit can mutate scene into invalid intermediate state.  
**Why it happens:** Developers optimize for immediacy and skip shared pipeline.  
**How to avoid:** IO input change must call same preview+commit contract as Apply Script.  
**Warning signs:** IO path has direct component edits or separate error handling.

### Pitfall 3: Parser extension breaks deterministic emit
**What goes wrong:** Script churn/noisy diffs and unstable editor text after no-op changes.  
**Why it happens:** New IO fields emitted in non-deterministic order/format.  
**How to avoid:** Preserve stable key ordering and numeric formatting conventions used today.  
**Warning signs:** No-op emit stability tests start failing.

## Code Examples

### Existing commit pathway from Script Editor
```c
// Source: src/app.c:256-263
if (igButton("Apply Script", (ImVec2){180.0f, 0.0f})) {
    sketch_script_error_t apply_error = {0};
    bool applied = scene_script_apply_commit(&state.ecs_scene,
                                             state.script_editor_sketch,
                                             state.script_editor_text,
                                             &apply_error);
}
```

### Existing façade boundary
```c
// Source: src/ecs/ecs_scene.h:3098-3109
static inline bool scene_script_preview_parse(ecs_scene_t *scene, ecs_entity_t sketch,
                                              const char *script_text, sketch_script_error_t *out_error) {
    return sketch_script_apply_preview_model(scene, sketch, script_text, out_error);
}
```

### Existing undo entrypoints
```c
// Source: src/undo_redo_exec.h:1520-1537
bool undo_redo_undo(undo_redo_t *ur);
bool undo_redo_redo(undo_redo_t *ur);
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Ad-hoc script editing without formal tests | Deterministic parser/apply/emit with dedicated `script_roundtrip_tests` | Phase 13 completion | Strong baseline for extending IO + transactions safely |
| UI logic owning behavior details | Scene façade + header-only scene/scripting contracts | Phases 10-13 pattern | Enables API-01 compliance with thin UI callers |

**Deprecated/outdated:**
- Treating script apply as “just editor feature” is outdated; Phase 14 requires cross-cutting API + undo transaction semantics.

## Open Questions

1. **How should script IO variables map to executable script state?**
   - What we know: Product intent requires `inputs` editable, `outputs` read-only, optional `min/max/step`.
   - What's unclear: Exact storage location and evaluation lifecycle for output values in current Lua flow.
   - Recommendation: Define explicit scene-level IO model structs and a single evaluation/apply function reused by editor and IO window.

2. **What is minimal snapshot payload for script undo command?**
   - What we know: Existing undo snapshots can recreate entity trees and linked constraints.
   - What's unclear: Best granularity for script command (whole sketch subtree vs selective diff).
   - Recommendation: Start with full sketch subtree snapshot for correctness, optimize later only if profiling proves needed.

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| cmake | Configure/build tests | ✓ | 4.3.0 | — |
| ctest | Run regression suite | ✓ | 4.3.0 | — |
| C compiler toolchain (`cl` on Windows path) | Build native targets | ✗ (in current shell path) | — | Use existing prebuilt `build-vulkan` artifacts for read-only analysis only |
| git | commit_docs workflow | ✓ | 2.51.1.windows.1 | — |
| node | gsd tooling/commit helper | ✓ | v25.8.1 | — |

**Missing dependencies with no fallback:**
- Active C compiler in PATH for fresh local build/validation.

**Missing dependencies with fallback:**
- None.

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | CTest + native C test executable (`script_roundtrip_tests`) |
| Config file | `CMakeLists.txt` + `src/CMakeLists.txt` |
| Quick run command | `ctest --test-dir build-vulkan -C Release -R script_roundtrip_tests --output-on-failure` |
| Full suite command | `ctest --test-dir build-vulkan -C Release --output-on-failure` |

### Phase Requirements → Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| SCRP-04 | Failed parse/apply preserves last-valid scene | unit/integration | `ctest --test-dir build-vulkan -C Release -R script_roundtrip_tests --output-on-failure` | ✅ |
| SCRP-05 | Script IO numeric inputs/outputs + min/max/step dynamic UI | integration/UI | `ctest --test-dir build-vulkan -C Release -R script_roundtrip_tests --output-on-failure` (plus manual UI until new tests added) | ❌ Wave 0 |
| API-01 | Scene façade entrypoints cover sketch/geometry/constraint/script workflows | unit/integration | `ctest --test-dir build-vulkan -C Release -R script_roundtrip_tests --output-on-failure` | ✅ (partial; IO API gap) |
| API-02 | Script-driven mutations undo/redo as one atomic transaction | integration | `ctest --test-dir build-vulkan -C Release -R script_roundtrip_tests --output-on-failure` (after adding new undo tests) | ❌ Wave 0 |

### Sampling Rate
- **Per task commit:** `ctest --test-dir build-vulkan -C Release -R script_roundtrip_tests --output-on-failure`
- **Per wave merge:** `ctest --test-dir build-vulkan -C Release --output-on-failure`
- **Phase gate:** Full suite green before `/gsd-verify-work`

### Wave 0 Gaps
- [ ] `src/tests/script_roundtrip_tests.c` — add explicit script-transaction undo/redo atomicity coverage for API-02.
- [ ] `src/tests/script_roundtrip_tests.c` — add parser/emit/apply tests for `inputs`/`outputs` with `min/max/step`.
- [ ] Add UI-level contract test hook (or deterministic state test) proving IO auto-apply uses same transactional path as Apply Script.

## Sources

### Primary (HIGH confidence)
- `src/ecs/ecs_scene.h` — script façade entrypoints, sketch/constraint APIs, script revision hooks.
- `src/scripting/sketch_script_apply.h` — parse/validate/commit flow and rollback behavior.
- `src/scripting/sketch_script_parse.h` — current script schema/parser boundaries.
- `src/scripting/sketch_script_emit.h` — deterministic emit ordering/number formatting/revision logic.
- `src/app.c` — Script Editor lifecycle, preview/apply error handling, undo shortcuts.
- `src/ui/ui_entity_inspector.h` — SketchManager script editor trigger and thin UI patterns.
- `src/undo_redo.h`, `src/undo_redo_exec.h` — command model and apply/unapply restore mechanics.
- `src/tests/script_roundtrip_tests.c` — existing regression coverage and gaps.
- `.planning/phases/14-script-io-api-undo-integration/14-CONTEXT.md` — locked decisions and out-of-scope constraints.

### Secondary (MEDIUM confidence)
- `.planning/ROADMAP.md`, `.planning/REQUIREMENTS.md`, `.planning/STATE.md` — requirement mapping and accepted prior-phase contracts.
- `docs/feature-proposal/Sketches, Constraints, Scripting.md` — product-intent details for script IO frontend behavior.

### Tertiary (LOW confidence)
- None.

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — directly verified from repo CMake/vendor definitions.
- Architecture: HIGH — directly verified from current scene/app/undo code paths.
- Pitfalls: MEDIUM — inferred from current gaps vs locked Phase 14 behavior.

**Research date:** 2026-04-01  
**Valid until:** 2026-05-01
