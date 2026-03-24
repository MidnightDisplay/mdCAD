# Phase 1: Selection and Conventions - Context

**Gathered:** 2026-03-24
**Status:** Ready for planning

<domain>
## Phase Boundary

Select and vendor `cglm` `0.9.6` with minimal build disruption, define mdCAD's project-wide math and alignment contract, and lock the adoption strategy for subsequent phases. This phase establishes the direct-adoption path and convention baseline that later migration work must follow; it does not yet widen the runtime rollout beyond the selection, integration, and convention decisions needed to start implementation.

</domain>

<decisions>
## Implementation Decisions

### Backend commitment
- **D-01:** `cglm` `0.9.6` is the chosen math backend for the first migration milestone.
- **D-02:** The backend choice only reopens for a hard build-system or license blocker.
- **D-03:** Planning should not carry an active fallback candidate; if `cglm` fails on a hard blocker, the project should explicitly reopen research instead of shadow-planning backups.

### Adoption surface
- **D-04:** The migration path is direct `cglm` adoption in mdCAD code, not a long-lived project-owned compatibility facade.
- **D-05:** This intentionally supersedes the current `FOUND-02` / Phase 2 assumption that mdCAD will expose a stable compatibility layer around the vendor library; downstream planning should revise that roadmap/requirements language instead of preserving it.
- **D-06:** `cglm` API family may vary by subsystem. Planners and executors may choose struct or array-style usage per subsystem instead of enforcing one global style, as long as the global conventions and alignment contract remain consistent.

### Alignment and SIMD policy
- **D-07:** Phase 1 is performance-first with respect to `cglm` adoption; the project may accept early alignment work to unlock the stronger SIMD path.
- **D-08:** Data-layout refactors are acceptable when needed to satisfy the chosen alignment strategy, including hot structs and other math-heavy storage, not only thin adapters.
- **D-09:** mdCAD must define one global alignment policy with explicit rules and enforcement rather than allowing per-subsystem drift or implicit compiler defaults.
- **D-10:** If a supported native gate cannot satisfy the chosen alignment policy cleanly, the migration slice should block instead of silently dropping to a weaker fallback mode.

### Math conventions
- **D-11:** The migration will standardize mdCAD on a new project-wide math convention instead of preserving `src/math3d.h` semantics as the internal baseline.
- **D-12:** The target convention is column-major matrices, right-handed coordinates, and clip depth `0..1`.
- **D-13:** That convention is global across supported backends; backend glue may adapt, but the math semantics should not fork per backend.
- **D-14:** During staged rollout, migrated runtime slices should still normalize user-visible behavior so camera, picking, and gizmo interaction feel remain stable while the internal convention changes land.

### Vendor integration mode
- **D-15:** Phase 1 starts with header-only / inline `cglm`.
- **D-16:** Initial vendoring should use the least disruptive CMake wiring possible, centered on header vendoring and minimal include-path changes.
- **D-17:** Linked `cglm` is deferred; reconsider it only if measurable compile-time, binary-size, or runtime pressure appears after initial adoption.

### the agent's Discretion
- Choose struct vs array `cglm` API usage per subsystem based on code shape and measurable cost.
- Decide the exact enforcement mechanism for the global alignment policy, such as compile-time assertions, helper headers, or targeted macros.
- Decide how to normalize visible behavior during rollout while the internal `0..1` clip-depth convention is introduced.
- Decide the exact file layout for vendored headers and minimal CMake wiring as long as it stays lightweight and consistent with existing build patterns.

</decisions>

<specifics>
## Specific Ideas

- The user wants Phase 1 to commit directly to `cglm` `0.9.6`, not continue carrying backup candidates through planning.
- The user deliberately rejected a long-lived project-owned compatibility facade even though current research and requirements assumed one.
- The desired internal target is a modern GPU-style convention: column-major, right-handed, clip depth `0..1`.
- Even with that internal convention change, camera, picking, and gizmo behavior should feel stable to the user during staged rollout.
- The user is willing to refactor data layout early if that is what a performance-first SIMD/alignment strategy requires.

