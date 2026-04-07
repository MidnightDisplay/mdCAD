---
phase: 17
slug: constraint-driven-geometry-solving
artifact: verification
status: draft
updated: 2026-04-07
authoritative: true
---

# Phase 17 Verification Matrix (Authoritative)

This file is the authoritative requirement-level verification artifact for Phase 17 closure.

## Scope

- Requirements covered: `D-01..D-12`
- Closure policy: citation-first reuse; escalate to targeted rerun only when evidence is ambiguous.
- Supporting artifacts:
  - `.planning/phases/17-constraint-driven-geometry-solving/17-VALIDATION.md`
  - `.planning/phases/17-constraint-driven-geometry-solving/17-UAT.md`
  - `.planning/phases/17-constraint-driven-geometry-solving/17-05-SUMMARY.md`
  - `.planning/phases/17-constraint-driven-geometry-solving/17-04-SUMMARY.md`

## Requirement Matrix

| Req | Requirement text | Status | Implementation anchor | Automated evidence | Manual evidence | Source citations | Disposition notes |
|---|---|---|---|---|---|---|---|
| D-01 | Recalculate validates full active sketch constraints before committing geometry updates. | Passed | `scene_solver_*` transaction path (Phase 17 baseline) | `scene_solver_contract` transactional solve fixture mapping exists and fresh targeted rerun passed. | N/A | Citation: `17-VALIDATION.md` → “Decision-to-Evidence Traceability (D-01..D-12)” row `D-01`; `17-VALIDATION.md` → “Per-Task Verification Map” row `17-01-01`; `17-VERIFICATION.md` → “Targeted Rerun Evidence (2026-04-07)” command #2 (`scene_solver_contract` pass). | Ambiguity resolved by targeted rerun; no conflicting evidence found. |
| D-02 | Successful solve attempts apply resulting geometry updates atomically and immediately. | Passed | Transactional solve/apply contract path | `scene_solver_contract` commit-on-success assertions mapped in existing validation strategy and fresh targeted rerun passed. | N/A | Citation: `17-VALIDATION.md` → “Decision-to-Evidence Traceability (D-01..D-12)” row `D-02`; `17-VALIDATION.md` → “Per-Task Verification Map” row `17-01-01`; `17-VERIFICATION.md` → “Targeted Rerun Evidence (2026-04-07)” command #2 (`scene_solver_contract` pass). | Ambiguity resolved by targeted rerun; no conflicting evidence found. |
| D-03 | Drag interactions use live constrained solve projection when satisfiable. | Passed | Live drag projection path (`scene_solver_drag`) | `scene_solver_drag` feasible projection assertions mapped in validation strategy and fresh targeted rerun passed. | N/A | Citation: `17-VALIDATION.md` → “Decision-to-Evidence Traceability (D-01..D-12)” row `D-03`; `17-VALIDATION.md` → “Per-Task Verification Map” row `17-02-01`; `17-VERIFICATION.md` → “Targeted Rerun Evidence (2026-04-07)” command #2 (`scene_solver_drag` pass). | Ambiguity resolved by targeted rerun; no conflicting evidence found. |
| D-04 | Unsatisfiable drag keeps last valid solved state and surfaces immediate feedback. | Passed | Unsat drag no-mutation contract + implication feedback path | `scene_solver_drag` unsat no-mutation assertions mapped; fresh targeted rerun passed. | Manual unsat interaction check recorded as passing in UAT set. | Citation: `17-VALIDATION.md` → traceability row `D-04`; `17-VALIDATION.md` → “Manual-Only Verifications” row for `D-04,D-05`; `17-UAT.md` → Tests summary `7/7` pass and Test 1 coverage baseline; `17-VERIFICATION.md` → “Targeted Rerun Evidence (2026-04-07)” command #2 (`scene_solver_drag` pass). | Ambiguity resolved via targeted rerun + canonical UAT baseline confirmation. |
| D-05 | Contradictory solves fail deterministically with no scene mutation and stable implication payload. | Passed | Deterministic implication/no-mutation contract path | `scene_solver_contract` + `scene_solver_diagnostics` implication ordering and no-mutation mappings exist; fresh targeted rerun passed. | Manual failure-clarity check included in prior UAT baseline. | Citation: `17-VALIDATION.md` → traceability row `D-05`; `17-VALIDATION.md` → “Gap-Closure Remediation Evidence” solver command set; `17-UAT.md` → Tests summary `7/7` pass; `17-VERIFICATION.md` → “Targeted Rerun Evidence (2026-04-07)” command #2 (`scene_solver_contract` + `scene_solver_diagnostics` pass). | Ambiguity resolved via targeted rerun and stable prior manual baseline evidence. |
| D-06 | Solver diagnostics dedupe identical consecutive entries while preserving append order. | Passed | Diagnostics ring buffer + dedupe policy | `scene_solver_diagnostics` dedupe ordering fixture mapped in validation strategy/task map and fresh targeted rerun passed. | Inspector diagnostics behavior covered by prior manual checks. | Citation: `17-VALIDATION.md` → traceability row `D-06`; `17-VALIDATION.md` → per-task row `17-03-02`; `17-VERIFICATION.md` → “Targeted Rerun Evidence (2026-04-07)” command #2 (`scene_solver_diagnostics` pass). | Ambiguity resolved by targeted rerun; no conflicting evidence found. |
| D-07 | Live solve path uses bounded per-frame budget with graceful degradation contract. | Passed | Bounded projected-delta path in constrained drag solver | `scene_solver_drag` bounded projection expectation (`projected_delta` clamp) mapped in validation strategy and fresh targeted rerun passed. | N/A | Citation: `17-VALIDATION.md` → traceability row `D-07`; `17-VALIDATION.md` → per-task row `17-02-01`; `17-VERIFICATION.md` → “Targeted Rerun Evidence (2026-04-07)” command #2 (`scene_solver_drag` pass). | Ambiguity resolved by targeted rerun; no conflicting evidence found. |
| D-08 | Deterministic solver regression fixtures are wired as automated acceptance gates. | Passed | CTest target registration + aggregate regression gate | Aggregate command (`scene_solver_contract|scene_solver_drag|endpoint_pick|scene_solver_diagnostics`) recorded as pass in validation evidence; solver subset rerun freshly passed for D-01..D-08 closure. | N/A | Citation: `17-VALIDATION.md` → traceability row `D-08`; `17-VALIDATION.md` → executed evidence command list with aggregate gate; `20-VALIDATION.md` → Per-Task map row `20-01-01`; `17-VERIFICATION.md` → “Targeted Rerun Evidence (2026-04-07)” command #2. | Ambiguity resolved with fresh targeted rerun and existing aggregate gate citation continuity. |
| D-09 | Constraint participant representation supports endpoint/sub-entity metadata. | Passed | `EndPointsComp` owner/role/sub-index metadata on sketch-scoped native endpoint entities (`src/components/endpoints_comp.h`, `src/ecs/ecs_scene.h`) | `endpoint_pick` metadata/entity-resolution path validated by `test_endpoint_pick_native_endpoint_entity_resolution`, `test_endpoint_pick_lookup_decodes_encoded_pick_ids`, and endpoint-owner sync guard regressions; targeted rerun passed (`ctest -R endpoint_pick ...`). | Canonical endpoint manual baseline passed (`17-UAT.md` tests 1-7, with direct-selection anchor in Test 1 and sync anchor in Test 5). | Citation: `17-04-SUMMARY.md` → one-liner + “Accomplishments” bullets 1-3 + key-files (`EndPointsComp` and `ecs_scene` endpoint contract); `17-05-SUMMARY.md` → “Follow-up remediation pass” endpoint sync notes + regression test addition; `src/tests/endpoint_pick_test.c` → `test_endpoint_pick_native_endpoint_entity_resolution`, `test_endpoint_direct_geometry_edit_syncs_owner_and_entities`; `17-UAT.md` → Tests 1-7 summary (`7/7` pass). | Reused evidence was sufficient but previously pending bookkeeping; targeted endpoint rerun in this plan removed ambiguity and finalized closure. |
| D-10 | Endpoint controls are first-class selectable/pickable participants for Coincident authoring. | Passed | Endpoint point selection + point-context legality filtering in constraint menu flow (`src/app.c` selection routing + legality contract in `constraint_types.h`) | `endpoint_pick` legality/selection tests cover endpoint-first context behavior: `test_endpoint_point_context_filters_line_only_constraints`, `test_endpoint_point_context_keeps_coincident_for_endpoint_pairs`, plus overlay selection tests; targeted rerun passed (`ctest -R endpoint_pick ...`). | Canonical manual baseline passed (`17-UAT.md` tests 1-7), with explicit legality/authoring anchors in Tests 1-3 and non-regression safety in Tests 4-7. | Citation: `17-VALIDATION.md` → traceability row `D-10`; `17-05-SUMMARY.md` → remediation evidence + legality/menu-flow closure notes; `src/tests/endpoint_pick_test.c` → point-context and overlay test anchors; `17-UAT.md` → Tests 1-7 summary (`7/7` pass), Test 1, Test 2, Test 3. | Ambiguity policy satisfied: no row marked passed without explicit endpoint legality automation and canonical manual sign-off linkage. |
| D-11 | Coincident authoring supports endpoint-to-endpoint participant semantics through direct selection model. | Passed | Endpoint pair participant model in native pick/context authoring path (owner+role mapping from endpoint entities to Coincident participants) | `endpoint_pick` participant legality and endpoint-to-owner mapping are covered by `test_endpoint_pick_native_endpoint_entity_resolution` and `test_endpoint_point_context_keeps_coincident_for_endpoint_pairs`; targeted rerun passed (`ctest -R endpoint_pick ...`). | Canonical manual baseline passed (`17-UAT.md` tests 1-7), with direct Coincident endpoint-authoring evidence in Test 3 and supporting selection behavior in Tests 1-2. | Citation: `17-VALIDATION.md` → traceability row `D-11`; `17-04-SUMMARY.md` → native endpoint pick-path routing accomplishment; `17-05-SUMMARY.md` → checkpoint remediation/follow-up notes; `src/tests/endpoint_pick_test.c` → endpoint participant legality anchors; `17-UAT.md` → Tests 1-7 summary (`7/7` pass), Test 3. | Reuse + targeted rerun basis is explicit; row upgraded to passed because participant semantics are evidenced in both deterministic tests and canonical UAT authoring flow. |
| D-12 | Pick/render layering maintains endpoint selection priority over continuous primitives. | Passed | Overlay/native endpoint pick precedence path for endpoint point entities over line/arc primitives | `endpoint_pick` layering and collision determinism verified by `test_endpoint_pick_overlay_precedence_contract`, `test_endpoint_pick_collision_prefers_overlay`, and `test_endpoint_pick_collision_deterministic_repeated_sampling`; targeted rerun passed (`ctest -R endpoint_pick ...`). | Canonical manual baseline passed (`17-UAT.md` tests 1-7), with endpoint visibility/selection priority anchor in Test 1 and interaction continuity across Tests 2-7. | Citation: `17-VALIDATION.md` → traceability row `D-12`; `17-04-SUMMARY.md` → one-liner + native endpoint precedence outcome; `17-05-SUMMARY.md` → remediation verification evidence (`endpoint_pick` pass); `src/tests/endpoint_pick_test.c` → overlay/collision test anchors; `17-UAT.md` → Tests 1-7 summary (`7/7` pass), Test 1. | Prior pending state closed: endpoint layering evidence is unambiguous with deterministic automation plus canonical manual baseline linkage. |

