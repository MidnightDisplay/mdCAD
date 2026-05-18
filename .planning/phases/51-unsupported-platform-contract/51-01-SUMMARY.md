---
phase: 51-unsupported-platform-contract
plan: 01
subsystem: testing
tags: [avalonia, embed, unsupported-platform, unit-tests, wave-0]

# Dependency graph
requires:
  - phase: 50-backend-seam-extraction-and-windows-behavior-lock
    provides: Extracted backend seam, locked Windows behavior, and the coordinator/backend lifecycle split that Phase 51 can extend safely
provides:
  - Wave 0 red-first proof for unsupported backend selection, canonical unsupported messaging, and inert unsupported lifecycle behavior
  - Wave 0 red-first proof for blocked coordinator reconcile and immediate unsupported imperative behavior
  - A clean execution handoff from red contract tests into the internal unsupported-runtime implementation plan
affects: [phase-51, phase-52]

# Tech tracking
tech-stack:
  added: []
  patterns: [red-first contract tests, canonical unsupported-runtime message pinning, blocked-reason lifecycle proof]

key-files:
  created:
    - .planning/phases/51-unsupported-platform-contract/51-01-SUMMARY.md
    - samples/avalonia-mdcad-control.tests/UnsupportedMdCadEmbedBackendTests.cs
  modified:
    - samples/avalonia-mdcad-control.tests/MdCadSessionCoordinatorTests.cs

key-decisions:
  - "Kept Wave 0 entirely inside the existing Windows-targeted test project and forced the unsupported branch through internal seams instead of widening public runtime claims."
  - "Pinned one canonical unsupported-runtime truth in the red tests before implementing any backend or shell behavior so later plans cannot drift on wording or precedence."
  - "Separated unsupported backend contract proof from coordinator blocked-reason proof into two commits so the red-first history matches the plan tasks."

patterns-established:
  - "Pattern 1: Introduce unsupported-platform behavior by naming the internal factory/backend/coordinator contract in failing tests first, then implement against that exact seam."
  - "Pattern 2: Keep unsupported-platform proof inside the current Windows test lane by using internal-only selection hooks rather than public capability APIs."

requirements-completed: [PLAT-01, PLAT-02, PLAT-03]

# Metrics
duration: continued-session
completed: 2026-05-18
---

# Phase 51 Plan 01: Wave 0 Contract Test Summary

**Phase 51 now has explicit red-first proof for the unsupported-platform contract: the missing factory/backend/coordinator seams are named in tests before implementation, while the Phase 50 Windows path remains untouched.**

## Performance

- **Duration:** continued from the Phase 51 execution session
- **Completed:** 2026-05-18T14:29:01.8050162+01:00
- **Tasks:** 2
- **Files modified:** 2

## Accomplishments

- Added `UnsupportedMdCadEmbedBackendTests` to pin internal unsupported backend selection, the canonical Windows-only unsupported message, and safe inert lifecycle behavior.
- Extended `MdCadSessionCoordinatorTests` to pin blocked auto-start reconcile behavior, blocked launch-setting reconcile behavior, and immediate unsupported `StartAsync()` failure plus safe `StopAsync()`.
- Verified the Wave 0 lane is red for the intended reasons: missing unsupported backend type, missing canonical runtime helper, missing backend factory, missing `StartBlockedReason`, and missing blocked-reason coordinator constructor seam.

## Task Commits

1. **Task 1: Add unsupported backend contract tests for factory selection and inert lifecycle behavior** - `b87a69b` (test)
2. **Task 2: Add coordinator tests for blocked no-launch reconcile and immediate unsupported imperative behavior** - `a18c7d1` (test)

## Files Created/Modified

- `samples/avalonia-mdcad-control.tests/UnsupportedMdCadEmbedBackendTests.cs` - New Wave 0 contract file for internal unsupported backend selection and inert lifecycle behavior.
- `samples/avalonia-mdcad-control.tests/MdCadSessionCoordinatorTests.cs` - Adds blocked-reason reconcile and immediate unsupported imperative API coverage.

## Decisions Made

- Kept Wave 0 entirely test-only and deliberately red so Plan 51-02 must satisfy the exact internal contract.
- Reused the existing test project and `InternalsVisibleTo` boundary rather than adding a headless Avalonia harness or widening the public surface.
- Pinned the canonical unsupported message once in tests so backend/coordinator/shell work all converge on one truth.

## Deviations from Plan

None. The Wave 0 proof landed exactly as planned.

## Issues Encountered

- An initial parallel red-test run conflicted on shared build output; rerunning the lane sequentially against the Release configuration produced the intended red failure set.

## User Setup Required

None.

## Next Phase Readiness

- Plan `51-02` can now implement the missing unsupported backend, factory, canonical message helper, and blocked-reason coordinator seam directly against the red Wave 0 contract.
- Plan `51-03` remains downstream shell/UI work only after the internal runtime-selection and coordinator behavior are green.

---
*Phase: 51-unsupported-platform-contract*
*Completed: 2026-05-18*
