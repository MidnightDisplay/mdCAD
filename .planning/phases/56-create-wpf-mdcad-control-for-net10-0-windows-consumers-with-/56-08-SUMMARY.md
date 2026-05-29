---
phase: 56-create-wpf-mdcad-control-for-net10-0-windows-consumers-with-
plan: 08
subsystem: ui
tags: [wpf, net10-windows, sample-host, minimal-host]
requires:
  - phase: 56-06
    provides: WPF control library with backend/runtime-copy wiring
provides:
  - Minimal WPF proof host that project-references MdCad.Wpf.Control
  - Single-window sealed WPF sample bound only to JsonlPath
  - Smallest WPF consumer path kept separate from the diagnostic host scope
affects: [phase-56-plan-09]
tech-stack:
  added: []
  patterns: [WPF minimal proof host, single-control sealed consumer sample, direct project-reference consumption]
key-files:
  created:
    - samples/wpf-host-minimal/WpfHostMinimal.csproj
    - samples/wpf-host-minimal/App.xaml
    - samples/wpf-host-minimal/App.xaml.cs
    - samples/wpf-host-minimal/MainWindow.xaml
    - samples/wpf-host-minimal/MainWindow.xaml.cs
    - samples/wpf-host-minimal/ViewModels/MainWindowViewModel.cs
  modified: []
key-decisions:
  - Keep the minimal host to one window and one MdCadEmbeddedControl so it stays a truthful smallest-consumer proof.
  - Bind only JsonlPath and set sealed/viewer-start defaults in XAML instead of importing diagnostic-host lifecycle chrome.
patterns-established:
  - "Minimal WPF consumer samples in mdCAD should use a direct project reference plus a single bound MdCadEmbeddedControl instance."
  - "The WPF minimal host should stay sealed and omit diagnostic controls so onboarding proof stays distinct from runtime-harness proof."
requirements-completed: [P56-04]
duration: 8 min
completed: 2026-05-29
---

# Phase 56 Plan 08: Add the minimal sealed WPF host as the smallest consumer proof Summary

**A new `samples/wpf-host-minimal` app now proves the smallest WPF consumer path with one sealed `MdCadEmbeddedControl`, one bound `JsonlPath`, and no diagnostic-host chrome.**

## Performance

- **Duration:** 8 min
- **Tasks:** 1
- **Files modified:** 6

## Accomplishments
- Added a standalone `WpfHostMinimal` WPF app that project-references `MdCad.Wpf.Control`.
- Kept the host surface to a single `MdCadEmbeddedControl` configured for sealed presentation with a bound `JsonlPath`.
- Preserved the proof split by leaving lifecycle/diagnostic responsibilities in the richer WPF harness instead of this onboarding sample.

## Task Commits

User requested no commits for this execution, so no task or metadata commits were created.

## Files Created/Modified
- `samples/wpf-host-minimal/WpfHostMinimal.csproj` - defines the minimal `net10.0-windows` WPF app and references the reusable control project.
- `samples/wpf-host-minimal/App.xaml` - declares the app entry point with `MainWindow.xaml` startup.
- `samples/wpf-host-minimal/App.xaml.cs` - minimal WPF application partial class.
- `samples/wpf-host-minimal/MainWindow.xaml` - hosts a single sealed `MdCadEmbeddedControl` bound to `JsonlPath`.
- `samples/wpf-host-minimal/MainWindow.xaml.cs` - assigns the minimal view model as window data context.
- `samples/wpf-host-minimal/ViewModels/MainWindowViewModel.cs` - exposes the simple `JsonlPath` binding source.

## Decisions Made
- Matched the Avalonia minimal-host shape by using a single control instance inside one window.
- Left `JsonlPath` empty by default so the sample proves binding/instantiation without pretending to be the richer diagnostic harness.

## Deviations from Plan

None - plan executed exactly as written.

## Known Stubs

None.

## Threat Flags

None.

## Issues Encountered

None.

## User Setup Required

None - verification stayed local to the repo.

## Next Phase Readiness
- The repo now has both the reusable WPF control and the smallest WPF consumer proof host needed for downstream validation.
- Phase 56-09 can build this host as part of the final WPF proof ledger.

## Self-Check: PASSED

- Verified `samples/wpf-host-minimal/WpfHostMinimal.csproj` exists.
- Verified `samples/wpf-host-minimal/MainWindow.xaml` exists.
- Verified `samples/wpf-host-minimal/ViewModels/MainWindowViewModel.cs` exists.
- Automated verification passed: `dotnet build .\samples\wpf-host-minimal\WpfHostMinimal.csproj -c Release`
- Commit-hash verification was intentionally skipped because this execution honored the user's no-commit request.

---
*Phase: 56-create-wpf-mdcad-control-for-net10-0-windows-consumers-with-*
*Completed: 2026-05-29*
