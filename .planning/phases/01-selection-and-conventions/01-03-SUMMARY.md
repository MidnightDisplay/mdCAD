---
phase: 01-selection-and-conventions
plan: 03
subsystem: math
tags: [cglm, conventions, alignment, docs]
requires:
  - phase: 01-02
    provides: Thin project-owned cglm entrypoint compiled by the mdCAD target
provides:
  - Compiled math convention and alignment contract
  - Human-readable convention document for rollout-sensitive hotspots
  - cglm entrypoint enforcement of contract include order and layout assumptions
affects: [phase-02, camera, picking, gizmo, transform-migration]
tech-stack:
  added: [math-conventions-contract]
  patterns: [convention-capsule, compile-time-layout-asserts]
key-files:
  created:
    - src/math/math_conventions.h
    - docs/MATH_CONVENTIONS.md
  modified:
    - src/math/cglm_entry.h
key-decisions:
  - "Migrated mdCAD math semantics standardize on column-major, right-handed, clip depth 0..1."
  - "Alignment policy is performance-first and native gates must block on policy breaks."
patterns-established:
  - "Math conventions live in one compiled header plus one matching doc."
  - "The cglm entrypoint enforces include order and layout assumptions with compile-time assertions."
requirements-completed: [FOUND-03]
duration: 1 min
completed: 2026-03-24
---

# Phase 01 Plan 03: Selection and Conventions Summary

**Phase 1 now has a compiled math contract for column-major, right-handed, `0..1` clip-depth cglm adoption with enforced layout checks**

## Performance

- **Duration:** 1 min
- **Started:** 2026-03-24T14:42:33Z
- **Completed:** 2026-03-24T14:43:15Z
- **Tasks:** 2
- **Files modified:** 3

## Accomplishments

- Added `src/math/math_conventions.h` as the single code-owned contract for matrix layout, handedness, clip depth, alignment policy, and rollout behavior.
- Added `docs/MATH_CONVENTIONS.md` to explain the same contract and name the rollout-sensitive hotspots that must preserve visible behavior.
- Updated `src/math/cglm_entry.h` to include the convention header first and enforce `mat4` / `mat4s` layout expectations with compile-time assertions.

## Task Commits

Each task was committed atomically:

1. **Task 1: Create the convention and alignment contract** - `51ac809` (docs)
2. **Task 2: Wire the convention contract into the cglm entrypoint** - `c883a62` (feat)

**Plan metadata:** recorded in the summary/state/roadmap completion commit

## Files Created/Modified

- `src/math/math_conventions.h` - Compiled contract for matrix layout, handedness, clip depth, alignment policy, and rollout-sensitive hotspots.
- `docs/MATH_CONVENTIONS.md` - Human-readable explanation of the same contract and visible-behavior rules.
- `src/math/cglm_entry.h` - Entry point now includes the contract before vendor headers and enforces layout compatibility.

## Decisions Made

- Standardized migrated mdCAD math on column-major matrices, right-handed coordinates, and `0..1` clip depth instead of preserving `src/math3d.h` semantics as the internal baseline.
- Chose a performance-first alignment policy and made policy breaks a stop condition for supported native gates rather than allowing quiet fallback behavior.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Phase 1 now ends with a vendored `cglm` integration and one authoritative convention contract wired into the entrypoint.
- Phase-level verification can now check FOUND-01 and FOUND-03 against the actual repo artifacts before Phase 2 planning starts.

---
*Phase: 01-selection-and-conventions*
*Completed: 2026-03-24*
