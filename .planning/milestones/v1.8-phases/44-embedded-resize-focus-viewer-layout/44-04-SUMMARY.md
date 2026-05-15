---
phase: 44-embedded-resize-focus-viewer-layout
plan: 04
subsystem: ui
tags: [imgui, dockbuilder, persistence, embedded, layout]

# Dependency graph
requires:
  - phase: 43-embed-contract-child-window-bootstrap
    provides: Embedded launch mode, child-window bootstrap, and the baseline embedded viewer defaults that layout work builds on
provides:
  - Separate embedded ImGui persistence via `imgui.embedded.ini`
  - One-time viewer-first DockBuilder seeding for first embedded run
  - Embedded layout persistence that leaves standalone `imgui.ini` behavior unchanged
affects: [phase-45, phase-47, embedded-layout, manual-verification]

# Tech tracking
tech-stack:
  added: []
  patterns: [policy-driven persistence selection, manual native desktop ImGui storage, one-shot DockBuilder bootstrapping]

key-files:
  created: []
  modified:
    - src/app.c
    - src/imgui_storage.h

key-decisions:
  - "Used `embed_layout_state.h` as the single source of truth for deciding between standalone automatic persistence and embedded manual persistence."
  - "Stored embedded layout state in `imgui.embedded.ini` alongside the existing native `imgui.ini` behavior instead of overloading one shared file."
  - "Seeded the embedded viewer layout once with DockBuilder only when the embedded store is absent or empty."

patterns-established:
  - "Pattern 1: Resolve embedded-vs-standalone layout policy before `simgui_setup()` and pass that policy through the shared storage seam."
  - "Pattern 2: Manual embedded persistence only runs on native desktop and preserves the existing iOS/Android/Web code paths."
  - "Pattern 3: Use DockBuilder for first-run hosted layout instead of frame-by-frame panel placement."

requirements-completed: [INPT-04]

# Metrics
duration: continued-session
completed: 2026-05-14
---

# Phase 44 Plan 04: Embedded Layout Persistence Summary

**Separate embedded ImGui storage with a one-time DockBuilder viewer layout that leaves standalone mdCAD layout state alone**

## Performance

- **Duration:** continued from the active Phase 44 execution session
- **Started:** continued from the 44-03 host harness handoff
- **Completed:** 2026-05-14T18:50:06+01:00
- **Tasks:** 2
- **Files modified:** 2

## Accomplishments
- Split embedded layout persistence from standalone mode by routing embedded launches through manual native storage backed by `imgui.embedded.ini`.
- Seeded the approved embedded viewer layout on first run with DockBuilder and kept Pick Buffer Debug, Slot Buffer Debug, and FPS Debug closed by default.
- Marked embedded layout changes for persistence on unfocus/shutdown so rearranged hosted panels survive relaunches without disturbing standalone `imgui.ini`.

## Task Commits

The two planned tasks landed together in one atomic layout commit because the persistence policy and first-run DockBuilder recipe shared the same `app.c` initialization seam:

1. **Task 1: Split embedded ImGui settings persistence from standalone layout state** - `7e042ab` (feat)
2. **Task 2: Seed the approved embedded DockBuilder recipe only on first embedded run** - `7e042ab` (feat, shared atomic layout change set)

**Plan metadata:** pending phase-closeout docs commit

## Files Created/Modified
- `src/app.c` - Resolves the embedded layout policy before `simgui_setup()`, disables shared automatic ini persistence for embedded mode, seeds the dock layout once, and saves embedded layout changes on unfocus.
- `src/imgui_storage.h` - Adds native desktop manual load/save support for the embedded-only store while preserving existing mobile and web persistence behavior.

## Decisions Made
- Left the existing standalone automatic `imgui.ini` path untouched and confined the new behavior to embedded launches only.
- Saved the seeded DockBuilder layout through the shared storage seam instead of hardcoding any runtime-only layout state.
- Kept the embedded debug-window defaults closed in code while still docking the windows so they remain available when opened later.

## Deviations from Plan

None - both planned tasks were delivered together because the persistence split and DockBuilder seed share the same startup/dockspace lifecycle.

## Issues Encountered

None.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness
- Phase 44 is ready for live manual verification across resize, focus, teardown, and embedded-layout isolation scenarios.
- Phase 45 can now add startup JSONL import on top of a stable embedded runtime, host harness, and layout baseline.

---
*Phase: 44-embedded-resize-focus-viewer-layout*
*Completed: 2026-05-14*
