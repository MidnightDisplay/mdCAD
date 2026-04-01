---
status: resolved
trigger: "Investigate issue: phase13-script-editor-uat-regressions"
created: 2026-04-01T23:17:39.8508919+01:00
updated: 2026-04-01T23:33:40.4701768+01:00
---

## Current Focus
<!-- OVERWRITE on each update - reflects NOW -->

hypothesis: Resolved — human verification confirmed the original UAT regressions are fixed.
test: Finalize archive workflow (move session file, update knowledge base, commit code/docs updates).
expecting: Debug session and documentation state are consistent with a resolved find_and_fix workflow.
next_action: archive this session and report closure

## Symptoms
<!-- Written during gathering, then IMMUTABLE -->

expected: Parser/preview/apply behavior must align with D-05/D-06/D-07. Invalid scripts should produce clear diagnostics and must not silently block subsequent valid edits. Deterministic UI->script refresh should include geometry edits (including moved line vertices) and preserve stable output. Constraint participant edits in script should apply and reflect in sketch workspace when valid. Illegal constraint participant/type combinations should fail explicitly with diagnostics (not silent no-op), and should not poison later applies. Reset/discard dirty-state should not get stuck due to failed-apply bookkeeping bugs. Script editor text interactions should support expected copy/paste shortcuts.
actual: 1) Script window opens. 2) Some syntax errors raise parse diagnostics; some random letters between curly-braced blocks do not. Apply removes those. 3) Changing length in script updates scene and vice versa, but moving line vertices in UI does not update corresponding line point xyz in script; script->apply does update geometry. 4) Changing constraint participants in script does not update screen/sketch workspace. 5) Adding a third point 'c' does not raise an issue. 6) Illegal constraint edit (e.g., swapping line for arc in line-line perpendicular) shows no issue, does not update geometry, then silently blocks other changes. 7) Reset/discard work unless issue (6) occurs; then discard prompt repeats because last changes never applied. 8) Copy/paste shortcut support requested (Ctrl+C/Ctrl+V, Cmd on mac).
errors: Missing/partial diagnostics for invalid script forms. Silent failure/no-op apply for illegal constraints. Stale/blocked apply state after invalid edit path.
reproduction: Use 13-03 checkpoint flow: open Script Editor from SketchManager, mutate script with syntax/reference/participant/type-invalid edits, apply, then perform UI-side geometry edits and observe script refresh + dirty/discard behavior.
started: These are new features introduced in Phase 13 and discovered during initial human verification.

## Eliminated
<!-- APPEND only - prevents re-investigating -->

## Evidence
<!-- APPEND only - facts discovered -->

- timestamp: 2026-04-01T23:18:40+01:00
  checked: .planning/debug/knowledge-base.md
  found: No matching prior debug pattern for Phase 13 script editor symptoms (only unrelated phase5 entry).
  implication: Proceed with fresh hypothesis investigation.

- timestamp: 2026-04-01T23:18:40+01:00
  checked: repository text search for script editor/parse/apply/constraint/dirty
  found: Script Editor UI and apply/dirty popup logic are concentrated in src/app.c (window + preview parse + apply + reset/close flow).
  implication: app.c is a primary locus for symptoms involving apply, dirty/discard, and keyboard shortcuts; parser/apply engine likely elsewhere.

- timestamp: 2026-04-01T23:19:32+01:00
  checked: src/app.c script editor window code (lines ~180-317)
  found: InputTextMultiline uses only ImGuiInputTextFlags_AllowTabInput (no Ctrl/Cmd shortcut flag); apply button is disabled only by preview parse result and resets apply_requested each frame.
  implication: Copy/paste shortcut regression is likely directly in app.c flags; apply/dirty anomalies likely originate deeper in scene_script_apply_commit behavior.

