---
phase: 43-embed-contract-child-window-bootstrap
plan: 01
subsystem: infra
tags: [sokol, win32, avalonia, cli, embedding]

# Dependency graph
requires: []
provides:
  - Pinned Sokol to a reproducible Windows baseline with a repo-hosted embed patch seam
  - Added strict `--embedded --parent-hwnd` parsing with a dedicated CTest target
  - Created a buildable Avalonia `NativeControlHost` scaffold and Phase 43 smoke checklist shell
affects: [43-02, 43-03, phase-44]

# Tech tracking
tech-stack:
  added: [Avalonia 11.3.*, NuGet.Config source override]
  patterns: [header-only launch parser, repo-hosted vendor patch seam, NativeControlHost placeholder HWND scaffold]

key-files:
  created:
    - src/app_launch_config.h
    - src/tests/embed_launch_config_test.c
    - samples/avalonia-host/AvaloniaHost.csproj
    - samples/avalonia-host/MainWindow.axaml.cs
    - .planning/phases/43-embed-contract-child-window-bootstrap/43-MANUAL-CHECKLIST.md
  modified:
    - vendors/libsokol/CMakeLists.txt
    - src/CMakeLists.txt
    - src/app.c

key-decisions:
  - "Pinned Sokol to commit `2356d22badb8b02b5b4fc745216023a2a699d840` before any Win32 embed patching."
  - "Scoped the Avalonia host to a Wave 0 scaffold with a real placeholder HWND and deferred full launch/attach flow to 43-03."

patterns-established:
  - "Pattern 1: Fail embedded startup before window creation by parsing launch flags in `sokol_main()`."
  - "Pattern 2: Keep Sokol customization reproducible through a checked-in patch seam instead of editing build-tree outputs."
  - "Pattern 3: Build the host harness around `NativeControlHost` plus a Win32 placeholder child window."

requirements-completed: [EMBD-01, EMBD-03, HOST-01]

# Metrics
duration: 7 min
completed: 2026-05-14
---

# Phase 43 Plan 01: Bootstrap Foundation Summary

**Pinned Sokol to a reproducible Windows baseline, added strict embed-flag parsing, and created a buildable Avalonia host scaffold with a `NativeControlHost` placeholder HWND.**

## Performance

- **Duration:** 7 min
- **Started:** 2026-05-14T15:04:11+01:00
- **Completed:** 2026-05-14T15:11:28+01:00
- **Tasks:** 3
- **Files modified:** 13

## Accomplishments
- Locked the Sokol fetch to a known Windows commit and added the checked-in embed patch hook plus `src` include path for later Win32 glue.
- Added `src/app_launch_config.h`, the new `embed_launch_config_test`, and pre-window fail-fast validation in `sokol_main()`.
- Created the initial Avalonia host shell, placeholder HWND control, and the three-scenario manual checklist template for later attach/failure proof.

## Task Commits

Each task was committed atomically:

1. **Task 1: Pin Sokol and lock the repo-hosted Win32 patch seam** - `96085bc` (chore)
2. **Task 2: Add and wire the strict embed launch parser with CTest coverage** - `0c58450` (feat)
3. **Task 3: Front-load the sample-host proof harness scaffold for Wave 0 feedback** - `03e3cf8` (feat)

**Plan metadata:** pending closeout docs commit

## Files Created/Modified
- `vendors/libsokol/CMakeLists.txt` - Pins Sokol to the known Windows baseline and adds the repo-hosted embed patch hook.
- `src/app_launch_config.h` - Header-only parser for `--embedded` and `--parent-hwnd`.
- `src/tests/embed_launch_config_test.c` - Fast CLI-contract regression coverage for valid and invalid embedded launch arguments.
- `src/CMakeLists.txt` - Registers `embed_launch_config_test` with CTest.
- `src/app.c` - Fails fast in `sokol_main()` when the embed launch contract is invalid.
- `samples/avalonia-host/AvaloniaHost.csproj` - Minimal Windows Avalonia sample host project.
- `samples/avalonia-host/MainWindow.axaml` - Minimal host layout with a `NativeControlHost` region and status text.
- `samples/avalonia-host/MainWindow.axaml.cs` - Wave 0 CLI parsing, embed test modes, and placeholder HWND scaffold.
- `samples/avalonia-host/NuGet.Config` - Host-local NuGet source override to avoid the blocked machine-level private feed.
- `.planning/phases/43-embed-contract-child-window-bootstrap/43-MANUAL-CHECKLIST.md` - Phase 43 valid/invalid/destroyed parent-HWND checklist shell.

## Decisions Made
- Pinned Sokol before any Win32 embed work so the later patch targets a stable upstream baseline.
- Kept the new launch parser header-only so the native app and dedicated test binary can share one source of truth.
- Added a host-local `NuGet.Config` because the inherited `ProGet` source returned 403 and blocked a reproducible Avalonia restore on this machine.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Added a host-local NuGet source override**
- **Found during:** Task 3 (Front-load the sample-host proof harness scaffold for Wave 0 feedback)
- **Issue:** `dotnet build` for the new Avalonia sample failed because an inherited machine-level `ProGet` source returned 403 for public Avalonia packages.
- **Fix:** Added `samples/avalonia-host/NuGet.Config` with `nuget.org` as the only package source for the sample host.
- **Files modified:** `samples/avalonia-host/NuGet.Config`
- **Verification:** `dotnet build samples/avalonia-host/AvaloniaHost.csproj -c Release`
- **Committed in:** `03e3cf8` (Task 3 commit)

---

**Total deviations:** 1 auto-fixed (1 blocking)
**Impact on plan:** The override was necessary to make the planned Wave 0 host scaffold build reproducibly in this environment. No feature scope changed.

## Issues Encountered
- The first Avalonia host build surfaced a C# accessibility mismatch and a missing public constructor warning in the scaffold; both were corrected before the Task 3 commit.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness
- Ready for `43-02`: the pinned Sokol baseline, parser CTest, and repo-hosted patch seam are in place.
- Ready for `43-03`: the Avalonia host project, placeholder HWND control, and manual checklist shell now exist for the later attach/failure proof pass.
- Requirement checkboxes in `.planning/REQUIREMENTS.md` remain phase-pending until the shared Phase 43 requirements are fully closed across plans `43-02` and `43-03`.

---
*Phase: 43-embed-contract-child-window-bootstrap*
*Completed: 2026-05-14*
