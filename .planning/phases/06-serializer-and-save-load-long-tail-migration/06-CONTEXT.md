# Phase 6: Serializer and Save/Load Long-Tail Migration - Context

**Gathered:** 2026-03-26
**Status:** Ready for planning

<domain>
## Phase Boundary

Migrate serializer save/load math paths to cglm-backed helpers inside `src/scene_serializer.h`, with serializer-adjacent invocation/status plumbing in `src/ui/ui_scene_hierarchy.h` included only where it supports save/load clarity. Keep this phase scoped to serializer/save-load behavior and do not widen into importer or undo/editor migration work.

</domain>

<decisions>
## Implementation Decisions

### Serializer scope cutline
- **D-01:** Phase 6 scope includes `src/scene_serializer.h` migration plus serializer-related callsite/plumbing work in `src/ui/ui_scene_hierarchy.h`.
- **D-02:** `ui_scene_hierarchy.h` changes may include minor save/load UX clarity polish, but not broad UX redesign.
- **D-03:** Importer (`jsonl`/`ply`) and undo/editor migration concerns remain out of scope for this phase.

### Scene format compatibility policy
- **D-04:** Phase 6 is allowed to perform scene format cleanup as part of serializer migration.
- **D-05:** Backward compatibility is not required in-phase; old format breakage is acceptable if explicit migration notes are provided.
- **D-06:** Provide a converter workflow to move old-format scenes to the cleaned format introduced by this phase.

### Validation and gate depth
- **D-07:** Phase 6 gate posture is light: compile plus targeted serializer save/load checks (no expanded strict harness gate required in this phase).
- **D-08:** Required targeted checks are: save+reload representative scene and verify entity count, parent links, transform fields, and geometry types.

### Legacy-helper retirement aggressiveness
- **D-09:** Within serializer scope, aggressively retire legacy `math3d` helper usage while migrating to cglm-backed paths.
- **D-10:** Aggressive retirement applies to serializer internals in this phase boundary; unrelated subsystem migration remains deferred.

### the agent's Discretion
- Exact converter format/tooling shape and invocation UX.
- Exact targeted-check implementation mechanism (automation vs scripted/manual checklist), as long as required verification fields are covered.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Phase scope and milestone constraints
- `.planning/ROADMAP.md` — Phase 6 goal, scope boundary, success criteria, and plan skeleton (`06-01`..`06-03`)
- `.planning/REQUIREMENTS.md` — `TAIL-01` and related v1.1 requirement intent
- `.planning/PROJECT.md` — active milestone framing, migration constraints, and locked platform priorities
- `.planning/STATE.md` — current session position and next-step routing

### Prior migration decisions to carry forward
- `.planning/phases/03-macos-core-transform-migration/03-CONTEXT.md` — cglm-first migration posture and parity-first boundary discipline
- `.planning/phases/04-interaction-math-and-api-expansion/04-CONTEXT.md` — shared-helper migration pattern and legacy-helper retirement boundaries
- `.planning/phases/05-windows-vulkan-hardening-and-performance-gates/05-CONTEXT.md` — evidence posture and deferred backlog framing used to open v1.1

### Runtime files in this phase
- `src/scene_serializer.h` — primary serializer save/load implementation and migration target
- `src/ui/ui_scene_hierarchy.h` — serializer invocation/status plumbing and small UX clarity adjustments
- `src/math/cglm_entry.h` — thin entrypoint contract for cglm-backed migrated paths
- `src/math/` — target location for any new serializer-adjacent thin helper boundaries
- `docs/QUICKSTART.md` — operator runbook location for save/load validation steps and converter usage notes

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `scene_save_to_string/file` and `scene_load_from_string/file` in `src/scene_serializer.h` already provide clear migration choke points.
- JSON parse/write helpers in `src/scene_serializer.h` centralize vec/array parsing and are suitable for serializer-focused migration/refactor.
- Save/load invocation/status reporting in `src/ui/ui_scene_hierarchy.h` is already centralized around file-browser callbacks.
- `src/math/cglm_entry.h` already enforces shared cglm contract and should anchor any serializer migration helpers.

### Established Patterns
- Header-only `static inline` style remains standard in serializer and UI modules.
- File-browser driven save/load flow with short status strings (`Saved X entities` / `Failed to load`) is the current UX baseline.
- Staged migration boundaries are enforced milestone-by-milestone; importer/undo work has dedicated later phases.

### Integration Points
- Serializer migration work connects directly to `scene_save_to_*` / `scene_load_from_*` paths in `src/scene_serializer.h`.
- Any save/load UX clarity polish lands in `src/ui/ui_scene_hierarchy.h` file-browser result handling.
- Converter workflow/documentation will connect through `docs/QUICKSTART.md` and phase evidence artifacts.

</code_context>

<specifics>
## Specific Ideas

- Include serializer-adjacent UI callsite work only when it improves save/load clarity during migration.
- Accept scene format break in this phase, but ship an explicit converter workflow instead of silent incompatibility.
- Keep validation lightweight but concrete: representative save/reload verification across entity count, parent links, transforms, and geometry types.

</specifics>

<deferred>
## Deferred Ideas

- Import pipeline migration details (`jsonl`/`ply`) — deferred to Phase 7.
- Undo/editor utility migration details and broader glue burn-down — deferred to Phase 8.
- Expanded strict harness/performance closure for long-tail surfaces — deferred to Phase 9.

</deferred>

---

*Phase: 06-serializer-and-save-load-long-tail-migration*
*Context gathered: 2026-03-26*
