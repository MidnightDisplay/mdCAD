---
phase: 48-reusable-avalonia-mdcad-user-control
plan: 01
subsystem: ui
tags: [avalonia, windows, usercontrol, nativecontrolhost, embedding]

# Dependency graph
requires:
  - phase: 47-sample-host-workflow-proof
    provides: Repeatable child-HWND hosting, placeholder recreation, and honest host-owned embedded status patterns.
provides:
  - Windows-only Avalonia control library project for reusable mdCAD embedding
  - Bindable public control contract with startup configuration properties and async start/stop entrypoints
  - Extracted reusable NativeControlHost placeholder HWND seam owned by the library
affects: [phase-48-runtime-bundle, phase-48-relaunch-coordinator, avalonia-host]

# Tech tracking
tech-stack:
  added: [Avalonia control library project, project-local NuGet.Config]
  patterns: [bindable UserControl launch contract, internal NativeControlHost placeholder seam]

key-files:
  created:
    - samples/avalonia-mdcad-control/MdCad.Avalonia.Control.csproj
    - samples/avalonia-mdcad-control/MdCadEmbeddedControl.axaml
    - samples/avalonia-mdcad-control/Host/MdCadPresentationMode.cs
    - samples/avalonia-mdcad-control/Host/EmbedNativeControlHost.cs
    - samples/avalonia-mdcad-control/NuGet.Config
  modified:
    - .gitignore
    - samples/avalonia-mdcad-control/MdCadEmbeddedControl.axaml.cs

key-decisions:
  - "Keep the reusable viewer shell as a fixed UserControl with StyledProperty configuration instead of a constructor-only options object."
  - "Move the proven Win32 placeholder HWND implementation into an internal library seam so later launch/relaunch work reuses the validated child-window behavior."
  - "Add a project-local NuGet.Config so the new library stays independently buildable despite a machine-level ProGet source returning 403."

patterns-established:
  - "Pattern 1: Reusable Windows host surfaces expose launch configuration through StyledProperty-backed control properties plus async lifecycle entrypoints."
  - "Pattern 2: The NativeControlHost placeholder seam stays library-owned and internal; consumers interact through MdCadEmbeddedControl rather than direct HWND plumbing."

requirements-completed: [P48-01]

# Metrics
duration: 8 min
completed: 2026-05-15
---

# Phase 48 Plan 01: Reusable control foundation summary

**Windows-only Avalonia `MdCadEmbeddedControl` library with bindable launch properties and an internal extracted Win32 placeholder HWND seam**

## Performance

- **Duration:** 8 min
- **Started:** 2026-05-15T15:23:44.031Z
- **Completed:** 2026-05-15T15:31:41.3981797+01:00
- **Tasks:** 2
- **Files modified:** 7

## Accomplishments
- Added `samples/avalonia-mdcad-control` as a standalone Windows-only Avalonia library that builds independently from the sample host.
- Added `MdCadEmbeddedControl` with bindable `JsonlPath`, `StartupLiveRefreshEnabled`, `AutoStart`, and `PresentationMode` properties plus explicit async start/stop entrypoints.
- Extracted the proven `NativeControlHost` placeholder HWND creation/destruction seam into the new library so later runtime and relaunch work can reuse the validated child-window behavior.
- Added standard `.NET` `bin/` and `obj/` ignore rules so the new library and sample host do not leave generated output noise in the repo.

## Task Commits

Each task was committed atomically:

1. **Task 1: Create the Windows-only reusable control project and public bindable contract** - `b0c92e1` (feat)
2. **Task 2: Extract the reusable placeholder-HWND host seam into the new library** - `776e9a2` (feat)

**Plan metadata:** pending final docs commit

## Files Created/Modified
- `samples/avalonia-mdcad-control/MdCad.Avalonia.Control.csproj` - Declares the Windows-only reusable control library and keeps the repo on the existing Avalonia 11.3.x line.
- `samples/avalonia-mdcad-control/MdCadEmbeddedControl.axaml` - Provides the minimal reusable viewer shell with a warning region and embedded-surface container.
- `samples/avalonia-mdcad-control/MdCadEmbeddedControl.axaml.cs` - Defines the bindable public contract, async lifecycle entrypoints, and the internal composition point for the placeholder host.
- `samples/avalonia-mdcad-control/Host/MdCadPresentationMode.cs` - Declares the sealed vs diagnostic enum that later plans will render.
- `samples/avalonia-mdcad-control/Host/EmbedNativeControlHost.cs` - Owns reusable Win32 placeholder HWND creation/destruction for the control library.
- `samples/avalonia-mdcad-control/NuGet.Config` - Restricts the new standalone library restore to `nuget.org` so it can build independently on this machine.
- `.gitignore` - Ignores standard `.NET` build output directories to keep sample-project builds from surfacing as untracked noise.

## Decisions Made
- Kept the Phase 48 public surface property-based and XAML-friendly by registering `StyledProperty`s directly on `MdCadEmbeddedControl`.
- Preserved the Phase 43-47 embedding seam by copying the proven placeholder HWND implementation into the library rather than rewriting it around a different native-host approach.
- Scoped the restore fix to the new library with a local `NuGet.Config` instead of broadening repo-wide NuGet behavior.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Added project-local NuGet source scoping for the new library**
- **Found during:** Task 1 (Create the Windows-only reusable control project and public bindable contract)
- **Issue:** The first standalone build of `MdCad.Avalonia.Control.csproj` failed because a machine-level ProGet source returned `403 Forbidden` while restoring Avalonia packages.
- **Fix:** Added `samples/avalonia-mdcad-control/NuGet.Config` with the same `nuget.org`-only source restriction already used by the sample host.
- **Files modified:** `samples/avalonia-mdcad-control/NuGet.Config`
- **Verification:** `dotnet build samples/avalonia-mdcad-control/MdCad.Avalonia.Control.csproj -c Release` and `dotnet build samples/avalonia-host/AvaloniaHost.csproj -c Release`
- **Committed in:** `b0c92e1` (part of Task 1 commit)

---

**Total deviations:** 1 auto-fixed (1 blocking)
**Impact on plan:** The fix was required to satisfy the plan's "builds independently" contract. No scope creep.

## Issues Encountered
- The first restore for the new library failed against a machine-level ProGet feed with `403 Forbidden`; copying the sample host's project-local NuGet source restriction resolved the blocker cleanly.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness
- Ready for `48-02-PLAN.md` to materialize the pinned runtime bundle, deterministic `mdcad-runtime/` output copy, and output-rooted runtime resolution seam.
- The bindable control contract and reusable placeholder HWND seam now exist for the later launch/relaunch coordinator to consume.

---
*Phase: 48-reusable-avalonia-mdcad-user-control*
*Completed: 2026-05-15*
