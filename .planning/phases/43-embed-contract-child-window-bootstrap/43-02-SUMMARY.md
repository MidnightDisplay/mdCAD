---
phase: 43-embed-contract-child-window-bootstrap
plan: 02
subsystem: infra
tags: [win32, sokol, child-window, startup, embedding]

# Dependency graph
requires:
  - phase: 43-01
    provides: Pinned Sokol baseline, embed launch parser, and Wave 0 host scaffold
provides:
  - Shared Win32 embed startup state accessible from both `app.c` and the patched Sokol window creation path
  - Sokol child-window bootstrap patch that creates mdCAD as `WS_CHILD` from birth
  - Embedded startup defaults that hide standalone-heavy chrome and keep Pick/Slot/FPS debug windows off
affects: [43-03, phase-44]

# Tech tracking
tech-stack:
  added: []
  patterns: [shared pre-window embed state, pinned vendor patch application, embedded viewer baseline defaults]

key-files:
  created:
    - src/platform/win32_embed.h
    - vendors/libsokol/patches/0001-win32-embed-child-window-bootstrap.patch
  modified:
    - src/app.c
    - vendors/libsokol/CMakeLists.txt

key-decisions:
  - "Used shared Win32 embed state instead of late reparenting so the Sokol window is created as a child from birth."
  - "Kept embedded startup strict by exiting on invalid parent HWND or child-window creation failure with no standalone fallback path."

patterns-established:
  - "Pattern 1: Pass parsed embed config into a shared helper before `sokol_main()` returns."
  - "Pattern 2: Keep vendor patch application reproducible through CMake-driven `git apply` against the pinned upstream source."
  - "Pattern 3: Default embedded startup to viewer-first mode by hiding standalone-heavy chrome in `init()`."

requirements-completed: [EMBD-02, EMBD-03]

# Metrics
duration: 3 min
completed: 2026-05-14
---

# Phase 43 Plan 02: Child-Window Bootstrap Summary

**Patched Sokol to create mdCAD as a true Win32 child window from birth and wired embedded startup into a trimmed viewer baseline with no silent fallback path.**

## Performance

- **Duration:** 3 min
- **Started:** 2026-05-14T15:17:27+01:00
- **Completed:** 2026-05-14T15:20:29+01:00
- **Tasks:** 2
- **Files modified:** 4

## Accomplishments
- Added `src/platform/win32_embed.h` so the parsed launch config can be shared between `app.c` and the patched Sokol Win32 bootstrap.
- Created the repo-hosted Sokol patch that validates the parent HWND, uses `GetClientRect`, and creates mdCAD with `WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN`.
- Wired `app.c` to push embed state before window creation and to start embedded mode with standalone chrome hidden plus Pick/Slot/FPS debug windows closed.

## Task Commits

Each task was committed atomically:

1. **Task 1: Add the checked-in Win32 child-window patch and embed bootstrap helper** - `ee35eee` (feat)
2. **Task 2: Wire embedded startup and immediate viewer trimming in app.c** - `a743875` (feat)

**Plan metadata:** pending closeout docs commit

## Files Created/Modified
- `src/platform/win32_embed.h` - Shared Win32 embed state and fail-fast startup helpers for both the app and patched Sokol code.
- `vendors/libsokol/patches/0001-win32-embed-child-window-bootstrap.patch` - Pinned child-window bootstrap patch for `sokol_app.h`.
- `src/app.c` - Stores parsed launch state, hands it to the Win32 helper before window creation, and trims embedded startup chrome.
- `vendors/libsokol/CMakeLists.txt` - Applies the vendor patch with `git apply --recount` so the pinned patch remains reproducible during configure.

## Decisions Made
- Shared embed startup state through `src/platform/win32_embed.h` so `app.c` and the patched Win32 Sokol path can coordinate before `init()`.
- Explicitly fail child-window startup on invalid parent HWND, missing client area, `CreateWindowExW` failure, or `GetDC` failure instead of risking a standalone fallback.
- Used `state.ui_visible = !state.launch.embedded` plus explicit debug-window closes to establish the Phase 43 viewer-first embedded baseline immediately.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Hardened the Sokol patch hook with `git apply --recount`**
- **Found during:** Task 2 (Wire embedded startup and immediate viewer trimming in `app.c`)
- **Issue:** The checked-in Sokol patch content matched the pinned source semantically, but `git apply --check` rejected the stored hunk counts during configure and blocked the native build.
- **Fix:** Updated `vendors/libsokol/CMakeLists.txt` to use `git apply --recount` for the check/apply/reverse-check path.
- **Files modified:** `vendors/libsokol/CMakeLists.txt`
- **Verification:** `cmake -S . -B build-vulkan -DUSE_VULKAN=ON && cmake --build build-vulkan --config Release --target mdCAD`
- **Committed in:** `a743875` (Task 2 commit)

---

**Total deviations:** 1 auto-fixed (1 blocking)
**Impact on plan:** The hook hardening was required for the planned pinned patch workflow to build cleanly. No scope creep or behavior change beyond patch application reliability.

## Issues Encountered
- The first reconfigure after adding the patch failed at configure time because the stored patch hunk counts were too strict for plain `git apply`; switching the hook to `--recount` resolved it.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness
- Ready for `43-03`: the host scaffold from 43-01 now has a real embedded mdCAD child-window target to launch into.
- The fast parser CTest still passes after the Win32 bootstrap changes, so the launch-contract guardrail remains intact.
- Phase-level requirement checkboxes remain pending until the end-to-end host proof plan closes the remaining shared Phase 43 behaviors.

---
*Phase: 43-embed-contract-child-window-bootstrap*
*Completed: 2026-05-14*
