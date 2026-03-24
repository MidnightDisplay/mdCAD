# Phase 2: Direct Adoption Tooling and Validation Harness - Context

**Gathered:** 2026-03-24
**Status:** Ready for planning

<domain>
## Phase Boundary

Expand mdCAD's thin project-owned `src/math/` entrypoint just enough to support direct `cglm` rollout, and add the standalone comparison, benchmark, and native validation tooling needed to compare old and new math behavior safely before hotspot migration begins. This phase prepares the migration slices; it does not broadly migrate orbit camera, ECS, picking, or gizmo math yet.

</domain>

<decisions>
## Implementation Decisions

### Entrypoint scope
- **D-01:** `src/math/cglm_entry.h` remains an include-only entrypoint for convention/config includes and compile-time layout checks.
- **D-02:** Phase 2 may add a small set of migration-only helper headers under `src/math/` for validation, comparison, and benchmark plumbing.
- **D-03:** Those helper headers must not become a subsystem-facing compatibility facade and must not reintroduce project-owned aliases for vendor math types.
- **D-04:** Migration helpers should live in adjacent headers such as `math_compare.h`, `math_bench.h`, or `math_validate.h`, not inside `cglm_entry.h`.

### Comparison harness shape
- **D-05:** Phase 2 should create a standalone native harness as the primary old/new comparison and benchmark surface.
- **D-06:** In-app smoke hooks are secondary and may complement the harness later, but they are not the main validation surface for this phase.
- **D-07:** The harness should be structured as a pilot foundation that can later expand into broader app coverage, while keeping current scope centered on math-migration validation.

### Coverage priorities
- **D-08:** Phase 2 should include both hotspot workflow comparisons and primitive microbenchmarks from the start.
- **D-09:** Workflow comparisons should explicitly cover rollout-sensitive math around camera/view-projection, pick-ray or unproject behavior, and transform composition before Phase 3 widens runtime adoption.
- **D-10:** Primitive benches exist to support interpretation and regression tracking, not as a substitute for workflow-level checks.

### API-family bias by subsystem
- **D-11:** Early migration and comparison work should bias toward the `cglm` struct API in camera, app, and gizmo-facing slices where current code is strongly value-struct shaped.
- **D-12:** ECS and GPU-heavy paths such as transforms, batching, and picking remain free to adopt the `cglm` array API where it measures better or maps more cleanly to the code.
- **D-13:** Phase planning must state the expected `cglm` API-family bias per subsystem instead of pretending the migration uses one universal style.

### Native validation workflow
- **D-14:** Phase 2 should establish a lightweight but repeatable native validation workflow around dedicated harness targets and documented commands.
- **D-15:** macOS Metal is the first-class execution path for that workflow in this phase, but the workflow shape must be designed to mirror on Windows Vulkan later rather than being macOS-only by design.
- **D-16:** Phase 2 should avoid forcing full test-framework integration if that adds unnecessary process weight before the harness model proves itself.

### the agent's Discretion
- Choose exact helper header names and file layout under `src/math/`.
- Choose the exact harness target and source layout in CMake as long as invocation stays lightweight and repeatable.
- Choose the exact split between harness assertions, comparison tolerances, benchmark reporting, and smoke-hook follow-up work.
- Decide whether any app smoke hooks land in this phase or remain planned follow-on work, as long as the standalone native harness remains primary.

</decisions>

<specifics>
## Specific Ideas

- Keep `src/math/cglm_entry.h` thin and explicit rather than letting Phase 2 drift back into a wrapper-first migration.
- Treat the standalone harness as the primary comparison surface for direct `cglm` adoption.
- Include both workflow-level comparisons and primitive microbenchmarks in the first pass instead of deferring either category.
- Bias early migration and comparison work toward struct-style `cglm` in camera, app, and gizmo code, while allowing ECS and GPU paths to choose array-style usage where it pays off.
- Design the harness as a pilot for a broader future testing foundation, but keep current phase scope centered on math-migration coverage.

