---
phase: 56-create-wpf-mdcad-control-for-net10-0-windows-consumers-with-
plan: 01
subsystem: ui
tags: [wpf, avalonia, shared-core, xunit, net10]
requires:
  - phase: 55-viewport-only-startup-mode-and-f-camera-reset-hotkey
    provides: Avalonia-proven launch snapshot semantics including viewport-only startup behavior
provides:
  - Framework-neutral launch snapshot contract shared by future Avalonia and WPF host surfaces
  - Shared presentation enum and unsupported-runtime truth constants
  - Independent xUnit lane for snapshot parity and warning semantics
affects: [phase-56-plan-02, phase-56-plan-04, phase-56-plan-05]
tech-stack:
  added: [MdCad.Embed.Core]
  patterns: [shared non-visual embed contracts before UI-framework-specific shells]
key-files:
  created:
    - samples/mdcad-embed-core/MdCad.Embed.Core.csproj
    - samples/mdcad-embed-core/MdCadLaunchSnapshot.cs
    - samples/mdcad-embed-core/MdCadPresentationMode.cs
    - samples/mdcad-embed-core/MdCadUnsupportedRuntime.cs
    - samples/mdcad-embed-core.tests/MdCad.Embed.Core.Tests.csproj
    - samples/mdcad-embed-core.tests/MdCadLaunchSnapshotTests.cs
    - samples/mdcad-embed-core.tests/MdCadUnsupportedRuntimeTests.cs
  modified:
    - .planning/phases/56-create-wpf-mdcad-control-for-net10-0-windows-consumers-with-/56-01-SUMMARY.md
key-decisions:
  - Shared launch snapshot, presentation mode, and unsupported-runtime messaging now live in a framework-neutral core assembly before WPF UI work begins.
  - Plan 56-01 stays non-visual; session coordination, runtime resolution, and Windows backend seams remain for later plans.
patterns-established:
  - "Shared embed-core types keep parity-critical launch semantics out of UI-specific projects."
  - "Warning-text and launch-argument eligibility are locked by standalone xUnit tests before consumer repointing."
requirements-completed: [P56-01]
duration: 15 min
completed: 2026-05-28
---

# Phase 56 Plan 01: Create the first shared embed-core slice for launch snapshot and presentation/support truth Summary

**Framework-neutral mdCAD launch snapshot, presentation mode, and unsupported-runtime truth now ship in a dedicated shared core with its own green xUnit parity lane.**

## Performance

- **Duration:** 15 min
- **Started:** 2026-05-28T22:45:27Z
- **Completed:** 2026-05-28T23:00:27Z
- **Tasks:** 1
- **Files modified:** 11

## Accomplishments
- Added a new `MdCad.Embed.Core` class library for parity-critical embed contracts.
- Ported launch snapshot tests into a framework-neutral xUnit project and preserved viewport/live-refresh/warning semantics.
- Locked shared presentation-mode names and canonical unsupported-runtime messaging for later Avalonia and WPF reuse.

## Task Commits

User requested no commits for this execution, so no task or metadata commits were created.

## Files Created/Modified
- `samples/mdcad-embed-core/MdCad.Embed.Core.csproj` - Shared `net10.0` contract project for non-visual embed types.
- `samples/mdcad-embed-core/MdCadLaunchSnapshot.cs` - Public launch snapshot record with absolute-path validation and warning semantics.
- `samples/mdcad-embed-core/MdCadPresentationMode.cs` - Public sealed/diagnostic enum shared across UI stacks.
- `samples/mdcad-embed-core/MdCadUnsupportedRuntime.cs` - Canonical Windows-only runtime support message.
- `samples/mdcad-embed-core.tests/MdCad.Embed.Core.Tests.csproj` - Standalone xUnit lane for shared-core verification.
- `samples/mdcad-embed-core.tests/MdCadLaunchSnapshotTests.cs` - Snapshot parity tests covering JSONL, live refresh, viewport-only, and warning behavior.
- `samples/mdcad-embed-core.tests/MdCadUnsupportedRuntimeTests.cs` - Shared presentation/runtime-support truth tests.

## Decisions Made
- Shared the parity-critical launch and runtime-support contracts first so later WPF work can consume them without copying Avalonia internals.
- Kept the new project strictly non-visual to avoid prematurely moving coordinator, backend, or runtime-resolution code ahead of later plans.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered
- None

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness
- Ready to extract the next shared-core slice for runtime resolution and session coordination onto the new `MdCad.Embed.Core` foundation.
- No known blockers from this plan.

## Self-Check: PASSED

- Verified all plan-created files exist on disk.
- Automated verification passed: `dotnet test .\samples\mdcad-embed-core.tests\MdCad.Embed.Core.Tests.csproj -c Release`
- Commit-hash verification was intentionally skipped because this execution honored the user's no-commit request.

---
*Phase: 56-create-wpf-mdcad-control-for-net10-0-windows-consumers-with-*
*Completed: 2026-05-28*
