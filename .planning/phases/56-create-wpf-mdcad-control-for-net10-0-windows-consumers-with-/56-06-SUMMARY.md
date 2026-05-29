---
phase: 56-create-wpf-mdcad-control-for-net10-0-windows-consumers-with-
plan: 06
subsystem: ui
tags: [wpf, hwndhost, win32, xunit, net10-windows]
requires:
  - phase: 56-03
    provides: Shared Windows start-info builder and embed-core lifecycle primitives
  - phase: 56-05
    provides: WPF control scaffold and STA-backed parity lane
provides:
  - WPF mdCAD control shell wired through the shared session coordinator into a Windows backend
  - Real HwndHost placeholder seam for external-process child-HWND embedding
  - Canonical runtime-copy wiring that reuses the Avalonia-managed runtime bundle and refresh helper
affects: [phase-56-plan-07, phase-56-plan-08, phase-56-plan-09]
tech-stack:
  added: []
  patterns: [WPF HwndHost child-HWND hosting, shared embed-core session coordination, single-sourced runtime payload linking]
key-files:
  created:
    - samples/wpf-mdcad-control/Host/IMdCadEmbedBackend.cs
    - samples/wpf-mdcad-control/Host/MdCadEmbedBackendFactory.cs
    - samples/wpf-mdcad-control/Host/Windows/MdCadHwndHost.cs
    - samples/wpf-mdcad-control/Host/Windows/WindowsMdCadEmbedBackend.cs
    - samples/wpf-mdcad-control/Properties/AssemblyInfo.cs
  modified:
    - samples/wpf-mdcad-control/MdCad.Wpf.Control.csproj
    - samples/wpf-mdcad-control/MdCadEmbeddedControl.xaml
    - samples/wpf-mdcad-control/MdCadEmbeddedControl.xaml.cs
    - samples/wpf-mdcad-control.tests/MdCadEmbeddedControlTests.cs
    - samples/wpf-mdcad-control.tests/WindowsMdCadEmbedBackendTests.cs
    - samples/wpf-mdcad-control.tests/WpfStaThreadFixture.cs
key-decisions:
  - Keep the WPF embed seam on a real HwndHost placeholder and reuse the shared Win32 start-info builder instead of adding a WPF-specific launch path.
  - Copy the runtime payload from samples/avalonia-mdcad-control/runtime/win-x64 so WPF does not create a second runtime source or helper toolchain.
patterns-established:
  - "WPF host shells in mdCAD should flow through MdCadSessionCoordinator plus a backend factory instead of embedding lifecycle logic directly in the control."
  - "WPF async tests need a dispatcher-backed STA fixture so continuations stay on the UI thread."
requirements-completed: [P56-01, P56-02]
duration: 53 min
completed: 2026-05-28
---

# Phase 56 Plan 06: Implement the WPF backend and single-source runtime packaging Summary

**The WPF control now launches mdCAD through a real `HwndHost`/external-process child-HWND backend, while reusing the shared Windows launch builder and the canonical Avalonia-managed runtime bundle.**

## Performance

- **Duration:** 53 min
- **Tasks:** 1
- **Files modified:** 12

## Accomplishments
- Added the WPF backend seam (`IMdCadEmbedBackend`, factory, `MdCadHwndHost`, and `WindowsMdCadEmbedBackend`) and kept it on the existing external child-HWND model.
- Rewired `MdCadEmbeddedControl` so dependency-property changes, `StartAsync()`, and `StopAsync()` flow through `MdCadSessionCoordinator` into the backend.
- Reworked the WPF project file to link `samples/avalonia-mdcad-control/runtime/win-x64/**` into `mdcad-runtime/**` and reuse the existing Windows runtime refresh helper project.
- Extended the WPF tests to cover coordinator/backend wiring, `HwndHost` usage, shared start-info parity, and canonical runtime-copy wiring.

## Task Commits

User requested no commits for this execution, so no task or metadata commits were created.

