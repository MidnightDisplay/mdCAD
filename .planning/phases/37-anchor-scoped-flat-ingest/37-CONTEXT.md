# Phase 37: Anchor-Scoped Flat Ingest - Context

**Gathered:** 2026-04-27
**Status:** Ready for planning

<domain>
## Phase Boundary

Phase 37 implements flat ingest structure semantics: each flat JSONL import run must create one root anchor and place imported content under it as plain non-sketch scene entities.

In scope:
- Root-anchor creation and hierarchy shape for flat import runs
- Entry-to-entity placement behavior under the root
- Naming/metadata behavior for root and entry anchors
- Re-import behavior for repeated runs (new anchor structure creation)

Out of scope for this phase:
- Observer refresh execution (Phase 38+)
- Automatic observer safety loop (Phase 39)
- Large-scale responsiveness hardening (Phase 40)
</domain>

<decisions>
## Implementation Decisions

### Carry-forward constraints
- **D-01:** Keep Phase 36 behavior constraints intact: the dedicated `Import JSONL (Flat Large Dump)...` path remains distinct, and existing `Import JSONL Geometry Log...` / `Import JSONL as Sketch` flows are not repurposed.

### Hierarchy shape
- **D-02:** Flat ingest uses a two-level hierarchy: `root anchor -> per-entry anchors -> geometry entities`.
- **D-03:** Do not create per-entry anchors when an entry yields zero importable geometry.

### Naming and metadata
- **D-04:** Root anchor label contract:
  - `Label.name` = JSONL filename stem (no extension)
  - `Label.description` = full source path
- **D-05:** Entry anchor label contract:
  - `Label.name` = entry `Name`
  - `Label.description` = entry `Description`

### Re-import behavior
- **D-06:** Re-importing the same source file keeps previous anchors and creates a fresh root anchor per run.
- **D-07:** Root-name collisions are resolved with numeric suffixes (`name`, `name (2)`, `name (3)`).

### Post-import UX contract
- **D-08:** Successful flat import keeps the current selection unchanged (no auto-select of new root or geometry).

### the agent's Discretion
- Exact collision-resolution helper placement and lookup strategy, as long as D-07 naming semantics remain stable.
- Internal tracking strategy for filtering out zero-geometry entries, as long as D-03 is preserved.
- Exact implementation split between `jsonl_import_job.h` and UI submit wiring, as long as D-01 boundary is preserved.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Milestone and phase contracts
- `.planning/ROADMAP.md` — Phase 37 scope, dependencies, and success criteria.
- `.planning/REQUIREMENTS.md` — `FIMP-03` requirement definition and milestone traceability.
- `.planning/PROJECT.md` — v1.6 scope framing and non-negotiables.
- `.planning/STATE.md` — active phase position and continuity.

### Upstream phase contracts
- `.planning/phases/36-flat-import-entry-configuration/36-CONTEXT.md` — locked entry/configuration and flat-flow UX decisions.
- `.planning/phases/36-flat-import-entry-configuration/36-VERIFICATION.md` — validated Phase 36 behavior that Phase 37 must preserve.
- `.planning/phases/35-observable-jsonl-as-sketch-import-with-optional-live-file-observer/35-CONTEXT.md` — observer metadata and label/path behavior precedents.

### Code anchors
- `src/jsonl_import_job.h` — current root/entry anchor creation, chunked creation, and parenting flow.
- `src/ui/ui_scene_hierarchy.h` — flat-large import entry path and submit behavior from Phase 36.
- `src/ecs/ecs_scene.h` — `scene_add_anchor(...)` and parent-child APIs used by import jobs.
- `src/ecs/ecs_world.h` — low-level parent helpers and anchor entity creation behavior.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `jsonl_import_job.h` already has chunked parse/create/parent phases and currently builds a root + per-entry anchor structure that can be adapted to Phase 37 rules.
- `scene_add_anchor(...)` (`ecs_scene.h`) provides transform-only anchor creation with label support.
- Flat-large import submit wiring in `ui_scene_hierarchy.h` already passes options and starts the async/sync import job path.

### Established Patterns
- Importers are chunked state machines with explicit progress/status strings and cancellation/reset behavior.
- Hierarchy mutations are batched/deferred for consistency and performance (`EcsChildOf` usage in world helpers and import job parenting stage).
- Label contracts are explicit and persisted through ECS components.

### Integration Points
- `jsonl_import_job_tick(...)` parse-complete and parenting phases are the core integration points for D-02/D-03.
- Root/entry label assignment should be enforced at anchor creation boundaries (D-04/D-05).
- Re-import naming collision handling should occur at root-anchor creation time (D-07) without changing Phase 36 submit contract behavior.

</code_context>

<specifics>
## Specific Ideas

- Preserve the current import operator workflow cadence (start import -> wait for completion) while making hierarchy outcomes deterministic.
- Keep hierarchy readable for large dumps by preserving entry grouping, but avoid clutter from empty entries.

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope.

</deferred>

---

*Phase: 37-anchor-scoped-flat-ingest*
*Context gathered: 2026-04-27*
