---
phase: 57-investigate-mdcad-embedded-crash-when-deleting-imported-json
plan: 01
subsystem: native-runtime
tags: [jsonl, embedded, win32, diagnostics, testing]
requires:
  - phase: 56-create-wpf-mdcad-control-for-net10-0-windows-consumers-with-
    provides: embedded Windows host/control baseline and runtime-root working-directory contract
provides:
  - startup-import delete regression coverage for native hierarchy deletion
  - embedded crash-log writer and one-line stderr summary helpers
  - app wiring for embedded startup diagnostics and fatal-handler installation
affects: [phase-57-plan-02, phase-57-plan-03, phase-57-plan-04]
tech-stack:
  added: []
  patterns: [canonicalized hierarchy delete roots, working-directory embed crash log]
key-files:
  created:
    - src/tests/startup_jsonl_delete_regression_test.c
    - src/tests/win32_embed_diagnostics_test.c
  modified:
    - src/CMakeLists.txt
    - src/ui/ui_scene_hierarchy.h
    - src/jsonl_observer_system.h
    - src/platform/win32_embed.h
    - src/app.c
    - .planning/phases/57-investigate-mdcad-embedded-crash-when-deleting-imported-json/57-VALIDATION.md
key-decisions:
  - "Canonicalize hierarchy deletes to live roots before undo snapshotting or teardown."
  - "Write one stable mdcad-embed-crash.log record and derive stderr summaries from the same native helper."
patterns-established:
  - "Pattern 1: embedded diagnostics stay in the native seam and flow outward through stderr/log references."
  - "Pattern 2: hierarchy delete paths remove selected descendants before recursive scene teardown."
requirements-completed: [P57-01, P57-02, P57-03]
duration: 18 min
completed: 2026-05-29
---

# Phase 57 Plan 01: Fix the native startup-import delete crash and add embedded working-directory crash logging with native regression coverage Summary

**Startup-import delete coverage now hardens hierarchy root canonicalization and emits stable embedded crash-log summaries from the native Win32 seam.**

## Performance

- **Duration:** 18 min
- **Started:** 2026-05-29T10:57:09Z
- **Completed:** 2026-05-29T11:15:21Z
- **Tasks:** 2
- **Files modified:** 8

## Accomplishments
- Added a native startup-import delete regression lane covering plain startup import, startup live refresh, canonicalized bulk delete ordering, and root-delete refresh cancellation.
- Hardened `ui_scene_hierarchy_delete_entities(...)` to collapse descendant selections into live roots, cancel active observer refresh roots once, and clear subtree selections before teardown.
- Added embedded crash-log helpers that overwrite `mdcad-embed-crash.log`, emit one truthful stderr summary line, and are wired into embedded startup parse failure plus fatal-handler installation.

## Task Commits

Each task was committed atomically:

1. **Task 1: Lock the startup-imported delete regression and harden the native delete path** - `c8bf8a0`, `c81ba7e` (test, feat)
2. **Task 2: Emit a stable embedded crash record and stderr summary from the native seam** - `e02af8a`, `6db2d39` (test, feat)

_Note: TDD tasks used separate RED and GREEN commits._

## Files Created/Modified
- `src/tests/startup_jsonl_delete_regression_test.c` - Headless native regression lane for startup-import delete behavior.
- `src/tests/win32_embed_diagnostics_test.c` - Native diagnostics contract tests for crash-log overwrite, summary formatting, and app wiring.
- `src/CMakeLists.txt` - Registers both new native test targets with CTest.
- `src/ui/ui_scene_hierarchy.h` - Canonicalizes delete roots, clears subtree selection, and deduplicates observer refresh cancellation.
- `src/jsonl_observer_system.h` - Adds observer-root resolution for imported entities.
- `src/platform/win32_embed.h` - Defines crash-log summary/record helpers and embedded fatal-handler plumbing.
- `src/app.c` - Routes embedded parse failures through the diagnostics seam and installs embedded fatal handlers.
- `.planning/phases/57-investigate-mdcad-embedded-crash-when-deleting-imported-json/57-VALIDATION.md` - Marks 57-01 native validation rows green.

## Decisions Made
- Native delete proof stays headless by exercising startup-imported roots plus an attached child anchor in the regression lane rather than requiring a live graphics device in CTest.
- Embedded diagnostics reuse the working directory and stderr seam instead of introducing new host/runtime IPC.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Kept the startup delete regression headless**
- **Found during:** Task 1 (startup-import delete regression)
- **Issue:** Import-generated geometry requires a live Sokol graphics device, which the native CTest lane does not initialize.
- **Fix:** Kept the regression focused on the startup-imported root plus an attached child anchor so the hierarchy delete/undo/observer path remains covered without introducing graphics-device bootstrap into the native test lane.
- **Files modified:** src/tests/startup_jsonl_delete_regression_test.c
- **Verification:** `ctest --test-dir .\build -C Debug -R "startup_jsonl_delete_regression_test|win32_embed_diagnostics_test" --output-on-failure`
- **Committed in:** c81ba7e

---

**Total deviations:** 1 auto-fixed (Rule 3: 1)
**Impact on plan:** Native delete behavior and diagnostics scope stayed intact; the deviation only adapted the regression fixture to the existing headless test environment.

## Issues Encountered
- Multi-config CTest on Windows required `-C Debug` to execute the newly built native tests against the generated Visual Studio configuration.

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- Avalonia and WPF host plans can now consume a stable native stderr/log contract from `mdcad-embed-crash.log`.
- Native proof lanes for startup-import delete behavior and embedded diagnostics are green and ready for phase-57 host surfacing work.

## Self-Check: PASSED
