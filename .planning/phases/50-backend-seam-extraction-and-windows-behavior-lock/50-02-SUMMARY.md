---
phase: 50-backend-seam-extraction-and-windows-behavior-lock
plan: 02
subsystem: ui
tags: [avalonia, windows, backend, win32, lifecycle]

# Dependency graph
requires:
  - phase: 50-backend-seam-extraction-and-windows-behavior-lock
    provides: Wave 1 behavior-lock tests, internal seam contract, and the manual Windows checklist baseline
provides:
  - Windows backend ownership for placeholder, launch, attach, resize, and cleanup lifecycle
  - Thin shared shell composed over `IMdCadEmbedBackend`
  - Coordinator orchestration routed through backend delegates instead of shell-owned Win32 state
affects: [phase-50-03, phase-51, phase-52]

# Tech tracking
tech-stack:
  added: []
  patterns: [interface-backed backend composition, coordinator-to-backend lifecycle delegation, host file relocation without namespace breakage]

key-files:
  created:
    - .planning/phases/50-backend-seam-extraction-and-windows-behavior-lock/50-02-SUMMARY.md
  modified:
    - samples/avalonia-mdcad-control/MdCadEmbeddedControl.axaml.cs
    - samples/avalonia-mdcad-control/Host/MdCadSessionCoordinator.cs
    - samples/avalonia-mdcad-control/Host/Windows/EmbedNativeControlHost.cs
    - samples/avalonia-mdcad-control/Host/Windows/WindowsMdCadEmbedBackend.cs
    - samples/avalonia-mdcad-control.tests/MdCadSessionCoordinatorTests.cs

key-decisions:
  - "Changed `MdCadSessionCoordinator` to call backend-owned start/stop/recreate delegates directly while preserving the existing generation gate and stop-recreate-restart order."
  - "Moved `EmbedNativeControlHost` into `Host/Windows/` but kept the existing `MdCad.Avalonia.Control.Host` namespace so the extraction stays behavior-preserving."
  - "Kept warning/status text and presentation mode ownership in `MdCadEmbeddedControl`, with `UnexpectedSessionLoss` routing back into the shell."

patterns-established:
  - "Pattern 1: The shared shell composes `IMdCadEmbedBackend` and mounts `backend.Surface`, while all Win32 lifecycle ownership lives in the backend."
  - "Pattern 2: Backend-driven lifecycle changes notify the shell through state/loss events, but user-facing warning and status text remain shell-owned."

requirements-completed: [WPRS-01, WPRS-02]

# Metrics
duration: continued-session
completed: 2026-05-18
---

# Phase 50 Plan 02: Backend Extraction Summary

**Windows placeholder and session lifecycle ownership now live behind the backend seam while the public Avalonia shell keeps the same launch, warning, and presentation contract.**

## Performance

- **Duration:** continued from the Phase 50 execution session
- **Started:** after `50-01` behavior lock baseline completed
- **Completed:** 2026-05-18T11:27:57.0101018+01:00
- **Tasks:** 2
- **Files modified:** 5

## Accomplishments
- Expanded `WindowsMdCadEmbedBackend` to own placeholder creation, runtime launch, attach polling, resize sync, unexpected-loss cleanup, and surface recreation.
- Moved `EmbedNativeControlHost` under `Host/Windows/` and left `MdCadEmbeddedControl` as a thin shell that owns public properties, warning/status UI, and presentation mode only.
- Rewired `MdCadSessionCoordinator` to start, stop, and recreate through backend delegates so the serialized relaunch logic no longer depends on shell-owned Win32/process state.

## Task Commits

Each task was committed atomically:

1. **Task 1: Move native surface and Windows lifecycle state into `WindowsMdCadEmbedBackend`** - `4e80b8a` (refactor)
2. **Task 2: Thin `MdCadEmbeddedControl` and rewire the coordinator to the backend seam** - `c91e805` (refactor)

## Files Created/Modified
- `samples/avalonia-mdcad-control/Host/Windows/WindowsMdCadEmbedBackend.cs` - Now owns the Win32/process lifecycle and still exposes the tested launch-spec builder.
- `samples/avalonia-mdcad-control/Host/Windows/EmbedNativeControlHost.cs` - Becomes the single placeholder-host implementation under the Windows folder.
- `samples/avalonia-mdcad-control/MdCadEmbeddedControl.axaml.cs` - Reduced to public control surface, warning/status text, presentation mode, and backend/coordinator orchestration.
- `samples/avalonia-mdcad-control/Host/MdCadSessionCoordinator.cs` - Now calls backend-driven start/stop/recreate delegates directly.
- `samples/avalonia-mdcad-control.tests/MdCadSessionCoordinatorTests.cs` - Updated for the new backend-driven start delegate shape without changing coordinator behavior expectations.

## Decisions Made
- Preserved the public `MdCadEmbeddedControl` API and diagnostic/sealed UI contract while moving only the Windows-specific ownership beneath it.
- Kept the backend seam internal and concrete in this phase; unsupported-platform backends remain deferred to Phase 51.
- Preserved the existing coordinator sequencing semantics instead of inventing a second lifecycle controller inside the backend.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Updated the coordinator test harness for the backend-driven start delegate**
- **Found during:** Task 2 (Thin `MdCadEmbeddedControl` and rewire the coordinator to the backend seam)
- **Issue:** Changing `MdCadSessionCoordinator` to call `backend.StartSessionAsync(...)` directly reduced the injected start delegate from three parameters to two, which broke `MdCadSessionCoordinatorTests.cs`.
- **Fix:** Updated the test harness lambda to match the new `Func<MdCadLaunchSnapshot, CancellationToken, Task>` delegate shape while preserving the existing session-coalescing assertions.
- **Files modified:** `samples/avalonia-mdcad-control.tests/MdCadSessionCoordinatorTests.cs`
- **Verification:** Re-ran the filtered coordinator/runtime/backend tests and `dotnet build samples/avalonia-host/AvaloniaHost.csproj -c Release`
- **Committed in:** `c91e805` (part of Task 2 commit)

---

**Total deviations:** 1 auto-fixed (1 blocking)
**Impact on plan:** The only extra change was the required test-harness adaptation for the new delegate contract. No behavior scope expanded.

## Issues Encountered
- None beyond the expected coordinator-test harness adjustment caused by the delegate signature change.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness
- `50-03` can now close the automated/manual regression loop on top of the extracted backend seam.
- The manual checklist from `50-01` remains the authoritative host-proof artifact for attach, stop, relaunch, resize, and sealed/diagnostic equivalence.
- No blockers remain for the final regression closure plan.

---
*Phase: 50-backend-seam-extraction-and-windows-behavior-lock*
*Completed: 2026-05-18*
