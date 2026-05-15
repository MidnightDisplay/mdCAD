---
phase: 43-embed-contract-child-window-bootstrap
plan: 03
subsystem: ui
tags: [avalonia, nativecontrolhost, win32, embedding, checklist]

# Dependency graph
requires:
  - phase: 43-01
    provides: Avalonia host scaffold, placeholder HWND control, and checklist shell
  - phase: 43-02
    provides: Embedded mdCAD child-window bootstrap path and startup defaults
provides:
  - Auto-launching Avalonia host flow with PID-based child attach detection and embed-failure capture
  - Host runtime fixes for NativeControlHost manifest requirements and surface-readiness launch timing
  - Manual bootstrap checklist evidence showing valid attach plus invalid/destroyed parent-HWND failure coverage
affects: [phase-44, phase-45, phase-47]

# Tech tracking
tech-stack:
  added: [Win32 application manifest]
  patterns: [PID-based child attach polling, host-side child resize sync, manual checklist evidence capture]

key-files:
  created:
    - samples/avalonia-host/app.manifest
  modified:
    - samples/avalonia-host/AvaloniaHost.csproj
    - samples/avalonia-host/MainWindow.axaml.cs
    - src/app.c
    - .planning/phases/43-embed-contract-child-window-bootstrap/43-MANUAL-CHECKLIST.md

key-decisions:
  - "Added a Win32 application manifest because Avalonia `NativeControlHost` would crash without supported OS declarations."
  - "Pulled initial host-side resize sync and restored the requested mdCAD windows into Phase 43 after manual verification feedback blocked sign-off."

patterns-established:
  - "Pattern 1: Launch mdCAD only after the host placeholder HWND reports a usable client area."
  - "Pattern 2: Capture `Embedded startup failed:` from the child process and surface it in the host status UI."
  - "Pattern 3: Keep the attached mdCAD child HWND sized from the host side during drag/snap/fullscreen transitions."

requirements-completed: [HOST-01, EMBD-03]

# Metrics
duration: 76 min
completed: 2026-05-14
---

# Phase 43 Plan 03: Host Proof Harness Summary

**Completed the Avalonia proof harness with auto-launch, PID-based child attach detection, explicit failure-path capture, host-side child resize syncing, and a passing manual bootstrap checklist.**

## Performance

- **Duration:** 76 min
- **Started:** 2026-05-14T15:25:32+01:00
- **Completed:** 2026-05-14T16:41:31+01:00
- **Tasks:** 3
- **Files modified:** 5

## Accomplishments
- Completed the host launch flow so the sample auto-launches mdCAD into the `NativeControlHost` placeholder HWND and detects attach by child PID.
- Hardened the sample host against real Win32 runtime issues by adding the required application manifest, waiting for a valid host surface before launch, and keeping the child HWND synced during resize-heavy interactions.
- Recorded the final manual checklist evidence for valid attach plus invalid/destroyed parent-HWND fast-failure behavior with no standalone fallback.

## Task Commits

Each task was committed atomically:

1. **Task 1: Complete the minimal Avalonia host with attach polling and invalid-parent smoke modes** - `847cb90` (feat), `c4121ee` (fix), `00d3ecd` (fix), `8699ffa` (fix)
2. **Task 2: Write the Phase 43 bootstrap smoke checklist with invalid/destroyed parent-HWND proof** - `8730650` (docs)
3. **Task 3: Run the host bootstrap smoke pass and record the result** - `f72cab9` (docs)

**Plan metadata:** pending closeout docs commit

## Files Created/Modified
- `samples/avalonia-host/AvaloniaHost.csproj` - Host project manifest wiring for `NativeControlHost` compatibility.
- `samples/avalonia-host/app.manifest` - Supported-OS declarations required for Avalonia Win32 native host creation.
- `samples/avalonia-host/MainWindow.axaml.cs` - Auto-launch, attach polling, stderr capture, invalid/destroyed parent modes, and host-side resize sync.
- `src/app.c` - Keeps the requested mdCAD windows visible in embedded mode while preserving debug-window defaults off.
- `.planning/phases/43-embed-contract-child-window-bootstrap/43-MANUAL-CHECKLIST.md` - Final PASS verdict and recorded bootstrap evidence.

