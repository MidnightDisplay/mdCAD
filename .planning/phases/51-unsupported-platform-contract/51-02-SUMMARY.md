---
phase: 51-unsupported-platform-contract
plan: 02
subsystem: host-runtime
tags: [avalonia, embed, unsupported-platform, backend, coordinator]

# Dependency graph
requires:
  - phase: 51-unsupported-platform-contract
    provides: Wave 0 red tests for unsupported backend selection, canonical unsupported messaging, and blocked coordinator behavior
provides:
  - Internal backend factory that selects supported Windows vs unsupported runtime behavior without widening the public API
  - Single-source canonical unsupported runtime message plus inert unsupported backend implementation
  - Blocked-reason-aware coordinator behavior that suppresses unsupported launch work and fails explicit starts deterministically
affects: [phase-51, phase-52]

# Tech tracking
tech-stack:
  added: []
  patterns: [internal backend factory selection, canonical unsupported-runtime message helper, blocked-reason coordinator gating]

key-files:
  created:
    - .planning/phases/51-unsupported-platform-contract/51-02-SUMMARY.md
    - samples/avalonia-mdcad-control/Host/MdCadEmbedBackendFactory.cs
    - samples/avalonia-mdcad-control/Host/MdCadUnsupportedRuntime.cs
    - samples/avalonia-mdcad-control/Host/UnsupportedMdCadEmbedBackend.cs
  modified:
    - samples/avalonia-mdcad-control/Host/IMdCadEmbedBackend.cs
    - samples/avalonia-mdcad-control/Host/MdCadSessionCoordinator.cs
    - samples/avalonia-mdcad-control/Host/Windows/WindowsMdCadEmbedBackend.cs

key-decisions:
  - "Kept the unsupported-platform decision internal through a factory seam so the public control API and Windows runtime path stay unchanged until the later shell wave."
  - "Made the unsupported message single-source in `MdCadUnsupportedRuntime` so backend throws, coordinator warnings, and later shell text all converge on one truth."
  - "Added the coordinator's blocked-reason delegate as an optional constructor seam first, then wired the actual blocked behavior in a separate commit so the task history matches the plan."

patterns-established:
  - "Pattern 1: Add unsupported runtime behavior as an inert backend implementation instead of branching the public shell into a second lifecycle path."
  - "Pattern 2: Let the coordinator own blocked launch suppression and immediate imperative API failure so AutoStart, property changes, and explicit start all share one gate."

requirements-completed: [PLAT-01, PLAT-02, PLAT-03]

# Metrics
duration: continued-session
completed: 2026-05-18
---

# Phase 51 Plan 02: Unsupported Backend and Coordinator Summary

**Phase 51 now has the internal unsupported-runtime contract implemented: the control can select an inert unsupported backend internally, use one canonical Windows-only message, and keep unsupported auto-start/property-change behavior descriptive-only while explicit start fails deterministically.**

## Performance

- **Duration:** continued from the Phase 51 execution session
- **Completed:** 2026-05-18T14:33:30.5395754+01:00
- **Tasks:** 2
- **Files modified:** 7

## Accomplishments

- Added `MdCadEmbedBackendFactory` so backend selection is centralized and can force the unsupported branch through an internal platform probe.
- Added `MdCadUnsupportedRuntime` and `UnsupportedMdCadEmbedBackend` so unsupported runtime behavior is explicit, inert, and lifecycle-compatible.
- Extended `IMdCadEmbedBackend` with `StartBlockedReason` while keeping `WindowsMdCadEmbedBackend` behavior unchanged beyond returning `null` for that surface.
- Updated `MdCadSessionCoordinator` to prefer blocked-reason warnings over snapshot warnings, suppress unsupported launch work during reconcile, and fail blocked `StartAsync()` immediately while leaving blocked `StopAsync()` safe.
- Re-closed the full control test suite green after the internal unsupported-runtime behavior landed.

## Task Commits

1. **Task 1: Add the internal backend factory, canonical unsupported message, and inert unsupported backend** - `c3cd51d` (feat)
2. **Task 2: Make the coordinator blocked-reason aware so unsupported reconcile paths never launch** - `fd7e928` (feat)

## Files Created/Modified

- `samples/avalonia-mdcad-control/Host/MdCadEmbedBackendFactory.cs` - Central internal selector for supported vs unsupported backend creation.
- `samples/avalonia-mdcad-control/Host/MdCadUnsupportedRuntime.cs` - Single canonical Windows-only unsupported message.
- `samples/avalonia-mdcad-control/Host/UnsupportedMdCadEmbedBackend.cs` - Inert unsupported backend with safe lifecycle no-ops and immediate unsupported start failure.
- `samples/avalonia-mdcad-control/Host/IMdCadEmbedBackend.cs` - Adds `StartBlockedReason` to the backend seam.
- `samples/avalonia-mdcad-control/Host/MdCadSessionCoordinator.cs` - Adds blocked-reason-aware reconcile and explicit start behavior.
- `samples/avalonia-mdcad-control/Host/Windows/WindowsMdCadEmbedBackend.cs` - Exposes `StartBlockedReason` as `null` and otherwise preserves the locked Windows runtime path.

## Decisions Made

- Kept unsupported backend selection and message ownership internal-only for this wave; the public control surface still changes in Plan 51-03 only.
- Treated unsupported runtime as a blocked lifecycle state inside the existing coordinator rather than creating a second unsupported-specific orchestration flow.
- Preserved the Phase 50 Windows implementation exactly except for the new null blocked-reason surface.

## Deviations from Plan

None. The implementation matched the red proof and stayed inside the Phase 51 boundary.

## Issues Encountered

- The first task-1 implementation pass exposed a constructor optional-parameter mismatch at the existing control call site; moving the hook to the end with a default restored behavior-preserving compatibility before the blocked behavior landed.

## User Setup Required

None.

## Next Phase Readiness

- Plan `51-03` can now wire the public control shell through `MdCadEmbedBackendFactory`, surface the canonical unsupported truth immediately, and keep diagnostic-only requested JSONL/live-refresh details secondary.
- The internal unsupported-runtime contract is stable and ready for shell/UI proof without reopening the Windows backend path.

---
*Phase: 51-unsupported-platform-contract*
*Completed: 2026-05-18*
