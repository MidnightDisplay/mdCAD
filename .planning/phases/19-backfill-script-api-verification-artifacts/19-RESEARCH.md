# Phase 19: Backfill Script/API Verification Artifacts - Research

**Researched:** 2026-04-05  
**Domain:** Verification artifact backfill, requirement-evidence traceability, targeted Windows MSVC + Vulkan reruns  
**Confidence:** HIGH

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
### Evidence freshness level
- **D-01:** Use **fresh targeted reruns** plus new verification docs as the default closure strategy.
- **D-02:** Prefer targeted script/API verification commands over mandatory full-suite rebuilds unless a specific closure row requires broader coverage.

### Manual evidence policy
- **D-03:** Reuse previously approved manual/UAT outcomes where acceptance already happened, but require explicit cross-links to the originating evidence artifacts.
- **D-04:** Do not invent manual pass claims; every reused manual claim must cite concrete prior checkpoint/UAT records.

### Verification status normalization
- **D-05:** Requirements previously blocked by `human_needed` may be upgraded to `passed` when implementation and acceptance are already evidenced, with explicit traceability links.
- **D-06:** Status upgrades must include rationale in verification docs so audit readers can reconstruct why closure is valid without rerunning the full history.

### Scope boundary enforcement
- **D-07:** Keep Phase 19 strictly scoped to Phase 13/14 script+API verification closure (`SCRP-01..06`, `API-01..02`).
- **D-08:** No new runtime behavior/features are introduced in this phase.

### Prep notes for downstream closure
- **D-09:** Include concise prep notes for Phase 20/21 only as handoff context (not as implemented scope in Phase 19).

### the agent's Discretion
- Exact command mix for targeted reruns (`script_roundtrip_tests`-focused smoke/gate variants) as long as evidence remains auditable.
- Exact structure of the new verification documents (tables/sections/frontmatter), provided requirement traceability is explicit and complete.
- Exact wording of cross-artifact status-upgrade rationale, provided it is objective and source-linked.

### Deferred Ideas (OUT OF SCOPE)
- Any new scripting/API features beyond closure evidence belong in future feature phases, not Phase 19.
- Broader milestone cleanup outside `SCRP-*` and `API-*` closure belongs to Phases 20 and 21.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| SCRP-01 | Standalone sketch script editor window from SketchManager | `app.c` script editor lifecycle + `script_roundtrip_tests` (`test_script_editor_launch_request_is_exposed_from_inspector_state`) + Phase 13 summary/checkpoint evidence cross-link pattern |
| SCRP-02 | Reconstruct full sketch sub-scene from script parse output | `scene_script_apply_commit` contract in `ecs_scene.h` + tests for supported-scope reconstruction and two-pass references |
| SCRP-03 | UI-side edits deterministically update script output | `scene_script_reemit_for_sketch` and revision flow + deterministic emit tests (`orders_by_type_and_script_id`, `noop_stability`, numeric format test) |
| SCRP-04 | Script edits safe with last-valid preservation on failure | `scene_script_preview_parse`/`scene_script_apply_commit` rollback behavior + tests for committed-scene preservation on parse/apply failure |
| SCRP-05 | Numeric script IO inputs/outputs with optional min/max/step and live interaction | `scene_script_io_*` APIs + Script IO window flow in `app.c` + tests for schema, live edit transaction, and coalescing |
| SCRP-06 | Lua 5.4.x baseline | Runtime assertion test (`test_runtime_rejects_non_54`) and prior Phase 13 validation command history |
| API-01 | Scene API entrypoints for sketch/geometry/constraint/script workflows | Scene façade declarations in `ecs_scene.h` and usage from UI thin callers (`app.c`) |
| API-02 | Transactional undo/redo for script-driven mutations without partial restore | `CMD_SCRIPT_APPLY_TRANSACTION` in `undo_redo.h` + apply/unapply execution in `undo_redo_exec.h` + tests for command type + noop suppression + single-step undo |
</phase_requirements>

