---
phase: 56-create-wpf-mdcad-control-for-net10-0-windows-consumers-with-
plan: 07
subsystem: ui
tags: [wpf, net10-windows, diagnostic-host, proof-jsonl]
requires:
  - phase: 56-03
    provides: Shared Windows start-info builder and embed-core lifecycle/status primitives
  - phase: 56-06
    provides: Reusable WPF mdCAD control plus canonical runtime-copy wiring
provides:
  - Full WPF diagnostic host harness with host-owned lifecycle controls and truthful status text
  - Bundled proof JSONL copied into WPF host output for deployment-rooted scenario selection
affects: [phase-56-plan-08, phase-56-plan-09]
tech-stack:
  added: []
  patterns: [WPF sample host consuming reusable mdCAD control by project reference, deployment-rooted proof asset linking, host-owned lifecycle/status messaging]
key-files:
  created:
    - samples/wpf-host/WpfHost.csproj
    - samples/wpf-host/App.xaml
    - samples/wpf-host/App.xaml.cs
    - samples/wpf-host/MainWindow.xaml
    - samples/wpf-host/MainWindow.xaml.cs
  modified: []
key-decisions:
  - Keep the WPF host as a thin consumer of MdCad.Wpf.Control and mirror the Avalonia harness semantics instead of inventing a separate lifecycle model.
  - Link the committed proof JSONL asset into the app output so scenario selection stays deployment-rooted instead of repo-rooted.
  - Keep runtime and harness status text explicitly host-owned so the sample never claims viewer-side IPC insight it cannot prove.
patterns-established:
  - "WPF proof hosts should surface all diagnostics outside the hosted HWND region to stay honest about airspace limits."
  - "Sample-host status lines should report only host-known state such as copied runtime presence, requested launch options, and requested control actions."
requirements-completed: [P56-03]
duration: 14 min
completed: 2026-05-28
---

# Phase 56 Plan 07: Add the full diagnostic WPF proof host Summary

**A dedicated `samples/wpf-host` app now consumes the reusable WPF mdCAD control with host-owned lifecycle controls, truthful runtime/status messaging, and a bundled proof JSONL asset copied into deployment output.**

## Performance

- **Duration:** 14 min
- **Tasks:** 1
- **Files modified:** 5

## Accomplishments
- Created a standalone `samples/wpf-host` WPF app that references `MdCad.Wpf.Control`.
- Added operator controls for presentation mode, auto-start, live refresh, viewport-only startup, JSONL scenario selection, and explicit `StartAsync()` / `StopAsync()` requests.
- Wired runtime and harness status text to host-owned facts and copied the proof JSONL asset into app output.

## Task Commits

User requested no commits for this execution, so no task or metadata commits were created.

## Files Created/Modified
- `samples/wpf-host/WpfHost.csproj` - defines the WPF sample app, references the reusable control, and links the proof JSONL asset into output.
- `samples/wpf-host/App.xaml` - bootstraps the WPF application shell.
- `samples/wpf-host/App.xaml.cs` - starts the main window with parsed launch options.
- `samples/wpf-host/MainWindow.xaml` - lays out the host controls and keeps chrome outside the hosted native viewer region.
- `samples/wpf-host/MainWindow.xaml.cs` - applies host launch settings, manages truthful status text, resolves bundled JSONL scenarios, and forwards explicit lifecycle requests to the control.

## Decisions Made
- Reused the WPF control by project reference so the proof host exercises the same consumer path downstream apps will use.
- Resolved proof-input scenarios from `AppContext.BaseDirectory/resources/examples` so the host stays deployment-rooted.
- Reported runtime presence from `AppContext.BaseDirectory/mdcad-runtime` and kept harness text limited to host-owned knowledge.

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

- Ready for downstream consumer and verification work to launch the new WPF proof host against the reusable control.
- Bundled proof input and truthful status surfaces are now in place for manual and automated host validation.

## Self-Check: PASSED

- Verified `samples/wpf-host/WpfHost.csproj` exists.
- Verified `samples/wpf-host/MainWindow.xaml` exists.
- Verified `samples/wpf-host/MainWindow.xaml.cs` exists.
- Automated verification passed: `dotnet build .\samples\wpf-host\WpfHost.csproj -c Release`
- Output verification passed: bundled proof JSONL exists at `samples\wpf-host\bin\Release\net10.0-windows10.0.19041.0\resources\examples\sample-host-proof.jsonl`
- Output verification passed: runtime bundle exists at `samples\wpf-host\bin\Release\net10.0-windows10.0.19041.0\mdcad-runtime\mdCAD.exe`
- Commit-hash verification was intentionally skipped because this execution honored the user's no-commit request.

---
*Phase: 56-create-wpf-mdcad-control-for-net10-0-windows-consumers-with-*
*Completed: 2026-05-28*
