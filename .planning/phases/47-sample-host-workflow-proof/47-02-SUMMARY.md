---
phase: 47-sample-host-workflow-proof
plan: 02
subsystem: avalonia-host
tags: [host, avalonia, lifecycle, relaunch, embedded]

# Dependency graph
requires:
  - phase: 47-sample-host-workflow-proof
    provides: Bundled example launch proof, restored MSVC Vulkan validation lane, and the existing embedded teardown/focus seams
provides:
  - Explicit host controls and status surfaces for repeated embedded launch, close, and relaunch
  - Placeholder recreation and session-state reset between embedded runs
  - A manual proof checklist bound to the MSVC Vulkan validation loop
affects: [phase-47-closeout, host-lifecycle-proof, manual-uat]

# Tech tracking
tech-stack:
  added: []
  patterns: [host-owned status surfaces, placeholder recreation before relaunch, shared win32 embed test stub for standalone MSVC tests]

key-files:
  created:
    - .planning/phases/47-sample-host-workflow-proof/47-MANUAL-CHECKLIST.md
    - .planning/phases/47-sample-host-workflow-proof/47-02-SUMMARY.md
  modified:
    - samples/avalonia-host/MainWindow.axaml
    - samples/avalonia-host/MainWindow.axaml.cs
    - src/tests/endpoint_pick_test.c
    - src/tests/jsonl_flat_observer_auto_safety_test.c
    - src/tests/jsonl_flat_observer_manual_refresh_test.c
    - src/tests/jsonl_reparse_transaction_test.c
    - src/tests/jsonl_sketch_import_test.c
    - src/tests/jsonl_sketch_mapping_test.c
    - src/tests/scene_solver_contract_test.c
    - src/tests/scene_solver_diagnostics_test.c
    - src/tests/scene_solver_drag_test.c
    - src/tests/scene_solver_pass_policy_test.c
    - src/tests/scene_solver_trigger_test.c
    - src/tests/script_roundtrip_tests.c

key-decisions:
  - "The sample host remains a workflow harness: it reports only launch, attach, JSONL-request, and live-refresh-request state that it actually owns, then switches to `viewer-managed` wording after attach."
  - "Relaunch reuses the existing graceful teardown seam and recreates the native placeholder surface before the next embedded session starts."
  - "The restored MSVC full-suite link failure is solved by reusing the existing `win32_embed_test_stub.h` in standalone test entry points instead of introducing a new production-side symbol owner."

patterns-established:
  - "Pattern 1: Repeatable Win32 embedding harnesses should recreate their placeholder/native host surface after teardown before attempting a second child attach."
  - "Pattern 2: Standalone Windows tests that link `libsokol` outside `app.c` must define the shared embed-state symbol through `win32_embed_test_stub.h`."

requirements-completed: [HOST-03, HOST-04]

# Metrics
duration: continued-session
completed: 2026-05-15
---

# Phase 47 Plan 02: Repeated Host Lifecycle Proof Summary

**The Avalonia sample host now has explicit workflow controls and honest lifecycle status for repeated embedded launch/close/relaunch, and the Phase 47 proof path now includes an MSVC-bound manual checklist that passed across all five scenarios**

## Performance

- **Duration:** continued from the Phase 47 execution session after 47-01 completed
- **Started:** after bundled example launch proof landed
- **Completed:** 2026-05-15T12:11:18.6856201+01:00
- **Tasks:** 2
- **Files modified:** 14

## Accomplishments
- Updated `samples/avalonia-host/MainWindow.axaml` to add `Launch Session`, `Close Session`, and `Live refresh` controls plus separate `launch:`, `attach:`, `jsonl:`, and `live refresh:` status surfaces above the embed region.
- Refactored `samples/avalonia-host/MainWindow.axaml.cs` into an explicit session controller with button-driven launch/close, resettable process state, dynamic embed-surface recreation, and status helpers that preserve the exact resolved absolute example path before launch.
- Removed the old one-shot auto-launch flow so repeated launch/close/relaunch happens from the same host window using the existing graceful-teardown and fallback-cleanup seams.
- Kept the status boundary honest: the host reports only what it owns and switches JSONL/live-refresh lines to `viewer-managed after launch` wording instead of inventing import-confirmation IPC.
- Added `.planning/phases/47-sample-host-workflow-proof/47-MANUAL-CHECKLIST.md` with the five required scenarios: bundled example launch with live refresh off, bundled example launch with live refresh on, repeated relaunch workflow, orphan-process inspection, and bundled-example failure-path proof.
- Ran the full restored MSVC validation gate successfully: `cmake --build build-vulkan --config Release`, `ctest --test-dir build-vulkan -C Release --output-on-failure` with 24/24 passing, and `dotnet build samples/avalonia-host/AvaloniaHost.csproj -c Release`.
- Recorded a full **PASS** in `.planning/phases/47-sample-host-workflow-proof/47-MANUAL-CHECKLIST.md`, including repeated relaunch, focus/resize continuity, graceful cleanup, and orphan-free host close behavior.
- Fixed a validation-blocking MSVC link seam by adding `win32_embed_test_stub.h` to the standalone Windows tests that link `libsokol` without `app.c`, restoring full-suite linkability on the intended build lane.

## Task Commits

Each task is implemented and summarized, but the 47-02 work is not committed yet:

1. **Task 1: Add minimal session controls and refactor the host into a repeatable launch/close controller** - pending
2. **Task 2: Write the Phase 47 manual proof checklist and bind it to the MSVC Vulkan validation loop** - pending

## Files Created/Modified
- `samples/avalonia-host/MainWindow.axaml` - Adds the workflow controls, split status text, and replaceable embed-surface host.
- `samples/avalonia-host/MainWindow.axaml.cs` - Owns the repeated launch/close/relaunch controller, placeholder recreation, and host-owned lifecycle messaging.
- `.planning/phases/47-sample-host-workflow-proof/47-MANUAL-CHECKLIST.md` - Defines the five required manual scenarios on the MSVC Vulkan workflow.
- `src/tests/*.c` (standalone Windows test entry points listed above) - Include `win32_embed_test_stub.h` so the restored MSVC full suite links cleanly with the shared embed-state symbol expected by `libsokol`.

## Decisions Made
- Kept Phase 47 strictly within host-owned workflow/status proof; no IPC, no host-side import-success claims, and no change to the embedded focus ownership rules proven in Phase 44.
- Reused the existing teardown/wait/fallback path and recreated the placeholder surface after cleanup instead of inventing a second relaunch-specific embedding path.
- Treated the MSVC link failure as a validation-lane regression and fixed it at the shared standalone-test seam rather than weakening the build gate or reverting to the temporary MinGW path.

## Deviations from Plan

- No host-behavior deviation. The only extra work beyond the planned host/controller/checklist scope was a small test-entry-point include sweep required to make the restored MSVC full suite link successfully.

## Issues Encountered

- The first full `cmake --build build-vulkan --config Release` pass on the restored MSVC lane failed because several standalone test executables linked `libsokol` without defining `g_mdcad_win32_embed_state`. Reusing the existing `win32_embed_test_stub.h` in those test entry points resolved the issue.

## User Setup Required

None - the checklist proof is complete and the sample host now ships the bundled example plus the repeatable lifecycle controls needed for local demonstration.

## Next Phase Readiness
- Ready for Phase 47 closeout and commit finalization.
- Ready for milestone v1.8 completion/archival whenever you want to close the milestone formally.

---
*Phase: 47-sample-host-workflow-proof*
*Completed: 2026-05-15*
