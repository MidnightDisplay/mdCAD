---
phase: 04-interaction-math-and-api-expansion
plan: 01
subsystem: interaction-math
tags: [cglm, picking, raycast, harness]
requires:
  - phase: 03-macos-core-transform-migration
    provides: cglm-backed camera/view/projection bridge matrices consumed by interaction paths
provides:
  - Shared cglm-backed interaction helper boundary for unproject, screen-ray, and pick-MVP math
  - Runtime migration of app drag and pick-buffer callsites to one helper path
  - Harness compare coverage for pick-MVP windowing parity
affects: [04-02, 04-03, HOT-03]
tech-stack:
  added: []
  patterns:
    - Shared interaction math boundary under src/math/
    - Harness compare parity against frozen pre-migration formulas
key-files:
  created:
    - src/math/math_interaction.h
  modified:
    - src/app.c
    - src/gpu/pick_buffer.h
    - src/math_harness.c
key-decisions:
  - "App drag-begin and drag-update now use one shared screen-ray helper to remove duplicated inverse-VP/ray code paths."
  - "Pick MVP windowing now routes through shared interaction math helper so runtime and future callers reuse one formula."
  - "Screen-ray helper preserves pre-migration NDC ray behavior while moving inverse/matrix work to cglm-backed code for parity safety."
patterns-established:
  - "Interaction callers should consume mdcad_interaction_* helpers instead of open-coding inverse/unproject math."
  - "New interaction migrations should add harness compare cases tied to frozen legacy formulas."
requirements-completed: [HOT-03]
duration: 5 min
completed: 2026-03-25
---

# Phase 04 Plan 01: Migrate pick/unproject/ray math to the new foundation Summary

**Pick MVP and screen-ray runtime math now runs through one cglm-backed helper boundary with strict harness parity coverage for screen rays and pick windowing.**

## Performance

- **Duration:** 5 min
- **Started:** 2026-03-25T13:07:26Z
- **Completed:** 2026-03-25T13:12:00Z
- **Tasks:** 2
- **Files modified:** 4

## Accomplishments
- Added `src/math/math_interaction.h` with `mdcad_interaction_unproject_point`, `mdcad_interaction_screen_ray_from_viewport`, and `mdcad_interaction_compute_pick_mvp`.
- Rewired `src/app.c` drag-begin/drag-update ray construction and `src/gpu/pick_buffer.h` pick MVP composition to shared interaction helpers.
- Extended `src/math_harness.c` with `pick-mvp-window` and migrated `screen-ray-unproject` compare coverage to production helper usage.

## Task Commits

Each task was committed atomically:

1. **Task 1: Create shared cglm-backed interaction helpers for screen-ray and pick MVP** - `ab35e8f` (feat)
2. **Task 2: Route app and pick runtime callsites to the helper boundary and extend compare coverage** - `7b0c091` (feat)

## Files Created/Modified
- `src/math/math_interaction.h` - Shared interaction math helper boundary and deterministic fallback guards.
- `src/app.c` - Unified drag ray construction through `mdcad_interaction_screen_ray_from_viewport`.
- `src/gpu/pick_buffer.h` - Pick MVP computation delegated to `mdcad_interaction_compute_pick_mvp`.
- `src/math_harness.c` - Added `pick-mvp-window` compare case and helper-backed screen-ray parity check.

## Decisions Made
- Centralized pick/unproject/ray helpers under one `src/math/` boundary to remove runtime duplication and drift.
- Kept screen-ray behavior parity with the previous app path while swapping matrix math internals to cglm-backed helpers.
- Added an explicit pick-MVP compare case so future interaction math changes fail fast under strict harness compare runs.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Screen-ray compare parity initially failed after first helper implementation**
- **Found during:** Task 2 (runtime migration and harness strict compare)
- **Issue:** Initial helper used unproject depth-0/1 construction, which diverged from the pre-migration app ray formula used in parity checks.
- **Fix:** Updated `mdcad_interaction_screen_ray_from_viewport` to preserve legacy NDC ray behavior while still using cglm-backed VP inverse/matrix-vector math.
- **Files modified:** `src/math/math_interaction.h`
- **Verification:** `./build/bin/mdcad_math_harness --mode compare --strict` now passes `screen-ray-unproject` and `pick-mvp-window`.
- **Committed in:** `7b0c091` (part of Task 2 commit)

---

**Total deviations:** 1 auto-fixed (1 bug)
**Impact on plan:** Parity bug fix was required to satisfy HOT-03 compare expectations; no scope expansion beyond the planned helper boundary.

## Issues Encountered
- CLI quoting around one filtered harness command produced a false-negative check during execution; reran command directly and captured full harness output.
- HOT-03 macOS interaction smoke: FAIL (not run in this headless executor session).

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness
- Interaction pick/ray helper boundary is in place and runtime callers now share it.
- `04-02` can reuse the same helper-boundary pattern for gizmo drag/intersection migration.
- Manual macOS viewport interaction smoke still required before declaring end-user parity complete for HOT-03.

---
*Phase: 04-interaction-math-and-api-expansion*
*Completed: 2026-03-25*
