---
phase: 14-script-io-api-undo-integration
plan: 03
subsystem: ui
tags: [script-io-window, transactional-live-apply, undo-redo, clipboard, regression-closure]
requires:
  - phase: 14-01
    provides: script-apply transactional undo command and rollback guarantees
  - phase: 14-02
    provides: numeric script IO parser/emitter model and scene_script_io_* façade
provides:
  - Dedicated Script IO window launch path from SketchManager beside Script Editor
  - Live IO apply behavior that preserves transactional rollback and one-step undo feel
  - Regression closure for startup seeding, large-script IO apply path, label persistence, clipboard/hotkeys, and IO interaction undo coalescing
affects: [SCRP-05, SCRP-04, API-02]
tech-stack:
  added: []
  patterns: [inspector request/consume window wiring, IO edits routed through scene transaction façade, interaction-level undo coalescing]
key-files:
  created:
    - .planning/phases/14-script-io-api-undo-integration/14-03-SUMMARY.md
  modified:
    - src/ui/ui_entity_inspector.h
    - src/app.c
    - src/ecs/ecs_scene.h
    - src/scripting/sketch_script_apply.h
    - src/scripting/sketch_script_emit.h
    - src/tests/script_roundtrip_tests.c
key-decisions:
  - "Keep Script IO in a dedicated window and use scene_script_io_apply_input_value for all live edits."
  - "Treat startup IO seeding as baseline state and suppress undo recording during seed apply."
  - "Unify numeric input and slider edits under a shared interaction session so one user interaction maps to one undo transaction."
patterns-established:
  - "Script IO and Script Editor stay decoupled at the UI level while sharing transactional scene apply contracts."
  - "scene_script_reemit_for_sketch uses the same script buffer size contract as scene apply/emit flows."
requirements-completed: [SCRP-05, SCRP-04, API-02]
duration: multi-session
completed: 2026-04-02
---

# Phase 14 Plan 03: Script IO window and transactional UX closure Summary

**Phase 14 is now closed: dedicated Script IO UX shipped, live transactional behavior hardened through UAT loop fixes, and automation/build gates are passing on Windows MSVC + Vulkan.**

## Performance

- **Duration:** multi-session
- **Completed:** 2026-04-02
- **Tasks:** 3/3 complete (including human verify loop closure)
- **Files modified:** 6

## Completed Tasks

| Task | Name | Status | Commit(s) |
|---|---|---|---|
| 1 | Add inspector/app-level tests for IO window toggle and live apply contracts | Complete | `fca87ea` |
| 2 | Implement dedicated Script IO window + SketchManager toggle with transactional auto-apply | Complete | `b8ea377` |
| 3 | Human verify Script IO UX and transactional undo feel | Complete (UAT feedback loop resolved) | `e45ae7a` + final stabilization updates in this closure |

## Accomplishments

- Added a dedicated `Open Script IO` launch path from SketchManager and consumed it in app loop window lifecycle.
- Delivered separate Script IO window rendering editable numeric inputs and read-only outputs with immediate sync.
- Kept each IO mutation on the shared transactional apply pipeline with rollback-safe failure behavior.
- Closed UAT-reported regressions:
  - false parser diagnostics from script buffer truncation in IO live apply path
  - label persistence loss across script rebuild/apply
  - clipboard + undo/redo shortcut mismatches on Windows
  - startup seeding creating an unwanted initial undo entry
  - missing undo step for numeric IO edits in interaction coalescing path
- Stabilized large-script IO regression coverage by aligning test fixture scale and emitter reemit buffer use with parser/model limits.

## Task Commits

1. **Task 1: Add failing coverage for Script IO window request and live transactional apply** - `fca87ea` (test)
2. **Task 2: Implement dedicated Script IO window and inspector toggle wiring** - `b8ea377` (feat)
3. **Task 3: UAT stabilization batch for Script IO undo/labels/clipboard behavior** - `e45ae7a` (fix)

## Files Created/Modified

- `src/ui/ui_entity_inspector.h` - added Script IO open request/consume state and SketchManager button wiring.
- `src/app.c` - added dedicated Script IO window lifecycle, interaction tracking/coalescing, startup IO seed behavior, clipboard + shortcut handling.
- `src/ecs/ecs_scene.h` - standardized script text buffer contract for transactional apply/IO flows.
- `src/scripting/sketch_script_apply.h` - preserved labels by script identity during apply rebuild.
- `src/scripting/sketch_script_emit.h` - aligned `scene_script_reemit_for_sketch` sink buffer with shared script buffer contract.
- `src/tests/script_roundtrip_tests.c` - added and stabilized regressions for IO launch/live apply, large-script IO edits, label persistence, undo suppression/coalescing.

