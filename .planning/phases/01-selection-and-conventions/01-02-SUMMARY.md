---
phase: 01-selection-and-conventions
plan: 02
subsystem: infra
tags: [cglm, cmake, native-build, vendoring]
requires:
  - phase: 01-01
    provides: Direct-adoption docs and locked cglm backend decision
provides:
  - Vendored cglm 0.9.6 header tree with local version marker
  - Thin project-owned cglm entrypoint compiled by the mdCAD target
  - Native Ninja build proof for the vendored integration
affects: [phase-01-03, phase-02, build-validation]
tech-stack:
  added: [cglm-0.9.6]
  patterns: [header-only-vendoring, compile-anchor]
key-files:
  created:
    - vendors/cglm/VERSION.txt
    - vendors/cglm/LICENSE
    - src/math/cglm_entry.h
  modified:
    - src/CMakeLists.txt
    - src/app.c
key-decisions:
  - "cglm is vendored locally as headers only for the first adoption slice."
  - "The mdCAD native app proves integration through a compile anchor in app.c rather than staged-on-disk headers."
patterns-established:
  - "Vendored math dependencies live under vendors/ with a local VERSION.txt marker."
  - "Direct vendor adoption still flows through a thin entrypoint header that centralizes include order."
requirements-completed: [FOUND-01]
duration: 2 min
completed: 2026-03-24
---

# Phase 01 Plan 02: Selection and Conventions Summary

**Vendored `cglm` `0.9.6` now compiles through a thin `src/math/cglm_entry.h` entrypoint in the native mdCAD target**

## Performance

- **Duration:** 2 min
- **Started:** 2026-03-24T14:38:15Z
- **Completed:** 2026-03-24T14:40:43Z
- **Tasks:** 2
- **Files modified:** 192

## Accomplishments

- Vendored the upstream `cglm` `0.9.6` header tree into `vendors/cglm` with the upstream MIT license and a local version marker.
- Added a thin `src/math/cglm_entry.h` include entrypoint that exposes `cglm/cglm.h` and `cglm/struct.h` without introducing mdCAD typedef aliases.
- Wired the vendored include path into `src/CMakeLists.txt` and proved the integration by compiling `mdcad_cglm_compile_anchor()` from `src/app.c`.

## Task Commits

Each task was committed atomically:

1. **Task 1: Vendor cglm 0.9.6 into vendors/cglm** - `af65cb9` (chore)
2. **Task 2: Add the minimal build wiring and compile anchor** - `01dc5b4` (feat)

**Plan metadata:** recorded in the summary/state/roadmap completion commit

## Files Created/Modified

- `vendors/cglm/include/cglm/` - Upstream `cglm` 0.9.6 header tree used for direct vendoring.
- `vendors/cglm/LICENSE` - Upstream MIT license text for the vendored dependency.
- `vendors/cglm/VERSION.txt` - Local version marker pinned to `0.9.6`.
- `src/math/cglm_entry.h` - Thin mdCAD entrypoint for direct `cglm` inclusion and compile anchoring.
- `src/CMakeLists.txt` - Added the vendored header include path to the mdCAD target.
- `src/app.c` - Included the new entrypoint and invoked the compile anchor during initialization.

## Decisions Made

- Kept the first integration slice header-only and local to the repo instead of adding any fetch-at-build-time or linked-library path.
- Used a compile anchor in the main app target so the build proof covers both `mat4` and `mat4s` availability immediately.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- The repo now contains a real, compiled `cglm` integration point for future convention and hotspot work.
- Wave 3 can layer the global convention/alignment contract into `src/math/cglm_entry.h` without changing the basic vendoring path.

---
*Phase: 01-selection-and-conventions*
*Completed: 2026-03-24*
