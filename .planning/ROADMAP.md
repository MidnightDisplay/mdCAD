# Roadmap: mdCAD

## Milestones

- ✅ **v1.0 Math Migration** — Phases 1-5 shipped 2026-03-26 ([archive](milestones/v1.0-ROADMAP.md))
- ✅ **v1.1 Long-Tail Migration** — Phases 6-9 shipped 2026-03-30 ([archive](milestones/v1.1-ROADMAP.md))
- ✅ **v1.2 Sketches, Constraints, Scripting** — Phases 10-21 shipped 2026-04-07 ([archive](milestones/v1.2-ROADMAP.md))
- ✅ **v1.3 Sketch Solver Audit + Constraint Expansion** — Phases 22-25 shipped 2026-04-08 ([archive](milestones/v1.3-ROADMAP.md))
- 🚧 **v1.4 Solver Robustness + Sketch Gizmo Corrections** — Phases 26-30 (active)

## Current Status

v1.4 roadmap is active. Next step: `/gsd-discuss-phase 30`.

## Overview

v1.4 is a robustness-first milestone focused on solver correctness, deterministic interaction behavior, and active-sketch line gizmo fixes. Delivery flows from line-line constraint coverage through ALONG/tangency hardening, then closes with solver documentation and deterministic Windows Vulkan validation gates.

## Phases

- [x] **Phase 26: Line-Line Constraint Coverage** - Deliver pair/group parallel-perpendicular constraints with legality/runtime parity. (completed 2026-04-08)
- [x] **Phase 27: Principal-Axis Line ALONG Reliability** - Fix ALONG X/Y/Z line behavior for deterministic mixed-constraint solving. (completed 2026-04-09)
- [x] **Phase 28: Tangency Drag Robustness** - Harden arc-line tangency and mixed-constraint drag transactional reliability. (completed 2026-04-09)
- [x] **Phase 29: Active-Sketch Line Gizmo Endpoint Authority** - Correct midpoint anchoring and endpoint-driven gizmo interaction semantics. (completed 2026-04-09)
- [ ] **Phase 30: Deterministic Closure Gate (Windows Vulkan) + Solver Docs** - Close targeted tests/docs and enforce deterministic milestone sign-off.

## Phase Details

### Phase 26: Line-Line Constraint Coverage
**Goal**: Users can apply line-line `PARALLEL` and `PERPENDICULAR` constraints in active sketches with deterministic, legality-validated behavior.
**Depends on**: Phase 25
**Requirements**: LCON-01, LCON-02, LCON-03, LCON-04, LCON-05
**Success Criteria** (what must be TRUE):
  1. User can apply `PARALLEL` between two feasible sketch lines and see a solved result.
  2. User can apply `PARALLEL`/`PERPENDICULAR` to supported multi-line selections and all participants satisfy the relation after solve.
  3. Equivalent line selections produce the same solved outcome regardless of selection order.
  4. Invalid line-line selections are rejected at legality time with explicit feedback instead of ambiguous solver failure.
**Plans**: 2 plans
Plans:
- [x] 26-01-PLAN.md — Pair line-line legality/runtime parity with transactional diagnostics and deterministic tests.
- [x] 26-02-PLAN.md — Group line-line canonical anchor semantics, explicit legality UX, and deterministic rerun closure.

### Phase 27: Principal-Axis Line ALONG Reliability
**Goal**: Users can constrain lines to principal axes (`ALONG X/Y/Z`) without immediate solver failure and with deterministic behavior in mixed constraints.
**Depends on**: Phase 26
**Requirements**: ALIN-01, ALIN-02, ALIN-03, ALIN-04
**Success Criteria** (what must be TRUE):
  1. User can apply `ALONG X` to a sketch line without immediate unsatisfied-driving failure.
  2. User can apply `ALONG Y` and `ALONG Z` to sketch lines with the same no-immediate-failure behavior.
  3. User can combine line `ALONG` with `LENGTH`, `ANGLE`, and connectivity constraints and get deterministic solved outcomes in feasible setups.
