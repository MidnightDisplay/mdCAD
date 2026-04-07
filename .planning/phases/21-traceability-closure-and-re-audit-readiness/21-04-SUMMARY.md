---
phase: 21-traceability-closure-and-re-audit-readiness
plan: 04
subsystem: ui
tags: [sketch, geometry-manager, scene-hierarchy, inspector, callback]
requires:
  - phase: 21-traceability-closure-and-re-audit-readiness
    provides: "21-03 traceability baseline and SKCH-01 blocker confirmation"
provides:
  - "Inspector mutation callback contract for sketch geometry operations"
  - "App wiring from inspector mutations to scene hierarchy dirty invalidation"
  - "GeometryManager mutation paths now invalidate hierarchy immediately on success"
affects: [phase-21-re-audit, SKCH-01, scene-hierarchy-refresh]
tech-stack:
  added: []
  patterns: ["header-only callback contract + app-side wiring for UI cache invalidation", "mutation helper reuse between workspace and standard inspector paths"]
key-files:
  created: [".planning/phases/21-traceability-closure-and-re-audit-readiness/21-04-SUMMARY.md"]
  modified: ["src/ui/ui_entity_inspector.h", "src/app.c", "src/tests/script_roundtrip_tests.c"]
key-decisions:
  - "Use callback + user-data on inspector state instead of direct hierarchy dependency to keep modules decoupled."
  - "Centralize GeometryManager mutation actions in inspector helpers so both inspector entrypoints share identical dirty-callback semantics."
patterns-established:
  - "Sketch geometry mutation paths must call a single null-safe notifier only when mutation succeeds."
requirements-completed: [SKCH-01]
duration: 17min
completed: 2026-04-07
---

# Phase 21 Plan 04: SKCH-01 Scene Hierarchy Refresh Closure Summary

**Scene Hierarchy invalidation now fires immediately for GeometryManager sketch mutations via inspector callback wiring, closing the delayed-refresh SKCH-01 runtime gap.**

## Performance

- **Duration:** 17 min
- **Started:** 2026-04-07T10:46:18Z
- **Completed:** 2026-04-07T11:03:19Z
- **Tasks:** 2
- **Files modified:** 3

## Accomplishments
- Added an inspector-level sketch-geometry mutation callback contract (callback + user-data + setter + notifier).
- Wired app initialization so inspector mutation notifications mark the Scene Hierarchy cache dirty via `ui_scene_hierarchy_mark_dirty`.
- Implemented GeometryManager mutation helper functions and reused them in both inspector GeometryManager views so add/fix/unfix/delete success paths trigger dirty invalidation consistently.
- Added and executed regression coverage proving mutation paths trigger callback and non-mutating/no-op flow does not.

## Task Commits

1. **Task 1: Add inspector-to-hierarchy dirty callback contract** - `66cac39` (feat)
2. **Task 2 (TDD RED): Add failing test for GeometryManager mutation callback behavior** - `b3b5da1` (test)
3. **Task 2 (TDD GREEN): Invoke dirty callback on GeometryManager mutation success paths** - `62bc88a` (feat)

## Files Created/Modified
- `src/ui/ui_entity_inspector.h` - callback contract, mutation helper APIs, and callback-triggered GeometryManager mutation integration.
- `src/app.c` - runtime callback wiring from inspector to Scene Hierarchy dirty marking.
- `src/tests/script_roundtrip_tests.c` - mutation callback behavior coverage and backward-compatible init callsite updates.

## Decisions Made
- Kept callback contract in inspector state (no direct hierarchy include dependency in inspector).
- Kept callback invocation guarded and success-only to avoid false refresh churn on non-mutating UI actions.
- Refactored duplicate GeometryManager mutation logic in inspector to shared helper functions to guarantee dual-entrypoint parity.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Verification command needed explicit test-target build before ctest**
- **Found during:** Task 2 GREEN verification
- **Issue:** `ctest -R "script_roundtrip_tests"` failed because the executable was not built when running the plan’s `mdCAD`-only build command.
- **Fix:** Executed `cmake --build build-vulkan --config Release --target script_roundtrip_tests` before rerunning ctest.
- **Files modified:** None (execution-time fix)
- **Verification:** `script_roundtrip_tests` built and ctest passed.
- **Committed in:** N/A (verification step only)

---

**Total deviations:** 1 auto-fixed (1 blocking)
**Impact on plan:** No scope creep; verification pipeline unblocked while preserving planned implementation scope.

## Issues Encountered
- TDD RED/Green cycle required intentionally validating failure first before restoring implementation; resolved with a controlled temporary stash/pop workflow.

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- SKCH-01 now has product-code mutation invalidation closure in runtime paths.
- Phase 21 follow-up plans can re-audit with immediate hierarchy refresh behavior available in both sketch geometry entrypoints.

## Self-Check: PASSED
- FOUND: .planning/phases/21-traceability-closure-and-re-audit-readiness/21-04-SUMMARY.md
- FOUND: 66cac39
- FOUND: b3b5da1
- FOUND: 62bc88a
