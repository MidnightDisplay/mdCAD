# Phase 42: Refresh & Teardown Stability - Context

**Gathered:** 2026-05-05
**Status:** Ready for planning

<domain>
## Phase Boundary

Phase 42 stabilizes the linked large flat JSONL lifecycle after the initial import baseline already exists: later observer-driven refreshes, manual `Re-import now`, and linked-root deletion must converge cleanly without geometry collapse, late churn, or orphaned render state.

In scope:
- Linked large flat JSONL refresh after the Phase 41 baseline has already been established.
- Both observer-driven auto-reload and manual `Re-import now` for linked flat roots.
- Steady-state cleanup after refresh so rendered coverage, hierarchy counts, and slot-buffer occupancy stop changing once refresh settles.
- Delete cleanup for linked flat roots after prior refresh activity, including delete while a refresh is actively running.
- Safety behavior when refresh commit/cleanup invariants fail late in the process.
- Automated and manual acceptance evidence for the refresh/delete lifecycle using the real `lamp_11.jsonl` file.

Out of scope:
- The initial linked import convergence fixed in Phase 41.
- New incremental diff/patch refresh architecture.
- New observer/debug UX surfaces beyond the existing inspector/status-message/debug-panel patterns.
- Behavior for unrelated non-linked or sketch JSONL import flows.

</domain>

<decisions>
## Implementation Decisions

### Refresh convergence contract
- **D-01:** After any linked auto-reload or completed manual `Re-import now`, the settled slot-buffer state must converge back to the exact live entity footprint for that import; bounded leftover slack is not acceptable.
- **D-02:** Linked file auto-reloads and manual `Re-import now` must obey the same settle/cleanup contract.
- **D-03:** Temporary slot-buffer overlap while a refresh is actively in flight is acceptable, but the final settled state must return to exact 1:1 occupancy with no cumulative growth across successive refreshes.

### Teardown behavior
- **D-04:** Deleting a linked import root after one or more settled refreshes must remove all related visible geometry and retire that import's slot-buffer occupancy in the same user action.
- **D-05:** If the user deletes a linked import root while its refresh is actively running, the app should cancel any in-flight or staged refresh work and complete the delete immediately with no leftovers.
- **D-06:** If delete cancels an in-flight refresh, undo restores only the last committed import state and never resurrects the canceled staged refresh payload.

### Cleanup-failure safety policy
- **D-07:** If a linked refresh builds staged content but refresh commit or cleanup invariants fail, keep the last committed import content visible and discard the staged result.
- **D-08:** After a cleanup/commit anomaly, automatic observe should disable itself and surface a warning, but explicit manual `Re-import now` attempts should remain available.

### Acceptance gate
- **D-09:** Phase 42 closes only after all four lifecycle scenarios are covered: linked auto-reload, repeated manual `Re-import now`, delete after settled refresh history, and delete while refresh is active.
- **D-10:** Phase 42 requires both deterministic automated regressions for the key refresh/delete paths and a real-file `lamp_11.jsonl` manual pass with viewport, Scene Hierarchy, and slot-buffer evidence.

### the agent's Discretion
- Exact internal cleanup/cancel mechanism, provided D-01 through D-08 are enforced and same-root transactional refresh semantics from earlier phases are preserved.
- Exact warning copy and UI surface, provided the user can clearly see that automatic observe was disabled for safety after a cleanup anomaly.
- Exact automated regression decomposition and manual checklist field design, provided D-09 and D-10 are proven directly.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Milestone and phase contracts
- `.planning/ROADMAP.md` — Phase 42 goal, dependencies, and success criteria.
- `.planning/REQUIREMENTS.md` — `OBSF-07`, `OBSF-08`, `PERF-04`, and `PERF-05`.
- `.planning/PROJECT.md` — v1.7 milestone framing and current refresh/delete focus.
- `.planning/STATE.md` — current active phase and continuity notes.

### Upstream phase contracts to preserve
- `.planning/phases/36-flat-import-entry-configuration/36-CONTEXT.md` — flat-import entry contract and `Link file for refresh (optional)` behavior.
- `.planning/phases/38-observable-link-manual-transactional-refresh/38-CONTEXT.md` — same-root transactional refresh and observer metadata contract.
- `.planning/phases/39-automatic-observer-safety-loop/39-CONTEXT.md` — auto-observe lifecycle and safety defaults.
- `.planning/phases/40-large-dump-stability-interaction-coherence/40-CONTEXT.md` — large-dump single-flight/coalesced rerun guardrails and interaction coherence.
- `.planning/phases/41-linked-import-convergence/41-CONTEXT.md` — initial linked-import baseline decision that Phase 42 builds on.
- `.planning/phases/41-linked-import-convergence/41-MANUAL-CHECKLIST.md` — real-file convergence evidence plus the deferred Phase 42 observations that motivated this phase.
- `.planning/phases/41-linked-import-convergence/41-VERIFICATION.md` — verified closeout boundary between Phase 41 and Phase 42.

