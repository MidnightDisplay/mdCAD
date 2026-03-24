# Phase 3: macOS Core Transform Migration - Research

**Researched:** 2026-03-24
**Domain:** Direct `cglm` migration of orbit camera, app-side viewport matrices, and ECS transform composition on the primary macOS workflow
**Confidence:** HIGH

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- **D-01:** `src/orbit_camera.h` and app-side view/projection work should bias toward the `cglm` struct API.
- **D-02:** New camera/view/projection math must originate from `cglm`, not from new `src/math3d.h` usage.
- **D-03:** Pick-buffer projection math, `ray_from_screen(...)`, and gizmo drag math stay out of scope except for explicit bridge inputs.
- **D-04:** `TransformComp` and `src/ecs/ecs_scene.h` should migrate their core transform math in Phase 3.
- **D-05:** Cached `vec3_t` / `mat4_t` storage should remain stable during this slice unless a narrow bridge proves impossible.
- **D-06:** ECS and render-adjacent transform code may prefer the `cglm` array/raw API where it fits contiguous float buffers better.
- **D-07:** The existing harness-first validation workflow remains the primary automated gate.
- **D-08:** Automated parity checks must compare live migrated production helpers against frozen legacy baselines.
- **D-09:** A manual macOS smoke pass is required after automated checks.

### the agent's Discretion
- Choose exact helper names for camera and raw-matrix bridge functions.
- Decide whether legacy-returning orbit-camera wrappers remain temporarily.
- Choose the exact harness case additions or rewrites required to protect production parity after migration.

### Deferred Ideas (OUT OF SCOPE)
- Pick/unproject/ray and gizmo drag migration
- Windows Vulkan hardening
- Broad data-layout rewrites outside the active hotspots

</user_constraints>

<research_summary>
## Summary

Phase 3 should be split into three deliberate slices:

1. Migrate orbit camera and app-side view/projection/MVP construction to the `cglm` struct API while keeping bridge copies explicit at unchanged subsystem boundaries.
2. Migrate `TransformComp` composition and ECS world-point application to `cglm` raw/array helpers while preserving cached `mat4_t` storage for existing render, gizmo, and pick consumers.
3. Tighten the harness and Quickstart/manual smoke workflow so the migrated production path is what gets validated on macOS.

**Primary recommendation:** keep the migration compute-centric, not storage-centric. The biggest Phase 3 win is replacing the math implementation under the current camera and transform hot paths without simultaneously refactoring every downstream struct that still embeds `mat4_t`.

</research_summary>

<standard_stack>
## Standard Stack

### Core
| Library / Tool | Version | Purpose | Why Standard |
|----------------|---------|---------|--------------|
| `cglm` struct API (`glms_*`) | `0.9.6` | Orbit camera and app-facing view/projection/MVP construction | Matches the value-style call shape in `src/orbit_camera.h` and `src/app.c` |
| `cglm` array API (`glm_*`) | `0.9.6` | Transform composition and dense world-point application | Maps cleanly to raw float buffers and current `mat4_t` storage |
| `mdcad_math_harness` | repo-local | Automated parity and bench gate | Already green from Phase 2 and easy to extend |
| `math-validation` target | repo-local | Full native validation entry point | Existing named target that execution can reuse |

### Supporting
| Library / Tool | Version | Purpose | When to Use |
|----------------|---------|---------|-------------|
| `src/math/cglm_entry.h` | repo-local | Shared migration contract and vendor config | Include before all Phase 3 `cglm` use |
| `memcpy` | C stdlib | Bridge between `mat4_t` storage and `cglm` temporary matrices | Use where storage stays legacy-shaped but compute is migrated |
| `docs/QUICKSTART.md` | repo-local | User-facing macOS workflow and smoke instructions | Update when validation commands or manual checks become phase-critical |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Struct API in camera/app | Array API everywhere | Worse fit for the current value-returning orbit-camera code and app-side matrix composition |
| Stable `mat4_t` storage with raw bridges | Full `mat4s` storage rewrite in `TransformComp` and render structs | Larger blast radius into ECS, gizmo, pick, and shader-uniform code than Phase 3 needs |
| Harness updates tied to production helpers | Static grep-only verification | Too weak to prove visible parity on camera and transform paths |

</standard_stack>

<architecture_patterns>
## Architecture Patterns

### Pattern 1: Struct-First Camera Helpers with Explicit Legacy Bridges
**What:** add `cglm`-backed helpers to `src/orbit_camera.h` for eye/view matrix construction, then keep any legacy-returning helpers as thin wrappers while the rest of the repo catches up.
**Why it fits:** the current orbit camera is value-oriented, and `glms_lookat_rh_zo(...)` already matches the proven Phase 2 harness path.

### Pattern 2: Raw-Matrix Compute with Stable Cached Storage
**What:** compute local/world matrices through temporary `cglm` `mat4` buffers, then copy the results into existing `mat4_t` fields.
**Why it fits:** `TransformComp`, gizmo, and several render/pick consumers still read `mat4_t` directly. Phase 3 can migrate the compute path without a broad ABI/storage rewrite.

