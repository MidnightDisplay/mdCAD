---
phase: 54-docs-and-onboarding-truthfulness
plan: 02
subsystem: quickstart-onboarding
tags: [docs, onboarding, quickstart, avalonia, sample]

# Dependency graph
requires:
  - phase: 54-docs-and-onboarding-truthfulness
    provides: Updated README support boundary from plan 01
provides:
  - Updated QUICKSTART that matches the plain-net10 host contract
  - Neutral minimal-sample JsonlPath onboarding default
  - Green validation rows 54-02-01 and 54-02-02
affects: [phase-54]

# Tech tracking
tech-stack:
  added: []
  patterns: [truthful onboarding defaults, unsupported-runtime documentation]

key-files:
  created:
    - .planning/phases/54-docs-and-onboarding-truthfulness/54-02-SUMMARY.md
  modified:
    - samples/avalonia-mdcad-control/QUICKSTART.md
    - samples/avalonia-host-minimal/ViewModels/MainWindowViewModel.cs
    - .planning/phases/54-docs-and-onboarding-truthfulness/54-VALIDATION.md

key-decisions:
  - "The control quickstart now documents a plain net10 host contract while keeping the embedded viewer explicitly Windows-only at runtime."
  - "The minimal sample now uses `string.Empty` for `JsonlPath`, making startup import opt-in and avoiding a misleading personal absolute path."
  - "The minimal sample remains a compile/build proof surface; the authoritative Windows runtime proof stays on `samples/avalonia-host`."

patterns-established:
  - "Pattern 1: Use neutral or replace-me onboarding defaults instead of repo-external personal paths."
  - "Pattern 2: Document unsupported runtime behavior directly in the consumer quickstart instead of leaving it implicit."

requirements-completed: [PROOF-02]

# Metrics
duration: continued-session
completed: 2026-05-18
---

# Phase 54 Plan 02: QUICKSTART and Minimal Sample Summary

**Wave 2 is complete: the control QUICKSTART now matches the plain `net10.0` host contract, the minimal sample uses a truthful neutral `JsonlPath` default, and the minimal-host build lane remains green.**

## Performance

- **Duration:** continued from the Phase 54 execution session
- **Completed:** 2026-05-18T18:27:36.1741781+01:00
- **Tasks:** 2
- **Files modified:** 3

## Accomplishments

- Changed `samples/avalonia-mdcad-control/QUICKSTART.md` so it tells consumers to target plain `net10.0`.
- Added explicit unsupported-platform guidance to QUICKSTART: the host/control remains valid, but embedded viewing is Windows-only at runtime and will not launch on unsupported platforms.
- Replaced the personal `GeoMate` JSONL path example with `string.Empty` and a replace-with-your-own-path note.
- Updated the minimal sample viewmodel so its `JsonlPath` default is neutral and no longer repo-external.
- Rebuilt `samples/avalonia-host-minimal` in Release after the onboarding cleanup.
- Marked validation rows `54-02-01` and `54-02-02` green.

## Task Commits

1. **Task 1: Rewrite QUICKSTART around the plain-net10 host contract and unsupported-runtime truth** - `83a5825` (docs)
2. **Task 2: Clean up the minimal sample JsonlPath example only as far as truthfulness requires** - `83a5825` (docs)

## Files Created/Modified

- `samples/avalonia-mdcad-control/QUICKSTART.md` - now documents the plain-net10 host contract, unsupported runtime behavior, and the compile/runtime proof split.
- `samples/avalonia-host-minimal/ViewModels/MainWindowViewModel.cs` - now uses `string.Empty` for `JsonlPath`.
- `.planning/phases/54-docs-and-onboarding-truthfulness/54-VALIDATION.md` - rows `54-02-01` and `54-02-02` are green.

## Decisions Made

- Kept the minimal sample onboarding default neutral instead of inventing local file discovery or shipping a new example asset.
- Preserved `JsonlPath` as the public example property so the quickstart still mirrors the sample layout.
- Kept QUICKSTART scoped to consumer wiring and proof-boundary truth rather than expanding into broader docs cleanup.

## Deviations from Plan

- Tasks 1 and 2 landed together in one cohesive docs/sample commit because QUICKSTART and the minimal sample share the same onboarding example and truth boundary.

## Issues Encountered

- None.

## User Setup Required

- None.

## Next Phase Readiness

- `54-03` can now close the validation ledger and proof lock against the already-approved Phase 53 proof surfaces.

---
*Phase: 54-docs-and-onboarding-truthfulness*
*Completed: 2026-05-18*
