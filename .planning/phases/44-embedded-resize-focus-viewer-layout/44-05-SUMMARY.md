---
phase: 44-embedded-resize-focus-viewer-layout
plan: 05
subsystem: runtime
tags: [embedded, keyboard, imgui, focus, shortcuts]

# Dependency graph
requires:
  - phase: 44-embedded-resize-focus-viewer-layout
    provides: Embedded reducer-owned focus state and the diagnosed UAT gap in 44-UAT.md
provides:
  - Embedded shortcut gating that honors reducer-owned keyboard focus
  - First-click ownership path that no longer suppresses Tab and other mdCAD shortcuts
affects: [44-06, 44-UAT, phase-44-verification]

# Tech tracking
tech-stack:
  added: []
  patterns: [embedded shortcut gating driven by reducer state instead of blanket WantCaptureKeyboard rejection]

key-files:
  created: []
  modified:
    - src/app.c

key-decisions:
  - "Embedded mode now trusts reducer-owned keyboard state as the authority for shortcut enablement."
  - "Standalone behavior keeps the previous `!WantCaptureKeyboard` guard unchanged."

patterns-established:
  - "Pattern 1: Embedded shortcut handling may diverge from standalone ImGui capture rules when reducer-owned focus is the product contract."

requirements-completed: [INPT-02]

# Metrics
duration: continued-session
completed: 2026-05-14
---

# Phase 44 Plan 05: Embedded Keyboard Ownership Gap Summary

**Embedded first-click ownership now reaches the mdCAD shortcut block instead of being swallowed by `WantCaptureKeyboard`**

## Performance

- **Duration:** continued from the Phase 44 gap-closure session
- **Started:** after `44-UAT.md` diagnosis
- **Completed:** 2026-05-14T20:54:44+01:00
- **Tasks:** 1
- **Files modified:** 1

## Accomplishments
- Removed the embedded-mode blanket block on `ImGuiIO.WantCaptureKeyboard` so reducer-owned first-click focus can reach mdCAD shortcuts.
- Preserved standalone behavior by keeping the old `!WantCaptureKeyboard` rule outside embedded mode.
- Re-ran the embedded launch-config and reducer regression targets after the shortcut-gate change.

## Task Commits

Each task was committed atomically:

1. **Task 1: Let reducer-owned embedded keyboard focus reach the mdCAD shortcut block** - `1620054` (fix)

**Plan metadata:** pending phase-gap closeout docs commit

## Files Created/Modified
- `src/app.c` - Updates `mdcad_embedded_shortcuts_allowed(...)` so embedded shortcut enablement follows reducer-owned focus instead of `WantCaptureKeyboard`.

## Decisions Made
- Trusted `state.embed_input` as the only authority for embedded keyboard ownership, matching the original Phase 44 design.
- Kept the standalone shortcut guard untouched to avoid changing non-embedded behavior while closing the UAT gap.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness
- Ready for the manual re-check of UAT Scenario 2.
- Ready for `44-06`, which closes the remaining teardown and layout-persistence gaps.

---
*Phase: 44-embedded-resize-focus-viewer-layout*
*Completed: 2026-05-14*