## Decisions Made
- Treated the missing manifest as a runtime blocker rather than a local machine quirk because the sample host must be reproducible for other developers.
- Kept child resize correction in the host for Phase 43 so the proof harness stays usable even before deeper embedded interaction hardening in Phase 44.
- Accepted the user’s updated preference to keep the requested mdCAD panels visible in embedded mode and overrode the earlier chrome-trimming choice for this milestone.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Added the Win32 application manifest required by Avalonia `NativeControlHost`**
- **Found during:** Task 1 (Complete the minimal Avalonia host with attach polling and invalid-parent smoke modes)
- **Issue:** The host crashed before reaching our code because Avalonia Win32 native hosting requires supported OS declarations in the application manifest.
- **Fix:** Added `samples/avalonia-host/app.manifest` and wired it through `AvaloniaHost.csproj`.
- **Files modified:** `samples/avalonia-host/AvaloniaHost.csproj`, `samples/avalonia-host/app.manifest`
- **Verification:** `dotnet build samples/avalonia-host/AvaloniaHost.csproj -c Release` and host process stayed alive long enough to launch `mdCAD.exe`
- **Committed in:** `c4121ee` (Task 1 fix commit)

**2. [Rule 3 - Blocking] Delayed launch until the host surface had a usable client area**
- **Found during:** Task 1 (Complete the minimal Avalonia host with attach polling and invalid-parent smoke modes)
- **Issue:** mdCAD was being launched before the placeholder HWND had a stable client size, causing invisible or non-usable initial child sizing.
- **Fix:** Added a surface-readiness retry gate and initialized the placeholder HWND from the parent client rect.
- **Files modified:** `samples/avalonia-host/MainWindow.axaml.cs`
- **Verification:** Host stayed alive, `mdCAD.exe` launched, and Scenario 1 attach could progress to `attached`
- **Committed in:** `c4121ee` (Task 1 fix commit)

### User-directed scope change

**1. [Rule 4 - User-directed] Pulled initial resize syncing and panel visibility into Phase 43**
- **Found during:** Task 3 (Run the host bootstrap smoke pass and record the result)
- **Issue:** Manual verification blocked sign-off because the child surface could desync during resize/snap/fullscreen changes and the requested mdCAD panels were hidden.
- **Decision:** The user explicitly chose to pull the embedded resize behavior into Phase 43 instead of pausing to re-plan the Phase 43/44 boundary.
- **Fix:** Kept `state.ui_visible` on in embedded mode and added persistent host-side child resize syncing.
- **Files modified:** `src/app.c`, `samples/avalonia-host/MainWindow.axaml.cs`
- **Verification:** User re-ran Scenario 1 and confirmed it now passes.
- **Committed in:** `00d3ecd`, `8699ffa`

---

**Total deviations:** 2 auto-fixed (2 blocking), 1 user-directed scope change
**Impact on plan:** The blocking fixes were required to make the proof harness actually runnable. The user-directed resize/visibility pull expanded Phase 43 slightly but resolved the exact manual-signoff issues without changing later JSONL work.

## Issues Encountered
- The first host runs failed silently from the user’s perspective because the process was crashing in Avalonia’s Win32 native host setup before our launch code executed.
- After the host stopped crashing, valid attach still exposed timing and resize-sync issues that only showed up under real window resizing, snap layouts, and fullscreen/windowed transitions.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness
- Phase 43 now has recorded attach/failure evidence and the sample host is viable for later JSONL workflow work.
- Phase 44 can focus on deeper embedded interaction/lifecycle polish instead of first-pass host viability.
- Phase 45 can build on a working host + embedded viewer baseline for startup JSONL auto-import.

---
*Phase: 43-embed-contract-child-window-bootstrap*
*Completed: 2026-05-14*
