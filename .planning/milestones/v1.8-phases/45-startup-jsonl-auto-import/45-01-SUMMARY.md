---
phase: 45-startup-jsonl-auto-import
plan: 01
subsystem: launch-runtime
tags: [jsonl, startup, cli, controller]

# Dependency graph
requires:
  - phase: 45-startup-jsonl-auto-import
    provides: Startup JSONL research, validation, and the Phase 44 embedded runtime foundation
provides:
  - Absolute `--jsonl` launch parsing alongside the embedded HWND contract
  - Reusable non-UI startup JSONL import controller
  - Controller-owned runtime error latching for bad startup files
affects: [45-02, launch-contract, startup-import]

# Tech tracking
tech-stack:
  added: []
  patterns: [absolute launch-path validation, controller-owned startup import orchestration, runtime error latching outside launch parse]

key-files:
  created:
    - .planning/phases/45-startup-jsonl-auto-import/45-01-SUMMARY.md
  modified:
    - src/app_launch_config.h
    - src/startup_jsonl_import_controller.h
    - src/tests/startup_jsonl_import_controller_test.c

key-decisions:
  - "The `--jsonl` CLI contract accepts exactly one absolute path and deliberately avoids probing the filesystem at parse time."
  - "Startup JSONL import reuses `jsonl_import_job_t` with the proven flat-import defaults instead of creating a second importer."
  - "Missing or unreadable startup JSONL becomes a controller-owned runtime error after launch rather than a fatal argument-parse failure."

patterns-established:
  - "Pattern 1: Keep launch parsing focused on syntax and shape validation; leave file-open failure to the runtime controller."
  - "Pattern 2: Bridge launch config to long-running import work through a small app-owned controller, not through Scene Hierarchy UI state."

requirements-completed: []

# Metrics
duration: continued-session
completed: 2026-05-15
---

# Phase 45 Plan 01: Startup JSONL Launch Contract Summary

**mdCAD now accepts a validated absolute startup JSONL path and has a reusable controller that can drive the existing flat import job without depending on Scene Hierarchy visibility**

## Performance

- **Duration:** continued from the Phase 45 execution session after the RED scaffold landed
- **Started:** after `dd99b2a` established the failing parser/controller contracts
- **Completed:** 2026-05-15T09:44:59.1592246+01:00
- **Tasks:** 2
- **Files modified:** 3

## Accomplishments
- Extended `src/app_launch_config.h` so `--jsonl <absolute-path>` parses once, rejects duplicates/missing values/relative paths/truncation, and preserves the existing embedded launch contract plus unknown-flag tolerance.
- Replaced the `src/startup_jsonl_import_controller.h` scaffold with a working header-only controller that arms, starts, ticks, completes, resets, and latches runtime failures around `jsonl_import_job_t`.
- Locked the controller defaults to the proven flat-import path: scale `1.0f`, JSONL colours on, white fallback colour, no CoM shift, zero rotation, mesh mode `0`, and observer metadata captured with `link_enabled=false`.
- Kept tiny startup imports synchronous via `jsonl_import_job_should_sync()` while leaving larger imports ready for one-tick-per-frame ownership in `app.c`.
- Fixed the new controller contract test to initialize Sokol time before exercising timing-backed import-job ticks.

## Task Commits

Each task was committed atomically where practical:

1. **Task 1: RED parser/controller contract scaffold** - `dd99b2a`
2. **Task 2: Implement parser/controller and close the plan** - `e2fb543`

## Files Created/Modified
- `src/app_launch_config.h` - Adds absolute `--jsonl` launch parsing and fixed-size startup path storage.
- `src/startup_jsonl_import_controller.h` - Implements the non-UI startup import ownership seam around `jsonl_import_job_t`.
- `src/tests/startup_jsonl_import_controller_test.c` - Verifies controller defaults, runtime error latching, reset behavior, and now initializes Sokol time explicitly.

## Decisions Made
- Kept filesystem existence checks out of CLI parsing so a bad launch path still produces a usable viewer session once `app.c` owns the runtime surface.
- Reused the existing JSONL flat-import job instead of copying Scene Hierarchy import code into a second startup-specific workflow.

## Deviations from Plan

- None.

## Issues Encountered

- The first green attempt still failed under CTest because the new controller test used timing-backed import-job ticks without calling `stm_setup()`. Adding the same time initialization used by the other JSONL job tests resolved the harness failure.

## User Setup Required

None - startup JSONL remains an optional CLI flag and no new external configuration is required.

## Next Phase Readiness
- Plan 45-02 can now integrate startup import into `app.c` without touching the parser/controller contract again.
- Remaining work is limited to app ownership, overlay/error surfacing, and cache-dirty integration outside Scene Hierarchy visibility.

---
*Phase: 45-startup-jsonl-auto-import*
*Completed: 2026-05-15*
