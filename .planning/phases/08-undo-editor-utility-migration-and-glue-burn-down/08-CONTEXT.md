# Phase 8: Undo/Editor Utility Migration and Glue Burn-Down - Context

**Gathered:** 2026-03-27
**Status:** Ready for planning

<domain>
## Phase Boundary

Migrate undo/redo and editor utility math touchpoints to cglm-backed helpers and remove temporary migration glue that is no longer required by active runtime paths. This phase is about migration/cutover behavior and validation only; it does not add new editor capabilities.

</domain>

<decisions>
## Implementation Decisions

### Transform migration policy
- **D-01:** Phase 8 is correctness-first for undo/editor utility math: prefer mathematically correct behavior even when this changes prior behavior.
- **D-02:** User-visible behavior shifts are allowed when mathematically justified and explicitly documented in validation evidence.
- **D-03:** Correctness-driven changes must remain scoped to undo/editor utility migration surfaces.

### Glue burn-down scope
- **D-04:** Use an aggressive glue-reduction posture in Phase 8: remove most temporary migration glue unless an immediate blocker appears.
- **D-05:** If a glue removal causes regressions that are not safely resolved in-phase, carry-over to Phase 9 is allowed when explicitly documented with rationale and impact.
- **D-06:** Glue burn-down must still respect runtime safety; removals must be traceable to proven non-consumers or replaced by migrated cglm-backed paths.

### Validation depth and evidence
- **D-07:** Validation gate for Phase 8 remains light: compile gate plus focused undo/editor parity workflows.
- **D-08:** Mandatory focused workflows are:
  1. Undo/redo transform edits
  2. Gizmo vertex edit workflow
  3. Inspector edit workflow
- **D-09:** Validation artifacts must call out correctness-driven behavior deltas and any explicitly deferred follow-up.

### Scope control
- **D-10:** No new editor/undo feature capabilities are in scope for Phase 8.

### the agent's Discretion
- Exact helper API names/signatures and placement under `src/math/` for undo/editor migration.
- Exact glue inventory and removal sequencing in plans (`08-01`..`08-03`), as long as locked decisions above are respected.
- Exact shape of focused validation evidence format, provided mandatory workflows and behavior-delta notes are covered.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Phase scope and requirement contract
- `.planning/ROADMAP.md` — Phase 8 goal, dependencies, success criteria, and plan slots (`08-01`..`08-03`)
- `.planning/REQUIREMENTS.md` — `TAIL-03` and `TRED-01` requirement intent and traceability
- `.planning/PROJECT.md` — milestone constraints and migration posture
- `.planning/STATE.md` — active milestone/session routing context

### Prior migration decisions to carry forward
- `.planning/phases/04-interaction-math-and-api-expansion/04-CONTEXT.md` — shared helper boundary and migration posture
- `.planning/phases/06-serializer-and-save-load-long-tail-migration/06-CONTEXT.md` — light-gate validation posture and migration cutline discipline
- `.planning/phases/07-import-pipeline-long-tail-migration/07-CONTEXT.md` — correctness-first migration precedent and evidence discipline

### Runtime files in Phase 8 scope
- `src/undo_redo.h` — undo command model and snapshot data structures using legacy math types
- `src/undo_redo_exec.h` — undo/redo execution and transform/geometry edit mutation paths
- `src/ui/ui_entity_inspector.h` — transform/geometry edit UI and undo command recording
- `src/ui/ui_scene_hierarchy.h` — undo/redo integration touchpoints and editor wiring
- `src/gizmo/gizmo_vertex_mode.h` — vertex edit path touched by mandatory validation workflows
- `src/math/cglm_entry.h` — cglm entrypoint contract
- `src/math/math_interaction.h` — existing cglm-backed interaction helper pattern
- `src/math3d.h` — legacy helper surface being reduced in migrated scope

### Risk and validation context
- `.planning/codebase/CONCERNS.md` — fragile/editor/picking test-gap notes informing focused validation
- `.planning/codebase/TESTING.md` — current validation patterns and practical gate style
- `docs/QUICKSTART.md` — operator runbook location for validation command/workflow updates

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `undo_snapshot_entity(...)` and `undo_create_from_snapshot(...)` in `src/undo_redo_exec.h` centralize entity snapshot/restore semantics and are migration choke points.
- `undo_apply_command(...)` and `undo_unapply_command(...)` centralize transform/geometry mutation for undo/redo and are primary parity-sensitive paths.
- `src/ui/ui_entity_inspector.h` already records granular transform/geometry edits through `undo_cmd_*` hooks, making it the key inspector-side integration point.
- `src/math/math_interaction.h` provides an established pattern for thin cglm-backed helper boundaries with legacy-type bridging.

### Established Patterns
- Header-only `static inline` modules are the dominant style for undo/editor/runtime math-adjacent code.
- Undo/redo behavior is command-pattern based with explicit old/new snapshots rather than implicit state diffs.
- Runtime mutation paths consistently mark transform/render state dirty (`t->dirty`, `instance_dirty`) after edits.
- Recent phases favor scoped migrations + explicit evidence artifacts instead of broad one-pass refactors.

### Integration Points
- New or expanded cglm-backed helper usage should connect first to `src/undo_redo_exec.h` command apply/unapply and snapshot-related math touchpoints.
- Inspector and hierarchy UI paths (`src/ui/ui_entity_inspector.h`, `src/ui/ui_scene_hierarchy.h`) are required call-site boundaries for Phase 8 validation workflows.
- Glue burn-down decisions must be reflected across `src/math/` helper boundaries and direct `math3d` helper use in scoped undo/editor paths.

</code_context>

<specifics>
## Specific Ideas

- Correctness-first migration is explicitly approved even if behavior shifts, as long as the deltas are justified and documented.
- Glue reduction should be aggressive in this phase rather than deferred-by-default.
- Validation should stay lightweight but must include undo/redo transform edits, gizmo vertex edits, and inspector edit workflows.

</specifics>

<deferred>
## Deferred Ideas

- Detailed editor UX contract tuning (beyond migration parity/correctness behavior) was not expanded in this discussion.
- Any unresolved regressions from aggressive glue removal may be explicitly deferred to Phase 9 with evidence, instead of blocking Phase 8 close by default.

</deferred>

---

*Phase: 08-undo-editor-utility-migration-and-glue-burn-down*
*Context gathered: 2026-03-27*
