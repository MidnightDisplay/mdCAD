# Phase 29: Active-Sketch Line Gizmo Endpoint Authority - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-04-09
**Phase:** 29-active-sketch-line-gizmo-endpoint-authority
**Areas discussed:** Scope boundary and eligibility, midpoint anchoring policy, undo/redo interaction granularity, mode behavior

---

## Scope boundary and eligibility

| Option | Description | Selected |
|--------|-------------|----------|
| Single-line only | Endpoint-authority applies only when exactly one active-sketch line is selected | |
| Multi-line eligible | Allow multi-selection of active-sketch lines; move each as rigid endpoint geometry | ✓ |

**User's choice:** Multi-line eligible.
**Notes:** User explicitly requested support beyond single-line interactions.

| Option | Description | Selected |
|--------|-------------|----------|
| Full fallback for mixed selection | Any mixed selection reverts entire drag to legacy semantics | |
| Hybrid mixed behavior | Apply endpoint-authority only to eligible active-sketch lines; apply existing semantics to other selected entities | ✓ |

**User's choice:** Hybrid mixed behavior.
**Notes:** Preserves targeted fix scope while avoiding regressions for non-line/non-active selections.

---

## Midpoint anchoring policy

| Option | Description | Selected |
|--------|-------------|----------|
| Average selected line midpoints | One gizmo anchored to average of selected line midpoints in multi-line selection | ✓ |
| Existing generic center | Keep current average entity-center behavior | |
| Disable multi-line endpoint-authority gizmo | No special gizmo anchor for multi-line line selection | |

**User's choice:** Average selected line midpoints.
**Notes:** Aligns gizmo location with geometric intent for line translation.

---

## Undo/redo interaction granularity

| Option | Description | Selected |
|--------|-------------|----------|
| Grouped drag entry | One drag commit creates one grouped undo/redo interaction across affected endpoints | ✓ |
| Per-entity/per-endpoint entries | Keep separate granular undo entries | |

**User's choice:** Grouped drag entry.
**Notes:** User wants coherent restoration semantics for full drag gestures.

---

## Mode behavior

| Option | Description | Selected |
|--------|-------------|----------|
| Transform-only activation | Endpoint-authority active only in transform mode | |
| Transform + geometry (vertex-mode inactive) | Endpoint-authority active in transform mode and geometry mode when vertex mode is not active | ✓ |

**User's choice:** Transform + geometry (vertex-mode inactive).
**Notes:** Vertex-mode editing remains distinct and should preserve direct vertex semantics.

---

## the agent's Discretion

- Exact helper/function factoring for eligibility checks, midpoint aggregation, and grouped undo command assembly.
- Exact test fixture distribution across existing drag/endpoint/undo-coverage suites.

## Deferred Ideas

- None.
