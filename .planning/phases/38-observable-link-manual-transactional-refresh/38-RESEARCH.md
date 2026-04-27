# Phase 38: Observable Link + Manual Transactional Refresh - Research

**Researched:** 2026-04-27  
**Domain:** Flat JSONL anchor observability + manual transactional refresh in ECS/ImGui workflow  
**Confidence:** HIGH

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- **D-01:** Keep Phase 36 entry/config behavior intact: flat import observer opt-in default remains OFF at import time.
- **D-02:** Keep Phase 37 hierarchy contract intact: refresh targets flat root anchors and preserves root-anchor identity semantics.
- **D-03:** Flat observer controls live in Entity Inspector when a flat root anchor is selected.
- **D-04:** Phase 38 Inspector surface is minimal: link toggle, source path, `Re-import now`, and last refresh message.
- **D-05:** Manual refresh runs in background mode without the modal flat-import progress popup.
- **D-06:** Refresh status is surfaced in Inspector only (state + last result message), not global status/toast.
- **D-07:** Only one in-flight refresh per anchor is allowed; repeated trigger while running is ignored.
- **D-08:** On success, preserve the same root anchor entity and replace only its imported subtree.
- **D-09:** On failure, keep the previous subtree untouched and report failure.
- **D-10:** If selected entity is inside replaced subtree on successful refresh, move selection to the root anchor.
- **D-11:** Anchors imported with link OFF can be linked later from Inspector by choosing a source file.
- **D-12:** Replay settings (scale/rotation/shift/colour mode/mesh mode) remain locked to original import values in Phase 38 (no post-import editing yet).
- **D-13:** Relinking to a different source updates root label metadata to new file stem/path.

### the agent's Discretion
- Exact ECS component shape for flat-anchor refresh metadata, provided D-11/D-12 behavior is preserved.
- Exact background worker/tick orchestration for manual refresh, provided D-05/D-07 transactional behavior is preserved.
- Exact Inspector microcopy/layout, provided D-04/D-06 signal clarity is preserved.

### Deferred Ideas (OUT OF SCOPE)
- Automatic observer debounce/retry/auto-disable loop remains Phase 39 scope.
- Post-import replay-settings editing remains deferred beyond Phase 38.
- Cross-refresh interaction coherence/performance hardening remains Phase 40 scope.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| OBSF-01 | User can opt in or opt out of file observability during flat import, with default set to OFF. | Flat import UI already defaults OFF (`jsonl_flat_link_file_for_refresh=false`), preserve and test contract. |
| OBSF-02 | Imported anchor stores source-link metadata and replay settings required for refresh. | Use persisted component on flat root (extend/reuse `JsonlObserverComp`) and wire serializer for flat roots. |
| OBSF-03 | User can trigger manual refresh to re-import source data into the same anchor. | Add inspector-driven background refresh runner bound to selected flat root anchor. |
| OBSF-05 | Refresh is transactional: commit on success, preserve last-good anchor content on failure. | Use staging import + commit swap pattern (only replace root children after successful stage). |
| PERF-02 | Flat refresh path avoids sketch/script overhead and operates on anchor-subtree replacement semantics. | Reuse `jsonl_import_job` flat pipeline; no sketch comp, no script reemit, no solver path. |
</phase_requirements>

## Summary

Phase 38 should be implemented as a **flat-root metadata + staged swap refresh** feature, not as sketch observer reuse. Current sketch observer helpers (`jsonl_observer_manual_reparse`, `jsonl_sketch_reparse_transactional`) are sketch-specific and call sketch metadata/script paths, which violates PERF-02 for flat imports.

Best-fit architecture is: keep existing flat import pipeline (`jsonl_import_job`) for staging, then transactionally replace the selected root anchor’s subtree only after staging succeeds. This gives commit/rollback behavior with minimal new parser logic and aligns with D-08/D-09.

