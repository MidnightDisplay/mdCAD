---
phase: 03-macos-core-transform-migration
plan: 02
subsystem: transforms
tags: [cglm, ecs, transforms, parity]
requires:
  - phase: 03-macos-core-transform-migration
    provides: cglm-backed camera/view/projection assembly and frozen harness baselines from 03-01
provides:
  - cglm-backed `TransformComp` local and parented world matrix composition with stable `mat4_t` storage
  - A centralized ECS world-point helper that applies cached world matrices through cglm
  - Production-backed transform and hierarchy compare cases in the standalone harness
affects: [phase-03-03, macos-parity, ecs-render-path]
tech-stack:
  added: [ecs-world-point-helper, production-backed-transform-parity]
  patterns: [raw-cglm-compute-with-legacy-storage, centralized-world-point-application]
key-files:
  created: []
  modified:
    - src/components/transform_comp.h
    - src/ecs/ecs_scene.h
    - src/math_harness.c
    - src/CMakeLists.txt
key-decisions:
  - "TransformComp keeps vec3_t and mat4_t cached storage in Phase 3 while local and parented matrix composition moves to cglm raw matrices through memcpy bridge helpers."
  - "World-point application in ecs_scene.h now flows through one cglm-backed helper instead of leaving mat4_transform_point(t->world_matrix, ...) scattered across creation, update, render, and pick paths."
patterns-established:
  - "Use cglm raw matrices for dense transform compute, then copy results back into stable cached storage when downstream render code still consumes mat4_t."
  - "Pull repeated world-matrix point application into one helper before migrating parity tests so harness coverage can exercise the same production helper everywhere."
requirements-completed: [HOT-02]
duration: 35 min
completed: 2026-03-24
---

# Phase 03 Plan 02: ECS Transform Composition and World-Point Usage Summary

**Transform composition and ECS world-point application now run through cglm on the production path, while cached storage stays stable and the harness validates both single-transform and parent-child behavior**

## Performance

- **Duration:** 35 min
- **Started:** 2026-03-24T17:37:04Z
- **Completed:** 2026-03-24T18:12:58Z
- **Tasks:** 2
- **Files modified:** 4

## Accomplishments

- Reimplemented `TransformComp` local and parented world matrix composition in `src/components/transform_comp.h` with cglm raw matrices and explicit `memcpy` bridges back to cached `mat4_t` fields.
- Added `ecs_scene_transform_point_world(...)` in `src/ecs/ecs_scene.h` and routed the repeated `t->world_matrix` point transforms through that single helper across entity creation, scene update, light/render uploads, and pick population.
- Reworked the standalone harness so `transform-compose` and the new `hierarchy-world-transform` case exercise the live production helpers rather than local ad hoc cglm formulas.
- Extended the harness include path in `src/CMakeLists.txt` just enough to compile the new ECS-backed parity case against the real headers.

## Task Commits

Each task was committed atomically:

1. **Task 1: Migrate `TransformComp` composition to raw cglm helpers while preserving cached storage** - `e9b1ed0` (feat)
2. **Task 2: Centralize cglm-backed world-point application in ECS scene sync and harness coverage** - `86929ae` (feat)

**Plan metadata:** recorded in the summary/state/roadmap completion commit

## Files Created/Modified

- `src/components/transform_comp.h` - Swapped local/world composition to cglm raw matrices while preserving the existing cached field types.
- `src/ecs/ecs_scene.h` - Added the centralized world-point helper and replaced the scattered `t->world_matrix` transform call sites with it.
- `src/math_harness.c` - Pointed transform parity at `TransformComp` and ECS production helpers and added the new hierarchy compare case.
- `src/CMakeLists.txt` - Added the include-path support needed for the harness to read the ECS headers directly.

## Decisions Made

- Kept Phase 3 compute-centric rather than storage-centric by bridging between cglm raw matrices and existing `mat4_t` caches.
- Accepted a small harness compile-path tweak in CMake so parity coverage can exercise the live ECS helper instead of a duplicated local copy.

## Deviations from Plan

- `src/CMakeLists.txt` was modified in addition to the planned file list so `mdcad_math_harness` could compile the new ECS-backed hierarchy compare case against the real production headers.

## Issues Encountered

- The first harness rebuild failed because fetched Sokol include paths were not available as raw variables in `src/CMakeLists.txt`.
- Resolved by switching the harness include wiring to target-derived include directories from `libsokol` and `libcimgui`.

## User Setup Required

None - the existing native build and harness commands remain the validation entry points.

## Next Phase Readiness

- `03-03` can now finalize Phase 3 around the live camera, transform, and hierarchy parity cases without adding more structural migration work.
- The harness already exposes `hierarchy-world-transform` through `--list` and the strict compare run.

---
*Phase: 03-macos-core-transform-migration*
*Completed: 2026-03-24*
