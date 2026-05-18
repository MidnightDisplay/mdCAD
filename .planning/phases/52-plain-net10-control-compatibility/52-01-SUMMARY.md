---
phase: 52-plain-net10-control-compatibility
plan: 01
subsystem: host-contract
tags: [avalonia, net10, tfm, control, compatibility]

# Dependency graph
requires:
  - phase: 51-unsupported-platform-contract
    provides: Internal supported/unsupported backend split, Windows-only runtime truth, and a public shell that no longer requires a Windows-only public TFM
provides:
  - Plain-`net10.0` host-facing target framework for the reusable control
  - Preserved runtime content-copy contract for `mdcad-runtime`
  - Green Wave 1 regression bundle after the control boundary widens
affects: [phase-52, phase-53]

# Tech tracking
tech-stack:
  added: []
  patterns: [plain host-facing control TFM, preserved runtime content copy contract, compatibility-first regression close]

key-files:
  created:
    - .planning/phases/52-plain-net10-control-compatibility/52-01-SUMMARY.md
  modified:
    - .planning/phases/52-plain-net10-control-compatibility/52-VALIDATION.md
    - samples/avalonia-mdcad-control/MdCad.Avalonia.Control.csproj

key-decisions:
  - "Changed only the reusable control project's target framework in this wave because the compatibility wall lived at the control boundary, not in the hosts."
  - "Kept the runtime content-copy item group untouched so the Windows runtime bundle behavior from Phases 48-51 stays intact."
  - "Closed Wave 1 with the full regression bundle, not just the control build, so the widened contract was proven against both sample consumers immediately."

patterns-established:
  - "Pattern 1: Fix compile-time compatibility by widening the host-facing control target first, then prove consumers against that widened contract in later waves."
  - "Pattern 2: Treat runtime bundle copy behavior as part of the control contract even when only the public TFM changes."

requirements-completed: [HOSTC-01]

# Metrics
duration: continued-session
completed: 2026-05-18
---

# Phase 52 Plan 01: Control TFM Compatibility Summary

**Phase 52 now has the control-side compatibility wall removed: `MdCad.Avalonia.Control` targets plain `net10.0`, its runtime content-copy behavior is preserved, and the immediate regression bundle remains green.**

## Performance

- **Duration:** continued from the Phase 52 execution session
- **Completed:** 2026-05-18T15:24:23.2631294+01:00
- **Tasks:** 1
- **Files modified:** 2

## Accomplishments

- Retargeted `samples/avalonia-mdcad-control/MdCad.Avalonia.Control.csproj` from `net10.0-windows10.0.19041.0` to plain `net10.0`.
- Left the `runtime\win-x64` content-copy block unchanged so Windows consumers still receive the same `mdcad-runtime` payload contract.
- Updated `52-VALIDATION.md` to record Task `52-01-01` as green after the exact control build lane passed.
- Re-closed the full Wave 1 regression bundle: control tests, minimal host build, and Windows diagnostic host build all stayed green after the TFM widening.

## Task Commits

1. **Task 1: Retarget the reusable control project to plain net10 without touching the Windows runtime seam** - `59467f6` (feat)

## Files Created/Modified

- `samples/avalonia-mdcad-control/MdCad.Avalonia.Control.csproj` - Now targets plain `net10.0` while preserving the existing assembly identity, package references, and runtime content-copy block.
- `.planning/phases/52-plain-net10-control-compatibility/52-VALIDATION.md` - Records Task `52-01-01` as green.

## Decisions Made

- Did not multi-target or split packages; the single-target plain-`net10.0` control was sufficient for this wave.
- Did not change any public control API, runtime message contract, or Windows-specific implementation files.
- Used the full regression bundle immediately after the change to guard against hidden consumer fallout.

## Deviations from Plan

None. The wave stayed within the exact compatibility-boundary scope it planned.

## Issues Encountered

- None. The control built cleanly as plain `net10.0`, and both sample consumers continued to build against it on the first pass.

## User Setup Required

None.

## Next Phase Readiness

- Plan `52-02` can now retarget `samples/avalonia-host-minimal` itself as the explicit plain-`net10.0` proof host.
- The remaining work is host-side proof and final regression closeout, not more control-boundary architecture change.

---
*Phase: 52-plain-net10-control-compatibility*
*Completed: 2026-05-18*
