# Phase 2: Direct Adoption Tooling and Validation Harness - Research

**Researched:** 2026-03-24
**Domain:** Thin `src/math/` expansion and standalone native validation tooling for staged direct `cglm` adoption
**Confidence:** HIGH

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- **D-01:** `src/math/cglm_entry.h` must remain an include-only entrypoint for convention/config includes and compile-time checks.
- **D-02:** Migration-only helper headers may live under `src/math/`, but only for comparison, validation, and benchmark plumbing.
- **D-03:** Phase 2 must not recreate a subsystem-facing compatibility facade or project-owned aliases for vendor math types.
- **D-04:** Helper headers belong beside `cglm_entry.h`, not inside it.
- **D-05:** A standalone native harness is the primary comparison and benchmark surface for this phase.
- **D-06:** App smoke hooks are secondary, not the main validation mechanism.
- **D-07:** The harness should be designed as a pilot for broader future testing, while current scope stays focused on math migration.
- **D-08:** Both hotspot workflow comparisons and primitive microbenchmarks are required in the first pass.
- **D-09:** Workflow coverage must explicitly protect camera/view-projection, pick-ray or unproject behavior, and transform composition before runtime migration widens.
- **D-10:** Primitive benches support interpretation and regression tracking; they do not replace workflow-level checks.
- **D-11:** Early migration/comparison work should bias toward the `cglm` struct API in camera, app, and gizmo-facing slices.
- **D-12:** ECS and GPU-heavy paths remain free to use the `cglm` array API where it measures better.
- **D-13:** The plan must state per-subsystem API-family bias explicitly.
- **D-14:** Phase 2 should establish a lightweight but repeatable native validation workflow around dedicated harness targets and documented commands.
- **D-15:** macOS Metal is the first execution path, but the workflow must be shaped to mirror on Windows Vulkan later.
- **D-16:** Avoid forcing full test-framework integration before the harness model proves itself.

### the agent's Discretion
- Choose exact helper header names and file layout under `src/math/`.
- Choose the exact harness target and source layout in CMake as long as invocation stays lightweight and repeatable.
- Choose the exact split between harness assertions, comparison tolerances, benchmark reporting, and smoke-hook follow-up work.
- Decide whether any app smoke hooks land in this phase or remain follow-on work, as long as the standalone native harness remains primary.

### Deferred Ideas (OUT OF SCOPE)
- Broaden the harness pattern into wider app regression coverage after the math-migration pilot proves itself.
</user_constraints>

<research_summary>
## Summary

Phase 2 should be executed as a deliberately narrow foundation-for-migration slice, not as the first broad runtime adoption wave. The best implementation shape is:

1. Keep `src/math/cglm_entry.h` include-only, but make it a real configuration choke point by encoding the vendor-side clip-depth policy there and compile-checking the expected `cglm` clip-control mode.
2. Add adjacent migration-only helper headers under `src/math/` for three concerns only: comparison tolerances/bridges, validation reporting, and benchmark timing.
3. Build a standalone native executable target, `mdcad_math_harness`, that compares old and new math behavior without linking the full app runtime.
4. Add lightweight custom targets plus Quickstart documentation so the harness becomes a repeatable native workflow on macOS and a mirrorable one on Windows Vulkan.

**Primary recommendation:** Plan the phase as three sequential waves:
- **02-01:** turn `src/math/` into a configured thin entrypoint plus migration helper surface
- **02-02:** add `mdcad_math_harness` with both workflow comparisons and primitive benches
- **02-03:** wire custom validation targets and user-facing native validation commands into the repo workflow
</research_summary>

<standard_stack>
## Standard Stack

The established libraries and tools for this phase:

### Core
| Library / Tool | Version | Purpose | Why Standard |
|----------------|---------|---------|--------------|
| `cglm` | `0.9.6` | New math backend under test | Already vendored and locked by Phase 1 |
| `src/math/cglm_entry.h` | repo-local | Single config/convention choke point | Matches the direct-adoption decision without rewrapping vendor types |
| `src/math/math_compare.h` | new repo-local header | Old/new comparison helpers | Lets struct and array `cglm` paths be compared through the same raw-float interface |
| `src/math/math_validate.h` | new repo-local header | Harness reporting and exit-code helpers | Keeps compare mode deterministic and machine-readable |
| `src/math/math_bench.h` | new repo-local header | Lightweight timing utilities | Supports primitive microbenchmarks without introducing a test framework |
| `mdcad_math_harness` | new native target | Standalone compare + bench executable | Fits the user's "standalone harness first" decision |

