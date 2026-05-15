---
phase: 46-launch-time-live-refresh
plan: 01
subsystem: launch-runtime
tags: [jsonl, live-refresh, cli, controller]

# Dependency graph
requires:
  - phase: 46-launch-time-live-refresh
    provides: Verified research and validation strategy for startup-linked live refresh
provides:
  - Explicit `--jsonl-live-refresh` launch contract
  - Startup controller opt-in bridge to existing observer metadata
  - Default-off protection for plain startup JSONL imports
affects: [46-02, 46-03, launch-contract, startup-refresh]

# Tech tracking
tech-stack:
  added: []
  patterns: [scoped companion CLI flag, controller-owned observer intent bridge, default-off startup refresh protection]

key-files:
  created:
    - .planning/phases/46-launch-time-live-refresh/46-01-SUMMARY.md
  modified:
    - src/app_launch_config.h
    - src/startup_jsonl_import_controller.h
    - src/app.c
    - src/tests/embed_launch_config_test.c
    - src/tests/startup_jsonl_import_controller_test.c

key-decisions:
  - "Startup live refresh is enabled only by the explicit `--jsonl-live-refresh` companion flag; plain `--jsonl` remains unlinked."
  - "The startup controller forwards only observer intent into `jsonl_import_job_set_observer_contract(...)`; it does not own a second refresh loop."
  - "The new launch flag requires `--jsonl` and remains scoped to startup JSONL instead of becoming a generic refresh switch."

patterns-established:
  - "Pattern 1: New launch-time behaviors should extend the existing startup controller seam rather than bypassing it in `app.c`."
  - "Pattern 2: Default-off behavior stays protected through explicit parser and controller contract tests before runtime integration grows."

requirements-completed: []

# Metrics
duration: continued-session
completed: 2026-05-15
---

# Phase 46 Plan 01: Startup Live-Refresh Launch Contract Summary

**mdCAD now has an explicit startup live-refresh launch bit, and the Phase 45 startup controller can forward that opt-in into the existing observer contract while keeping plain startup imports default-off**

## Performance

- **Duration:** continued from the Phase 46 execution session after the toolchain/test loop was restored
- **Started:** after Phase 46 planning completed and the parser/controller RED contracts were added
- **Completed:** 2026-05-15T10:30:15.0756771+01:00
- **Tasks:** 2
- **Files modified:** 4

## Accomplishments
- Extended `src/app_launch_config.h` with `startup_jsonl_live_refresh` and parsed `--jsonl-live-refresh` as a valueless companion flag that may appear once and requires `--jsonl`.
- Extended `src/tests/embed_launch_config_test.c` so the launch contract now covers standalone and embedded opt-in success, duplicate-flag rejection, and the `requires --jsonl` dependency.
- Extended `src/startup_jsonl_import_controller.h` so controller arm state captures `live_refresh_enabled` explicitly and forwards that bit through `jsonl_import_job_set_observer_contract(...)`.
- Applied the minimal `src/app.c` call-site signature alignment needed to keep the repo buildable while 46-02 is still pending.
- Extended `src/tests/startup_jsonl_import_controller_test.c` so default-off and opt-in observer-contract behavior are both locked on tiny startup imports.
- Kept the existing observer runtime untouched while proving the parser/controller contract alongside the existing observer regression tests.

## Task Commits

Each task was committed atomically where practical:

1. **Task 1: RED parser/controller live-refresh contracts** - pending
2. **Task 2: Implement launch flag and controller opt-in bridge** - pending

## Files Created/Modified
- `src/app_launch_config.h` - Adds `startup_jsonl_live_refresh` and enforces the companion-flag dependency.
- `src/startup_jsonl_import_controller.h` - Stores explicit live-refresh intent and forwards it into the observer contract.
- `src/app.c` - Passes the new launch-config bool into the startup controller with no runtime-behavior expansion yet.
- `src/tests/embed_launch_config_test.c` - Verifies success/failure/default-off parser behavior for `--jsonl-live-refresh`.
- `src/tests/startup_jsonl_import_controller_test.c` - Verifies false/true observer-contract bridging on startup import.

## Decisions Made
- Kept the launch flag narrow and startup-specific instead of introducing a broader `--live-refresh` flag that would imply unsupported scope.
- Limited controller changes to observer-intent capture only, deferring app wiring and overlay growth to 46-02.

## Deviations from Plan

- None in phase scope. A separate compile-blocker fix was required to restore the MinGW-based `build-vulkan` loop on this machine.

## Issues Encountered

- The original `build-vulkan` Visual Studio generator became unusable because the installed VS 2026 instance is incomplete on this machine. Phase execution switched the generated build tree to the available MinGW Vulkan toolchain and uncovered a declaration-order issue in `src/ecs/ecs_scene.h`, which was fixed separately in `0d38608`.

## User Setup Required

None - the new startup live-refresh behavior is still fully opt-in and layered on top of the existing executable.

## Next Phase Readiness
- 46-02 can now wire the new launch bit through `app.c` and extend the startup overlay with observer-running/warning state.
- 46-03 can focus on proving default-off safety and semantic reuse at runtime without revisiting the CLI contract.

---
*Phase: 46-launch-time-live-refresh*
*Completed: 2026-05-15*
