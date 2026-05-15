---
phase: 45-startup-jsonl-auto-import
plan: 02
subsystem: app-runtime
tags: [jsonl, startup, app, overlay, embedded]

# Dependency graph
requires:
  - phase: 45-startup-jsonl-auto-import
    provides: Absolute `--jsonl` launch parsing plus reusable startup import controller
provides:
  - App-owned startup JSONL import lifecycle in `app.c`
  - Embedded-safe startup status/error overlay outside Scene Hierarchy visibility
  - Scene hierarchy cache dirtying on startup import completion
affects: [phase-45-verification, phase-46, embedded-startup]

# Tech tracking
tech-stack:
  added: []
  patterns: [app-owned startup import lifecycle, frame-loop import tick before panel visibility gate, dismissible startup error overlay]

key-files:
  created:
    - .planning/phases/45-startup-jsonl-auto-import/45-02-SUMMARY.md
  modified:
    - src/app.c
    - src/tests/startup_jsonl_app_contract_test.c
    - src/CMakeLists.txt

key-decisions:
  - "Startup JSONL import ticks immediately after `simgui_new_frame(...)` and before `if (state.ui_visible)` so embedded viewer-first layouts still progress."
  - "A successful startup import only dirties the Scene Hierarchy cache; ownership stays in `app.c` instead of moving back into the panel."
  - "Startup import failure is non-fatal and surfaces through a dismissible app-level overlay rather than by quitting or silently ignoring the request."

patterns-established:
  - "Pattern 1: Launch-owned background work that must survive hidden panels belongs in the app frame loop, not in panel draw code."
  - "Pattern 2: Embedded-safe startup failures should surface through small app-level overlays that never own process shutdown."

requirements-completed: [JSON-01, JSON-03]

# Metrics
duration: continued-session
completed: 2026-05-15
---

# Phase 45 Plan 02: App Integration and Startup Overlay Summary

**Startup JSONL import is now owned by `app.c`, runs without requiring Scene Hierarchy visibility, and leaves the viewer usable with a clear dismissible error surface when launch-time import fails**

## Performance

- **Duration:** continued from the same Phase 45 execution session after 45-01 landed and was committed
- **Started:** after `e2fb543` closed the parser/controller foundation
- **Completed:** 2026-05-15T09:51:15.0756771+01:00
- **Tasks:** 2
- **Files modified:** 3

## Accomplishments
- Added `src/tests/startup_jsonl_app_contract_test.c` and wired it into `src/CMakeLists.txt` so the app ownership seam is locked before future changes touch startup import behavior.
- Integrated `startup_jsonl_import_controller_t` into `src/app.c` state, initialized and armed it during `init()`, and reset it during `cleanup()` before scene shutdown.
- Ticked startup import in `frame()` immediately after `simgui_new_frame(...)` and before the `state.ui_visible` gate, then marked `ui_scene_hierarchy` dirty when startup import completed.
- Added a small app-level `Startup JSONL Import` overlay that shows progress while importing and red latched error text with a dismiss action on failure.
- Preserved the embedded lifecycle boundary from Phase 44 by keeping startup failures non-fatal and leaving the viewer running.

## Task Commits

Each task was committed atomically where practical:

1. **Task 1: RED app ownership/source-contract test** - pending
2. **Task 2: `app.c` integration, overlay, and phase closeout** - pending

## Files Created/Modified
- `src/tests/startup_jsonl_app_contract_test.c` - Source-contract coverage for app-owned init/frame/cleanup ownership and overlay wiring.
- `src/CMakeLists.txt` - Registers the new startup app contract test target in CTest.
- `src/app.c` - Owns startup controller lifecycle, marks hierarchy dirty on success, and draws the startup status/error overlay outside the panel gate.

## Decisions Made
- Kept startup import progress in the app frame loop rather than in `ui_scene_hierarchy_draw()` so embedded viewer-first layouts behave the same as visible-panel launches.
- Hid the overlay after success and kept it visible only while running or when an error is latched, which keeps startup feedback clear without adding a new persistent panel.

## Deviations from Plan

- None.

## Issues Encountered

- None beyond the planned contract-first red/green flow.

## User Setup Required

None - startup JSONL stays available through the existing executable with an absolute-path `--jsonl` argument.

## Next Phase Readiness
- Phase 45 is ready to close with JSON-01 and JSON-03 complete.
- The next phase can focus purely on opt-in launch-time live refresh because startup auto-import now has a stable app-owned seam.

---
*Phase: 45-startup-jsonl-auto-import*
*Completed: 2026-05-15*
