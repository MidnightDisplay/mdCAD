---
phase: 44-embedded-resize-focus-viewer-layout
plan: 01
subsystem: testing
tags: [embedded, ctest, validation, avalonia, dock-layout]

# Dependency graph
requires:
  - phase: 43-embed-contract-child-window-bootstrap
    provides: Embedded launch parsing, child-window bootstrap, and the baseline Avalonia host commands that Phase 44 validation extends
provides:
  - Pure embedded input ownership reducer coverage for first-click focus and cancel semantics
  - Pure embedded layout policy coverage for standalone-vs-embedded persistence decisions
  - A Phase 44 manual checklist shell covering resize, focus, Alt+Tab, and orphan teardown evidence
affects: [44-02, 44-03, 44-04, phase-44]

# Tech tracking
tech-stack:
  added: []
  patterns: [header-only embedded state reducers, CTest seam-first validation, checklist-first manual evidence]

key-files:
  created:
    - src/embed_input_state.h
    - src/embed_layout_state.h
    - src/tests/embed_input_state_test.c
    - src/tests/embed_layout_state_test.c
    - .planning/phases/44-embedded-resize-focus-viewer-layout/44-MANUAL-CHECKLIST.md
  modified:
    - src/CMakeLists.txt

key-decisions:
  - "Added pure reducers for embedded input and layout policy so Phase 44 runtime work can consume tested decisions instead of re-encoding them in app code."
  - "Created the Phase 44 manual checklist before runtime changes so resize, focus, and orphan scenarios have a stable evidence shape across Plans 44-02 and 44-03."

patterns-established:
  - "Pattern 1: Model embedded focus ownership as a pure reducer before wiring it into Sokol or ImGui events."
  - "Pattern 2: Model embedded layout persistence choice separately from runtime ImGui storage code."
  - "Pattern 3: Predeclare manual host lifecycle scenarios before the host harness implements every mode."

requirements-completed: [EMBD-04, INPT-01, INPT-02, INPT-03, INPT-04]

# Metrics
duration: 7 min
completed: 2026-05-14
---

# Phase 44 Plan 01: Validation Seams and Checklist Summary

**Pure embedded input/layout reducers with native CTest coverage and a Phase 44 manual checklist shell for resize, focus, and orphan lifecycle verification**

## Performance

- **Duration:** 7 min
- **Started:** 2026-05-14T17:20:00Z
- **Completed:** 2026-05-14T17:27:26Z
- **Tasks:** 2
- **Files modified:** 6

## Accomplishments
- Added `embed_input_state.h` plus `embed_input_state_test` to lock the first-click ownership, click-away return, off-surface drag, and deactivation-cancel rules before `app.c` changes land.
- Added `embed_layout_state.h` plus `embed_layout_state_test` to lock the embedded-vs-standalone persistence split and first-run layout seeding rule before ImGui storage changes land.
- Created the Phase 44 manual checklist shell with concrete valid, invalid-parent, destroyed-parent, and `destroy-after-attach` scenarios.

## Task Commits

Each task was committed atomically:

1. **Task 1: Create pure Wave 0 helper seams and register native tests** - `e683e55` (test)
2. **Task 2: Write the Phase 44 manual checklist shell before runtime changes** - `3c0de6e` (docs)

**Plan metadata:** pending closeout docs commit

## Files Created/Modified
- `src/embed_input_state.h` - Header-only reducer for embedded keyboard ownership and cancel semantics.
- `src/embed_layout_state.h` - Header-only helper for embedded layout persistence choice and first-run dock seeding.
- `src/tests/embed_input_state_test.c` - Native CTest coverage for first-click activation, click-away return, off-surface drag, and deactivation cancel.
- `src/tests/embed_layout_state_test.c` - Native CTest coverage for standalone-vs-embedded settings selection and first-run seeding.
- `src/CMakeLists.txt` - Registers the new embedded state tests with CTest.
- `.planning/phases/44-embedded-resize-focus-viewer-layout/44-MANUAL-CHECKLIST.md` - Wave 0 manual verification shell for resize, focus, Alt+Tab, and orphan teardown scenarios.

## Decisions Made
- Used pure header-only reducers so Phase 44 can validate the focus/layout policy before touching Win32, Sokol, or ImGui runtime seams.
- Kept the checklist concrete from the start, including the planned `destroy-after-attach` host mode, so later plans only need to bind observations instead of inventing scenario structure.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness
- Ready for `44-02` and `44-03`: the native runtime and Avalonia host work now have fast regression seams plus a shared manual evidence shell.
- No blockers identified from the Wave 0 scaffolding work.

---
*Phase: 44-embedded-resize-focus-viewer-layout*
*Completed: 2026-05-14*
