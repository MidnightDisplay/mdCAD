# Feature Research

**Domain:** Brownfield migration from custom C graphics math to a mature MIT C math library for mdCAD
**Researched:** 2026-03-24
**Confidence:** HIGH

## Feature Landscape

### Table Stakes (Users Expect These)

| Feature | Why Expected | Complexity | Notes |
|---------|--------------|------------|-------|
| Behavior parity for existing vector, matrix, projection, inverse, and ray operations | A math migration that changes viewport, gizmo, or picking behavior is a regression, not a feature | HIGH | Must cover camera, ECS transforms, pick buffer math, gizmo drag math, and render matrices |
| Explicit control of handedness and clipspace conventions | Cross-platform graphics math breaks when conventions are implicit | HIGH | The replacement must preserve mdCAD’s current convention choices and isolate any future Vulkan/Web differences |
| No-heap, POD-friendly math types in hot paths | Runtime-critical math should not add allocations or hidden ownership | MEDIUM | This matters for camera, rendering, picking, and tight geometry loops |
| SIMD-aware native paths with safe fallback behavior | The migration is partly justified by native performance upside | MEDIUM | macOS Apple Silicon and Windows x86-64/arm64 are the first-class targets; web fallback remains important later |
| Staged migration support | Brownfield adoption must be testable in slices | HIGH | A compatibility header or adapter layer is part of the feature set, not just an implementation detail |

### Differentiators (Competitive Advantage)

| Feature | Value Proposition | Complexity | Notes |
|---------|-------------------|------------|-------|
| Built-in quaternions, decomposition, and broader transform helpers | Reduces future engine-side math code and unlocks cleaner rotation/orientation work | MEDIUM | Good expansion target once parity is secure |
| Built-in frustum, AABB, project/unproject, and ray helpers | Directly benefits mdCAD’s pick/cull/edit flows and cuts custom helper maintenance | MEDIUM | Maps well to current `math3d.h` use plus future scene growth |
| Apple `simd` interop and portable clipspace support | Improves native Metal friendliness without platform lock-in | MEDIUM | Particularly useful for Apple-native work even though iOS parity is deferred |
| Benchmark-driven migration gates | Converts “feels faster” into measured rollout decisions | LOW | Strong differentiator for a safe infrastructure phase |

### Anti-Features (Commonly Requested, Often Problematic)

| Feature | Why Requested | Why Problematic | Alternative |
|---------|---------------|-----------------|-------------|
| Repo-wide big-bang replacement | Feels faster and “cleaner” on paper | Hard to localize regressions across camera, ECS, rendering, gizmo, importers, and serializer code | Staged migration with explicit subsystem gates |
| Mixing multiple math conventions during rollout | Seems flexible while comparing old and new behavior | Creates hard-to-debug transform and projection bugs | Freeze conventions first, then migrate behind one adapter |
| Expanding every possible math feature in phase 1 | Maximizes “value” from the migration | Increases risk before parity and perf are proven | Lock parity and hotspot wins first, then expand deliberately |

## Feature Dependencies

```text
Compatibility facade
    └──requires──> library selection + convention audit
                           └──requires──> benchmark/regression harness

Hot-path migration ──requires──> compatibility facade

API expansion ──enhances──> hot-path migration

Convention drift ──conflicts──> reliable staged rollout
```

### Dependency Notes

- **Compatibility facade requires library selection + convention audit:** The wrapper only works if mdCAD decides on layout, handedness, clipspace, and naming rules up front
- **Hot-path migration requires compatibility facade:** The safe way to migrate camera, transforms, picking, gizmo, and rendering math is behind a stable local boundary
- **API expansion enhances hot-path migration:** Quaternions and richer helpers are valuable once the core adoption path is stable
- **Convention drift conflicts with reliable staged rollout:** Mixing projection or layout conventions across phases makes regressions difficult to isolate

## MVP Definition

### Launch With (v1)

- [ ] Choose and vendor the replacement library with documented adoption constraints
- [ ] Build a compatibility layer that supports staged replacement of `src/math3d.h`
- [ ] Migrate the runtime-critical math paths first: camera, transforms, rendering matrices, picking, and gizmo math
- [ ] Add regression and performance checks for macOS Metal and Windows Vulkan

### Add After Validation (v1.x)

- [ ] Expand the wrapper or direct usage surface to include quaternions, decomposition helpers, and broader geometry utility coverage
- [ ] Migrate less performance-sensitive areas such as serializer/import helper code once native hotspots are proven stable

### Future Consideration (v2+)

- [ ] Revisit iOS and web parity once native rollout is stable
- [ ] Evaluate whether some wrapper layers can be removed after the migration is fully validated

## Feature Prioritization Matrix

| Feature | User Value | Implementation Cost | Priority |
|---------|------------|---------------------|----------|
| Behavior parity in current native workflows | HIGH | HIGH | P1 |
| Performance non-regression / gains in native hotspots | HIGH | MEDIUM | P1 |
| Staged compatibility facade | HIGH | MEDIUM | P1 |
| Expanded transform / quaternion / projection helper surface | MEDIUM | MEDIUM | P2 |
| Broader migration into importers and serializer utilities | MEDIUM | MEDIUM | P2 |
| iOS and web parity during the first milestone | LOW | HIGH | P3 |

**Priority key:**
- P1: Must have for launch
- P2: Should have, add when possible
- P3: Nice to have, future consideration

## Competitor Feature Analysis

| Feature | cglm | HandmadeMath | Our Approach |
|---------|------|--------------|--------------|
| Licensing fit | MIT | CC0/public domain | Prefer MIT to match the stated adoption requirement |
| Integration style | Header-only or linked; array, struct, call, SIMD APIs | Single-header inline API | Start simple like a header-only library, but keep the option to link later if data shows value |
| Graphics convention coverage | Explicit clipspace, handedness, frustum, project/unproject, Apple `simd` helpers | Good graphics primitives and convention variants | Use a library with richer graphics-specific helpers so mdCAD can retire more local code over time |
| SIMD / platform breadth | SSE, AVX, NEON, WASM documented | Less explicit SIMD positioning in the official repo | Choose the library with clearer portable SIMD intent for native performance goals |

## Sources

- Official candidate library repos and docs:
  - https://github.com/recp/cglm
  - https://cglm.readthedocs.io/en/latest/features.html
  - https://github.com/HandmadeMath/HandmadeMath
  - https://github.com/Kazade/kazmath
- Local mdCAD context:
  - `.planning/PROJECT.md`
  - `src/math3d.h`
  - `rg` usage scan across `src/`

---
*Feature research for: Brownfield migration from custom C graphics math to a mature MIT C math library for mdCAD*
*Researched: 2026-03-24*
