# Phase 7: Import Pipeline Long-Tail Migration - Context

**Gathered:** 2026-03-27
**Status:** Ready for planning

<domain>
## Phase Boundary

Migrate importer math paths for JSONL and PLY (`loader` + `import_job` surfaces) to cglm-backed helpers while preserving importer reliability and user-visible import trust. This phase covers math migration, parity/correctness validation, and importer-path boundary cleanup only; it does not add new importer capabilities or broaden into undo/editor migration.

</domain>

<decisions>
## Implementation Decisions

### Import transform semantics
- **D-01:** Phase 7 is correctness-first for importer transform semantics; migration may adjust behavior where current transform handling is mathematically incorrect, with explicit evidence notes for any visible deltas.
- **D-02:** Exact semantic fixes are delegated to implementation analysis of current code reality, but must remain within Phase 7 importer scope and be justified in validation artifacts.
- **D-03:** Existing transform-order behavior is not automatically preserved when it conflicts with correctness goals.

### Importer math boundary and helper placement
- **D-04:** Use shared `src/math/` helpers where practical, and keep math importer-local only when behavior is clearly importer-specific.
- **D-05:** Migrate both parser/loader math (`jsonl_loader.h`, `ply_loader.h`) and import-job/runtime transform math (`jsonl_import_job.h`, `ply_import_job.h`, `ply_mesh_import_job.h`) in this phase.
- **D-06:** Avoid introducing a broad compatibility facade; continue thin-entrypoint style established in earlier phases.

### Validation posture and evidence depth
- **D-07:** Validation gate remains light for this phase: compile gate plus targeted importer checks (no broad new harness framework required).
- **D-08:** Targeted checks must explicitly record, for both representative and variant/converted samples: placement, orientation, scale, entity/triangle counts, and parenting structure.
- **D-09:** Evidence must explicitly call out any correctness-driven behavior change versus prior importer output.

### Large-file and chunked-import safety
- **D-10:** Existing chunked import/progress semantics must be preserved across JSONL/PLY paths.
- **D-11:** Regressions in large-file behavior/perceived performance in migrated importer paths are blocking for Phase 7 completion.

### Scope control
- **D-12:** New importer capabilities (new formats/features) are out of scope for Phase 7 and must be deferred.

### the agent's Discretion
- Exact helper names/signatures and placement under `src/math/`.
- Exact split of shared-helper vs importer-local math for importer-specific behavior.
- Exact targeted sample fixtures and evidence format, as long as required outcomes are covered.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Phase scope and requirement contract
- `.planning/ROADMAP.md` — Phase 7 goal, canonical refs, success criteria, and plan skeleton (`07-01`..`07-03`)
- `.planning/REQUIREMENTS.md` — `TAIL-02` requirement intent and milestone traceability
- `.planning/PROJECT.md` — v1.1 constraints (staged migration, platform priority, performance posture)
- `.planning/STATE.md` — current phase focus and routing state

### Prior migration decisions to carry forward
- `.planning/phases/04-interaction-math-and-api-expansion/04-CONTEXT.md` — helper-boundary strategy and legacy retirement boundaries
- `.planning/phases/05-windows-vulkan-hardening-and-performance-gates/05-CONTEXT.md` — evidence and gate discipline
- `.planning/phases/06-serializer-and-save-load-long-tail-migration/06-CONTEXT.md` — light-gate validation pattern and scoped migration cutline

### Runtime importer files in scope
- `src/jsonl_loader.h` — JSONL parser math parsing/normalization and geometric interpretation
- `src/jsonl_import_job.h` — JSONL transform application and entity-creation path
- `src/ply_loader.h` — PLY parser and data conversion math surfaces
- `src/ply_import_job.h` — point-cloud/editable PLY import transform path
- `src/ply_mesh_import_job.h` — mesh/triangle PLY import transform path
- `src/math/cglm_entry.h` — thin cglm entrypoint contract
- `src/math/` — destination for shared importer math helpers
- `docs/QUICKSTART.md` — operator runbook location for phase validation workflow

### Risk and quality context
- `.planning/codebase/CONCERNS.md` — known importer memory/performance fragility and test coverage gaps relevant to Phase 7 risk handling
- `.planning/codebase/TESTING.md` — current lightweight validation culture and practical test patterns

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `jsonl_import_job_apply_transforms(...)` in `src/jsonl_import_job.h` is a clear migration choke point for JSONL coordinate transforms.
- `ply_import_job_apply_transforms(...)` in `src/ply_import_job.h` and `ply_mesh_import_job_apply_transforms(...)` in `src/ply_mesh_import_job.h` are direct PLY transform migration choke points.
- `src/jsonl_loader.h` has concentrated geometry interpretation paths (arc basis/orientation parsing, vector normalization) suitable for correctness-focused math migration.
- Chunked import state machines and progress reporting are already centralized in import job headers and should be preserved.

### Established Patterns
- Header-only `static inline` module style dominates importer/runtime code.
- Import jobs use chunked parsing/entity creation with explicit progress and cancellation; this is a behavior contract, not an implementation detail.
- Prior long-tail phases used scoped migrations + explicit evidence artifacts instead of broad refactors.

### Integration Points
- Shared math helper introduction belongs under `src/math/` and should be consumed by `jsonl_*` and `ply_*` importer paths.
- Importer migration validation feeds phase evidence artifacts and `docs/QUICKSTART.md` runbook updates.
- Import workflows remain integrated through existing scene creation APIs in `src/ecs/ecs_scene.h` (no new import capability surfaces in this phase).

</code_context>

<specifics>
## Specific Ideas

- Prefer mathematically correct importer transforms where existing behavior is weak, with explicit documentation of any user-visible output differences.
- Keep shared-helper adoption practical and scoped; do not force artificial abstraction where importer-specific logic is clearer.
- Preserve chunked import UX and progress semantics while migrating math internals.

</specifics>

<deferred>
## Deferred Ideas

- Add new importer capabilities/formats beyond current JSONL/PLY migration scope.
- Any undo/editor migration work (Phase 8) and broad long-tail validation/perf expansion (Phase 9).

</deferred>

---

*Phase: 07-import-pipeline-long-tail-migration*
*Context gathered: 2026-03-27*
