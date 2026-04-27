# Phase 38: Observable Link + Manual Transactional Refresh - Context

**Gathered:** 2026-04-27
**Status:** Ready for planning

<domain>
## Phase Boundary

Phase 38 adds observer-link metadata and manual refresh behavior for flat JSONL anchors created in Phase 37.

In scope:
- Flat-root observer controls and link management UX
- Persisted source-link + replay metadata on flat root anchors
- Manual refresh into the same root anchor identity
- Transactional subtree replacement semantics (commit on success, preserve last-good on failure)

Out of scope for this phase:
- Automatic observer refresh loop (Phase 39)
- Debounce/retry/auto-disable automation for observer polling (Phase 39)
- Large-scale interaction/performance hardening across repeated refreshes (Phase 40)
</domain>

<decisions>
## Implementation Decisions

### Carry-forward constraints
- **D-01:** Keep Phase 36 entry/config behavior intact: flat import observer opt-in default remains OFF at import time.
- **D-02:** Keep Phase 37 hierarchy contract intact: refresh targets flat root anchors and preserves root-anchor identity semantics.

### Observer controls placement and surface
- **D-03:** Flat observer controls live in Entity Inspector when a flat root anchor is selected.
- **D-04:** Phase 38 Inspector surface is minimal: link toggle, source path, `Re-import now`, and last refresh message.

### Manual refresh execution and UX
- **D-05:** Manual refresh runs in background mode without the modal flat-import progress popup.
- **D-06:** Refresh status is surfaced in Inspector only (state + last result message), not global status/toast.
- **D-07:** Only one in-flight refresh per anchor is allowed; repeated trigger while running is ignored.

### Transaction semantics
- **D-08:** On success, preserve the same root anchor entity and replace only its imported subtree.
- **D-09:** On failure, keep the previous subtree untouched and report failure.
- **D-10:** If selected entity is inside replaced subtree on successful refresh, move selection to the root anchor.

### Link lifecycle and metadata
- **D-11:** Anchors imported with link OFF can be linked later from Inspector by choosing a source file.
- **D-12:** Replay settings (scale/rotation/shift/colour mode/mesh mode) remain locked to original import values in Phase 38 (no post-import editing yet).
- **D-13:** Relinking to a different source updates root label metadata to new file stem/path.

### the agent's Discretion
- Exact ECS component shape for flat-anchor refresh metadata, provided D-11/D-12 behavior is preserved.
- Exact background worker/tick orchestration for manual refresh, provided D-05/D-07 transactional behavior is preserved.
- Exact Inspector microcopy/layout, provided D-04/D-06 signal clarity is preserved.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Milestone and phase contracts
- `.planning/ROADMAP.md` — Phase 38 boundary, dependency chain, and success criteria.
- `.planning/REQUIREMENTS.md` — `OBSF-01`, `OBSF-02`, `OBSF-03`, `OBSF-05`, `PERF-02` requirements for this phase.
- `.planning/PROJECT.md` — v1.6 goals, constraints, and active milestone framing.
- `.planning/STATE.md` — current continuity and handoff position.

### Upstream phase contracts
- `.planning/phases/36-flat-import-entry-configuration/36-CONTEXT.md` — flat import entry/config and observer opt-in default OFF contract.
- `.planning/phases/36-flat-import-entry-configuration/36-VERIFICATION.md` — validated Phase 36 UI/start behavior to preserve.
- `.planning/phases/37-anchor-scoped-flat-ingest/37-CONTEXT.md` — root/entry hierarchy, naming, and selection contracts.
- `.planning/phases/37-anchor-scoped-flat-ingest/37-VERIFICATION.md` — validated Phase 37 ingest behavior baseline.
- `.planning/phases/35-observable-jsonl-as-sketch-import-with-optional-live-file-observer/35-CONTEXT.md` — observer lifecycle and transactional reparse precedents.

### Feature docs and references
- `docs/feature-proposal/observable-jsonl-as-sketch.md` — observer/reparse UX precedent and transactional safety language.

### Code anchors
- `src/ui/ui_scene_hierarchy.h` — flat import path, observer-link submit contract capture, and progress behavior.
- `src/jsonl_import_job.h` — flat import options/state machine and captured observer contract at submit.
- `src/ui/ui_entity_inspector.h` — existing sketch observer control patterns and manual reparse UX baseline.
- `src/components/jsonl_observer_comp.h` — observer metadata/message schema and defaults.
- `src/jsonl_observer_system.h` — manual/automatic observer semantics and transactional reparse orchestration patterns.
- `src/scene_serializer.h` — persisted `jsonl_observer` component storage/restore behavior.
- `src/app.c` — observer system tick integration point.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- Existing `JsonlObserverComp` already persists source path, enable state, retry metadata, and message history (`src/components/jsonl_observer_comp.h`).
- Existing serializer already round-trips `jsonl_observer` on entities (`src/scene_serializer.h`).
- Existing Inspector controls for sketch observers provide reusable control semantics and messaging patterns (`src/ui/ui_entity_inspector.h`).
- Existing flat import flow already captures observer-link intent/path into `jsonl_import_job` contract (`src/ui/ui_scene_hierarchy.h`, `src/jsonl_import_job.h`).

### Established Patterns
- Import/reparse behavior favors transactional commit-on-success with explicit failure messaging.
- Observer UX uses explicit toggles and retained recent messages rather than silent retries.
- ECS hierarchy operations use deferred/batched parent mutations and explicit `EcsChildOf` relationships.

### Integration Points
- Extend flat-anchor metadata persistence path from Phase 37 root anchors.
- Add flat-anchor Inspector controls (non-sketch path) parallel to sketch observer controls.
- Implement manual refresh engine that rebuilds anchor subtree using stored replay settings and source link.
- Keep root label contract updates in sync with relink behavior.

</code_context>

<specifics>
## Specific Ideas

- Manual refresh should be low-friction: one button in Inspector, background execution, and clear local status.
- Root anchor identity should remain stable so users retain a predictable hierarchy node over repeated refreshes.
- Link-later workflow is required for anchors initially imported with link OFF.

</specifics>

<deferred>
## Deferred Ideas

- Automatic observer debounce/retry/auto-disable loop remains Phase 39 scope.
- Post-import replay-settings editing remains deferred beyond Phase 38.
- Cross-refresh interaction coherence/performance hardening remains Phase 40 scope.

</deferred>

---

*Phase: 38-observable-link-manual-transactional-refresh*
*Context gathered: 2026-04-27*
