# Requirements: mdCAD

**Defined:** 2026-04-08
**Core Value:** Interactive geometry editing and rendering must remain stable, responsive, and trustworthy on supported native platforms while the math foundation evolves underneath it.

## v1.4 Requirements

Requirements for the solver robustness and sketch-line gizmo correction milestone.

### Line-Line Constraint Coverage

- [x] **LCON-01**: User can apply `PARALLEL` between two sketch lines and get a solved result when geometry is feasible.
- [x] **LCON-02**: User can apply `PARALLEL` to multi-line groups and all participating lines remain parallel after solve.
- [x] **LCON-03**: User can apply `PERPENDICULAR` between two sketch lines and get a solved result when geometry is feasible.
- [x] **LCON-04**: User can apply `PERPENDICULAR` to supported multi-line selections with deterministic runtime behavior.
- [x] **LCON-05**: User gets explicit legality feedback for invalid line-line constraint selections instead of solver-side ambiguous failure.

### Principal-Axis Line Semantics

- [x] **ALIN-01**: User can apply `ALONG X` to sketch lines without immediate unsatisfied-driving solver failure.
- [x] **ALIN-02**: User can apply `ALONG Y` to sketch lines without immediate unsatisfied-driving solver failure.
- [x] **ALIN-03**: User can apply `ALONG Z` to sketch lines without immediate unsatisfied-driving solver failure.
- [x] **ALIN-04**: User can combine line `ALONG` constraints with `LENGTH`, `ANGLE`, and connectivity constraints and still get deterministic solve outcomes in feasible cases.

### Arc-Line Tangency and Drag Robustness

- [x] **TRDG-01**: User can author line-end/arc-end tangency in common fillet-like corner setups and receive stable solved geometry when feasible.
- [x] **TRDG-02**: User can drag shared and adjacent participants in tangency-constrained setups without solver deadlock in feasible cases.
- [x] **TRDG-03**: User receives transactional rollback and clear diagnostics for infeasible tangency edits, with solver responsiveness preserved for subsequent edits.
- [x] **TRDG-04**: User gets consistent drag feasibility outcomes for equivalent mirrored interactions in mixed-constraint sketches.

### Active-Sketch Line Gizmo Behavior

- [x] **GZM-01**: User sees the gizmo anchored at the midpoint of the selected line when that line belongs to the active sketch.
- [x] **GZM-02**: User moving an active-sketch line with the gizmo updates the line geometry endpoints (`A` and `B`) in sync as a rigid translation.
- [x] **GZM-03**: User sees active-sketch line endpoint-driven gizmo behavior only for active-sketch lines; non-active or non-line selections keep existing semantics.
- [x] **GZM-04**: User can undo and redo a completed active-sketch line gizmo drag as a coherent single interaction that restores exact endpoint geometry.

### Solver Architecture Documentation

- [x] **SDOC-01**: Developer can read an easy-to-follow solver architecture overview that maps authoring, solve, diagnostics, and UI feedback flow.
- [x] **SDOC-02**: Developer can use references to relevant literature and direct code-structure anchors (files/functions) to understand implementation intent.
- [x] **SDOC-03**: Developer can use a TL;DR primer that explains how key constraints are implemented and where to start when debugging failures.

### Validation and Regression

- [x] **V14-01**: Developer can run targeted automated tests covering new line-line constraints, line ALONG semantics, tangency robustness, and active-sketch line gizmo behavior.
- [x] **V14-02**: Developer can run milestone closure reruns on Windows Vulkan and obtain deterministic pass results suitable for sign-off.

## v1.5+ Requirements (Deferred)

### Capability Expansion

- **CAP-01**: mdCAD supports multi-sketch dependency solving and cross-sketch constraint relations.
- **CAP-02**: mdCAD supports runtime-selectable solver backend choices and migration-safe backend switching.
- **CAP-03**: mdCAD expands script language/runtime features beyond deterministic sketch-parametric workflows.

### Platform Expansion

- **PLAT-01**: mdCAD validates sketch/constraint runtime flows on iOS native builds.
- **PLAT-02**: mdCAD validates sketch/constraint runtime flows on web builds (Emscripten) and closes runtime-specific gaps.

## Out of Scope

Explicitly excluded from v1.4 to avoid scope creep.

| Feature | Reason |
|---------|--------|
| Full solver architecture rewrite | v1.4 is a robustness and correctness cycle over the existing solver structure |
| New external nonlinear solver dependency | Adds integration risk without being necessary for current reported failures |
| Broad platform validation expansion beyond Windows Vulkan gate | Would dilute focus from the user-reported reliability blockers |
| Major UI framework or interaction model replacement | v1.4 targets constrained fixes to current gizmo and solver behavior |

## Traceability

Which phases cover which requirements. Populated during roadmap creation.

| Requirement | Phase | Status |
|-------------|-------|--------|
| LCON-01 | Phase 26 | Complete |
| LCON-02 | Phase 26 | Complete |
| LCON-03 | Phase 26 | Complete |
| LCON-04 | Phase 26 | Complete |
| LCON-05 | Phase 26 | Complete |
| ALIN-01 | Phase 27 | Complete |
| ALIN-02 | Phase 27 | Complete |
| ALIN-03 | Phase 27 | Complete |
| ALIN-04 | Phase 27 | Complete |
| TRDG-01 | Phase 28 | Complete |
| TRDG-02 | Phase 28 | Complete |
| TRDG-03 | Phase 28 | Complete |
| TRDG-04 | Phase 28 | Complete |
| GZM-01 | Phase 29 | Complete |
| GZM-02 | Phase 29 | Complete |
| GZM-03 | Phase 29 | Complete |
| GZM-04 | Phase 29 | Complete |
| SDOC-01 | Phase 30 | Complete |
| SDOC-02 | Phase 30 | Complete |
| SDOC-03 | Phase 30 | Complete |
| V14-01 | Phase 30 | Complete |
| V14-02 | Phase 30 | Complete |

**Coverage:**
- v1.4 requirements: 22 total
- Mapped to phases: 22
- Unmapped: 0 ✓

---
*Requirements defined: 2026-04-08*
*Last updated: 2026-04-08 after v1.4 milestone initialization*
