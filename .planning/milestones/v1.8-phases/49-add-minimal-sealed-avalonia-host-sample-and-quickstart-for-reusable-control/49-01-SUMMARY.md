---
phase: 49-add-minimal-sealed-avalonia-host-sample-and-quickstart-for-reusable-control
plan: 01
subsystem: ui
tags: [avalonia, onboarding, sealed-mode, docs, embedding]

# Dependency graph
requires:
  - phase: 48-reusable-avalonia-mdcad-user-control
    provides: Reusable control contract, sealed mode, copied runtime bundle, and verified launch/relaunch behavior.
provides:
  - Minimal sealed Avalonia host sample bound from a viewmodel-backed `JsonlPath`
  - Control-library quickstart documentation aligned with the minimal host
  - README discovery updates for the new onboarding path
affects: [phase-49-onboarding, avalonia-host-minimal, reusable-control-docs]

# Tech tracking
tech-stack:
  added: [minimal Avalonia control consumer sample, reusable control quickstart]
  patterns: [viewmodel-bound JsonlPath startup, sealed-mode single-control host]

key-files:
  created:
    - samples/avalonia-host-minimal/AvaloniaHostMinimal.csproj
    - samples/avalonia-host-minimal/App.axaml
    - samples/avalonia-host-minimal/App.axaml.cs
    - samples/avalonia-host-minimal/Program.cs
    - samples/avalonia-host-minimal/MainWindow.axaml
    - samples/avalonia-host-minimal/MainWindow.axaml.cs
    - samples/avalonia-host-minimal/ViewModels/MainWindowViewModel.cs
    - samples/avalonia-host-minimal/NuGet.Config
    - samples/avalonia-host-minimal/app.manifest
    - samples/avalonia-mdcad-control/QUICKSTART.md
  modified:
    - README.md

key-decisions:
  - "Keep the new sample separate from the diagnostic harness so external consumers can copy the smallest possible sealed-mode setup without inheriting test controls."
  - "Bind `JsonlPath` from a simple `MainWindowViewModel` property to prove the reusable control works in a normal MVVM-style XAML host."
  - "Document the exact sealed-mode wiring path inside the control project so onboarding guidance lives beside the reusable package rather than only in the repo root."

patterns-established:
  - "Pattern 1: The lightest external host can be a single sealed `MdCadEmbeddedControl` in a grid with `JsonlPath` bound from a viewmodel property."
  - "Pattern 2: Reusable control onboarding docs should live in the control project and point at the minimal sample as the reference consumer."

requirements-completed: [P49-01, P49-02]

# Metrics
duration: 8 min
completed: 2026-05-15
---

# Phase 49 Plan 01: Minimal host onboarding summary

**Added the smallest sealed Avalonia host sample plus a step-by-step reusable-control quickstart that binds startup JSONL from a viewmodel**

## Performance

- **Duration:** 8 min
- **Started:** 2026-05-15T16:26:43+01:00
- **Completed:** 2026-05-15T16:34:25.5000263+01:00
- **Tasks:** 1
- **Files modified:** 11

## Accomplishments
- Added `samples/avalonia-host-minimal` as a Windows Avalonia sample project that hosts exactly one sealed `MdCadEmbeddedControl` inside a grid.
- Bound the control's startup `JsonlPath` from `MainWindowViewModel` using the requested hardcoded absolute JSONL path.
- Added `samples/avalonia-mdcad-control/QUICKSTART.md` with step-by-step ProjectReference, XAML namespace, viewmodel, DataContext, and sealed-control wiring guidance.
- Updated the repo README so the minimal sample and quickstart are discoverable beside the existing diagnostic harness.

## Task Commits

Implementation landed across:

1. **Task 1: Create the minimal sealed host sample and quickstart** - `b50cebf` (feat), `ba7878f` (feat)

**Plan metadata:** pending final docs commit

## Files Created/Modified
- `samples/avalonia-host-minimal/AvaloniaHostMinimal.csproj` - Minimal Windows Avalonia sample referencing the reusable control.
- `samples/avalonia-host-minimal/App.axaml` - Minimal app shell using the Fluent theme.
- `samples/avalonia-host-minimal/App.axaml.cs` - Creates the minimal main window with no extra host scaffolding.
- `samples/avalonia-host-minimal/Program.cs` - Standard Avalonia desktop bootstrap for the new sample.
- `samples/avalonia-host-minimal/MainWindow.axaml` - Hosts a single sealed `MdCadEmbeddedControl` in a grid cell.
- `samples/avalonia-host-minimal/MainWindow.axaml.cs` - Assigns the viewmodel-backed DataContext for the sample window.
- `samples/avalonia-host-minimal/ViewModels/MainWindowViewModel.cs` - Supplies the hardcoded absolute `JsonlPath`.
- `samples/avalonia-host-minimal/NuGet.Config` - Keeps restore pinned to `nuget.org` on this machine.
- `samples/avalonia-host-minimal/app.manifest` - Standard Windows compatibility manifest for the sample app.
- `samples/avalonia-mdcad-control/QUICKSTART.md` - Step-by-step control wiring guide for external consumers.
- `README.md` - Adds discovery pointers for the minimal sample and quickstart.

## Decisions Made
- Kept the new onboarding sample separate from `samples/avalonia-host` so the existing diagnostic harness remains intact.
- Used a plain viewmodel property instead of code-behind assignment to prove the control fits a normal bound XAML layout.
- Documented the control wiring in the control project itself so consumers can discover it without reading the larger repo history.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None.

## User Setup Required

None - the sample already carries the requested absolute JSONL path in its viewmodel.

## Next Phase Readiness
- Phase 49 is ready for metadata closeout and renewed milestone completion.
- v1.8 can now close with both a diagnostic proof harness and a minimal sealed consumer sample in the repo.

---
*Phase: 49-add-minimal-sealed-avalonia-host-sample-and-quickstart-for-reusable-control*
*Completed: 2026-05-15*
