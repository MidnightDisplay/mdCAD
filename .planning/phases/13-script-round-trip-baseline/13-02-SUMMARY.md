---
phase: 13-script-round-trip-baseline
plan: 02
subsystem: scripting
tags: [lua, parser, apply, emitter, deterministic, ecs]
requires:
  - phase: 13-script-round-trip-baseline
    provides: Script identity/runtime baselines and script_roundtrip_tests target from 13-01
provides:
  - Declarative parser that maps supported sketch script scope into an intermediate model
  - Atomic two-pass script apply flow (create then link) exposed through scene façade
  - Deterministic scene-to-script emitter with stable ordering and canonical numeric formatting
  - Round-trip tests for reconstruction, forward-reference linking, atomic commit failure, and deterministic emit stability
affects: [SCRP-02, SCRP-03, phase-13-script-editor]
tech-stack:
  added: []
  patterns: [two-pass link reconstruction, scene-owned script façade calls, deterministic type-group plus script-id emit sorting]
key-files:
  created:
    - src/scripting/sketch_script_parse.h
    - src/scripting/sketch_script_apply.h
    - src/scripting/sketch_script_emit.h
  modified:
    - src/ecs/ecs_scene.h
    - src/scripting/sketch_script_contract.h
    - src/tests/script_roundtrip_tests.c
key-decisions:
  - "Parser and apply stay in header-only scripting modules and expose scene-level façade entrypoints for UI callers."
  - "Deterministic emit ordering is enforced by explicit type grouping (point/line/arc) and script-local ID sort."
patterns-established:
  - "Two-pass script apply: create all geometry first, then resolve and create constraints by script-local participant IDs."
  - "All sketch mutation paths call scene_script_reemit_for_sketch, which increments scene-owned script emit revision."
requirements-completed: [SCRP-02, SCRP-03]
duration: 35min
completed: 2026-04-01
---

# Phase 13 Plan 02: Declarative parser/apply and deterministic emitter Summary

**Supported sketch scripts now parse into an intermediate model, apply atomically through create-then-link passes, and re-emit deterministically with stable ordering and fixed-decimal numeric text.**

## Performance

- **Duration:** 35 min
- **Started:** 2026-04-01T19:10:00Z
- **Completed:** 2026-04-01T19:45:00Z
- **Tasks:** 2
- **Files modified:** 6

## Accomplishments
- Implemented `sketch_script_parse.h` to parse supported Phase 13 declarative entities/constraints into a typed intermediate model.
- Implemented `sketch_script_apply.h` with two-pass reconstruction and atomic commit behavior (reject unresolved/illegal links before mutating committed state).
- Added scene façade entrypoints in `ecs_scene.h` for script preview/commit orchestration.
- Implemented `sketch_script_emit.h` canonical emitter with deterministic ordering and non-scientific trimmed fixed-decimal numeric formatting.
- Added deterministic emit + reconstruction tests in `script_roundtrip_tests.c`, including no-op byte stability and mutation-triggered re-emit revision checks.

## Task Commits

1. **Task 1: Implement parser + two-pass reconstruction façade for supported sketch scope (D-02, D-09)** - `c7a906e` (feat)
2. **Task 2: Implement deterministic scene->script emitter and mutation-triggered re-emit (D-10, D-11)** - `62d7287` (feat)

## Files Created/Modified
- `src/scripting/sketch_script_parse.h` - Declarative parser and intermediate model definitions for supported entity/constraint script scope.
- `src/scripting/sketch_script_apply.h` - Preview/commit apply helpers with two-pass reconstruction and atomic failure semantics.
- `src/scripting/sketch_script_emit.h` - Deterministic emitter with stable ordering, canonical numeric formatting, and scene re-emit revision helpers.
- `src/ecs/ecs_scene.h` - Scene-owned script façade functions and mutation hooks that trigger re-emit updates.
- `src/scripting/sketch_script_contract.h` - Added shared script error contract and `sketch_script_contract_validate` interface stub expected by plan interfaces.
- `src/tests/script_roundtrip_tests.c` - Added reconstruction, forward reference, atomic commit, deterministic ordering, formatting, no-op stability, and revision increment checks.

## Verification Evidence
- Fast smoke:
  - `ctest --test-dir build-vulkan -C Release --output-on-failure -R script_roundtrip_tests` → 1/1 passed
- Full gate:
  - `cmake --build build-vulkan --config Release --target script_roundtrip_tests` → success
  - `ctest --test-dir build-vulkan -C Release --output-on-failure -R script_roundtrip_tests` → 1/1 passed

## Decisions Made
- Kept parsing/apply/emission contracts header-only to match existing project style and ease scene/UI integration.
- Used explicit emission sort rules (type group + script-local ID) instead of relying on ECS iteration order.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Added missing interface-level error contract symbol**
- **Found during:** Task 1 compile integration
- **Issue:** Plan interface expected shared `sketch_script_error_t` and `sketch_script_contract_validate`, but contract header did not define them.
- **Fix:** Added both declarations/implementation in `sketch_script_contract.h` to unblock parser/apply façade integration.
- **Files modified:** `src/scripting/sketch_script_contract.h`
- **Verification:** `script_roundtrip_tests` builds and runs with scene façade signatures using shared error type.
- **Committed in:** `c7a906e`

---

**Total deviations:** 1 auto-fixed (Rule 3 blocking)
**Impact on plan:** Change was required to satisfy declared interface contract and complete planned parser/apply integration.

## Issues Encountered
None beyond the interface contract blocker resolved during Task 1.

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- Scene-level script parser/apply/emitter baseline is in place for Script Editor UX integration in 13-03.
- Deterministic output and atomic apply behavior are covered by dedicated `script_roundtrip_tests` assertions.

## Self-Check: PASSED
- FOUND: `.planning/phases/13-script-round-trip-baseline/13-02-SUMMARY.md`
- FOUND: `src/scripting/sketch_script_parse.h`
- FOUND: `src/scripting/sketch_script_apply.h`
- FOUND: `src/scripting/sketch_script_emit.h`
- FOUND: `c7a906e`
- FOUND: `62d7287`
