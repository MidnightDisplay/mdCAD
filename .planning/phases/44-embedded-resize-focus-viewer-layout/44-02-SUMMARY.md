---
phase: 44-embedded-resize-focus-viewer-layout
plan: 02
subsystem: runtime
tags: [embedded, win32, sokol, focus, capture, gizmo]

# Dependency graph
requires:
  - phase: 43-embed-contract-child-window-bootstrap
    provides: Strict child-HWND launch parsing, Win32 child-window bootstrap, and the baseline Avalonia host needed to exercise embedded runtime behavior
provides:
  - First-click embedded keyboard ownership backed by the shared reducer
  - Capture-loss and deactivate cancellation across the patched Win32 Sokol seam
  - mdCAD-side invalid-parent shutdown with gizmo/orbit rollback on embedded teardown
affects: [44-03, 44-04, phase-45, embedded-runtime]

# Tech tracking
tech-stack:
  added: []
  patterns: [embedded reducer-driven shortcut gating, runtime invalid-parent quit guard, capture-loss rollback through shared cancel helpers]

key-files:
  created: []
  modified:
    - src/app.c
    - src/orbit_camera.h
    - src/platform/win32_embed.h
    - vendors/libsokol/patches/0001-win32-embed-child-window-bootstrap.patch

key-decisions:
  - "Embedded attach starts with keyboard ownership disabled and only promotes mdCAD shortcuts after a deliberate click inside the child HWND."
  - "Invalid or ended parent HWNDs are treated as an immediate mdCAD-side quit condition instead of waiting for the host to kill the process."
  - "Capture loss, deactivate, and unfocus all flow through a shared cancel path so orbit and gizmo drags never leave partial state behind."

patterns-established:
  - "Pattern 1: Drive embedded focus ownership through the pure reducer before checking shortcut keys."
  - "Pattern 2: Patch the Win32 seam to emit project-owned cancel signals instead of duplicating host-specific logic."
  - "Pattern 3: Reuse drag-start snapshots for rollback on cancel rather than pushing undo records for aborted drags."

requirements-completed: [EMBD-04, INPT-02, INPT-03]

# Metrics
duration: continued-session
completed: 2026-05-14
---

# Phase 44 Plan 02: Runtime Focus, Capture, and Orphan Teardown Summary

**First-click embedded keyboard ownership, capture-loss cancellation, and mdCAD-side invalid-parent quit wiring in the Win32 runtime**

## Performance

- **Duration:** continued from the active Phase 44 execution session
- **Started:** continued from the 44-01 handoff
- **Completed:** 2026-05-14T18:40:23+01:00
- **Tasks:** 2
- **Files modified:** 4

## Accomplishments
- Gated embedded shortcuts behind deliberate first-click ownership instead of letting attach/focus hand mdCAD the keyboard immediately.
- Patched the Win32 Sokol seam to clear capture bookkeeping on `WM_CAPTURECHANGED`, `WM_CANCELMODE`, `WM_ACTIVATEAPP(FALSE)`, and `WM_KILLFOCUS`.
- Added a shared cancel/rollback path so embedded deactivate, unfocus, or orphan teardown reverts active orbit and gizmo interactions cleanly.

## Task Commits

The two planned tasks landed together in one atomic runtime commit because they shared the same `app.c` and Win32 lifecycle seam:

1. **Task 1: Fix the embedded focus/capture seam and child-side invalid-parent shutdown without host-side proxy input** - `dc46516` (fix)
2. **Task 2: Add explicit orbit/gizmo cancel and rollback on embedded deactivate** - `dc46516` (fix, shared atomic runtime change set)

**Plan metadata:** pending phase-closeout docs commit

## Files Created/Modified
- `src/app.c` - Adds embedded reducer state, shortcut gating, invalid-parent quit guard, and cancel/rollback handling.
- `src/orbit_camera.h` - Exposes `orbit_camera_cancel_interaction(...)` for embedded deactivate/cancel paths.
- `src/platform/win32_embed.h` - Tracks runtime cancel generations and validates the live parent/child HWND chain.
- `vendors/libsokol/patches/0001-win32-embed-child-window-bootstrap.patch` - Regenerates the checked-in Sokol patch with the capture-loss and deactivate hooks.

## Decisions Made
- Used the mdCAD runtime, not the host harness, as the source of truth for parent-invalid shutdown so embedded teardown does not depend on `Process.Kill(true)` alone.
- Reused the pure `embed_input_state` reducer for keyboard ownership decisions instead of adding special-case shortcut checks throughout the event loop.
- Kept cancel handling distinct from normal mouse-up commit so aborted drags never create undo entries or partial scene mutations.

## Deviations from Plan

None - both planned tasks were delivered together because the focus/capture and rollback changes shared the same runtime seam.

## Issues Encountered
- The stored Sokol patch first failed because of malformed hand-edited hunks and then because a regenerated patch file was written with a BOM. Regenerating the patch from a clean upstream diff and rewriting it as ASCII/no-BOM fixed `git apply` and the live build.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness
- Ready for `44-03`: the Avalonia harness can now distinguish mdCAD-side parent-ended shutdown from host fallback cleanup against a stable native runtime.
- Ready for `44-04`: embedded layout persistence can now build on top of the finalized focus/cancel lifecycle.

---
*Phase: 44-embedded-resize-focus-viewer-layout*
*Completed: 2026-05-14*