## Summary

Phase 19 is a **verification closure phase**, not an implementation phase. The core technical task is to produce missing auditable verification artifacts for Phase 13 and Phase 14 so `SCRP-01..06` and `API-01..02` move from orphaned/missing evidence status to explicit requirement-level closure. The milestone audit identifies the blocker clearly: implementation and summaries exist, but `13-VERIFICATION.md` and `14-VERIFICATION.md` are missing.

The existing code and tests already contain strong coverage anchors. `script_roundtrip_tests` includes targeted checks for Lua baseline, reconstruct/apply behavior, deterministic emit, script safety on failure, IO schema and live edits, and script transaction undo semantics. Scene-level façades (`scene_script_preview_parse`, `scene_script_apply_commit`, `scene_script_io_*`, `scene_script_reemit_for_sketch`) and undo command contracts (`CMD_SCRIPT_APPLY_TRANSACTION`) are present and testable without new runtime changes.

The plan should focus on: (1) generating phase verification documents with requirement-by-requirement evidence tables, (2) running fresh targeted reruns on Windows MSVC + Vulkan, (3) cross-linking previously approved manual/UAT evidence from Phase 13/14 summaries, and (4) adding explicit rationale text for any status normalization from historical `human_needed`/missing states to `passed`.

**Primary recommendation:** Create `13-VERIFICATION.md` and `14-VERIFICATION.md` using fresh `script_roundtrip_tests` reruns plus explicit links to accepted prior manual evidence; do not change runtime code.

## Project Constraints (from copilot-instructions.md)

No `./copilot-instructions.md` file exists in repository root at research time, so no additional project-specific constraints were extracted from that source.

## Standard Stack

### Core
| Library/Tool | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| CMake | 4.3.0 (local) | Build orchestration for mdCAD targets (`mdCAD`, `script_roundtrip_tests`) | Canonical build path already used in phase validation artifacts |
| CTest | 4.3.0 (local) | Requirement evidence reruns for native tests | Existing validation docs and roadmap gates already rely on CTest |
| `script_roundtrip_tests` target | In-repo native target | Focused script/API/undo regression coverage | Directly maps to all Phase 19 requirements without broad rebuild noise |

### Supporting
| Library/Tool | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| MSVC build (`build-vulkan`, Release) | Project-local configured toolchain | Gate-platform proof artifact (Windows MSVC + Vulkan) | Use for requirement rows requiring executable/build proof, not every row |
| Lua runtime baseline checks (in tests) | 5.4.x contract (asserted by tests) | SCRP-06 evidence support | Use in requirement-specific verification narrative and rerun command evidence |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Targeted reruns (`-R script_roundtrip_tests`) | Full CTest suite every row | Slower and contradicts D-02 unless row explicitly needs broader coverage |
| Reusing approved manual evidence with links | Re-run all manual flows | Higher cost, unnecessary per D-03 if prior acceptance is concrete and cited |

**Installation:**
```bash
# No new package installation required for Phase 19.
# Existing project toolchain is used.
```

**Version verification:** Local environment probe confirms:
- `cmake version 4.3.0`
- `ctest version 4.3.0`
- `VULKAN_SDK=C:\VulkanSDK\1.4.341.1`

## Architecture Patterns

### Recommended Project Structure
```text
.planning/phases/13-script-round-trip-baseline/
  13-VERIFICATION.md        # NEW (phase-level requirement evidence)
  13-VALIDATION.md          # existing command strategy
  13-03-SUMMARY.md          # accepted manual evidence source

.planning/phases/14-script-io-api-undo-integration/
  14-VERIFICATION.md        # NEW (phase-level requirement evidence)
  14-VALIDATION.md          # existing command strategy
  14-03-SUMMARY.md          # accepted UAT/manual evidence source
```

