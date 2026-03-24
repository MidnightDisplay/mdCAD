---
phase: 03-macos-core-transform-migration
plan: 03
subsystem: validation
tags: [cglm, validation, docs, parity]
requires:
  - phase: 03-macos-core-transform-migration
    provides: migrated camera, transform, and ECS world-point production paths from 03-01 and 03-02
provides:
  - A default compare order that leads with the Phase 3 hotspot parity cases
  - Verified `math-validation` coverage for camera, transform, hierarchy, and screen-ray compares
  - Checked-in macOS parity commands and a manual smoke checklist in Quickstart
affects: [phase-complete, macos-parity, onboarding]
tech-stack:
  added: [phase-3-parity-workflow]
  patterns: [hotspot-first-compare-order, harness-first-smoke-docs]
key-files:
  created: []
  modified:
    - src/math_harness.c
    - docs/QUICKSTART.md
key-decisions:
  - "The default compare suite now leads with the four Phase 3 hotspot cases so routine harness runs surface the migrated camera and transform parity checks first."
  - "Quickstart documents Phase 3 as a harness-first macOS workflow with app launch kept explicitly as a manual smoke confirmation step."
patterns-established:
  - "Keep rollout-critical parity cases at the front of the default compare order so strict runs fail fast on the current migration slice."
  - "Document the automated gate and manual smoke checklist together in checked-in docs so another agent can rerun the phase without tribal knowledge."
requirements-completed: [HOT-01, HOT-02]
duration: 1 min
completed: 2026-03-24
---

# Phase 03 Plan 03: Phase 3 Parity Workflow and Smoke Docs Summary

**Phase 3 now closes with a hotspot-first parity suite and a concrete macOS smoke workflow that keeps the harness primary and the app launch secondary**

## Performance

- **Duration:** 1 min
- **Started:** 2026-03-24T18:14:59Z
- **Completed:** 2026-03-24T18:15:07Z
- **Tasks:** 2
- **Files modified:** 2

## Accomplishments

- Reordered the default compare suite in `src/math_harness.c` so the Phase 3 hotspot cases lead the list and strict compare run.
- Ran the full `math-validation` target successfully after the Phase 3 migration, with `orbit-camera-view`, `view-projection-roundtrip`, `transform-compose`, and `hierarchy-world-transform` all passing.
- Added a dedicated `## Phase 3 macOS parity smoke` section to `docs/QUICKSTART.md` with the exact command order and manual interaction checklist.

## Task Commits

Each task was committed atomically:

1. **Task 1: Finalize production-backed Phase 3 parity coverage in the harness** - `a2dd7de` (chore)
2. **Task 2: Document the exact Phase 3 macOS parity workflow and smoke checklist** - `17d2cab` (docs)

**Plan metadata:** recorded in the summary/state/roadmap completion commit

## Files Created/Modified

- `src/math_harness.c` - Moved the Phase 3 hotspot compare cases to the front of the default list/run order so routine parity checks lead with the migrated paths.
- `docs/QUICKSTART.md` - Added the exact macOS `math-validation`, strict harness, and `mdCAD` smoke commands plus the required orbit/pan/zoom and parented-geometry checks.

## Decisions Made

- Treated compare ordering as part of the Phase 3 validation UX so the current migration hotspots fail fast and stay visible in routine harness runs.
- Kept the docs explicit that `math-validation` and strict harness compare are the primary gate, while the app launch remains a manual smoke confirmation.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None

## User Setup Required

None - the documented commands run against the existing macOS/Ninja build workflow.

## Next Phase Readiness

- Phase 03 is complete on disk and ready for the next workflow step.
- Phase 04 can start from a repo that now has migrated camera/transform hotspots plus a repeatable macOS parity workflow.

---
*Phase: 03-macos-core-transform-migration*
*Completed: 2026-03-24*
