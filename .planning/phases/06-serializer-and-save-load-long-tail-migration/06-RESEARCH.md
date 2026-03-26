# Phase 6: Serializer and Save/Load Long-Tail Migration - Research

**Researched:** 2026-03-27  
**Domain:** Serializer/save-load migration to cglm-backed math paths (TAIL-01)  
**Confidence:** HIGH

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
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

### Deferred Ideas (OUT OF SCOPE)
- Import pipeline migration details (`jsonl`/`ply`) — deferred to Phase 7.
- Undo/editor utility migration details and broader glue burn-down — deferred to Phase 8.
- Expanded strict harness/performance closure for long-tail surfaces — deferred to Phase 9.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| TAIL-01 | User can save and reload scene data through cglm-backed serializer math paths without reintroducing migrated `math3d` helper dependencies. | Recommends serializer-local cglm bridge helpers, two-pass load retention, explicit legacy-helper retirement list, converter workflow, and targeted save/reload checks for entity count/parent links/transforms/geometry types. |
</phase_requirements>

## Summary

Phase 6 should be planned as a **surgical serializer migration**: keep existing save/load flow shape (`scene_save_to_file/string`, `scene_load_from_file/string`) and UI entry points in `ui_scene_hierarchy.h`, but move serializer-internal math defaults/conversions to cglm-backed helpers anchored by `src/math/cglm_entry.h`. This preserves behavior while meeting TAIL-01 and avoids scope leak into importer/undo/editor paths.

Current serializer behavior is structurally sound for migration: it already has centralized parse/write helpers, a two-pass load path (create entities first, then parent relink via `scene_set_parents_batch`), and focused UI status plumbing (`Saved X entities`, `Loaded X entities`, fail strings). These are good choke points for migration and targeted verification.

Because format cleanup is allowed (and old-format compatibility is not required), the safest plan is: define a cleaned format contract explicitly, update serializer to emit/read it, and ship a converter for old files. Validation should stay light per decision D-07: compile plus targeted save/reload checks covering exactly D-08 fields.

**Primary recommendation:** Keep architecture stable, replace serializer-local legacy math helpers with cglm-backed helper boundaries, and pair format cleanup with a documented converter + targeted parity checks.

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| cglm (vendored) | 0.9.6 (`vendors/cglm/VERSION.txt`) | Canonical math backend for migrated paths | Locked project backend; enforced by `src/math/cglm_entry.h` and milestone decisions |
| Flecs (vendored) | 4.1.4 (`vendors/flecs/flecs.h`) | ECS entity/component queries and parenting | Existing scene/serializer runtime contract depends on it |
| Serializer module | in-repo header-only (`src/scene_serializer.h`) | Scene JSON save/load and parse pipeline | Existing production choke point for TAIL-01 |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| cJSON (vendored) | 1.7.19 (`vendors/cjson/cJSON.h`) | Optional converter-side JSON handling | Prefer for standalone converter utility instead of extending fragile ad-hoc parser logic |
| CMake targets (`mdcad_math_harness`, `math-regression`) | in-repo | Build and quick validation hooks | Use for compile gate and baseline confidence before/after serializer changes |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| In-place backward compatibility layer | Hard break + explicit converter | Faster cleaner serializer internals; requires operator migration step (acceptable per D-05/D-06) |
| Broad UI refactor around save/load | Minor status/wording polish only | Reduces risk and respects D-02 scope |

**Version verification:** vendored versions were verified directly from repository source-of-truth files (`vendors/cglm/VERSION.txt`, `vendors/flecs/flecs.h`, `vendors/cjson/cJSON.h`).

## Architecture Patterns

### Recommended Project Structure
```text
src/
├── scene_serializer.h          # Primary migration target (save/load + parser)
├── ui/ui_scene_hierarchy.h     # Save/load invocation + concise status UX only
└── math/
   ├── cglm_entry.h             # Canonical cglm contract
   └── [new serializer helper]  # Thin serializer-focused cglm adapter(s)
scripts/
└── [scene converter script]    # Old-format -> Phase 6 cleaned format
docs/
└── QUICKSTART.md               # Converter + targeted validation runbook
```

### Pattern 1: Keep Two-Pass Load + Parent Batch Relink
**What:** Parse entities, create all entities first, then resolve parent links in one batched pass.  
**When to use:** Always for scene load to preserve hierarchy integrity and avoid order-dependent parent failures.  
**Example:**
```c
// Source: src/scene_serializer.h
for (int i = 0; i < entity_count; i++) {
    entities[i].new_entity = scene_create_from_loaded(scene, &entities[i]);
}
// then map old parent IDs to new IDs and call:
scene_set_parents_batch(scene, load_children, load_parents, parent_count);
```

