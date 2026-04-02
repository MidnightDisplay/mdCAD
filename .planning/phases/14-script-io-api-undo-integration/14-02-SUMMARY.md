---
phase: 14-script-io-api-undo-integration
plan: 02
subsystem: api
tags: [scripting, script-io, parser, emitter, scene-façade]
requires:
  - phase: 14-01
    provides: transactional script apply undo integration
provides:
  - Numeric-only script IO parser model for inputs/outputs with optional min/max/step
  - Scene-level script IO façade APIs for enumerate/read/apply
  - Deterministic script IO emission integrated into existing script emitter
affects: [phase-14-script-io-window, script-editor-io-sync, api-01]
tech-stack:
  added: []
  patterns: [scene façade owns script IO apply path, IO edits reuse scene_script_apply_commit transaction semantics]
key-files:
  created: []
  modified:
    - src/tests/script_roundtrip_tests.c
    - src/scripting/sketch_script_parse.h
    - src/scripting/sketch_script_emit.h
    - src/ecs/ecs_scene.h
key-decisions:
  - "Keep script IO scope numeric-only and reject non-numeric value declarations explicitly."
  - "Use scene_script_io_apply_input_value to re-emit and call scene_script_apply_commit so IO edits share transactional behavior."
patterns-established:
  - "Script IO state is scene-owned per sketch and surfaced only through scene_script_io_* façade functions."
requirements-completed: [SCRP-05, API-01]
duration: 7m 22s
completed: 2026-04-02
---

# Phase 14 Plan 02: Script IO numeric schema and scene façade Summary

**Numeric script inputs/outputs with optional slider metadata now parse, emit deterministically, and apply through scene-level transactional APIs.**

## Performance

- **Duration:** 7m 22s
- **Started:** 2026-04-02T09:24:16Z
- **Completed:** 2026-04-02T09:31:23Z
- **Tasks:** 2
- **Files modified:** 4

## Accomplishments
- Added TDD coverage for numeric IO parse/emit contracts, non-numeric rejection, and scene IO transactional apply behavior.
- Extended script parser model with `inputs`/`outputs` numeric declarations and optional `min/max/step` metadata.
- Added scene façade APIs (`scene_script_io_*`) for IO descriptor enumeration, value readback, and input edit apply via shared script transaction pipeline.
- Updated deterministic script emitter to include IO blocks while preserving existing entity/constraint order.

## Task Commits

1. **Task 1: Add tests for numeric IO parse/emit/apply contracts** - `a9d0df5` (test)
2. **Task 2: Implement numeric IO schema + scene façade APIs for script workflows** - `7559da3` (feat)

## Files Created/Modified
- `src/tests/script_roundtrip_tests.c` - added IO-focused RED/GREEN tests including façade apply transaction behavior.
- `src/scripting/sketch_script_parse.h` - added IO model structs and parser support for numeric inputs/outputs with metadata.
- `src/scripting/sketch_script_emit.h` - added deterministic IO block emission using scene IO descriptors.
- `src/ecs/ecs_scene.h` - added scene-owned IO state and façade entrypoints to enumerate/read/apply IO values.

## Decisions Made
- Preserved locked numeric-only IO decision by validating `value/min/max/step` as numeric tokens and rejecting booleans/strings.
- Kept scene-façade-first API by adding IO operations only on `ecs_scene.h` and routing edits through `scene_script_apply_commit`.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Rebuilt `script_roundtrip_tests` target before RED verification**
- **Found during:** Task 1 (TDD RED)
- **Issue:** Initial targeted ctest run executed stale binary and falsely reported pass.
- **Fix:** Built `script_roundtrip_tests` target explicitly, then reran tests to capture true RED failure.
- **Files modified:** none (build step only)
- **Verification:** `cmake --build build-vulkan --config Release --target script_roundtrip_tests` then `ctest ... -R script_roundtrip_tests`
- **Committed in:** `a9d0df5`

---

**Total deviations:** 1 auto-fixed (Rule 3 blocking)
**Impact on plan:** No scope creep; required to preserve valid TDD red/green evidence.

## Issues Encountered
- Initial ctest did not rebuild changed test executable; resolved by explicit target build before validation.

## Known Stubs
None.

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- Scene API now exposes script IO descriptors and transactional input edits for upcoming dedicated IO window wiring.
- No blockers identified for remaining Phase 14 plans.

## Self-Check: PENDING


## Self-Check: PASSED

