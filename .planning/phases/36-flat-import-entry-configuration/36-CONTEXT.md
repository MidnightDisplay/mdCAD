# Phase 36: Flat Import Entry & Configuration - Context

**Gathered:** 2026-04-27
**Status:** Ready for planning

<domain>
## Phase Boundary

Phase 36 delivers the user-facing entry path and import-configuration contract for a new flat JSONL large-dump workflow.

In scope:
- Dedicated File/import action for flat JSONL large dumps
- Import dialog contract and defaults for this new path
- Observer opt-in capture at import time (default OFF), wired for forward compatibility
- Import start behavior contract (job start/progress model)

Out of scope for this phase:
- Flat anchor/entity ingestion implementation details (Phase 37)
- Transactional manual refresh engine (Phase 38)
- Auto observer refresh loop (Phase 39)
- Large-dump hardening closure (Phase 40)
</domain>

<decisions>
## Implementation Decisions

### Entry point and naming
- **D-01:** Add a third explicit import action: `Import JSONL (Flat Large Dump)...`.
- **D-02:** Keep existing `Import JSONL Geometry Log...` and `Import JSONL as Sketch...` actions unchanged; do not replace either flow.

### Dialog contract
- **D-03:** Mirror current JSONL options in the new flat dialog: units, import colours/default colour, shift-to-CoM, XYZ rotation, and mesh mode when mesh data is present.
- **D-04:** Reuse existing progress-popup style and status messaging patterns already used by JSONL import.

### Observer opt-in UX
- **D-05:** Show `Link file for refresh (optional)` in the flat dialog.
- **D-06:** Observer link default is OFF for the flat workflow.
- **D-07:** Persist the import-time observer choice and settings in a forward-compatible shape for Phase 38 refresh behavior.

### Import-start behavior
- **D-08:** Submit starts the existing chunked asynchronous job path for non-trivial files and preserves tiny-file synchronous fast path.
- **D-09:** Keep progress popup behavior for asynchronous imports; do not switch to fully blocking synchronous import.

### the agent's Discretion
- Exact wording/tooltip copy for the new menu item and observer toggle, provided intent remains explicit for "large dump" and "optional link".
- Exact UI control grouping/order inside the popup, provided D-03 option coverage remains complete.
</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Milestone and phase contracts
- `.planning/ROADMAP.md` — canonical phase 36 boundary, dependencies, and success criteria.
- `.planning/REQUIREMENTS.md` — v1.6 requirement IDs mapped to phases.
- `.planning/PROJECT.md` — v1.6 goal framing and platform constraints.
- `.planning/STATE.md` — active milestone continuity and current position.

### Upstream behavior to preserve
- `.planning/phases/35-observable-jsonl-as-sketch-import-with-optional-live-file-observer/35-CONTEXT.md` — observer defaults/safety and UI ergonomics precedent.

### Code anchors
- `src/ui/ui_scene_hierarchy.h` — existing file menu actions, JSONL dialogs, and progress popup flow.
- `src/jsonl_import_job.h` — chunked import start/tick/reset contract and sync-threshold behavior.
- `src/jsonl_loader.h` — quick-scan and parse state used by import option dialogs.
- `src/components/jsonl_observer_comp.h` — observer metadata shape and defaults used for persistence.
- `src/jsonl_observer_system.h` — retry/debounce/auto-disable semantics to carry forward.
</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `ui_scene_hierarchy.h` already has parallel JSONL and JSONL-as-sketch browser + popup flows that can be copied for a third import action.
- `jsonl_import_job_start()` + progress popup lifecycle is already integrated and suitable for the flat-large entry path.
- Existing JSONL option controls (units, colors, transforms, mesh mode) are already wired and can be mirrored.

### Established Patterns
- Import flows use file-browser selection -> quick scan -> modal options popup -> async progress popup for large jobs.
- Entity-creation jobs are chunked and track status/progress string for UI.
- Observer metadata and message-history conventions exist via `JsonlObserverComp`.

### Integration Points
- Add new File action and popup state fields in `ui_scene_hierarchy_state_t`.
- Reuse import-option-to-job-start conversion path (scale/rotation/etc.) from existing JSONL import.
- Capture observer opt-in data at import submit boundary for later phase usage.
</code_context>

<specifics>
## Specific Ideas

- Keep the new flow explicit as a blend of existing flat import and observer linkage without sketch conversion.
- Optimize user guidance around very large geometry dumps by naming the entry path for that scenario.
- Preserve existing import UX muscle memory by mirroring current option controls.
</specifics>

<deferred>
## Deferred Ideas

- Transactional anchor refresh implementation details (Phase 38).
- Automatic observer polling/retry loop behavior integration (Phase 39).
- Large-scale responsiveness hardening and interaction coherence gates (Phase 40).
</deferred>

---

*Phase: 36-flat-import-entry-configuration*
*Context gathered: 2026-04-27*
