# Roadmap: mdCAD

## Overview

This roadmap modernizes mdCAD’s math foundation without destabilizing the native workflows that already work. The path is deliberately staged: choose and integrate the replacement library first, establish a thin project-owned math entrypoint plus validation tooling, migrate the most performance- and interaction-sensitive subsystems on macOS first, then prove the same hotspot set on Windows Vulkan before widening the rollout or chasing full repo cleanup.

## Phases

**Phase Numbering:**
- Integer phases (1, 2, 3): Planned milestone work
- Decimal phases (2.1, 2.2): Urgent insertions (marked with INSERTED)

Decimal phases appear between their surrounding integers in numeric order.

- [x] **Phase 1: Selection and Conventions** - Lock `cglm` `0.9.6`, vendor it lightly, and define mdCAD’s math conventions (completed 2026-03-24)
- [x] **Phase 2: Direct Adoption Tooling and Validation Harness** - Build the thin project-owned math entrypoint and comparison tooling for direct cglm adoption (completed 2026-03-24)
- [ ] **Phase 3: macOS Core Transform Migration** - Migrate camera, transforms, and render-matrix hotspots on the primary native path
- [ ] **Phase 4: Interaction Math and API Expansion** - Migrate picking/gizmo math and introduce richer helper coverage
- [ ] **Phase 5: Windows Vulkan Hardening and Performance Gates** - Validate the migrated hotspot set on Windows and close native performance gates

## Phase Details

### Phase 1: Selection and Conventions
**Goal**: Lock `cglm` `0.9.6`, vendor it with minimal build disruption, and define the conventions the migration must preserve
**Depends on**: Nothing (first phase)
**Requirements**: FOUND-01, FOUND-03
**Canonical refs**: `.planning/PROJECT.md`, `.planning/research/STACK.md`, `.planning/research/SUMMARY.md`, `src/math3d.h`, `src/platform.h`, `CMakeLists.txt`, `src/CMakeLists.txt`
**Success Criteria** (what must be TRUE):
  1. mdCAD has one chosen MIT-licensed C math library that builds in the current native workflow without introducing C++
  2. Matrix layout, handedness, clipspace, and alignment policy are documented in one project-owned place
  3. The adoption mode for the chosen library is decided clearly enough to start building a thin project-owned math entrypoint
**Plans**: 3 plans

Plans:
- [x] 01-01: Reconcile roadmap, requirements, and repo docs with the locked `cglm` decision
- [x] 01-02: Vendor the chosen library and wire the minimal include/build path
- [x] 01-03: Define mdCAD convention and alignment policy for the migration

### Phase 2: Direct Adoption Tooling and Validation Harness
**Goal**: Create a thin project-owned math entrypoint and the validation tools needed to compare old and new behavior safely during direct cglm adoption
**Depends on**: Phase 1
**Requirements**: FOUND-02, PERF-01
**Canonical refs**: `.planning/PROJECT.md`, `.planning/research/ARCHITECTURE.md`, `.planning/research/PITFALLS.md`, `src/math3d.h`, `src/orbit_camera.h`, `src/ecs/ecs_scene.h`, `src/gpu/geometry_batch.h`, `src/gpu/pick_buffer.h`
**Success Criteria** (what must be TRUE):
  1. mdCAD has a thin project-owned math entrypoint that centralizes convention/config includes without re-wrapping vendor math types
  2. Regression and benchmark entry points exist for the migrated hotspot set
  3. The migration can be rolled forward subsystem by subsystem instead of as one repo-wide change
**Plans**: 3 plans

Plans:
- [x] 02-01: Expand the `src/math/` thin project-owned math entrypoint for direct cglm adoption
- [x] 02-02: Add old/new comparison utilities and benchmark scaffolding for hotspot math
- [x] 02-03: Wire regression hooks or smoke scenes into the native validation workflow for direct cglm adoption

