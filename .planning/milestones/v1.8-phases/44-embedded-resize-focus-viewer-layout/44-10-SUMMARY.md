---
phase: 44-embedded-resize-focus-viewer-layout
plan: 10
subsystem: runtime
tags: [embedded, input, win32, focus, shortcuts]

# Dependency graph
requires:
  - phase: 44-embedded-resize-focus-viewer-layout
    provides: Routed embedded shortcuts plus native child-window focus handoff
provides:
  - Embedded keyboard ownership preserved across normal mouse release
  - Embedded drag commit and camera inertia preserved after release
  - Embedded child claims Tab-class dialog keys from the Win32 host path
affects: [44-HUMAN-UAT, phase-44-verification, phase-45]

# Tech tracking
tech-stack:
  added: []
  patterns: [embedded runtime cancel tied only to true focus loss, embedded child WM_GETDLGCODE claim, dockspace-owned embedded ImGui shortcut routing]

key-files:
  created:
    - .planning/phases/44-embedded-resize-focus-viewer-layout/44-10-SUMMARY.md
  modified:
    - src/app.c
    - vendors/libsokol/patches/0001-win32-embed-child-window-bootstrap.patch

key-decisions:
  - "Normal WM_CAPTURECHANGED/WM_CANCELMODE is not host deactivation in embedded mode, so it must clear capture without clearing mdCAD's reducer-owned keyboard ownership."
  - "The embedded child window explicitly claims WM_GETDLGCODE keyboard/dialog keys so Tab-class input stays available to mdCAD instead of the host message pump."
  - "The embedded shortcut helper remains bound to the dockspace owner so post-release global shortcuts stay routed within mdCAD once keyboard ownership is preserved."

patterns-established:
  - "Pattern 1: In embedded Win32 child mode, only real focus/activation loss should trigger runtime input cancellation."
  - "Pattern 2: If the host stack can legally consume Tab-class keys, have the child HWND advertise WM_GETDLGCODE from the native seam."

requirements-completed: [INPT-02, INPT-03]

# Metrics
duration: continued-session
completed: 2026-05-14
---

# Phase 44 Plan 10: Embedded Release-Path Closure Summary

**Embedded mdCAD now keeps keyboard ownership and interaction completion across a normal mouse release, which closes the last remaining hosted-input gap**

## Performance

- **Duration:** continued from the Phase 44 final human-UAT follow-up session
- **Started:** after the failed post-44-09 re-check exposed dead post-release shortcuts plus canceled drag/inertia behavior
- **Completed:** 2026-05-14T23:42:47.2166361+01:00
- **Tasks:** 1
- **Files modified:** 2

## Accomplishments
- Narrowed the embedded Win32 runtime-cancel path so WM_CAPTURECHANGED and WM_CANCELMODE only clear capture instead of being treated as host deactivation.
- Added embedded WM_GETDLGCODE handling so the child window claims dialog keys such as Tab from the Win32 host path.
- Kept the embedded ImGui shortcut helper bound to the dockspace owner so post-release mdCAD-global shortcuts now remain usable once keyboard ownership survives release.
- Confirmed with final human UAT that Delete/C/Tab/Ctrl+Z work, gizmo drops stick on release, camera inertia continues after release, and the earlier Scenario 5 and Scenario 7 passes remain intact.

## Task Commits

Each task was committed atomically:

1. **Task 1: Narrow runtime cancel to genuine focus loss and claim embedded dialog keys** - pending

**Plan metadata:** pending roadmap/state summary commit

## Files Created/Modified
- `vendors/libsokol/patches/0001-win32-embed-child-window-bootstrap.patch` - Stops normal capture-change from requesting embedded runtime cancel and adds embedded WM_GETDLGCODE handling.
- `src/app.c` - Keeps the dockspace-owned routed embedded shortcut helper used by the final passing build.

## Decisions Made
- Treated the drag snap-back, missing post-release shortcuts, and dead camera inertia as one shared release-path bug instead of separate viewport or shortcut bugs.
- Left runtime cancel on actual focus/activation loss so real host deactivation still tears down embedded interaction safely.

## Deviations from Plan

- None.

## Issues Encountered

- Early ImGui routing-only fixes changed shortcut ownership strategy but produced no visible user-facing change because the real regression lived in the native release path.
- A rebuild was briefly blocked by a still-running `mdCAD.exe` from the manual host test and succeeded after the embedded process was closed.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness
- Phase 44 is ready to close with Scenario 2, Scenario 5, and Scenario 7 passing.
- Ready to move into Phase 45 planning for startup JSONL auto-import.

---
*Phase: 44-embedded-resize-focus-viewer-layout*
*Completed: 2026-05-14*