## Ambiguity Handling Protocol

For each requirement row, a status can only move to `Passed` when citations are unambiguous. If evidence is ambiguous, the row must be escalated to the smallest targeted rerun/manual recheck and that rerun evidence must be attached before pass.

## Targeted Rerun Evidence (2026-04-07)

1. Discovery check (tests present and scoped):  
   `ctest -N -R "scene_solver_contract|scene_solver_drag|scene_solver_diagnostics" --test-dir build-vulkan -C Release`  
   Result: listed `scene_solver_contract`, `scene_solver_drag`, `scene_solver_diagnostics`.

2. Targeted ambiguity-resolution rerun:  
   `ctest -R "scene_solver_contract|scene_solver_drag|scene_solver_diagnostics" --test-dir build-vulkan -C Release --output-on-failure`  
   Result: **3/3 passed** (`scene_solver_contract`, `scene_solver_drag`, `scene_solver_diagnostics`), 0 failed.

3. Endpoint closure discovery check (Plan 20-02):  
   `ctest -N -R endpoint_pick --test-dir build-vulkan -C Release`  
   Result: listed `endpoint_pick` for targeted endpoint-row closure scope.

4. Endpoint closure targeted rerun (Plan 20-02):  
   `ctest -R endpoint_pick --test-dir build-vulkan -C Release --output-on-failure`  
   Result: **1/1 passed** (`endpoint_pick`), 0 failed.

## Endpoint Row Ambiguity Disposition (Plan 20-02)

- D-09: **Targeted rerun required and completed** (`endpoint_pick`) to remove pending-row ambiguity before pass.
- D-10: **Targeted rerun required and completed** (`endpoint_pick`) because this row anchors endpoint legality/menu behavior.
- D-11: **Reused evidence + targeted rerun sufficient**; no fresh manual recheck required because canonical `17-UAT.md` Test 3 remains unambiguous and aligned with deterministic automation.
- D-12: **Reused evidence + targeted rerun sufficient**; no fresh manual recheck required because canonical `17-UAT.md` Test 1 visibility/selection baseline remains unambiguous.

