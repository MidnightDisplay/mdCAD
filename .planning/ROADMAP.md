# Roadmap: mdCAD

## Milestones

- ✅ **v1.0 Math Migration** — Phases 1-5 shipped 2026-03-26 ([archive](milestones/v1.0-ROADMAP.md))
- ✅ **v1.1 Long-Tail Migration** — Phases 6-9 shipped 2026-03-30 ([archive](milestones/v1.1-ROADMAP.md))
- ✅ **v1.2 Sketches, Constraints, Scripting** — Phases 10-21 shipped 2026-04-07 ([archive](milestones/v1.2-ROADMAP.md))
- ✅ **v1.3 Sketch Solver Audit + Constraint Expansion** — Phases 22-25 shipped 2026-04-08 ([archive](milestones/v1.3-ROADMAP.md))
- ✅ **v1.4 Solver Robustness + Sketch Gizmo Corrections** — Phases 26-30 shipped 2026-04-09 ([archive](milestones/v1.4-ROADMAP.md))
- ✅ **v1.5 Solver Workflow Robustness + Script Reapply Integrity** — Phases 31-35 shipped 2026-04-13 ([archive](milestones/v1.5-ROADMAP.md))
- ✅ **v1.6 Observable Flat JSONL Import for Large Geometry Dumps** — Phases 36-40 shipped 2026-04-27 ([archive](milestones/v1.6-ROADMAP.md))
- ◆ **v1.7 Linked Flat JSONL Large-File Refresh Stability** — Phases 41-42 planned

## Phases

- [x] **Phase 41: Linked Import Convergence** - Users can complete a linked large flat JSONL import and keep the full rendered geometry after import settles. (completed 2026-05-05)
- [ ] **Phase 42: Refresh & Teardown Stability** - Users can refresh and delete linked large flat imports without geometry collapse, late churn, or orphaned render state.

## Phase Details

### Phase 41: Linked Import Convergence
**Goal**: Users can complete a linked large flat JSONL import and keep the full rendered geometry after import settles.  
**Depends on**: Phase 40  
**Requirements**: FIMP-05, FIMP-06  
**Success Criteria** (what must be TRUE):
  1. User can import a large flat JSONL file with linking enabled and still see the full line/point geometry after the import settles.
  2. Scene Hierarchy totals for the linked import converge to the committed final counts instead of continuing to climb.
  3. The committed linked import remains visually complete rather than collapsing to a later tail subset.
**Plans**: 2 plans

Plans:
- [x] 41-01-PLAN.md — Arm linked-import observer baseline at commit time and lock deterministic no-self-refresh regressions.
- [x] 41-02-PLAN.md — Record the exact lamp_11 manual convergence checklist and run the blocking acceptance pass.

### Phase 42: Refresh & Teardown Stability
**Goal**: Users can refresh and delete linked large flat imports without geometry collapse, late churn, or orphaned render state.  
**Depends on**: Phase 41  
**Requirements**: OBSF-07, OBSF-08, PERF-04, PERF-05  
**Success Criteria** (what must be TRUE):
  1. User can leave observer refresh enabled without the visible geometry collapsing to a tail subset or points-only remnants.
  2. User can refresh a linked large flat import and see replaced geometry removed cleanly without premature deletion of retained geometry.
  3. After import or refresh settles, rendered coverage and counts stop changing from observer-related background churn.
  4. User can delete a previously refreshed linked flat import root and remove all related line/point instances from both the viewport and slot-buffer debug state.
**Plans**: 0 (not started)

## Progress

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 41. Linked Import Convergence | 2/2 | Complete | 2026-05-05 |
| 42. Refresh & Teardown Stability | 0/0 | Pending | - |