### Supporting
| Library / Tool | Version | Purpose | When to Use |
|----------------|---------|---------|-------------|
| `glms_*` struct API | `cglm 0.9.6` | Camera/app/gizmo-shaped comparison cases | Preferred where current code is value-struct-heavy |
| `glm_*` array API | `cglm 0.9.6` | Lower-level ECS/GPU or raw-array bench paths | Preferred where raw float arrays or throughput-oriented loops fit better |
| `timespec_get` | C11 | Cross-platform benchmark timing | Good enough for lightweight native benches without adding another dependency |
| `cmake` + `ninja` | existing repo workflow | macOS build and harness execution | Already the active local native path |
| `rg` | existing tool | Static verification for docs and plan checks | Consistent with the repo's current validation style |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Thin entrypoint + helper headers | Broad compatibility facade | Conflicts directly with the user's locked direct-adoption posture |
| Standalone native harness | In-app panel-driven checks only | Harder to automate and less repeatable for regression gating |
| Custom targets + docs | CTest or a formal test framework | Heavier process than Phase 2 needs and not required by the user decision |

**Installation / build posture:**
```bash
# Keep Phase 2 inside the current native workflow.
cmake -B build -G Ninja
ninja -C build
```
</standard_stack>

<architecture_patterns>
## Architecture Patterns

### Recommended Project Structure
```text
src/
├── math/
│   ├── cglm_entry.h
│   ├── math_compare.h
│   ├── math_validate.h
│   └── math_bench.h
├── math_harness.c
└── CMakeLists.txt

docs/
└── QUICKSTART.md
```

### Pattern 1: Include-Only Entry Point with Explicit Vendor Config
**What:** Keep `src/math/cglm_entry.h` include-only, but make it define the vendor clip-depth macro and assert the resulting `cglm` clip-control mode.
**When to use:** When the project wants a thin entrypoint that actually centralizes vendor configuration instead of merely forwarding includes.
**Example:**
```c
#ifndef CGLM_FORCE_DEPTH_ZERO_TO_ONE
#define CGLM_FORCE_DEPTH_ZERO_TO_ONE
#endif

#include "math_conventions.h"
#include <cglm/cglm.h>
#include <cglm/struct.h>

_Static_assert(CGLM_CONFIG_CLIP_CONTROL == CGLM_CLIP_CONTROL_RH_ZO,
               "mdCAD cglm entrypoint must use RH_ZO clip control");
```

### Pattern 2: Raw-Float Comparison Helpers
**What:** Comparison helpers should accept legacy `vec3_t` / `mat4_t` values and compare them against raw `float[3]` or `float[16]` candidate buffers.
**When to use:** When the migration intentionally mixes struct and array `cglm` APIs by subsystem.
**Why this fits:** A raw-float comparison surface works equally well for `vec3s.raw`, `mat4s.raw`, and array API buffers without reintroducing mdCAD-owned aliases for vendor types.
**Example:**
```c
static inline bool mdcad_compare_mat4_close(mat4_t legacy,
                                            const float candidate[16],
                                            float epsilon);
```

### Pattern 3: Behavior-First Workflow Comparisons
**What:** For projection-sensitive paths, compare user-visible or interaction-visible behavior rather than requiring raw matrix equality.
**When to use:** When the new internal convention intentionally changes clip depth from the legacy `math3d.h` baseline.
**Why this fits:** View matrices and transform composition can be compared numerically, but projection-sensitive cases should compare NDC behavior, unprojected rays, or transformed sample points instead of insisting every coefficient matches.

### Pattern 4: Lightweight Validation Targets, Not a Framework Migration
**What:** Add custom CMake targets such as `math-regression`, `math-bench`, and `math-validation` that run the standalone harness with exact flags.
**When to use:** When the repo wants repeatable native validation commands without introducing CTest or another test framework.
**Example:**
```cmake
add_custom_target(math-regression
    COMMAND $<TARGET_FILE:mdcad_math_harness> --mode compare --strict
    DEPENDS mdcad_math_harness
    USES_TERMINAL
)
```

### Anti-Patterns to Avoid
- **Re-expanding `src/math/` into a facade:** violates the direct-adoption decision and makes Phase 3 harder to interpret.
- **Comparing raw projection matrices only:** the internal clip-depth convention changed; behavior-level checks are more reliable.
- **Linking the harness against the whole app by default:** defeats the "standalone native harness" decision and bloats build/iteration time.
- **Turning Phase 2 into a test-framework migration:** custom targets and docs are enough for this slice.
</architecture_patterns>

