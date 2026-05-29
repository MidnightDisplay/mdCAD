---
phase: 57-investigate-mdcad-embedded-crash-when-deleting-imported-json
plan: 04
subsystem: validation
tags: [validation, native, hosts, runtime-refresh, manual-proof]
requires:
  - phase: 57-investigate-mdcad-embedded-crash-when-deleting-imported-json
    provides: Native delete hardening and embedded crash diagnostics from 57-01
  - phase: 57-investigate-mdcad-embedded-crash-when-deleting-imported-json
    provides: Avalonia and WPF crash-detail surfacing from 57-02 and 57-03
provides:
  - Closed Phase 57 validation ledger with green automated and manual proof
  - Host-owned runtime refresh that re-stamps the full runtime bundle on every host build
  - Startup-controller dead-root cleanup preventing stale startup-root observer access after delete
affects: [phase-57]
tech-stack:
  added: []
  patterns: [host builds refresh the full runtime bundle, startup controller forgets deleted startup roots before overlay reads]
key-files:
  created:
    - .planning/phases/57-investigate-mdcad-embedded-crash-when-deleting-imported-json/57-04-SUMMARY.md
  modified:
    - .planning/phases/57-investigate-mdcad-embedded-crash-when-deleting-imported-json/57-VALIDATION.md
    - samples/avalonia-mdcad-control/tools/MdCad.WindowsRuntimeRefresh/RuntimeRefreshOrchestrator.cs
    - samples/avalonia-mdcad-control.tests/RuntimeRefreshHelperTests.cs
    - samples/avalonia-host/AvaloniaHost.csproj
    - samples/avalonia-host-minimal/AvaloniaHostMinimal.csproj
    - samples/wpf-host/WpfHost.csproj
    - samples/wpf-host-minimal/WpfHostMinimal.csproj
    - samples/avalonia-host.tests/MainWindowHarnessStatusTests.cs
    - samples/wpf-host.tests/MainWindowHarnessStatusTests.cs
    - src/startup_jsonl_import_controller.h
    - src/app.c
    - src/tests/startup_jsonl_delete_regression_test.c
key-decisions:
  - "Host builds now run the runtime refresh helper directly so manual launches cannot keep stale runtime-bundle files in output."
  - "The refresh helper now mirrors the full committed runtime bundle into host outputs instead of force-copying only mdCAD.exe."
  - "The startup JSONL controller must forget deleted startup roots before any overlay or frame logic queries observer state from them."
requirements-completed: [P57-01, P57-02, P57-03, P57-04, P57-05]
duration: continued-session
completed: 2026-05-29
---

# Phase 57 Plan 04: Validation and Manual Closure Summary

**Phase 57 is closed: the startup-import delete crash is gone in all six shipped host rows, embedded crash diagnostics remain truthful, and host builds now refresh a full fresh runtime bundle before manual proof.**

## Performance

- **Duration:** continued from the Phase 57 execution session
- **Completed:** 2026-05-29
- **Tasks:** 2
- **Files modified:** 12

## Accomplishments

- Reworked runtime refresh so every host build runs the helper itself and mirrors the full committed `mdcad-runtime` bundle into output directories.
- Added refresh assertions in the Avalonia and WPF host test lanes so host-build refresh cannot silently regress.
- Hardened the startup JSONL controller to forget deleted startup roots before the startup overlay queries observer state on later frames.
- Extended the native startup delete regression with a startup-root controller-cleanup case.
- Marked all six manual host rows green after rerunning the full Avalonia/WPF full/minimal startup-root delete matrix.

## Task Commits

User requested no additional commits during this closeout pass, so the final validation fixes remain uncommitted in the worktree.

## Files Created/Modified

- `samples/avalonia-mdcad-control/tools/MdCad.WindowsRuntimeRefresh/RuntimeRefreshOrchestrator.cs` - mirrors the full runtime bundle into host outputs.
- `samples/avalonia-mdcad-control.tests/RuntimeRefreshHelperTests.cs` - locks full-bundle refresh behavior.
- `samples/avalonia-host/AvaloniaHost.csproj` - refreshes the runtime bundle after every host build.
- `samples/avalonia-host-minimal/AvaloniaHostMinimal.csproj` - refreshes the runtime bundle after every host build.
- `samples/wpf-host/WpfHost.csproj` - refreshes the runtime bundle after every host build.
- `samples/wpf-host-minimal/WpfHostMinimal.csproj` - refreshes the runtime bundle after every host build.
- `samples/avalonia-host.tests/MainWindowHarnessStatusTests.cs` - verifies the host project owns the refresh target.
- `samples/wpf-host.tests/MainWindowHarnessStatusTests.cs` - verifies the host project owns the refresh target.
- `src/startup_jsonl_import_controller.h` - forgets deleted startup roots once the imported root is gone.
- `src/app.c` - syncs startup overlay reads with startup-root liveness.
- `src/tests/startup_jsonl_delete_regression_test.c` - covers controller cleanup after startup-root delete.
- `.planning/phases/57-investigate-mdcad-embedded-crash-when-deleting-imported-json/57-VALIDATION.md` - records green manual proof for the six-host matrix.

## Decisions Made

- Treated stale host runtime outputs as phase-owned blocking evidence, not as a reason to weaken the manual matrix.
- Fixed the remaining startup-only crash by narrowing the difference between startup-loaded roots and later imports, rather than broadening undo changes speculatively.

## Deviations from Plan

### Auto-fixed Issues

**1. [Blocking] Host rebuilds could leave stale runtime-bundle files in output**
- **Found during:** runtime-refresh integration follow-up after the first failed manual rerun
- **Issue:** `imgui.embedded.ini` and other bundle files could stay stale in host outputs even when `mdCAD.exe` refreshed, and incremental host builds could skip the referenced control-project refresh target entirely.
- **Fix:** moved runtime refresh ownership into each host project and changed the helper to mirror the full committed runtime bundle into the output runtime directory.

**2. [Blocking] Startup controller kept a dead startup-root entity after delete**
- **Found during:** startup-only crash differential analysis after the refreshed outputs still crashed
- **Issue:** the startup overlay could keep reading observer state through `controller->job.root_entity` after the startup-loaded root had been deleted.
- **Fix:** added `startup_jsonl_import_controller_forget_deleted_root(...)`, called it during controller ticks and overlay rendering, and locked the behavior with a native regression.

## Issues Encountered

- The first post-fix manual reruns were not trustworthy because stale host runtime outputs masked native changes.

## User Setup Required

- None.

## Next Phase Readiness

- Phase 57 execution and validation are fully complete.

