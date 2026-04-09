# Phase 29: Active-Sketch Line Gizmo Endpoint Authority - Context

**Gathered:** 2026-04-09
**Status:** Ready for planning

<domain>
## Phase Boundary

Correct active-sketch line gizmo behavior so the gizmo is midpoint-anchored and geometry-authoritative: dragging moves line endpoints `A` and `B` together as rigid geometry, while non-active or non-line selections retain existing behavior.

This phase is limited to `GZM-01..04` interaction semantics and undo/redo coherence for active-sketch line drags. It does not include broader solver architecture docs or milestone closure gates (Phase 30).

</domain>

<decisions>
## Implementation Decisions

### Scope boundary and eligibility
- **D-01:** Endpoint-authority behavior applies to active-sketch `GEOM_LINE` entities and is not limited to single-line selection.
- **D-02:** Multi-selection of active-sketch lines is supported; selected lines translate as rigid geometry by moving their endpoints `A`/`B` together.
- **D-03:** For mixed selections, apply endpoint-authority only to eligible active-sketch lines while non-line/non-active entities continue existing semantics in the same drag interaction.
- **D-04:** Non-active sketch lines and non-line geometry must preserve current behavior (`GZM-03` guardrail).

### Mode behavior
- **D-05:** Endpoint-authority can activate in `GIZMO_TRANSFORM_MODE`.
- **D-06:** Endpoint-authority also activates in `GIZMO_GEOMETRY_MODE` when vertex mode is inactive.
- **D-07:** Vertex-mode active editing remains vertex-authoritative and is not replaced by line endpoint-authority.

### Midpoint anchoring behavior
- **D-08:** For a single active-sketch line, gizmo center is anchored to the line midpoint.
- **D-09:** For multi-line active-sketch selection, use one gizmo anchored at the average of selected line midpoints.

### Undo/redo interaction contract
- **D-10:** One completed mouse drag is recorded as one grouped undo/redo interaction across all affected active-sketch line endpoints.
- **D-11:** Undo/redo must restore exact endpoint geometry for the full drag interaction (`GZM-04`), not fragmented per-endpoint history entries.

### the agent's Discretion
- Exact internal data-path mechanics for hybrid mixed-selection application (eligible line subset + existing fallback path), provided D-01..D-11 remain true.
- Exact helper boundaries and naming for midpoint-center computation and grouped undo packaging.
- Exact regression test split across existing test binaries.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Phase and milestone contracts
- `.planning/ROADMAP.md` — Phase 29 goal, dependency, requirements mapping, and success criteria.
- `.planning/REQUIREMENTS.md` — `GZM-01..04` requirement contracts.
- `.planning/PROJECT.md` — v1.4 robustness-first scope and active phase priorities.
- `.planning/STATE.md` — current continuity and sequencing state.

### Upstream behavioral constraints to preserve
- `.planning/phases/28-tangency-drag-robustness/28-CONTEXT.md` — drag-authority and transactional interaction posture preserved for solver-integrated editing.
- `.planning/phases/27-principal-axis-line-along-reliability/27-CONTEXT.md` — deterministic mixed-constraint expectations.
- `.planning/phases/26-line-line-constraint-coverage/26-CONTEXT.md` — explicit diagnostics and deterministic selection-order posture.
- `.planning/phases/12-solver-control-and-constrained-interaction/12-CONTEXT.md` — solver-control interaction boundaries and constrained drag behavior expectations.

### Code and test anchors
- `src/app.c` — gizmo drag lifecycle, direct-point mode gating, and undo capture path.
- `src/gizmo/gizmo.h` — gizmo center computation and drag-space update semantics.
- `src/ecs/ecs_scene.h` — endpoint/owner geometry synchronization and scene-level drag application paths.
- `src/undo_redo.h` — undo command model used to group drag history.
- `src/undo_redo_exec.h` — runtime replay/apply behavior for geometry endpoint restoration.
- `src/tests/endpoint_pick_test.c` — endpoint/owner geometry behavior contracts and undo-related geometry expectations.
- `src/tests/scene_solver_drag_test.c` — drag and solver integration contracts relevant to interactive edits.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `mdcad_selection_is_active_sketch_standalone_points(...)` in `src/app.c` shows an existing eligibility-gating pattern for special-case active-sketch behavior.
- `gizmo_entity_world_center(...)` and `gizmo_update(...)` in `src/gizmo/gizmo.h` are the current gizmo-center choke points.
- Drag lifecycle in `src/app.c` already has mode-specific branches and an end-of-drag undo commit boundary.

### Established Patterns
- App/UI remain orchestration layers while scene/solver owns authoritative geometry behavior.
- Drag interactions are transactional at interaction boundaries (start/update/end) with explicit undo recording on release.
- Existing behavior differentiates transform-mode and geometry vertex-mode; mode contracts should stay explicit.

### Integration Points
- Add active-sketch line eligibility and midpoint-center path into gizmo center/update flow.
- Route eligible active-sketch line drags through endpoint `A/B` geometry updates instead of transform-only movement.
- Preserve current non-eligible behavior path for mixed selections while introducing grouped undo for endpoint-authority drags.

</code_context>

<specifics>
## Specific Ideas

- User expects selected active-sketch lines to move via geometry endpoints, not only via transform position updates.
- User expects gizmo visual anchor to represent line midpoint semantics.
- User explicitly chose multi-line support with midpoint averaging and hybrid mixed-selection behavior.

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope.

</deferred>

---

*Phase: 29-active-sketch-line-gizmo-endpoint-authority*
*Context gathered: 2026-04-09*