</specifics>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Scope and locked migration posture
- `.planning/PROJECT.md` — current milestone goals, rollout priorities, and non-negotiable constraints for the math migration
- `.planning/REQUIREMENTS.md` — Phase 2 requirement coverage for `FOUND-02` and `PERF-01`
- `.planning/ROADMAP.md` — official Phase 2 goal, success criteria, and the three expected plan slots
- `.planning/STATE.md` — current focus and the unresolved Phase 2 concerns around harness choice and API-family bias
- `docs/MATH_BACKEND_DECISION.md` — locked direct-`cglm` adoption posture and mixed API-family allowance
- `docs/MATH_CONVENTIONS.md` — global math contract and the rollout-sensitive hotspots that validation must protect

### Research and risk framing
- `.planning/research/SUMMARY.md` — broader migration rationale, especially parity-first rollout and harness expectations
- `.planning/research/ARCHITECTURE.md` — background on migration boundaries and validation harness structure; use as research context, not as a mandate for a broad wrapper facade
- `.planning/research/PITFALLS.md` — risks around convention drift, alignment surprises, big-bang churn, and paper-only performance wins
- `.planning/research/STACK.md` — `cglm` API-family options and harness-related tooling guidance that informs subsystem bias decisions

### Existing math surface and hotspot code
- `src/math/cglm_entry.h` — current thin entrypoint that Phase 2 expands without turning into a facade
- `src/math/math_conventions.h` — compiled math contract for layout, handedness, clip depth, alignment policy, and rollout behavior
- `src/math3d.h` — legacy baseline math surface that the comparison harness must measure against
- `src/orbit_camera.h` — early struct-shaped hotspot and comparison target
- `src/app.c` — current view/projection, inverse, ray, and startup wiring for the native app
- `src/ecs/ecs_scene.h` — transform-heavy runtime code and likely early array-API pressure point
- `src/gpu/geometry_batch.h` — render-path matrix and batch data context
- `src/gpu/pick_buffer.h` — pick-MVP and readback-related math hotspot
- `src/gizmo/gizmo.h` — interaction-sensitive drag and ray math hotspot

### Build and validation wiring
- `src/CMakeLists.txt` — existing native target wiring and the likely integration point for harness targets
- `scripts/test-imgui.mjs` — example of the repo's current lightweight project-local validation style; useful as a pattern reference only, not as the primary native harness

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `src/math/cglm_entry.h`: existing thin entrypoint and the obvious place to define what remains include-only versus what must move into adjacent helper headers.
- `src/math/math_conventions.h`: already-centralized contract for conventions and alignment policy that validation code can treat as authoritative.
- `src/math3d.h`: current baseline implementation for old/new comparisons and benchmark inputs.
- `src/orbit_camera.h`, `src/app.c`, and `src/gizmo/gizmo.h`: strongly value-struct-shaped call sites that make good early struct-API comparison targets.
- `src/ecs/ecs_scene.h` and `src/gpu/pick_buffer.h`: dense transform and rendering math paths that are better candidates for performance-driven array-API exploration.
- `src/CMakeLists.txt`: existing lightweight native build entry where one or more harness targets can be added without introducing a heavyweight test framework.

### Established Patterns
- Header-only `static inline` modules dominate `src/`, so any new migration helpers should stay light and composable unless measurement proves a heavier structure is needed.
- The native build is intentionally minimal and centralized, which favors dedicated harness targets with explicit commands over a large new testing framework.
- Current validation culture is compile-driven and script-driven rather than framework-heavy, so a lightweight harness-first approach matches the repo's existing operating style.
- Hot runtime math still relies heavily on value structs and direct field access, which is why subsystem-level API-family bias matters more than a one-style-fits-all rule.

### Integration Points
- New migration helper headers belong under `src/math/` beside `cglm_entry.h` and `math_conventions.h`.
- Standalone native harness targets should wire in through `src/CMakeLists.txt` and use the existing macOS + Ninja workflow first.
- The harness needs direct access to legacy `math3d.h` behavior and the new `src/math/` migration helpers so old/new results can be compared without destabilizing the app target.
- Future Phase 3 planning should treat camera/app/gizmo and ECS/GPU paths differently when deciding where struct or array `cglm` APIs are expected.

</code_context>

<deferred>
## Deferred Ideas

- Broaden the standalone harness pattern into wider app regression coverage after the math-migration pilot proves itself. That expansion is intentionally out of scope for Phase 2.

</deferred>

---
*Phase: 02-direct-adoption-tooling-and-validation-harness*
*Context gathered: 2026-03-24*
