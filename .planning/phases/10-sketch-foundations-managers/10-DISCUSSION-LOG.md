# Phase 10: Sketch Foundations & Managers - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-03-30
**Phase:** 10-sketch-foundations-managers
**Areas discussed:** Sketch creation flow, geometry attachment flow, manager organization, fixed-state semantics, status/color presentation, multiselect behavior

---

## Sketch creation flow

| Option | Description | Selected |
|--------|-------------|----------|
| Add Entity menu -> Create Sketch | Consistent with existing menu-driven creation UX | ✓ |
| Button in Entity Inspector when nothing selected | Creation from inspector surface only | |
| Dedicated Sketch toolbar/panel button | New top-level entrypoint pattern | |
| You decide | Delegate decision to agent | |

**User's choice:** Add Entity menu -> Create Sketch  
**Notes:** User preferred consistency with existing mdCAD interaction patterns.

---

## Geometry attachment flow

| Option | Description | Selected |
|--------|-------------|----------|
| Add Entity menu only | Single global creation path | |
| GeometryManager Add buttons only | Localized manager-driven creation path | |
| Both menu and manager add paths | Support both global and local workflows | ✓ |
| You decide | Delegate decision to agent | |

**User's choice:** Both menu and manager add paths  
**Notes:** User wants flexibility for both global and contextual attachment workflows.

---

## GeometryManager list organization

| Option | Description | Selected |
|--------|-------------|----------|
| Flat list with type + name + fixed/loose status | Lowest complexity and quick scan behavior | ✓ |
| Grouped by geometry type | Sectioned list by type buckets | |
| Hierarchical tree under sketch node | Nested tree structure | |
| You decide | Delegate decision to agent | |

**User's choice:** Flat list with type + name + fixed/loose status  
**Notes:** User selected the recommended simple structure.

---

## Fixed-state semantics (Phase 10)

| Option | Description | Selected |
|--------|-------------|----------|
| Lock geometry immediately in gizmo + inspector edits | Full enforcement in Phase 10 | |
| Lock gizmo only, allow inspector edits | Partial enforcement | |
| Mark fixed visually now, enforce later in solver phase | Phase 10 state-first, Phase 12 enforcement | ✓ |
| You decide | Delegate decision to agent | |

**User's choice:** Mark fixed visually now, enforce later in solver phase  
**Notes:** User explicitly deferred strict enforcement to later constrained interaction phase.

---

## Sketch color propagation

| Option | Description | Selected |
|--------|-------------|----------|
| Implement full propagation/override now | Include black-inherit/non-black override semantics now | ✓ |
| Show control now, defer strict propagation logic | UI-first, behavior later | |
| You decide | Delegate decision to agent | |

**User's choice:** Implement full propagation/override semantics now  
**Notes:** User wants full intended behavior from proposal in Phase 10.

---

## Sketch status presentation pre-solver

| Option | Description | Selected |
|--------|-------------|----------|
| Provisional statuses only | Temporary labels until solver phase | |
| Reuse final labels now (`solved/loose/fixed/error`) with placeholder logic | Stable vocabulary from start | ✓ |
| Hide status until solver phase | No status in Phase 10 | |
| You decide | Delegate decision to agent | |

**User's choice:** Reuse final labels now with placeholder logic  
**Notes:** User prefers stable end-state terminology early.

---

## Multiselect behavior

| Option | Description | Selected |
|--------|-------------|----------|
| Multiselect + one combined undo entry per action | Atomic user action behavior | ✓ |
| Multiselect + one undo per entity | Granular but noisy undo stack | |
| Single-select only in Phase 10 | Minimal action scope | |
| You decide | Delegate decision to agent | |

**User's choice:** Multiselect + combined undo behavior  
**Notes:** User chose atomic interaction and undo semantics.

---

## the agent's Discretion

- Precise placeholder status derivation logic before solver integration.
- Exact UI composition details and visual presentation refinements.
- Internal command representation for grouped undo entries.

## Deferred Ideas

- Strict fixed-state enforcement in manipulation paths deferred to solver/constrained interaction phase.
- Constraint authoring and glyph editing behaviors deferred to Phase 11.
- Solver backend and diagnostics depth deferred to Phase 12.
- Script subsystem deferred to Phases 13-14.
