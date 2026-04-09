# Phase 27: Principal-Axis Line ALONG Reliability - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-04-09
**Phase:** 27-principal-axis-line-along-reliability
**Areas discussed:** Principal-axis semantics contract, legality/runtime parity, mixed-constraint determinism and fixed-state policy

---

## Principal-axis semantics contract

| Option | Description | Selected |
|--------|-------------|----------|
| Axis-parallel line direction | `ALONG` constrains line direction to the named axis (`X`/`Y`/`Z`) | ✓ |
| Perpendicular lockstep interpretation | Endpoint motion tied perpendicular to named axis | |
| Legacy point-equality fallback | Keep existing point-group axis equality behavior for lines as-is | |

**User's choice:** Axis-parallel line direction
**Notes:** Carries forward explicit user clarification from prior interactive feedback that "Along X/Y/Z" should match principal-axis direction semantics for lines.

---

## Legality and runtime parity

| Option | Description | Selected |
|--------|-------------|----------|
| Shared legality/runtime contract | Keep legality rejection and runtime behavior synchronized via shared helper boundaries | ✓ |
| Runtime-only change | Change solver behavior without updating legality contract expectations | |
| UI-only message change | Keep behavior but only alter user-facing menu text | |

**User's choice:** Shared legality/runtime contract
**Notes:** Consistent with Phase 26 policy: no silent divergence between authoring gate and runtime path.

---

## Mixed-constraint determinism and fixed-state policy

| Option | Description | Selected |
|--------|-------------|----------|
| Transactional hard-anchor policy | Fixed participants remain anchors; all-fixed unsatisfied fails transactionally; reruns must be deterministic | ✓ |
| Relax fixed-state for feasibility | Move fixed participants if needed to satisfy `ALONG` | |
| Best-effort/no-fail mode | Permit partial solves and non-fatal unsatisfied states | |

**User's choice:** Transactional hard-anchor policy
**Notes:** Preserves prior phase reliability contracts and closure-gate expectations.

---

## the agent's Discretion

- Exact math/projection formulation for line axis-parallel enforcement.
- Exact diagnostic wording and assertion structure.
- Exact fixture decomposition across existing test binaries.

## Deferred Ideas

- Tangency drag robustness (Phase 28).
- Active-sketch line gizmo behavior (Phase 29).
- Solver docs and final closure gate (Phase 30).
