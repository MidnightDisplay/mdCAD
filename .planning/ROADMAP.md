# Roadmap: mdCAD

## Milestones

- ✅ **v1.0 Math Migration** — Phases 1-5 shipped 2026-03-26 ([archive](milestones/v1.0-ROADMAP.md))
- ✅ **v1.1 Long-Tail Migration** — Phases 6-9 shipped 2026-03-30 ([archive](milestones/v1.1-ROADMAP.md))
- 🚧 **v1.2 Sketches, Constraints, Scripting** — Phases 10-16 planned (current milestone)

## Active Roadmap (v1.2)

Milestone outcome: users can build constrained sketches, diagnose/resolve solver behavior, and round-trip sketch state through deterministic scripting workflows.

## Phases

- [x] **Phase 10: Sketch Foundations & Managers** - Users can create sketches, attach/edit core geometry, and inspect sketch-level status. (completed 2026-03-30)
- [ ] **Phase 11: Constraint Authoring UX** - Users can author, inspect, and edit legal constraints through manager and viewport workflows.
- [x] **Phase 12: Solver Control & Constrained Interaction** - Users can control solving, read diagnostics, and manipulate geometry with constraints respected. (completed 2026-04-01)
- [x] **Phase 13: Script Round-Trip Baseline** - Users can open script editing and deterministically round-trip sketch scene state with Lua 5.4.x runtime. (completed 2026-04-01)
- [ ] **Phase 14: Script IO + API/Undo Integration** - Users can drive sketches via script IO while API and undo/redo remain transactional.
- [ ] **Phase 15: Validation & Acceptance Closure** - Teams have shipped examples and platform gate evidence for v1.2 acceptance.
- [x] **Phase 16: Constraint UX Closure & Verification** - Teams close Phase 11 audit gaps, harden glyph-selection highlighting behavior, and produce complete verification evidence. (completed 2026-04-01)

## Phase Details

### Phase 10: Sketch Foundations & Managers
**Goal**: Users can create and manage sketch containers with core sketch geometry and immediate sketch health visibility.
**Depends on**: Phase 9
**Requirements**: SKCH-01, SKCH-02, SKCH-03
**Success Criteria** (what must be TRUE):
  1. User can create a sketch and attach point, line, and arc/circle geometry to it.
  2. User can view per-sketch solve status, color policy, geometry count, and constraint count in Entity Inspector.
  3. User can single-select or multi-select sketch geometries in GeometryManager and fix, unfix, or delete them.
**Plans**: 3 plans
Plans:
- [x] 10-01-PLAN.md — Create sketch ECS contracts, scene helpers, and atomic bulk undo primitives.
- [x] 10-02-PLAN.md — Implement sketch creation/attachment UX and inspector sketch status surfaces.
- [x] 10-03-PLAN.md — Implement GeometryManager flat-list bulk actions and validation checklist evidence.
**UI hint**: yes

### Phase 11: Constraint Authoring UX
**Goal**: Users can create and inspect legal constraints through fast in-context and manager-driven workflows.
**Depends on**: Phase 10
**Requirements**: SKCH-04, CONS-01, CONS-02, CONS-03, CONS-04, CONS-05
**Success Criteria** (what must be TRUE):
  1. User can open a Tab-triggered in-context constraint menu that only shows currently applicable constraints and auto-hides after apply.
  2. User can apply the full v1.2 initial constraint set to geometrically legal entity/sub-entity combinations.
  3. User can hover/select constraints from constant-screen-size viewport glyphs anchored to constrained geometry.
  4. User can select any constraint and immediately see all participating geometry/sub-entities highlighted.
  5. User can create, view, and edit LENGTH/ANGLE constraints from viewport dimensions and ConstraintManager with mirrored values, and can mark them as driven.
**Plans**: 3 plans
Plans:
- [ ] 11-01-PLAN.md — Add constraint ECS contracts, naming counters, and scene lifecycle helpers.
- [x] 11-02-PLAN.md — Implement ConstraintManager list/filter/select/delete and dimensional/driven editing.
- [ ] 11-03-PLAN.md — Wire C-key in-context menu, constraint glyph picking, and dimension popup UX.
**UI hint**: yes

### Phase 12: Solver Control & Constrained Interaction
**Goal**: Users can control sketch solving, understand failures, and interact with geometry under active constraint rules.
**Depends on**: Phase 11
**Requirements**: SOLV-01, SOLV-02, SOLV-03, SOLV-04, API-03
**Success Criteria** (what must be TRUE):
  1. User can toggle auto-solve per sketch and manually trigger solve recalculation from solver controls.
  2. User can see sketch solve states (`solved`, `loose`, `fixed`, `error`) and timestamped diagnostics with `INFO`, `WARNING`, and `ERROR` levels.
  3. User can see which single solver backend type is active for v1.2 in sketch solver controls.
  4. User can identify implicated constraints/geometries when solve is invalid or fails.
  5. User manipulation/gizmo transforms respect active constraints during interaction.
