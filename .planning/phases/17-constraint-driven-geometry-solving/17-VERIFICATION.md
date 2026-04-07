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
| D-01 | Recalculate validates full active sketch constraints before committing geometry updates. | Pending ambiguity audit | `scene_solver_*` transaction path (Phase 17 baseline) | `scene_solver_contract` transactional solve fixture mapping exists. | N/A | Citation: `17-VALIDATION.md` → “Decision-to-Evidence Traceability (D-01..D-12)” row `D-01`; `17-VALIDATION.md` → “Per-Task Verification Map” row `17-01-01`. | Reused evidence appears sufficient; marked for targeted ambiguity review in Plan 20-01 Task 2. |
| D-02 | Successful solve attempts apply resulting geometry updates atomically and immediately. | Pending ambiguity audit | Transactional solve/apply contract path | `scene_solver_contract` commit-on-success assertions mapped in existing validation strategy. | N/A | Citation: `17-VALIDATION.md` → “Decision-to-Evidence Traceability (D-01..D-12)” row `D-02`; `17-VALIDATION.md` → “Per-Task Verification Map” row `17-01-01`. | Reused evidence appears sufficient; marked for targeted ambiguity review in Plan 20-01 Task 2. |
| D-03 | Drag interactions use live constrained solve projection when satisfiable. | Pending ambiguity audit | Live drag projection path (`scene_solver_drag`) | `scene_solver_drag` feasible projection assertions mapped in validation strategy. | N/A | Citation: `17-VALIDATION.md` → “Decision-to-Evidence Traceability (D-01..D-12)” row `D-03`; `17-VALIDATION.md` → “Per-Task Verification Map” row `17-02-01`. | Reused evidence appears sufficient; marked for targeted ambiguity review in Plan 20-01 Task 2. |
| D-04 | Unsatisfiable drag keeps last valid solved state and surfaces immediate feedback. | Pending ambiguity audit | Unsat drag no-mutation contract + implication feedback path | `scene_solver_drag` unsat no-mutation assertions mapped; solver suite rerun evidence recorded. | Manual unsat interaction check recorded as passing in UAT set. | Citation: `17-VALIDATION.md` → traceability row `D-04`; `17-UAT.md` → Test 1-7 results include unsat feedback coverage statement; `17-VALIDATION.md` → “Manual-Only Verifications” row for `D-04,D-05`. | Reused evidence appears sufficient; marked for targeted ambiguity review in Plan 20-01 Task 2. |
| D-05 | Contradictory solves fail deterministically with no scene mutation and stable implication payload. | Pending ambiguity audit | Deterministic implication/no-mutation contract path | `scene_solver_contract` + `scene_solver_diagnostics` implication ordering and no-mutation mappings exist. | Manual failure-clarity check included in prior UAT baseline. | Citation: `17-VALIDATION.md` → traceability row `D-05`; `17-VALIDATION.md` → “Gap-Closure Remediation Evidence” solver command set; `17-UAT.md` → manual baseline pass set. | Reused evidence appears sufficient; marked for targeted ambiguity review in Plan 20-01 Task 2. |
| D-06 | Solver diagnostics dedupe identical consecutive entries while preserving append order. | Pending ambiguity audit | Diagnostics ring buffer + dedupe policy | `scene_solver_diagnostics` dedupe ordering fixture mapped in validation strategy and task map. | Inspector diagnostics behavior covered by prior manual checks. | Citation: `17-VALIDATION.md` → traceability row `D-06`; `17-VALIDATION.md` → per-task row `17-03-02`; `17-VALIDATION.md` → gap-closure command `scene_solver_diagnostics`. | Reused evidence appears sufficient; marked for targeted ambiguity review in Plan 20-01 Task 2. |
| D-07 | Live solve path uses bounded per-frame budget with graceful degradation contract. | Pending ambiguity audit | Bounded projected-delta path in constrained drag solver | `scene_solver_drag` bounded projection expectation (`projected_delta` clamp) mapped in validation strategy. | N/A | Citation: `17-VALIDATION.md` → traceability row `D-07`; `17-VALIDATION.md` → per-task row `17-02-01`. | Reused evidence appears sufficient; marked for targeted ambiguity review in Plan 20-01 Task 2. |
| D-08 | Deterministic solver regression fixtures are wired as automated acceptance gates. | Pending ambiguity audit | CTest target registration + aggregate regression gate | Aggregate command (`scene_solver_contract|scene_solver_drag|endpoint_pick|scene_solver_diagnostics`) recorded as pass in validation evidence. | N/A | Citation: `17-VALIDATION.md` → traceability row `D-08`; `17-VALIDATION.md` → executed evidence command list with aggregate gate; `20-VALIDATION.md` → Per-Task map row `20-01-01`. | Reused evidence appears sufficient; marked for targeted ambiguity review in Plan 20-01 Task 2. |
| D-09 | Constraint participant representation supports endpoint/sub-entity metadata. | Pending (Plan 20-02 closure row) | `EndPointsComp` and sketch-scoped endpoint entity contract | `endpoint_pick` metadata mapping tests and endpoint entity sync regressions exist. | UAT endpoint direct-selection behavior passed. | Citation: `17-04-SUMMARY.md` → “Accomplishments” + key-files (`EndPointsComp` contract); `17-VALIDATION.md` traceability row `D-09`; `17-UAT.md` Test 1 pass. | Deferred to Plan 20-02 for final endpoint-row closure bookkeeping. |
| D-10 | Endpoint controls are first-class selectable/pickable participants for Coincident authoring. | Pending (Plan 20-02 closure row) | Endpoint point selection + legality filtering in context menu flow | `endpoint_pick` legality/layering coverage exists and is listed as pass in validation evidence. | UAT point-context legality and endpoint authoring tests passed. | Citation: `17-VALIDATION.md` traceability row `D-10`; `17-UAT.md` Test 1 and Test 2; `17-05-SUMMARY.md` remediation verification evidence (`ctest -R endpoint_pick ...` pass). | Deferred to Plan 20-02 for endpoint UX/manual-proof consolidation. |
| D-11 | Coincident authoring supports endpoint-to-endpoint participant semantics through direct selection model. | Pending (Plan 20-02 closure row) | Endpoint pair participant model in pick/context authoring path | `endpoint_pick` participant legality checks + solver contract integration mapping exist. | UAT chain/loop Coincident authoring passed. | Citation: `17-VALIDATION.md` traceability row `D-11`; `17-UAT.md` Test 3; `17-05-SUMMARY.md` checkpoint remediation notes. | Deferred to Plan 20-02 for endpoint UX/manual-proof consolidation. |
| D-12 | Pick/render layering maintains endpoint selection priority over continuous primitives. | Pending (Plan 20-02 closure row) | Overlay/native endpoint pick precedence path | `endpoint_pick` layering/collision deterministic tests mapped and validated. | UAT endpoint visibility + direct selection baseline passed. | Citation: `17-VALIDATION.md` traceability row `D-12`; `17-UAT.md` Test 1; `17-05-SUMMARY.md` remediation verification evidence. | Deferred to Plan 20-02 for endpoint UX/manual-proof consolidation. |

## Ambiguity Handling Protocol

For each requirement row, a status can only move to `Passed` when citations are unambiguous. If evidence is ambiguous, the row must be escalated to the smallest targeted rerun/manual recheck and that rerun evidence must be attached before pass.

