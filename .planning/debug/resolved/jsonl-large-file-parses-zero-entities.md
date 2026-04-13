---
status: resolved
trigger: "Investigate issue: jsonl-large-file-parses-zero-entities"
created: 2026-04-13T00:00:00Z
updated: 2026-04-13T23:51:26+01:00
---

## Current Focus
<!-- OVERWRITE on each update - reflects NOW -->

hypothesis: Human verification confirmed; no remaining blockers, proceed to archive session and record knowledge-base entry.
test: Finalize by closing debug artifact and ensuring resolved session + KB capture root cause and fix.
expecting: Session moved to resolved, knowledge base updated, and no open blocker for this issue.
next_action: Archive session artifacts and report final root-cause/fix summary.

## Symptoms
<!-- Written during gathering, then IMMUTABLE -->

expected: No arbitrary hard cap; import/reparse should handle large JSONL until true memory/slot growth limits are reached.
actual: For large JSONL (around just over 4MB), import to scene or import-as-sketch collapses and reports parsed 0 entities.
errors: No explicit UI/console error message seen by user.
reproduction: Import large JSONL directly to scene OR as sketch; file around >4MB triggers 0 entities. Example path user provided: C:\Users\RodionRadchenko\source\repos\ParkerSteel-GenerateDimensionedDrawing\ParkerSteel.Drawing.Test\bin\Debug\net8.0-windows\test_lamp.jsonl
started: Unknown whether it ever worked before recent work.

## Eliminated
<!-- APPEND only - prevents re-investigating -->

## Evidence
<!-- APPEND only - facts discovered -->

- timestamp: 2026-04-13T00:07:00Z
  checked: .planning/debug/knowledge-base.md
  found: No existing resolved entry matched JSONL large-file/zero-entities symptom pattern.
  implication: Proceed with fresh hypothesis generation and direct code trace.

- timestamp: 2026-04-13T00:10:00Z
  checked: src/jsonl_loader.h, src/jsonl_import_job.h, src/jsonl_sketch_import_job.h (symbol scan)
  found: Hardcoded line buffers observed (1MB quick scan, 4MB main parse chunk, 8MB mesh scan); direct and sketch paths both call jsonl_parse_lines_chunk and only branch on parse.error.
  implication: Strong candidate that overlong JSONL line gets truncated and skipped without setting parse.error, producing success-with-zero behavior.

- timestamp: 2026-04-13T00:18:00Z
  checked: src/jsonl_loader.h:717-761, src/jsonl_loader.h:562-609, src/jsonl_import_job.h:421-433, src/jsonl_sketch_import_job.h:408-417 and 553-560
  found: jsonl_parse_lines_chunk reads with fgets(4MB); jsonl_parse_entry false is ignored; callers only fail when parse.error != JSONL_OK. Thus truncated/invalid line can be skipped while job reaches COMPLETE and reports imported 0 elements.
  implication: Root-cause mechanism confirmed and affects both direct scene import and sketch import/reparse.

- timestamp: 2026-04-13T00:27:00Z
  checked: src/jsonl_loader.h and new src/tests/jsonl_loader_limits_test.c + src/CMakeLists.txt
  found: Implemented dynamic line reader used by quick scan, mesh quick scan, and incremental parse; parse-entry failures now set JSONL_ERROR_PARSE_ERROR. Added regression tests for >4MB single-line success and malformed-line explicit error.
  implication: Removes hard 4MB per-line cap and restores deterministic failure diagnostics instead of silent zero-entity completion.

- timestamp: 2026-04-13T00:32:00Z
  checked: Build + targeted tests
  found: Built targets jsonl_loader_limits_test, jsonl_sketch_import_test, jsonl_reparse_transaction_test; all three executables returned exit code 0.
  implication: Fix compiles cleanly and passes regression checks for large-line parsing and sketch import/reparse flows.

- timestamp: 2026-04-13T23:51:26+01:00
  checked: Human verification checkpoint response
  found: User confirmed fixed in real workflow/environment.
  implication: No remaining blocker from user-facing reproduction; safe to close session as resolved.

## Resolution
<!-- OVERWRITE as understanding evolves -->

root_cause: Incremental parser used fixed 4MB fgets line buffer and silently ignored jsonl_parse_entry failures, causing >4MB single-line JSONL entries to be truncated then skipped with no parse.error; downstream import paths treated this as success and produced zero entities.
fix: Replace fixed-cap fgets with dynamic realloc-backed line reader in all JSONL scan/parse paths; set JSONL_ERROR_PARSE_ERROR when non-empty line fails to parse so import/reparse returns explicit error instead of silent zero-result success.
verification: Built and ran targeted tests successfully: jsonl_loader_limits_test (new >4MB + explicit parse error checks), jsonl_sketch_import_test, jsonl_reparse_transaction_test (all exit 0). Human verification checkpoint response: confirmed fixed.
files_changed: [src/jsonl_loader.h, src/tests/jsonl_loader_limits_test.c, src/CMakeLists.txt]
