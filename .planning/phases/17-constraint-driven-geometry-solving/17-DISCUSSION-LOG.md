# Phase 17: Constraint-driven geometry solving - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves alternatives considered.

**Date:** 2026-04-02
**Phase:** 17-constraint-driven-geometry-solving
**Areas discussed:** Solve capability scope, Interaction semantics during drag, Conflict handling and diagnostics, Precision/performance guardrails

---

## Solve capability scope

| Option | Description | Selected |
|--------|-------------|----------|
| Enforce full v1.2 set | Enforce all v1.2 constraints in solve updates | ✓ |
| Core subset first | FIXED/COINCIDENT/PARALLEL/PERPENDICULAR/LENGTH/ANGLE only; others diagnostics | |
| Dimensional + fixed only | Constrain scope to dimensional and fixed behaviors | |

**User's choice:** Enforce the full v1.2 constraint set in solve updates.
**Notes:** User wants constraints to be truly functional, not cosmetic.

| Option | Description | Selected |
|--------|-------------|----------|
| Immediate full update | Apply full solved propagation to affected geometry immediately | ✓ |
| Direct-edit only + deferred propagation | Only directly edited entities update immediately | |
| Preview then confirm | Show projected solve and apply on explicit confirmation | |

**User's choice:** Immediate full update of all affected sketch geometry.
**Notes:** Confirms authoritative solver behavior expectation.

---

## Interaction semantics during drag

| Option | Description | Selected |
|--------|-------------|----------|
| Live constrained solve each frame | Smooth projected drag under active constraints; block when unsatisfiable | ✓ |
| Project during drag, exact solve on end | Lower per-frame cost, exactness on release | |
| Free drag + post-snap | Unconstrained motion then resolve after | |

**User's choice:** Live constrained solve each frame with smooth projection.
**Notes:** Interaction should continuously honor constraints.

| Option | Description | Selected |
|--------|-------------|----------|
| Keep last valid solved state | No partial invalid pose; immediate feedback on failure | ✓ |
| Allow temporary invalid pose then rollback | More permissive drag, revert on release | |
| Clamp to nearest feasible pose | Continue drag with constrained clamp | |

**User's choice:** Keep last valid solved state and show immediate feedback.
**Notes:** No tolerance for invalid intermediate committed geometry state.

---

## Conflict handling and diagnostics

| Option | Description | Selected |
|--------|-------------|----------|
| Deterministic failure, no mutation | Explicit implicated constraints + reason, reject scene mutation | ✓ |
| Best-effort partial apply | Apply partial results with warnings where possible | |
| Auto-relax constraints | Automatically drop lower-priority constraints | |

**User's choice:** Deterministic failure with explicit implication and no scene mutation.
**Notes:** Predictability and auditability are preferred over hidden heuristics.

| Option | Description | Selected |
|--------|-------------|----------|
| Append + dedupe consecutive identical | Keep history, avoid noisy duplicates, explicit clear action | ✓ |
| Latest-only | Overwrite with latest result only | |
| Failures-only log | Suppress info/warning entries | |

**User's choice:** Append every attempt with dedupe for identical consecutive messages, explicit clear action retained.
**Notes:** Useful for iterative debugging without runaway log spam.

---

## Precision/performance guardrails

| Option | Description | Selected |
|--------|-------------|----------|
| Responsiveness-first bounded budget | Bounded per-frame solve budget + graceful degrade, preserve correctness | ✓ |
| Exactness-first always | Full exact solve even if framerate drops | |
| Auto-disable live solve on larger sketches | Force manual recalc for heavy cases | |

**User's choice:** Maintain responsiveness first with bounded per-frame budget and graceful degrade while preserving correctness.
**Notes:** UX fluidity is required, but no correctness compromise.

| Option | Description | Selected |
|--------|-------------|----------|
| Deterministic fixture gates | Deterministic outputs with tolerance assertions in acceptance gates | ✓ |
| Manual testing only | Human verification without deterministic fixtures | |
| Smoke tests only | Basic sanity checks, defer deep verification | |

**User's choice:** Include deterministic solve fixtures and tolerance-based assertions as acceptance gates.
**Notes:** Regression-proofing is required for solver evolution.

---

## the agent's Discretion

- Per-frame budget numbers and fallback trigger heuristics.
- Exact tolerance constants and fixture set structure.
- Internal data-structure choices for diagnostics dedupe.

## Deferred Ideas

- None raised beyond current phase boundary during this discussion.

---

## Sketch endpoint/sub-entity selection gap (addendum)

| Option | Description | Selected |
|--------|-------------|----------|
| Keep gizmo vertex mode as selection path | Endpoint picking remains tied to Tab vertex mode | |
| Add first-class endpoint sub-elements in normal viewport selection | Endpoint points hover/select without entering gizmo mode | ✓ |
| Constraint authoring by whole-entity inference only | No explicit endpoint selection, infer from line/arc entity picks | |

**User's choice:** Add first-class endpoint/sub-entity selection in normal viewport flow; do not rely on gizmo vertex mode.
**Notes:** Coincident endpoint chaining/loop closure is a core sketch-authoring requirement.

| Option | Description | Selected |
|--------|-------------|----------|
| Render endpoint points above continuous geometry in viewport + pick buffer | Prioritize endpoint pickability for authoring reliability | ✓ |
| Keep existing primitive layering and use mode-switch filtering | Lower structural change, but less direct authoring | |

**User's choice:** Keep endpoint points rendered/pickable above lines/arcs/circles for reliable direct selection.
**Notes:** Must support line/arc endpoint Coincident constraints and robust chain/loop construction.
