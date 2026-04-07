# Phase 22: Solver Trigger + Recalculate Determinism - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md - this log preserves alternatives considered.

**Date:** 2026-04-07
**Phase:** 22-solver-trigger-recalculate-determinism
**Areas discussed:** Solve trigger semantics, Recalculate pass policy, Constraint behavior contract, Migration safety

---

## Solve trigger semantics

| Option | Description | Selected |
|--------|-------------|----------|
| Run immediately on each committed mutation | Fire solve per commit without queueing | |
| Queue per sketch and flush once per frame | Coalesce requests to frame-level flush | |
| Queue with short debounce window (e.g., 50-100ms) | Delay solve briefly to absorb burst edits | ✓ |

**User's choice:** Queue with short debounce window.
**Notes:** User requested discussing all areas in order and chose debounce over immediate execution.

| Option | Description | Selected |
|--------|-------------|----------|
| Coalesce into existing pending request | One pending request absorbs incoming mutations | ✓ |
| Start a new request for each mutation | Preserve every event as separate solve request | |
| Keep only latest mutation | Drop intermediate mutation intents | |

**User's choice:** Coalesce pending requests.
**Notes:** Chosen to reduce redundant solve runs during rapid edits.

| Option | Description | Selected |
|--------|-------------|----------|
| 75ms | Moderate default debounce | |
| 50ms | Lower latency debounce | ✓ |
| 100ms | More batching, lower frequency | |

**User's choice:** 50ms default debounce.
**Notes:** User preferred lower-latency queue flush.

| Option | Description | Selected |
|--------|-------------|----------|
| Run manual recalc and clear pending queue | Manual recalc takes precedence | ✓ |
| Run manual recalc and keep pending queue | Follow-up queued pass still executes | |
| Ignore manual recalc if pending | Prevents override of queued solve | |

**User's choice:** Manual recalc runs immediately and clears queue.
**Notes:** Manual recalc is treated as authoritative override.

---

## Recalculate pass policy

| Option | Description | Selected |
|--------|-------------|----------|
| Fixed internal tolerances only | UI hides tolerances in v1.3 | |
| Expose position + angle tolerances in UI | User-adjustable tolerance policy | ✓ |
| Expose single global tolerance in UI | Simpler but less expressive | |

**User's choice:** Expose position + angle tolerances in UI.
**Notes:** User explicitly wants configurable tolerance controls surfaced.

| Option | Description | Selected |
|--------|-------------|----------|
| 10 | Default pass cap | ✓ |
| 15 | Higher cap | |
| 20 | Highest cap | |

**User's choice:** 10 pass default.
**Notes:** Matches milestone baseline expectation.

| Option | Description | Selected |
|--------|-------------|----------|
| Error + explicit max-pass diagnostic | Strong failure signal | ✓ |
| Loose + warning | Recoverable/non-fatal state | |
| Keep previous status + info diagnostic | Minimal status disruption | |

**User's choice:** Error with explicit max-pass-reached diagnostic.
**Notes:** Failure should be unambiguous and explicit.

| Option | Description | Selected |
|--------|-------------|----------|
| Clear implication immediately on success | Preserve existing successful-solve lifecycle | ✓ |
| Keep until user action | Manual dismissal required | |
| Keep one extra frame | Transitional delay before clear | |

**User's choice:** Clear immediately on success.
**Notes:** Keep parity with current clear-on-success expectations.

---

## Constraint behavior contract

| Option | Description | Selected |
|--------|-------------|----------|
| No mutation + explicit failure + implication | Strict transactional failure contract | ✓ |
| Partial nearest-fit update + warning | Best-effort geometry mutation | |
| Apply and mark Loose | Non-failing mutation with degraded status | |

**User's choice:** No mutation, explicit failure, implicated constraints.
**Notes:** Unsat behavior must remain transactional and explicit.

| Option | Description | Selected |
|--------|-------------|----------|
| Atomic commit on successful solve | One-shot visible update per solve | ✓ |
| Progressive convergence via repeated clicks | Gradual user-driven convergence | |
| Per-sketch configurable mode | Dual behavior support | |

**User's choice:** Atomic commit on successful solve.
**Notes:** Avoid gradual multi-click convergence UX.

---

## Migration safety

| Option | Description | Selected |
|--------|-------------|----------|
| Preserve diagnostics ring behavior unchanged | Keep dedupe/cap/clear semantics | ✓ |
| Change cap size now | Modify diagnostics retention behavior | |
| Change dedupe behavior now | Modify message suppression behavior | |

**User's choice:** Preserve diagnostics behavior unchanged.
**Notes:** Dedupe + cap 100 + explicit clear remains baseline.

| Option | Description | Selected |
|--------|-------------|----------|
| Preserve `scene_solver_*` API authority | Runtime decisions remain in scene solver layer | ✓ |
| Allow UI-side shortcut logic | UI may make solve decisions directly | |

**User's choice:** Preserve `scene_solver_*` authority as hard requirement.
**Notes:** Keeps architecture boundary from prior phases intact.

---

## the agent's Discretion

- Exact tolerance field naming and storage representation.
- Exact debounce queue implementation details as long as chosen semantics hold.
- Exact diagnostic phrasing for max-pass failure reason.

## Deferred Ideas

- Grouped ALONG X/Y/Z mixed participant support (Phase 23).
- Advanced arc/line-arc constraints (Phase 24).