**Plans**: 2 plans
Plans:
- [x] 27-01-PLAN.md — ALONG X/Y/Z line runtime normalization and legality/runtime parity reliability.
- [x] 27-02-PLAN.md — Mixed ALONG+LENGTH+ANGLE+connectivity determinism and transactional diagnostics closure.

### Phase 28: Tangency Drag Robustness
**Goal**: Users can create and edit arc-line tangency constraints with stable, transactional, deterministic drag behavior.
**Depends on**: Phase 27
**Requirements**: TRDG-01, TRDG-02, TRDG-03, TRDG-04
**Success Criteria** (what must be TRUE):
  1. User can author line-end/arc-end tangency in common fillet-like setups and get stable solved geometry when feasible.
  2. User can drag shared or adjacent tangency participants in feasible setups without solver deadlock.
  3. Infeasible tangency edits rollback transactionally, surface clear diagnostics, and keep solver responsive for subsequent edits.
  4. Equivalent mirrored drag interactions in mixed-constraint sketches produce consistent feasibility outcomes.
**Plans**: 2 plans
Plans:
- [x] 28-01-PLAN.md — Tangency drag-authority/shared-adjacent feasibility hardening with transactional rollback and responsiveness guarantees.
- [x] 28-02-PLAN.md — Mirrored tangency interaction determinism parity and deterministic closure rerun evidence capture.

### Phase 29: Active-Sketch Line Gizmo Endpoint Authority
**Goal**: Users get correct active-sketch line interaction behavior where gizmo movement is geometry-authoritative and midpoint-anchored.
**Depends on**: Phase 28
**Requirements**: GZM-01, GZM-02, GZM-03, GZM-04
**Success Criteria** (what must be TRUE):
  1. User sees the gizmo anchored at the selected active-sketch line midpoint.
  2. User dragging the active-sketch line gizmo translates endpoints `A` and `B` together as rigid geometry.
  3. User sees endpoint-driven behavior only for active-sketch lines; non-active or non-line selections preserve existing semantics.
  4. User can undo/redo a completed active-sketch line drag as one coherent interaction restoring exact endpoint geometry.
**Plans**: 2 plans
Plans:
- [x] 29-01-PLAN.md — Midpoint anchoring and eligible active-sketch line endpoint-authority drag routing with mixed-selection guardrails.
- [x] 29-02-PLAN.md — Grouped single-interaction undo/redo for active-sketch line drags with deterministic rerun closure.
**UI hint**: yes

### Phase 30: Deterministic Closure Gate (Windows Vulkan) + Solver Docs
**Goal**: Developers can trust deterministic solver reliability via targeted regression closure and Windows Vulkan sign-off, with clear solver architecture documentation for future iteration.
**Depends on**: Phase 29
**Requirements**: SDOC-01, SDOC-02, SDOC-03, V14-01, V14-02
**Success Criteria** (what must be TRUE):
  1. Developer can run targeted automated tests covering line-line constraints, line `ALONG`, tangency robustness, and active-sketch line gizmo behavior.
  2. Re-running the targeted closure suite produces deterministic pass results suitable for sign-off.
  3. Developer can run milestone closure reruns on Windows Vulkan and obtain deterministic pass results for final gate approval.
  4. Developer can use solver architecture docs with literature references, code/file anchors, and a TL;DR implementation primer to debug key constraints.
**Plans**: 2 plans
Plans:
- [x] 30-01-PLAN.md — Publish practical solver architecture docs with code anchors, references, and TL;DR debug primer.
- [ ] 30-02-PLAN.md — Lock canonical Windows Vulkan deterministic closure gate and capture baseline+rerun verification contract.

## Progress

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 26. Line-Line Constraint Coverage | 2/2 | Complete    | 2026-04-08 |
| 27. Principal-Axis Line ALONG Reliability | 2/2 | Complete    | 2026-04-09 |
| 28. Tangency Drag Robustness | 2/2 | Complete    | 2026-04-09 |
| 29. Active-Sketch Line Gizmo Endpoint Authority | 2/2 | Complete | 2026-04-09 |
| 30. Deterministic Closure Gate (Windows Vulkan) + Solver Docs | 1/2 | In Progress|  |

