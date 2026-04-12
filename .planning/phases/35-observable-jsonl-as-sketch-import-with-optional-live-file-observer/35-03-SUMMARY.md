---
phase: 35-observable-jsonl-as-sketch-import-with-optional-live-file-observer
plan: 03
subsystem: ui
tags: [jsonl, sketch-import, observer, script-editor, regression, uat]
requires:
  - phase: 35-observable-jsonl-as-sketch-import-with-optional-live-file-observer
    provides: "Wave 0 RED contracts and observer persistence/reparse runtime from 35-01/35-02."
provides:
  - "Dedicated File → Import JSONL as Sketch flow with sketch-only geometry mapping and transactional observer-link setup."
  - "Observer control relocation between Entity Inspector and Active Sketch Workspace with explicit overwrite-warning UX."
  - "Large-script stability fixes (adaptive editor acceptance, parser limit increase, heap apply workspaces) validated by targeted regressions and UAT approval."
affects: [jsonl sketch UX, observer ergonomics, script apply reliability, phase 35 closure]
tech-stack:
  added: []
  patterns: ["Sketch import via sketch API-only mapping", "Observer settings persisted and reused for manual/auto reparse", "Heap-backed script apply workspaces for large model safety"]
key-files:
  created: []
  modified:
    - src/ui/ui_scene_hierarchy.h
    - src/ui/ui_entity_inspector.h
    - src/app.c
    - src/jsonl_sketch_import_job.h
    - src/ecs/ecs_scene.h
    - src/scripting/sketch_script_apply.h
    - src/scripting/sketch_script_emit.h
    - src/scripting/sketch_script_parse.h
    - src/tests/jsonl_sketch_import_test.c
    - src/tests/jsonl_sketch_mapping_test.c
    - src/tests/script_roundtrip_tests.c
key-decisions:
  - "Kept observer link persisted but defaulted active observation OFF after UAT feedback and explicit checkpoint approval."
  - "Decoupled editor emission completeness from preview parser success via footer-complete checks to avoid blank-editor regressions."
  - "Moved large parse/apply workspaces to heap and added adaptive snapshot capture to prevent stack overflow and undo snapshot truncation failure modes."
patterns-established:
  - "JSONL mesh records are ignored in sketch import while polyline/polygon flatten to unconstrained line entities."
  - "Observer UX is single-surface: editable in inspector only when sketch inactive, otherwise moved to active workspace."
requirements-completed: [P35-01, P35-02, P35-05]
duration: 620min
completed: 2026-04-12
---

# Phase 35 Plan 03: JSONL-as-sketch import UX + observer relocation Summary

**Shipped end-to-end JSONL-as-sketch import and observer UX relocation with approved large-script stability fixes, resulting in passing targeted regressions and human-verified workflow closure.**

## Performance

- **Duration:** 620 min
- **Started:** 2026-04-12T13:09:06Z
- **Completed:** 2026-04-12T23:29:54Z
- **Tasks:** 3
- **Files modified:** 11

## Accomplishments
- Implemented and wired the dedicated JSONL-as-sketch import path, including sketch geometry mapping contract (Point/Line/Arc/Circle, polyline/polygon flattening, mesh ignore, unconstrained output).
- Completed observer control relocation + overwrite warning UX contract across Entity Inspector and Active Sketch Workspace with latest-two observer message behavior.
- Closed UAT-discovered regressions (blank script editor on large imports, parser-cap mismatch, Apply crash) and finalized with human checkpoint approval.

## Task Commits

Each task was committed atomically:

1. **Task 1 (TDD RED): add sketch import/mapping contract coverage** - `77e92bc` (test)
2. **Task 1 (GREEN): implement JSONL-as-sketch import path and mapping** - `6b69d0b` (feat)
3. **Task 2: relocate observer controls + warnings UX** - `be756d3` (feat)
4. **Task 2 follow-up: restore JSONL-as-sketch browser + rollback popup behavior** - `7aa17d0` (fix)
5. **Task 3 follow-up: adaptive editor buffer acceptance for large imports** - `aa6915d` (fix)
6. **Task 3 follow-up: set observe default OFF (approved UAT adjustment)** - `43dade2` (fix)
7. **Task 3 follow-up: stabilize large-script apply/snapshot handling and regressions** - `ab005cd` (fix)

**Plan metadata:** `(pending final docs commit)`

## Files Created/Modified
- `src/ui/ui_scene_hierarchy.h` - Added dedicated File menu action/import popup wiring for JSONL-as-sketch flow and options forwarding.
- `src/jsonl_sketch_import_job.h` - Implemented mapping contract, mesh ignore, transactional observer-link semantics, and persisted transform settings.
- `src/ui/ui_entity_inspector.h` - Added inactive/active observer control routing, relocation hint, overwrite warning, and latest-two message rendering.
- `src/app.c` - Added active workspace observer control surface and script-editor emission robustness logic.
- `src/scripting/sketch_script_parse.h` - Increased deterministic model limits for supported large imported sketches.
- `src/scripting/sketch_script_apply.h` - Moved large apply workspace structures to heap and added OOM-safe paths.
- `src/ecs/ecs_scene.h` - Added heap-backed parse model and adaptive script snapshot capture for transactional apply/undo reliability.
- `src/scripting/sketch_script_emit.h` - Aligned emitter collection capacity with parser limits and restored explicit overflow diagnostics.
- `src/tests/jsonl_sketch_import_test.c` - Added import defaults + transactional failure assertions.
- `src/tests/jsonl_sketch_mapping_test.c` - Added mapping/mesh-ignore/unconstrained behavior coverage.
- `src/tests/script_roundtrip_tests.c` - Added large-sketch emit/apply regressions, max-model crash guard, and over-limit preview failure coverage.

