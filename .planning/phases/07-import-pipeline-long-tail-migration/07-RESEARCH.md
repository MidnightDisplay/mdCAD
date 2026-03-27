# Phase 7: Import Pipeline Long-Tail Migration - Research

**Researched:** 2026-03-27  
**Domain:** JSONL/PLY importer math-path migration to cglm-backed helpers (TAIL-02)  
**Confidence:** HIGH

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
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

### Deferred Ideas (OUT OF SCOPE)
- Add new importer capabilities/formats beyond current JSONL/PLY migration scope.
- Any undo/editor migration work (Phase 8) and broad long-tail validation/perf expansion (Phase 9).
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| TAIL-02 | User can import JSONL/PLY geometry through cglm-backed importer math paths without reintroducing migrated `math3d` helper dependencies. | Defines importer cutlines, cglm-backed helper strategy, math touchpoint migration order, targeted parity evidence matrix, and plan-by-plan execution guidance for 07-01..07-03. |
</phase_requirements>

## Summary

Phase 7 should be planned as a **targeted importer math migration**, not an importer redesign. The in-scope files are clearly separated between loader math (`jsonl_loader.h`, `ply_loader.h`) and runtime transform application (`jsonl_import_job.h`, `ply_import_job.h`, `ply_mesh_import_job.h`). These files currently depend heavily on legacy `math3d.h` vector/matrix helpers for normalization, basis construction, CoM shift, rotation matrix composition, and scaling.

The safest strategy is to introduce a **thin shared importer-math helper boundary** under `src/math/` (e.g., importer-focused vec3/mat4 helpers backed by `cglm_entry.h`), then migrate call sites in place while preserving state-machine behavior (chunking, progress, cancellation, parenting flow). This matches D-04/D-06 and avoids broad compatibility wrappers.

Validation must stay light (D-07) but evidence-rich (D-08/D-09): compile gate plus targeted JSONL/PLY checks that explicitly capture placement, orientation, scale, entity/triangle counts, and parenting structure on both representative and variant/converted samples. Any correctness-driven behavior change must be called out as an intentional delta.

**Primary recommendation:** Implement Phase 7 as a three-step sequence: (1) migrate importer math touchpoints and boundaries, (2) add targeted parity check artifacts, (3) execute native compile + importer workflow evidence and document deltas.

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| cglm (vendored) | 0.9.6 (`vendors/cglm/VERSION.txt`) | Canonical math backend | Locked backend in `.planning/STATE.md` and enforced by `src/math/cglm_entry.h` |
| Importer modules (header-only) | in-repo | JSONL/PLY parse + import-job state machines | Existing runtime contract and migration target surface |
| Flecs (vendored) | 4.1.4 (`vendors/flecs/flecs.h`) | Entity creation, parenting, import pending tags | Import jobs rely on ECS parenting/defer semantics |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| cJSON (vendored) | 1.7.19 (`vendors/cjson/cJSON.h`) | JSONL line parsing | Existing JSONL parser backend; do not replace in this phase |
| mdcad_math_harness target | in-repo CMake target | Compile gate + basic regression confidence | Use for D-07 compile gate before/after importer math edits |
| Quickstart runbook | docs artifact | Reproducible operator validation steps | Use to record and execute Phase 7 targeted checks |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Thin importer helper boundary | Broad math compatibility facade | Facade increases glue/maintenance and conflicts with D-06 |
| In-place touched-call migration | Full importer architecture rewrite | Rewrite is higher risk to chunk/progress semantics (D-10/D-11) |

**Installation:** vendored C dependencies already present; no new external package manager install required for Phase 7.

**Version verification:** versions confirmed from repository vendor sources (`vendors/cglm/VERSION.txt`, `vendors/flecs/flecs.h`, `vendors/cjson/cJSON.h`).

## Migration Cutlines (What Changes, What Must Not)

### In Scope (must migrate)
- `src/jsonl_loader.h`
  - Arc parsing math: normal normalization, basis generation (`cross`/`dot`), angle extraction.
- `src/jsonl_import_job.h`
  - CoM accumulation and shift, rotation matrix construction/composition, transformed point/normal/radius updates.
- `src/ply_loader.h`
  - Vec constructors and bounds initialization/mutation touchpoints used in parse paths.
- `src/ply_import_job.h`
  - CoM/rotation/scale transform pass on parsed point clouds.
- `src/ply_mesh_import_job.h`
  - CoM/rotation/scale transform pass on parsed mesh vertices.

### Out of Scope (must not expand)
- New import formats/features (D-12).
- Undo/editor migration (Phase 8).
- Broad harness expansion/perf gate expansion (Phase 9).
- Parser architecture rewrite, allocator redesign, or full memory-hardening initiative (can be noted as follow-up risks, not Phase 7 goals).

