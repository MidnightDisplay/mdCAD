# Phase 40: Large-Dump Stability & Interaction Coherence - Context

**Gathered:** 2026-04-27
**Status:** Ready for planning

<domain>
## Phase Boundary

Phase 40 delivers large-dump runtime hardening for the flat JSONL pipeline by proving:
1) large imports/refreshes remain stable and operational, and
2) repeated refreshes preserve coherent user interaction state.

In scope:
- Stability and responsiveness behavior for large flat JSONL import/refresh workloads.
- Refresh scheduling behavior under bursty source updates.
- Selection/hierarchy/inspector coherence across repeated transactional subtree replacements.
- Verification strategy and evidence for PERF-01 and PERF-03.

Out of scope:
- New import entry flow or option surface changes (Phases 36-38 baseline).
- Core automatic observer semantics redesign (Phase 39 baseline stays in place).
- Diff/patch incremental refresh architecture (future milestone).

</domain>

<decisions>
## Implementation Decisions

### Performance gate model
- **D-01:** Use a hybrid gate for PERF-01: no lockups/crashes is the hard pass condition.
- **D-02:** Capture timing metrics for large import/refresh runs, but treat budget overruns as warnings (advisory), not hard phase failure.

### Burst refresh scheduling
- **D-03:** Enforce single-flight refresh per anchor with coalesced pending rerun semantics during bursty file-change windows.
- **D-04:** Keep queue depth effectively at one active run plus one coalesced pending rerun (no unbounded per-anchor queueing).

### Interaction coherence policy
- **D-05:** Preserve root-anchor identity as the stable interaction pivot across repeated refreshes.
- **D-06:** If a selected child is replaced by refresh, remap selection to the root anchor (not clear-all and not deep heuristic remap).
- **D-07:** Preserve hierarchy visibility/expanded-state coherence and inspector continuity for the anchor workflow.

### Verification scope
- **D-08:** Add deterministic small/medium/large fixture coverage in native C test slices for repeated refresh behavior.
- **D-09:** Require one manual stress pass with a very large JSONL dataset to confirm operational behavior under scale.
- **D-10:** Record stress timings as evidence only; do not gate completion on hard numeric SLA thresholds in this phase.

### the agent's Discretion
- Exact fixture sizes and composition (point/line/mesh ratio), as long as tiered scale behavior is represented.
- Exact instrumentation location and warning-copy format for advisory timing output.
- Whether coalesced rerun tracking is encoded directly in refresh-slot state or in observer component state, as long as D-03/D-04 guarantees are preserved.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Milestone and phase contracts
- `.planning/ROADMAP.md` — Phase 40 goal, requirements mapping, and success criteria.
- `.planning/REQUIREMENTS.md` — `PERF-01` and `PERF-03` definitions and traceability.
- `.planning/PROJECT.md` — v1.6 scope framing, constraints, and milestone rationale.
- `.planning/STATE.md` — current execution position and continuity notes.

### Upstream phase contracts to preserve
- `.planning/phases/38-observable-link-manual-transactional-refresh/38-CONTEXT.md` — transactional flat refresh and inspector behavior baseline.
- `.planning/phases/38-observable-link-manual-transactional-refresh/38-VERIFICATION.md` — validated phase-38 baseline behavior.
- `.planning/phases/39-automatic-observer-safety-loop/39-CONTEXT.md` — locked observer lifecycle/safety decisions.
- `.planning/phases/39-automatic-observer-safety-loop/39-VERIFICATION.md` — verified phase-39 behavior and guarantees.

### Feature intent and behavior references
- `docs/feature-proposal/observable-jsonl-as-sketch.md` — observer/reload interaction intent and transactional safety language to remain consistent with.

### Code anchors
- `src/jsonl_observer_system.h` — flat refresh slot lifecycle, selection fallback, observer tick scheduling.
- `src/jsonl_import_job.h` — large-file import state-machine behavior and job progression.
- `src/ui/ui_entity_inspector.h` — flat-root observer controls and refresh status surface.
- `src/ui/ui_scene_hierarchy.h` — import launch/progress integration and user interaction entrypoints.
- `src/selection.h` — selection fallback behavior primitives used during refresh replacement.
- `src/tests/jsonl_flat_observer_auto_safety_test.c` — phase-39 observer-safety regression baseline.
- `src/tests/jsonl_flat_observer_manual_refresh_test.c` — transactional refresh and selection-fallback baseline.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `jsonl_observer_request_flat_refresh(...)` + `jsonl_observer_tick_flat_refreshes(...)` already provide transactional subtree replacement execution.
- `jsonl_observer_is_flat_refresh_running(...)` gives an existing single-flight guard surface for coalescing policy extension.
- Existing flat observer tests already cover missing-path safety, auto-disable lifecycle, and manual refresh fallback; these can be extended with scale-oriented fixtures.

### Established Patterns
- Import/refresh operations are state-machine based with bounded per-frame progress and explicit status messages.
- Selection coherence currently falls back to root anchor when replaced subtree invalidates selected entities.
- Flat observer and refresh behavior is anchored in Entity Inspector controls rather than global modal flows.

### Integration Points
- Extend refresh-slot runtime policy in `src/jsonl_observer_system.h` for burst coalescing and advisory timing signals.
- Add phase-40 regression fixtures/tests in existing flat observer test targets instead of creating a parallel test stack.
- Preserve current inspector and hierarchy interaction contracts while validating visibility and selection coherence under repeated refresh loops.

</code_context>

<specifics>
## Specific Ideas

- Keep phase closure deterministic by using reproducible tiered fixtures in native tests, then layer one manual very-large-file pass for real-world confidence.
- Prefer additive hardening of existing refresh/selection paths over introducing a new pipeline branch.

</specifics>

<deferred>
## Deferred Ideas

- Hard numeric SLA gating for import/refresh latency across all machines/backends.
- Incremental diff/patch refresh strategy for very large logs.
- Cross-platform backend performance parity matrix beyond the active native development gates.

</deferred>

---

*Phase: 40-large-dump-stability-interaction-coherence*
*Context gathered: 2026-04-27*