## Automated Verification Evidence

- Targeted gate:
  - `cmake --build build-vulkan --config Release --target script_roundtrip_tests`
  - `ctest --test-dir build-vulkan -C Release -R script_roundtrip_tests --output-on-failure`
  - Result: **Passed (1/1)**
- Full configured suite:
  - `ctest --test-dir build-vulkan -C Release --output-on-failure`
  - Result: **Passed (1/1)**
- Build gate:
  - `cmake --build build-vulkan --config Release --target mdCAD`
  - Result: **Succeeded**

## Human-Verify Outcome

- User checklist run confirmed dedicated window behavior, live update flow, and undo/redo behavior after the stabilization loop.
- Follow-up issues were addressed and re-verified by user in-session (copy/paste + undo hotkeys, startup undo suppression, numeric IO undo).
- Final outcome: Phase 14 interactive acceptance criteria satisfied for implemented scope.

## Decisions Made

- Keep numeric-only IO in v1.2; non-numeric widgets remain intentionally out of scope.
- Preserve scene API transactional ownership; UI remains a thin caller into `scene_script_io_*`/`scene_script_apply_commit`.
- Use interaction-level session boundaries in app for undo coalescing while preserving live visual updates.

## Deviations from Plan

### Auto-fixed issues from UAT and closure gating

**1. [Rule 1 - Bug] Script IO large-script path truncated internal script text**
- **Issue:** false parser diagnostics and blocked live apply when script grew beyond internal fixed buffers.
- **Fix:** unified script buffer usage to `ECS_SCENE_SCRIPT_TEXT_BUFFER_SIZE` across emit/apply and aligned reemit sink buffer.
- **Files modified:** `src/ecs/ecs_scene.h`, `src/scripting/sketch_script_emit.h`, `src/tests/script_roundtrip_tests.c`

**2. [Rule 1 - Bug] Label persistence across script apply rebuild**
- **Issue:** labels were lost/re-autonumbered after apply recreation.
- **Fix:** snapshot/restore labels keyed by script identity during apply commit.
- **Files modified:** `src/scripting/sketch_script_apply.h`, `src/tests/script_roundtrip_tests.c`

**3. [Rule 1 - Bug] Startup seed + IO interaction undo edge cases**
- **Issue:** seed apply produced initial undo noise; numeric edits intermittently missed undo transaction push.
- **Fix:** startup undo suppression and unified IO interaction coalescing path.
- **Files modified:** `src/app.c`, `src/tests/script_roundtrip_tests.c`

**4. [Rule 1 - Bug] Windows copy/paste and undo/redo shortcut behavior**
- **Issue:** shortcut semantics diverged due to forced macOS behavior path.
- **Fix:** enable clipboard in app descriptor and use explicit `(Ctrl || Super)` mod handling without forcing mac behavior.
- **Files modified:** `src/app.c`

---

**Total deviations:** 4 auto-fixed issue groups  
**Impact on plan:** All deviations were directly tied to acceptance criteria and closure stability; no scope creep beyond Phase 14 requirements.

## Issues Encountered

- `mdCAD.exe` link gate intermittently failed with file lock (`LNK1104`) when app remained open; resolved by stopping locked process and rebuilding.
- CTest surfaced real regressions once test binary was rebuilt; direct binary runs alone were insufficient for closure confidence.

## User Setup Required

None.

## Next Phase Readiness

- Phase 14 requirements are now complete and evidenced (`SCRP-04`, `SCRP-05`, `API-01`, `API-02`).
- Milestone flow can advance to Phase 15 (`VAL-01`, `VAL-02`, `VAL-03`) for packaged validation and acceptance closure.

## Self-Check: PASSED
- FOUND: `.planning/phases/14-script-io-api-undo-integration/14-03-SUMMARY.md`
- FOUND: `fca87ea`
- FOUND: `b8ea377`
- FOUND: `e45ae7a`
- VERIFIED: targeted and full `ctest` gates pass
- VERIFIED: `mdCAD` Release target builds successfully on Windows MSVC + Vulkan
