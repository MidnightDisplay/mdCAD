---
phase: 56-create-wpf-mdcad-control-for-net10-0-windows-consumers-with-
plan: 04
subsystem: ui
tags: [avalonia, shared-core, parity, xunit, net10]
requires:
  - phase: 56-03
    provides: Shared Windows start-info and HWND helper seams in MdCad.Embed.Core.Windows
provides:
  - Avalonia control shell now captures launch snapshots through MdCad.Embed.Core
  - Avalonia control now runs relaunch coordination through MdCad.Embed.Core without public API drift
  - Avalonia parity tests lock the existing public surface and shared-core wiring
affects: [phase-56-plan-05, phase-56-plan-06]
tech-stack:
  added: []
  patterns: [Avalonia shell translates between legacy host contracts and shared-core snapshot/coordinator types]
key-files:
  created: []
  modified:
    - samples/avalonia-mdcad-control/MdCadEmbeddedControl.axaml.cs
    - samples/avalonia-mdcad-control.tests/MdCadEmbeddedControlTests.cs
key-decisions:
  - Kept the Avalonia public property and method surface unchanged while switching the shell-owned snapshot and coordinator state to MdCad.Embed.Core.
  - Used explicit snapshot translation at the backend boundary so this plan stayed limited to the control shell and parity tests instead of widening into a broader host-layer refactor.
duration: unknown
completed: 2026-05-28
---

# Phase 56 Plan 04: Repoint the Avalonia control surface to shared-core types without API drift Summary

**The Avalonia parity control now captures launch snapshots and runs relaunch coordination through `MdCad.Embed.Core`, while preserving the existing public property/method surface and diagnostics behavior for upcoming WPF parity work.**

## Performance

- **Tasks:** 1
- **Files modified:** 2

## Accomplishments

- Rewired `MdCadEmbeddedControl` to store its shell snapshot state as `MdCad.Embed.Core.MdCadLaunchSnapshot` and to use the shared `MdCadSessionCoordinator`.
- Added shell-boundary translation helpers so the existing backend contract remains intact without changing the public Avalonia API or widening plan scope.
- Expanded `MdCadEmbeddedControlTests` to lock the public API surface and assert shared-core snapshot/coordinator wiring.

## Task Commits

User requested no commits, so no TDD or task commits were created.

## Files Created/Modified

- `samples/avalonia-mdcad-control/MdCadEmbeddedControl.axaml.cs` - switched shell-owned snapshot/coordinator state to shared-core types and added backend-boundary translation helpers.
- `samples/avalonia-mdcad-control.tests/MdCadEmbeddedControlTests.cs` - added public API parity coverage and shared-core wiring assertions.

## Decisions Made

- Kept `MdCadPresentationMode` in the Avalonia control namespace to avoid public API drift during the shared-core repointing.
- Local host/backend contracts stay in place for now; the control shell adapts shared-core snapshots at that seam until later Phase 56 work can widen the refactor safely.

## Deviations from Plan

None - plan executed as written.

## Known Stubs

None.

## Self-Check: PASSED

- Verified `samples/avalonia-mdcad-control/MdCadEmbeddedControl.axaml.cs` and `samples/avalonia-mdcad-control.tests/MdCadEmbeddedControlTests.cs` exist on disk.
- Verified `.planning/phases/56-create-wpf-mdcad-control-for-net10-0-windows-consumers-with-/56-04-SUMMARY.md` exists on disk.
- Automated verification passed:
  - `dotnet test .\samples\avalonia-mdcad-control.tests\MdCad.Avalonia.Control.Tests.csproj -c Release --filter "FullyQualifiedName~MdCadEmbeddedControlTests"`
- Commit-hash verification was skipped because this execution honored the user's no-commit request.
