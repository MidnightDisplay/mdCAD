---
phase: 12-solver-control-and-constrained-interaction
verified: 2026-04-01T16:47:35Z
status: gaps_found
score: 6/7 must-haves verified
gaps:
  - truth: "Failure implication highlighting persists until next successful solve."
    status: failed
    reason: "Failure implication clear-on-success logic exists but is not wired into the active solve path."
    artifacts:
      - path: "src/ecs/ecs_scene.h"
        issue: "scene_solver_request_recalculate writes sk->status directly and never calls scene_solver_apply_status, so solved status does not clear failure implication."
    missing:
      - "Route status updates through scene_solver_apply_status (or invoke equivalent clear-on-solved behavior) in the recalculate/solve completion path."
      - "Add a solve-success path assertion/check that failure implication state is cleared when status becomes SKETCH_STATUS_SOLVED."
---

# Phase 12: Solver Control & Constrained Interaction Verification Report

**Phase Goal:** Users can control sketch solving, understand failures, and interact with geometry under active constraint rules.  
**Verified:** 2026-04-01T16:47:35Z  
**Status:** gaps_found  
**Re-verification:** No — initial verification

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
| --- | --- | --- | --- |
| 1 | User can toggle auto-solve per sketch and manually trigger Recalculate from SketchManager. | ✓ VERIFIED | `ui_entity_inspector.h` checkbox + button call `scene_solver_set_auto_solve`, `scene_solver_request_auto`, `scene_solver_request_recalculate` (lines ~983-1001). Default auto-solve ON in `sketch_comp_default()` (`sketch_comp.h` lines 57-66). |
| 2 | User can see current sketch solver status (`solved`, `loose`, `fixed`, `error`) and timestamped INFO/WARNING/ERROR diagnostics. | ✓ VERIFIED | Status names in `sketch_comp.h` (`sketch_status_name`), severity names INFO/WARNING/ERROR (`sketch_solver_diagnostic_severity_name`), inspector renders status + last solve timestamp + diagnostics list (`ui_entity_inspector.h` lines ~975-1020), scene diagnostic ring APIs in `ecs_scene.h` lines ~1615-1671. |
| 3 | User can see the single active solver backend type in solver controls with no backend-switch UX. | ✓ VERIFIED | Fixed backend id/name in scene init (`ecs_scene.h` lines 259-260), read-only display in inspector (`ui_entity_inspector.h` lines 973-974), no backend selector controls found in inspected UI path. |
| 4 | When solve fails or is invalid, implicated constraints and participant geometries are highlighted and first implicated constraint is focused. | ✓ VERIFIED | Failure payload stores constraints/participants and first constraint (`ecs_scene.h` lines ~1694-1755); app failure path sets selected constraint + applies participant highlight helper (`app.c` lines ~543-553). |
| 5 | Failure implication highlighting persists until next successful solve. | ✗ FAILED | Clear-on-success exists only in `scene_solver_apply_status` (`ecs_scene.h` lines ~1674-1681), but active recalc path sets `sk->status` directly (`ecs_scene.h` line ~1611), bypassing implication clear behavior. |
| 6 | During gizmo drag, feasible constrained movement is projected in real time; unsatisfiable movement is blocked (not temporarily applied). | ✓ VERIFIED | Drag loop calls `scene_solver_can_apply_drag` before mutation (`app.c` lines ~1216-1234); feasible path uses `drag_decision.projected_delta` (line ~1229); unsatisfiable path forces zero delta (line ~1231). |
| 7 | Unsatisfiable drag immediately shows feedback via viewport toast, diagnostics entry, and implication highlight. | ✓ VERIFIED | Toast text `Movement blocked by active constraints.` (`app.c` line 518), diagnostic message `Drag rejected...` with `scene_solver_add_diagnostic` (`app.c` lines ~535-540), implication/focus/highlight wiring (`app.c` lines ~529-553). |

**Score:** 6/7 truths verified

### Required Artifacts

| Artifact | Expected | Status | Details |
| --- | --- | --- | --- |
| `src/components/sketch_comp.h` | Per-sketch solver policy/runtime metadata defaults incl. auto-solve ON | ✓ VERIFIED | Exists; substantive solver fields (`auto_solve_enabled`, serials, timestamp, backend id, diagnostics ring max 100); used by scene/UI. |
| `src/ecs/ecs_scene.h` | Scene-level solver request/recalc/diagnostics/failure implication/drag feasibility APIs | ⚠️ HOLLOW — wired but data lifecycle incomplete | Exists, substantive, heavily used by app/UI; however clear-on-success implication lifecycle not wired through active recalc status update path. |
| `src/ui/ui_entity_inspector.h` | SketchManager solver controls and diagnostics panel UI | ✓ VERIFIED | Exists; renders backend/status/timestamp, auto-solve toggle, recalc action, diagnostics history, explicit clear-confirm UX. |
| `src/app.c` | Drag-loop integration for constrained projection/blocking and failure feedback | ✓ VERIFIED | Calls scene drag feasibility API, blocks unsatisfiable movement, emits toast + diagnostic + participant highlight focus. |
| `src/gizmo/gizmo.h` | Constrained drag integration contract for app drag loop boundary | ✓ VERIFIED | `gizmo_update_drag` contract explicitly delegates constrained policy to caller (`gizmo.h` lines ~434-435); function used by app drag loop. |

### Key Link Verification

