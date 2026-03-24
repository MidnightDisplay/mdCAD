# Phase 2: Direct Adoption Tooling and Validation Harness - Discussion Log

**Gathered:** 2026-03-24
**Status:** Complete

## Discussion Summary

- Kept `src/math/cglm_entry.h` as an include-only entrypoint rather than expanding it into a wrapper surface
- Allowed migration-only helper headers under `src/math/` for comparison, validation, and benchmark plumbing
- Chose a standalone native harness as the primary old/new comparison and benchmark surface
- Required both hotspot workflow comparisons and primitive microbenchmarks in the first pass
- Chose a subsystem-specific `cglm` API-family bias: struct-first in camera/app/gizmo-facing code, array-friendly in ECS and GPU-heavy paths
- Chose a lightweight, repeatable native validation workflow for macOS first, designed to mirror on Windows Vulkan later

## Entrypoint surface

### Q1
**Question:** For phase 2, what should `src/math/` become beyond the current `cglm_entry.h`?

**Options presented:**
- `Strictly entrypoint-only`
- `Thin plus migration helpers`
- `Broader facade`

**Selected:** `Thin plus migration helpers`

### Q2
**Question:** Where should those helpers live?

**Options presented:**
- `Keep cglm_entry.h include-only and put helpers in adjacent headers like math_compare.h / math_bench.h / math_validate.h`
- `Put a few migration helpers directly into cglm_entry.h`
- `Add one umbrella header in src/math/ that re-exports the entrypoint plus migration helpers`

**Selected:** `Keep cglm_entry.h include-only and put helpers in adjacent headers like math_compare.h / math_bench.h / math_validate.h`

## Comparison harness

### Q1
**Question:** Which direction should the first comparison system take?

**Options presented:**
- `Standalone native harness first, with app smoke hooks as secondary`
- `In-app debug panels or smoke scenes first, no separate harness initially`
- `Build both equally from the start`

**Selected:** `Standalone native harness first, with app smoke hooks as secondary`

### Q2
**Question:** What should the first-pass harness emphasize?

**Options presented:**
- `Hotspot workflow comparisons first: camera/view-projection, pick-ray or unproject, and transform composition, with optional microbench hooks`
- `Primitive microbenchmarks first: vec and mat ops plus helper functions, with workflow checks later`
- `Both are mandatory in phase 2 from the start`

**Selected:** `Both are mandatory in phase 2 from the start`

## API-family bias by subsystem

### Q1
**Question:** How should struct versus array `cglm` adoption bias be handled in early migration work?

**Options presented:**
- `Bias early migration and comparison code toward cglm struct API in camera, app, and gizmo-facing slices, while leaving ECS and GPU math free to adopt array API where it measures better`
- `Standardize on struct API almost everywhere early for consistency`
- `Push array API early in ECS and GPU and use struct API only in the most interaction-shaped code`

**Selected:** `Bias early migration and comparison code toward cglm struct API in camera, app, and gizmo-facing slices, while leaving ECS and GPU math free to adopt array API where it measures better`

## Native validation workflow

### Q1
**Question:** How formal should the phase-2 native validation workflow become?

**Options presented:**
- `Add dedicated native harness target(s) plus documented repeatable commands for macOS first, with the same workflow designed to mirror on Windows Vulkan later`
- `Wire the new harness into CTest or a more formal native test runner immediately in phase 2`
- `Keep phase 2 minimal and only document ad hoc manual commands around the app and harness`

**Selected:** `Add dedicated native harness target(s) plus documented repeatable commands for macOS first, with the same workflow designed to mirror on Windows Vulkan later`

## Notes

- The user explicitly wants the standalone harness to act as a pilot for a broader future testing foundation, but that broader expansion stays out of current phase scope.
- Phase 1 decisions remained locked during this discussion: `cglm` stays pinned to `0.9.6`, direct adoption remains the strategy, and `src/math/cglm_entry.h` is still not a broad compatibility facade.
- The discussion focused on preparing safe hotspot migration rather than reopening backend choice, convention policy, or build-mode questions already settled in Phase 1.

---
*Phase: 02-direct-adoption-tooling-and-validation-harness*
*Discussion logged: 2026-03-24*
