---
phase: 57-investigate-mdcad-embedded-crash-when-deleting-imported-json
plan: 03
subsystem: ui
tags: [wpf, embedded, diagnostics, host, testing]
requires:
  - phase: 56-create-wpf-mdcad-control-for-net10-0-windows-consumers-with-
    provides: WPF embedded control, backend seam, and diagnostic/full-host scaffolding
  - phase: 57-investigate-mdcad-embedded-crash-when-deleting-imported-json
    provides: native stderr/crash-log diagnostics contract from 57-01
provides:
  - WPF backend reuse of native crash-log stderr summaries after attach loss
  - control-owned sticky warning relay for unexpected embedded session loss
  - full-host harness status mirroring of the exact observed crash detail
affects: [phase-57-plan-04]
tech-stack:
  added: []
  patterns: [sticky control-owned warning relay, host harness mirrors observed control warning text]
key-files:
  created:
    - samples/wpf-mdcad-control/Host/Windows/WindowsMdCadEmbedBackend.cs
    - samples/wpf-mdcad-control/MdCadEmbeddedControl.xaml.cs
    - samples/wpf-mdcad-control.tests/WindowsMdCadEmbedBackendTests.cs
    - samples/wpf-mdcad-control.tests/MdCadEmbeddedControlTests.cs
    - samples/wpf-host/MainWindow.xaml.cs
    - samples/wpf-host.tests/WpfHost.Tests.csproj
    - samples/wpf-host.tests/MainWindowHarnessStatusTests.cs
  modified:
    - .planning/phases/57-investigate-mdcad-embedded-crash-when-deleting-imported-json/57-VALIDATION.md
key-decisions:
  - "The WPF backend now reuses the native stderr/log summary line when available instead of inventing a crash cause from host state."
  - "MdCadEmbeddedControl keeps SetLaunchWarning as the single warning source of truth, while the full host only mirrors the already-observed detail."
patterns-established:
  - "Pattern 1: embedded host warnings stay control-owned and survive stop/reconcile flows via a sticky warning seam."
  - "Pattern 2: diagnostic hosts append mirrored crash detail to host-owned status text without recomputing a second message."
requirements-completed: [P57-04, P57-05]
duration: 52 min
completed: 2026-05-29
---

# Phase 57 Plan 03: Surface native crash/log detail through the WPF backend, control, and full host Summary

**WPF embedded hosts now preserve native crash-log stderr summaries through the backend, keep the sealed control warning as the source of truth, and mirror that exact detail in the diagnostic host harness.**

## Performance

- **Duration:** 52 min
- **Started:** 2026-05-29T11:21:46Z
- **Completed:** 2026-05-29T12:13:35Z
- **Tasks:** 2
- **Files modified:** 20

## Accomplishments
- Extended the WPF Windows backend to capture embedded startup/runtime failure summaries and prefer observed native stderr detail over inferred host-side wording.
- Added a public WPF control relay seam while keeping `SetLaunchWarning(...)` as the sticky control-owned warning source after unexpected session loss.
- Updated the full WPF host plus a dedicated `samples/wpf-host.tests` lane so harness status mirrors the exact observed crash/log detail and Release verification stays green.

## Task Commits

Each task was committed atomically:

1. **Task 1: Capture truthful native failure detail in the WPF backend** - `295669c`, `d23541e` (test, feat)
2. **Task 2: Relay the same detail through the WPF control and full host** - `92027eb`, `5b3c09b` (test, feat)
3. **Support tracking:** `ecebe9b` (chore) - tracked the plan-scoped WPF XAML/csproj/interface assets required to make the committed WPF work reproducible from git history.

_Note: TDD tasks used separate RED and GREEN commits._

## Files Created/Modified
- `samples/wpf-mdcad-control/Host/Windows/WindowsMdCadEmbedBackend.cs` - Captures relevant embedded stderr summaries and reuses them for unexpected-session-loss detail composition.
- `samples/wpf-mdcad-control.tests/WindowsMdCadEmbedBackendTests.cs` - Locks startup-summary capture, runtime-summary preference, and truthful fallback wording.
- `samples/wpf-mdcad-control/MdCadEmbeddedControl.xaml.cs` - Adds the host-facing relay seam and sticky warning preservation through stop/reconcile flows.
- `samples/wpf-mdcad-control.tests/MdCadEmbeddedControlTests.cs` - Verifies the control keeps the warning surface source-of-truth while relaying the same observed detail.
- `samples/wpf-host/MainWindow.xaml.cs` - Mirrors observed unexpected-session-loss detail inside `BuildHarnessStatus()` without overriding control-owned warning text.
- `samples/wpf-host.tests/WpfHost.Tests.csproj` - Adds the WPF host proof lane for harness-status mirroring.
- `samples/wpf-host.tests/MainWindowHarnessStatusTests.cs` - Verifies the diagnostic host echoes the exact crash/log detail verbatim.
- `.planning/phases/57-investigate-mdcad-embedded-crash-when-deleting-imported-json/57-VALIDATION.md` - Marks WPF Phase 57 plan-03 verification rows green.

## Decisions Made
- Used the native summary/log-path line as the preferred WPF backend detail source so WPF stays aligned with the Phase 57 native diagnostics contract.
- Preserved control-owned warning truth by routing coordinator warning updates through a sticky warning helper instead of letting stop/reconcile clear the observed crash detail.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking issue] Tracked required WPF support files that were still unversioned**
- **Found during:** Final untracked-file audit
- **Issue:** The plan-scoped WPF source/test commits built successfully only because required project/XAML/interface files were still present as untracked worktree files.
- **Fix:** Committed the WPF host/control/test scaffolding used by verification so the branch remains reproducible outside the dirty local worktree.
- **Files modified:** `samples/wpf-host/App.xaml`, `samples/wpf-host/App.xaml.cs`, `samples/wpf-host/MainWindow.xaml`, `samples/wpf-host/WpfHost.csproj`, `samples/wpf-mdcad-control.tests/MdCad.Wpf.Control.Tests.csproj`, `samples/wpf-mdcad-control.tests/WpfStaThreadFixture.cs`, `samples/wpf-mdcad-control/Host/IMdCadEmbedBackend.cs`, `samples/wpf-mdcad-control/Host/MdCadEmbedBackendFactory.cs`, `samples/wpf-mdcad-control/Host/Windows/MdCadHwndHost.cs`, `samples/wpf-mdcad-control/MdCad.Wpf.Control.csproj`, `samples/wpf-mdcad-control/MdCadEmbeddedControl.xaml`, `samples/wpf-mdcad-control/Properties/AssemblyInfo.cs`
- **Commit:** `ecebe9b`

## Issues Encountered
- `MdCadSessionCoordinator` reapplied snapshot warnings during stop/reconcile, which initially cleared the observed unexpected-session-loss warning; the control now routes coordinator warning updates through a sticky warning helper so the same control-owned detail remains visible and relayable.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness
- WPF backend, control, host, and host-test diagnostics propagation are green in Release configuration.
- Phase 57-04 can now rebaseline the validation ledger and run the manual WPF/Avalonia delete-smoke matrix against a shared native diagnostics contract.

## Self-Check: PASSED
- Verified summary and all WPF plan-03 source/test artifacts exist on disk.
- Verified task/support commits `295669c`, `d23541e`, `92027eb`, `5b3c09b`, and `ecebe9b` exist in git history.