### Pattern 2: Serializer-Local Math Bridge
**What:** Introduce serializer-local helper(s) that convert/initialize vector values via cglm-backed semantics, rather than scattering legacy helper calls (`vec3_make`, etc.) throughout parser branches.  
**When to use:** Defaults, parse assignment normalization, and transform copy logic in serializer scope.  
**Example:**
```c
// Source anchor: src/math/cglm_entry.h
// pattern: helper wraps cglm vec operations but returns project vec3_t/vec4_t
static inline vec3_t scene_ser_vec3_default_zero(void) { ... }
```

### Anti-Patterns to Avoid
- **Touching importer/undo/editor files in this phase:** explicitly out of scope (D-03).
- **Broad save/load UI redesign:** violates D-02; only clarity polish allowed.
- **Changing hierarchy wiring model:** unnecessary risk; keep current create-then-parent behavior.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Parent remap on load | Ad-hoc per-entity immediate parent assignment | Existing second-pass `scene_set_parents_batch` flow | Avoids ordering bugs and keeps O(n)-style batch behavior |
| JSON conversion migration | Manual one-off text replacement | Dedicated converter script/tool with parse+write | Reduces corruption risk and gives reproducible migration path |
| Serializer math defaults | Repeated direct legacy helper literals all over parser | Thin serializer cglm-backed helper boundary | Centralized retirement of legacy helper usage |

**Key insight:** most serializer risk is not math complexity but **state integrity** (hierarchy + component data consistency). Reuse existing structural choke points instead of inventing new control flow.

## Runtime State Inventory

| Category | Items Found | Action Required |
|----------|-------------|------------------|
| Stored data | User scene JSON files on disk may contain old serializer format versions. | **Data migration**: provide converter workflow (required by D-06). |
| Live service config | None found (phase is local file serializer/UI logic; no external service config in scope). | None — verified from scoped files and project docs. |
| OS-registered state | None found (no scheduler/service registration tied to serializer format). | None. |
| Secrets/env vars | None found for serializer/save-load path. | None. |
| Build artifacts | Existing binaries in `build*` dirs may embed pre-migration serializer behavior. | **Code edit + rebuild**: recompile target build after migration before validation. |

## Common Pitfalls

### Pitfall 1: Silent Parent Loss After Load
**What goes wrong:** entity count matches but hierarchy is flattened/incorrect.  
**Why it happens:** old->new ID remap or parent assignment pass is changed incorrectly.  
**How to avoid:** preserve two-pass create+batch-parent pattern; verify parent link set explicitly in targeted checks.  
**Warning signs:** loaded entity count passes while child objects appear at root level.

### Pitfall 2: Transform Defaults Drift
**What goes wrong:** entities load with altered default scale/rotation/position semantics.  
**Why it happens:** inconsistent defaults across parse paths during helper migration.  
**How to avoid:** centralize defaults in serializer-local helper(s) and test `position/rotation/scale` field round-trip.  
**Warning signs:** identity-scene reload shows non-unit scale or shifted positions.

### Pitfall 3: Geometry Type/Data Mismatch
**What goes wrong:** type string parses but wrong union branch receives data.  
**Why it happens:** format cleanup changes keys/type names without coordinated parser updates/converter rules.  
**How to avoid:** freeze cleaned schema and test representative set (point/line/polyline/polygon/arc/bezier/helix/triangle/mesh + light).  
**Warning signs:** load succeeds but geometry renders incorrectly or missing.

## Code Examples

Verified patterns from project sources:

### Save/Load UI plumbing with bounded scope
```c
// Source: src/ui/ui_scene_hierarchy.h
if (state->file_browser.mode == FILE_BROWSER_MODE_SAVE) {
    scene_save_to_file(state->scene, path);
} else {
    int count = scene_load_from_file(state->scene, path, state->clear_on_load);
}
```

