# Phase 4: Interaction Math and API Expansion - Context

**Gathered:** 2026-03-25
**Status:** Ready for planning

<domain>
## Phase Boundary

Migrate interaction-sensitive math in the pick/unproject/ray and gizmo drag/intersection paths to the adopted `cglm` foundation, then add the richer helper surface needed by those migrated paths (including quaternion-capable expansion) while retiring equivalent `src/math3d.h` helpers in the migrated slices. This phase is scoped to interaction math and its immediate API boundary; broad repo-wide math cleanup remains out of scope.

</domain>

<decisions>
## Implementation Decisions

### Pick and screen-ray migration
- **D-01:** Phase 4 should migrate runtime pick/unproject/ray work to a `cglm`-first path through a project-owned helper boundary rather than keeping per-call-site legacy inverse-VP + `ray_from_screen(...)` logic.
- **D-02:** `src/app.c` interaction call sites should consume the shared helper boundary, not open-code divergent unproject math.
- **D-03:** Parity for this migration remains behavior-first: selection/hover interaction feel and accuracy must stay stable while internals switch to `cglm`.

### Gizmo drag and intersection migration
- **D-04:** Axis/plane drag intersection and delta math should migrate through shared `cglm`-backed helpers reused by both `src/gizmo/gizmo.h` and `src/gizmo/gizmo_vertex_mode.h`.
- **D-05:** The migration should keep gizmo interaction feel parity-first in this phase; no intentional UX retuning is required to declare success.
- **D-06:** Vertex-mode world/local delta conversion should move off legacy inverse/direction math and align with the new shared helper surface.

### Helper surface expansion
- **D-07:** Phase 4 should add a project-owned helper set under `src/math/` that covers interaction needs plus expansion targets: project/unproject, ray-axis/plane helpers, and quaternion-capable utilities.
- **D-08:** Helper additions must stay thin and avoid reintroducing vendor type aliases or a compatibility facade.
- **D-09:** Runtime subsystems should consume these helpers consistently where the phase migrates behavior, rather than mixing ad hoc direct calls and legacy helpers.

### Legacy retirement boundary
- **D-10:** Equivalent `src/math3d.h` helpers should be retired in migrated pick/gizmo/app interaction slices during Phase 4.
- **D-11:** Retirement outside those migrated slices is deferred; this phase should not expand into broad repo-wide helper removal.

### the agent's Discretion
- Choose exact helper file names and API signatures under `src/math/` as long as they remain thin, explicit, and aligned with direct `cglm` adoption.
- Choose struct vs array `cglm` internals per helper based on local code shape and measurable clarity/performance.
- Choose exact parity checks and harness case updates needed to keep interaction regressions detectable.

</decisions>

<specifics>
## Specific Ideas

- Route pick/unproject/ray through one shared `cglm`-backed project helper boundary and wire `src/app.c` to it.
- Route gizmo axis/plane drag math and vertex-mode world/local delta math through shared helpers instead of file-local duplicated legacy math.
- Expand the helper surface in Phase 4 rather than deferring quaternion-capable coverage.
- Keep migration parity-first for interaction feel while replacing internals.

</specifics>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Phase scope and requirements
- `.planning/ROADMAP.md` — Phase 4 goal, plans, and success criteria
- `.planning/REQUIREMENTS.md` — `HOT-03`, `EXP-01`, and `EXP-02` requirement intent
- `.planning/PROJECT.md` — migration constraints, rollout priorities, and C-only/MIT posture
- `.planning/STATE.md` — current project position and active migration notes

### Prior locked migration decisions
- `.planning/phases/01-selection-and-conventions/01-CONTEXT.md` — global math contract and direct-adoption decisions
- `.planning/phases/02-direct-adoption-tooling-and-validation-harness/02-CONTEXT.md` — harness-first validation and helper-boundary posture
- `.planning/phases/03-macos-core-transform-migration/03-CONTEXT.md` — Phase 3 bridge boundaries and deferred interaction scope
- `.planning/phases/03-macos-core-transform-migration/03-VERIFICATION.md` — latest parity gate status and remaining human checks

### Research and contract docs
- `.planning/research/FEATURES.md` — expansion opportunities and staged rollout posture
- `.planning/research/PITFALLS.md` — convention drift, interaction regression, and scope-control risks
- `docs/MATH_CONVENTIONS.md` — project-wide matrix/handedness/clip-depth contract and rollout-sensitive hotspots
- `docs/MATH_BACKEND_DECISION.md` — locked direct `cglm` adoption and mixed API-family posture

### Runtime interaction hotspots
- `src/app.c` — pick pass orchestration, screen-ray construction, and gizmo drag loop integration
- `src/gpu/pick_buffer.h` — pick MVP construction and pick render/readback path
- `src/gizmo/gizmo.h` — gizmo drag state machine and axis/plane interaction math
- `src/gizmo/gizmo_vertex_mode.h` — vertex-mode world/local transform math during drag
- `src/gizmo/gizmo_rendering.h` — gizmo render/pick overlay coupling
- `src/math3d.h` — legacy helpers being retired in migrated slices
- `src/math/cglm_entry.h` — thin entrypoint and contract enforcement
- `src/math_harness.c` — existing parity/bench harness to extend for interaction math checks

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `src/math/cglm_entry.h`: existing thin migration boundary that already enforces clip/depth/layout policy.
- `src/math_harness.c`: existing strict compare and bench framework with screen-ray parity coverage (`screen-ray-unproject`) that can be expanded for Phase 4 interaction math.
- `src/app.c`: central interaction orchestration point already bridging cglm camera matrices to legacy consumers; this is the integration choke point for pick/ray migration.
- `src/gpu/pick_buffer.h`: centralized pick projection/MVP pipeline and cursor-gated rebuild logic.
- `src/gizmo/gizmo.h` + `src/gizmo/gizmo_vertex_mode.h`: concentrated drag/intersection math and world/local delta handling.

### Established Patterns
- Header-only `static inline` modules remain the dominant pattern for runtime math-adjacent code.
- Harness-first validation is the established automated gate before manual viewport interaction smoke checks.
- Direct `cglm` adoption with thin project-owned boundaries is locked; wrapper-style type facades are explicitly disallowed.
- Runtime still contains explicit bridge copies between `mat4s` and `mat4_t` at subsystem boundaries.

### Integration Points
- New Phase 4 helpers should live under `src/math/` and be consumed from `src/app.c`, `src/gpu/pick_buffer.h`, and gizmo modules.
- `src/math_harness.c` should absorb new interaction-focused compare cases to detect pick/gizmo regressions early.
- Legacy helper retirement should be scoped to migrated interaction paths in app/pick/gizmo code to avoid widening the phase.

</code_context>

<deferred>
## Deferred Ideas

- Broader aggressive retirement of `src/math3d.h` helpers outside migrated interaction slices (candidate for later phase or milestone cleanup).

</deferred>

---

*Phase: 04-interaction-math-and-api-expansion*
*Context gathered: 2026-03-25*
