---
phase: 48-reusable-avalonia-mdcad-user-control
plan: 03
subsystem: ui
tags: [avalonia, tdd, coordinator, jsonl, embedding]

# Dependency graph
requires:
  - phase: 48-reusable-avalonia-mdcad-user-control
    provides: Control-library foundation, copied runtime resolution, and the committed `mdcad-runtime` bundle from Plans 01-02.
provides:
  - Managed test seam for launch snapshot semantics and lifecycle coordination
  - `MdCadLaunchSnapshot` JSONL warning/argument rules with automated coverage
  - Serialized `MdCadSessionCoordinator` and control-owned start/stop/relaunch wiring
affects: [phase-48-presentation, avalonia-host, embedded-lifecycle]

# Tech tracking
tech-stack:
  added: [xUnit-managed control seam, InternalsVisibleTo test access for the control library]
  patterns: [single-gate lifecycle coordination, snapshot-based launch argument normalization]

key-files:
  created:
    - samples/avalonia-mdcad-control.tests/MdCad.Avalonia.Control.Tests.csproj
    - samples/avalonia-mdcad-control.tests/NuGet.Config
    - samples/avalonia-mdcad-control.tests/MdCadLaunchSnapshotTests.cs
    - samples/avalonia-mdcad-control.tests/MdCadSessionCoordinatorTests.cs
    - samples/avalonia-mdcad-control/Host/MdCadLaunchSnapshot.cs
    - samples/avalonia-mdcad-control/Host/MdCadSessionCoordinator.cs
    - samples/avalonia-mdcad-control/Properties/AssemblyInfo.cs
  modified:
    - samples/avalonia-mdcad-control/MdCadEmbeddedControl.axaml.cs
    - samples/avalonia-mdcad-control/Host/EmbedNativeControlHost.cs

key-decisions:
  - "Keep the launch snapshot and warning model internal to the control library and expose it to tests through InternalsVisibleTo rather than widening the public control API."
  - "Serialize start/stop/relaunch work through a single session coordinator so launch-affecting property changes never launch directly from setters."
  - "Launch the embedded process from the copied runtime resolver and keep WorkingDirectory rooted at the runtime bundle so embedded layout persistence stays stable."

patterns-established:
  - "Pattern 1: Bindable control properties feed an internal launch snapshot helper; the control does not normalize launch arguments ad hoc across setters or UI callbacks."
  - "Pattern 2: Rapid launch-affecting changes are tested against a fake session host and reconciled through one queued coordinator instead of independent timers or setters."

requirements-completed: [P48-03, P48-04, P48-05]

# Metrics
duration: 12 min
completed: 2026-05-15
---

# Phase 48 Plan 03: Lifecycle coordinator summary

**Managed launch snapshot tests and a serialized `MdCadSessionCoordinator` now drive auto-start, explicit start/stop, and coalesced relaunch from the reusable control**

## Performance

- **Duration:** 12 min
- **Started:** 2026-05-15T15:43:23+01:00
- **Completed:** 2026-05-15T15:55:22.4020837+01:00
- **Tasks:** 2
- **Files modified:** 9

## Accomplishments
- Added `samples/avalonia-mdcad-control.tests` as a Windows-targeted managed test project with focused launch-snapshot and session-coordinator coverage.
- Implemented `MdCadLaunchSnapshot` so unset, invalid-relative, and absolute-missing JSONL requests all produce one canonical launch/warning decision path.
- Implemented `MdCadSessionCoordinator` with a single lifecycle gate and queued reconcile flow for auto-start, explicit start/stop, and coalesced launch-affecting relaunch requests.
- Wired `MdCadEmbeddedControl` into the coordinator so the reusable control now owns mdCAD process start, graceful stop, placeholder invalidation/recreation, attach polling, and resize sync against the copied runtime bundle.

## Task Commits

Each task was committed atomically:

1. **Task 1: Add the managed test seam and lock launch-snapshot semantics before control wiring** - `40bd927` (test), `a968880` (feat)
2. **Task 2: Implement the serialized relaunch coordinator and wire the control to it** - `45fc821` (test), `c2dada3` (feat)

**Plan metadata:** pending final docs commit

