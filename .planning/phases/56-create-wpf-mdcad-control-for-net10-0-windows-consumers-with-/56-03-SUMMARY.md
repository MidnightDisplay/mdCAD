---
phase: 56-create-wpf-mdcad-control-for-net10-0-windows-consumers-with-
plan: 03
subsystem: ui
tags: [windows, avalonia, shared-core, win32, xunit, net10]
requires:
  - phase: 56-02
    provides: Shared runtime resolution and session coordination in MdCad.Embed.Core
provides:
  - Shared Windows embedded-process start-info construction with locked arg ordering
  - Shared Win32 placeholder and child-HWND helper seam for Avalonia and future WPF backends
  - Avalonia backend proof that delegated launch creation preserves runtime-root and child-HWND behavior
affects: [phase-56-plan-04, phase-56-plan-05, phase-56-plan-06]
tech-stack:
  added: []
  patterns: [single-source Windows launch and child-HWND helper seam in MdCad.Embed.Core before WPF backend work]
key-files:
  created:
    - samples/mdcad-embed-core/Windows/MdCadWindowsStartInfoBuilder.cs
    - samples/mdcad-embed-core/Windows/Win32NativeMethods.cs
    - samples/mdcad-embed-core.tests/MdCadWindowsStartInfoBuilderTests.cs
  modified:
    - samples/avalonia-mdcad-control/MdCad.Avalonia.Control.csproj
    - samples/avalonia-mdcad-control/Host/Windows/EmbedNativeControlHost.cs
    - samples/avalonia-mdcad-control/Host/Windows/WindowsMdCadEmbedBackend.cs
    - samples/avalonia-mdcad-control.tests/WindowsMdCadEmbedBackendTests.cs
    - samples/avalonia-mdcad-control/Host/Windows/Win32NativeMethods.cs
key-decisions:
  - Kept the shared Windows builder on raw launch inputs so Avalonia could adopt the single-source launch seam without switching its remaining duplicated host contracts ahead of plan 04.
  - Moved placeholder creation/destruction and child-window discovery into shared Win32 helpers so Avalonia and WPF can rely on one HWND lifecycle implementation.
patterns-established:
  - "Windows launch argument ordering now lives in MdCad.Embed.Core.Windows and UI backends call into it instead of composing ProcessStartInfo inline."
  - "Child-HWND placeholder and lookup helpers are shared-core infrastructure, while UI hosts keep only shell-specific surface wiring."
requirements-completed: [P56-02]
duration: 4 min
completed: 2026-05-28
---

# Phase 56 Plan 03: Single-source the Windows start-info builder and repoint the Avalonia backend Summary

**Shared Windows launch/start-info construction and HWND helper logic now live in `MdCad.Embed.Core.Windows`, while the Avalonia backend keeps its existing runtime-root and child-HWND contract by delegating to that shared seam.**

## Performance

- **Duration:** 4 min
- **Started:** 2026-05-28T23:05:30Z
- **Completed:** 2026-05-28T23:09:25Z
- **Tasks:** 1
- **Files modified:** 9

## Accomplishments
- Added a shared `MdCadWindowsStartInfoBuilder` with xUnit proof for required arg ordering, optional flags, and working-directory behavior.
- Moved reusable placeholder-window and child-HWND lookup helpers into `MdCad.Embed.Core.Windows`.
- Repointed the Avalonia backend and native host surface to the shared Windows seam and kept backend launch tests green.

## Task Commits

User requested no commits for this execution, so no task or metadata commits were created.

## Files Created/Modified
- `samples/mdcad-embed-core/Windows/MdCadWindowsStartInfoBuilder.cs` - shared embedded-process `ProcessStartInfo` builder.
- `samples/mdcad-embed-core/Windows/Win32NativeMethods.cs` - shared placeholder HWND, sizing, and child-window helpers.
- `samples/mdcad-embed-core.tests/MdCadWindowsStartInfoBuilderTests.cs` - shared-core proof for arg order and runtime-root working directory.
- `samples/avalonia-mdcad-control/MdCad.Avalonia.Control.csproj` - adds the shared-core project reference.
- `samples/avalonia-mdcad-control/Host/Windows/EmbedNativeControlHost.cs` - uses shared placeholder-window helpers.
- `samples/avalonia-mdcad-control/Host/Windows/WindowsMdCadEmbedBackend.cs` - delegates start-info creation and child lookup to shared-core Windows helpers.
- `samples/avalonia-mdcad-control.tests/WindowsMdCadEmbedBackendTests.cs` - proves backend start-info generation matches the shared builder.
- `samples/avalonia-mdcad-control/Host/Windows/Win32NativeMethods.cs` - removed in favor of the shared-core Windows helper seam.

## Decisions Made
- Kept the builder API scalar-based so this plan could isolate the Windows launch seam without prematurely repointing the full Avalonia shell contract to shared-core types.
- Reused the existing placeholder/child-window helper pattern exactly, but moved it under shared core so the future WPF backend consumes the same HWND behavior.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 2 - Missing Critical] Repointed the native host surface to the shared Win32 helper seam**
- **Found during:** Task 1
- **Issue:** Moving only the backend would have left placeholder HWND creation/destruction in a duplicate Avalonia-only helper, violating the plan's single-source Win32 seam goal.
- **Fix:** Updated `EmbedNativeControlHost` to use shared `Win32NativeMethods` and removed the duplicate Avalonia helper file.
- **Files modified:** samples/avalonia-mdcad-control/Host/Windows/EmbedNativeControlHost.cs, samples/avalonia-mdcad-control/Host/Windows/Win32NativeMethods.cs
- **Verification:** `dotnet test .\samples\avalonia-mdcad-control.tests\MdCad.Avalonia.Control.Tests.csproj -c Release --filter "FullyQualifiedName~WindowsMdCadEmbedBackendTests"`
- **Committed in:** none (user requested no commits)

---

**Total deviations:** 1 auto-fixed (1 missing critical)
**Impact on plan:** Required to truly single-source the Windows helper seam; no broader scope change.

## Issues Encountered
- None

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness
- Ready to repoint the remaining Avalonia duplicated host contracts to shared-core types in plan 56-04.
- The Windows launch/HWND seam is now stable for upcoming WPF backend work.

## Self-Check: PASSED

- Verified summary-created files exist on disk, including the new shared Windows builder and helper files.
- Automated verification passed:
  - `dotnet test .\samples\mdcad-embed-core.tests\MdCad.Embed.Core.Tests.csproj -c Release --filter "FullyQualifiedName~MdCadWindowsStartInfoBuilderTests"`
  - `dotnet test .\samples\avalonia-mdcad-control.tests\MdCad.Avalonia.Control.Tests.csproj -c Release --filter "FullyQualifiedName~WindowsMdCadEmbedBackendTests"`
- Commit-hash verification was intentionally skipped because this execution honored the user's no-commit request.

---
*Phase: 56-create-wpf-mdcad-control-for-net10-0-windows-consumers-with-*
*Completed: 2026-05-28*
