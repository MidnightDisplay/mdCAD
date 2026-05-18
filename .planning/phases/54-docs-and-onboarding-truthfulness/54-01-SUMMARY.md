---
phase: 54-docs-and-onboarding-truthfulness
plan: 01
subsystem: readme-contract
tags: [docs, onboarding, readme, avalonia, runtime-boundary]

# Dependency graph
requires: []
provides:
  - Updated README support contract for Phase 54
  - Green validation rows 54-01-01 and 54-01-02
  - Clear split between plain-net10 compile/build proof and Windows runtime proof in the top-level onboarding surface
affects: [phase-54]

# Tech tracking
tech-stack:
  added: []
  patterns: [docs truth alignment, compile-vs-runtime proof split]

key-files:
  created:
    - .planning/phases/54-docs-and-onboarding-truthfulness/54-01-SUMMARY.md
  modified:
    - README.md
    - .planning/phases/54-docs-and-onboarding-truthfulness/54-VALIDATION.md

key-decisions:
  - "README now treats `samples/avalonia-host-minimal` as the plain net10 compile/build proof surface and `samples/avalonia-host` as the Windows runtime proof surface."
  - "The top-level docs now say the reusable control can be referenced from plain net10 hosts while the embedded mdCAD viewer remains Windows-only at runtime."
  - "Unsupported-platform behavior is described as an intentional, truthful warning path rather than implied runtime support."

patterns-established:
  - "Pattern 1: State the host/control compile contract and the embedded runtime contract separately in primary onboarding docs."
  - "Pattern 2: Treat `dotnet run` on the minimal host as optional local smoke, not as proof of cross-platform runtime embedding."

requirements-completed: [PROOF-02]

# Metrics
duration: continued-session
completed: 2026-05-18
---

# Phase 54 Plan 01: README Contract Rewrite Summary

**Wave 1 is complete: `README.md` now describes the plain `net10.0` host contract, the Windows-only embedded runtime boundary, and the two distinct proof surfaces without reopening runtime implementation.**

## Performance

- **Duration:** continued from the Phase 54 execution session
- **Completed:** 2026-05-18T18:25:25.2437251+01:00
- **Tasks:** 2
- **Files modified:** 2

## Accomplishments

- Rewrote the embedding feature bullet so it says `MdCad.Avalonia.Control` can be referenced from a plain `net10.0` Avalonia host.
- Reframed `samples/avalonia-host` as the authoritative Windows runtime proof harness.
- Reframed `samples/avalonia-host-minimal` as the plain `net10.0` compile/build proof host.
- Explicitly documented that the embedded mdCAD viewer remains Windows-only at runtime.
- Added truthful unsupported-platform wording to README so unsupported hosts are described as valid host/control scenarios with a blocked embedded viewer launch.
- Marked validation rows `54-01-01` and `54-01-02` green.

## Task Commits

1. **Task 1: Rewrite the README sample-host contract around the proven Phase 53 split** - `e0ae012` (docs)
2. **Task 2: Add explicit Windows-only runtime and unsupported-platform truth to README** - `e0ae012` (docs)

## Files Created/Modified

- `README.md` - now separates compile/build proof from runtime proof and documents unsupported runtime behavior truthfully.
- `.planning/phases/54-docs-and-onboarding-truthfulness/54-VALIDATION.md` - rows `54-01-01` and `54-01-02` are green.

## Decisions Made

- Kept README focused on onboarding truth rather than broader build-doc cleanup.
- Preserved `samples/avalonia-host` as the runtime proof surface and did not turn the minimal sample into a cross-platform runtime claim.
- Used the implemented unsupported-platform meaning rather than inventing new wording.

## Deviations from Plan

- Tasks 1 and 2 landed together in one cohesive README/docs commit because both changes touched the same support-contract section of `README.md`.

## Issues Encountered

- None.

## User Setup Required

- None.

## Next Phase Readiness

- `54-02` can now align QUICKSTART and the minimal sample against the README contract without redefining the proof split.

---
*Phase: 54-docs-and-onboarding-truthfulness*
*Completed: 2026-05-18*
