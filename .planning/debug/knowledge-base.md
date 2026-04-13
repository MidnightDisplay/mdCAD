# GSD Debug Knowledge Base

Resolved debug sessions. Used by `gsd-debugger` to surface known-pattern hypotheses at the start of new investigations.

---

## phase5-quat-ops-gate-fail — Phase 5 quat gate fail was capture-noise from low iteration count
- **Date:** 2026-03-26
- **Error patterns:** gate fail, bench-quat-ops, slowdown criterion, OVERALL FAIL, no runtime error, performance regression row
- **Root cause:** Phase 5 gate policy/tooling intentionally treats bench-quat-ops as a required case and fails overall when any required case fails; the observed quaternion FAIL came from noisy single-shot 20k-iteration captures where short benchmark durations are highly variable, not from missing baseline or invalid row computation.
- **Fix:** Updated Phase 5 QUICKSTART performance-gate capture instructions to use 2,000,000 iterations (instead of 20,000) and documented rationale (reduce timer-noise outliers), while keeping strict per-case gate policy unchanged.
- **Files changed:** docs/QUICKSTART.md
---

## phase13-script-editor-uat-regressions — Script Editor silently ignored invalid edits and missed geometry refresh
- **Date:** 2026-04-01
- **Error patterns:** missing diagnostics, silent no-op apply, illegal constraint edit, stale/blocked apply state, moved line vertices not reflected in script, discard prompt repeats, random letters between blocks ignored
- **Root cause:** Phase 13 implementation had four gaps: parser tolerated non-delimiter junk inside script blocks, preview only did syntax parse (not link/type legality), Script Editor diagnostics UI cleared apply failures while preview stayed parse-OK, and geometry vertex drag path did not bump script emit revision.
- **Fix:** Enforced strict delimiter/token checks in parser blocks; added legality validation in preview path; preserved/displayed apply-failure diagnostics until edit/successful apply; and triggered script re-emit after geometry vertex drags. Added regression tests for unexpected-token rejection and illegal participant preview rejection.
- **Files changed:** src/scripting/sketch_script_parse.h, src/scripting/sketch_script_apply.h, src/app.c, src/tests/script_roundtrip_tests.c
---

## post-delete-undo-endpoint-desync — Bulk delete undo lost endpoint linkage and pick decode after recreate
- **Date:** 2026-04-05
- **Error patterns:** delete undo desync, endpoint gizmo transform move, endpoint owner linkage lost, encoded endpoint pick lookup, no visible errors
- **Root cause:** Bulk delete undo restore did not explicitly preserve/remap endpoint linkage metadata in undo snapshots, and endpoint pick lookup path did not decode encoded endpoint pick IDs. In delete+undo flows this could leave endpoint interactions desynced from owner geometry semantics.
- **Fix:** Extended undo_entity_snapshot_t to include EndPointsComp and restored it in undo_create_from_snapshot; added endpoint owner/binding ID remap in undo_restore_bulk_entity_relationships. Added regression coverage for bulk delete undo line/arc endpoint sync and restored endpoint pick mapping.
- **Files changed:** src/undo_redo.h, src/undo_redo_exec.h, src/tests/endpoint_pick_test.c
---

## jsonl-large-file-parses-zero-entities — Large JSONL import silently completed with 0 entities
- **Date:** 2026-04-13
- **Error patterns:** jsonl large file, parses 0 entities, import-as-sketch, no explicit error, >4MB line, truncated entry
- **Root cause:** Incremental parser used fixed 4MB fgets line buffer and ignored jsonl_parse_entry failures, so >4MB single-line JSONL entries were truncated/skipped without parse.error; import/reparse paths then reported success with zero entities.
- **Fix:** Replaced fixed-cap fgets line reads with dynamic realloc-backed line reader across JSONL scan/parse paths; now non-empty parse-entry failures set JSONL_ERROR_PARSE_ERROR so malformed/truncated input fails explicitly.
- **Files changed:** src/jsonl_loader.h, src/tests/jsonl_loader_limits_test.c, src/CMakeLists.txt
---