<dont_hand_roll>
## Don't Hand-Roll

Problems that look simple but already have a better Phase-2 answer:

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Migration boundary | A new `vec3_t` / `mat4_t` facade over `cglm` | `cglm_entry.h` plus adjacent helper headers | The user explicitly rejected a broad compatibility layer |
| Old/new comparison surface | Duplicate compare code per harness case | Shared `math_compare.h` helpers | Keeps struct and array cases comparable through one interface |
| Benchmark timing | Platform-specific timer branches everywhere | `timespec_get` in one `math_bench.h` helper | Sufficient for lightweight cross-platform benches |
| Validation workflow | Full CTest adoption | Custom CMake targets + Quickstart commands | Matches the repo's lightweight native workflow |

**Key insight:** The real work in Phase 2 is not inventing new abstractions. It is creating just enough thin structure to let later migration slices be measured and rolled forward safely.
</dont_hand_roll>

<common_pitfalls>
## Common Pitfalls

### Pitfall 1: `cglm_entry.h` Stays "Thin" but Not Useful
**What goes wrong:** The entrypoint still only proves vendoring and does not actually centralize vendor configuration.
**Why it happens:** Teams stop at compile anchors and never move vendor macros or compile-time policy checks into the choke point.
**How to avoid:** Make `cglm_entry.h` own the `CGLM_FORCE_DEPTH_ZERO_TO_ONE` define and assert the resulting `CGLM_CLIP_CONTROL_RH_ZO` mode.
**Warning signs:** Compare cases silently use the legacy clip convention or rely on file-local `cglm` config.

### Pitfall 2: Comparison Helpers Accidentally Become a Facade
**What goes wrong:** Helper headers start exporting project-owned aliases or long-lived wrappers for common math operations.
**Why it happens:** Comparison and bench code wants ergonomic helpers, so teams drift back into an abstraction layer.
**How to avoid:** Limit helpers to compare, validate, and bench utilities only. No aliasing `vec3s`, `mat4s`, `versor`, or similar vendor types.
**Warning signs:** New headers introduce `typedef vec3s ...` or replace ordinary `cglm` calls in subsystem code.

### Pitfall 3: Harness Reuses Heavy Runtime Headers for Pure-Math Cases
**What goes wrong:** The harness drags in Sokol, GPU, or UI dependencies for cases that only need math inputs and outputs.
**Why it happens:** It is tempting to reuse the exact app path even when a pure-math fixture would be simpler.
**How to avoid:** Keep Phase 2 compare cases in pure-math hotspots such as `orbit_camera.h`, `math3d.h`, and standalone transform/ray helpers. Use behavior fixtures rather than full runtime wiring.
**Warning signs:** `math_harness.c` starts linking `libsokol`, `cimgui`, or ECS scene state just to run a compare case.

### Pitfall 4: Primitive Benches Replace Workflow Checks
**What goes wrong:** The harness prints speed numbers, but nothing proves camera feel, ray behavior, or transform outputs are still trustworthy.
**Why it happens:** Benches are easy to add and look quantitative.
**How to avoid:** Require compare suites and benches together, with compare mode failing the harness when behavior drifts outside tolerance.
**Warning signs:** `--mode bench` exists, but `--mode compare` cannot fail the build.

### Pitfall 5: Validation Workflow Exists Only as Local Tribal Knowledge
**What goes wrong:** One person knows how to run the harness, but the commands are not captured in the repo.
**Why it happens:** The harness target is added, but Quickstart or docs are never updated.
**How to avoid:** Add custom targets and explicit Quickstart commands for both macOS and Windows Vulkan.
**Warning signs:** Phase 2 completes, but no checked-in doc tells another agent which command to run.
</common_pitfalls>

<code_examples>
## Code Examples

Verified patterns from the codebase and vendored `cglm` headers:

### Struct-API camera/view functions already available in vendored `cglm`
```c
mat4s view = glms_lookat_rh_zo(eye, center, up);
mat4s proj = glms_perspective_rh_zo(fovy, aspect, nearZ, farZ);
mat4s vp   = glms_mat4_mul(proj, view);
```

### `cglm` project/unproject support for behavior-level ray checks
```c
vec3s near_world = glms_unprojecti_zo(near_ndc, inv_vp, viewport);
vec3s far_world  = glms_unprojecti_zo(far_ndc,  inv_vp, viewport);
```

