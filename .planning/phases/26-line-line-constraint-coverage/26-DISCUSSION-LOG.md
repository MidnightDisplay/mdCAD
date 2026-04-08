# Phase 26: Line-Line Constraint Coverage - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-04-08
**Phase:** 26-line-line-constraint-coverage
**Areas discussed:** Perpendicular group semantics, deterministic participant ordering, invalid selection/failure diagnostics, fixed/free participant movement policy

---

## Perpendicular group semantics

| Option | Description | Selected |
|--------|-------------|----------|
| Anchor-line contract | One canonical anchor line, every other selected line must be perpendicular to that anchor | ✓ |
| Pair-only contract | Perpendicular supported only for exactly 2 lines; 3+ rejected | |
| All-pairs contract | Every selected line must be perpendicular to every other selected line | |

**User's choice:** Anchor-line contract
**Notes:** User accepted recommended anchor-based group behavior for deterministic and practical multi-line semantics.

---

## Deterministic participant ordering

| Option | Description | Selected |
|--------|-------------|----------|
| Stable entity-id canonicalization | Sort participants by stable entity id and use first as canonical anchor | ✓ |
| First user selection anchor | Preserve click order as anchor | |
| Explicit anchor UX | User chooses anchor line interactively in this phase | |

**User's choice:** Stable entity-id canonicalization
**Notes:** User accepted recommended deterministic ordering independent of selection order.

---

## Invalid selection and failure diagnostics contract

| Option | Description | Selected |
|--------|-------------|----------|
| Explicit type-specific diagnostics | Legality rejection on create; unsatisfied-driving messages name family and implicated entities | ✓ |
| Generic diagnostics | Generic invalid/unsatisfied message only | |
| Silent block | Disable apply with no extra diagnostics | |

**User's choice:** Explicit type-specific diagnostics
**Notes:** User explicitly wants clear family-level diagnostics and implication visibility.

---

## Fixed/free participant movement policy

| Option | Description | Selected |
|--------|-------------|----------|
| Respect fixed-state anchors | Fixed participants stay fixed; all-fixed unsatisfied setup fails transactionally | ✓ |
| Even split ignoring fixed-state | Split movement across all participants regardless of fixed state | |
| Move non-anchor first | Prefer non-anchor movement, then anchor if needed while respecting fixed state | |

**User's choice:** Respect fixed-state anchors
**Notes:** User accepted recommended strict fixed-state behavior and transactional failure when over-constrained.

---

## the agent's Discretion

- Exact diagnostic string phrasing and formatting.
- Exact internal math implementation details that satisfy selected behavior contracts.
- Exact test fixture decomposition across existing solver tests.

## Deferred Ideas

- ALONG line semantics fixes (Phase 27).
- Tangency drag robustness (Phase 28).
- Sketch-line gizmo endpoint/midpoint behavior (Phase 29).
- Solver docs and final closure gate (Phase 30).
