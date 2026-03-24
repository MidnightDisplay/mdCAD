---
phase: 02-direct-adoption-tooling-and-validation-harness
plan: 02
subsystem: testing
tags: [cglm, harness, benchmarks, validation]
requires:
  - phase: 02-direct-adoption-tooling-and-validation-harness
    provides: Configured thin cglm entrypoint and migration helper headers
provides:
  - Standalone `mdcad_math_harness` executable for native compare and bench runs
  - Workflow comparisons for camera view, projection-sensitive roundtrips, screen rays, and transform composition
  - Primitive benchmark entry points for legacy and `cglm` matrix and ray operations
affects: [phase-02-03, phase-03, perf-gates]
tech-stack:
  added: [mdcad-math-harness]
  patterns: [harness-first-validation, behavior-first-compare-cases]
key-files:
  created:
    - src/math_harness.c
  modified:
    - src/CMakeLists.txt
    - src/math/math_validate.h
key-decisions:
  - "Phase 2's primary validation surface is now a standalone native harness instead of app-coupled smoke checks."
  - "Projection-sensitive comparison cases validate behavior-level NDC X/Y or ray outputs instead of demanding raw projection-matrix equality across clip-depth conventions."
patterns-established:
  - "Use workflow-level compare cases for rollout-sensitive hotspots and microbench cases for primitive interpretation in the same harness binary."
  - "Bias early camera/app/gizmo migration checks toward the cglm struct API while keeping the compare surface raw-float-based."
requirements-completed: [FOUND-02, PERF-01]
duration: 4 min
completed: 2026-03-24
---

# Phase 02 Plan 02: Direct Adoption Tooling and Validation Harness Summary

**A standalone `mdcad_math_harness` now builds in the native workflow, proves parity for the first migration-sensitive math paths, and exposes repeatable primitive benchmarks**

## Performance

- **Duration:** 4 min
- **Started:** 2026-03-24T15:51:03Z
- **Completed:** 2026-03-24T15:55:03Z
- **Tasks:** 2
- **Files modified:** 3

## Accomplishments

- Added a native-desktop-only `mdcad_math_harness` target with `--list`, `--mode compare`, `--mode bench`, `--iterations`, and `--strict` CLI support.
- Implemented strict comparison cases for orbit camera view matrices, projection-sensitive NDC roundtrips, screen-ray reconstruction, and transform composition.
- Added primitive benchmark suites for legacy versus `cglm` matrix multiply, matrix inverse, and screen-ray paths with stable `BENCH` output prefixes.

## Task Commits

Each task was committed atomically:

1. **Task 1: Add the standalone `mdcad_math_harness` target and CLI skeleton** - `ce557ef` (feat)
2. **Task 2: Implement workflow compare suites and primitive benchmark suites** - `a4432cf` (feat)

**Plan metadata:** recorded in the summary/state/roadmap completion commit

## Files Created/Modified

- `src/CMakeLists.txt` - Added the native-desktop-only `mdcad_math_harness` executable and lightweight include/link configuration.
- `src/math_harness.c` - Added the harness CLI, behavior-sensitive compare cases, and primitive benchmark suites.
- `src/math/math_validate.h` - Quieted the shared validation helper so harness failures flow through the required `COMPARE FAIL` output lines only.

## Decisions Made

- Kept the harness standalone and free of app/runtime library dependencies so math validation stays lightweight and fast to iterate.
- Compared behavior-level outputs for projection-sensitive workflows because the legacy and target clip-depth conventions intentionally differ internally.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Wave 3 can wrap the harness with repeatable `math-regression`, `math-bench`, and `math-validation` targets.
- Quickstart documentation can now point to concrete native harness commands on macOS and Windows Vulkan.

---
*Phase: 02-direct-adoption-tooling-and-validation-harness*
*Completed: 2026-03-24*
