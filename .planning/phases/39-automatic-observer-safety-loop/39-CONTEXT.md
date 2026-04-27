# Phase 39: Automatic Observer Safety Loop - Context

**Gathered:** 2026-04-27
**Status:** Ready for planning

<domain>
## Phase Boundary

Phase 39 delivers automatic refresh safety behavior for **flat JSONL observer roots** created in Phases 36-38.

In scope:
- Automatic refresh attempts when flat-root observer link + observe state are enabled.
- Debounce/retry/auto-disable safety behavior during unstable file-write windows.
- Observer state/UI behavior required to operate this loop safely.

Out of scope:
- New import entry flows (Phase 36).
- Flat-root hierarchy/ingest model changes (Phase 37).
- Manual transactional refresh engine changes (Phase 38 baseline remains).
- Large-scene responsiveness and repeated-refresh interaction hardening (Phase 40).

</domain>

<decisions>
## Implementation Decisions

### Auto-refresh opt-in lifecycle
- **D-01:** For flat roots, auto-observe starts immediately when link is active and a source path is present.
- **D-02:** Turning `Link file for refresh` OFF stops observing immediately but keeps the stored source path for quick re-enable.
- **D-03:** Flat-root inspector must expose a separate `Observe automatically` toggle next to link state (observe is not implicit-only).
- **D-04:** If observe is ON but source path is empty/invalid, runtime stays idle and surfaces a warning; this state must not consume retry budget.

### Carry-forward safety defaults (from prior phases)
- **D-05:** Keep debounce/retry/auto-disable safety semantics aligned with established observer behavior from Phase 35 unless explicitly overridden by this phase plan.
- **D-06:** Manual `Re-import now` remains available even when automatic observe flow is disabled by safety behavior.
- **D-07:** Automatic safety loop applies to flat observer roots only (non-sketch entities with `JsonlObserverComp`), preserving sketch observer path behavior.

### the agent's Discretion
- Exact inspector microcopy/layout for link/observe/safety status, as long as D-01..D-04 remain explicit.
- Whether interval/retry controls are shown directly in Phase 39 flat inspector or preserved as internal defaults, as long as D-05 behavior is enforced and observable.
- In-flight coalescing policy when file changes during active flat refresh, provided safety and transactional guarantees remain intact.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Milestone and phase contracts
- `.planning/ROADMAP.md` — Phase 39 goal, dependency chain, and success criteria.
- `.planning/REQUIREMENTS.md` — `OBSF-04` requirement plus milestone traceability.
- `.planning/PROJECT.md` — v1.6 milestone framing and constraints.
- `.planning/STATE.md` — active continuity and current phase position.

### Upstream phase decisions to preserve
- `.planning/phases/35-observable-jsonl-as-sketch-import-with-optional-live-file-observer/35-CONTEXT.md` — established observer debounce/retry/auto-disable precedent.
- `.planning/phases/38-observable-link-manual-transactional-refresh/38-CONTEXT.md` — flat-root observer/manual refresh contract baseline.
- `.planning/phases/38-observable-link-manual-transactional-refresh/38-VERIFICATION.md` — validated Phase 38 runtime/inspector behavior baseline.

### Code anchors
- `src/jsonl_observer_system.h` — current sketch observer loop and flat manual refresh runtime APIs.
- `src/components/jsonl_observer_comp.h` — observer persisted fields (`observe_enabled`, `interval_ms`, retry state, source metadata).
- `src/ui/ui_entity_inspector.h` — current flat observer controls and sketch observer control precedent.
- `src/jsonl_import_job.h` — observer metadata capture at flat import completion.
- `src/scene_serializer.h` — observer serialization/deserialization contract durability.
- `src/app.c` — observer system/frame tick integration point.
- `src/tests/jsonl_flat_observer_manual_refresh_test.c` — flat observer transactional baseline tests.
- `src/tests/jsonl_flat_observer_inspector_contract_test.c` — flat observer inspector surface contract baseline.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `jsonl_observer_tick_one(...)` already implements debounce/retry/auto-disable safety logic for sketch observers.
- `jsonl_observer_request_flat_refresh(...)` and `jsonl_observer_tick_flat_refreshes(...)` provide flat-root transactional refresh execution.
- `JsonlObserverComp` already persists all required safety-loop state fields and message history.

### Established Patterns
- Observer workflows use explicit messages with rolling history (`info/warn/error`) instead of silent fallback.
- Refresh paths are transactional: commit on success, preserve last-good content on failure.
- Runtime tick orchestration is centralized in `app.c` frame loop.

### Integration Points
- Extend observer tick logic in `jsonl_observer_system.h` from sketch-only query to include flat observer roots.
- Extend flat inspector controls in `ui_entity_inspector.h` with explicit observe state and safety feedback controls.
- Keep serializer roundtrip compatibility for all observer fields touched by automatic loop behavior.

</code_context>

<specifics>
## Specific Ideas

- Preserve Phase 38 manual flow while adding low-friction auto safety loop for linked flat roots.
- Keep auto behavior operationally visible and controllable in the same flat-root inspector surface.

</specifics>

<deferred>
## Deferred Ideas

- Large-scale repeated-refresh interaction/performance hardening remains Phase 40.
- Any diff/patch incremental refresh strategy remains outside v1.6 scope.

</deferred>

---

*Phase: 39-automatic-observer-safety-loop*
*Context gathered: 2026-04-27*
