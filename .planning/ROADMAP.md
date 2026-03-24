# Roadmap: mdCAD

## Overview

This roadmap modernizes mdCAD’s math foundation without destabilizing the native workflows that already work. The path is deliberately staged: choose and integrate the replacement library first, wrap it behind a project-owned boundary, migrate the most performance- and interaction-sensitive subsystems on macOS first, then prove the same hotspot set on Windows Vulkan before widening the rollout or chasing full repo cleanup.

## Phases

**Phase Numbering:**
- Integer phases (1, 2, 3): Planned milestone work
- Decimal phases (2.1, 2.2): Urgent insertions (marked with INSERTED)

Decimal phases appear between their surrounding integers in numeric order.

- [ ] **Phase 1: Selection and Conventions** - Choose the replacement library and lock mdCAD’s math conventions
- [ ] **Phase 2: Compatibility Layer and Validation Harness** - Build the project-owned migration boundary and comparison tooling
- [ ] **Phase 3: macOS Core Transform Migration** - Migrate camera, transforms, and render-matrix hotspots on the primary native path
- [ ] **Phase 4: Interaction Math and API Expansion** - Migrate picking/gizmo math and introduce richer helper coverage
- [ ] **Phase 5: Windows Vulkan Hardening and Performance Gates** - Validate the migrated hotspot set on Windows and close native performance gates

## Phase Details

### Phase 1: Selection and Conventions
**Goal**: Select the replacement math library, vendor it with minimal build disruption, and define the conventions the migration must preserve
**Depends on**: Nothing (first phase)
**Requirements**: FOUND-01, FOUND-03
**Canonical refs**: `.planning/PROJECT.md`, `.planning/research/STACK.md`, `.planning/research/SUMMARY.md`, `src/math3d.h`, `src/platform.h`, `CMakeLists.txt`, `src/CMakeLists.txt`
**Success Criteria** (what must be TRUE):
  1. mdCAD has one chosen MIT-licensed C math library that builds in the current native workflow without introducing C++
  2. Matrix layout, handedness, clipspace, and alignment policy are documented in one project-owned place
  3. The adoption mode for the chosen library is decided clearly enough to start building a compatibility layer
**Plans**: 3 plans

Plans:
- [ ] 01-01: Evaluate final library candidates against mdCAD constraints and select the backend
- [ ] 01-02: Vendor the chosen library and wire the minimal include/build path
- [ ] 01-03: Define mdCAD convention and alignment policy for the migration

### Phase 2: Compatibility Layer and Validation Harness
**Goal**: Create a project-owned math boundary and the validation tools needed to compare old and new behavior safely
**Depends on**: Phase 1
**Requirements**: FOUND-02, PERF-01
**Canonical refs**: `.planning/PROJECT.md`, `.planning/research/ARCHITECTURE.md`, `.planning/research/PITFALLS.md`, `src/math3d.h`, `src/orbit_camera.h`, `src/ecs/ecs_scene.h`, `src/gpu/geometry_batch.h`, `src/gpu/pick_buffer.h`
**Success Criteria** (what must be TRUE):
  1. mdCAD has a local compatibility layer that lets old and new math coexist without leaking vendor types everywhere
  2. Regression and benchmark entry points exist for the migrated hotspot set
  3. The migration can be rolled forward subsystem by subsystem instead of as one repo-wide change
**Plans**: 3 plans

Plans:
- [ ] 02-01: Create the `src/math/` compatibility boundary and backend selection layer
- [ ] 02-02: Add old/new comparison utilities and benchmark scaffolding for hotspot math
- [ ] 02-03: Wire regression hooks or smoke scenes into the native validation workflow

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
| 1. Selection and Conventions | 0/3 | Not started | - |
| 2. Compatibility Layer and Validation Harness | 0/3 | Not started | - |
| 3. macOS Core Transform Migration | 0/3 | Not started | - |
| 4. Interaction Math and API Expansion | 0/3 | Not started | - |
| 5. Windows Vulkan Hardening and Performance Gates | 0/3 | Not started | - |
