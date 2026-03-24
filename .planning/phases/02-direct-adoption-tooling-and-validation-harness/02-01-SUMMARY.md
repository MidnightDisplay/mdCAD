---
phase: 02-direct-adoption-tooling-and-validation-harness
plan: 01
subsystem: infra
tags: [cglm, math-migration, validation]
requires:
  - phase: 01-selection-and-conventions
    provides: Thin cglm entrypoint baseline and shared math conventions
provides:
  - Configured `cglm` entrypoint with zero-to-one clip control enforcement
  - Migration-only compare helpers for legacy `math3d.h` parity checks
  - Lightweight validation and benchmark utilities for the standalone harness
affects: [phase-02-02, phase-02-03, phase-03]
tech-stack:
  added: [math-compare-helpers, math-validation-helpers, math-bench-helpers]
  patterns: [thin-entrypoint-config-boundary, raw-float-comparison-helpers]
key-files:
  created:
    - src/math/math_compare.h
    - src/math/math_validate.h
    - src/math/math_bench.h
  modified:
    - src/math/cglm_entry.h
key-decisions:
  - "The project-owned cglm entrypoint now owns clip-depth configuration and compile-time policy checks instead of staying a compile anchor only."
  - "Comparison, validation, and benchmark helpers live in adjacent headers and accept raw float buffers so struct and array cglm call sites can share one migration surface."
patterns-established:
  - "Keep `src/math/cglm_entry.h` include-only while using it as the single vendor configuration choke point."
  - "Expose migration plumbing as narrow helper headers rather than reintroducing project-owned vendor-type aliases."
requirements-completed: [FOUND-02]
duration: 1 min
completed: 2026-03-24
---

# Phase 02 Plan 01: Direct Adoption Tooling and Validation Harness Summary

**The math boundary now enforces `cglm` RH_ZO configuration and exposes narrow compare, validation, and benchmark helpers for the upcoming standalone harness**

## Performance

- **Duration:** 1 min
- **Started:** 2026-03-24T15:46:12Z
- **Completed:** 2026-03-24T15:47:03Z
- **Tasks:** 2
- **Files modified:** 4

## Accomplishments

- Turned `src/math/cglm_entry.h` into the real vendor-config choke point for the migration while keeping it include-only.
- Added comparison helpers that bridge legacy `vec3_t`, `mat4_t`, and `ray_t` values against raw float candidate buffers.
- Added reusable validation and benchmark utilities so the standalone math harness can report failures and timings without growing a facade layer.

## Task Commits

Each task was committed atomically:

1. **Task 1: Harden `cglm_entry.h` as the configured thin entrypoint** - `fc9946c` (feat)
2. **Task 2: Add migration-only compare, validate, and bench helper headers** - `3271047` (feat)

**Plan metadata:** recorded in the summary/state/roadmap completion commit

## Files Created/Modified

- `src/math/cglm_entry.h` - Centralized the zero-to-one clip-control define, added RH_ZO compile-time enforcement, and kept the entrypoint wrapper-free.
- `src/math/math_compare.h` - Added raw-float comparison helpers for legacy matrices, vectors, and screen rays.
- `src/math/math_validate.h` - Added a tiny validation report type plus expectation and exit-code helpers.
- `src/math/math_bench.h` - Added cross-platform timing and benchmark runner helpers for harness workloads.

## Decisions Made

- Enforced the clip-depth contract directly at the `cglm` entrypoint so drift fails loudly before runtime migration spreads.
- Kept migration helpers narrow and header-only so later subsystem work can mix `cglm` struct and array APIs without a compatibility facade.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Wave 2 can build the standalone harness against a stable configured math boundary.
- The harness can reuse the new compare, validation, and benchmark helpers without adding project-owned aliases for vendor math types.

---
*Phase: 02-direct-adoption-tooling-and-validation-harness*
*Completed: 2026-03-24*