## Architecture Patterns

### Recommended Project Structure
```text
src/
├── math/
│  ├── cglm_entry.h                 # locked backend contract
│  └── [new importer helper].h      # thin importer-focused shared helper(s)
├── jsonl_loader.h                  # parser math path migration
├── jsonl_import_job.h              # runtime transform migration
├── ply_loader.h                    # parser math path migration
├── ply_import_job.h                # point-cloud transform migration
└── ply_mesh_import_job.h           # mesh transform migration
```

### Pattern 1: Thin Shared Helper + Local Specialization
**What:** Put reusable vec/mat transform primitives in `src/math/`, keep importer-unique logic (e.g., arc-specific interpretation) local.  
**When to use:** Shared CoM/rotation/scale transform math and other repeated operations across JSONL/PLY jobs.  
**Example:**
```c
// Source anchor: src/math/cglm_entry.h
// Pattern: importer helper returns project vec3_t/mat4_t while implemented via cglm-backed math.
static inline vec3_t mdcad_import_vec3_normalize_safe(vec3_t v) { ... }
static inline mat4_t mdcad_import_rotation_xyz(float rx, float ry, float rz) { ... }
```

### Pattern 2: Preserve Chunked Job State Machines
**What:** Keep parse->create->parent state progression, timing samples, and cancellation behavior intact while swapping math internals.  
**When to use:** All import job migrations in this phase.  
**Example:**
```c
// Source: src/jsonl_import_job.h / src/ply_import_job.h / src/ply_mesh_import_job.h
// Keep state transitions untouched; only replace math ops used inside transform application.
if (jsonl_is_complete(&job->parse_state)) {
    jsonl_import_job_apply_transforms(job);
    job->state = JSONL_JOB_CREATING_ENTITIES;
}
```

### Anti-Patterns to Avoid
- **Broad adapter/facade layer:** conflicts with D-06 and delays glue burn-down.
- **Changing transform order unintentionally:** if order changes, it must be a deliberate correctness fix with documented evidence (D-01/D-09).
- **Touching only import jobs and skipping loader math:** violates D-05.
- **Altering chunk/progress semantics while “just migrating math”:** violates D-10/D-11.

## Implementation Approach (Prescriptive)

1. **Inventory and classify math calls** in each scoped file:
   - Shared candidates (normalize, dot/cross, CoM accumulation, rotation composition, transform-point).
   - Importer-specific candidates (JSONL arc basis/angle interpretation).
2. **Add thin importer helper header(s)** under `src/math/` using cglm-backed internals and project types.
3. **Migrate runtime transform functions first** (`jsonl_import_job_apply_transforms`, `ply_import_job_apply_transforms`, `ply_mesh_import_job_apply_transforms`) because they are clear choke points.
4. **Migrate loader/parser math paths** (`jsonl_parse_arc3d`, PLY bounds/vec touchpoints) with no format/feature changes.
5. **Run compile gate immediately** after each file slice to catch header-only breakage early.
6. **Execute targeted importer checks** and capture required evidence fields for representative + variant/converted samples.
7. **Document deltas explicitly** where correctness-first behavior differs from legacy output (D-09).

## Plan Guidance for 07-01 .. 07-03

### 07-01: Migrate importer math touchpoints and update boundaries
**Must include:**
- Create/import new thin shared importer math helper(s) under `src/math/`.
- Migrate all five scoped importer files (D-05), not just one path.
- Keep state machine semantics (parse chunks, create chunks, parenting/cancel flow) unchanged.
- Add a mechanical audit command in plan acceptance criteria to detect remaining importer-scope `math3d` helper usage.

**Definition of done for 07-01:**
- Scoped importer files compile with cglm-backed helper usage in migrated paths.
- No new broad compatibility facade exists.
- No out-of-scope feature additions.

### 07-02: Add parity checks for importer geometry transforms and placement
**Must include:**
- A repeatable checklist/report template in Phase 7 evidence folder.
- Required recorded fields for each sample: placement, orientation, scale, entity/triangle counts, parenting structure (D-08).
- Coverage for representative and variant/converted samples (JSONL + PLY point + PLY mesh).
- Delta recording section: “intentional correctness changes vs prior behavior” (D-09).

**Definition of done for 07-02:**
- Evidence template exists and is executable.
- All required fields are explicit (not implied).

### 07-03: Validate native import workflows and capture evidence
**Must include:**
- Compile gate execution result.
- Targeted workflow execution for JSONL + PLY import modes with evidence attached.
- Explicit blocker check for chunk/progress/large-file regression (D-10/D-11).
- Update phase status/continuity docs after evidence capture.

