---
status: awaiting_human_verify
trigger: "Investigate issue: phase35-blank-script-editor"
created: 2026-04-12T00:00:00Z
updated: 2026-04-12T01:26:00Z
---

## Current Focus

hypothesis: Confirmed root cause: Apply crash came from stack overflow risk after raising parser caps, due to large on-stack model/workspace structs in apply/preview/commit functions.
test: Human end-to-end verification on phase35 JSONL import + Script Editor Apply workflow after heap-allocation fix.
expecting: Apply no longer crashes app; script applies successfully and imported geometry remains.
next_action: ask user to verify crash path is resolved in real UI workflow

## Symptoms

expected: Script text area should contain Lua script matching entities parsed from JSONL and imported into sketch.
actual: Script editor text area is completely blank after import; Apply clears sketch entities and then editor shows empty starter return statement.
errors: none visible.
reproduction: File -> Import JSONL as Sketch -> choose file/options -> sketch imported -> activate sketch -> open Script Editor -> blank text.
started: Previously script appeared truncated due to hard cap. After adaptive script buffer size change, it became fully blank.

## Eliminated

## Evidence

- timestamp: 2026-04-12T00:03:00Z
  checked: .planning/debug/knowledge-base.md pattern overlap
  found: No matching entry for blank script editor/imported JSONL after adaptive buffer.
  implication: Proceed with fresh hypothesis investigation.
- timestamp: 2026-04-12T00:04:00Z
  checked: commits aa6915d and 43dade2 changed files
  found: aa6915d only touched src/app.c for adaptive script buffer; 43dade2 touched JSONL observing defaults.
  implication: Primary suspect is src/app.c adaptive buffer change; observing-default commit likely unrelated to blank editor content.
- timestamp: 2026-04-12T00:10:00Z
  checked: src/app.c mdcad_script_editor_emit_complete + open/load flow
  found: emit_complete loops until scene_script_preview_parse succeeds; mdcad_script_editor_open_for_sketch clears text then calls load_emitted_script but ignores false return.
  implication: Any preview-parse failure in emit_complete leaves cleared blank editor text.
- timestamp: 2026-04-12T00:12:00Z
  checked: src/scripting/sketch_script_parse.h limits
  found: parser caps at SKETCH_SCRIPT_MODEL_MAX_ENTITIES=128 and SKETCH_SCRIPT_MODEL_MAX_CONSTRAINTS=128, returning errors when exceeded.
  implication: Large imported JSONL sketches can emit valid scripts that preview parser rejects, causing false failure in emit_complete.
- timestamp: 2026-04-12T00:14:00Z
  checked: apply behavior path in src/app.c and scene_script_apply_commit parse behavior
  found: blank editor text still parses as empty model; Apply uses editor text and can commit empty script, clearing sketch entities.
  implication: Explains observed “Apply clears imported entities then shows empty starter script” after blank-load regression.
- timestamp: 2026-04-12T00:20:00Z
  checked: src/app.c fix in mdcad_script_editor_emit_complete
  found: Replaced preview-parse gate with emitted-tail completeness check (`"  }\\n}\\n"`), preserving adaptive growth intent without parser-model coupling.
  implication: Editor load now accepts complete emitted scripts even when preview parser rejects for model-size limits.
- timestamp: 2026-04-12T00:22:00Z
  checked: src/tests/script_roundtrip_tests.c new regression
  found: Added test creating 140-point sketch; verifies emit succeeds with complete footer while preview fails with `too many entities in script model.`
  implication: Captures the exact mechanism behind blank-editor regression and guards against reintroducing parse-gated emit acceptance.
- timestamp: 2026-04-12T00:27:00Z
  checked: build and tests
  found: `cmake --build build --config Debug --target script_roundtrip_tests` succeeded; `build/bin/Debug/script_roundtrip_tests.exe` exited 0.
  implication: Fix compiles and regression suite (including new large-sketch case) passes.
- timestamp: 2026-04-12T00:43:00Z
  checked: Script Editor apply call path in src/app.c (draw window handlers)
  found: UI preview (`scene_script_preview_parse`) gates Apply button enabled state; Apply executes `scene_script_apply_commit` on same script text.
  implication: Any parse model limit failure blocks preview and also causes apply commit failure with identical diagnostics.
- timestamp: 2026-04-12T00:45:00Z
  checked: scene_script_preview_parse / scene_script_apply_commit and parse model internals
  found: Both preview and apply call `sketch_script_parse_model`; entities/constraints arrays are fixed at 128 and parse fails at guard checks in sketch_script_parse.h.
  implication: Root cause is centralized parser model capacity, not importer or UI-only behavior.
- timestamp: 2026-04-12T00:47:00Z
  checked: apply helper tables in src/scripting/sketch_script_apply.h
  found: link/label/pair tables derive capacities from same model max constants.
  implication: Raising model constants consistently scales apply pipeline without introducing truncation paths.