### Phase 3: macOS Core Transform Migration
**Goal**: Move the core camera and transform/render-matrix math to the new foundation on the primary native path
**Depends on**: Phase 2
**Requirements**: HOT-01, HOT-02
**Canonical refs**: `.planning/research/SUMMARY.md`, `src/orbit_camera.h`, `src/app.c`, `src/ecs/ecs_scene.h`, `src/gpu/geometry_batch.h`, `src/components/transform_comp.h`
**Success Criteria** (what must be TRUE):
  1. Orbit camera and viewport matrix behavior match the current macOS Metal experience
  2. ECS transform composition and render-matrix-dependent drawing paths produce correct scene output on macOS
  3. The migrated core transform path remains regression-free in native smoke checks
**Plans**: 3 plans

Plans:
- [ ] 03-01: Migrate orbit camera and viewport matrix construction
- [ ] 03-02: Migrate ECS transform composition and render-matrix usage
- [ ] 03-03: Validate render and navigation parity on the active macOS workflow

### Phase 4: Interaction Math and API Expansion
**Goal**: Migrate interaction-sensitive math and start using the richer helper surface of the adopted library
**Depends on**: Phase 3
**Requirements**: HOT-03, EXP-01, EXP-02
**Canonical refs**: `.planning/research/FEATURES.md`, `.planning/research/PITFALLS.md`, `src/gpu/pick_buffer.h`, `src/gizmo/gizmo.h`, `src/gizmo/gizmo_rendering.h`, `src/gizmo/gizmo_vertex_mode.h`, `src/math3d.h`
**Success Criteria** (what must be TRUE):
  1. Pick/unproject/ray math and translation gizmo drag math run through the new foundation without interaction regressions on macOS
  2. mdCAD exposes quaternion and broader transform/helper capability behind the project-owned math boundary
  3. Migrated subsystems no longer depend on equivalent `src/math3d.h` helpers
**Plans**: 3 plans

Plans:
- [ ] 04-01: Migrate pick/unproject/ray math to the new foundation
- [ ] 04-02: Migrate gizmo drag/intersection math and validate interaction feel
- [ ] 04-03: Add expanded helper coverage and retire redundant local helpers in migrated areas

### Phase 5: Windows Vulkan Hardening and Performance Gates
**Goal**: Prove the migrated hotspot set is stable on Windows Vulkan and meets the native performance goals on both supported validation targets
**Depends on**: Phase 4
**Requirements**: HOT-04, PERF-02, PERF-03
**Canonical refs**: `.planning/research/SUMMARY.md`, `.planning/research/PITFALLS.md`, `src/platform.h`, `src/gpu/pick_buffer.h`, `src/gpu/geometry_batch.h`, `docs/VULKAN_WINDOWS.md`, `docs/QUICKSTART.md`
**Success Criteria** (what must be TRUE):
  1. The migrated hotspot workflows build and behave correctly on Windows Vulkan
  2. Native benchmark results show no regression on macOS Metal for the migrated hotspot set
  3. Native benchmark results show no regression on Windows Vulkan for the migrated hotspot set
  4. Remaining long-tail migration work and deferred platform follow-ups are captured cleanly for the next milestone
**Plans**: 3 plans

Plans:
- [ ] 05-01: Validate the migrated hotspot set on Windows Vulkan
- [ ] 05-02: Run and interpret native benchmark results on macOS and Windows
- [ ] 05-03: Resolve rollout issues and capture the next-wave migration backlog

## Progress

**Execution Order:**
Phases execute in numeric order: 1 → 2 → 3 → 4 → 5

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 1. Selection and Conventions | 3/3 | Complete    | 2026-03-24 |
| 2. Direct Adoption Tooling and Validation Harness | 3/3 | Complete | 2026-03-24 |
| 3. macOS Core Transform Migration | 0/3 | Not started | - |
| 4. Interaction Math and API Expansion | 0/3 | Not started | - |
| 5. Windows Vulkan Hardening and Performance Gates | 0/3 | Not started | - |