**Definition of done for 07-03:**
- Evidence artifacts show pass/fail per required field and note any intentional behavioral deltas.
- No unresolved blocking regressions in chunk/perceived performance semantics.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Importer-wide math abstraction | New broad compatibility API | Thin helper(s) anchored to `cglm_entry.h` | Keeps boundary minimal and aligned with D-06 |
| New progress/state-machine framework | Replacement import pipeline engine | Existing chunked state machines in import-job headers | Preserves large-file UX contracts and minimizes risk |
| Ad-hoc evidence notes | One-off manual prose | Structured checklist/report with required fields | Ensures D-08/D-09 auditable parity evidence |

**Key insight:** The highest-risk regressions in this phase are behavioral contracts (placement/orientation/scale and chunked UX), not missing math primitives.

## Runtime State Inventory

| Category | Items Found | Action Required |
|----------|-------------|------------------|
| Stored data | No internal datastore migration found; importer consumes external JSONL/PLY files at runtime. | Code edit only (no repository data migration). |
| Live service config | None found; importer behavior is in-process and file-based, with no external service config in scope. | None. |
| OS-registered state | None found for importer migration slice. | None. |
| Secrets/env vars | None found for importer math path behavior. | None. |
| Build artifacts | Existing binaries in `build*` directories can retain pre-migration importer behavior. | Rebuild before validation evidence capture. |

## Common Pitfalls

### Pitfall 1: Transform-order drift without explicit intent
**What goes wrong:** Rotation/scale/CoM sequencing changes accidentally during helper migration.  
**Why it happens:** Refactor merges steps while replacing vec/mat calls.  
**How to avoid:** Preserve existing sequence unless intentionally correcting behavior; document any delta (D-01/D-09).  
**Warning signs:** Placement/scale appears plausible but orientation or offset differs from baseline.

### Pitfall 2: Arc orientation/basis regressions in JSONL loader
**What goes wrong:** Arc start/end angles or normals become flipped/offset.  
**Why it happens:** Basis vectors (`arbitrary`, `cross`, `dot`, `atan2`) are sensitive to normalization and axis fallback logic.  
**How to avoid:** Keep arc-local logic intact; only swap math primitives under same semantics, then re-check orientation evidence fields.  
**Warning signs:** Imported arc appears mirrored, rotated, or sweeps wrong direction.

### Pitfall 3: Chunk/progress behavior regression masked as “math change”
**What goes wrong:** Import feels slower, progress messaging drifts, or large-file responsiveness degrades.  
**Why it happens:** Extra allocations/computation inserted in hot chunk loops.  
**How to avoid:** Keep chunk loops/weights stable; avoid expensive per-item conversions inside tight loops where possible.  
**Warning signs:** Status message cadence changes, progress stalls/jumps, or import UI hitching on large samples.

### Pitfall 4: Partial migration leaves mixed legacy/helper calls
**What goes wrong:** Some paths remain on legacy helpers, causing inconsistency and violating TAIL-02 intent.  
**Why it happens:** Multiple importer files and modes make it easy to miss a branch.  
**How to avoid:** Add explicit grep-based completion checks for scoped files and review both loader + import-job surfaces.  
**Warning signs:** `math3d` helper symbols still present in in-scope importer math paths after migration.

## Required Validation Evidence

Phase 7 evidence should include (at minimum):

1. **Compile gate result** (command, timestamp, pass/fail).
2. **Representative sample report** (JSONL + PLY point + PLY mesh where applicable):
   - placement
   - orientation
   - scale
   - entity/triangle counts
   - parenting structure
3. **Variant/converted sample report** with same fields.
4. **Correctness delta note section**:
   - “No visible delta” OR explicit list of intentional differences with rationale.
5. **Chunk/large-file safety note**:
   - confirmation that chunking/progress/perceived responsiveness is not regressed.

## Code Examples

### Existing JSONL transform choke point
```c
// Source: src/jsonl_import_job.h
static inline void jsonl_import_job_apply_transforms(jsonl_import_job_t *job) {
    // CoM shift -> rotation (X/Y/Z) -> scale across element geometry
}
```

### Existing PLY transform choke point
```c
// Source: src/ply_import_job.h
static inline void ply_import_job_apply_transforms(ply_import_job_t *job) {
    // CoM shift -> rotation -> scale across parsed points
}
```

