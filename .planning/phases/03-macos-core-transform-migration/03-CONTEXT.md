# Phase 3: macOS Core Transform Migration - Context

**Gathered:** 2026-03-24
**Status:** Ready for planning
**Source:** Execute-mode fallback (`03-DISCUSSION-LOG.md`)

<domain>
## Phase Boundary

Move the orbit camera, app-side viewport matrix construction, ECS transform composition, and world-matrix-driven scene update/render inputs onto the new math foundation on the primary macOS workflow. This slice should preserve the current macOS Metal camera feel and visible scene output while avoiding a premature expansion into pick/unproject/gizmo math or Windows hardening work.

</domain>

<decisions>
## Implementation Decisions

### Camera and app matrix path
- **D-01:** `src/orbit_camera.h` and the view/projection/MVP setup in `src/app.c` should bias toward the `cglm` struct API in Phase 3.
- **D-02:** New camera/view/projection math should originate from `cglm`, not from fresh `src/math3d.h` calls, even if temporary `vec3_t` / `mat4_t` bridge values remain at unchanged subsystem boundaries.
- **D-03:** Phase 3 must not widen into pick-buffer projection math, `ray_from_screen(...)`, or gizmo drag math beyond the explicit bridge inputs required to keep current code paths compiling.

### Transform and ECS path
- **D-04:** `TransformComp` local/world matrix computation and the dense world-point application in `src/ecs/ecs_scene.h` should migrate through `cglm` helpers during this phase.
- **D-05:** Cached storage in `TransformComp` should remain `vec3_t` and `mat4_t` for this slice unless a narrower bridge proves impossible. The Phase 3 goal is math migration, not a broad storage-layout rewrite.
- **D-06:** ECS and render-adjacent transform math may use the `cglm` array/raw API where it maps better to contiguous float buffers and existing render consumers.

### Validation posture
- **D-07:** The existing `mdcad_math_harness`, `math-regression`, `math-bench`, and `math-validation` workflow remains the primary automated gate for this phase.
- **D-08:** Phase 3 validation should compare live migrated production helpers against frozen legacy baselines so parity checks remain meaningful after the runtime code changes.
- **D-09:** A manual macOS smoke pass is still required after automated checks to confirm camera orbit/pan/zoom feel and transformed scene output remain stable.
- **D-10:** Windows Vulkan validation remains deferred to Phase 5.

### the agent's Discretion
- Choose the exact helper names for cglm-backed camera accessors and raw-matrix bridge helpers.
- Choose whether legacy-returning helpers in `src/orbit_camera.h` remain as thin wrappers during this phase.
- Choose the exact harness case additions or rewrites needed to keep Phase 3 comparisons production-backed and deterministic.

</decisions>

<specifics>
## Specific Ideas

- Keep the public `orbit_camera_t` state model stable while migrating the underlying eye/view/projection math to `cglm`.
- Treat `src/app.c` as the one place where the new camera/view/projection chain should be assembled before any legacy bridge copies are made for unchanged consumers.
- Favor a narrow ECS helper that centralizes world-point transformation instead of leaving dozens of scattered `mat4_transform_point(...)` call sites on the legacy path.
- Preserve the existing harness-first workflow and add Phase 3-specific parity coverage instead of inventing a heavier test framework.

</specifics>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Scope, requirements, and current state
- `.planning/PROJECT.md` — milestone goals, constraints, and the harness-first validation posture established by Phase 2
- `.planning/ROADMAP.md` — official Phase 3 goal, success criteria, and the expected three-plan shape
- `.planning/REQUIREMENTS.md` — `HOT-01` and `HOT-02` definitions
- `.planning/STATE.md` — current project position and active focus
- `.planning/phases/03-macos-core-transform-migration/03-DISCUSSION-LOG.md` — fallback planning assumptions for this phase

### Prior migration decisions and validation posture
- `.planning/phases/02-direct-adoption-tooling-and-validation-harness/02-CONTEXT.md` — locked entrypoint, harness, and API-family bias decisions that Phase 3 inherits
- `.planning/phases/02-direct-adoption-tooling-and-validation-harness/02-RESEARCH.md` — direct-adoption and validation recommendations already proven in the repo
- `.planning/phases/02-direct-adoption-tooling-and-validation-harness/02-VERIFICATION.md` — confirms the existing harness workflow is real and green
- `docs/MATH_BACKEND_DECISION.md` — direct `cglm` adoption and mixed API-family posture
- `docs/MATH_CONVENTIONS.md` — shared math contract and rollout-sensitive hotspots

### Runtime hotspots for this phase
- `src/orbit_camera.h` — orbit camera eye/view math and interaction-sensitive defaults
- `src/app.c` — primary macOS render path, MVP composition, pick-buffer bridge points, and camera-driven gizmo update inputs
- `src/components/transform_comp.h` — local/world matrix composition
- `src/ecs/ecs_scene.h` — recursive transform propagation and dense world-point application for scene sync and draw inputs
- `src/gpu/geometry_batch.h` — matrix/uniform consumers that constrain how far Phase 3 can change render-facing storage
- `src/math_harness.c` — existing comparison and benchmark harness that Phase 3 must extend rather than bypass
- `src/math3d.h` — frozen legacy baseline for parity comparisons

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `src/math/cglm_entry.h` already locks the clip-depth and layout contract for migrated code.
- `src/math_harness.c` already exercises orbit camera view, view-projection behavior, screen-ray unproject, and transform composition.
- `TransformComp` already isolates local/world matrix composition in one header, making it the cleanest place to swap the underlying math engine without rewriting ECS storage.
- `src/ecs/ecs_scene.h` already centralizes most world-point application through repeated `mat4_transform_point(...)` calls, which means one helper can collapse a lot of migration churn.

### Established Patterns
- Header-only `static inline` helpers remain the dominant source pattern in `src/`.
- `orbit_camera_t` is stored by value and used directly in UI debug widgets, so Phase 3 should avoid a disruptive state-layout rewrite.
- `geom_batch_params_t`, `geom_triangle_params_t`, and several pick/gizmo paths still consume `mat4_t`, which makes bridge copies safer than a repo-wide render-struct rewrite in this phase.

### Integration Points
- `src/app.c` is the correct choke point for Phase 3 view/projection/MVP assembly.
- `src/components/transform_comp.h` is the correct choke point for local/world matrix composition.
- `src/ecs/ecs_scene.h` is the correct choke point for dense world-point application into scene/render inputs.
- `src/math_harness.c` is the correct place to freeze legacy baselines once production helpers move to `cglm`.

</code_context>

<deferred>
## Deferred Ideas

- Pick-buffer projection, unproject, and screen-ray math
- Gizmo drag and intersection math
- Windows Vulkan hardening and benchmark interpretation
- Broader storage/alignment refactors outside the Phase 3 hotspots

</deferred>

---
*Phase: 03-macos-core-transform-migration*
*Context gathered: 2026-03-24*
