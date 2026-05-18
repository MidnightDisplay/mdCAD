---
phase: 55-viewport-only-startup-mode-and-f-camera-reset-hotkey
plan: 01
subsystem: avalonia-control-launch
tags: [avalonia, embed, launch, windows, viewport-only]

# Dependency graph
requires: []
provides:
  - Viewport-only startup contract on `MdCadEmbeddedControl`
  - Snapshot equality now includes viewport-only launch intent
  - Windows backend forwards `--viewport-only` in stable argument order
affects: [phase-55, phase-55-02]

# Tech tracking
tech-stack:
  added: []
  patterns: [styled-property launch contract, snapshot-equality relaunch trigger, ordered ProcessStartInfo arg forwarding]

key-files:
  created:
    - .planning/phases/55-viewport-only-startup-mode-and-f-camera-reset-hotkey/55-01-SUMMARY.md
  modified:
    - samples/avalonia-mdcad-control/MdCadEmbeddedControl.axaml.cs
    - samples/avalonia-mdcad-control/Host/MdCadLaunchSnapshot.cs
    - samples/avalonia-mdcad-control/Host/Windows/WindowsMdCadEmbedBackend.cs
    - samples/avalonia-mdcad-control.tests/MdCadLaunchSnapshotTests.cs
    - samples/avalonia-mdcad-control.tests/MdCadEmbeddedControlTests.cs
    - samples/avalonia-mdcad-control.tests/WindowsMdCadEmbedBackendTests.cs
    - samples/avalonia-mdcad-control.tests/MdCadSessionCoordinatorTests.cs
    - samples/avalonia-mdcad-control.tests/UnsupportedMdCadEmbedBackendTests.cs
    - .planning/phases/55-viewport-only-startup-mode-and-f-camera-reset-hotkey/55-VALIDATION.md

key-decisions:
  - "Viewport-only startup lives as a dedicated boolean StyledProperty and snapshot field instead of expanding `PresentationMode`."
  - "Viewport-only-only launch changes reuse the existing `MdCadSessionCoordinator` snapshot equality and relaunch path."
  - "Windows `CreateStartInfo(...)` forwards `--viewport-only` immediately after the required embedded HWND arguments and before optional JSONL/live-refresh args."

patterns-established:
  - "Pattern 1: Launch-affecting embed settings flow from StyledProperty -> `MdCadLaunchSnapshot` -> backend `ProcessStartInfo`."
  - "Pattern 2: New startup flags preserve existing required-arg ordering and optional JSONL/live-refresh suffix ordering."

requirements-completed: [P55-01]

# Metrics
duration: continued-session
completed: 2026-05-18
---

# Phase 55 Plan 01: Viewport-Only Launch Contract Summary

**Wave 1 is complete: the Avalonia control now captures viewport-only startup intent in its launch snapshot, and the Windows backend forwards that intent as `--viewport-only` without touching presentation mode or widening runtime scope.**

## Performance

- **Duration:** continued from the Phase 55 execution session
- **Completed:** 2026-05-18
- **Tasks:** 2
- **Files modified:** 9

## Accomplishments

- Added `ViewportOnlyStartupMode` as a dedicated launch-affecting `StyledProperty` on `MdCadEmbeddedControl`.
- Extended `MdCadLaunchSnapshot` so viewport-only intent participates in snapshot equality and coordinator relaunch decisions.
- Forwarded `--viewport-only` from `WindowsMdCadEmbedBackend.CreateStartInfo(...)` in stable argument order before optional JSONL/live-refresh flags.
- Locked the new contract with focused snapshot, control, backend, and coordinator tests.
- Marked validation rows `55-01-01` and `55-01-02` green.

## Task Commits

Each task was committed atomically:

1. **Task 1: Add the control/snapshot viewport-only launch contract** - `121a7fa`, `644a148` (test -> feat)
2. **Task 2: Forward viewport-only through the Windows backend and relaunch-proof tests** - `a2f693c`, `5e5b920` (test -> feat)

## Files Created/Modified

- `samples/avalonia-mdcad-control/MdCadEmbeddedControl.axaml.cs` - adds the public viewport-only launch property and captures it in the launch snapshot.
- `samples/avalonia-mdcad-control/Host/MdCadLaunchSnapshot.cs` - carries the viewport-only flag through the launch contract.
- `samples/avalonia-mdcad-control/Host/Windows/WindowsMdCadEmbedBackend.cs` - emits `--viewport-only` in the Windows child-process start arguments.
- `samples/avalonia-mdcad-control.tests/MdCadLaunchSnapshotTests.cs` - locks default/true viewport-only snapshot behavior.
- `samples/avalonia-mdcad-control.tests/MdCadEmbeddedControlTests.cs` - proves the control updates its captured launch snapshot when viewport-only changes.
- `samples/avalonia-mdcad-control.tests/WindowsMdCadEmbedBackendTests.cs` - locks exact `--viewport-only` argument ordering.
- `samples/avalonia-mdcad-control.tests/MdCadSessionCoordinatorTests.cs` - proves viewport-only-only snapshot changes still trigger the existing relaunch path.
- `samples/avalonia-mdcad-control.tests/UnsupportedMdCadEmbedBackendTests.cs` - keeps explicit snapshot construction aligned with the new launch contract.
- `.planning/phases/55-viewport-only-startup-mode-and-f-camera-reset-hotkey/55-VALIDATION.md` - rows `55-01-01` and `55-01-02` are green.

## Decisions Made

- Kept viewport-only startup out of `MdCadPresentationMode`; it remains a child-process launch concern, not host chrome.
- Reused record equality in `MdCadLaunchSnapshot` rather than adding a separate relaunch comparison path.
- Preserved required argument ordering and inserted `--viewport-only` only when the new snapshot flag is true.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Updated all explicit `MdCadLaunchSnapshot.Create(...)` call sites to the new signature**
- **Found during:** Task 1 (Add the control/snapshot viewport-only launch contract)
- **Issue:** Adding the viewport-only launch field required all explicit snapshot construction sites in the test project to pass the new boolean parameter or the suite would not compile.
- **Fix:** Updated the existing snapshot construction call sites, including the unsupported-backend and coordinator proof surfaces, to pass `viewportOnlyStartupMode: false` explicitly unless the test was proving the new behavior.
- **Files modified:** `samples/avalonia-mdcad-control.tests/MdCadSessionCoordinatorTests.cs`, `samples/avalonia-mdcad-control.tests/UnsupportedMdCadEmbedBackendTests.cs`, `samples/avalonia-mdcad-control.tests/WindowsMdCadEmbedBackendTests.cs`
- **Verification:** `dotnet test .\samples\avalonia-mdcad-control.tests\MdCad.Avalonia.Control.Tests.csproj -c Release --filter "FullyQualifiedName~MdCadLaunchSnapshotTests" && dotnet test .\samples\avalonia-mdcad-control.tests\MdCad.Avalonia.Control.Tests.csproj -c Release --filter "FullyQualifiedName~MdCadEmbeddedControlTests"`
- **Committed in:** `644a148` (part of Task 1 commit)

---

**Total deviations:** 1 auto-fixed (1 blocking)
**Impact on plan:** The signature-alignment fix was required to keep the existing proof surfaces compiling. No scope creep.

## Issues Encountered

- None.

## User Setup Required

- None.

## Next Phase Readiness

- `55-02` can now add native parser/layout policy and the viewport-only app contract against a stable control/backend launch surface.

---
*Phase: 55-viewport-only-startup-mode-and-f-camera-reset-hotkey*
*Completed: 2026-05-18*
