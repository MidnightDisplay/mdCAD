---
phase: 13-script-round-trip-baseline
plan: 01
subsystem: scripting
tags: [lua, script-roundtrip, ecs, serializer, ctest]
requires:
  - phase: 12-solver-control-and-constrained-interaction
    provides: sketch/constraint scene APIs and solver-safe scene contracts
provides:
  - stable script-local identity component for script-addressable sketch entities
  - serializer save/load persistence of script-local IDs with sketch normalization
  - Lua 5.4.x runtime guard helpers and declarative script contract validators
  - executable script_roundtrip_tests CTest target
affects: [SCRP-02, SCRP-06, phase-13-parser-apply, phase-13-editor]
tech-stack:
  added: []
  patterns: [scene-level script ID normalization before emit/load, runtime version guard helper baseline]
key-files:
  created:
    - src/components/script_identity_comp.h
    - src/scripting/sketch_script_runtime.h
    - src/scripting/sketch_script_contract.h
    - src/tests/script_roundtrip_tests.c
  modified:
    - CMakeLists.txt
    - src/CMakeLists.txt
    - src/ecs/ecs_world.h
    - src/ecs/ecs_scene.h
    - src/scene_serializer.h
key-decisions:
  - "Normalize script-local IDs at scene level per sketch before serializer emit/load to keep uniqueness deterministic."
  - "Implement Lua 5.4 baseline checks in header-only runtime helpers and verify through dedicated CTest target."
patterns-established:
  - "Pattern 1: Script-addressable geometry/constraint entities own ScriptIdentityComp persisted through serializer paths."
  - "Pattern 2: script_roundtrip_tests is the Wave-0 baseline gate for runtime + contract + identity round-trip checks."
requirements-completed: [SCRP-02, SCRP-06]
duration: 2min
completed: 2026-04-01
---

# Phase 13 Plan 01: Script identity/runtime contracts and round-trip test baseline Summary

**Script-local IDs now persist and normalize across scene save/load while a dedicated `script_roundtrip_tests` target enforces Lua 5.4.x and declarative contract baselines.**

## Performance

- **Duration:** 2 min
- **Started:** 2026-04-01T19:07:46Z
- **Completed:** 2026-04-01T19:08:45Z
- **Tasks:** 2
- **Files modified:** 9

## Accomplishments
- Added `ScriptIdentityComp` and ECS registration/accessors so script-addressable sketch geometry/constraint entities can carry stable `script_local_id`.
- Extended scene serializer to emit/load `script_identity.script_local_id` and normalized per-sketch IDs through `scene_normalize_sketch_script_local_ids`.
- Added scripting baseline headers for Lua 5.4 runtime checks and declarative contract validation, plus `script_roundtrip_tests` executable wired into CTest.

## Task Commits

1. **Task 1: Add stable script-local identity contracts and persistence (D-08)** - `f9aa074` (feat)
2. **Task 2: Create Lua 5.4.x runtime and script contract baseline with test target (D-01, D-02, D-03)** - `4d80097` (feat)

## Files Created/Modified
- `src/components/script_identity_comp.h` - New stable script-local identity component contract and helpers.
- `src/ecs/ecs_world.h` - Registered `ScriptIdentityComp` and added get/set ECS helpers.
- `src/ecs/ecs_scene.h` - Added sketch-level script ID normalization/uniqueness helper APIs.
- `src/scene_serializer.h` - Persisted `script_identity` data on save/load and normalized IDs around emit/apply flow.
- `src/scripting/sketch_script_runtime.h` - Added Lua 5.4.x runtime/version guard helpers.
- `src/scripting/sketch_script_contract.h` - Added Phase 13 declarative entity/constraint schema validators.
- `src/tests/script_roundtrip_tests.c` - Added runtime/contract/identity round-trip tests.
- `src/CMakeLists.txt` - Added `script_roundtrip_tests` build target and CTest registration.
- `CMakeLists.txt` - Enabled top-level CTest so test discovery works in `build-vulkan`.

## Verification Evidence
- Fast smoke:
  - `lua -v` → `Lua 5.4.8`
  - `ctest --test-dir build-vulkan -C Release --output-on-failure -R script_roundtrip_tests` → 1/1 passed
- Full gate:
  - `cmake --build build-vulkan --config Release --target script_roundtrip_tests` → success
  - `ctest --test-dir build-vulkan -C Release --output-on-failure -R script_roundtrip_tests` → 1/1 passed
  - `lua -v` → `Lua 5.4.8`

## Decisions Made
- Used a dedicated `ScriptIdentityComp` instead of labels/raw ECS IDs for script identity persistence and normalization.
- Kept runtime contract validation header-only for immediate reuse by later parser/editor integration tasks.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Enabled top-level CTest registration**
- **Found during:** Task 2 verification
- **Issue:** `ctest` initially returned `No tests were found!!!` because CTest was not enabled at project root.
- **Fix:** Added `include(CTest)` in root `CMakeLists.txt` so `script_roundtrip_tests` registration is discoverable in `build-vulkan`.
- **Files modified:** `CMakeLists.txt`
- **Verification:** `ctest --test-dir build-vulkan -C Release --output-on-failure -R script_roundtrip_tests` now executes and passes.
- **Committed in:** `4d80097` (Task 2)

**2. [Rule 3 - Blocking] Linked and stubbed test executable runtime dependencies**
- **Found during:** Task 2 verification
- **Issue:** `script_roundtrip_tests` initially failed to link due to missing vendor library linkage and unresolved `sokol_main`.
- **Fix:** Linked `script_roundtrip_tests` against `libsokol/libcimgui/flecs_static/cjson` and added a no-op `sokol_main` stub in test source.
- **Files modified:** `src/CMakeLists.txt`, `src/tests/script_roundtrip_tests.c`
- **Verification:** `cmake --build build-vulkan --config Release --target script_roundtrip_tests` now succeeds.
- **Committed in:** `4d80097` (Task 2)

---

**Total deviations:** 2 auto-fixed (2 blocking)
**Impact on plan:** Both fixes were required to make the planned CTest verification executable; no scope creep.

## Issues Encountered
- None beyond build/test registration/linkage blockers auto-resolved during Task 2.

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- Phase 13 now has explicit runtime and schema baseline contracts, plus persistent script IDs required for parser/apply and deterministic emit plans.
- `script_roundtrip_tests` is available as a focused guardrail for subsequent Phase 13 tasks.

## Self-Check: PASSED
- Found file: `.planning/phases/13-script-round-trip-baseline/13-01-SUMMARY.md`
- Found file: `src/components/script_identity_comp.h`
- Found file: `src/scripting/sketch_script_runtime.h`
- Found file: `src/scripting/sketch_script_contract.h`
- Found file: `src/tests/script_roundtrip_tests.c`
- Found commit: `f9aa074`
- Found commit: `4d80097`