Main planning risk is metadata persistence scope: current serializer only emits `jsonl_observer` when `linked==true`, and current observer component does not yet carry all replay settings needed by D-12 (notably colour mode + mesh mode). This must be addressed early (Wave 0) to avoid rewriting refresh flow later.

**Primary recommendation:** Implement manual refresh as a background per-anchor staged `jsonl_import_job` transaction that commits by subtree swap into the existing root entity.

## Project Constraints (from copilot-instructions.md)

No `./copilot-instructions.md` found.

## Standard Stack

### Core

| Library/Module | Version | Purpose | Why Standard |
|---|---:|---|---|
| `jsonl_import_job.h` | repo module | Flat JSONL parse/create/parent pipeline | Already ships chunked import and root/entry hierarchy behavior |
| `JsonlObserverComp` | repo module | Source link + replay metadata + status messages | Already serialized and inspector-facing in project patterns |
| `scene_remove_entity` + `EcsChildOf` | Flecs 4.1.4 | Subtree mutation + parent-child control | Existing hierarchy mutation contract in current ECS |
| CTest native test executables | CMake/CTest 4.3.2 (local) | Regression gates | Existing test strategy used by Phases 36-37 |

### Supporting

| Library/Module | Version | Purpose | When to Use |
|---|---:|---|---|
| `scene_serializer.h` | repo module | Persist/load observer metadata | Required for OBSF-02 durability |
| `selection.h` | repo module | Selection clear/add semantics | Required for D-10 root fallback |
| `jsonl_sketch_reparse_transactional` | repo module pattern | Transaction design reference only | Use as semantic reference, not execution path for flat anchors |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|---|---|---|
| Staged flat import + swap | Rebuild flat subtree manually from parser in new code | High duplication risk; harder to keep parity with existing flat import behavior |
| Reusing sketch manual reparse directly | Add sketch component to flat root | Violates flat/perf boundary and introduces script/solver overhead |

## Architecture Patterns

### Recommended Project Structure

```text
src/
├── jsonl_import_job.h              # extend for staging refresh support hooks
├── components/jsonl_observer_comp.h # replay metadata schema updates
├── ui/ui_entity_inspector.h        # flat root observer controls + trigger/status
├── scene_serializer.h              # persist/load updated metadata for flat roots
└── tests/                          # phase-specific flat refresh contract tests
```

### Pattern 1: Flat-root observer metadata as durable contract
**What:** Attach persisted observer metadata to flat root anchor (not sketch-only behavior).  
**When to use:** On flat import completion and on relink updates.  
**Example:** `scene_serializer.h` already round-trips `jsonl_observer` and `jsonl_import_job.h` already captures observer intent/path.

### Pattern 2: Transactional refresh via staging + commit swap
**What:** Build refreshed content in isolation, then atomically swap root children on success.  
**When to use:** Manual refresh (OBSF-03/05, PERF-02).  
**Example:** Use `jsonl_import_job` for stage build, then commit by reparenting staged children to target root and removing old subtree.

### Pattern 3: Background manual refresh runner with single in-flight guard
**What:** Per-anchor running state + non-modal ticking in frame loop/inspector draw.  
**When to use:** D-05, D-07.  
**Example:** Existing popup-only tick in `ui_scene_hierarchy.h` is not sufficient for no-popup background refresh.

### Anti-Patterns to Avoid
- **Calling sketch reparse for flat root:** triggers sketch/script flow and breaks PERF-02.
- **Deleting old subtree before stage success:** breaks transactional safety (D-09).
- **Relying on popup-tied job ticking:** won’t satisfy background refresh UX.

## Don't Hand-Roll

| Problem | Don’t Build | Use Instead | Why |
|---|---|---|---|
| Flat JSONL parser/replay pipeline | New standalone refresh parser | Existing `jsonl_import_job` staging path | Keeps import/refresh geometry behavior consistent |
| Transaction rollback bookkeeping | Custom partial rollback logic | Stage-first then commit-swap pattern | Failure cleanup becomes simple (`remove stage root`) |
| Selection tag mutation | Manual ECS tag edits everywhere | `selection_clear` + `selection_add` helpers | Preserves render/selection coherence rules |

