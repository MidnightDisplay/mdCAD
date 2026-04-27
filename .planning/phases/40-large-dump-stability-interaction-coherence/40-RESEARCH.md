# Phase 40: large-dump-stability-interaction-coherence - Research

**Researched:** 2026-04-27  
**Scope:** PERF-01 and PERF-03 for flat JSONL import/refresh runtime

## Inputs reviewed

- `.planning/phases/40-large-dump-stability-interaction-coherence/40-CONTEXT.md`
- `.planning/ROADMAP.md`
- `.planning/REQUIREMENTS.md`
- `.planning/STATE.md`
- `src/jsonl_observer_system.h`
- `src/jsonl_import_job.h`
- `src/ui/ui_entity_inspector.h`
- `src/ui/ui_scene_hierarchy.h`
- `src/selection.h`
- `src/tests/jsonl_flat_observer_auto_safety_test.c`
- `src/tests/jsonl_flat_observer_manual_refresh_test.c`

## Findings

### 1) Runtime base is already suitable for phase hardening

- Flat refresh is already transactional (`commit` on success, preserve last-good on failure) in `jsonl_observer_system.h`.
- Import execution is already chunked via `jsonl_import_job` state-machine paths, reducing UI-stall risk compared to monolithic import.
- Observer tick + flat refresh tick are already frame-integrated in `src/app.c`.

### 2) Main gap for burst updates

- Current behavior rejects new refresh requests while one is in flight (`jsonl_observer_request_flat_refresh` short-circuits if slot exists).
- Phase-40 decision D-03/D-04 requires coalescing burst updates into one pending rerun, not drop-all and not queue-many.
- Best-fit extension: add a per-root coalesced `pending_rerun` flag in flat refresh slot state and consume once after active completion.

### 3) Interaction coherence baseline exists and should be extended

- Selection fallback to root on replaced-descendant already exists in `jsonl_observer_commit_flat_refresh(...)`.
- Root-anchor identity is preserved on refresh and should remain the stable interaction pivot.
- Phase 40 should explicitly verify repeated-cycle behavior (not one-shot) for:
  - selection remap consistency,
  - hierarchy continuity,
  - inspector continuity.

### 4) Verification should stay deterministic + one manual large stress pass

- Existing C tests are the right base; extend them with tiered fixture sizes.
- Keep advisory timing evidence (warnings/info) as locked in D-02/D-10.
- Avoid hard numeric pass/fail thresholds in this phase.

## Recommended implementation direction

1. Extend flat refresh slot runtime (`src/jsonl_observer_system.h`) for single-flight + one coalesced pending rerun.
2. Keep current transactional commit flow; only add bounded reschedule behavior.
3. Preserve root-stable interaction semantics and enforce/verify descendant->root remap under repeated replacement cycles.
4. Expand current flat observer tests rather than creating a parallel testing stack.

## Suggested planner decomposition

- **Plan 40-01 (runtime hardening):**
  - Add coalesced pending-rerun behavior and bounded scheduling.
  - Add advisory timing capture/reporting hooks.
- **Plan 40-02 (verification + coherence):**
  - Add deterministic small/medium/large fixture tests for repeated refresh.
  - Add explicit repeated-cycle interaction coherence assertions.
  - Define manual very-large stress protocol artifact.

## Risks and mitigations

| Risk | Impact | Mitigation |
|---|---|---|
| Burst changes still cause thrash | High | Enforce one active + one pending rerun max per root |
| Coherence regressions over repeated replacements | High | Add repeated-cycle tests (not one-shot) for selection/hierarchy/inspector behavior |
| Timing evidence too noisy | Medium | Advisory-only timings (no hard SLA gate in phase) |

## Validation Architecture

### Test Infrastructure

| Property | Value |
|---|---|
| Framework | CTest + native C test binaries |
| Config file | `src/CMakeLists.txt` |
| Quick run command | `ctest --test-dir build-vulkan -C Release -R "jsonl_flat_observer_(manual_refresh_test|auto_safety_test|inspector_contract_test)" --output-on-failure` |
| Full suite command | `ctest --test-dir build-vulkan -C Release --output-on-failure` |

### Requirement mapping

| Requirement | Strategy |
|---|---|
| PERF-01 | Tiered fixture runs (small/medium/large), prove no lock/crash, capture advisory timings |
| PERF-03 | Repeated-refresh coherence checks: root identity stability, descendant->root selection remap, inspector/hierarchy continuity |

### Wave-0 test gaps for this phase

- Add burst coalescing regression coverage (single active + one pending rerun).
- Add repeated-cycle coherence coverage (N refresh loops) in existing flat observer test targets.
- Add phase artifact for manual very-large-file stress evidence.
