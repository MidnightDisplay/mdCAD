# Phase 12: Solver Control & Constrained Interaction - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md - this log preserves the alternatives considered.

**Date:** 2026-04-01
**Phase:** 12-solver-control-and-constrained-interaction
**Areas discussed:** Solve trigger policy, Diagnostics panel/log behavior, Failure implication highlighting behavior, Gizmo behavior under constraints

---

## Solve trigger policy

| Option | Description | Selected |
|--------|-------------|----------|
| Auto-solve ON by default + manual Recalculate button | New sketches start in auto mode while still allowing explicit recalc | ✓ |
| Auto-solve OFF by default + manual Recalculate only | No implicit solve runs | |
| Global default in settings + per-sketch override | Configurable default strategy | |

**User's choice:** Auto-solve ON by default + manual Recalculate button.
**Notes:** Follow-up decision selected auto-solve execution after every sketch-affecting edit with lightweight debounce.

| Option | Description | Selected |
|--------|-------------|----------|
| After every sketch-affecting edit with lightweight debounce | Trigger solve after geometry/constraint/value/driven edits with coalescing | ✓ |
| Only on explicit Recalculate even if auto-solve ON | Auto flag has no trigger effect | |
| On pointer-release/end-of-edit only | Less frequent trigger cadence | |

**User's choice:** After every sketch-affecting edit with lightweight debounce.
**Notes:** Decision applies broadly to geometry edits, constraint changes, dimensional value edits, and driven toggles.

---

## Diagnostics panel/log behavior

| Option | Description | Selected |
|--------|-------------|----------|
| Per-sketch rolling log (latest N entries), clear on user action | Keeps operational context by sketch with explicit clearing | ✓ |
| Per-sketch latest message only | No history retained | |
| Global shared log across all sketches | Shared cross-sketch log stream | |

**User's choice:** Per-sketch rolling log, cleared only by explicit user action.
**Notes:** Follow-up capacity decision selected 100 entries.

| Option | Description | Selected |
|--------|-------------|----------|
| 100 entries | Medium rolling history | ✓ |
| 50 entries | Shorter history | |
| 200 entries | Longer history | |

**User's choice:** 100 entries.
**Notes:** Applies per sketch.

---

## Failure implication highlighting behavior

| Option | Description | Selected |
|--------|-------------|----------|
| Highlight implicated constraints + participant geometries, select first implicated constraint | Full context + focused primary target | ✓ |
| Highlight constraints only | Constraint-centric visibility only | |
| Highlight geometries only | Geometry-centric visibility only | |

**User's choice:** Highlight both implicated constraints and participant geometries, and select the first implicated constraint.
**Notes:** Follow-up persistence decision selected persistence until next successful solve.

| Option | Description | Selected |
|--------|-------------|----------|
| Persist until next successful solve | Keep failure context visible through iterative edits | ✓ |
| Clear on any subsequent user edit | Immediate clear after interaction | |
| Persist until explicit Clear button | Manual clear only | |

**User's choice:** Persist until next successful solve.
**Notes:** This drives both highlight and focus persistence behavior.

---

## Gizmo behavior under constraints

| Option | Description | Selected |
|--------|-------------|----------|
| Constrained projection in real time; block if unsatisfiable | Respect constraints continuously and prevent invalid placement | ✓ |
| Allow free drag, then snap back after solve | Temporary invalid placement allowed | |
| Always block drag for constrained geometry | No constrained dragging | |

**User's choice:** Constrained projection in real time where possible; if unsatisfiable, block movement and surface diagnostic.
**Notes:** Follow-up UX decision selected viewport toast + diagnostics log entry + implication highlight.

| Option | Description | Selected |
|--------|-------------|----------|
| Viewport toast + diagnostics log entry + implication highlight | Immediate visible feedback + traceability + contextual focus | ✓ |
| Diagnostics log entry only | Silent viewport behavior | |
| Modal popup requiring dismissal | Blocking feedback | |

**User's choice:** Viewport toast + diagnostics log entry + implicated highlight.
**Notes:** Non-modal feedback was preferred.

---

## the agent's Discretion

- Internal debounce timing constant and queue/coalescing details.
- Internal diagnostics data struct layout and storage implementation.
- Exact visual styling/placement details for non-modal viewport toast.

## Deferred Ideas

- Multi-backend solver selection UX.
- Cross-sketch/global solve graph behavior.
- Script-level solver control integration.

