# Phase 2 Plan: Direct Adoption Tooling and Validation Harness

**Created:** 2026-03-24
**Status:** Ready for execution planning review

## Scope

Build the thin project-owned `src/math/` boundary and the standalone native comparison/benchmark tooling needed before hotspot migration begins.

## Wave Summary

1. **Wave 1 / 02-01:** Convert `src/math/` into a configured thin entrypoint plus migration-only compare, validate, and bench helper headers.
2. **Wave 2 / 02-02:** Add the standalone `mdcad_math_harness` target with workflow compare suites and primitive microbenchmarks.
3. **Wave 3 / 02-03:** Wire repeatable CMake validation targets and Quickstart commands around the harness.

## Source of Truth

- `.planning/phases/02-direct-adoption-tooling-and-validation-harness/02-CONTEXT.md`
- `.planning/phases/02-direct-adoption-tooling-and-validation-harness/02-RESEARCH.md`
- `.planning/phases/02-direct-adoption-tooling-and-validation-harness/02-VALIDATION.md`
- `.planning/phases/02-direct-adoption-tooling-and-validation-harness/02-01-PLAN.md`
- `.planning/phases/02-direct-adoption-tooling-and-validation-harness/02-02-PLAN.md`
- `.planning/phases/02-direct-adoption-tooling-and-validation-harness/02-03-PLAN.md`

## Review Focus

- `src/math/cglm_entry.h` stays include-only and does not drift into a wrapper API.
- The harness is standalone, buildable, and behavior-first for projection-sensitive comparisons.
- The validation workflow stays lightweight and mirrorable to Windows Vulkan.
