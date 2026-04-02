# Phase 17: Constraint-driven geometry solving - Context

**Gathered:** 2026-04-02
**Status:** Ready for planning

<domain>
## Phase Boundary

Deliver true constraint-driven solve behavior for sketch geometry so constraints are not cosmetic: the solver must compute and apply geometry updates, keep interaction stable under constraints, and provide deterministic diagnostics and regression gates.

This phase upgrades solver behavior within the existing sketch/constraint architecture. It does **not** introduce multi-sketch/global solving, multi-backend solver UX, or broader scripting/runtime expansion.

</domain>

<decisions>
## Implementation Decisions

### Solve capability scope
- **D-01:** Phase 17 enforces the **full v1.2 constraint set** during solve-driven geometry updates.
- **D-02:** On successful solve, affected sketch geometry is updated **immediately and fully** (not deferred/preview-only).

### Interaction semantics during drag
- **D-03:** Default interaction uses **live constrained solve during drag** with smooth projection where feasible.
- **D-04:** If drag becomes unsatisfiable, preserve the **last valid solved state** (no partial invalid pose) and surface immediate feedback.

### Conflict handling and diagnostics
- **D-05:** Contradictory/over-constrained states use **deterministic failure** with explicit implicated constraints + reason and **no scene mutation**.
- **D-06:** Diagnostics append on each attempt with **dedupe of identical consecutive messages** and retain explicit user-clear action.

### Precision and performance guardrails
- **D-07:** Optimize for interaction responsiveness using a **bounded per-frame solve budget** with graceful degrade path, while preserving correctness contract.
- **D-08:** Add deterministic regression checks with fixtures and tolerance-based assertions as acceptance gates for solver outputs.

### the agent's Discretion
- Exact per-frame solve budget thresholds and degrade trigger heuristics.
- Dedupe window/rules beyond "identical consecutive message" minimum contract.
- Internal numeric tolerance values per constraint type, as long as deterministic gate expectations are preserved.
- Specific fixture corpus composition and organization for regression tests.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Milestone and phase contract
- `.planning/ROADMAP.md` — active phase list and newly added Phase 17 anchor.
- `.planning/REQUIREMENTS.md` — solver/constraint/scripting requirement continuity and traceability.
- `.planning/PROJECT.md` — milestone intent, constraints, and platform priorities.
- `.planning/STATE.md` — current continuity and recent decision trail.

### Product intent and prior phase decisions
- `docs/feature-proposal/Sketches, Constraints, Scripting.md` — canonical feature intent for solver-backed constraints.
- `.planning/phases/11-constraint-authoring-ux/11-CONTEXT.md` — constraint authoring/glyph/participant semantics to preserve.
- `.planning/phases/12-solver-control-and-constrained-interaction/12-CONTEXT.md` — existing solver-control and drag guardrail decisions that Phase 17 deepens.
- `.planning/phases/16-constraint-ux-closure-and-verification/16-VERIFICATION.md` — verified participant highlighting behavior that must not regress.

### Implementation anchors in code
- `src/ecs/ecs_scene.h` — current solver request/status/drag decision hooks and sketch metadata pipeline.
- `src/app.c` — drag loop and solver feedback integration path.
- `src/ui/ui_entity_inspector.h` — solver panel controls and diagnostics rendering.
- `src/components/sketch_comp.h` — sketch solver state/diagnostic storage model.
- `src/constraints/constraint_selection.h` — participant highlighting utility used by failure implication UX.

### Codebase constraints and architecture guidance
- `.planning/codebase/ARCHITECTURE.md` — subsystem boundaries and integration layers.
- `.planning/codebase/CONVENTIONS.md` — coding and module style constraints.
- `.planning/codebase/STRUCTURE.md` — ownership of files/subsystems touched by solver work.
- `.planning/codebase/STACK.md` — runtime/toolchain constraints relevant to deterministic gates.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `scene_solver_request_auto(...)`, `scene_solver_request_recalculate(...)`, `scene_solver_can_apply_drag(...)` in `src/ecs/ecs_scene.h` provide existing solver touchpoints to evolve from metadata/guardrail behavior to true solve application.
- `scene_solver_set_failure_implication(...)` + `constraint_selection_apply_participants(...)` provide an established failure/highlight path to keep UX consistent.
- `ui_entity_inspector` solver section already exposes backend, auto-solve toggle, recalc button, and diagnostics panel surface.

### Established Patterns
- Header-only `static inline` subsystem design is the dominant extension model for scene and UI modules.
- Solver and interaction contracts are centralized through `scene_solver_*` APIs; app/UI layers are thin callers.
- Drag behavior in `app.c` uses decision contracts (`scene_solver_drag_decision_t`) and blocks unsatisfiable movement by design.

### Integration Points
- Extend solve execution path in `src/ecs/ecs_scene.h` to compute and apply constrained geometry states, not only status/serial updates.
- Keep `app.c` drag loop wired to solver decisions while ensuring no invalid intermediate pose is committed.
- Expand diagnostics handling with deterministic dedupe behavior while preserving explicit clear controls in inspector UX.
- Add deterministic fixtures/assertions under existing test infrastructure to gate solver output stability.

</code_context>

<specifics>
## Specific Ideas

- Solver should feel authoritative: when constraints exist, geometry should visibly follow them in real time, not only log statuses.
- Interaction must stay responsive even on heavier sketches, but never violate correctness or commit invalid transient states.
- Deterministic fixture-based testing is required to avoid regressions from tolerance and projection tuning.

</specifics>

<deferred>
## Deferred Ideas

- Multi-sketch/global dependency solving (v1.3+ scope).
- Multiple selectable solver backends and backend-switch UX.
- Non-sketch general-purpose solver runtime expansion beyond current sketch domain.

</deferred>

---

*Phase: 17-constraint-driven-geometry-solving*
*Context gathered: 2026-04-02*
