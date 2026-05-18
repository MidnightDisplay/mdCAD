---
phase: 50-backend-seam-extraction-and-windows-behavior-lock
plan: 01
subsystem: testing
tags: [avalonia, windows, embed, win32, regression]

# Dependency graph
requires:
  - phase: 49-minimal-sealed-avalonia-host-sample-and-quickstart
    provides: Reusable control baseline, Windows diagnostic host, and the current copied-runtime contract that Phase 50 must preserve
provides:
  - Focused automated proof for copied-runtime lookup and launch argument construction
  - Internal backend seam contract for later lifecycle extraction
  - Phase 50 manual checklist bound to the existing Windows diagnostic host
affects: [phase-50-02, phase-50-03, phase-53]

# Tech tracking
tech-stack:
  added: []
  patterns: [contract-first regression locking, extracted win32 helper file, manual host proof scaffold]

key-files:
  created:
    - .planning/phases/50-backend-seam-extraction-and-windows-behavior-lock/50-MANUAL-CHECKLIST.md
    - samples/avalonia-mdcad-control.tests/MdCadRuntimeResolverTests.cs
    - samples/avalonia-mdcad-control.tests/WindowsMdCadEmbedBackendTests.cs
    - samples/avalonia-mdcad-control/Host/IMdCadEmbedBackend.cs
    - samples/avalonia-mdcad-control/Host/Windows/Win32NativeMethods.cs
    - samples/avalonia-mdcad-control/Host/Windows/WindowsMdCadEmbedBackend.cs
  modified:
    - samples/avalonia-mdcad-control/Host/EmbedNativeControlHost.cs
    - samples/avalonia-mdcad-control/Host/MdCadRuntimeResolver.cs

key-decisions:
  - "Kept `MdCadRuntimeResolver.Resolve()` unchanged and added only an internal `ResolveFromBaseDirectory(...)` overload so production callers keep the current API shape."
  - "Added `WindowsMdCadEmbedBackend.CreateStartInfo(...)` as a contract-first launch builder without moving session ownership out of `MdCadEmbeddedControl` yet."
  - "Extracted `Win32NativeMethods` into `Host/Windows/Win32NativeMethods.cs` but kept the existing `MdCad.Avalonia.Control.Host` namespace so current callers compile unchanged until Wave 2 rewires ownership."

patterns-established:
  - "Pattern 1: Lock Windows runtime lookup and launch arguments with focused tests before moving lifecycle ownership."
  - "Pattern 2: When extracting Win32 helpers ahead of a larger seam move, preserve namespace and behavior first, then rewire consumers in the next wave."

requirements-completed: [WPRS-01, WPRS-02]

# Metrics
duration: continued-session
completed: 2026-05-18
---

# Phase 50 Plan 01: Behavior Lock Baseline Summary

**Focused runtime-contract tests, an internal backend seam contract, and a Windows manual checklist now freeze the shipped embedding behavior before lifecycle extraction begins.**

## Performance

- **Duration:** continued from the Phase 50 execution session
- **Started:** after `/gsd-execute-phase 50` initialized Wave 1
- **Completed:** 2026-05-18T11:17:30.0593926+01:00
- **Tasks:** 2
- **Files modified:** 8

## Accomplishments
- Added `MdCadRuntimeResolverTests` and `WindowsMdCadEmbedBackendTests` to pin the copied-runtime lookup contract, working directory, parent-HWND formatting, and optional JSONL/live-refresh forwarding.
- Added the minimal production seam needed for those tests: `ResolveFromBaseDirectory(...)` plus `WindowsMdCadEmbedBackend.CreateStartInfo(...)`, without changing the shipped `MdCadEmbeddedControl` lifecycle yet.
- Defined `IMdCadEmbedBackend`, extracted `Win32NativeMethods` into a dedicated Windows helper file, and wrote `50-MANUAL-CHECKLIST.md` so later waves preserve the existing diagnostic host as the authoritative proof surface.

