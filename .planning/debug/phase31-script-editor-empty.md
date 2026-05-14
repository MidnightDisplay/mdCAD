---
status: awaiting_human_verify
trigger: "Investigate issue: phase31-script-editor-empty"
created: 2026-04-10T13:29:14.8777905+01:00
updated: 2026-04-10T14:10:00.0000000+01:00
---

## Current Focus

hypothesis: Scoped incremental script-ID assignment has addressed partial emit/update behavior without broad reclassification
test: finalize with human workflow verification in real app/editor interactions
expecting: user confirms existing and new sketches now show all entities/constraints and stay synced beyond first line
next_action: issue human-verify checkpoint with exact validation steps and self-test summary

## Symptoms

expected: Script editor should live-reflect sketch state (entities + constraints) during add/remove and geometry manipulations, same behavior as before Phase 31.
actual: Script editor always shows `return { entities = { }, constraints = { } }`.
errors: No visible errors in console/logs.
reproduction: Mix of (1) open existing sketch then edit and inspect script editor, and (2) create new sketch with 2 lines + 1 constraint then inspect script editor.
started: Worked before commit `0904d68` (Phase 31), regressed after.

## Eliminated

## Evidence

- timestamp: 2026-04-10T13:29:28.5780446+01:00
  checked: .planning/debug/knowledge-base.md
  found: Knowledge base match on [script editor, constraints, reflected, empty/no refresh] with entry phase13-script-editor-uat-regressions noting prior revision-bump and preview-path gaps.
  implication: Prior pattern is a hypothesis candidate (stale emit/revision disconnect), but current issue may be a new Phase 31-specific regression and needs direct code evidence.

- timestamp: 2026-04-10T13:29:44.6253198+01:00
  checked: git show --name-only 0904d68 and code search for empty template literal
  found: Phase 31 touched sketch_script_emit.h, parse/apply/contract/capability and ecs_scene; direct literal `return { entities = { }, constraints = { } }` is not present in src search.
  implication: Empty payload likely results from runtime emission path yielding zero collected entities/constraints rather than hardcoded static string.

- timestamp: 2026-04-10T13:30:11.9484482+01:00
  checked: src/scripting/sketch_script_emit.h and app.c symbol search
  found: sketch_script_emit_collect includes only sketch children with non-empty ScriptIdentityComp.script_local_id; app.c relies on scene_script_emit_for_sketch for editor and IO extraction.
  implication: If entities/constraints in sketch lack ScriptIdentityComp IDs, script editor will deterministically show empty entities/constraints with no error.

- timestamp: 2026-04-10T13:31:22.9131047+01:00
  checked: src/ecs/ecs_scene.h add/refresh paths and normalize function references
  found: scene_normalize_sketch_script_local_ids exists but is never called; scene_add_line/point/arc_to_sketch, scene_add_constraint_to_sketch_with_descriptors, and scene_refresh_sketch_metadata do not assign ScriptIdentityComp IDs.
  implication: Confirmed root cause mechanism—new/existing sketch entities can lack script_local_id, so Phase 31 emitter (which filters by ID) emits `entities={}` and `constraints={}`.

- timestamp: 2026-04-10T13:31:42.9286597+01:00
  checked: src/ecs/ecs_scene.h
  found: Added scene_normalize_sketch_script_local_ids(scene, sketch) at start of scene_refresh_sketch_metadata.
  implication: All flows that already refresh metadata now guarantee ScriptIdentityComp assignment/uniqueness, restoring emitter collection without broad behavior changes.

- timestamp: 2026-04-10T13:32:51.9366308+01:00
  checked: build + targeted ctest commands
  found: Targets script_roundtrip_tests, scene_solver_contract, and scene_solver_diagnostics build successfully; runtime test execution fails immediately with `Assertion failed: _sg.valid` inside sokol_gfx.h (environment/runtime init issue), not with script logic assertions.
  implication: Automated runtime verification is blocked by harness graphics init in this environment; need user-side workflow validation of the original regression in app.

- timestamp: 2026-04-10T14:01:00.0000000+01:00
  checked: user checkpoint response + current source snapshot (app.c + ecs_scene.h)
  found: Existing sketch still empty and new sketch shows only first line; app.c fallback normalizes IDs only when emitted script has no ids, while ecs_scene add-line/add-constraint paths do not assign IDs for later additions; scene_refresh_sketch_metadata currently does not normalize IDs.
  implication: Mechanism matches exactly: first line appears after one-time fallback normalization, subsequent geometry/constraints stay without IDs and are filtered out by emitter.

- timestamp: 2026-04-10T14:07:00.0000000+01:00
  checked: src/ecs/ecs_scene.h targeted patch
  found: Added scene_assign_script_local_id_if_needed() and invoked it in scene_attach_geometry_to_sketch and scene_add_constraint_to_sketch_with_descriptors; assignment is gated by scene_sketch_script_scope_enabled(), so only sketches already containing script-managed IDs receive incremental IDs for newly added geometry/constraints.
  implication: Fix targets partial emit root cause while avoiding broad reclassification of non-script/manual sketches.

- timestamp: 2026-04-10T14:08:00.0000000+01:00
  checked: src/tests/script_roundtrip_tests.c
  found: Added regression test test_script_emit_includes_incremental_manual_additions_after_script_activation covering script-seeded sketch + subsequent manual line/constraint addition + emit verification.
  implication: Prevents recurrence of “first entity only”/missing incremental constraint emission behavior.

- timestamp: 2026-04-10T14:10:00.0000000+01:00
  checked: required validation commands
  found: `cmake --build build-vulkan --config Release` succeeded; required ctest regex suite passed 7/7 (`script_roundtrip_tests`, `scene_solver_contract`, `scene_solver_pass_policy`, `scene_solver_diagnostics`, `scene_solver_trigger`, `scene_solver_drag`, `endpoint_pick`).
  implication: Build and targeted regression/contract suites validate fix and no observed collateral breakage in covered areas.

## Resolution

root_cause: Editor-open fallback can backfill script IDs once, but subsequent manual geometry/constraint additions do not receive IDs; emitter includes only entities with IDs, causing partial output (first line only, missing later lines/constraints).
fix: Added scoped incremental script-local-id assignment in scene attach/create paths (only when sketch already has script-managed children), plus regression test proving post-activation manual additions are emitted.
verification: Required build and targeted ctest suite passed (7/7); pending user workflow confirmation in Script Editor for existing/new sketch scenarios.
files_changed: [src/ecs/ecs_scene.h, src/tests/script_roundtrip_tests.c]
