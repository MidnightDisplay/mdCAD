# Roadmap: mdCAD

## Milestones

- ✅ **v1.0 Math Migration** — Phases 1-5 shipped 2026-03-26 ([archive](milestones/v1.0-ROADMAP.md))
- ✅ **v1.1 Long-Tail Migration** — Phases 6-9 shipped 2026-03-30 ([archive](milestones/v1.1-ROADMAP.md))
- ✅ **v1.2 Sketches, Constraints, Scripting** — Phases 10-21 shipped 2026-04-07 ([archive](milestones/v1.2-ROADMAP.md))
- 🚧 **v1.3 Sketch Solver Audit + Constraint Expansion** — Phases 22-25 (active)

## Active Roadmap (v1.3)

**Milestone Goal:** Stabilize sketch solver reliability and deliver advanced axis/arc constraint behavior with deterministic solve outcomes.

## Phases

- [x] **Phase 22: Solver Trigger + Recalculate Determinism** - Establish trustworthy solve triggering and deterministic pass-bounded recalculate behavior.
- [ ] **Phase 23: Principal-Direction Constraint Expansion** - Deliver ALONG X/Y/Z group constraints across supported point participant types.
- [ ] **Phase 24: Advanced Arc + Line-Arc Constraint Expansion** - Deliver arc-axis, tangency, and arc endpoint angle constraints with deterministic outcomes.
- [ ] **Phase 25: Regression and Reliability Closure** - Lock automated regression coverage for trigger integrity, pass policy behavior, and new constraint semantics.

## Phase Details

### Phase 22: Solver Trigger + Recalculate Determinism
**Goal**: Users can trust that sketch solves trigger at the right times and produce deterministic recalculate outcomes with bounded iterative behavior.
**Depends on**: Phase 21
**Requirements**: SRLV-01, SRLV-02, SRLV-03, SRLV-05
**Success Criteria** (what must be TRUE):
  1. User can commit sketch mutations (constraint add/remove/edit, geometry move) and auto-solve reliably runs without manual fallback.
  2. User can run manual recalculate repeatedly on unchanged sketch state and receive identical/idempotent results.
  3. User can observe solves stop only when tolerance is reached or the configured pass cap is hit, with default cap behavior of 400 when not overridden.
  4. User can apply driving LENGTH and ANGLE constraints and see expected geometric effects or a clear explicit failure diagnostic.
**Plans**: 3 plans

Plans:
- [x] 22-01-PLAN.md — Create Wave-0 solver trigger/pass-policy/contract tests and CTest wiring for Phase 22 contracts.
- [x] 22-02-PLAN.md — Implement scene-owned debounce queue, manual override, bounded recalc pass policy, and LENGTH/ANGLE deterministic runtime behavior.
- [x] 22-03-PLAN.md — Expose tolerance/max-pass controls in inspector, lock approved defaults, and run final human verification for deterministic recalc UX.

### Phase 23: Principal-Direction Constraint Expansion
**Goal**: Users can apply and solve principal-direction constraints across pair/group selections and mixed point participant sources.
**Depends on**: Phase 22
**Requirements**: AXIS-01, AXIS-02, AXIS-03, AXIS-04, SRLV-04
**Success Criteria** (what must be TRUE):
  1. User can apply ALONG X to point pairs and point groups and geometry solves consistently along principal X direction.
  2. User can apply ALONG Y and ALONG Z to point pairs and point groups with deterministic directional solve behavior.
  3. User can use directional group constraints with standalone points, line endpoints, and arc landmark points in supported combinations.
  4. User can recalculate sketches containing ALONG X/Y/Z and ANGLE constraints and complete solve attempts without broken/non-functional pathways.
**Plans**: TBD

### Phase 24: Advanced Arc + Line-Arc Constraint Expansion
**Goal**: Users can author and maintain advanced arc and line-arc constraints that solve predictably or fail with explicit diagnostics.
**Depends on**: Phase 23
**Requirements**: ARCI-01, ARCI-02, ARCI-03, ARCI-04
**Success Criteria** (what must be TRUE):
  1. User can constrain an arc-center axis relative to a line and the arc orientation follows the perpendicular-to-line authoring contract.
  2. User can constrain line-end and arc-end tangency at a shared point and solver preserves tangency by moving geometry as needed.
  3. User can apply and edit a single-arc start/end angle constraint and resulting geometry updates remain deterministic.
  4. User receives explicit diagnostics when advanced arc/line-arc constraints are unsatisfiable instead of silent or ambiguous failures.
**Plans**: TBD

### Phase 25: Regression and Reliability Closure
**Goal**: Developers can repeatedly validate solver trigger integrity and expanded constraint behavior through automated regression coverage.
**Depends on**: Phase 24
**Requirements**: V13-01
**Success Criteria** (what must be TRUE):
  1. Developer can run automated regression checks that verify auto-solve trigger integrity for committed sketch mutation paths.
  2. Developer can run automated regression checks that verify iterative pass behavior (tolerance stop and pass-cap stop) is enforced.
  3. Developer can run automated regression checks that cover legality and deterministic solve semantics for newly added axis and arc/line-arc constraints.
**Plans**: TBD

## Progress

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 22. Solver Trigger + Recalculate Determinism | 3/3 | Complete | 2026-04-08 |
| 23. Principal-Direction Constraint Expansion | 0/0 | Not started | - |
| 24. Advanced Arc + Line-Arc Constraint Expansion | 0/0 | Not started | - |
| 25. Regression and Reliability Closure | 0/0 | Not started | - |
