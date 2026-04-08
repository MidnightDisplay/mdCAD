---
phase: 23-principal-direction-constraint-expansion
plan: 01
subsystem: solver
tags: [constraints, along-axis, legality, descriptors, ctest]
requires:
  - phase: 22-solver-trigger-recalculate-determinism
    provides: deterministic scene-owned solver authority and transactional recalc contracts
provides:
  - ALONG X/Y/Z legality now accepts point-like descriptor signatures for pairs/groups
  - Shared legality authority remains centralized in constraint_type_is_selection_legal for UI/script/scene
  - Executable legality matrix coverage for standalone points, line endpoints, and arc landmarks
affects: [23-02, solver-runtime, constraint-authoring, script-apply]
tech-stack:
  added: []
  patterns: [descriptor-first legality matrix, scene-owned legality enforcement]
key-files:
  created: [.planning/phases/23-principal-direction-constraint-expansion/23-01-SUMMARY.md]
  modified:
    - src/constraints/constraint_types.h
    - src/ecs/ecs_scene.h
    - src/tests/endpoint_pick_test.c
    - src/tests/scene_solver_contract_test.c
key-decisions:
  - "Keep legacy single-line ALONG legality while expanding ALONG pair/group legality to point-like participant signatures."
  - "Allow ARC CENTER role in descriptor validation so scene-side legality can represent all Phase 23 point-like landmark participants."
patterns-established:
  - "Directional legality contract uses one source of truth: constraint_type_is_selection_legal."
  - "Scene descriptor creation tests guard legality drift between signature checks and scene API entrypoints."
requirements-completed: [AXIS-01, AXIS-02, AXIS-03, AXIS-04]
duration: 6m
completed: 2026-04-08
---

# Phase 23 Plan 01: Principal-direction legality expansion Summary

**ALONG X/Y/Z legality now supports descriptor-based point pairs/groups (including line/arc landmark roles) with executable scene+selection contract gates.**

## Performance

- **Duration:** 6m
- **Started:** 2026-04-08T10:33:40Z
- **Completed:** 2026-04-08T10:39:35Z
- **Tasks:** 2
- **Files modified:** 4

## Accomplishments
- Expanded ALONG legality from single-line-only to point-like descriptor signatures across 2..N participants.
- Preserved shared legality authority used by script apply (`sketch_script_apply_validate_model_links`) without introducing divergent rule tables.
- Added executable legality matrix coverage across endpoint pick and scene solver contract test targets with positive+negative directional signatures.

## Task Commits

1. **Task 1: Expand directional legality to descriptor-based point participant signatures** - `27732cb` (feat)
2. **Task 2: Add executable legality matrix tests for ALONG X/Y/Z mixed participant roles** - `5c73071` (test)

## Files Created/Modified
- `src/constraints/constraint_types.h` - Added point-like participant helpers and expanded ALONG X/Y/Z legality for pair/group descriptor signatures.
- `src/ecs/ecs_scene.h` - Allowed ARC CENTER descriptor roles in scene constraint creation; fixed encoded endpoint pick lookup resolution to endpoint entities.
- `src/tests/endpoint_pick_test.c` - Added D-01..D-06 tagged legality matrix tests for ALONG positive and negative signature contracts.
- `src/tests/scene_solver_contract_test.c` - Added scene-side descriptor ALONG creation tests for legal mixed signatures and illegal raw-entity signatures.

## Decisions Made
- Keep backward compatibility for existing single-line ALONG signatures while enabling point-like pair/group legality to satisfy AXIS expansion contracts.
- Keep legality centralized in `constraint_type_is_selection_legal` and validate scene descriptor entrypoints with executable tests instead of special-case script logic.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Restored encoded endpoint pick lookup contract in scene pick resolver**
- **Found during:** Task 1 verification
- **Issue:** `endpoint_pick` regression (`test_endpoint_pick_lookup_decodes_encoded_pick_ids`) surfaced because encoded endpoint pick IDs did not resolve through `ecs_scene_find_entity_by_pick_id`.
- **Fix:** Added encoded endpoint pick decode path in `ecs_scene_find_entity_by_pick_id` to map owner pick ID + role to native endpoint entity.
- **Files modified:** `src/ecs/ecs_scene.h`
- **Verification:** `ctest -R "endpoint_pick|scene_solver_contract" --test-dir build-vulkan -C Release --output-on-failure`
- **Committed in:** `27732cb`

---

**Total deviations:** 1 auto-fixed (Rule 1 bug)
**Impact on plan:** Required for deterministic test gate continuity; no scope creep beyond plan-coupled legality work.

## Issues Encountered
- Initial verification failed in `endpoint_pick` due to encoded endpoint pick lookup gap; fixed inline and reran task verification successfully.

## Known Stubs
None.

## Next Phase Readiness
- Phase 23-02 can implement ALONG runtime solve behavior with legality drift risk reduced by executable descriptor contract tests.
- Scene-owned solver authority/determinism from Phase 22 remains preserved and tested by unchanged solver contract/pass-policy gates.

## Self-Check: PASSED