| From | To | Via | Status | Details |
| --- | --- | --- | --- | --- |
| `src/ui/ui_entity_inspector.h` | `src/ecs/ecs_scene.h` | solver control actions call scene_solver APIs | ✓ WIRED | `scene_solver_set_auto_solve`, `scene_solver_request_auto`, `scene_solver_request_recalculate`, diagnostics APIs are called from solver panel. |
| `src/ecs/ecs_scene.h` | `src/components/sketch_comp.h` | per-sketch runtime/status/diagnostics state updates | ✓ WIRED | Scene APIs mutate/read `SketchComp` fields (`auto_solve_*`, status, timestamps, diagnostics ring). |
| `src/app.c` | `src/ecs/ecs_scene.h` | drag update path calls solver feasibility/projection and failure handlers | ✓ WIRED | `scene_solver_can_apply_drag`, `scene_solver_set_failure_implication`, `scene_solver_drag_make_rejected_diagnostic`, `scene_solver_add_diagnostic`. |
| `src/ecs/ecs_scene.h` | `src/constraints/constraint_selection.h` | failure implication uses shared participant highlighting helper path | ⚠️ PARTIAL | Helper is not called in `ecs_scene.h`; participant highlighting is applied in `app.c` (`constraint_selection_apply_participants`). Behavior works, but plan-declared direct link is not present. |

### Data-Flow Trace (Level 4)

| Artifact | Data Variable | Source | Produces Real Data | Status |
| --- | --- | --- | --- | --- |
| `src/ui/ui_entity_inspector.h` | `diag_count` / `diag` entries | `scene_solver_diagnostic_count` + `scene_solver_diagnostic_at` → `SketchComp.diagnostics[]` in scene | Yes (populated by `scene_solver_add_diagnostic` on manual recalc + blocked drag) | ✓ FLOWING |
| `src/ui/ui_entity_inspector.h` | `sketch->status`, `last_solve_timestamp_ms` | `scene_solver_request_recalculate` / metadata refresh in scene | Yes (status/timestamp updated in scene) | ✓ FLOWING |
| `src/app.c` | `drag_decision` | `scene_solver_can_apply_drag` computes feasible/projected/unsatisfiable decision from sketch geometry/fixed state | Yes | ✓ FLOWING |
| `src/ecs/ecs_scene.h` | `solver_failure_implication` lifecycle | set in failure path; clear only in `scene_solver_apply_status` | Not fully (clear on success not reached in active recalc path) | ⚠️ STATIC PARTIAL |

### Behavioral Spot-Checks

| Behavior | Command | Result | Status |
| --- | --- | --- | --- |
| Solver/drag runtime behaviors in this phase | N/A | Step 7b: SKIPPED (native interactive UI/gizmo flow; no safe non-interactive runtime entrypoint without launching app and driving viewport interactions) | ? SKIP |

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
| --- | --- | --- | --- | --- |
| SOLV-01 | 12-01-PLAN.md | Auto-solve toggle + manual recalc from solver controls | ✓ SATISFIED | Inspector toggle/button wired to scene solver APIs; `auto_solve_enabled` default true. |
| SOLV-02 | 12-01-PLAN.md | Show solve states + timestamped INFO/WARNING/ERROR diagnostics | ✓ SATISFIED | Status/severity names, diagnostics ring storage/retrieval, and inspector rendering present. |
| SOLV-03 | 12-01-PLAN.md | Show one active backend type, no switch UX | ✓ SATISFIED | Fixed backend id/name in scene; read-only UI display only. |
| SOLV-04 | 12-02-PLAN.md | Identify implicated constraints/geometries on failure | ✓ SATISFIED | Failure implication payload + focus constraint + participant highlight apply path in app. |
| API-03 | 12-02-PLAN.md | Gizmo transforms respect active constraints during interaction | ✓ SATISFIED | Drag feasibility gate before mutation, projected delta for feasible, zero delta for unsatisfiable. |

All requirement IDs declared in phase plans were found and accounted for in `REQUIREMENTS.md`.  
Orphaned Phase 12 requirements from `REQUIREMENTS.md` not claimed by plan frontmatter: **None**.

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
| --- | --- | --- | --- | --- |
| `src/ecs/ecs_scene.h` | 1944 | “Placeholder derivation…” comment in status derivation block | ⚠️ Warning | Signals heuristic status derivation path; not a hard blocker for this phase goal, but indicates non-final solver semantics. |
| `src/ecs/ecs_scene.h` | 1611 + 1679-1681 | Direct status assignment bypasses clear-on-success hook | 🛑 Blocker | Causes failure implication persistence lifecycle to violate “until next successful solve” contract. |

### Human Verification Required

### 1. Solver panel interaction UX
**Test:** In SketchManager, toggle auto-solve OFF/ON, perform edits, then click **Recalculate Sketch**.  
**Expected:** Auto-trigger behavior follows toggle state; manual recalc always triggers explicit solve feedback.  
**Why human:** Requires live UI interaction timing/state checks.

### 2. Failure implication visual persistence and clear
**Test:** Trigger unsatisfiable drag to show implication highlight, then perform a successful solve path.  
**Expected:** Highlight remains through failure states and clears immediately on successful solve.  
**Why human:** Visual highlight lifecycle and interaction sequencing are runtime viewport behaviors.

### 3. Constrained drag feel
**Test:** Drag constrained geometry in feasible vs infeasible directions.  
**Expected:** Feasible movement projects smoothly; infeasible movement is blocked with immediate toast/diagnostic/focus feedback.  
**Why human:** Real-time interaction feel and visual response cannot be fully validated via static inspection.

### Gaps Summary

Phase 12 is mostly implemented and wired (6/7 truths verified), but one goal-critical lifecycle gap remains: failure implication state is not reliably cleared on successful solve because the active recalc path bypasses the clear-on-success function. This prevents fully meeting the “persists until next successful solve” requirement boundary.

---

_Verified: 2026-04-01T16:47:35Z_  
_Verifier: the agent (gsd-verifier)_
