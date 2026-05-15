---
phase: 46-launch-time-live-refresh
plan: 02
subsystem: app-runtime
tags: [jsonl, live-refresh, app, overlay, observer]

# Dependency graph
requires:
  - phase: 46-launch-time-live-refresh
    provides: Explicit startup live-refresh launch contract and controller opt-in bridge
provides:
  - Startup-root live-refresh status surfaced from `app.c`
  - Source-contract coverage for overlay observer-state queries and preserved frame ordering
  - MinGW-safe mdCAD app target include path for the active `build-vulkan` loop
affects: [46-03, phase-46-verification, startup-refresh-overlay]

# Tech tracking
tech-stack:
  added: []
  patterns: [startup-root observer overlay, app-owned status outside hidden panels, no second refresh loop]

key-files:
  created:
    - .planning/phases/46-launch-time-live-refresh/46-02-SUMMARY.md
  modified:
    - src/app.c
    - src/tests/startup_jsonl_app_contract_test.c
    - src/CMakeLists.txt

key-decisions:
  - "The startup overlay reads refresh-running state and warning/error messages from the imported root's existing `JsonlObserverComp` instead of introducing a startup-specific refresh loop."
  - "Startup refresh status remains visible outside `if (state.ui_visible)` so embedded viewer-first layouts still surface linked-refresh issues."
  - "The mdCAD app target now includes `src/` directly so MinGW can resolve sibling headers consistently during the active Vulkan build path."

patterns-established:
  - "Pattern 1: Startup launch overlays may inspect runtime component state, but the owning system loop remains the only place that advances work."
  - "Pattern 2: Source-contract tests should lock frame ordering before app overlays expand around existing runtime systems."

requirements-completed: []

# Metrics
duration: continued-session
completed: 2026-05-15
---

# Phase 46 Plan 02: App Wiring and Startup Refresh Overlay Summary

**`app.c` now surfaces startup-root live-refresh status from the existing observer runtime, keeps the original startup import progress/error behavior, and stays buildable on the MinGW Vulkan loop used for this phase**

## Performance

- **Duration:** continued from the same Phase 46 execution session after 46-01 landed
- **Started:** after the launch/controller contract was verified and the observer overlay seam was mapped
- **Completed:** 2026-05-15T11:06:35.8699974+01:00
- **Tasks:** 2
- **Files modified:** 3

## Accomplishments
- Extended `src/tests/startup_jsonl_app_contract_test.c` so the app contract now locks the `startup_jsonl_live_refresh` arm call, preserved observer tick ordering, and startup-root overlay queries through `ecs_world_get_jsonl_observer(...)` and `jsonl_observer_is_flat_refresh_running(...)`.
- Extended `src/app.c` so the `Startup JSONL Import` overlay keeps its Phase 45 progress/error behavior while also showing startup-root refresh-running state and latest observer warning/error messages.
- Kept the existing observer runtime authoritative by leaving `jsonl_observer_system_tick(...)` and `jsonl_observer_tick_flat_refreshes(...)` unchanged and avoiding any startup-specific refresh loop.
- Added `${CMAKE_CURRENT_SOURCE_DIR}` to the mdCAD app target in `src/CMakeLists.txt` so the active MinGW Vulkan build resolves sibling headers such as `app_launch_config.h` during full app builds.
- Verified the Phase 46 app slice by rebuilding `mdCAD` and running `startup_jsonl_app_contract_test`, `startup_jsonl_import_controller_test`, `embed_launch_config_test`, `jsonl_flat_observer_manual_refresh_test`, and `jsonl_flat_observer_auto_safety_test`.

## Task Commits

Each task was committed atomically where practical:

1. **Task 1: RED source-contract extension for startup observer overlay** - pending
2. **Task 2: `app.c` overlay integration and app-build unblock** - pending

## Files Created/Modified
- `src/tests/startup_jsonl_app_contract_test.c` - Locks startup live-refresh arm wiring, frame ordering, and startup-root observer overlay queries.
- `src/app.c` - Surfaces startup-root refresh-running and warning/error state alongside the existing startup import overlay.
- `src/CMakeLists.txt` - Adds `src/` to the mdCAD app target include path for the working MinGW build loop.

## Decisions Made
- Kept the overlay focused on startup-root observer running/warning/error state instead of showing a persistent “armed” banner after success.
- Treated the MinGW include-path failure as tightly coupled build verification debt and fixed it where the app target is defined rather than weakening the phase gate.

## Deviations from Plan

- None in feature scope. The only extra change was the app-target include-path fix needed to keep `mdCAD` buildable on the active MinGW toolchain.

## Issues Encountered

- The full app build exposed a MinGW-only include-path bug: `platform/win32_embed.h` includes `app_launch_config.h` as a sibling header, but the mdCAD target did not include `${CMAKE_CURRENT_SOURCE_DIR}` directly. This was fixed in `src/CMakeLists.txt`.

## User Setup Required

None - the startup live-refresh UI status is automatic once the user launches with `--jsonl-live-refresh`.

## Next Phase Readiness
- 46-03 can focus entirely on runtime regression proof because the launch parser, startup controller, and app-owned overlay seams are now all locked.
- Phase 46 is ready for closeout once the observer regression suite proves default-off safety and semantic reuse end-to-end.

---
*Phase: 46-launch-time-live-refresh*
*Completed: 2026-05-15*