### Existing cglm entrypoint policy anchor
```c
// Source: src/math/cglm_entry.h
_Static_assert(CGLM_CONFIG_CLIP_CONTROL == CGLM_CLIP_CONTROL_RH_ZO,
               "mdCAD cglm entrypoint must use RH_ZO clip control");
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Legacy `math3d` helpers directly in importer math paths | cglm-backed migration through thin, project-owned entrypoint boundaries | v1.0-v1.1 staged migration | Phase 7 should continue thin-boundary pattern, not introduce facade creep |
| Broad heavy validation for hotspot phases | Light gate + targeted evidence for long-tail phases | Phase 6/7 context decisions | Faster execution, but evidence discipline is mandatory |

**Deprecated/outdated:**
- Big-bang repo-wide `math3d` removal in one phase.
- Expanding Phase 7 into new importer features or unrelated subsystems.

## Open Questions

1. **Exact helper API shape for importer migration**
   - What we know: shared helper use is preferred where practical (D-04), facade is disallowed (D-06).
   - What's unclear: exact function set/naming split between shared vs local.
   - Recommendation: start from three shared primitives (`normalize_safe`, `rotation_xyz`, `transform_point`) and keep arc-specific operations local.

2. **What qualifies as correctness-driven visible delta**
   - What we know: such changes are allowed and must be documented (D-01/D-09).
   - What's unclear: threshold for “visible” in operator workflows.
   - Recommendation: treat any changed placement/orientation/scale result in targeted checks as visible and explicitly annotate.

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| cmake | Compile gate and validation targets | ✓ | 4.3.0 | — |
| python | Optional evidence/checklist scripting and converter workflow reuse | ✓ | 3.13.12 | Manual checklist updates |
| node | Existing project script ecosystem | ✓ | v25.8.1 | — |
| ninja | Optional build path | ✗ | — | use configured `build-vulkan` via `cmake --build` |
| build-vulkan directory | Native validation command path in this workspace | ✓ | configured | configure if missing |

**Missing dependencies with no fallback:**
- None identified for Phase 7 execution in current environment.

**Missing dependencies with fallback:**
- `ninja` (fallback: Visual Studio generator build via `cmake --build build-vulkan --config Release ...`).

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | Native CMake targets + targeted importer workflow evidence |
| Config file | `src/CMakeLists.txt` |
| Quick run command | `cmake --build build-vulkan --config Release --target mdcad_math_harness` |
| Full suite command | `cmake --build build-vulkan --config Release --target math-validation` |

### Phase Requirements → Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| TAIL-02 | JSONL/PLY import math paths use cglm-backed migration with preserved user-visible behavior | targeted integration/manual + compile | `cmake --build build-vulkan --config Release --target mdcad_math_harness` | ⚠️ Partial (compile exists; importer-targeted report/checklist for Phase 7 must be added) |

### Sampling Rate
- **Per task commit:** compile gate (`mdcad_math_harness` target).
- **Per wave merge:** `math-validation` + targeted importer checklist update.
- **Phase gate:** compile green + completed evidence matrix (D-08/D-09) + no D-11 blockers.

### Wave 0 Gaps
- [ ] Add Phase 7 importer targeted checklist/report artifacts under `.planning/phases/07-import-pipeline-long-tail-migration/evidence/`.
- [ ] Add explicit Quickstart Phase 7 workflow section mirroring Phase 6 style.
- [ ] Add grep-based completion checks proving in-scope importer math-path migration is complete.

## Sources

### Primary (HIGH confidence)
- `.planning/phases/07-import-pipeline-long-tail-migration/07-CONTEXT.md` — locked decisions, scope, validation rules.
- `.planning/REQUIREMENTS.md` — TAIL-02 contract.
- `.planning/ROADMAP.md` — plan skeleton and phase success criteria.
- `.planning/STATE.md` / `.planning/PROJECT.md` — locked backend/adoption constraints.
- `src/jsonl_loader.h`, `src/jsonl_import_job.h`, `src/ply_loader.h`, `src/ply_import_job.h`, `src/ply_mesh_import_job.h` — in-scope runtime code paths.
- `src/math/cglm_entry.h`, `src/math/math_conventions.h`, `src/math/math_interaction.h`, `src/math/math_quat.h` — established cglm migration boundary patterns.
- `src/CMakeLists.txt`, `docs/QUICKSTART.md` — validation command and runbook conventions.
- `.planning/codebase/CONCERNS.md`, `.planning/codebase/TESTING.md` — importer risk/testing gap context.

### Secondary (MEDIUM confidence)
- `.planning/phases/06-serializer-and-save-load-long-tail-migration/06-RESEARCH.md` and Phase 6 plan/evidence artifacts — used as process precedent for light-gate long-tail planning style.

### Tertiary (LOW confidence)
- None.

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — locked by project/vendor sources.
- Architecture: HIGH — directly grounded in current in-scope importer source and phase decisions.
- Pitfalls: HIGH — derived from explicit code touchpoints and known concerns doc.

**Research date:** 2026-03-27  
**Valid until:** 2026-04-26
