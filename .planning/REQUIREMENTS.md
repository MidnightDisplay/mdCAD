# Requirements: mdCAD

**Defined:** 2026-03-30
**Core Value:** Interactive geometry editing and rendering must remain stable, responsive, and trustworthy on supported native platforms while the math foundation evolves underneath it.

## v1.2 Requirements

Requirements for the sketches/constraints/scripting milestone. Each maps to roadmap phases.

### Sketch Core

- [x] **SKCH-01**: User can create a sketch entity and attach point, line, and arc/circle geometry to that sketch.
- [x] **SKCH-02**: User can view per-sketch solve status, color policy, geometry count, and constraint count in the Entity Inspector.
- [x] **SKCH-03**: User can fix, unfix, and delete sketch geometries from GeometryManager using single-select and multi-select workflows.
- [x] **SKCH-04**: User can select a constraint and see all participating geometry entities/sub-entities highlighted.

### Constraint Authoring

- [x] **CONS-01**: User can apply the initial constraint set (`FIXED`, `COINCIDENT`, `COLLINEAR`, `PARALLEL`, `PERPENDICULAR`, `ALONG X`, `ALONG Y`, `ALONG Z`, `CORADIAL`, `CONCENTRIC`, `LENGTH`, `ANGLE`, `TANGENTIAL`) to geometrically legal entity types.
- [x] **CONS-02**: User can open a Tab-triggered in-context constraint menu that lists only currently applicable constraints and auto-hides after applying one.
- [x] **CONS-03**: User can hover and select constraints through constant-screen-size viewport glyphs anchored to constrained geometry/sub-geometry.
- [x] **CONS-04**: User can create, view, and edit `LENGTH`/`ANGLE` constraints both from viewport dimensions and ConstraintManager with mirrored values.
- [x] **CONS-05**: User can mark `LENGTH`/`ANGLE` constraints as driven so they remain visible/readable but do not drive solver equations.

### Solver Control

- [x] **SOLV-01**: User can toggle auto-solve per sketch and manually trigger solve recalculation from Solver controls.
- [x] **SOLV-02**: User can see sketch solve states (`solved`, `loose`, `fixed`, `error`) and timestamped solver diagnostics with `INFO`, `WARNING`, and `ERROR` levels.
- [x] **SOLV-03**: The system uses one solver backend type for v1.2 and exposes that active type in sketch solver controls.
- [x] **SOLV-04**: User can identify implicated constraints/geometries when a solve fails or is invalid.

### Scripting

- [x] **SCRP-01**: User can open a standalone sketch script editor window from SketchManager.
- [x] **SCRP-02**: User can reconstruct the full sketch sub-scene (entities, constraints, values, and links) from script parse output.
- [x] **SCRP-03**: UI-side sketch/geometry/constraint edits update script output deterministically.
- [x] **SCRP-04**: Script-side edits update the scene safely while preserving last-valid sketch state on parse/apply errors.
- [ ] **SCRP-05**: User can define script input/output variables and interact with dynamically generated numeric controls/readouts, including optional `min/max/step` slider behavior.
- [x] **SCRP-06**: v1.2 scripting runtime is Lua 5.4.x.

### Integration and Validation

- [ ] **API-01**: Developers can use scene API entrypoints for sketch, geometry-manager, constraint-manager, and script workflows.
- [x] **API-02**: User can undo/redo sketch, solver-impacting, and script-driven mutations transactionally without partial state restores.
- [x] **API-03**: User manipulation/gizmo transforms respect active constraints during interaction.
- [ ] **VAL-01**: The project ships example sketch/script case studies, including constraint-focused samples for development and debugging.
- [ ] **VAL-02**: v1.2 feature acceptance gates pass on Windows MSVC + Vulkan.
- [ ] **VAL-03**: macOS parity validation is executed after Windows gate pass and results are recorded.

## v1.3+ Requirements (Deferred)

### Platform Expansion

- **PLAT-01**: mdCAD validates sketch/constraint/scripting runtime flows on iOS native builds.
- **PLAT-02**: mdCAD validates sketch/constraint/scripting runtime flows on the web build (Emscripten) and closes runtime-specific gaps.

### Capability Expansion

- **CAP-01**: mdCAD supports multi-sketch dependency solving and cross-sketch constraint relations.
- **CAP-02**: mdCAD supports multiple solver backend choices and migration-safe backend switching.
- **CAP-03**: mdCAD expands script language/runtime features beyond deterministic sketch-parametric workflows.

## Out of Scope

Explicitly excluded from v1.2 to prevent scope creep.

| Feature | Reason |
|---------|--------|
| Equal-priority iOS/web validation gates during active development | v1.2 delivery is explicitly Windows MSVC+Vulkan first |
| Multi-sketch/global solve graph | Too large for first integrated constraint+scripting milestone |
| Multi-backend solver selection UX | v1.2 requires one backend only to reduce integration risk |
| General-purpose scripting platform features (modules/filesystem/async) | Not required for deterministic sketch bidirectional workflows |

## Traceability

Which phases cover which requirements. Updated during roadmap creation.

| Requirement | Phase | Status |
|-------------|-------|--------|
| SKCH-01 | Phase 10 | Complete |
| SKCH-02 | Phase 10 | Complete |
| SKCH-03 | Phase 10 | Complete |
| SKCH-04 | Phase 16 | Complete |
| CONS-01 | Phase 16 | Complete |
| CONS-02 | Phase 16 | Complete |
| CONS-03 | Phase 16 | Complete |
| CONS-04 | Phase 16 | Complete |
| CONS-05 | Phase 16 | Complete |
| SOLV-01 | Phase 12 | Complete |
| SOLV-02 | Phase 12 | Complete |
| SOLV-03 | Phase 12 | Complete |
| SOLV-04 | Phase 12 | Complete |
| SCRP-01 | Phase 13 | Complete |
| SCRP-02 | Phase 13 | Complete |
| SCRP-03 | Phase 13 | Complete |
| SCRP-04 | Phase 14 | Complete |
| SCRP-05 | Phase 14 | Pending |
| SCRP-06 | Phase 13 | Complete |
| API-01 | Phase 14 | Pending |
| API-02 | Phase 14 | Complete |
| API-03 | Phase 12 | Complete |
| VAL-01 | Phase 15 | Pending |
| VAL-02 | Phase 15 | Pending |
| VAL-03 | Phase 15 | Pending |

**Coverage:**
- v1.2 requirements: 25 total
- Mapped to phases: 25 ✅
- Unmapped: 0 ✅

---
*Requirements defined: 2026-03-30*
*Last updated: 2026-04-01 after v1.2 milestone gap-plan remap*
