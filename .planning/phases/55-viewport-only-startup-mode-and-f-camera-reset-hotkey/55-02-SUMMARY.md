---
phase: 55-viewport-only-startup-mode-and-f-camera-reset-hotkey
plan: 02
subsystem: native-embedded-layout
tags: [embedded, viewport-only, imgui, dockbuilder, launch-config]

# Dependency graph
requires:
  - phase: 55-01
    provides: "Control-side viewport-only launch contract and Windows `--viewport-only` arg flow"
provides:
  - Native `--viewport-only` parser support
  - Dedicated persisted embedded viewport-only layout store
  - App-side viewport-only layout/draw contract locked by a new native test
affects: [phase-55, phase-55-03]

# Tech tracking
tech-stack:
  added: []
  patterns: [native launch flag parsing, dedicated embedded layout store selection, viewport-only ui-visible gating]

key-files:
  created:
    - .planning/phases/55-viewport-only-startup-mode-and-f-camera-reset-hotkey/55-02-SUMMARY.md
    - src/tests/viewport_only_app_contract_test.c
  modified:
    - src/app_launch_config.h
    - src/embed_layout_state.h
    - src/app.c
    - src/tests/embed_launch_config_test.c
    - src/tests/embed_layout_state_test.c
    - src/CMakeLists.txt
    - .planning/phases/55-viewport-only-startup-mode-and-f-camera-reset-hotkey/55-VALIDATION.md

key-decisions:
  - "Native launch parsing now accepts `--viewport-only` once and rejects duplicates without changing existing `--embedded`, `--parent-hwnd`, `--jsonl`, or `--jsonl-live-refresh` rules."
  - "Embedded viewport-only mode persists through a dedicated `imgui.embedded.viewport-only.ini` store instead of reusing the normal embedded layout store."
  - "Viewport-only startup hides non-viewport panels by reusing the existing `state.ui_visible` gate and seeds a single-window embedded dock layout on first run."

patterns-established:
  - "Pattern 1: Embedded layout policy chooses its manual store filename from launch-mode-specific reducers rather than hard-coding one embedded store path."
  - "Pattern 2: App-level viewport-only behavior is validated with source-contract tests that lock init, dock seeding, and draw-gate placement without redesigning rendering."

requirements-completed: [P55-02, P55-03]

# Metrics
duration: continued-session
completed: 2026-05-18
---

# Phase 55 Plan 02: Native Viewport-Only Layout Summary

**Wave 2 is complete: mdCAD now parses `--viewport-only`, uses a dedicated persisted embedded viewport-only layout store, and starts embedded viewport-only sessions with only the 3D viewport panel visible while keeping the existing render path intact.**

## Performance

- **Duration:** continued from the Phase 55 execution session
- **Completed:** 2026-05-18
- **Tasks:** 2
- **Files modified:** 8

## Accomplishments

- Added native `--viewport-only` parsing and duplicate-flag rejection to `mdcad_launch_config_parse(...)`.
- Extended embedded layout policy so viewport-only mode uses its own `imgui.embedded.viewport-only.ini` persistence file.
- Added `viewport_only_app_contract_test.c` and registered it in the native CMake test lane.
- Updated `app.c` to suppress non-viewport panels for embedded viewport-only startup while keeping `ui_viewport_draw(&state.viewport)` outside the UI gate.
- Added a one-window viewport-only embedded dock seed for first-run layout initialization.
- Marked validation rows `55-02-01` and `55-02-02` green.

## Task Commits

Each task was committed atomically:

1. **Task 1: Add the native viewport-only flag and dedicated layout-store policy** - `ca6ffde`, `9d4fd1d` (test -> feat)
2. **Task 2: Lock viewport-only app init/layout/draw behavior with a new native contract test** - `6fb68fb`, `9c189dd` (test -> feat)

## Files Created/Modified

- `src/app_launch_config.h` - adds the `viewport_only` launch flag to the native parser contract.
- `src/embed_layout_state.h` - selects a dedicated viewport-only embedded manual store filename and seed policy.
- `src/app.c` - applies viewport-only layout/store selection, one-window dock seeding, and the existing `ui_visible` gate for panel suppression.
- `src/tests/embed_launch_config_test.c` - locks parser success and duplicate-failure cases for `--viewport-only`.
- `src/tests/embed_layout_state_test.c` - locks the dedicated viewport-only embedded store behavior.
- `src/tests/viewport_only_app_contract_test.c` - locks init/layout/draw placement for viewport-only startup.
- `src/CMakeLists.txt` - registers the new viewport-only app contract test target.
- `.planning/phases/55-viewport-only-startup-mode-and-f-camera-reset-hotkey/55-VALIDATION.md` - rows `55-02-01` and `55-02-02` are green.

## Decisions Made

- Kept `--viewport-only` parse support generic in the native config layer while scoping the actual behavior to embedded startup paths.
- Used a dedicated viewport-only embedded store file instead of reseeding the normal embedded store every launch.
- Reused the existing `state.ui_visible` gate and dock builder seams rather than inventing a new viewport-only render architecture.

## Deviations from Plan

- None - plan executed exactly as written.

## Issues Encountered

- None.

## User Setup Required

- None.

## Next Phase Readiness

- `55-03` can now add the `F` camera reset shortcut and close the validation lane against a stable viewport-only launch/layout contract.

---
*Phase: 55-viewport-only-startup-mode-and-f-camera-reset-hotkey*
*Completed: 2026-05-18*