- timestamp: 2026-04-12T00:52:00Z
  checked: src/scripting/sketch_script_parse.h capacity constants
  found: Increased deterministic parser/apply capacities from 128 to 512 for entities and constraints.
  implication: Phase35 imported scripts with >128 entities now parse in both preview and apply paths.
- timestamp: 2026-04-12T00:53:00Z
  checked: src/tests/script_roundtrip_tests.c large-script coverage
  found: Replaced prior expectation test with `test_script_apply_large_sketch_within_model_limit` (140 points parse+apply+deterministic re-emit) and added `test_script_preview_rejects_over_model_limit_without_truncation` (MAX+8 still hard-fails with explicit diagnostic).
  implication: Confirms supported large size now works and over-limit behavior remains explicit (no silent truncation).
- timestamp: 2026-04-12T00:54:00Z
  checked: build and tests after capacity/test updates
  found: `cmake --build build --config Debug --target script_roundtrip_tests` succeeded; `build/bin/Debug/script_roundtrip_tests.exe` exited 0.
  implication: Fix compiles and targeted regression suite passes with new large-script behavior.
- timestamp: 2026-04-12T01:10:00Z
  checked: human verification checkpoint response
  found: User reports Apply is enabled after model-limit fix, but pressing Apply immediately crashes app on large imported JSONL sketch.
  implication: Prior root-cause/fix was incomplete; need new investigation on runtime crash in Apply execution path.
- timestamp: 2026-04-12T01:16:00Z
  checked: apply/preview/commit call chain (`sketch_script_apply.h`, `ecs_scene.h`) after model max increase
  found: `sketch_script_model_t` (512 entity/constraint arrays) and apply link/label/pair tables are allocated as local stack variables in multiple nested Apply/Preview functions.
  implication: Raising model caps significantly increased stack frame size; immediate UI Apply crash is consistent with stack overflow in commit path despite parser success.
- timestamp: 2026-04-12T01:23:00Z
  checked: fix implementation for apply workspace allocation
  found: moved large model/workspace tables from stack to heap in `sketch_script_apply_preview_model`, `sketch_script_apply_commit_model`, `sketch_script_apply_model_on_sketch`, and post-commit IO parse in `scene_script_apply_commit`.
  implication: removes stack-overflow failure mode while preserving deterministic limits and behavior.
- timestamp: 2026-04-12T01:23:00Z
  checked: regression coverage
  found: added `test_script_apply_handles_max_entity_model_without_crash` to apply emitted script with exactly `SKETCH_SCRIPT_MODEL_MAX_ENTITIES` entities and verify apply succeeds with full geometry retained.
  implication: guards crash class in commit path at configured max supported model size.
- timestamp: 2026-04-12T01:25:00Z
  checked: build/test verification after heap-allocation fix
  found: `cmake --build build --config Debug --target script_roundtrip_tests` succeeded; `build/bin/Debug/script_roundtrip_tests.exe` exited 0.
  implication: fix compiles and regression suite (including new max-entity apply crash guard) passes.
- timestamp: 2026-04-12T00:43:00Z
  checked: Script Editor apply call path in src/app.c (draw window handlers)
  found: UI preview (`scene_script_preview_parse`) gates Apply button enabled state; Apply executes `scene_script_apply_commit` on same script text.
  implication: Any parse model limit failure blocks preview and also causes apply commit failure with identical diagnostics.
- timestamp: 2026-04-12T00:45:00Z
  checked: scene_script_preview_parse / scene_script_apply_commit and parse model internals
  found: Both preview and apply call `sketch_script_parse_model`; entities/constraints arrays are fixed at 128 and parse fails at guard checks in sketch_script_parse.h.
  implication: Root cause is centralized parser model capacity, not importer or UI-only behavior.
- timestamp: 2026-04-12T00:47:00Z
  checked: apply helper tables in src/scripting/sketch_script_apply.h
  found: link/label/pair tables derive capacities from same model max constants.
  implication: raising model constants consistently scales apply pipeline without introducing truncation paths.

## Resolution

root_cause: After raising script model caps to 512, multiple apply-path functions still allocated large `sketch_script_model_t` and apply workspace tables on stack. Large imported script Apply then overflowed stack during commit path, causing immediate app crash.
fix: Moved large apply/preview/commit workspaces from stack to heap with explicit OOM handling in `src/scripting/sketch_script_apply.h` and `src/ecs/ecs_scene.h`; added regression `test_script_apply_handles_max_entity_model_without_crash` in `src/tests/script_roundtrip_tests.c` to apply at configured max entity count without crash.
verification: `cmake --build build --config Debug --target script_roundtrip_tests` succeeded; `build/bin/Debug/script_roundtrip_tests.exe` exited 0 with new crash-regression test passing. Pending user confirmation in full UI import/apply workflow.
files_changed: [src/scripting/sketch_script_apply.h, src/ecs/ecs_scene.h, src/tests/script_roundtrip_tests.c]
