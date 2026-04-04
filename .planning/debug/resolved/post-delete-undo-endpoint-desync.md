---
status: resolved
trigger: "Investigate issue: post-delete-undo-endpoint-desync"
created: 2026-04-04T23:59:30.4164635+01:00
updated: 2026-04-05T00:17:15.4614829+01:00
---

## Current Focus
hypothesis: Confirmed root cause fixed; user verified end-to-end UI behavior is restored.
test: Finalize archive flow after human verification checkpoint and persist session to resolved history.
expecting: Session is marked resolved, archived, and documented in knowledge base with fix/test evidence.
next_action: Commit code changes and archive debug session + knowledge base entry.

## Symptoms
expected: Undo restore should recreate sketch geometry entity as before, with endpoints synced to owner geometry landmarks (line A/B; arc center/start/end). Endpoint gizmo movement should change GeometryComp point values, not endpoint TransformComp position.
actual: After UI delete + undo restore, endpoint drags behave as transform moves and no longer drive owner geometry landmarks.
errors: No visible errors/logs.
reproduction: (1) create sketch line/arc (2) delete via GeometryManager UI Delete button (3) undo delete (4) drag endpoint gizmo (5) observe desync and transform-driven movement.
started: Regression started just after Phase 18 completion.

## Eliminated

- hypothesis: Bulk delete undo restore path always loses EndPointsComp metadata causing all restored endpoints to become generic points.
  evidence: Newly added regression test test_bulk_delete_undo_restores_endpoint_owner_sync_metadata passes, confirming metadata and endpoint-driven owner geometry movement are intact in generic bulk delete undo path.
  timestamp: 2026-04-05T00:03:07.0003636+01:00

## Evidence

- timestamp: 2026-04-04T23:59:43.2829772+01:00
  checked: .planning/debug/knowledge-base.md pattern overlap
  found: No matching prior resolved entry for endpoint delete+undo desync pattern.
  implication: Proceed with fresh hypothesis formation; no known quick pattern match available.

- timestamp: 2026-04-04T23:59:43.2829772+01:00
  checked: recent git history around Phase 18
  found: Multiple endpoint/undo commits, including "fix(18-add-undo-steps-for-endpoint-moves-03): harden geometry manager delete + selection sync" at 559f43d.
  implication: Phase 18 delete+selection hardening commit is high-priority suspect for UI delete + undo regression.

- timestamp: 2026-04-04T23:59:55.7406179+01:00
  checked: commit 559f43d diff and touched files
  found: Changes are in selection pruning/sync and GeometryManager UI selection behavior; no direct endpoint binding reconstruction logic changed.
  implication: Desync likely not caused by selection sync code directly; must inspect delete/undo serialization and endpoint metadata restoration paths.

- timestamp: 2026-04-05T00:00:50.4413581+01:00
  checked: undo snapshot/restore + endpoint sync code (undo_redo_exec.h, ecs_scene.h)
  found: undo_snapshot_entity captures geometry/transform/renderable/constraint/sketch state but does not capture EndPointsComp; undo_create_from_snapshot restores components but does not restore EndPointsComp; bulk delete undo uses these APIs.
  implication: Restored endpoint point entities lose endpoint-owner metadata, matching symptom where endpoint drags behave like generic transform moves.

- timestamp: 2026-04-05T00:01:39.2575513+01:00
  checked: targeted regression test addition in src/tests/endpoint_pick_test.c
  found: Added test_bulk_delete_undo_restores_endpoint_owner_sync_metadata reproducing UI-equivalent flow (bulk delete sketch line, undo, verify endpoint metadata + drag behavior via owner geometry mutation and zero endpoint transform drift).
  implication: Provides direct falsifiable guard for suspected restore-path metadata loss.

- timestamp: 2026-04-05T00:01:39.2575513+01:00
  checked: build invocation cmake --build build-vs-probe --target endpoint_pick
  found: Failed because build-vs-probe is not an initialized CMake build dir (missing CMakeCache.txt).
  implication: Need configure/build in valid directory before running regression tests.

- timestamp: 2026-04-05T00:03:07.0003636+01:00
  checked: endpoint_pick build+run in build-vulkan after adding regression test
  found: endpoint_pick target builds and full suite (including new bulk delete + undo endpoint sync test) passes.
  implication: Initial hypothesis (bulk-delete undo always dropping endpoint metadata) is likely incomplete/incorrect; issue likely specific to UI GeometryManager delete flow rather than generic undo restore.

- timestamp: 2026-04-05T00:03:07.0003636+01:00
  checked: scene_set_parent and endpoint sync code contracts
  found: scene_set_parent preserves local geometry coordinates, and scene_sync_endpoint_entities_for_owner only runs when explicitly invoked; not automatically triggered by reparent.
  implication: Desync may stem from restoring endpoint point local coordinates from stale world-space snapshots during UI flow, requiring a post-restore endpoint-owner resync step.

- timestamp: 2026-04-05T00:14:34.9807247+01:00
  checked: endpoint pick lookup path (app click -> ecs_scene_find_entity_by_pick_id)
  found: viewport clicks use raw pick_id lookup; endpoint_pick_encode/decode exists, but ecs_scene_find_entity_by_pick_id previously did not decode encoded endpoint pick IDs.
  implication: After delete+undo, endpoint interaction can route incorrectly unless encoded endpoint IDs map back to endpoint entities.

- timestamp: 2026-04-05T00:14:34.9807247+01:00
  checked: undo snapshot/create data model
  found: undo_entity_snapshot_t lacked EndPointsComp capture/restore before patch; bulk delete restore remapped parents/constraints but not endpoint owner/binding IDs.
  implication: recreated entity ID remap could leave endpoint linkage inconsistent after undo in ID-changing restore paths.

- timestamp: 2026-04-05T00:14:34.9807247+01:00
  checked: code fix in undo + tests
  found: Added EndPointsComp fields to undo snapshots, persisted in undo_snapshot_entity/undo_create_from_snapshot, remapped endpoint owner and owner binding endpoint_entity IDs in undo_restore_bulk_entity_relationships; added regressions for bulk delete undo line/arc endpoint sync and endpoint pick mapping.
  implication: Restored entities now preserve endpoint-owner contract and pick routing after bulk delete undo.

- timestamp: 2026-04-05T00:14:34.9807247+01:00
  checked: verification runs
  found: endpoint_pick.exe passes after new tests; scene_solver_contract.exe and script_roundtrip_tests.exe also pass (built/run from build-vulkan Debug).
  implication: Targeted fix holds for endpoint regressions and does not break solver/script transaction related suites.

- timestamp: 2026-04-05T00:17:15.4614829+01:00
  checked: human verification checkpoint response
  found: User confirmed fixed in real workflow/environment.
  implication: End-to-end issue resolved; session can be archived as resolved.

## Resolution
root_cause: Bulk delete undo restore did not explicitly preserve/remap endpoint linkage metadata in undo snapshots, and endpoint pick lookup path did not decode encoded endpoint pick IDs. In delete+undo flows this could leave endpoint interactions desynced from owner geometry semantics.
fix: Extended undo_entity_snapshot_t to include EndPointsComp and restored it in undo_create_from_snapshot; added endpoint owner/binding ID remap in undo_restore_bulk_entity_relationships. Added regression coverage for bulk delete undo line/arc endpoint sync and restored endpoint pick mapping.
verification: Built and ran endpoint_pick suite (including new regressions), plus scene_solver_contract and script_roundtrip_tests binaries in build-vulkan Debug; all passed.
files_changed: [src/undo_redo.h, src/undo_redo_exec.h, src/tests/endpoint_pick_test.c]
