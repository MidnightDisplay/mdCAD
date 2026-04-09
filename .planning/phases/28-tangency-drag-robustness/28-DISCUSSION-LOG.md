# Phase 28: Tangency Drag Robustness - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-04-09
**Phase:** 28-tangency-drag-robustness
**Areas discussed:** Drag anchor precedence during tangency edits, shared-point mobility vs adjacent-handle freedom, mirrored drag equivalence rules, infeasible edit rollback + diagnostics UX

---

## Drag anchor precedence during tangency edits

| Option | Description | Selected |
|--------|-------------|----------|
| Dragged endpoint as hard anchor | Keep dragged endpoint fixed in world space and adjust counterpart geometry around it | |
| Line-side priority | Prefer preserving line-side geometry over dragged-handle authority | |
| Arc-side priority | Prefer preserving arc-side geometry over dragged-handle authority | |
| User-provided policy | Dragged entity remains freely draggable within DoF limits; connected geometry adjusts to satisfy constraints | ✓ |

**User's choice:** User-provided policy: dragged participant remains manipulable (bounded by active DoF constraints) and connected entities solve around it.
**Notes:** User explicitly requested this as preferred UX and suggested it may generalize to other drag workflows, but called out the need for proper research/scrutiny before broad rollout.

---

## Shared-point mobility vs adjacent-handle freedom

| Option | Description | Selected |
|--------|-------------|----------|
| Allow both shared and adjacent drags; solve around dragged handle | Keep both interaction paths available in feasible setups | ✓ |
| Adjacent-only drags | Keep shared point effectively locked | |
| Shared-only drags | Limit adjacent handle movement | |

**User's choice:** Allow both shared-point and adjacent-handle drags, with solver behavior centered on the dragged handle.
**Notes:** Aligns with prior feedback that hard-locking common points creates unusable editing behavior.

---

## Mirrored drag equivalence rules

| Option | Description | Selected |
|--------|-------------|----------|
| Enforce mirrored feasibility parity | Equivalent mirrored interactions should pass/fail consistently under the same constraints | ✓ |
| Allow orientation-dependent divergence | Permit mirrored outcomes to diverge for numeric convenience | |

**User's choice:** Enforce mirrored equivalent feasibility outcomes.
**Notes:** Determinism and symmetry remain explicit quality targets.

---

## Infeasible edit rollback + diagnostics UX

| Option | Description | Selected |
|--------|-------------|----------|
| Transactional rollback + explicit family diagnostic + responsive next edits | No partial mutation, clear error reason, solver remains usable immediately | ✓ |
| Partial movement + warning | Keep partial geometry updates and emit warning | |
| Silent no-op | Discard edit without explicit diagnostic | |

**User's choice:** Transactional rollback with explicit tangency-family diagnostics and immediate continued responsiveness.
**Notes:** Continues the reliability posture established in prior phases.

---

## the agent's Discretion

- Exact tangency solve ordering details, test split, and diagnostic wording polish, provided chosen interaction/rollback semantics remain intact.

## Deferred Ideas

- Generalizing dragged-participant authority as a cross-constraint interaction policy beyond tangency (future phase candidate).