### Pattern 1: Requirement-first verification matrix
**What:** One row per requirement ID (`SCRP-*`, `API-*`) with implementation anchors, fresh command evidence, and manual evidence links where needed.  
**When to use:** Always for audit closure phases where requirements are implemented but verification artifacts are missing.  
**Example:**
```markdown
| Requirement | Status | Implementation Anchor | Automated Evidence | Manual Evidence | Rationale |
|-------------|--------|-----------------------|--------------------|-----------------|-----------|
| SCRP-04 | passed | scene_script_apply_commit | ctest -R script_roundtrip_tests ... (PASS) | 14-03-SUMMARY.md §Human-Verify Outcome | last-valid rollback already accepted; rerun confirms no regression |
```

### Pattern 2: Scene façade + thin UI caller evidence
**What:** Verify API requirements against scene-level façades, with UI as caller only.  
**When to use:** API-01 and API-02 claims to avoid accidental low-level coupling claims.  
**Example:**
```c
// Source: src/ecs/ecs_scene.h
static inline bool scene_script_preview_parse(...);
static inline bool scene_script_apply_commit(...);
static inline bool scene_script_io_apply_input_value(...);
```

### Pattern 3: Transactional script undo command proof
**What:** Anchor API-02 to dedicated command type + replay behavior.  
**When to use:** Any claim about script undo atomicity or IO edit coalescing semantics.  
**Example:**
```c
// Source: src/undo_redo.h / src/undo_redo_exec.h
CMD_SCRIPT_APPLY_TRANSACTION
undo_cmd_script_apply_transaction(...)
```

### Anti-Patterns to Avoid
- **Feature creep during closure:** Do not modify script runtime/UI behavior in Phase 19 (violates D-08).
- **Manual claim without source:** Never mark manual pass unless linked to accepted prior artifacts (D-04).
- **Unscoped verification doc:** Avoid broad non-Phase-19 requirements in new verification reports (D-07).

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Script/API regression harness | New ad-hoc test binary | Existing `script_roundtrip_tests` target | Already covers all required script/API surfaces with established trust |
| Transaction replay logic for evidence | New fake undo model | Existing `CMD_SCRIPT_APPLY_TRANSACTION` paths | Prevents divergence from actual runtime behavior |
| Manual test narratives | New uncited retrospective claims | Existing accepted Phase 13/14 summaries/checkpoints with explicit cross-links | Audit requires traceability, not memory-based claims |

**Key insight:** Phase 19 should compose existing tested contracts into auditable artifacts, not build new behavior.

## Common Pitfalls

### Pitfall 1: “Implemented” mistaken for “verified”
**What goes wrong:** Team assumes completed summaries are enough and skips `*-VERIFICATION.md`.  
**Why it happens:** Summaries and validation docs exist, giving false closure confidence.  
**How to avoid:** Require requirement-by-requirement verification tables in new phase verification artifacts.  
**Warning signs:** Milestone audit flags requirements as orphaned despite completed plans.

### Pitfall 2: Status upgrade without rationale trail
**What goes wrong:** Requirement status flips to passed without explanation.  
**Why it happens:** Fast cleanup focuses only on checkbox parity.  
**How to avoid:** Include explicit “status-upgrade rationale” text + links to prior accepted evidence and fresh reruns.  
**Warning signs:** Reviewer cannot reconstruct why `human_needed`/missing became passed.

### Pitfall 3: Overusing full-suite reruns
**What goes wrong:** Verification cycle becomes slow and noisy, delaying closure.  
**Why it happens:** Teams default to broad reruns for every row.  
**How to avoid:** Use `script_roundtrip_tests` targeted reruns by default per D-02; run broader gate only where claim requires it.  
**Warning signs:** Repeated full rebuilds with no additional requirement signal.

## Code Examples

Verified patterns from in-repo sources:

### Script transaction command contract
```c
// Source: src/undo_redo.h
CMD_SCRIPT_APPLY_TRANSACTION,
...
static inline void undo_cmd_script_apply_transaction(undo_redo_t *ur,
                                                      ecs_entity_t sketch,
                                                      const char *before_script,
                                                      const char *after_script)
```

