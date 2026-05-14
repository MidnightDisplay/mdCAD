---
phase: 44-embedded-resize-focus-viewer-layout
plan: 03
subsystem: ui
tags: [avalonia, hwnd, resize, orphan-teardown, host-harness]

# Dependency graph
requires:
  - phase: 43-embed-contract-child-window-bootstrap
    provides: Minimal Avalonia host scaffold, child attach polling, and the strict embedded launch contract
provides:
  - Deterministic post-attach placeholder teardown modes in the sample host
  - Host-side resize/orphan monitoring that preserves valid-path `MoveWindow(...)` sync
  - A bound Phase 44 checklist with exact harness commands and teardown expectations
affects: [44-04, phase-45, phase-47, manual-verification]

# Tech tracking
tech-stack:
  added: []
  patterns: [mode-driven host harness lifecycle tests, explicit teardown status messaging, checklist-bound host commands]

key-files:
  created: []
  modified:
    - samples/avalonia-host/MainWindow.axaml.cs
    - .planning/phases/44-embedded-resize-focus-viewer-layout/44-MANUAL-CHECKLIST.md

key-decisions:
  - "Kept `MoveWindow(...)` as the host-owned resize authority for the valid path instead of introducing any input or layout proxy layer."
  - "Used separate `destroyed-parent` and `destroy-after-attach` modes so the harness can distinguish mdCAD self-exit from host fallback cleanup."
  - "Bound the manual checklist to exact `dotnet run` commands and teardown evidence so orphan handling is not left to ad hoc testing."

patterns-established:
  - "Pattern 1: Encode teardown scenarios as explicit host CLI modes instead of one-off manual mutations."
  - "Pattern 2: Keep teardown evidence in the host status area so mdCAD-side exit and host fallback cleanup are easy to tell apart."
  - "Pattern 3: Preserve the valid attached resize loop while hardening invalid-handle cleanup around it."

requirements-completed: [EMBD-04, INPT-01]

# Metrics
duration: continued-session
completed: 2026-05-14
---

# Phase 44 Plan 03: Host Resize and Orphan Proof Summary

**Deterministic Avalonia teardown modes, resize-safe child HWND monitoring, and a concrete host checklist for embedded lifecycle proof**

## Performance

- **Duration:** continued from the active Phase 44 execution session
- **Started:** continued from the 44-02 runtime handoff
- **Completed:** 2026-05-14T18:49:59+01:00
- **Tasks:** 2
- **Files modified:** 2

## Accomplishments
- Added `destroy-after-attach` and hardened `destroyed-parent` behavior so the host can reproduce post-attach teardown without closing the full window.
- Preserved valid-path `MoveWindow(...)` resize sync while stopping the lifecycle timers and cleaning up `mdCAD.exe` whenever the placeholder or child HWND becomes invalid.
- Bound the Phase 44 checklist to exact harness commands, teardown expectations, resize stress coverage, and embedded layout isolation evidence.

## Task Commits

Each task was committed atomically:

1. **Task 1: Add deterministic placeholder teardown and orphan-kill handling to the Avalonia host** - `ddbb425` (fix)
2. **Task 2: Bind the manual checklist to the real Phase 44 host harness** - `37baae3` (docs)

**Plan metadata:** pending phase-closeout docs commit

## Files Created/Modified
- `samples/avalonia-host/MainWindow.axaml.cs` - Adds the new harness mode, post-attach placeholder destroy flow, teardown-status handling, and invalid-handle cleanup around resize sync.
- `.planning/phases/44-embedded-resize-focus-viewer-layout/44-MANUAL-CHECKLIST.md` - Replaces the shell wording with exact commands and teardown/layout expectations for the host harness.

## Decisions Made
- Kept the teardown proof inside the sample host instead of inventing any new mdCAD/host IPC or input proxy surface.
- Allowed a short grace window in `destroyed-parent` mode so the host can observe the mdCAD-side invalid-parent quit path before falling back to host cleanup.
- Expanded the checklist to include embedded layout isolation because INPT-04 still requires live confirmation across embedded and standalone launches.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness
- Ready for `44-04`: the host now exposes deterministic teardown modes and the manual checklist already covers embedded-layout verification steps.
- No blockers identified for the remaining embedded layout persistence work.

---
*Phase: 44-embedded-resize-focus-viewer-layout*
*Completed: 2026-05-14*
