---
status: awaiting_human_verify
trigger: "Investigate issue: phase35-observer-reparse-loop."
created: 2026-04-13T00:00:00Z
updated: 2026-04-13T00:42:00Z
---

## Current Focus

hypothesis: Fixed: observer now skips unchanged source using metadata-first + hash-confirm gate; linked import defaults to Observe ON (opt-out).
test: run targeted JSONL observer/import tests and request human UI verification of loop absence and default toggle behavior.
expecting: unchanged file produces no observer reparse; changed file reparses transactionally; import creates linked sketch with observe enabled by default.
next_action: checkpoint for human verification in real UI workflow

## Symptoms

expected: When linked JSONL file has not changed, observer should skip reparse entirely and leave sketch untouched. User should feel in full control; no silent edits/regeneration.
actual: At every rate-limit interval, sketch appears recreated from scratch; entity IDs and line labels change continuously despite no file edits.
errors: None visible; app appears normal while looping.
reproduction: Launch mdCAD -> File -> Import JSONL as Sketch -> pick file -> choose import options and import -> do nothing. Entities keep changing IDs at rate limiter frequency.
started: Present since Phase 35 implementation; observed immediately during human testing.

## Eliminated

## Evidence

- timestamp: 2026-04-13T00:08:00Z
  checked: .planning/debug/knowledge-base.md keyword overlap for observer/reparse/JSONL
  found: no matching resolved pattern for continuous JSONL observer reparse loop.
  implication: proceed with fresh hypothesis investigation.
- timestamp: 2026-04-13T00:12:00Z
  checked: src/jsonl_observer_system.h jsonl_observer_tick_one
  found: tick path rate-limits by next_retry_at_ms but always calls jsonl_sketch_reparse_transactional once interval elapses; no file unchanged detection before reparse.
  implication: observer is structurally guaranteed to rebuild sketch each interval even when source file is unchanged.
- timestamp: 2026-04-13T00:16:00Z
  checked: src/jsonl_sketch_import_job.h transactional reparse implementation
  found: successful reparse removes prior children and recreates geometry, then script re-emits; this naturally changes entity IDs/labels repeatedly.
  implication: explains symptom of apparent sketch recreation at cadence despite no visible errors.
- timestamp: 2026-04-13T00:18:00Z
  checked: src/jsonl_sketch_import_job.h import defaults and src/ui/ui_scene_hierarchy.h import copy text
  found: import explicitly sets observer.observe_enabled=false and UI text says Observe is OFF by default.
  implication: conflicts with requested opt-out default ON behavior.
- timestamp: 2026-04-13T00:21:00Z
  checked: existing tests (jsonl_sketch_import_test/jsonl_observer_state_test/jsonl_reparse_transaction_test)
  found: current coverage lacks unchanged-file gate behavior and currently asserts default Observe OFF in sketch import test.
  implication: tests must be updated/extended to guard requested behavior and prevent regression.
- timestamp: 2026-04-13T00:31:00Z
  checked: src/jsonl_sketch_import_job.h + src/jsonl_observer_system.h implementation changes
  found: added persisted source snapshot fields and helpers; observer tick now checks source change before reparse (metadata fast-path; hash confirmation when metadata differs); import default changed to observe_enabled=true.
  implication: removes unconditional rebuild loop while preserving transactional reparse on actual source changes; aligns default behavior with opt-out observe.
- timestamp: 2026-04-13T00:33:00Z
  checked: src/scene_serializer.h + src/ui/ui_scene_hierarchy.h
  found: serializer now saves/loads observer source snapshot fields; UI import copy updated from Observe OFF to Observe ON by default.
  implication: source-change gating state survives scene persistence and user-facing contract matches implementation.
- timestamp: 2026-04-13T00:36:00Z
  checked: src/tests/jsonl_reparse_transaction_test.c and src/tests/jsonl_sketch_import_test.c
  found: added regression tests for observer unchanged-skip and changed-reparse; updated import test expectation to default Observe ON.
  implication: automated coverage now guards both root-cause fix and default behavior requirement.
- timestamp: 2026-04-13T00:41:00Z
  checked: targeted build/test execution
  found: build target jsonl_sketch_import_test/jsonl_observer_state_test/jsonl_reparse_transaction_test succeeded; all three test executables exit code 0.
  implication: fix compiles and targeted regressions pass.

## Resolution

root_cause: Observer tick reparsed transactionally at every interval without checking whether the linked JSONL source content changed. Since reparse is authoritative replace, unchanged files still caused child entity recreation (new IDs/labels) each cadence. Additionally, JSONL sketch import forced observe_enabled=false, conflicting with desired opt-out observe behavior.
fix: Added robust source-change gate in observer flow using metadata-first detection (size + mtime) with hash confirmation fallback (FNV-1a 64-bit) before reparse. Persisted source snapshot state in JsonlObserverComp and scene serializer. On successful import/reparse, stamp source snapshot. Updated JSONL sketch import default to observe_enabled=true and adjusted UI copy. Added regression tests for unchanged skip, changed reparse, and default observe ON.
verification: Built and ran targeted tests: jsonl_sketch_import_test=0, jsonl_observer_state_test=0, jsonl_reparse_transaction_test=0.
files_changed: [src/components/jsonl_observer_comp.h, src/jsonl_sketch_import_job.h, src/jsonl_observer_system.h, src/scene_serializer.h, src/ui/ui_scene_hierarchy.h, src/tests/jsonl_reparse_transaction_test.c, src/tests/jsonl_sketch_import_test.c]
