---
phase: 44-embedded-resize-focus-viewer-layout
plan: 09
subsystem: runtime
tags: [embedded, keyboard, imgui, shortcuts, focus]

# Dependency graph
requires:
  - phase: 44-embedded-resize-focus-viewer-layout
    provides: Working native child-window focus handoff and a narrowed human-UAT shortcut-routing diagnosis
provides:
  - Embedded global shortcut routing through ImGui instead of raw key polling
  - Standalone shortcut behavior preserved alongside embedded-only routing changes
  - The last code-side fix required before the final Scenario 2 human re-check
affects: [44-HUMAN-UAT, phase-44-verification, phase-45]

# Tech tracking
tech-stack:
  added: []
  patterns: [embedded-only ImGui global shortcut routing, standalone shortcut polling preserved]

key-files:
  created:
    - .planning/phases/44-embedded-resize-focus-viewer-layout/44-09-SUMMARY.md
  modified:
    - src/app.c

key-decisions:
  - "Embedded mode now routes mdCAD-global shortcuts through ImGui's `Shortcut()` API because the shortcut block runs outside a specific window and raw key polling missed routed non-text shortcuts."
  - "Standalone mdCAD keeps the previous raw key polling path so the embedded fix does not change non-embedded shortcut behavior."

patterns-established:
  - "Pattern 1: When embedded shortcut handling lives outside a specific ImGui window, use a routed global shortcut helper instead of raw key polling."
  - "Pattern 2: Keep text widgets in charge of their own keys by letting ImGui routing arbitrate shortcut ownership."

requirements-completed: [INPT-02]

# Metrics
duration: continued-session
completed: 2026-05-14
---

# Phase 44 Plan 09: Embedded Global Shortcut Routing Summary

**Embedded mdCAD now routes its global shortcuts through ImGui so non-text `Tab` and undo/redo can work after focus is acquired without regressing standalone behavior**

## Performance

- **Duration:** continued from the Phase 44 final human-UAT follow-up session
- **Started:** after `44-09-PLAN.md` was created and verified
- **Completed:** 2026-05-14T22:00:00Z
- **Tasks:** 1
- **Files modified:** 1

## Accomplishments
- Replaced the embedded global shortcut block's raw key polling with an embedded-only routed ImGui shortcut helper.
- Routed embedded `Tab`, undo/redo, constraint-menu, and delete shortcuts through ImGui while preserving the existing reducer-owned keyboard gate.
- Kept the standalone shortcut block unchanged so the fix stays isolated to embedded mode.

## Task Commits

Each task was committed atomically:

1. **Task 1: Route embedded global shortcuts through ImGui instead of raw key polling** - `d70772a` (fix)

**Plan metadata:** pending roadmap/state summary commit

## Files Created/Modified
- `src/app.c` - Adds the embedded routed-shortcut helper and splits embedded versus standalone shortcut evaluation so embedded mdCAD no longer depends on raw `igIsKeyPressed_Bool(...)` polling for global actions.

## Decisions Made
- Used ImGui shortcut routing for embedded mode because the remaining failing keys were above the Win32 focus seam and needed ImGui-level ownership arbitration.
- Chose a global route for the embedded helper because the shortcut block runs outside any particular ImGui window and therefore cannot rely on root-window routing from a current window context.

## Deviations from Plan

- Adjusted the routed shortcut flags from focused/root-window routing to a global route during execution after confirming from ImGui source that `RouteFromRootWindow` requires a current window context, which this global shortcut block does not have.

## Issues Encountered

None.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness
- Ready for the final human re-check of Scenario 2.
- Ready to refresh Phase 44 verification artifacts and close the phase if Scenario 2 now passes.

---
*Phase: 44-embedded-resize-focus-viewer-layout*
*Completed: 2026-05-14*
