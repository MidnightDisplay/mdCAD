# Requirements: mdCAD

**Defined:** 2026-03-24
**Core Value:** Interactive geometry editing and rendering must remain stable, responsive, and trustworthy on supported native platforms while the math foundation evolves underneath it.

## v1 Requirements

Requirements for the first math-foundation migration milestone. Each maps to roadmap phases.

### Adoption Foundation

- [x] **FOUND-01**: mdCAD can vendor and build the selected MIT-licensed C math library within the current native build workflow without adding a C++ dependency
- [x] **FOUND-02**: mdCAD exposes a thin project-owned math entrypoint for convention/config includes without re-wrapping vendor math types during the staged rollout
- [x] **FOUND-03**: mdCAD documents and enforces its matrix layout, handedness, clipspace, and alignment strategy in one place before hotspot migration begins

### Hot-Path Migration

- [x] **HOT-01**: mdCAD migrates orbit camera and viewport matrix construction to the new math foundation on the macOS Metal build without user-visible regressions
- [x] **HOT-02**: mdCAD migrates ECS transform composition and world-matrix-dependent rendering math to the new foundation on the macOS Metal build without user-visible regressions
- [x] **HOT-03**: mdCAD migrates pick/unproject/ray and translation gizmo drag math to the new foundation on the macOS Metal build without user-visible regressions
- [ ] **HOT-04**: The same migrated hot paths build and behave correctly on the Windows Vulkan path

### Performance & Validation

- [x] **PERF-01**: mdCAD includes repeatable regression and benchmark checks for migrated math hotspots
- [ ] **PERF-02**: Migrated hot paths show no native performance regression on macOS Metal
- [ ] **PERF-03**: Migrated hot paths show no native performance regression on Windows Vulkan

### Foundation Expansion

- [ ] **EXP-01**: The adopted math foundation exposes quaternion and broader transform/helper capabilities for future mdCAD work
- [ ] **EXP-02**: Migrated subsystems can retire equivalent `src/math3d.h` helpers without losing required runtime functionality

## v2 Requirements

Deferred until the first native migration milestone is proven.

### Long-Tail Migration

- **TAIL-01**: mdCAD migrates lower-priority math consumers such as serializer, importers, undo/redo helpers, and editor utility code to the new foundation
- **TAIL-02**: mdCAD removes or substantially shrinks temporary thin entrypoint glue once staged migration is complete and safe

### Platform Expansion

- **PLAT-01**: mdCAD validates the new math foundation on iOS native builds
- **PLAT-02**: mdCAD validates the new math foundation on the web build and resolves any Web/WASM-specific math or alignment issues

## Out of Scope

Explicitly excluded from the first migration milestone.

| Feature | Reason |
|---------|--------|
| Repo-wide big-bang replacement of `src/math3d.h` | Too risky for a math layer used across camera, ECS, rendering, picking, gizmo, importers, and serializer code |
| Adoption of a C++ math library | Conflicts with the project’s explicit C-only requirement |
| iOS or web parity as a phase-1 gate | User explicitly deferred those targets until macOS and Windows Vulkan are stable |
| Using multiple vendor math libraries at once | Would increase convention and debugging complexity during migration |

## Traceability

Which phases cover which requirements. Updated during roadmap creation.

| Requirement | Phase | Status |
|-------------|-------|--------|
| FOUND-01 | Phase 1 | Complete |
| FOUND-02 | Phase 2 | Complete |
| FOUND-03 | Phase 1 | Complete |
| HOT-01 | Phase 3 | Complete |
| HOT-02 | Phase 3 | Complete |
| HOT-03 | Phase 4 | Complete |
| HOT-04 | Phase 5 | Pending |
| PERF-01 | Phase 2 | Complete |
| PERF-02 | Phase 5 | Pending |
| PERF-03 | Phase 5 | Pending |
| EXP-01 | Phase 4 | Pending |
| EXP-02 | Phase 4 | Pending |

**Coverage:**
- v1 requirements: 12 total
- Mapped to phases: 12
- Unmapped: 0 ✓

---
*Requirements defined: 2026-03-24*
*Last updated: 2026-03-24 after Phase 2 completion*