## Verification Results
- `ctest --test-dir build-vulkan -C Release -R "jsonl_sketch_import_test|jsonl_sketch_mapping_test|jsonl_observer_state_test|jsonl_reparse_transaction_test|jsonl_label_contract_test|script_roundtrip_tests|scene_solver_contract" --output-on-failure`  
  Result: **PASS** (7/7)
- `git --no-pager grep -n "Import JSONL as Sketch\|JSONL_GEOM_MESH\|scene_add_.*_to_sketch\|scale\|rotation\|shift_to_center" src/ui/ui_scene_hierarchy.h src/jsonl_sketch_import_job.h`  
  Result: **PASS**
- `git --no-pager grep -n "Observer controls moved to Active Sketch Workspace\|two most recent\|overwritten by next successful JSONL re-parse\|scale\|rotation\|shift_to_center" src/ui/ui_entity_inspector.h src/app.c`  
  Result: **PASS**
- **Checkpoint Task 3 (human-verify):** **APPROVED**  
  User confirmed phase 35 UAT pass, including adaptive script buffer regression fix, parser limit increase, heap-workspace apply crash fix, and observe default OFF behavior.

## Decisions Made
- Used checkpoint-approved UX behavior where linked observer remains available but **observe default is OFF**.
- Preserved deterministic overflow diagnostics (`too many entities/constraints`) while expanding supported in-range capacity.
- Kept apply semantics transactional with rollback and adaptive snapshot capture to avoid partial mutation on failure.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Blank Script Editor after JSONL sketch import for large scripts**
- **Found during:** Task 3 verification/UAT
- **Issue:** Script emission load path was preview-parse gated, causing valid complete emitted scripts to be dropped when parser model limits were exceeded.
- **Fix:** Switched editor acceptance to emitted-footer completeness checks and kept adaptive buffer growth.
- **Files modified:** `src/app.c`, `src/tests/script_roundtrip_tests.c`
- **Verification:** `script_roundtrip_tests` pass including large-sketch coverage.
- **Committed in:** `aa6915d`

**2. [Rule 2 - Missing Critical] Parser/apply model limits insufficient for practical phase-35 imported sketches**
- **Found during:** Task 3 verification/UAT
- **Issue:** 128 entity/constraint limits blocked preview/apply for supported imported sketches.
- **Fix:** Raised deterministic model limits to 512 and added explicit over-limit regression assertions.
- **Files modified:** `src/scripting/sketch_script_parse.h`, `src/tests/script_roundtrip_tests.c`
- **Verification:** Full targeted CTest suite pass; explicit over-limit diagnostic assertion retained.
- **Committed in:** `ab005cd` (plus prior related fixes)

**3. [Rule 1 - Bug] Apply crash after limit increase due to large stack-allocated workspaces**
- **Found during:** Task 3 verification/UAT
- **Issue:** Apply/preview/commit paths allocated large model/workspace tables on stack, causing crash risk at high model sizes.
- **Fix:** Migrated parse/apply workspaces to heap with OOM handling; added max-entity apply crash regression.
- **Files modified:** `src/scripting/sketch_script_apply.h`, `src/ecs/ecs_scene.h`, `src/tests/script_roundtrip_tests.c`
- **Verification:** `script_roundtrip_tests` and full targeted suite pass.
- **Committed in:** `ab005cd`

**4. [Rule 1 - Bug] Undo/apply snapshot emission truncation after capacity expansion**
- **Found during:** Task 3 follow-up regression execution
- **Issue:** Fixed-size script snapshot buffers in apply commit could truncate post-change snapshots, breaking large-model apply transaction reliability.
- **Fix:** Added adaptive snapshot capture helper with footer-complete checks and capped growth.
- **Files modified:** `src/ecs/ecs_scene.h`
- **Verification:** `script_roundtrip_tests` and full targeted suite pass.
- **Committed in:** `ab005cd`

### Checkpoint-approved adjustment

**Observe default OFF**
- **Context:** UAT checkpoint feedback requested linked observer default OFF behavior.
- **Action:** Applied as approved checkpoint continuation adjustment.
- **Files modified:** `src/jsonl_sketch_import_job.h`, `src/tests/jsonl_sketch_import_test.c`
- **Committed in:** `43dade2`

---

**Total deviations:** 4 auto-fixed (3 bug, 1 missing critical) + 1 human-approved adjustment  
**Impact on plan:** All changes were required for correctness/stability of the shipped phase-35 UX contract; no unrelated scope expansion.

## Issues Encountered
- Intermittent environment-level file-lock (`resource busy or locked`) on one test binary launch; resolved by rerunning after build completion.

## User Setup Required
None - no external service configuration required.

## Known Stubs
None.

## Next Phase Readiness
- Phase 35 contract is now end-to-end implemented and UAT approved with regression coverage for the discovered large-script edge cases.
- Milestone closure/state advancement can proceed without further implementation work for this phase.

---
*Phase: 35-observable-jsonl-as-sketch-import-with-optional-live-file-observer*  
*Completed: 2026-04-12*

## Self-Check: PASSED
- FOUND: `.planning/phases/35-observable-jsonl-as-sketch-import-with-optional-live-file-observer/35-03-SUMMARY.md`
- FOUND commit: `77e92bc`
- FOUND commit: `6b69d0b`
- FOUND commit: `be756d3`
- FOUND commit: `7aa17d0`
- FOUND commit: `aa6915d`
- FOUND commit: `43dade2`
- FOUND commit: `ab005cd`
