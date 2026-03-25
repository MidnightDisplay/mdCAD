# Phase 4: Interaction Math and API Expansion - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-03-25
**Phase:** 04-interaction-math-and-api-expansion
**Areas discussed:** Pick/Unproject Ray Path, Gizmo Drag/Intersection Math, Helper Surface Expansion, Retirement Boundary for `math3d`

---

## Pick/Unproject Ray Path

| Option | Description | Selected |
|--------|-------------|----------|
| `cglm`-first through a project helper | Add one helper in `src/math/` and route runtime call sites through it | ✓ |
| Direct `cglm` at call sites | No shared helper boundary | |
| Keep legacy `ray_from_screen` primary | Defer migration in runtime path | |

**User's choice:** `cglm`-first through a project helper (`A1-1`)
**Notes:** User selected parity-focused shared migration path for interaction trust.

---

## Gizmo Drag/Intersection Math

| Option | Description | Selected |
|--------|-------------|----------|
| Shared `cglm`-backed helpers | Centralize axis/plane intersection + delta transforms across gizmo modules | ✓ |
| In-place gizmo-only migration | Update each gizmo file directly without shared helper API | |
| Keep legacy drag math | Defer migration | |

**User's choice:** Shared `cglm`-backed helpers (`A2-1`)
**Notes:** User chose reuse and consistency across transform and vertex edit modes.

---

## Helper Surface Expansion

| Option | Description | Selected |
|--------|-------------|----------|
| Add full Phase-4 helper set | Project/unproject, ray helpers, quaternion-capable expansion now | ✓ |
| Only pick/gizmo helpers now | Defer quaternion expansion | |
| Direct vendor calls only | No project helper additions | |

**User's choice:** Add full Phase-4 helper set (`A3-1`)
**Notes:** User explicitly chose to include expansion in this phase rather than defer.

---

## Retirement Boundary for `math3d`

| Option | Description | Selected |
|--------|-------------|----------|
| Retire in migrated interaction slices only | Remove equivalent helpers in app/pick/gizmo paths migrated in Phase 4 | ✓ |
| Keep compatibility in migrated files through Phase 5 | Delay retirement | |
| Aggressive broader retirement now | Wider cleanup beyond interaction slices | |

**User's choice:** Retire in migrated interaction slices only (`A4-1`)
**Notes:** Initial preference was aggressive broader retirement (`A4-3`), then corrected to scoped Phase-4 retirement after scope-boundary check.

---

## the agent's Discretion

- Exact helper names/signatures under `src/math/`
- Exact cglm struct-vs-array use per helper implementation
- Exact harness case additions for interaction parity gates

## Deferred Ideas

- Broader repo-wide retirement of equivalent `src/math3d.h` helpers outside migrated interaction slices
