---
phase: 44-embedded-resize-focus-viewer-layout
plan: 06
subsystem: ui
tags: [avalonia, teardown, imgui, persistence, embedded]

# Dependency graph
requires:
  - phase: 44-embedded-resize-focus-viewer-layout
    provides: Diagnosed destroyed-parent and embedded-layout UAT gaps, plus the existing embedded layout persistence seam
provides:
  - Graceful embedded teardown waiting before host fallback kill
  - Explicit resolved path contract for the embedded `imgui.embedded.ini` store
  - Manual checklist commands that target the real embedded store location
affects: [44-UAT, phase-44-verification, phase-45]

# Tech tracking
tech-stack:
  added: []
  patterns: [graceful-wait-then-kill teardown policy, resolved embedded ini paths, checklist verification bound to runtime store location]

key-files:
  created: []
  modified:
    - samples/avalonia-host/MainWindow.axaml.cs
    - src/imgui_storage.h
    - .planning/phases/44-embedded-resize-focus-viewer-layout/44-MANUAL-CHECKLIST.md

key-decisions:
  - "Destroyed-parent and host-close teardown now prefer mdCAD self-exit and only fall back to `Process.Kill(true)` on verified failure."
  - "The embedded `imgui.embedded.ini` store remains a separate file, but code now resolves its desktop runtime path explicitly."
  - "Scenario 7 verification now resets and inspects the real embedded store path under `build-vulkan\\bin\\Release`."

patterns-established:
  - "Pattern 1: Use a graceful process-exit wait before fallback kill when embedded teardown needs mdCAD cleanup to run."
  - "Pattern 2: Bind manual verification steps to the actual runtime artifact path, not an assumed repo-root filename."

requirements-completed: [EMBD-04, INPT-04]

# Metrics
duration: continued-session
completed: 2026-05-14
---

# Phase 44 Plan 06: Embedded Teardown and Persistence Gap Summary

**The host now gives mdCAD time to self-exit and flush `imgui.embedded.ini`, and Scenario 7 verifies the real embedded store path**

## Performance

- **Duration:** continued from the Phase 44 gap-closure session
- **Started:** after `44-05` execution
- **Completed:** 2026-05-14T20:58:50+01:00
- **Tasks:** 2
- **Files modified:** 3

## Accomplishments
- Replaced the fixed destroyed-parent teardown race with a graceful wait that prefers mdCAD self-exit before fallback cleanup.
- Changed normal host-close teardown to wait for mdCAD long enough to flush the embedded layout store before resorting to `Kill(true)`.
- Made the embedded store path explicit in code and rebound Scenario 7 to `build-vulkan\bin\Release\imgui.embedded.ini`.

## Task Commits

The two planned tasks landed together in one atomic gap-fix commit because they shared the same teardown/persistence contract:

1. **Task 1: Replace fixed teardown races with deterministic graceful-exit waiting in the Avalonia host** - `d532c29` (fix)
2. **Task 2: Make the embedded ini path explicit and bind Scenario 7 to the real store location** - `d532c29` (fix, shared atomic change set)

**Plan metadata:** pending phase-gap closeout docs commit

## Files Created/Modified
- `samples/avalonia-host/MainWindow.axaml.cs` - Adds graceful self-exit waiting for destroyed-parent and host-close teardown paths while preserving `destroy-after-attach` as the deliberate fallback scenario.
- `src/imgui_storage.h` - Resolves the embedded desktop store to an explicit runtime path and exposes that contract through the native manual-store helpers.
- `.planning/phases/44-embedded-resize-focus-viewer-layout/44-MANUAL-CHECKLIST.md` - Updates Scenario 7 to reset and inspect the real embedded store location and notes the graceful host-close flush expectation.

## Decisions Made
- Preserved `destroy-after-attach` as the forced fallback cleanup scenario while making destroyed-parent and normal close prefer mdCAD self-exit.
- Kept `imgui.embedded.ini` as the embedded store filename instead of introducing a new naming convention just for the harness.
- Updated the checklist commands instead of moving the store location to repo root, keeping runtime behavior and verification aligned.

## Deviations from Plan

None - both tasks were delivered together because the teardown and embedded-store-path fixes share the same lifecycle seam.

## Issues Encountered

None.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness
- Ready for the manual re-check of UAT Scenarios 5 and 7.
- Phase 44 gap execution is complete pending re-verification of the previously failing human-tested scenarios.

---
*Phase: 44-embedded-resize-focus-viewer-layout*
*Completed: 2026-05-14*