### Feature docs and user intent
- `docs/feature-proposal/observable-jsonl-as-sketch.md` — observable JSONL refresh intent and transactional safety framing to preserve.
- `.planning/phases/42-refresh-teardown-stability/42-DISCUSSION-LOG.md` — the exact user choices locked during discuss-phase for this phase.

### Code anchors
- `src/jsonl_observer_system.h` — flat refresh request, coalesced rerun handling, staged commit/abort/reset, and observer safety messaging.
- `src/jsonl_import_job.h` — chunked flat import lifecycle reused by observer-driven refresh.
- `src/components/jsonl_observer_comp.h` — observer source-path, linkage, observe-enabled, and message-history state.
- `src/ecs/ecs_scene.h` — `scene_remove_entity(...)` teardown path used by both staged-refresh cleanup and normal deletion.
- `src/ui/ui_entity_inspector.h` — `Observe automatically`, `Re-import now`, and refresh-in-progress UI gating.
- `src/ui/ui_scene_hierarchy.h` — flat import entry path and linked-root delete surface.
- `src/app.c` — keyboard delete path that currently removes selected roots directly.
- `src/ui/ui_slot_buffer_debug.h` — slot-buffer debug visibility used for manual acceptance.
- `src/gpu/instance_buffer.h` — slot allocation/free semantics and debug slot ownership mapping.
- `src/tests/jsonl_flat_observer_auto_safety_test.c` — observer-driven linked flat lifecycle test coverage.
- `src/tests/jsonl_flat_observer_manual_refresh_test.c` — manual refresh and flat linked import regression patterns.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `jsonl_observer_request_flat_refresh(...)` already enforces a single active flat refresh slot per root and can queue one coalesced rerun.
- `jsonl_observer_commit_flat_refresh(...)`, `jsonl_observer_abort_flat_refresh_stage(...)`, and `jsonl_observer_reset_flat_refresh_slot(...)` already centralize staged refresh cleanup behavior.
- `scene_remove_entity(...)` is already the common teardown hook used by both observer cleanup and UI delete flows.
- `JsonlObserverComp` already provides message history plus `linked` / `observe_enabled` state for warning-and-disable safety behavior.
- `ui_slot_buffer_debug` already exposes the exact occupancy symptoms the user observed and can anchor the manual pass again.

### Established Patterns
- Linked flat refresh remains transactional: staged geometry is only adopted on successful commit and last-good content is preserved on failure.
- Observer safety behavior is explicit and message-driven rather than silent.
- Large-dump refresh behavior favors bounded single-flight work with at most one coalesced follow-up rerun.
- The Inspector disables manual `Re-import now` while a refresh is already running, but delete paths currently still fall through to `scene_remove_entity(...)` directly.

### Integration Points
- `jsonl_observer_system.h` is the main choke point for settled refresh convergence, staged cleanup, rerun scheduling, and anomaly handling.
- `scene_remove_entity(...)` must fully retire committed and staged refresh-owned entities so delete semantics match D-04 through D-06.
- The acceptance proof must connect runtime behavior to three visible signals: viewport coverage, Scene Hierarchy totals, and slot-buffer occupancy.

</code_context>

<specifics>
## Specific Ideas

- The real-file manual acceptance anchor remains `C:\Users\RodionRadchenko\source\repos\ParkerSteel-GenerateDimensionedDrawing\ParkerSteel.Drawing.Test\bin\Debug\net8.0-windows\lamp_11.jsonl`.
- The user explicitly does not accept settled refreshes that leave the slot buffers inflated to 2x the live footprint or that keep growing toward the `2097152` capacity limit across repeated `Re-import now` runs.
- Auto-reload and manual `Re-import now` should feel behaviorally identical once they settle: no geometry collapse, no tail-only remnants, and no late observer churn changing counts after the refresh is done.
- Delete is part of the user-facing quality bar for this phase, not a follow-up polish item: after refresh history, and even during an active refresh, deleting the root should leave no dangling line/point remnants in the viewport or debug state.

</specifics>

<deferred>
## Deferred Ideas

- Incremental or diff-based refresh architecture remains future work beyond this phase.
- New observer/debug UX beyond the existing inspector/status/debug surfaces remains out of scope.
- Broader slot-buffer architecture changes are only in scope if they are necessary to satisfy D-01 through D-10; speculative redesign beyond that remains deferred.

</deferred>

---

*Phase: 42-refresh-teardown-stability*
*Context gathered: 2026-05-05*
