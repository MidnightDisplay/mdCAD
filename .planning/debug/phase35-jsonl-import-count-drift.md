---
status: awaiting_human_verify
trigger: "Investigate issue: phase35-jsonl-import-count-drift."
created: 2026-04-13T00:00:00Z
updated: 2026-04-13T01:16:00Z
---

## Current Focus

hypothesis: Confirmed and fixed: count drift came from high-water instance slot counting, not live entities.
test: validated by targeted executable runs and new repeated-reparse regression.
expecting: scene totals remain constant across unchanged-equivalent reparses.
next_action: human verification in real UI workflow.

## Symptoms

expected: Import summary label "Imported JSONL as Sketch\nTotal: X lines, Y points" should reflect actual current entities from latest sketch content. Re-parsing same JSONL should keep totals constant.
actual: Totals increment by full reparse entity counts after each observer reparse, even for unchanged-equivalent content.
errors: none
reproduction: import JSONL as sketch; observe label; allow/trigger observer reparse; totals keep increasing.
started: observed after phase 35 changes while testing observer/reparse.

## Eliminated

## Evidence

- timestamp: 2026-04-13T00:00:00Z
  checked: mandatory files_to_read
  found: Read ui_scene_hierarchy/jsonl_sketch_import_job/jsonl_observer_system/jsonl_observer_comp and targeted tests.
  implication: Enough context loaded to trace import/reparse count display and metadata flow.
- timestamp: 2026-04-13T00:52:00Z
  checked: .planning/debug/knowledge-base.md
  found: no matching prior resolved pattern for count drift/reparse total inflation.
  implication: continue fresh hypothesis testing.
- timestamp: 2026-04-13T00:56:00Z
  checked: src/ui/ui_scene_hierarchy.h and src/ecs/ecs_scene.h
  found: UI "Total: %d lines, %d points" uses ecs_scene_line_count/ecs_scene_point_count, which currently return instance_buffer_count for line/point buffers.
  implication: displayed totals depend on instance buffer counting semantics, not sketch metadata snapshot.
- timestamp: 2026-04-13T00:58:00Z
  checked: src/gpu/instance_buffer.h free/count behavior
  found: instance_buffer_free_slot marks slot free and clears entity mapping but does not decrement ib->count; ib->count is high-water allocated slots.
  implication: counts based on instance_buffer_count are cumulative across create/remove cycles, matching observed drift under transactional reparses.
- timestamp: 2026-04-13T01:04:00Z
  checked: source fixes in instance_buffer/ecs_scene
  found: added instance_buffer_live_count() and changed ecs_scene_line_count/point_count to use live slot count instead of high-water allocation count.
  implication: UI totals now reflect current entity snapshot, not cumulative historical allocations.
- timestamp: 2026-04-13T01:07:00Z
  checked: src/tests/jsonl_reparse_transaction_test.c
  found: added regression test `test_jsonl_reparse_repeated_keeps_live_scene_totals_stable` performing 4 transactional reparses and asserting sketch geometry=2, scene lines=1, scene points=1 each iteration.
  implication: automated guard added for stable totals across repeated reparses.
- timestamp: 2026-04-13T01:12:00Z
  checked: targeted build + ctest run
  found: build succeeded; ctest invocation failed for two tests with sokol `_sg.valid` assertion (test harness context issue), not logic assertions.
  implication: ctest runner environment is not reliable for these binaries in current setup; run executables directly for deterministic result.
- timestamp: 2026-04-13T01:14:00Z
  checked: direct executable runs for targeted tests
  found: jsonl_sketch_import_test=0, jsonl_observer_state_test=0, jsonl_reparse_transaction_test=0.
  implication: targeted regressions pass, including new repeated-reparse stable totals test.

## Resolution

root_cause: Scene totals used `instance_buffer_count()` (allocated slot high-water mark) instead of live occupied slots. Transactional JSONL reparses free old entities and create new ones; freed slots remain in `ib->count`, so UI totals drift upward cumulatively after each reparse.
fix: Added live-count API for instance buffers and switched scene line/point totals to live-slot counting; added regression test for repeated reparses keeping totals stable.
verification: Built targeted tests successfully. ctest harness showed sokol gfx-context assertion in two tests, so binaries were executed directly; all targeted executables exited 0, and new repeated-reparse test confirms totals stay constant across multiple reparses.
files_changed: [src/gpu/instance_buffer.h, src/ecs/ecs_scene.h, src/tests/jsonl_reparse_transaction_test.c]