- timestamp: 2026-04-01T23:19:32+01:00
  checked: symbol search for scene_script_* implementation and tests
  found: Core script behavior lives in inline headers under src/scripting/*.h with coverage in src/tests/script_roundtrip_tests.c.
  implication: Need code+test inspection there to map UAT mismatches to specific validation/serialization gaps.

- timestamp: 2026-04-01T23:21:20+01:00
  checked: src/scripting/sketch_script_parse.h
  found: parse_model only parses entities/constraints via strstr/brace scan and returns true without checking for extraneous tokens outside parsed blocks.
  implication: Random letters between blocks can be silently ignored then removed after apply/re-emit, matching UAT symptom #2.

- timestamp: 2026-04-01T23:21:20+01:00
  checked: src/scripting/sketch_script_apply.h + sketch_script_contract.h
  found: Contract only enforces participant_count <= max; apply validates minimum + type legality but not explicit maximum per constraint type and produces generic errors.
  implication: Extra participant cases (e.g., third point) may pass until downstream no-op/failure paths; diagnostics likely insufficiently explicit.

- timestamp: 2026-04-01T23:21:20+01:00
  checked: src/scripting/sketch_script_emit.h + app.c
  found: Script Editor refresh depends on scene_script_emit_revision() changes; app reloads text only when revision changes and no unsaved edits.
  implication: If geometry edits (vertex drag) fail to increment emit revision, script won't refresh despite scene changes (UAT symptom #3).

- timestamp: 2026-04-01T23:27:10+01:00
  checked: src/app.c drag update flow and src/undo_redo_exec.h geometry vertex command execution
  found: geometry vertex moves mutate GeometryComp directly (app.c + CMD_SET_GEOMETRY_VERTICES) without scene_script_reemit_for_sketch call.
  implication: Script Editor polling on script_emit_revision misses line-vertex drags/undo-redo vertex edits, causing stale script text after UI geometry edits.

- timestamp: 2026-04-01T23:27:10+01:00
  checked: src/scripting/sketch_script_apply.h preview function + src/app.c diagnostics logic
  found: preview only parses text (no link/type legality validation); apply failure message is overwritten/hidden because app clears errors when preview parses OK and diagnostics pane only displays parse errors.
  implication: Illegal constraint edits can appear as silent no-op and leave dirty/discard loops, matching UAT symptoms #5/#6/#7.

- timestamp: 2026-04-01T23:32:00+01:00
  checked: code changes in sketch_script_parse/apply, app.c, and script_roundtrip_tests.c
  found: Added strict delimiter checks in entities/constraints parser blocks, preview now runs link/type legality validation, diagnostics now persist apply failures, and geometry vertex drag end now triggers sketch script re-emit; added tests for illegal preview participants and unexpected tokens.
  implication: Core symptom paths now have explicit validation and feedback, and UI->script refresh should include moved line vertices.

- timestamp: 2026-04-01T23:34:40+01:00
  checked: cmake --build build-vulkan --config Release --target script_roundtrip_tests && ctest -R script_roundtrip_tests
  found: script_roundtrip_tests builds and passes, including new regression tests.
  implication: Parser strictness and preview legality checks are verified by automated coverage.

- timestamp: 2026-04-01T23:34:40+01:00
  checked: cmake --build build-vulkan --config Release --target mdCAD
  found: Initial link failed due to running mdCAD.exe lock (PID 59444); after stopping process with Stop-Process -Id 59444, mdCAD rebuilt successfully.
  implication: App-level code changes compile cleanly; interactive behavior ready for manual verification.

## Resolution
<!-- OVERWRITE as understanding evolves -->

root_cause: Phase 13 implementation had multiple gaps: parser tolerated non-delimiter junk inside script blocks, preview only performed syntactic parse (not reference/type legality validation), Script Editor diagnostics UI suppressed apply failures when preview remained parse-OK, and geometry vertex drag path did not increment script emit revision so editor text stayed stale.
fix: Enforced strict token delimiters inside entities/constraints blocks; made preview run full model link/type legality validation; preserved/displayed apply-failure diagnostics until user edits or successful apply; and triggered scene_script_reemit_for_sketch after geometry-mode vertex drags complete. Added regression tests for illegal constraint participant preview rejection and unexpected-token parse rejection.
verification: Automated verification passed (script_roundtrip_tests with new regression cases + mdCAD build), and human checkpoint response confirmed fixed in real workflow.
files_changed: [src/scripting/sketch_script_parse.h, src/scripting/sketch_script_apply.h, src/app.c, src/tests/script_roundtrip_tests.c]