</specifics>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Project scope and decision constraints
- `.planning/PROJECT.md` — project goals, non-negotiables, rollout priorities, and license/C-only constraints for the migration
- `.planning/REQUIREMENTS.md` — requirement mapping for Phase 1, including the current `FOUND-01` / `FOUND-03` scope and the now-conflicting `FOUND-02` compatibility-layer assumption
- `.planning/ROADMAP.md` — official phase goals, success criteria, and current plan structure that this context partially overrides
- `.planning/STATE.md` — current project position, active focus, and notes about unresolved Phase 1 questions

### Research and adoption rationale
- `.planning/research/SUMMARY.md` — top-level recommendation for `cglm`, adoption risks, and phase-level research flags
- `.planning/research/STACK.md` — `cglm` integration modes, version recommendation, and candidate-library comparison
- `.planning/research/ARCHITECTURE.md` — previously recommended wrapper-first architecture that this context deliberately revises
- `.planning/research/FEATURES.md` — desired migration properties, especially convention control and staged rollout risks
- `.planning/research/PITFALLS.md` — convention drift, alignment/SIMD risks, and rollout failure modes relevant to the user's decisions

### Current math semantics and runtime usage
- `src/math3d.h` — current vector/matrix/ray surface, explicit column-major note, and baseline semantics being replaced
- `src/orbit_camera.h` — camera math usage and current value-style field access patterns
- `src/app.c` — current view/projection/MVP composition, inverse/ray usage, and runtime behavior that must remain stable during rollout
- `src/ecs/ecs_scene.h` — transform-heavy geometry creation/update paths that will be early consumers of the new adoption style
- `src/gpu/geometry_batch.h` — uniform structs embedding `mat4_t`, revealing layout/alignment sensitivity in render paths

### Build and platform wiring
- `src/platform.h` — backend selection across Metal, Vulkan, D3D11, GLES3, WebGPU, and GLCore
- `CMakeLists.txt` — top-level build contract and the current preference for lightweight native integration
- `src/CMakeLists.txt` — executable/source wiring and the likely integration point for minimal `cglm` vendoring changes

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `src/math3d.h`: current mdCAD math surface and semantics map; planners should use it as the baseline inventory for what gets replaced or normalized.
- `src/orbit_camera.h`: compact first-class hotspot for direct `cglm` adoption, with clear camera/view-matrix behavior and minimal unrelated dependencies.
- `src/app.c`: centralized view/projection/MVP, inverse, and ray usage that exposes convention changes quickly during validation.
- `src/ecs/ecs_scene.h`: transform-heavy runtime code with repeated vector/matrix operations; a good pressure test for mixed struct/array `cglm` usage by subsystem.
- `src/gpu/geometry_batch.h`: render-uniform structs currently embedding `mat4_t`, making alignment/layout policy concrete rather than abstract.

### Established Patterns
- Header-only modules and `static inline` helpers dominate `src/`, so any new math wiring should respect low-friction inclusion patterns unless measurement proves otherwise.
- Current runtime math is value-struct based with direct field access (`.x`, `.y`, `.z`, `.m[...]`) and pass-by-value use in many headers.
- The build system is intentionally lightweight and centralized in the top-level and `src/` CMake files; Phase 1 should avoid introducing unnecessary build graph complexity.
- Math is currently included directly in many modules instead of behind an existing abstraction layer, which makes a direct-adoption strategy viable but increases the importance of consistent conventions.

### Integration Points
- Vendored `cglm` headers will plug into the native build through `CMakeLists.txt` and `src/CMakeLists.txt` with minimal include-path changes.
- Convention and alignment policy needs one project-owned home that downstream phases can treat as authoritative even though the user rejected a broad compatibility facade.
- Early migration planning should treat `src/orbit_camera.h`, `src/app.c`, `src/ecs/ecs_scene.h`, and `src/gpu/geometry_batch.h` as the reference subsystems where convention and alignment decisions will show consequences fastest.
- Because the user accepted mixed `cglm` API styles by subsystem, plan tasks should make per-subsystem API choices explicit instead of leaving them implicit.

</code_context>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope.

</deferred>

---
*Phase: 01-selection-and-conventions*
*Context gathered: 2026-03-24*
