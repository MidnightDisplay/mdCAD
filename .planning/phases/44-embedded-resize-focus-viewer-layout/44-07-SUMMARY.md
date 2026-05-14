---
phase: 44-embedded-resize-focus-viewer-layout
plan: 07
subsystem: windows
tags: [embedded, focus, win32, avalonia, keyboard]

# Dependency graph
requires:
  - phase: 44-embedded-resize-focus-viewer-layout
    provides: Diagnosed human-UAT focus gap, reducer-owned embedded shortcut gating, and the existing embedded child-window bootstrap seam
provides:
  - Native first-click focus acquisition inside the mdCAD child HWND
  - Avalonia-side host chrome focus return without cross-process focus forcing
  - A revised gap plan and diagnosis aligned with the Phase 44 research seam
affects: [44-HUMAN-UAT, phase-44-verification, 44-08, phase-45]

# Tech tracking
tech-stack:
  added: []
  patterns: [native child-window first-click focus acquisition, host-side chrome focus return without child proxying]

key-files:
  created:
    - .planning/phases/44-embedded-resize-focus-viewer-layout/44-07-SUMMARY.md
  modified:
    - vendors/libsokol/patches/0001-win32-embed-child-window-bootstrap.patch
    - samples/avalonia-host/MainWindow.axaml
    - samples/avalonia-host/MainWindow.axaml.cs

key-decisions:
  - "Embedded keyboard ownership is now claimed from mdCAD's own Win32 child-window message path instead of from the Avalonia host."
  - "Host chrome regains focus through Avalonia focus APIs only; the host never calls `SetFocus` or `SetActiveWindow` on the external mdCAD child HWND."

patterns-established:
  - "Pattern 1: Use embedded-only Win32 click/activation hooks in the child window to acquire focus without auto-focusing on attach."
  - "Pattern 2: Return focus to the host from focusable Avalonia chrome instead of proxying native focus into or out of the child process."

requirements-completed: [INPT-02]

# Metrics
duration: continued-session
completed: 2026-05-14
---

# Phase 44 Plan 07: Embedded First-Click Focus Gap Summary

**The embedded viewer now claims keyboard ownership from mdCAD's own child HWND, and the host can reclaim focus without forcing cross-process Win32 focus**

## Performance

- **Duration:** continued from the Phase 44 human-UAT gap-closure session
- **Started:** after the revised `44-07-PLAN.md` passed plan verification
- **Completed:** 2026-05-14T21:00:00Z
- **Tasks:** 2
- **Files modified:** 3

## Accomplishments
- Added an embedded-only first-click focus acquisition path in the Sokol Win32 child-window seam so mdCAD claims focus from its own `WM_MOUSEACTIVATE` and mouse-down path.
- Preserved the no-auto-focus contract by keeping focus acquisition off the attach, resize, and timer-driven paths.
- Added a focusable Avalonia host-chrome surface that explicitly returns focus to the host without forcing native child focus from the host process.
- Revised the remaining gap plan and human-UAT diagnosis so the Phase 44 artifacts now target the same native seam the research called out.

## Task Commits

Each task was committed atomically:

1. **Task 1: Add an embedded-only native first-click focus acquisition path inside mdCAD's Win32 child window** - `8b8df2a` (fix)
2. **Task 2: Give host chrome an explicit Avalonia-side focus return path** - `c4a8324` (fix)

**Plan metadata:** pending roadmap/state summary commit

## Files Created/Modified
- `vendors/libsokol/patches/0001-win32-embed-child-window-bootstrap.patch` - Extends the embedded Win32 patch with child-owned focus acquisition on first click while preserving the existing deactivate/capture-loss cancellation path.
- `samples/avalonia-host/MainWindow.axaml` - Makes the status/chrome area focusable and wires a host-chrome pointer hook for focus return.
- `samples/avalonia-host/MainWindow.axaml.cs` - Implements the host-chrome pointer handler and keeps the host free of any cross-process child focus forcing.

## Decisions Made
- Kept first-click keyboard ownership native to mdCAD's own Win32 child-window procedure, matching the Phase 44 research guidance and avoiding unreliable host-side focus forcing.
- Solved host focus return separately on the Avalonia side so D-02 remains true without introducing keyboard proxying or synthetic input.

## Deviations from Plan

None - the revised plan executed exactly as written.

## Issues Encountered

None.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness
- Ready for `44-08`, which closes the remaining normal-close embedded-layout flush gap.
- Ready for the final manual re-check of Scenario 2 after `44-08` lands and Scenario 7 can be re-verified alongside it.

---
*Phase: 44-embedded-resize-focus-viewer-layout*
*Completed: 2026-05-14*