## Files Created/Modified
- `samples/wpf-mdcad-control/MdCad.Wpf.Control.csproj` - links the canonical runtime bundle and optional refresh helper into WPF output.
- `samples/wpf-mdcad-control/MdCadEmbeddedControl.xaml` - hosts the native surface with warning/diagnostic chrome outside the HWND region.
- `samples/wpf-mdcad-control/MdCadEmbeddedControl.xaml.cs` - wires dependency properties, coordinator lifecycle, backend events, and control diagnostics together.
- `samples/wpf-mdcad-control/Host/IMdCadEmbedBackend.cs` - WPF backend contract for hosted-surface lifecycle management.
- `samples/wpf-mdcad-control/Host/MdCadEmbedBackendFactory.cs` - backend selection seam with an unsupported-runtime fallback.
- `samples/wpf-mdcad-control/Host/Windows/MdCadHwndHost.cs` - real WPF `HwndHost` placeholder window implementation.
- `samples/wpf-mdcad-control/Host/Windows/WindowsMdCadEmbedBackend.cs` - Windows external-process attach/resize/teardown backend that reuses `MdCadWindowsStartInfoBuilder`.
- `samples/wpf-mdcad-control/Properties/AssemblyInfo.cs` - exposes internals to the WPF test assembly.
- `samples/wpf-mdcad-control.tests/MdCadEmbeddedControlTests.cs` - coordinator/control-shell wiring coverage.
- `samples/wpf-mdcad-control.tests/WindowsMdCadEmbedBackendTests.cs` - backend/runtime-copy contract coverage.
- `samples/wpf-mdcad-control.tests/WpfStaThreadFixture.cs` - dispatcher-backed STA test harness for WPF async flows.

## Decisions Made
- Reused `MdCadWindowsStartInfoBuilder` so WPF and Avalonia preserve identical working-directory and argument ordering rules.
- Kept warning and diagnostic UI outside the hosted window by using a separate post-host panel in the WPF shell.
- Used the Avalonia-owned runtime folder/helper as the only runtime source, avoiding duplicated packaging logic.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking Issue] Upgraded the STA fixture to run a real dispatcher loop**
- **Found during:** Task 1 verification
- **Issue:** Async WPF test continuations left the UI thread, causing cross-thread dependency-property access failures unrelated to the backend logic itself.
- **Fix:** Reworked `WpfStaThreadFixture` to install `DispatcherSynchronizationContext`, run the dispatcher loop, and shut it down after each test action completes.
- **Files modified:** `samples/wpf-mdcad-control.tests/WpfStaThreadFixture.cs`

**2. [Rule 3 - Blocking Issue] Added assembly-level test visibility for new backend seams**
- **Found during:** Task 1 implementation
- **Issue:** The new internal backend/factory/testing hooks needed controlled access from the WPF test project.
- **Fix:** Added `InternalsVisibleTo("MdCad.Wpf.Control.Tests")` in `samples/wpf-mdcad-control/Properties/AssemblyInfo.cs`.
- **Files modified:** `samples/wpf-mdcad-control/Properties/AssemblyInfo.cs`

## Known Stubs

None.

## Threat Flags

None.

## Issues Encountered
- None after the dispatcher-backed STA fix.

## User Setup Required

None - verification stayed local to the repo.

## Next Phase Readiness
- Ready for downstream WPF consumer plans to build on the now-wired backend/control seam.
- Runtime payload and refresh tooling are now proven to stay single-sourced for WPF output.

## Self-Check: PASSED

- Verified `samples/wpf-mdcad-control/Host/Windows/MdCadHwndHost.cs` exists.
- Verified `samples/wpf-mdcad-control/Host/Windows/WindowsMdCadEmbedBackend.cs` exists.
- Verified `samples/wpf-mdcad-control/MdCadEmbeddedControl.xaml.cs` exists.
- Automated verification passed: `dotnet test .\samples\wpf-mdcad-control.tests\MdCad.Wpf.Control.Tests.csproj -c Release --filter "FullyQualifiedName~MdCadEmbeddedControlTests|FullyQualifiedName~WindowsMdCadEmbedBackendTests"`
- Automated verification passed: `dotnet build .\samples\wpf-mdcad-control\MdCad.Wpf.Control.csproj -c Release`
- Commit-hash verification was intentionally skipped because this execution honored the user's no-commit request.

---
*Phase: 56-create-wpf-mdcad-control-for-net10-0-windows-consumers-with-*
*Completed: 2026-05-28*