**Key insight:** Most complexity is in safe hierarchy mutation and consistency, not parsing itself.

## Common Pitfalls

### Pitfall 1: Metadata not persisted when link is OFF
**What goes wrong:** Flat root loses refresh settings after save/load.  
**Why:** Serializer currently emits `jsonl_observer` only if `linked` is true.  
**How to avoid:** Persist component when refresh metadata exists, not only when linked.  
**Warning signs:** Anchor imported with link OFF cannot later refresh with original replay semantics after reload.

### Pitfall 2: Replay settings incomplete for D-12
**What goes wrong:** Manual refresh uses defaults instead of original import behavior.  
**Why:** Current observer component lacks colour-mode/mesh-mode replay fields.  
**How to avoid:** Extend metadata schema (and serializer/tests) before refresh implementation.  
**Warning signs:** Refresh output differs from initial import under non-default colour/mesh settings.

### Pitfall 3: “Background refresh” accidentally tied to modal popup lifecycle
**What goes wrong:** Refresh does not progress without popup open.  
**Why:** `jsonl_import_job_tick` currently executed in popup blocks in `ui_scene_hierarchy.h`.  
**How to avoid:** Add non-modal tick path with explicit in-flight state.  
**Warning signs:** Re-import button appears to do nothing unless modal opens.

## Code Examples

### Existing observer serialization gate (must be adapted for flat metadata durability)
```c
// Source: src/scene_serializer.h
if (jsonl_observer && jsonl_observer->linked) {
    json_builder_append(b, "\"jsonl_observer\": {\n");
    ...
}
```

### Existing transactional semantics reference (sketch path concept)
```c
// Source: src/jsonl_sketch_import_job.h
// success: commit complete replacement
// failure: rollback by discarding staged children and keeping last-good state
```