### Transform application after entity creation
```c
// Source: src/scene_serializer.h
TransformComp *t = ecs_world_get_transform(scene->world, e);
if (t) {
    t->position = ent->position;
    t->rotation = ent->rotation;
    t->scale = ent->scale;
    t->dirty = true;
}
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Legacy `math3d`-style helper usage scattered in long-tail paths | cglm-first migration via thin entrypoint contract (`src/math/cglm_entry.h`) | v1.0 phases 1-5 | Phase 6 must align serializer with established migration pattern |
| Strict native gate for hotspot phases | Light gate for serializer phase (compile + targeted checks) | Phase 6 decisions | Faster iteration, but targeted checks must be explicit and disciplined |

**Deprecated/outdated:**
- Re-expanding serializer scope into importer/undo/editor migration in this phase (deferred by roadmap/context).

## Migration Order (Recommended)

1. **Map serializer math touchpoints** in `scene_serializer.h` (defaults, vec assignments, transform handling).  
2. **Add serializer-local cglm-backed helper boundary** under `src/math/` and wire serializer to it.  
3. **Retire legacy helper usage in serializer scope** (aggressive within this file only, per D-09/D-10).  
4. **Apply format cleanup** (if desired) and lock cleaned schema.  
5. **Implement converter workflow** (script/tool + usage docs in `docs/QUICKSTART.md`).  
6. **Small UI clarity polish** in `ui_scene_hierarchy.h` status messages only (no redesign).  
7. **Run light gate validation**: compile + targeted save/reload checks for D-08 fields.  
8. **Capture evidence** in phase artifacts and quickstart runbook.

## Verification Strategy (Light Gate Aligned)

- **Compile gate:** build configured target (`cmake --build build-vulkan --config Release --target mdcad_math_harness` proved workable in environment).  
- **Targeted save/reload checks (required):**
  1. Save representative scene.
  2. Reload scene.
  3. Verify entity count.
  4. Verify parent links (at least one multi-level hierarchy case).
  5. Verify transform fields (position/rotation/scale round-trip).
  6. Verify geometry types survive round-trip.
- **Converter check:** convert old-format sample -> load converted output -> run same checks.

## Open Questions

1. **Converter implementation location**
   - What we know: scripts directory already hosts Python utilities; format break is allowed and converter is required.
   - What's unclear: preferred runtime (Python vs C utility).
   - Recommendation: Python converter in `scripts/` for fastest delivery and operator ergonomics.

2. **Cleaned schema scope**
   - What we know: cleanup is allowed; compatibility not required.
   - What's unclear: exact key-level changes to make now vs defer.
   - Recommendation: keep schema changes minimal and value-adding (consistency/clarity), avoid speculative redesign.

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| cmake | Compile gate | ✓ | 4.3.0 | — |
| build-vulkan configured toolchain | Compile gate in this workspace | ✓ | MSBuild 18.0.5 via `cmake --build` | Reconfigure build dir if stale |
| python | Converter workflow (recommended) | ✓ | 3.13.12 | C utility in repo if Python disallowed |
| ninja | Optional alt build path | ✗ | — | Use existing Visual Studio generator via `cmake --build build-vulkan` |
| direct `msbuild` CLI in PATH | Optional direct invocation | ✗ | — | Use `cmake --build` wrapper (works) |

**Missing dependencies with no fallback:**
- None identified for this phase in current environment.

**Missing dependencies with fallback:**
- `ninja`, direct `msbuild` command.

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | Custom native harness target (`mdcad_math_harness`) + targeted manual/scripted serializer checks |
| Config file | none — CMake custom targets in `src/CMakeLists.txt` |
| Quick run command | `cmake --build build-vulkan --config Release --target mdcad_math_harness` |
| Full suite command | `cmake --build build-vulkan --config Release --target math-validation` |

### Phase Requirements → Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| TAIL-01 | Save/reload via migrated serializer path without legacy helper regression | targeted integration/manual | `cmake --build build-vulkan --config Release --target mdcad_math_harness` then run serializer save/reload checklist | ❌ Wave 0 (dedicated serializer check harness/script not present) |

### Sampling Rate
- **Per task commit:** compile gate + one representative save/reload check.
- **Per wave merge:** compile gate + full targeted save/reload matrix (entity count, parents, transforms, geometry types).
- **Phase gate:** D-07/D-08 light gate complete with recorded evidence.

### Wave 0 Gaps
- [ ] Add a repeatable serializer targeted-check script/checklist artifact for TAIL-01.
- [ ] Add converter usage/verification steps to `docs/QUICKSTART.md`.
- [ ] Add sample old-format + converted-format scene fixture paths in phase evidence folder.

## Sources

### Primary (HIGH confidence)
- `.planning/phases/06-serializer-and-save-load-long-tail-migration/06-CONTEXT.md` — locked decisions, scope boundaries, validation constraints.
- `src/scene_serializer.h` — current serializer architecture, parse/write behavior, load ordering.
- `src/ui/ui_scene_hierarchy.h` — save/load invocation and status UX plumbing boundaries.
- `src/math/cglm_entry.h` and `src/math/math_conventions.h` — cglm contract and project math policy.
- `.planning/REQUIREMENTS.md`, `.planning/ROADMAP.md`, `.planning/STATE.md`, `.planning/PROJECT.md` — requirement/phase traceability and milestone constraints.
- `src/CMakeLists.txt` — existing validation target commands.
- `docs/QUICKSTART.md` — existing operator runbook conventions.

### Secondary (MEDIUM confidence)
- `AGENTS.md` — project workflow conventions and platform build notes (helpful but not source of locked phase decisions).

### Tertiary (LOW confidence)
- None.

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — derived from locked project/milestone files and vendored versions.
- Architecture: HIGH — directly verified in serializer/UI source.
- Pitfalls: HIGH — grounded in concrete current code paths and required gate checks.

**Research date:** 2026-03-27  
**Valid until:** 2026-04-26
