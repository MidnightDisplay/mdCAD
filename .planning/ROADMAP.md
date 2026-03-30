# Roadmap: mdCAD

## Milestones

- ✅ **v1.0 Math Migration** — Phases 1-5 shipped 2026-03-26 ([archive](milestones/v1.0-ROADMAP.md))
- ✅ **v1.1 Long-Tail Migration** — Phases 6-9 shipped 2026-03-30 ([archive](milestones/v1.1-ROADMAP.md))
- 🚧 **v1.2 Sketches, Constraints, Scripting** — Phases 10-15 planned (current milestone)

## Active Roadmap (v1.2)

Milestone outcome: users can build constrained sketches, diagnose/resolve solver behavior, and round-trip sketch state through deterministic scripting workflows.

## Phases

- [ ] **Phase 10: Sketch Foundations & Managers** - Users can create sketches, attach/edit core geometry, and inspect sketch-level status.
- [ ] **Phase 11: Constraint Authoring UX** - Users can author, inspect, and edit legal constraints through manager and viewport workflows.
- [ ] **Phase 12: Solver Control & Constrained Interaction** - Users can control solving, read diagnostics, and manipulate geometry with constraints respected.
- [ ] **Phase 13: Script Round-Trip Baseline** - Users can open script editing and deterministically round-trip sketch scene state with Lua 5.4.x runtime.
- [ ] **Phase 14: Script IO + API/Undo Integration** - Users can drive sketches via script IO while API and undo/redo remain transactional.
- [ ] **Phase 15: Validation & Acceptance Closure** - Teams have shipped examples and platform gate evidence for v1.2 acceptance.

## Phase Details

### Phase 10: Sketch Foundations & Managers
**Goal**: Users can create and manage sketch containers with core sketch geometry and immediate sketch health visibility.
**Depends on**: Phase 9
**Requirements**: SKCH-01, SKCH-02, SKCH-03
**Success Criteria** (what must be TRUE):
  1. User can create a sketch and attach point, line, and arc/circle geometry to it.
  2. User can view per-sketch solve status, color policy, geometry count, and constraint count in Entity Inspector.
  3. User can single-select or multi-select sketch geometries in GeometryManager and fix, unfix, or delete them.
**Plans**: TBD
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
**Plans**: TBD
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
**Plans**: TBD
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
**Plans**: TBD
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
**Plans**: TBD
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

## Progress

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 10. Sketch Foundations & Managers | 0/TBD | Not started | - |
| 11. Constraint Authoring UX | 0/TBD | Not started | - |
| 12. Solver Control & Constrained Interaction | 0/TBD | Not started | - |
| 13. Script Round-Trip Baseline | 0/TBD | Not started | - |
| 14. Script IO + API/Undo Integration | 0/TBD | Not started | - |
| 15. Validation & Acceptance Closure | 0/TBD | Not started | - |
