# Phase 13: Script Round-Trip Baseline - Context

**Gathered:** 2026-04-01
**Status:** Ready for planning

<domain>
## Phase Boundary

Deliver baseline sketch scripting workflow for v1.2: open a standalone script editor from SketchManager, parse script into supported sketch sub-scene entities/constraints, and keep UI-side sketch edits producing deterministic script output. Lock runtime baseline to Lua 5.4.x.

This phase does **not** include script-side safe apply/rollback policy for partial scene updates (Phase 14), dynamic script IO controls (Phase 14), or cross-platform validation packaging (Phase 15).

</domain>

<decisions>
## Implementation Decisions

### Script contract and scope
- **D-01:** Script format is a **declarative Lua table model** (not imperative command stream) as the Phase 13 canonical contract.
- **D-02:** Phase 13 parser/reconstructor supports **exact current v1.2 sketch scope only**: points, lines, arcs/circles, and current constraint set.
- **D-03:** Lua runtime baseline is locked to **Lua 5.4.x** for Phase 13 deliverables and verification evidence.

### Editor interaction model
- **D-04:** Script editor is a standalone ImGui window opened from SketchManager (not embedded in Entity Inspector body).
- **D-05:** Editing flow uses **both** auto-apply preview and explicit Apply commit.
- **D-06:** Auto-apply preview parse/validation failure must show diagnostics while preserving last committed valid scene state.
- **D-07:** Explicit Apply commit is **atomic all-or-nothing**: commit only when script is fully valid and all references resolve.

### Round-trip identity and reconstruction
- **D-08:** Script entities/constraints use **script-local stable IDs** persisted via components/serialization, not labels and not raw ECS IDs.
- **D-09:** Script reconstruction uses a **two-pass link strategy**: create all entities first, then resolve all references/participants atomically.

### Deterministic UI->script emission
- **D-10:** Deterministic output ordering is stable sort by script-local IDs within type groups.
- **D-11:** Numeric output format is fixed-decimal with trimming and no scientific notation.

### the agent's Discretion
- Exact Lua embedding wiring and allocation boundaries in C, as long as API/runtime behavior is Lua 5.4.x-compliant.
- Exact editor panel layout details (button placement, diagnostics widget composition) while preserving current ImGui conventions.
- Exact internal storage schema for script-local IDs, provided IDs remain stable across save/load and round-trip edits.
- Exact deterministic grouping order precedence among type groups, provided ordering is documented and stable.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Milestone and requirement contract
- `.planning/ROADMAP.md` - Phase 13 goal, dependencies, and success criteria (`SCRP-01`, `SCRP-02`, `SCRP-03`, `SCRP-06`).
- `.planning/REQUIREMENTS.md` - scripting requirement definitions and traceability expectations for Phase 13.
- `.planning/PROJECT.md` - v1.2 scripting milestone intent and platform constraints.
- `.planning/STATE.md` - current continuity and active-phase handoff context.

### Product behavior specification
- `docs/feature-proposal/Sketches, Constraints, Scripting.md` - canonical proposal for script editor entrypoint, declarative sketch representation intent, and runtime portability constraints.

### Upstream phase contracts to preserve
- `.planning/phases/10-sketch-foundations-managers/10-CONTEXT.md` - sketch ownership and manager workflow foundations.
- `.planning/phases/11-constraint-authoring-ux/11-CONTEXT.md` - constraint entity model, participant linkage, and manager/glyph contracts.
- `.planning/phases/12-solver-control-and-constrained-interaction/12-CONTEXT.md` - solver status/diagnostics and interaction semantics that script round-trip must not regress.

### Implementation anchors in code
- `src/ui/ui_entity_inspector.h` - SketchManager surface and active sketch workspace patterns to host script-editor launch controls.
- `src/ecs/ecs_scene.h` - sketch/geometry/constraint scene APIs and participant-link semantics for reconstruction wiring.
- `src/scene_serializer.h` - existing persisted sketch/constraint structure, participant remap flow, and numeric formatting precedents.
- `src/components/sketch_comp.h` - sketch-level runtime metadata extension point.
- `src/components/constraint_comp.h` - constraint value/participant contracts to mirror in script serialization.
- `src/app.c` - frame/window orchestration patterns for standalone modal-independent editor windows.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `src/ui/ui_entity_inspector.h`: existing SketchManager and standalone "Active Sketch Workspace" pattern can host script-editor launch + status controls.
- `src/scene_serializer.h`: already serializes sketch + constraint graphs, including participant remap after load, which is reusable for script reconstruction architecture.
- `src/ecs/ecs_scene.h`: central scene mutation helpers and constraint participant link maintenance provide stable integration points for script->scene application.

### Established Patterns
- Header-only `static inline` module style with ECS-first state ownership and scene-level mutation APIs.
- Atomic behavior preference already used in undo/scene operations; aligns with all-or-nothing Apply commit requirement.
- Existing dimensional/constraint formatting already constrains numeric display expectations, supporting fixed-decimal deterministic script output.

### Integration Points
- Add script editor state + launch action in SketchManager UI path.
- Add script parse/apply facade at scene layer so UI remains thin and reconstruction logic is centralized.
- Extend serialization-adjacent codepaths (or dedicated scripting module) for deterministic emit and two-pass reconstruction.
- Introduce stable script-local IDs into sketch geometry/constraint component graph and persist through scene save/load.

</code_context>

<specifics>
## Specific Ideas

- Script should read like a declarative sketch model (entities + constraints + links), not a procedural command log.
- Editor should support rapid iteration (preview) without risking scene corruption; explicit Apply remains authoritative commit point.
- Deterministic output must be stable enough for diff/review workflows and future script round-trip debugging.

</specifics>

<deferred>
## Deferred Ideas

- Script-side safe recovery/last-valid-state guarantees during full scene apply lifecycle are Phase 14 scope.
- Dynamic script IO controls (`min/max/step`, generated sliders/readouts) are Phase 14 scope.
- Cross-platform scripting parity evidence for macOS/web/iOS validation is Phase 15+ scope.

</deferred>

---

*Phase: 13-script-round-trip-baseline*
*Context gathered: 2026-04-01*
