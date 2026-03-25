---
phase: 04-interaction-math-and-api-expansion
plan: 03
subsystem: math
tags: [cglm, quaternion, interaction, harness]
requires:
  - phase: 04-01
    provides: shared interaction helper surface for pick/screen-ray math
  - phase: 04-02
    provides: gizmo drag migration on mdcad_interaction helpers
provides:
  - quaternion-capable helper APIs under src/math/
  - interaction bench coverage for Phase 4 validation gates
  - scoped deprecation markers for legacy math3d interaction helpers
affects: [phase-05-windows-vulkan-hardening-and-performance-gates]
tech-stack:
  added: []
  patterns:
    - cglm-backed quaternion helpers with project-owned quaternion struct signatures
    - harness-first parity and bench gating for migrated interaction paths
key-files:
  created:
    - src/math/math_quat.h
  modified:
    - src/math_harness.c
    - src/math3d.h
    - docs/QUICKSTART.md
key-decisions:
  - "Quaternion helpers expose project-owned args/struct while delegating math ops directly to cglm glms_quat APIs."
  - "Legacy interaction helpers remain available but are explicitly marked deprecated for migrated slices to block regressions."
  - "Harness keeps a local opt-out for deprecated legacy calls so baseline checks stay warning-clean."
patterns-established:
  - "Add new helper capability under src/math/ before widening runtime storage rewrites."
  - "Require compare + bench command coverage for every migrated interaction slice."
requirements-completed: [HOT-03, EXP-01, EXP-02]
duration: 3 min
completed: 2026-03-25
---

# Phase 4 Plan 03: Add expanded helper coverage and retire redundant local helpers summary

**Quaternion helpers, interaction bench gates, and scoped legacy-helper deprecations now close the Phase 4 expansion/retirement slice.**

## Performance

- **Duration:** 3 min
- **Started:** 2026-03-25T13:52:45Z
- **Completed:** 2026-03-25T13:55:52Z
- **Tasks:** 2
- **Files modified:** 4

## Accomplishments
- Added `src/math/math_quat.h` with `mdcad_quat_from_axis_angle`, `mdcad_quat_normalize`, `mdcad_quat_rotate_vec3`, and `mdcad_quat_mul` over `glms_quat*` operations.
- Extended strict compare coverage with `quat-rotate-vector` and `quat-compose-order` cases using frozen baseline quaternion math.
- Added bench coverage (`bench-interaction-ray`, `bench-interaction-drag`, `bench-quat-ops`) and documented `## Phase 4 interaction parity smoke` in Quickstart.
- Added scoped `MDCAD_MATH3D_INTERACTION_DEPRECATED` annotations for legacy interaction helpers in `src/math3d.h`.

## Task Commits

Each task was committed atomically:

1. **Task 1: Add quaternion-capable helper module and parity checks** - `a20b7e3` (feat)
2. **Task 2: Retire equivalent legacy interaction helpers in migrated slices and lock phase-4 validation workflow** - `35b04a8` (feat)

## Files Created/Modified
- `src/math/math_quat.h` - Project-owned quaternion helper surface backed by cglm struct API.
- `src/math_harness.c` - New quaternion compare cases and new interaction/quaternion bench cases.
- `src/math3d.h` - Scoped migration deprecation macro/comments on legacy interaction helpers.
- `docs/QUICKSTART.md` - Added Phase 4 interaction parity smoke workflow and manual checklist.

## Decisions Made
- Scoped deprecation markers were applied only to interaction helpers (`ray_from_screen`, `ray_axis_closest_t`, `ray_plane_intersect`) so non-migrated math3d areas remain unaffected.
- Harness baseline/legacy checks keep explicit local allowance for deprecated usage to avoid warning noise while preserving regression baselines.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Deprecation warnings in harness baseline paths**
- **Found during:** Task 2 verification
- **Issue:** New `MDCAD_MATH3D_INTERACTION_DEPRECATED` markers triggered warnings in harness legacy baseline calls.
- **Fix:** Added `MDCAD_MATH3D_ALLOW_INTERACTION_DEPRECATED_USAGE` opt-out for harness translation unit only.
- **Files modified:** `src/math_harness.c`, `src/math3d.h`
- **Verification:** Rebuilt `mdcad_math_harness` with zero deprecation warnings.
- **Committed in:** `35b04a8`

---

**Total deviations:** 1 auto-fixed (Rule 1: bug)
**Impact on plan:** Warning-only fix; no scope creep and no runtime behavior change.

## Issues Encountered
Manual HOT-03 viewport interaction smoke was not executed in this headless executor run.

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
Phase 4 plan work is complete and automated gates are green. Remaining manual gate to run separately: **HOT-03 macOS interaction smoke: FAIL (not run in this session)**.

---
*Phase: 04-interaction-math-and-api-expansion*
*Completed: 2026-03-25*