### Pattern 3: Central World-Point Helper in `ecs_scene.h`
**What:** replace repeated direct `mat4_transform_point(...)` usage with one ECS helper that applies a `cglm`-backed world matrix to a local point.
**Why it fits:** `src/ecs/ecs_scene.h` has many repeated world-point transforms across draw, light, and pick population code. One helper collapses that drift and makes validation more tractable.

### Pattern 4: Production-vs-Legacy Harness Cases
**What:** once production code migrates, freeze the old formulas inside `src/math_harness.c` and compare the live production helpers against those baselines.
**Why it fits:** otherwise the harness stops being meaningful and starts comparing migrated code against itself.

</architecture_patterns>

<dont_hand_roll>
## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Camera view/projection migration | A custom mdCAD camera matrix wrapper layer | `glms_lookat_rh_zo(...)`, `glms_perspective_rh_zo(...)`, and thin app/orbit helpers | Phase 1 and 2 already locked direct `cglm` adoption |
| ECS transform migration | A repo-wide render-struct rewrite to `mat4s` | Temporary raw-matrix bridges plus stable `mat4_t` storage | Smaller blast radius, safer Phase 3 rollout |
| Phase 3 parity checks | Reusing migrated production functions on both sides of the compare | Frozen legacy helpers inside `src/math_harness.c` | Keeps comparisons honest after the migration lands |
| macOS validation | New framework-level test integration | Existing `math-validation` target plus manual smoke checklist | Matches the repo's lightweight validation posture |

</dont_hand_roll>

<common_pitfalls>
## Common Pitfalls

### Pitfall 1: Camera Migration Leaves a Hidden Legacy Projection Path
**What goes wrong:** `src/orbit_camera.h` migrates, but `src/app.c` still constructs projection or MVP data with `mat4_perspective(...)` and `mat4_mul(...)`.
**How to avoid:** migrate the app-side view/projection/MVP assembly in the same slice and make the legacy bridge copies explicit.

### Pitfall 2: Harness Parity Becomes Meaningless
**What goes wrong:** compare cases keep calling the same migrated helper on both sides after Phase 3 changes land.
**How to avoid:** preserve frozen legacy formulas inside `src/math_harness.c` and compare those baselines against the live production helpers.

### Pitfall 3: `TransformComp` Storage Refactor Widens the Blast Radius
**What goes wrong:** changing cached matrix field types from `mat4_t` to `mat4s` drags Phase 3 into gizmo, pick, uniform, and ECS-storage cleanup that belongs later.
**How to avoid:** keep `mat4_t` storage stable and migrate the compute path with temporary `cglm` buffers plus explicit copies.

### Pitfall 4: ECS World-Point Migration Stays Fragmented
**What goes wrong:** some `ecs_scene.h` paths move to `cglm`, but others keep `mat4_transform_point(...)`, creating mixed semantics and review noise.
**How to avoid:** add one shared helper for world-point application and route the repeated call sites through it.

### Pitfall 5: Manual Smoke Is Too Vague
**What goes wrong:** automated checks pass, but nobody has a concrete macOS checklist for validating visible orbit/pan/zoom and scene stability.
**How to avoid:** document a short, explicit macOS parity smoke workflow in `docs/QUICKSTART.md`.

</common_pitfalls>

## Validation Architecture

Phase 3 should keep using the existing lightweight native workflow:

- **Quick command:** `cmake -B build -G Ninja && ninja -C build mdcad_math_harness && ./build/bin/mdcad_math_harness --mode compare --strict`
- **Full command:** `cmake -B build -G Ninja && ninja -C build math-validation`
- **Manual follow-up:** `./build/bin/mdCAD`

Recommended automated parity coverage after Phase 3 execution:

- `orbit-camera-view` compares frozen legacy orbit camera view math against the live production helper
- `view-projection-roundtrip` compares the live production camera/projection chain against frozen legacy NDC behavior
- `transform-compose` compares frozen legacy transform composition against the migrated production transform path
- `hierarchy-world-transform` compares parent-child world transforms and representative world-point application against a frozen legacy baseline

Sampling guidance:

- Run the quick command after each task commit
- Run the full command after each plan wave
- Run the manual follow-up after the final wave on macOS

## Code Examples

### Camera/View Construction
```c
mat4s view = orbit_camera_get_view_matrix_cglm(&state.camera);
mat4s proj = glms_perspective_rh_zo(0.785398f, aspect_ratio, 0.1f, 100.0f);
mat4s vp   = glms_mat4_mul(proj, view);
```

### Stable Storage with Raw Compute
```c
mat4 translation;
mat4 rotate_x;
mat4 rotate_y;
mat4 rotate_z;
mat4 scale;
mat4 result;

glm_translate_make(translation, position_raw);
glm_rotate_x(GLM_MAT4_IDENTITY, rot_x, rotate_x);
glm_rotate_y(GLM_MAT4_IDENTITY, rot_y, rotate_y);
glm_rotate_z(GLM_MAT4_IDENTITY, rot_z, rotate_z);
glm_scale_make(scale, scale_raw);
glm_mat4_mul(translation, rotation, result);
```

### ECS World-Point Application
```c
vec3 world;
glm_mat4_mulv3(world_matrix_raw, local_point_raw, 1.0f, world);
```

---
*Research completed: 2026-03-24*
*Ready for planning: yes*