**Plans**: 3 plans
Plans:
- [x] 12-01-PLAN.md — Implement per-sketch solver controls, status/diagnostics pipeline, and fixed backend display.
- [x] 12-02-PLAN.md — Implement failure implication highlighting/focus and constraint-respecting gizmo drag behavior.
- [x] 12-03-PLAN.md — Close verification gap by wiring clear-on-success implication lifecycle in active recalculate path.
**UI hint**: yes

### Phase 13: Script Round-Trip Baseline
**Goal**: Users can reconstruct supported sketch sub-scenes from script and get deterministic script updates from UI edits.
**Depends on**: Phase 12
**Requirements**: SCRP-01, SCRP-02, SCRP-03, SCRP-06
**Success Criteria** (what must be TRUE):
  1. User can open a standalone sketch script editor window from SketchManager.
  2. User can reconstruct entities, constraints, values, and links for the supported sketch sub-scene from script parse output.
  3. UI-side sketch/geometry/constraint edits update script output deterministically.
  4. Developers can run v1.2 sketch scripts on Lua 5.4.x runtime behavior as the locked scripting baseline.
**Plans**: 3 plans
Plans:
- [x] 13-01-PLAN.md — Add script identity/runtime contracts and Wave-0 script round-trip test target.
- [x] 13-02-PLAN.md — Implement declarative parser, atomic two-pass reconstruction, and deterministic emitter.
- [x] 13-03-PLAN.md — Wire standalone Script Editor UX with preview/apply flow and checkpoint verification.
**UI hint**: yes

### Phase 14: Script IO + API/Undo Integration
**Goal**: Users can safely apply script-driven edits with dynamic IO controls while scene API and undo/redo stay coherent.
**Depends on**: Phase 13
**Requirements**: SCRP-04, SCRP-05, API-01, API-02
**Success Criteria** (what must be TRUE):
  1. Script-side edits apply safely to scene state and preserve the last-valid sketch state on parse/apply failure.
  2. User can define script input/output variables and interact with generated numeric controls/readouts, including optional `min/max/step` slider behavior.
  3. Developers can use scene API entrypoints for sketch, geometry-manager, constraint-manager, and script workflows.
  4. User can undo/redo sketch, solver-impacting, and script-driven mutations transactionally without partial restores.
**Plans**: 3 plans
Plans:
- [ ] 14-01-PLAN.md — Add script-transaction undo command and atomic apply rollback guarantees.
- [ ] 14-02-PLAN.md — Implement numeric script IO schema and scene-level IO façade APIs.
- [ ] 14-03-PLAN.md — Wire dedicated Script IO window with live transactional auto-apply and UX verification.
**UI hint**: yes

### Phase 15: Validation & Acceptance Closure
**Goal**: v1.2 ships with verification evidence, reference examples, and platform validation records.
**Depends on**: Phase 14
**Requirements**: VAL-01, VAL-02, VAL-03
**Success Criteria** (what must be TRUE):
  1. Project ships example sketch/script case studies, including constraint-focused samples for development and debugging.
  2. v1.2 feature acceptance gates pass on Windows MSVC + Vulkan.
  3. macOS parity validation is executed after Windows gate pass and results are recorded.
**Plans**: TBD

### Phase 16: Constraint UX Closure & Verification
**Goal**: Close outstanding Phase 11 audit gaps by hardening constraint glyph selection behavior and producing complete Phase 11 verification/validation evidence.
**Depends on**: Phase 11
**Requirements**: SKCH-04, CONS-01, CONS-02, CONS-03, CONS-04, CONS-05
**Gap Closure:** Closes orphaned requirement evidence from `v1.2-MILESTONE-AUDIT.md` and the 10→11 integration/flow gap on glyph participant highlighting.
**Success Criteria** (what must be TRUE):
  1. Constraint glyph click/selection highlights all participant geometry immediately and consistently.
  2. Constraint authoring/editing flows in manager and viewport are re-verified against SKCH-04 and CONS-01..05.
  3. `11-VERIFICATION.md` exists with requirement-level evidence and no orphaned Phase 11 requirements.
  4. `11-VALIDATION.md` is updated from draft to a compliant state for implemented scope.
**Plans**: TBD
**UI hint**: yes

## Progress

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 10. Sketch Foundations & Managers | 3/3 | Complete    | 2026-03-30 |
| 11. Constraint Authoring UX | 1/3 | In Progress|  |
| 12. Solver Control & Constrained Interaction | 3/3 | Complete    | 2026-04-01 |
| 13. Script Round-Trip Baseline | 3/3 | Complete   | 2026-04-01 |
| 14. Script IO + API/Undo Integration | 0/TBD | Not started | - |
| 15. Validation & Acceptance Closure | 0/TBD | Not started | - |
| 16. Constraint UX Closure & Verification | 2/2 | Complete    | 2026-04-01 |