### Scene scripting façade APIs
```c
// Source: src/ecs/ecs_scene.h
static inline bool scene_script_preview_parse(...);
static inline bool scene_script_apply_commit(...);
static inline bool scene_script_io_apply_input_value(...);
static inline bool scene_script_reemit_for_sketch(...);
```

### CTest registration anchor
```cmake
# Source: src/CMakeLists.txt
add_executable(script_roundtrip_tests tests/script_roundtrip_tests.c)
add_test(NAME script_roundtrip_tests COMMAND script_roundtrip_tests)
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Plan summaries + validation only (no phase verification artifact) | Mandatory requirement-level `*-VERIFICATION.md` parity for audit closure | Enforced by v1.2 audit (2026-04-05) | Prevents orphaned requirement status |
| Implicit manual acceptance references | Explicit cross-linked manual evidence policy | Phase 19 context decisions | Improves audit reconstruction and trust |
| Broad “build everything” loop | Targeted reruns first (`script_roundtrip_tests`) | Phase 19 locked decision D-02 | Faster closure with requirement-focused evidence |

**Deprecated/outdated:**
- “Missing verification docs are acceptable if summaries exist” — outdated for milestone audit readiness.

## Open Questions

1. **Should any Phase 19 requirement rows require full `mdCAD` build evidence beyond `script_roundtrip_tests`?**
   - What we know: D-02 prefers targeted reruns; many requirements are API/behavior covered by script tests.
   - What's unclear: Whether reviewer expects at least one explicit `cmake --build ... --target mdCAD` rerun row in each new verification doc.
   - Recommendation: Include one explicit Windows build proof row per verification artifact for gate-platform continuity, but keep requirement rows targeted.

2. **How much manual detail should be copied vs linked from prior summaries?**
   - What we know: D-03/D-04 require reuse via explicit cross-links, not reinvention.
   - What's unclear: Preferred verbosity of repeated manual checklists.
   - Recommendation: Link and quote concise accepted outcomes from `13-03-SUMMARY.md` and `14-03-SUMMARY.md`, avoid duplicating full UAT text.

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| CMake | Build/rerun gate commands | ✓ | 4.3.0 | — |
| CTest | Targeted script/API reruns | ✓ | 4.3.0 | — |
| MSVC-configured build tree (`build-vulkan`) | Windows gate evidence continuity | ✓ | configured tree exists | Reconfigure if cache invalid |
| Vulkan SDK env (`VULKAN_SDK`) | Windows Vulkan target consistency | ✓ | `C:\VulkanSDK\1.4.341.1` | If missing, use non-Vulkan gate only with explicit deviation note |
| Git | Evidence provenance/checkpoint links | ✓ | 2.51.1.windows.1 | — |

**Missing dependencies with no fallback:**
- None identified.

**Missing dependencies with fallback:**
- None identified.

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | CMake/CTest native test targets (`script_roundtrip_tests`) |
| Config file | `src/CMakeLists.txt` |
| Quick run command | `ctest --test-dir build-vulkan -C Release --output-on-failure -R script_roundtrip_tests` |
| Full suite command | `cmake --build build-vulkan --config Release --target script_roundtrip_tests && ctest --test-dir build-vulkan -C Release --output-on-failure -R script_roundtrip_tests` |

### Windows MSVC + Vulkan targeted rerun strategy (Phase 19)
1. `cmake --build build-vulkan --config Release --target script_roundtrip_tests`
2. `ctest --test-dir build-vulkan -C Release --output-on-failure -R script_roundtrip_tests`
3. (Optional gate continuity row) `cmake --build build-vulkan --config Release --target mdCAD`

### Phase Requirements → Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| SCRP-01 | Script Editor request/standalone launch contract | unit/integration | `ctest --test-dir build-vulkan -C Release --output-on-failure -R script_roundtrip_tests` | ✅ |
| SCRP-02 | Reconstruct sub-scene + forward refs | integration | `ctest --test-dir build-vulkan -C Release --output-on-failure -R script_roundtrip_tests` | ✅ |
| SCRP-03 | Deterministic emit + stable reemit | integration | `ctest --test-dir build-vulkan -C Release --output-on-failure -R script_roundtrip_tests` | ✅ |
| SCRP-04 | Atomic apply/preview safety, last-valid preservation | integration | `ctest --test-dir build-vulkan -C Release --output-on-failure -R script_roundtrip_tests` | ✅ |
| SCRP-05 | IO parse/emit/apply and interaction coalescing semantics | integration | `ctest --test-dir build-vulkan -C Release --output-on-failure -R script_roundtrip_tests` | ✅ |
| SCRP-06 | Lua 5.4 baseline assertion behavior | unit | `ctest --test-dir build-vulkan -C Release --output-on-failure -R script_roundtrip_tests` | ✅ |
| API-01 | Scene façade coverage for script workflows | integration | `ctest --test-dir build-vulkan -C Release --output-on-failure -R script_roundtrip_tests` | ✅ |
| API-02 | Script transaction undo/redo atomicity | integration | `ctest --test-dir build-vulkan -C Release --output-on-failure -R script_roundtrip_tests` | ✅ |

### Sampling Rate
- **Per task commit:** `ctest --test-dir build-vulkan -C Release --output-on-failure -R script_roundtrip_tests`
- **Per wave merge:** `cmake --build build-vulkan --config Release --target script_roundtrip_tests && ctest --test-dir build-vulkan -C Release --output-on-failure -R script_roundtrip_tests`
- **Phase gate:** Include explicit Windows MSVC + Vulkan targeted rerun evidence in each new verification artifact before `/gsd-verify-work`

### Wave 0 Gaps
- None — existing test infrastructure and target registration already cover all Phase 19 requirement behaviors.

## Sources

### Primary (HIGH confidence)
- `.planning/phases/19-backfill-script-api-verification-artifacts/19-CONTEXT.md` — locked phase decisions and scope boundaries.
- `.planning/ROADMAP.md` — authoritative Phase 19 goal/requirements/dependencies.
- `.planning/REQUIREMENTS.md` — requirement definitions and Phase 19 traceability ownership.
- `.planning/v1.2-MILESTONE-AUDIT.md` — root-cause evidence for missing 13/14 verification artifacts.
- `src/CMakeLists.txt` — `script_roundtrip_tests` target registration and CTest binding.
- `src/tests/script_roundtrip_tests.c` — detailed automated evidence anchors for SCRP/API requirements.
- `src/ecs/ecs_scene.h` — scene-level script/API façade contracts and transactional apply hooks.
- `src/undo_redo.h`, `src/undo_redo_exec.h` — script transaction command type and replay semantics.
- `src/app.c` — Script Editor/IO behavior surfaces tied to manual evidence reuse.
- `.planning/phases/13-script-round-trip-baseline/13-VALIDATION.md`, `13-03-SUMMARY.md` — prior accepted validation/manual evidence.
- `.planning/phases/14-script-io-api-undo-integration/14-VALIDATION.md`, `14-03-SUMMARY.md` — prior accepted validation/manual evidence.

### Secondary (MEDIUM confidence)
- `.planning/STATE.md` and `CHECKPOINT.md` — continuity context and historical gate command posture (supporting context, not primary requirement proof).

### Tertiary (LOW confidence)
- None.

## Metadata

**Confidence breakdown:**
- Standard stack: **HIGH** - derived from in-repo build/test configuration and local environment checks.
- Architecture: **HIGH** - directly anchored in current Phase 19 context decisions and existing repository contracts.
- Pitfalls: **HIGH** - directly supported by milestone audit findings and prior phase artifact patterns.

**Research date:** 2026-04-05  
**Valid until:** 2026-05-05

