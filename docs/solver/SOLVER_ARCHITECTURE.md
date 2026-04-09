# Solver Architecture (mdCAD)

## Overview

This guide is the practical map for debugging mdCAD sketch solver behavior from authoring through user-visible feedback. It is intentionally code-anchored to current runtime ownership:

- `src/app.c` orchestrates interaction and UI feedback.
- `src/ecs/ecs_scene.h` owns solver requests, recalc passes, drag feasibility checks, diagnostics, and status updates.
- `src/constraints/constraint_types.h` defines constraint family naming and legality contracts used by authoring/runtime paths.

Use this document when a constraint operation appears to do nothing, when drag is rejected/rolled back, or when solve convergence is unclear.

## Solve pipeline

1. **Authoring/mutation enters through app interaction paths**
   - Geometry/constraint edits and drag attempts are initiated in `src/app.c`.
   - App code requests solve work via `scene_solver_request_auto(...)` after relevant sketch changes.

2. **Auto queue coalesces solve requests**
   - `scene_solver_request_auto(...)` marks sketch solve as pending.
   - `scene_solver_process_auto_queue(...)` applies debounce and dispatches queued recalc requests.

3. **Transactional recalculate executes**
   - `scene_solver_request_recalculate(...)` gathers sketch participants/constraints, runs iterative pass policy, and applies successful candidate updates.
   - Runtime pass controls include max-pass policy (`scene_solver_max_passes(...)`, `scene_solver_set_max_passes(...)`) and tolerance checks.

4. **Status and implications are committed**
   - On failure paths, solver records implicated constraints and reason using `scene_solver_set_failure_implication(...)`.
   - Diagnostics are emitted through `scene_solver_add_diagnostic(...)`.
   - Sketch status is finalized with `scene_solver_apply_status(...)`.

## Diagnostics flow

The diagnostics flow is designed for deterministic failure triage:

1. **Detect failure family**
   - Unsupported/invalid participant combinations or non-convergent pass loops are surfaced inside `scene_solver_request_recalculate(...)`.
   - Drag infeasibility is evaluated through `scene_solver_can_apply_drag(...)`.

2. **Record machine-readable implication + message**
   - Implicated constraints/participants and failure reason are stored by `scene_solver_set_failure_implication(...)`.
   - Rolling diagnostics ring is updated by `scene_solver_add_diagnostic(...)`.

3. **Surface user-facing feedback**
   - In `src/app.c`, `mdcad_apply_solver_failure_feedback(...)` routes drag-rejection failures into implication + diagnostics and focuses implicated constraints.
   - `mdcad_draw_solver_drag_block_toast(...)` displays immediate on-screen blocked-drag feedback.

4. **Clear on successful solve**
   - `scene_solver_apply_status(..., SKETCH_STATUS_SOLVED)` clears stale failure implication state through `scene_solver_clear_failure_implication(...)` semantics.

## Code anchors

### Stage-by-stage runtime map

| Stage | Responsibility | File | Key function anchors |
|---|---|---|---|
| Authoring trigger | Request auto solve when sketch mutation occurs | `src/app.c` | `scene_solver_request_auto(...)` (e.g., drag/geometry update callsites), app interaction loop |
| Queue processing | Debounce and dispatch queued solve requests | `src/ecs/ecs_scene.h` | `scene_solver_request_auto(...)`, `scene_solver_process_auto_queue(...)` |
| Recalculate core | Run iterative solve passes transactionally and apply results | `src/ecs/ecs_scene.h` | `scene_solver_request_recalculate(...)`, `scene_solver_set_max_passes(...)`, `scene_solver_max_passes(...)` |
| Drag feasibility gate | Decide whether requested drag can be projected/applied | `src/ecs/ecs_scene.h` | `scene_solver_can_apply_drag(...)` |
| Failure implication | Persist implicated constraints/participants + reason | `src/ecs/ecs_scene.h` | `scene_solver_set_failure_implication(...)`, `scene_solver_failure_implication(...)` |
| Diagnostics storage | Add/inspect rolling diagnostic events | `src/ecs/ecs_scene.h` | `scene_solver_add_diagnostic(...)`, `scene_solver_diagnostic_count(...)`, `scene_solver_diagnostic_at(...)` |
| UI feedback surface | Present blocked drag and select implicated constraints | `src/app.c` | `mdcad_apply_solver_failure_feedback(...)`, `mdcad_draw_solver_drag_block_toast(...)` |
| Authoring legality contract | Define legal participant signatures per constraint family | `src/constraints/constraint_types.h` | `constraint_type_is_selection_legal(...)`, `constraint_type_min_participants(...)`, `constraint_type_display_name(...)` |

### Curated references (practical, debugging-focused)

1. **SolveSpace source + docs** — https://solvespace.com/tech.pl  
   Why this helps mdCAD debugging: practical reference for CAD-style geometric constraint solver behavior, including failure modes and user-visible constraint diagnostics.

2. **Ceres Solver Nonlinear Least Squares Tutorial** — https://ceres-solver.readthedocs.io/latest/nnls_tutorial.html  
   Why this helps mdCAD debugging: useful mental model for convergence behavior, residual interpretation, and tuning pass-policy/tolerance tradeoffs.

3. **Eigen Levenberg-Marquardt module docs** — https://eigen.tuxfamily.org/dox/unsupported/group__NonLinearOptimization__Module.html  
   Why this helps mdCAD debugging: concise reference for iterative nonlinear solve steps and stall patterns relevant to pass-policy convergence diagnostics.

4. **Nocedal & Wright, Numerical Optimization (2nd ed.)** — https://link.springer.com/book/10.1007/978-0-387-40065-5  
   Why this helps mdCAD debugging: high-signal background on convergence criteria and step acceptance logic when diagnosing repeated max-pass failures.

5. **Bender et al., A Survey on Position-Based Dynamics** — https://matthias-research.github.io/pages/publications/posBasedDyn.pdf  
   Why this helps mdCAD debugging: practical projection/constraint-iteration framing that maps well to understanding drag projection and stability tradeoffs.

## TL;DR debug primer

- **Unsatisfied constraints — first place to look:**  
  Start with `scene_solver_request_recalculate(...)` failure branches and the latest entries from `scene_solver_diagnostic_at(...)`; then inspect implication payload from `scene_solver_failure_implication(...)` to identify the first implicated constraint and participant set.

- **Drag rollback behavior — first place to look:**  
  Inspect `scene_solver_can_apply_drag(...)` result (`SCENE_SOLVER_DRAG_UNSATISFIABLE`) and app-side handling in `mdcad_apply_solver_failure_feedback(...)`; verify the rejected drag path is emitting diagnostics and selecting implicated constraints.

- **Pass-policy convergence stalls — first place to look:**  
  Check max-pass/tolerance configuration (`scene_solver_max_passes(...)`, `scene_solver_set_max_passes(...)`) and the `max passes reached` failure path in `scene_solver_request_recalculate(...)`; confirm whether the stall is a true geometric conflict vs. tolerance/policy mismatch.
