---
phase: 35-observable-jsonl-as-sketch-import-with-optional-live-file-observer
plan: 01
subsystem: testing
tags: [jsonl, sketch-import, ctest, wave-0, red-first]
requires:
  - phase: 35-observable-jsonl-as-sketch-import-with-optional-live-file-observer
    provides: "Phase 35 context decisions D-01..D-20 and Wave 0 validation contract."
provides:
  - "Five deterministic RED-first jsonl_* test executables for import, mapping, observer, reparse, and label contracts."
  - "CTest registration for all Phase 35 Wave 0 jsonl test targets."
affects: [35-02-PLAN.md, 35-03-PLAN.md, jsonl observer implementation, sketch import implementation]
tech-stack:
  added: []
  patterns: ["Single-file native C test executables with explicit FAILED output", "RED-first scaffold contracts for deferred implementation"]
key-files:
  created:
    - src/tests/jsonl_sketch_import_test.c
    - src/tests/jsonl_sketch_mapping_test.c
    - src/tests/jsonl_observer_state_test.c
    - src/tests/jsonl_reparse_transaction_test.c
    - src/tests/jsonl_label_contract_test.c
  modified:
    - src/CMakeLists.txt
key-decisions:
  - "Keep all new Phase 35 Wave 0 contracts intentionally failing with deterministic RED diagnostics until implementation plans land."
patterns-established:
  - "Every jsonl_* scaffold test exposes at least one static int test_* contract function and reports failures consistently."
requirements-completed: [P35-01, P35-02, P35-03, P35-04, P35-06]
duration: 26min
completed: 2026-04-12
---

# Phase 35 Plan 01: Wave 0 JSONL sketch-import test harness Summary

**Registered five explicit jsonl_* RED-first CTest contracts covering sketch import, mapping, observer state, reparse transaction, and label semantics for Phase 35 implementation follow-on work.**

## Performance

- **Duration:** 26 min
- **Started:** 2026-04-12T12:11:39Z
- **Completed:** 2026-04-12T12:37:39Z
- **Tasks:** 1
- **Files modified:** 6

## Accomplishments
- Added five new `src/tests/jsonl_*_test.c` scaffold binaries with deterministic failure messages.
- Wired all five test binaries into `src/CMakeLists.txt` with `add_executable(...)` and `add_test(...)`.
- Verified CTest discovery and RED execution behavior for all named jsonl targets.

## Task Commits

Each task was committed atomically:

1. **Task 1: Create Wave 0 Phase 35 test scaffolds and wire them into CTest** - `fa1b2df` (test)

**Plan metadata:** `(pending final docs commit)`

## Files Created/Modified
- `src/CMakeLists.txt` - Registers five new `jsonl_*` test executables and CTest entries.
- `src/tests/jsonl_sketch_import_test.c` - RED scaffold for D-17/D-18/D-19/D-20 sketch import contract.
- `src/tests/jsonl_sketch_mapping_test.c` - RED scaffold for geometry mapping + mesh ignore/unconstrained contract.
- `src/tests/jsonl_observer_state_test.c` - RED scaffold for D-01/D-03/D-10/D-11/D-12/D-13 observer policy.
- `src/tests/jsonl_reparse_transaction_test.c` - RED scaffold for D-04/D-05/D-06/D-14 transactional reparse semantics.
- `src/tests/jsonl_label_contract_test.c` - RED scaffold for D-16 filename/fullpath label relink behavior.

## Verification Results
- `cmake --build build-vulkan --config Release`  
  Result: **partial / environment-blocked** (`LNK1104` lock on existing `mdCAD.exe` and `scene_solver_diagnostics.exe`; unrelated to scoped files).
- `cmake --build build-vulkan --config Release --target jsonl_sketch_import_test jsonl_sketch_mapping_test jsonl_observer_state_test jsonl_reparse_transaction_test jsonl_label_contract_test`  
  Result: **PASS** (all five targets build).
- `ctest --test-dir build-vulkan -C Release -N -R "jsonl_sketch_import_test|jsonl_sketch_mapping_test|jsonl_observer_state_test|jsonl_reparse_transaction_test|jsonl_label_contract_test"`  
  Result: **PASS** (5 tests discovered).
- `ctest --test-dir build-vulkan -C Release -R "jsonl_sketch_import_test|jsonl_sketch_mapping_test|jsonl_observer_state_test|jsonl_reparse_transaction_test|jsonl_label_contract_test" --output-on-failure`  
  Result: **EXPECTED RED** (5/5 fail with deterministic contract-pending messages).

## Decisions Made
- Used deterministic RED failure outputs instead of TODO skips so future implementation phases have hard failing targets.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Scoped build verification to new test targets after unrelated linker lock**
- **Found during:** Task 1 (verification)
- **Issue:** Full Release build intermittently failed with `LNK1104` due to locked pre-existing binaries outside this task’s changed files.
- **Fix:** Kept required CTest verification and additionally built only the newly added `jsonl_*` targets to confirm scaffold compilation.
- **Files modified:** `.planning/phases/35-observable-jsonl-as-sketch-import-with-optional-live-file-observer/deferred-items.md`
- **Verification:** Targeted build passed; CTest `-N` discovery and RED execution both passed expectations.
- **Committed in:** (metadata commit)

---

**Total deviations:** 1 auto-fixed (1 blocking)
**Impact on plan:** No scope creep; planned deliverables were completed and verified despite an unrelated environment lock issue.

## Issues Encountered
- Existing environment/process lock prevented full `Release` link of unrelated binaries; logged as deferred out-of-scope issue.

## User Setup Required
None - no external service configuration required.

## Known Stubs
- `src/tests/jsonl_sketch_import_test.c:11` — `test_jsonl_sketch_import_creates_sketch_entity` intentionally returns failure pending feature implementation.
- `src/tests/jsonl_sketch_mapping_test.c:11` — mapping contract intentionally RED-first.
- `src/tests/jsonl_sketch_mapping_test.c:17` — mesh-ignore/unconstrained contract intentionally RED-first.
- `src/tests/jsonl_observer_state_test.c:11` — observer defaults/retry policy contract intentionally RED-first.
- `src/tests/jsonl_reparse_transaction_test.c:11` — failure-preserves-last-good contract intentionally RED-first.
- `src/tests/jsonl_reparse_transaction_test.c:17` — success-authoritative-replace contract intentionally RED-first.
- `src/tests/jsonl_label_contract_test.c:11` — label filename/fullpath relink contract intentionally RED-first.

## Next Phase Readiness
- Phase 35 Wave 0 harness is now present and callable via named CTest filters.
- Phase 35 implementation plans can now drive these tests from RED toward GREEN behavior.

---
*Phase: 35-observable-jsonl-as-sketch-import-with-optional-live-file-observer*
*Completed: 2026-04-12*

## Self-Check: PASSED
- FOUND: `.planning/phases/35-observable-jsonl-as-sketch-import-with-optional-live-file-observer/35-01-SUMMARY.md`
- FOUND: `src/tests/jsonl_sketch_import_test.c`
- FOUND: `src/tests/jsonl_sketch_mapping_test.c`
- FOUND: `src/tests/jsonl_observer_state_test.c`
- FOUND: `src/tests/jsonl_reparse_transaction_test.c`
- FOUND: `src/tests/jsonl_label_contract_test.c`
- FOUND commit: `fa1b2df`