### Existing legacy hotspots that are good harness inputs
```c
mat4_t view = orbit_camera_get_view_matrix(&state.camera);
mat4_t proj = mat4_perspective(0.785398f, aspect, 0.1f, 100.0f);
mat4_t vp_mat = mat4_mul(proj, view);
ray_t mouse_ray = ray_from_screen(ndc_x, ndc_y, inv_vp);
```
</code_examples>

<sota_updates>
## State of the Art (2024-2025)

What matters for this phase:

| Old Approach | Current Approach | Impact on Phase 2 |
|--------------|------------------|-------------------|
| "Header-only means no config choke point" | Thin entrypoints often centralize config macros and compile-time policy assertions | `cglm_entry.h` should become a real config boundary, not just a compile anchor |
| Wrapper-first migration boundaries | Direct vendor adoption with narrow local support headers | Phase 2 should add helper headers only for migration plumbing, not subsystem APIs |
| Framework-heavy validation | Lightweight custom targets and dedicated harness executables | The repo can get repeatable native validation without moving to CTest immediately |

**New tools/patterns to consider:**
- Standalone migration harness executables with CLI modes for compare and bench
- Raw-float compare helpers to bridge struct and array `cglm` styles
- CMake custom targets that run harness commands as part of native workflows

**Deprecated/outdated for this project:**
- Broad compatibility facades
- Declaring migration success from microbenchmarks alone
- Leaving validation commands out of checked-in docs
</sota_updates>

## Validation Architecture

- **Primary build gate:** `cmake -B build -G Ninja && ninja -C build mdcad_math_harness`
- **Compare gate:** `cmake -B build -G Ninja && ninja -C build math-regression`
- **Benchmark gate:** `cmake -B build -G Ninja && ninja -C build math-bench`
- **Full native gate:** `cmake -B build -G Ninja && ninja -C build math-validation`
- **Manual smoke follow-up:** `./build/bin/mdCAD` after a green harness run, only as a secondary confidence check
- **Feedback cadence:** quick build after each task; full native gate after each completed wave
- **Manual verification expectation:** optional app launch only; the primary acceptance path is harness-driven

<open_questions>
## Open Questions

1. **How much shared case code should move out of `math_harness.c` later**
   - What we know: Phase 2 needs one standalone harness target now.
   - What's unclear: whether later phases should split reusable compare-case builders into separate source files.
   - Recommendation: keep Phase 2 simple with one harness source file, then refactor only if Phase 3 makes the file unwieldy.

2. **When app smoke hooks become worth the maintenance cost**
   - What we know: the user wants app smoke hooks to stay secondary.
   - What's unclear: whether Phase 2 should add any actual app hook, or only docs and harness targets.
   - Recommendation: make the harness and custom targets first-class now; revisit app smoke hooks only if Phase 3 needs them for navigation/render parity.
</open_questions>

<sources>
## Sources

### Primary (HIGH confidence)
- `.planning/phases/02-direct-adoption-tooling-and-validation-harness/02-CONTEXT.md`
- `.planning/PROJECT.md`
- `.planning/REQUIREMENTS.md`
- `.planning/STATE.md`
- `.planning/research/SUMMARY.md`
- `.planning/research/ARCHITECTURE.md`
- `.planning/research/PITFALLS.md`
- `.planning/research/STACK.md`
- `docs/MATH_BACKEND_DECISION.md`
- `docs/MATH_CONVENTIONS.md`
- `CMakeLists.txt`
- `src/CMakeLists.txt`
- `src/math/cglm_entry.h`
- `src/math/math_conventions.h`
- `src/math3d.h`
- `src/orbit_camera.h`
- `src/app.c`
- `src/ecs/ecs_scene.h`
- `src/gpu/geometry_batch.h`
- `src/gpu/pick_buffer.h`
- `src/gizmo/gizmo.h`
- `docs/QUICKSTART.md`
- `vendors/cglm/include/cglm/common.h`
- `vendors/cglm/include/cglm/cam.h`
- `vendors/cglm/include/cglm/project.h`
- `vendors/cglm/include/cglm/struct/cam.h`
- `vendors/cglm/include/cglm/struct/mat4.h`
- `vendors/cglm/include/cglm/struct/clipspace/project_zo.h`
- `vendors/cglm/include/cglm/struct/affine.h`

### Local inference (MEDIUM confidence)
- Lightweight native validation should stay in CMake custom targets and docs instead of a framework migration
- Pure-math harness cases are a better Phase-2 fit than full runtime reuse for compare-mode baselines
</sources>

---
*Research completed: 2026-03-24*
*Ready for planning: yes*
