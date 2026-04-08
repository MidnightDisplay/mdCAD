# Phase 24: Advanced Arc + Line-Arc Constraint Expansion - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in `24-CONTEXT.md`; this log preserves alternatives considered.

**Date:** 2026-04-08
**Phase:** 24-advanced-arc-line-arc-constraint-expansion
**Areas discussed:** Arc-center perpendicular contract, line-end/arc-end tangency, arc endpoint-angle semantics, diagnostics/recalc behavior

---

## Arc-center perpendicular-to-line behavior

| Option | Description | Selected |
|--------|-------------|----------|
| Arc normal axis parallel/anti-parallel to line direction | Treat line as orientation reference for arc axis relation | ✓ |
| Arc start tangent perpendicular to line | Tangency-at-start interpretation instead of axis relation | |
| Keep arc normal fixed, move center only | Positional relation only, no axis reorientation | |

**User's choice:** Arc normal axis must be parallel/anti-parallel to the line direction.
**Notes:** Additional lock-ins captured: legality is exactly one line entity + one arc entity; if both fixed and unsatisfied then transactional fail with explicit diagnostics; default movable side is arc (line as reference).

---

## Line-end and arc-end tangency behavior

| Option | Description | Selected |
|--------|-------------|----------|
| Exactly one line endpoint + one arc endpoint | Explicit endpoint-pair legality; no whole-entity inference | ✓ |
| Whole line + whole arc entities | Infer endpoints implicitly from geometry | |
| Multi-pair/group tangency | One constraint may include multiple endpoint pairs | |

**User's choice:** Exactly one line endpoint and one arc endpoint.
**Notes:** Solve contract locked as coincidence-first then tangency. Fixed/free policy: fixed anchors, free moves. If both fixed and unsatisfied: transactional fail with explicit tangency diagnostic.

---

## Arc start/end angle semantics

| Option | Description | Selected |
|--------|-------------|----------|
| Endpoint-pair on same arc with selection-order anchoring | First selected endpoint is reference; second endpoint moves | ✓ |
| Single arc entity with POINT_A anchored by default | Entity-level authoring, fixed implicit anchor | |
| Single arc entity with POINT_B anchored by default | Entity-level authoring, alternate implicit anchor | |

**User's choice:** Endpoint-pair on same arc, first selected endpoint is reference.
**Notes:** User explicitly requested selection-order predictability. Value domain locked to `[0, π]`, UI in degrees, internal solver radians.

---

## Diagnostics and recalc behavior

| Option | Description | Selected |
|--------|-------------|----------|
| Keep existing transactional failure + diagnostics + implication highlighting + deterministic recalc | Extend current solver contract to ARCI constraints unchanged | ✓ |
| Disable implication highlighting for ARCI failures | Keep diagnostics but reduce highlight behavior | |
| Allow partial commits on ARCI failures | Favor partial progression over strict rollback | |

**User's choice:** Keep existing policy unchanged and extend to ARCI constraints.
**Notes:** User requested dedicated explicit unsatisfied reason strings per ARCI family (arc-axis, tangency, arc endpoint-angle).

---

## the agent's Discretion

- Exact low-level solve math and projection/rotation formulas.
- Exact wording of ARCI diagnostic strings, provided they remain per-family explicit.
- Final test fixture split across existing solver targets.

## Deferred Ideas

None.