## Task Commits

Each task was committed atomically:

1. **Task 1: Add Wave 0 automated proof for runtime lookup and Windows launch construction** - `55611de` (test), `b3fbf74` (feat)
2. **Task 2: Define the internal backend seam and bind it to the authoritative manual regression checklist** - `dbf9984` (refactor)

_Note: Task 1 followed TDD with separate RED and GREEN commits._

## Files Created/Modified
- `samples/avalonia-mdcad-control.tests/MdCadRuntimeResolverTests.cs` - Pins `AppContext.BaseDirectory\mdcad-runtime\mdCAD.exe` resolution semantics and missing-runtime failures.
- `samples/avalonia-mdcad-control.tests/WindowsMdCadEmbedBackendTests.cs` - Pins launch-spec working directory, required embed args, and optional JSONL/live-refresh forwarding.
- `samples/avalonia-mdcad-control/Host/MdCadRuntimeResolver.cs` - Adds the internal base-directory overload while preserving `Resolve()`.
- `samples/avalonia-mdcad-control/Host/Windows/WindowsMdCadEmbedBackend.cs` - Holds the minimal contract-first `CreateStartInfo(...)` builder for later backend extraction.
- `samples/avalonia-mdcad-control/Host/IMdCadEmbedBackend.cs` - Defines the internal backend seam the next plan will wire.
- `samples/avalonia-mdcad-control/Host/Windows/Win32NativeMethods.cs` - Extracts the existing Win32 P/Invoke surface into a dedicated Windows helper file.
- `samples/avalonia-mdcad-control/Host/EmbedNativeControlHost.cs` - Drops the inline P/Invoke implementation and keeps placeholder-host behavior unchanged.
- `.planning/phases/50-backend-seam-extraction-and-windows-behavior-lock/50-MANUAL-CHECKLIST.md` - Defines the manual Windows attach/stop/relaunch/resize/sealed-diagnostic checklist bound to `samples/avalonia-host`.

## Decisions Made
- Kept the production runtime lookup entrypoint stable and introduced only the smallest internal seam needed to test it.
- Introduced the Windows backend file as a launch-spec owner first, deferring all session lifecycle movement to `50-02`.
- Bound the new manual checklist explicitly to `samples/avalonia-host` so Wave 3 can verify the already-shipped diagnostic host instead of inventing a new proof harness.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Resolved the new interface's `Control` namespace collision**
- **Found during:** Task 2 (Define the internal backend seam and bind it to the authoritative manual regression checklist)
- **Issue:** `IMdCadEmbedBackend.cs` initially used `Control` directly, but the `MdCad.Avalonia.Control` root namespace caused the host build to treat it as a namespace instead of the Avalonia control type.
- **Fix:** Aliased `Avalonia.Controls.Control` as `AvaloniaControl` in the interface so the internal seam stays explicit and the host build remains clean.
- **Files modified:** `samples/avalonia-mdcad-control/Host/IMdCadEmbedBackend.cs`
- **Verification:** Re-ran the filtered runtime/launch tests and `dotnet build samples/avalonia-host/AvaloniaHost.csproj -c Release`
- **Committed in:** `dbf9984` (part of Task 2 commit)

---

**Total deviations:** 1 auto-fixed (1 blocking)
**Impact on plan:** The fix was narrowly scoped to keep the planned seam contract compiling. No behavior change or scope creep.

## Issues Encountered
- None beyond the interface type-resolution blocker that was auto-fixed during Task 2 verification.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness
- `50-02` can now move placeholder/runtime/process ownership behind `IMdCadEmbedBackend` with launch arguments and runtime lookup already pinned by tests.
- `50-MANUAL-CHECKLIST.md` is ready for the final Windows regression checkpoint in `50-03`.
- No blockers remain for Wave 2.

---
*Phase: 50-backend-seam-extraction-and-windows-behavior-lock*
*Completed: 2026-05-18*
