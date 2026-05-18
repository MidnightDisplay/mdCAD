---
phase: 55-viewport-only-startup-mode-and-f-camera-reset-hotkey
plan: 03
subsystem: embedded-shortcuts
tags: [embedded, shortcut, camera, input, runtime-refresh]

# Dependency graph
requires:
  - phase: 55-02
    provides: "Native viewport-only launch parsing, layout persistence, and app contract coverage"
provides:
  - `F` camera reset shortcut in standalone and embedded paths
  - Closed Phase 55 validation ledger
  - Refreshed committed Windows runtime bundle matching the new native executable
affects: [phase-55]

# Tech tracking
tech-stack:
  added: []
  patterns: [shortcut routing parity between embedded and standalone, runtime bundle refresh after native executable changes]

key-files:
  created:
    - .planning/phases/55-viewport-only-startup-mode-and-f-camera-reset-hotkey/55-03-SUMMARY.md
    - src/tests/camera_shortcut_app_contract_test.c
  modified:
    - src/app.c
    - src/CMakeLists.txt
    - samples/avalonia-mdcad-control/runtime/win-x64/mdCAD.exe
    - .planning/phases/55-viewport-only-startup-mode-and-f-camera-reset-hotkey/55-VALIDATION.md

key-decisions:
  - "The `F` shortcut is routed through the same embedded-vs-standalone shortcut split as the existing global shortcuts instead of reopening focus ownership work."
  - "Camera reset uses only `orbit_camera_reset(&state.camera)` and never writes camera fields directly inside the shortcut block."
  - "When native app code changes, the committed Windows runtime bundle must be refreshed so the host runtime proof lane remains truthful."

patterns-established:
  - "Pattern 1: New embedded global shortcuts must be added in both the routed embedded branch and the standalone raw-key branch."
  - "Pattern 2: Native executable changes that affect the embedded host path require a runtime-bundle refresh before the host integration lane can pass."

requirements-completed: [P55-04]

# Metrics
duration: continued-session
completed: 2026-05-18
---

# Phase 55 Plan 03: Camera Shortcut and Validation Closure Summary

**Wave 3 is complete: pressing `F` now resets the camera in both embedded and standalone mdCAD sessions through the existing shortcut seams, and the full Phase 55 build/test lane is green after refreshing the committed Windows runtime bundle.**

## Performance

- **Duration:** continued from the Phase 55 execution session
- **Completed:** 2026-05-18
- **Tasks:** 2
- **Files modified:** 6

## Accomplishments

- Added `ImGuiKey_F` camera reset handling in both the embedded routed shortcut branch and the standalone raw-key branch.
- Locked the shortcut placement with a new native `camera_shortcut_app_contract_test`.
- Ran the full Phase 55 proof lane across both Avalonia host builds, the control test project, and the relevant native contract tests.
- Refreshed the committed Windows runtime bundle so the host/runtime integration proof remains truthful after the native executable changed.
- Marked validation rows `55-03-01` and `55-03-02` green and closed `55-VALIDATION.md`.

## Task Commits

Each task was committed atomically:

1. **Task 1: Add `F` camera reset inside the existing standalone and embedded shortcut branches** - `3cf1a05`, `3fab0f0` (test -> feat)
2. **Task 2: Run the final Phase 55 proof lane and close the validation ledger** - `399e454` (chore)

## Files Created/Modified

- `src/app.c` - adds the `F` shortcut to the existing embedded and standalone shortcut branches using `orbit_camera_reset(&state.camera)`.
- `src/tests/camera_shortcut_app_contract_test.c` - locks the shortcut placement and absence of direct camera resets/focus APIs.
- `src/CMakeLists.txt` - registers the new camera shortcut contract test.
- `samples/avalonia-mdcad-control/runtime/win-x64/mdCAD.exe` - refreshed committed Windows runtime executable matching the current `build-vulkan` output.
- `.planning/phases/55-viewport-only-startup-mode-and-f-camera-reset-hotkey/55-VALIDATION.md` - final validation rows are green and the ledger is closed.

## Decisions Made

- Kept the `F` shortcut inside the existing `mdcad_embedded_shortcuts_allowed(io)` guard so text-entry/focus behavior remains governed by the Phase 44 input model.
- Reused `orbit_camera_reset(&state.camera)` rather than duplicating camera-reset field logic in `app.c`.
- Treated the runtime-bundle hash mismatch as a phase-owned blocking fix and refreshed the committed runtime instead of weakening the final proof lane.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Refreshed the committed Windows runtime bundle after the final lane exposed executable drift**
- **Found during:** Task 2 (Run the final Phase 55 proof lane and close the validation ledger)
- **Issue:** `RuntimeRefreshIntegrationTests.RuntimeRefresh_HostOutputExecutableMatchesBuildAndCommittedRuntime` failed because the committed `samples/avalonia-mdcad-control/runtime/win-x64/mdCAD.exe` no longer matched the rebuilt `build-vulkan` executable after the native `app.c` changes.
- **Fix:** Ran the existing `MdCad.WindowsRuntimeRefresh` helper against `build-vulkan` to rebuild `mdCAD.exe` and copy it into both the committed runtime bundle and the current host output runtime directory.
- **Files modified:** `samples/avalonia-mdcad-control/runtime/win-x64/mdCAD.exe`
- **Verification:** `dotnet build .\samples\avalonia-host-minimal\AvaloniaHostMinimal.csproj -c Release && dotnet build .\samples\avalonia-host\AvaloniaHost.csproj -c Release && dotnet test .\samples\avalonia-mdcad-control.tests\MdCad.Avalonia.Control.Tests.csproj -c Release --filter "FullyQualifiedName~MdCad" && dotnet test .\samples\avalonia-mdcad-control.tests\MdCad.Avalonia.Control.Tests.csproj -c Release --filter "FullyQualifiedName~WindowsMdCadEmbedBackendTests" && ctest --test-dir build-vulkan -C Release -R embed_launch_config_test --output-on-failure && ctest --test-dir build-vulkan -C Release -R embed_layout_state_test --output-on-failure && ctest --test-dir build-vulkan -C Release -R viewport_only_app_contract_test --output-on-failure && ctest --test-dir build-vulkan -C Release -R camera_shortcut_app_contract_test --output-on-failure`
- **Committed in:** `399e454`

---

**Total deviations:** 1 auto-fixed (1 blocking)
**Impact on plan:** The refresh was required to keep the existing host/runtime proof surface honest after native executable changes. No scope creep.

## Issues Encountered

- None.

## User Setup Required

- None.

## Next Phase Readiness

- Phase 55 is fully executed and its validation ledger is closed.

---
*Phase: 55-viewport-only-startup-mode-and-f-camera-reset-hotkey*
*Completed: 2026-05-18*
