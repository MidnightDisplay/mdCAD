# Requirements: mdCAD

**Defined:** 2026-04-07
**Core Value:** Interactive geometry editing and rendering must remain stable, responsive, and trustworthy on supported native platforms while the math foundation evolves underneath it.

## v1.3 Requirements

Requirements for the sketch solver audit and advanced constraint expansion milestone.

### Solver Reliability

- [ ] **SRLV-01**: User can rely on auto-solve to trigger after committed sketch mutations, including constraint add/remove/edit and geometry move operations.
- [ ] **SRLV-02**: User can run manual recalculate and get deterministic, idempotent results for unchanged sketch state.
- [ ] **SRLV-03**: Solver runs iterative passes until tolerance is satisfied or max pass count is reached, with configurable pass cap (default `10`).
- [ ] **SRLV-04**: Recalculate remains functional when `ALONG X`, `ALONG Y`, `ALONG Z`, and `ANGLE` constraints are present.
- [ ] **SRLV-05**: Driving `LENGTH` and `ANGLE` constraints produce expected geometric effects or explicit failure diagnostics.

### Principal-Direction Group Constraints

- [ ] **AXIS-01**: User can apply `ALONG X` to pairs and groups of point participants to constrain them along principal X direction.
- [ ] **AXIS-02**: User can apply `ALONG Y` to pairs and groups of point participants to constrain them along principal Y direction.
- [ ] **AXIS-03**: User can apply `ALONG Z` to pairs and groups of point participants to constrain them along principal Z direction.
- [ ] **AXIS-04**: Directional group constraints support standalone points, line endpoints, and arc landmark points.

### Advanced Arc and Line-Arc Constraints

- [ ] **ARCI-01**: User can constrain an arc-center axis relative to a line so the arc orientation follows the perpendicular-to-line authoring contract.
- [ ] **ARCI-02**: User can constrain line-end and arc-end tangency at a shared point, with solver preserving tangency by moving geometric positions as needed.
- [ ] **ARCI-03**: User can apply and edit an angle constraint between a single arc's start and end points.
- [ ] **ARCI-04**: Advanced arc/line-arc constraints solve deterministically and surface explicit diagnostics when not satisfiable.

### Validation and Regression

- [ ] **V13-01**: Developers can run automated regression coverage for trigger integrity, iterative pass behavior, and new constraint legality/solve semantics.

## v1.4+ Requirements (Deferred)

### Platform Expansion

- **PLAT-01**: mdCAD validates sketch/constraint/scripting runtime flows on iOS native builds.
- **PLAT-02**: mdCAD validates sketch/constraint/scripting runtime flows on the web build (Emscripten) and closes runtime-specific gaps.

### Capability Expansion

- **CAP-01**: mdCAD supports multi-sketch dependency solving and cross-sketch constraint relations.
- **CAP-02**: mdCAD supports multiple solver backend choices and migration-safe backend switching.
- **CAP-03**: mdCAD expands script language/runtime features beyond deterministic sketch-parametric workflows.

## Out of Scope

Explicitly excluded from v1.3 to avoid scope creep.

| Feature | Reason |
|---------|--------|
| Multi-sketch/global solve graph | Solver reliability and single-sketch correctness must be stabilized first |
| Runtime-selectable solver backends in UI | Increases integration risk during reliability-focused milestone |
| Unbounded iterative solve loops | Violates deterministic and performance-safe solve behavior |
| General-purpose scripting features (modules/filesystem/async) | Not required for solver audit and constraint expansion goals |

## Traceability

Which phases cover which requirements. Updated during roadmap creation.

| Requirement | Phase | Status |
|-------------|-------|--------|
| SRLV-01 | TBD | Pending |
| SRLV-02 | TBD | Pending |
| SRLV-03 | TBD | Pending |
| SRLV-04 | TBD | Pending |
| SRLV-05 | TBD | Pending |
| AXIS-01 | TBD | Pending |
| AXIS-02 | TBD | Pending |
| AXIS-03 | TBD | Pending |
| AXIS-04 | TBD | Pending |
| ARCI-01 | TBD | Pending |
| ARCI-02 | TBD | Pending |
| ARCI-03 | TBD | Pending |
| ARCI-04 | TBD | Pending |
| V13-01 | TBD | Pending |

**Coverage:**
- v1.3 requirements: 14 total
- Mapped to phases: 0
- Unmapped: 14 ⚠️

---
*Requirements defined: 2026-04-07*
*Last updated: 2026-04-07 after v1.3 requirement draft*
