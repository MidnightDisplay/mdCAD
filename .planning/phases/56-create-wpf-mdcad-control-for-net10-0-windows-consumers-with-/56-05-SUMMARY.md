---
phase: 56-create-wpf-mdcad-control-for-net10-0-windows-consumers-with-
plan: 05
subsystem: ui
tags: [wpf, xaml, xunit, sta, net10-windows]
requires:
  - phase: 56-02
    provides: Shared launch snapshot, presentation mode, and lifecycle/runtime contracts for future UI shells
provides:
  - Compile-ready WPF mdCAD control scaffold targeting net10.0-windows
  - STA-backed xUnit parity tests that lock the WPF public surface before backend work
  - Shared-core launch snapshot consumption from the WPF shell
affects: [phase-56-plan-06, phase-56-plan-07, phase-56-plan-08, phase-56-plan-09]
tech-stack:
  added: []
  patterns: [WPF dependency-property parity scaffold before backend wiring, custom STA xUnit helper without UI automation packages]
key-files:
  created:
    - samples/wpf-mdcad-control/MdCad.Wpf.Control.csproj
    - samples/wpf-mdcad-control/MdCadEmbeddedControl.xaml
    - samples/wpf-mdcad-control/MdCadEmbeddedControl.xaml.cs
    - samples/wpf-mdcad-control.tests/MdCad.Wpf.Control.Tests.csproj
    - samples/wpf-mdcad-control.tests/WpfStaThreadFixture.cs
    - samples/wpf-mdcad-control.tests/MdCadEmbeddedControlTests.cs
  modified: []
key-decisions:
  - Lock the WPF host-facing API with real dependency properties and no-op StartAsync/StopAsync before introducing the HwndHost/backend seam.
  - Use a repo-local STA thread helper instead of adding a WPF-specific test package for this parity slice.
patterns-established:
  - "WPF shell work starts by mirroring Avalonia launch/property semantics against MdCad.Embed.Core types before any native-host implementation."
  - "STA construction proof lives in xUnit with a small fixture rather than heavyweight UI automation."
requirements-completed: [P56-01]
duration: 18 min
completed: 2026-05-29
---

# Phase 56 Plan 05: Create the WPF control scaffold and STA-backed parity test lane Summary

**A net10.0-windows WPF mdCAD control scaffold now mirrors the Avalonia host-facing property/method contract and proves STA-safe construction through a dedicated xUnit lane.**

## Performance

- **Duration:** 18 min
- **Tasks:** 1
- **Files modified:** 6

## Accomplishments
- Added `samples/wpf-mdcad-control` as a compile-ready WPF control project referencing `MdCad.Embed.Core`.
- Implemented parity dependency properties for `JsonlPath`, `StartupLiveRefreshEnabled`, `ViewportOnlyStartupMode`, `AutoStart`, and `PresentationMode`, plus public `StartAsync()` / `StopAsync()`.
- Added `samples/wpf-mdcad-control.tests` with a custom STA fixture and parity tests covering WPF construction and launch-snapshot updates.

## Task Commits

User requested no commits for this execution, so no task or metadata commits were created.

## Files Created/Modified
- `samples/wpf-mdcad-control/MdCad.Wpf.Control.csproj` - WPF class-library scaffold targeting `net10.0-windows10.0.19041.0`.
- `samples/wpf-mdcad-control/MdCadEmbeddedControl.xaml` - Minimal WPF control markup for the scaffolded shell.
- `samples/wpf-mdcad-control/MdCadEmbeddedControl.xaml.cs` - Dependency-property parity surface and shared-core launch snapshot capture.
- `samples/wpf-mdcad-control.tests/MdCad.Wpf.Control.Tests.csproj` - WPF xUnit lane for parity validation.
- `samples/wpf-mdcad-control.tests/WpfStaThreadFixture.cs` - Small STA execution helper for WPF control construction tests.
- `samples/wpf-mdcad-control.tests/MdCadEmbeddedControlTests.cs` - Public-surface and launch-snapshot parity tests.

## Decisions Made
- Used WPF `DependencyProperty.Register(...)` for the parity surface so the scaffold matches consumer expectations before backend work starts.
- Kept `StartAsync()` / `StopAsync()` backend-free and non-throwing in this slice to preserve the public contract without prematurely inventing runtime behavior.

## Deviations from Plan

None - plan executed exactly as written.

## Known Stubs

- `samples/wpf-mdcad-control/MdCadEmbeddedControl.xaml` - The visual body is intentionally minimal in this scaffold plan because `HwndHost` and runtime wiring are deferred to Plan 56-06.

## Issues Encountered
- None

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness
- Ready for the WPF backend/HwndHost plan to replace the temporary shell body while keeping the now-locked property/method contract.
- STA construction proof is in place for future backend wiring changes.

## Self-Check: PASSED

- Verified `samples/wpf-mdcad-control/MdCadEmbeddedControl.xaml.cs` exists.
- Verified `samples/wpf-mdcad-control.tests/MdCadEmbeddedControlTests.cs` exists.
- Automated verification passed: `dotnet build .\samples\wpf-mdcad-control.tests\MdCad.Wpf.Control.Tests.csproj -c Release`
- Additional validation passed: `dotnet test .\samples\wpf-mdcad-control.tests\MdCad.Wpf.Control.Tests.csproj -c Release --no-build`
- Commit-hash verification was intentionally skipped because this execution honored the user's no-commit request.

---
*Phase: 56-create-wpf-mdcad-control-for-net10-0-windows-consumers-with-*
*Completed: 2026-05-29*