### Existing flat import observer contract capture
```c
// Source: src/ui/ui_scene_hierarchy.h
jsonl_import_job_set_observer_contract(&state->jsonl_flat_import_job,
                                       state->jsonl_flat_link_file_for_refresh,
                                       state->jsonl_flat_import_path);
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|---|---|---|---|
| Sketch-linked JSONL reparse | Flat anchor import path with observer contract capture | Phase 36/37 | Enables high-volume non-sketch ingest |
| Immediate overwrite refresh logic risk | Transactional stage-then-commit pattern (in sketch path) | Phase 35 | Proven safety model to adapt for flat roots |

**Deprecated/outdated for this phase:**
- Reusing sketch observer execution functions for flat refresh logic.

## Open Questions

1. **Metadata schema choice:** extend `JsonlObserverComp` vs add flat-specific component  
   - What we know: Discretion allows schema choice; serializer already supports `jsonl_observer`.  
   - What’s unclear: cleanest compatibility path for sketch + flat without field confusion.  
   - Recommendation: Extend existing component minimally for Phase 38, defer type split unless complexity spikes.

2. **Background tick ownership:** inspector vs app-level refresh system  
   - What we know: no-popup background required; single in-flight per anchor required.  
   - What’s unclear: best ownership for lifecycle when inspector closes mid-refresh.  
   - Recommendation: app-level/state-level tick ownership with inspector as trigger/status surface only.

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|---|---|---|---|---|
| cmake | Build/test validation | ✓ | 4.3.2 | — |
| ctest | Targeted contract runs | ✓ | 4.3.2 | — |
| `build-vulkan` test dir | Existing test execution flow | ✓ | — | Reconfigure build tree if missing |

**Missing dependencies with no fallback:** None  
**Missing dependencies with fallback:** None

## Validation Architecture

### Test Framework

| Property | Value |
|---|---|
| Framework | CTest + native C executable tests |
| Config file | `src/CMakeLists.txt` (`include(CTest)` + `add_test(...)`) |
| Quick run command | `ctest --test-dir build-vulkan -C Release -R "jsonl_flat_.*|jsonl_observer_state_test|jsonl_reparse_transaction_test" --output-on-failure` |
| Full suite command | `ctest --test-dir build-vulkan -C Release --output-on-failure` |

### Phase Requirements → Test Map

| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|---|---|---|---|---|
| OBSF-01 | Flat import link opt-in default OFF | contract/unit | `ctest --test-dir build-vulkan -C Release -R "jsonl_flat_import_options_contract_test|jsonl_flat_import_ui_contract_test" --output-on-failure` | ✅ |
| OBSF-02 | Flat root stores link+replay metadata durably | unit/integration | `ctest --test-dir build-vulkan -C Release -R "jsonl_flat_observer_manual_refresh_test|jsonl_observer_state_test" --output-on-failure` | ❌ Wave 0 |
| OBSF-03 | Manual refresh on same root identity | integration | `ctest --test-dir build-vulkan -C Release -R "jsonl_flat_observer_manual_refresh_test" --output-on-failure` | ❌ Wave 0 |
| OBSF-05 | Transaction commit/rollback semantics | integration | `ctest --test-dir build-vulkan -C Release -R "jsonl_flat_observer_manual_refresh_test|jsonl_reparse_transaction_test" --output-on-failure` | ❌/✅ |
| PERF-02 | Refresh avoids sketch/script path, uses subtree replace | integration/contract | `ctest --test-dir build-vulkan -C Release -R "jsonl_flat_observer_manual_refresh_test|jsonl_flat_anchor_scoped_ingest_test" --output-on-failure` | ❌/✅ |

### Sampling Rate
- **Per task commit:** targeted command above for modified tests
- **Per wave merge:** JSONL-focused suite regex
- **Phase gate:** full ctest green before `/gsd-verify-work`

### Wave 0 Gaps
- [ ] `src/tests/jsonl_flat_observer_manual_refresh_test.c` — covers OBSF-02/03/05/PERF-02
- [ ] `src/tests/jsonl_flat_observer_inspector_contract_test.c` (or extend existing UI contract test) — covers D-03/D-04/D-06/D-07 surface contracts
- [ ] `src/CMakeLists.txt` add_test wiring for new Phase 38 tests

## Sources

### Primary (HIGH confidence)
- `.planning/phases/38-observable-link-manual-transactional-refresh/38-CONTEXT.md` (locked decisions/scope)
- `.planning/REQUIREMENTS.md` (OBSF-01/02/03/05, PERF-02)
- `src/ui/ui_scene_hierarchy.h` (flat import UX, observer contract capture, popup-tied job ticking)
- `src/jsonl_import_job.h` (flat import state machine + root/entry hierarchy)
- `src/ui/ui_entity_inspector.h` (existing sketch observer control pattern)
- `src/components/jsonl_observer_comp.h` (observer metadata schema/defaults)
- `src/jsonl_observer_system.h` (sketch-only observer tick/manual reparse paths)
- `src/jsonl_sketch_import_job.h` (transactional staging/rollback reference semantics)
- `src/scene_serializer.h` (observer persistence gate/parsing)
- `src/app.c` (observer system tick integration)
- `src/tests/jsonl_reparse_transaction_test.c`, `src/tests/jsonl_observer_state_test.c`, `src/tests/jsonl_flat_*` (test baselines)

### Secondary (MEDIUM confidence)
- `AGENTS.md` (stack version notes such as cimgui snapshot naming)

### Tertiary (LOW confidence)
- None

## Metadata

**Confidence breakdown:**
- Standard stack: **HIGH** — derived from in-repo modules/vendor version macros and active test/build usage
- Architecture: **HIGH** — grounded in existing flat import and sketch transactional code paths
- Pitfalls: **HIGH** — directly observed from current code constraints/gates

**Research date:** 2026-04-27  
**Valid until:** 2026-05-27
