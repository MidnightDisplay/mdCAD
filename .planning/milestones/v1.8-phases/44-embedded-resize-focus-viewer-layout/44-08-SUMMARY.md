---
phase: 44-embedded-resize-focus-viewer-layout
plan: 08
subsystem: ui
tags: [avalonia, teardown, embedded, imgui, persistence]

# Dependency graph
requires:
  - phase: 44-embedded-resize-focus-viewer-layout
    provides: The graceful wait-before-kill host teardown path, the revised child-focus fix, and the diagnosed normal-close layout flush gap
provides:
  - Shared placeholder invalidation before both destroyed-parent and normal host-close waits
  - A normal-close path that gives mdCAD the invalid-parent signal before fallback cleanup
  - The final code-side gap fix required before Phase 44 human re-verification
affects: [44-HUMAN-UAT, phase-44-verification, phase-45]

# Tech tracking
tech-stack:
  added: []
  patterns: [shared placeholder invalidation seam, normal-close graceful-wait after parent invalidation]

key-files:
  created:
    - .planning/phases/44-embedded-resize-focus-viewer-layout/44-08-SUMMARY.md
  modified:
    - samples/avalonia-host/MainWindow.axaml.cs

key-decisions:
  - "Normal host close now reuses the same placeholder invalidation seam that destroyed-parent mode already proved."
  - "The host-close path no longer performs an extra unconditional kill after the graceful wait path runs."

patterns-established:
  - "Pattern 1: Destroy or detach the embedded placeholder before any blocking close wait so the child can observe the invalid parent chain."
  - "Pattern 2: Keep fallback cleanup inside the graceful wait helper instead of duplicating kill logic in the close handler."

requirements-completed: [EMBD-04, INPT-04]

# Metrics
duration: continued-session
completed: 2026-05-14
---

# Phase 44 Plan 08: Normal-Close Layout Flush Gap Summary

**Normal host close now invalidates the embedded placeholder before waiting, giving mdCAD a real chance to self-exit and flush `imgui.embedded.ini`**

## Performance

- **Duration:** continued from the Phase 44 gap-closure session
- **Started:** after `44-07` completed
- **Completed:** 2026-05-14T21:40:00Z
- **Tasks:** 1
- **Files modified:** 1

## Accomplishments
- Extracted the placeholder invalidation seam into a shared helper so both destroyed-parent and normal host close destroy the placeholder the same way.
- Changed normal host close to invalidate the placeholder before `WaitForGracefulExitOnClose(...)`, giving mdCAD the same invalid-parent quit signal that already worked in destroyed-parent mode.
- Removed the redundant extra kill from `OnWindowClosed(...)` so fallback cleanup remains owned by the graceful wait path.

## Task Commits

Each task was committed atomically:

1. **Task 1: Invalidate the placeholder on normal host close before waiting for graceful mdCAD exit** - `1606ddf` (fix)

**Plan metadata:** pending roadmap/state summary commit

## Files Created/Modified
- `samples/avalonia-host/MainWindow.axaml.cs` - Extracts the shared placeholder invalidation helper, reuses it from normal host close, and leaves fallback cleanup inside the existing graceful wait path.

## Decisions Made
- Reused the existing destroyed-parent teardown seam instead of inventing a second normal-close-only invalidation path.
- Kept `src/app.c` unchanged because the missing behavior was entirely in the Avalonia host close ordering, not in mdCAD's runtime quit guard.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness
- Ready for the final manual re-check of Scenarios 2 and 7.
- Ready to refresh Phase 44 verification artifacts once the remaining human-UAT evidence is collected.

---
*Phase: 44-embedded-resize-focus-viewer-layout*
*Completed: 2026-05-14*
