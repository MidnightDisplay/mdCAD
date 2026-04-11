# Roadmap: mdCAD

## Milestones

- ✅ **v1.0 Math Migration** — Phases 1-5 shipped 2026-03-26 ([archive](milestones/v1.0-ROADMAP.md))
- ✅ **v1.1 Long-Tail Migration** — Phases 6-9 shipped 2026-03-30 ([archive](milestones/v1.1-ROADMAP.md))
- ✅ **v1.2 Sketches, Constraints, Scripting** — Phases 10-21 shipped 2026-04-07 ([archive](milestones/v1.2-ROADMAP.md))
- ✅ **v1.3 Sketch Solver Audit + Constraint Expansion** — Phases 22-25 shipped 2026-04-08 ([archive](milestones/v1.3-ROADMAP.md))
- ✅ **v1.4 Solver Robustness + Sketch Gizmo Corrections** — Phases 26-30 shipped 2026-04-09 ([archive](milestones/v1.4-ROADMAP.md))
- 🚧 **v1.5 Solver Workflow Robustness + Script Reapply Integrity** — Phases 31-34 (in progress)

## Phases

**Phase Numbering:**
- Integer phases (31, 32, 33, 34): Planned milestone work
- Decimal phases (31.1, 31.2): Urgent insertions (if needed later)

- [x] **Phase 31: Script Reapply Fidelity Foundation** - Preserve script remap semantics and metadata so replayed scenes stay valid and deterministic. (completed 2026-04-10)
- [x] **Phase 32: Explicit Coincidence Authoring Semantics** - Make ArcAxisLine and endpoint tangency authoring explicit and stable via required coincidence intent. (completed 2026-04-10)
- [x] **Phase 33: Large-Jump Robustness and PARALLEL/ALONG Parity** - Stabilize mixed arc/line large edits and parity behavior across equivalent constraint setups. (completed 2026-04-10)
- [ ] **Phase 34: Deterministic v1.5 Closure Gate** - Lock reproducible deterministic sign-off for targeted v1.5 regression coverage.

## Phase Details

### Phase 31: Script Reapply Fidelity Foundation
**Goal**: Users can re-apply scripts without losing constraint participant intent or visual metadata, and replay remains stable across repeated runs.
**Depends on**: Phase 30
**Requirements**: SCRI-01, SCRI-02, SCRI-03
**Success Criteria** (what must be TRUE):
  1. User can re-apply a valid script and constraint participants keep role/sub-index intent instead of degrading into unsupported-participant failures.
  2. User can re-apply a script and entity color metadata is preserved rather than resetting to default white.
  3. User can re-apply the same script repeatedly and get stable deterministic geometry and constraint outcomes each run.
**Plans**: 3 plans

Plans:
- [x] 31-01-PLAN.md — Establish shared script+solver capability registry and deterministic contract failure taxonomy.
- [x] 31-02-PLAN.md — Implement descriptor/color fidelity pipeline with repeat-apply determinism parity gates.

### Phase 32: Explicit Coincidence Authoring Semantics
**Goal**: Users get explicit, durable coincidence semantics when authoring composite ArcAxisLine and line-end/arc-end tangency relations.
**Depends on**: Phase 31
**Requirements**: COIN-01, COIN-02
**Success Criteria** (what must be TRUE):
  1. User authoring ArcAxisLine sees required center/axis coincidence represented explicitly rather than relying on implicit coupling.
  2. User authoring line-end/arc-end tangency gets explicit endpoint coincidence semantics that remain intact after subsequent edits.
  3. User can continue editing sketches containing these composite relations without hidden coupling drift or surprise relation breakage.
**Plans**: 2 plans

Plans:
- [x] 32-01-PLAN.md — Implement explicit composite authoring semantics and pair-link metadata plumbing for ArcAxisLine/tangency.
- [x] 32-02-PLAN.md — Enforce lifecycle durability, diagnostics, and deterministic script pair-link persistence with rerun evidence.

### Phase 33: Large-Jump Robustness and PARALLEL/ALONG Parity
**Goal**: Users can perform large-jump and mirrored linked edits in mixed constrained sketches with deterministic, parity-consistent outcomes.
**Depends on**: Phase 32
**Requirements**: SROB-01, SROB-02, SROB-03, DIAG-01, DIAG-02, PARI-01, PARI-02
**Success Criteria** (what must be TRUE):
  1. User can perform large-jump edits in quarter-arc closed-loop line/arc arrangements without manual "wiggle to latch" behavior.
  2. User making large coupled length changes either gets a fully solved update of dependent participants or a transactional rollback with no partial corruption.
  3. User can immediately continue editing after an infeasible large-jump attempt without deadlock or stale-failure lock behavior.
  4. User gets deterministic outcomes and actionable failure diagnostics for identical operation sequences, including large-jump and mixed-constraint cases.
  5. User interacting with geometrically equivalent PARALLEL and ALONG arrangements gets consistent drag feasibility and parity behavior.
**Plans**: 2 plans

Plans:
- [x] 33-01-PLAN.md — Implement two-stage large-jump staging with transactional rollback/recovery and deterministic failure ordering.
- [x] 33-02-PLAN.md — Enforce PARALLEL/ALONG equivalence parity matrix and actionable deterministic diagnostics for mixed-constraint failures.
- [x] 33-03-PLAN.md — Close UAT PARALLEL drag-authority gap with equal-priority bidirectional motion policy and deterministic regressions.

### Phase 34: Deterministic v1.5 Closure Gate
**Goal**: Developers and users can trust v1.5 reliability claims through reproducible deterministic baseline and immediate rerun evidence.
**Depends on**: Phase 33
**Requirements**: DIAG-03
**Success Criteria** (what must be TRUE):
  1. Developer can run the v1.5 targeted regression gate and obtain a deterministic baseline pass result.
  2. Developer can immediately rerun the identical gate command and observe parity with the baseline outcome.
  3. Milestone closure evidence demonstrates deterministic sign-off for the v1.5 targeted workflow set.
**Plans**: 2 plans

Plans:
- [x] 34-01-PLAN.md — Lock canonical deterministic closure contract and capture baseline + immediate rerun DIAG-03 evidence.
- [ ] 34-02-PLAN.md — Stabilize failing canonical gate tests and re-capture 7/7 baseline + immediate rerun parity evidence for DIAG-03 closure.

## Progress

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 31. Script Reapply Fidelity Foundation | 2/2 | Complete | 2026-04-10 |
| 32. Explicit Coincidence Authoring Semantics | 2/2 | Complete | 2026-04-10 |
| 33. Large-Jump Robustness and PARALLEL/ALONG Parity | 3/3 | Complete    | 2026-04-11 |
| 34. Deterministic v1.5 Closure Gate | 1/2 | In Progress | - |

