# Roadmap: mdCAD

## Milestones

- ✅ **v1.0 Math Migration** — Phases 1-5 shipped 2026-03-26 ([archive](milestones/v1.0-ROADMAP.md))
- ✅ **v1.1 Long-Tail Migration** — Phases 6-9 shipped 2026-03-30 ([archive](milestones/v1.1-ROADMAP.md))
- ✅ **v1.2 Sketches, Constraints, Scripting** — Phases 10-21 shipped 2026-04-07 ([archive](milestones/v1.2-ROADMAP.md))
- ✅ **v1.3 Sketch Solver Audit + Constraint Expansion** — Phases 22-25 shipped 2026-04-08 ([archive](milestones/v1.3-ROADMAP.md))
- ✅ **v1.4 Solver Robustness + Sketch Gizmo Corrections** — Phases 26-30 shipped 2026-04-09 ([archive](milestones/v1.4-ROADMAP.md))
- ✅ **v1.5 Solver Workflow Robustness + Script Reapply Integrity** — Phases 31-35 shipped 2026-04-13 ([archive](milestones/v1.5-ROADMAP.md))
- 🚧 **v1.6 Observable Flat JSONL Import for Large Geometry Dumps** — Phases 36-40 (planned)

## Phases

- [x] **Phase 36: Flat Import Entry & Configuration** - Users can start dedicated flat JSONL import and set transform/color options before execution. (completed 2026-04-27)
- [x] **Phase 37: Anchor-Scoped Flat Ingest** - Flat import creates one stable anchor and places imported entries as plain non-sketch entities. (completed 2026-04-27)
- [x] **Phase 38: Observable Link + Manual Transactional Refresh** - Anchor stores observer link metadata and supports manual safe refresh into same anchor. (completed 2026-04-27)
- [x] **Phase 39: Automatic Observer Safety Loop** - Optional live observer refresh runs with debounce/retry/auto-disable protections. (completed 2026-04-27)
- [ ] **Phase 40: Large-Dump Stability & Interaction Coherence** - Large import/refresh remains responsive and anchor interactions stay coherent across repeats.

## Phase Details

### Phase 36: Flat Import Entry & Configuration
**Goal**: Users can intentionally launch flat JSONL import for large dumps and configure import behavior before running it.  
**Depends on**: Phase 35  
**Requirements**: FIMP-01, FIMP-02  
**Success Criteria** (what must be TRUE):
  1. User can start flat JSONL import from a dedicated import menu/mode.
  2. User can set scale, rotation, shift, and color behavior before import.
  3. Import uses the options the user selected when execution begins.
**Plans**: 2 (complete)  
**UI hint**: yes

### Phase 37: Anchor-Scoped Flat Ingest
**Goal**: Imported flat JSONL data lands under one root anchor as plain scene entities (not sketch entities).  
**Depends on**: Phase 36  
**Requirements**: FIMP-03  
**Success Criteria** (what must be TRUE):
  1. A single root anchor is created for each flat import run.
  2. Imported entries appear as plain non-sketch scene entities under that anchor.
  3. Re-running import creates a new controlled flat import anchor structure rather than sketch-managed structures.
**Plans**: 2
Plans:
- [x] 37-01-PLAN.md — Add Phase 37 RED-first anchor-scoped ingest contract tests and CTest wiring.
- [x] 37-02-PLAN.md — Implement root/entry/geometry ingest hierarchy, naming collisions, reimport behavior, and selection invariance.

### Phase 38: Observable Link + Manual Transactional Refresh
**Goal**: Users can opt into observability and manually refresh an imported anchor with transactional safety.  
**Depends on**: Phase 37  
**Requirements**: OBSF-01, OBSF-02, OBSF-03, OBSF-05, PERF-02  
**Success Criteria** (what must be TRUE):
  1. User can enable/disable file observability at import time, and default is OFF.
  2. Imported anchor retains source-link metadata and replay settings needed for refresh.
  3. User can trigger manual refresh that re-imports into the same anchor identity.
  4. On refresh success, anchor content is replaced atomically; on refresh failure, last-good anchor content is preserved.
  5. Refresh path uses anchor-subtree replacement without sketch/script pipeline behavior.
**Plans**: 2
Plans:
- [x] 38-01-PLAN.md — Add Phase 38 RED-first observer metadata/inspector contracts and durable persistence wiring.
- [x] 38-02-PLAN.md — Implement inspector-driven background transactional flat refresh with same-root subtree replacement.

### Phase 39: Automatic Observer Safety Loop
**Goal**: Optional automatic refresh behaves safely during unstable file-write windows.  
**Depends on**: Phase 38  
**Requirements**: OBSF-04  
**Success Criteria** (what must be TRUE):
  1. When observability is enabled, file changes trigger automatic refresh attempts.
  2. Rapid consecutive file writes are debounced rather than causing refresh thrash.
  3. Failed automatic refresh attempts use retry behavior and then auto-disable when safety limits are hit.
**Plans**: 2
Plans:
- [x] 39-01-PLAN.md — Add Wave 0 auto-safety CTest scaffold and inspector safety control contract guards.
- [x] 39-02-PLAN.md — Implement flat auto observer safety loop runtime + inspector lifecycle semantics + OBSF-04 regressions.

### Phase 40: Large-Dump Stability & Interaction Coherence
**Goal**: Large-file flat import/refresh stays responsive and scene interactions remain coherent over repeated refreshes.  
**Depends on**: Phase 39  
**Requirements**: PERF-01, PERF-03  
**Success Criteria** (what must be TRUE):
  1. User can run large flat JSONL import and refresh without app lockups or crashes.
  2. During large import/refresh activity, app interaction remains responsive enough for continued operation.
  3. Across repeated refreshes, anchor selection, hierarchy visibility, and inspector interaction remain coherent.
**Plans**: 2
Plans:
- [ ] 40-01-PLAN.md — Implement bounded single-flight + coalesced rerun runtime with advisory timing evidence for large refresh stability.
- [ ] 40-02-PLAN.md — Add repeated-refresh anchor coherence coverage, hierarchy continuity guardrails, and manual very-large stress evidence protocol/checkpoint.

## Progress

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 36. Flat Import Entry & Configuration | 2/2 | Complete    | 2026-04-27 |
| 37. Anchor-Scoped Flat Ingest | 2/2 | Complete    | 2026-04-27 |
| 38. Observable Link + Manual Transactional Refresh | 2/2 | Complete | 2026-04-27 |
| 39. Automatic Observer Safety Loop | 2/2 | Complete    | 2026-04-27 |
| 40. Large-Dump Stability & Interaction Coherence | 0/TBD | Not started | - |
