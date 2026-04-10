# Phase 32: Explicit Coincidence Authoring Semantics - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-04-10
**Phase:** 32-explicit-coincidence-authoring-semantics
**Areas discussed:** ArcAxisLine explicit coincidence, line-end/arc-end tangency explicit coincidence, lifecycle semantics

---

## ArcAxisLine explicit coincidence authoring

| Option | Description | Selected |
|--------|-------------|----------|
| Require explicit endpoint selection from user | ArcAxisLine can be created only after user chooses line endpoint | |
| Auto-pick nearest line endpoint | Create explicit `COINCIDENT(arc.center, line.endpoint)` using nearest endpoint to arc center at author time | ✓ |
| Always use `POINT_A` | Deterministic fixed endpoint regardless of geometry proximity | |

**User's choice:** Auto-pick nearest line endpoint at creation time.
**Notes:** Endpoint assignment is stable after creation; no automatic endpoint switching on later edits.

## ArcAxisLine endpoint stability and failure policy

| Option | Description | Selected |
|--------|-------------|----------|
| Never auto-switch endpoint | Keep authored endpoint binding stable after creation | ✓ |
| Auto-switch by proximity | Rebind to closer endpoint during edits | |
| Auto-switch only by explicit command | Rebind only on direct user action | |

| Option | Description | Selected |
|--------|-------------|----------|
| Transactional fail with explicit diagnostic | If explicit coincidence cannot be established, do not create owner constraint and report explicit reason | ✓ |
| Create owner without coincidence | Keep ArcAxisLine but warn that explicit pair is missing | |
| Soft/driven coincidence fallback | Create soft pair and defer full resolution | |

**User's choice:** Never auto-switch; fail transactionally with explicit ArcAxisLine coincidence diagnostics.
**Notes:** User explicitly rejected permissive fallback behavior.

## Line-end/arc-end tangency explicit coincidence authoring

| Option | Description | Selected |
|--------|-------------|----------|
| Always create explicit pair | Author tangency together with explicit endpoint coincidence | ✓ |
| Conditional explicit pair | Add coincidence only when endpoints are initially separated | |
| Keep implicit behavior | Rely on hidden coincidence coupling only | |

| Option | Description | Selected |
|--------|-------------|----------|
| Atomic pair rollback | Tangency and coincidence must both hold or transaction fails | |
| Tangency-only preservation | Keep tangency while coincidence may break | |
| Coincidence-only preservation | Keep coincidence while tangency may become unsatisfied with explicit diagnostic | ✓ |

**User's choice:** Always add explicit pair; when infeasible, keep coincidence authoritative and report tangency unsatisfied explicitly.
**Notes:** This is intentionally non-atomic for edit-time outcomes.

## Lifecycle semantics

| Option | Description | Selected |
|--------|-------------|----------|
| Leave lifecycle to planning discretion | Capture only high-level semantics here | |
| Lock lifecycle semantics now | Define delete/reapply behavior during discuss-phase | ✓ |

| Option | Description | Selected |
|--------|-------------|----------|
| Cascade delete paired coincidence | Removing owner also removes auto-created coincidence | |
| Keep paired coincidence | Removing owner leaves auto-created coincidence in scene | ✓ |
| Ask each time | Prompt user on owner deletion | |

| Option | Description | Selected |
|--------|-------------|----------|
| Owner removed/disabled | Deleting pair also deletes/disables owner | |
| Implicit fallback | Owner remains and reverts to implicit coupling | |
| Owner remains unsatisfied | Owner stays but reports explicit unsatisfied diagnostic | ✓ |

| Option | Description | Selected |
|--------|-------------|----------|
| Preserve explicit pair metadata | Stable IDs + relationship metadata survive emit/reapply | ✓ |
| Heuristic recompute on apply | Rebuild pairings from geometry each apply | |

**User's choice:** Lock lifecycle semantics now; keep paired coincidence when owner deleted; owner remains unsatisfied if pair deleted; preserve explicit pair metadata across script emit/reapply.
**Notes:** User prioritized deterministic authored intent over convenience cleanup.
