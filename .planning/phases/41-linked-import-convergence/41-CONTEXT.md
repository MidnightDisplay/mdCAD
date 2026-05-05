# Phase 41: Linked Import Convergence - Context

**Gathered:** 2026-05-05
**Status:** Ready for planning

<domain>
## Phase Boundary

Phase 41 stabilizes the initial linked flat-large JSONL import so the first committed result becomes the lasting baseline: full geometry remains visible and Scene Hierarchy totals converge once import is done.

In scope:
- The initial linked import path from `Import JSONL (Flat Large Dump)` when `Link file for refresh (optional)` is enabled.
- Observer arming behavior immediately after the first successful import.
- Steady-state rules for viewport geometry and Scene Hierarchy totals after that import commits.
- Safe fallback if observer baseline arming cannot be established after a successful import.
- Regression proof for the real large-file scenario and slot-buffer/hierarchy invariants.

Out of scope:
- Refresh behavior after a later real file change (Phase 42).
- Delete/teardown cleanup for linked flat imports (Phase 42).
- New diff/patch incremental refresh architecture.

</domain>

<decisions>
## Implementation Decisions

### Observer arming
- **D-01:** After a successful linked flat import, stamp the just-imported file as the observer baseline before automatic observe can treat anything as changed.
- **D-02:** The initial linked import must not auto-refresh itself; automatic observe waits for a later real external file change.

### Steady-state convergence
- **D-03:** Once the first linked import commits, the viewport result and Scene Hierarchy totals stay stable until the source file actually changes later.

### Safe fallback
- **D-04:** If observer baseline arming fails after a successful import, keep the imported geometry and source path, turn automatic observe OFF, and surface a warning instead of rolling the import back.

### Acceptance anchor
- **D-05:** Phase 41 closes only with deterministic automated regression coverage plus manual confirmation against `C:\Users\RodionRadchenko\source\repos\ParkerSteel-GenerateDimensionedDrawing\ParkerSteel.Drawing.Test\bin\Debug\net8.0-windows\lamp_11.jsonl`, including slot-buffer and Scene Hierarchy convergence checks.

### the agent's Discretion
- Exact source-state stamping hook location, provided D-01 and D-02 behavior is guaranteed on the initial linked import path.
- Exact warning copy and UI surface for D-04, as long as the linked root clearly communicates that automatic observe was disabled for safety.
- Exact automated fixture composition and invariant helpers, provided they cover initial linked-import convergence rather than later change-driven refresh behavior.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Milestone and phase contracts
- `.planning/ROADMAP.md` — Phase 41 goal, dependency chain, and success criteria.
- `.planning/REQUIREMENTS.md` — `FIMP-05` and `FIMP-06` requirement definitions plus v1.7 traceability.
- `.planning/PROJECT.md` — v1.7 milestone framing, constraints, and the large-file regression summary.
- `.planning/STATE.md` — active milestone position and current continuity notes.

### Upstream phase contracts to preserve
- `.planning/phases/36-flat-import-entry-configuration/36-CONTEXT.md` — import entry path and `Link file for refresh (optional)` contract, including default-OFF linkage.
- `.planning/phases/38-observable-link-manual-transactional-refresh/38-CONTEXT.md` — same-root transactional refresh baseline and observer metadata contract.
- `.planning/phases/39-automatic-observer-safety-loop/39-CONTEXT.md` — auto-observe safety semantics and explicit flat-root observe lifecycle.
- `.planning/phases/40-large-dump-stability-interaction-coherence/40-CONTEXT.md` — bounded coalescing and large-dump interaction coherence guardrails.

### Feature docs and design intent
- `docs/feature-proposal/observable-jsonl-as-sketch.md` — observer/reparse intent and transactional safety language to stay aligned with.

### Code anchors
- `src/jsonl_import_job.h` — initial flat import lifecycle, observer component attachment, and final parenting/commit path.
- `src/jsonl_observer_system.h` — flat observer arming, source-change detection, staged refresh commit, and in-flight slot lifecycle.
- `src/jsonl_sketch_import_job.h` — `jsonl_observer_stamp_source_state(...)` and `jsonl_observer_source_changed(...)` source-baseline helpers.
- `src/components/jsonl_observer_comp.h` — observer defaults (`observe_enabled`, source-state fields, message history).
- `src/ui/ui_scene_hierarchy.h` — flat-large import dialog state and progress-popup entry path.
- `src/ui/ui_entity_inspector.h` — flat-root link/observe controls and manual `Re-import now` surface.
- `src/ui/ui_slot_buffer_debug.h` — slot-buffer visualization used in the manual repro and acceptance pass.
- `src/gpu/instance_buffer.h` — slot allocation/free semantics and debug slot-to-entity mapping that Phase 41 convergence must preserve.
- `src/tests/jsonl_flat_observer_auto_safety_test.c` — existing auto-observe test baseline and tiered fixture patterns.
- `src/tests/jsonl_flat_observer_manual_refresh_test.c` — existing transactional flat refresh test helpers and geometry-count invariants.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `jsonl_import_job_t` already provides chunked parse/create/parenting lifecycle with status/progress tracking for flat imports.
- `JsonlObserverComp` already stores the source path, enable state, and source-state stamp fields needed to differentiate a fresh baseline from a real later change.
- `jsonl_observer_stamp_source_state(...)` and `jsonl_observer_source_changed(...)` already exist and can be reused instead of inventing a second baseline mechanism.
- Existing flat observer tests already provide helper patterns for geometry counts, refresh-idle draining, and tiered file fixtures.

### Established Patterns
- Flat observer behavior prefers transactional commit-on-success rather than partial visible mutation.
- Observer safety behavior is explicit and message-driven: keep last-good content, warn clearly, and auto-disable observe when safety policy is exhausted.
- Large-dump hardening prefers bounded single-flight/coalesced rerun semantics instead of unbounded background work.
- UI status is localized to existing import/inspector surfaces rather than introducing new global notification systems.

### Integration Points
- The initial import completion path in `jsonl_import_job.h` is the natural place to stamp or defer observer baseline state for linked flat imports.
- The flat observer tick path in `jsonl_observer_system.h` enforces whether a newly imported root can trigger a same-file auto-refresh.
- Scene Hierarchy totals and slot-buffer debug output provide the two most visible convergence signals for this phase.

</code_context>

<specifics>
## Specific Ideas

- The provided real-world acceptance file is `C:\Users\RodionRadchenko\source\repos\ParkerSteel-GenerateDimensionedDrawing\ParkerSteel.Drawing.Test\bin\Debug\net8.0-windows\lamp_11.jsonl`.
- The manual success case is not just "no crash": after the first linked import commits, the visible geometry stays complete, Scene Hierarchy totals stop climbing, and slot-buffer debug no longer shows an early-slot chunk disappearing.
- This phase should prove convergence on the first linked import path only; later refresh-after-change and delete cleanup remain separate follow-up work in Phase 42.

</specifics>

<deferred>
## Deferred Ideas

- Refresh-after-real-file-change behavior and replacement cleanup remain Phase 42.
- Linked-root delete and teardown cleanup remain Phase 42.
- Incremental diff/patch refresh architecture remains future-scope work beyond v1.7.

</deferred>

---

*Phase: 41-linked-import-convergence*
*Context gathered: 2026-05-05*