## Files Created/Modified
- `samples/avalonia-mdcad-control.tests/MdCad.Avalonia.Control.Tests.csproj` - Windows-targeted managed test project for the reusable control seam.
- `samples/avalonia-mdcad-control.tests/NuGet.Config` - Restricts the new test project restore to `nuget.org` so it remains runnable on this machine.
- `samples/avalonia-mdcad-control.tests/MdCadLaunchSnapshotTests.cs` - Locks empty, relative, and absolute-missing JSONL launch semantics.
- `samples/avalonia-mdcad-control.tests/MdCadSessionCoordinatorTests.cs` - Locks auto-start, explicit start, coalesced relaunch, and stop/recreate lifecycle behavior against a fake host.
- `samples/avalonia-mdcad-control/Host/MdCadLaunchSnapshot.cs` - Centralizes JSONL launch argument and warning decisions.
- `samples/avalonia-mdcad-control/Host/MdCadSessionCoordinator.cs` - Serializes lifecycle reconciliation through a single gate and queued generations.
- `samples/avalonia-mdcad-control/Properties/AssemblyInfo.cs` - Exposes control-library internals to the managed test assembly only.
- `samples/avalonia-mdcad-control/MdCadEmbeddedControl.axaml.cs` - Wires public properties and explicit start/stop into the session coordinator and actual process lifecycle.
- `samples/avalonia-mdcad-control/Host/EmbedNativeControlHost.cs` - Expands the internal Win32 helper with child-enumeration, parent-check, and move APIs the control lifecycle now needs.

## Decisions Made
- Kept the launch-snapshot warning model internal to the reusable control instead of inventing public readiness/import events.
- Reused the proven placeholder invalidation and graceful wait/fallback cleanup pattern from the sample host rather than creating a second stop/relaunch path.
- Treated rapid relaunch coalescing as a queued lifecycle concern owned by the coordinator, not as special-case timing inside Avalonia property setters.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Added a project-local NuGet config for the new managed test project**
- **Found during:** Task 1 (Add the managed test seam and lock launch-snapshot semantics before control wiring)
- **Issue:** The new `.tests` project would have hit the same machine-level ProGet `403 Forbidden` restore failure as the standalone control library.
- **Fix:** Added `samples/avalonia-mdcad-control.tests/NuGet.Config` to scope restore to `nuget.org`.
- **Files modified:** `samples/avalonia-mdcad-control.tests/NuGet.Config`
- **Verification:** `dotnet test samples/avalonia-mdcad-control.tests/MdCad.Avalonia.Control.Tests.csproj -c Release --filter MdCadLaunchSnapshotTests`
- **Committed in:** `40bd927` (part of Task 1 commit)

**2. [Rule 1 - Bug] Tightened the relaunch test to model an in-flight rapid-change overlap instead of two fully serialized reconciles**
- **Found during:** Task 2 (Implement the serialized relaunch coordinator and wire the control to it)
- **Issue:** The first coordinator relaunch test let both fake reconciles complete synchronously, which incorrectly demanded coalescing across two fully completed sequential requests.
- **Fix:** Added a stop gate to the fake session host so the second request arrives while the first relaunch is still in flight, matching the queued-change race the coordinator is designed to collapse.
- **Files modified:** `samples/avalonia-mdcad-control.tests/MdCadSessionCoordinatorTests.cs`
- **Verification:** `dotnet test samples/avalonia-mdcad-control.tests/MdCad.Avalonia.Control.Tests.csproj -c Release`
- **Committed in:** `c2dada3` (part of Task 2 commit)

---

**Total deviations:** 2 auto-fixed (1 blocking, 1 bug)
**Impact on plan:** Both fixes tightened the intended TDD path and kept the lifecycle seam verifiable without expanding scope.

## Issues Encountered
- The new managed test project needed the same repo-local NuGet-source scoping as the standalone control library to avoid a machine-level feed returning `403 Forbidden`.
- The first coordinator overlap test accidentally modeled two fully serialized relaunches; tightening it around an in-flight stop window restored the intended coalescing scenario.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness
- Ready for `48-04-PLAN.md` to layer sealed vs diagnostic presentation on top of the working control lifecycle and convert the sample host into a real control consumer.
- The reusable control now owns the core launch/relaunch behavior, so the final wave can focus on presentation, harness conversion, and manual/regression proof instead of foundational lifecycle plumbing.

---
*Phase: 48-reusable-avalonia-mdcad-user-control*
*Completed: 2026-05-15*
