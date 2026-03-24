# Pitfalls Research

**Domain:** Brownfield migration from custom C graphics math to a mature MIT C math library for mdCAD
**Researched:** 2026-03-24
**Confidence:** HIGH

## Critical Pitfalls

### Pitfall 1: Convention Drift

**What goes wrong:**
Viewport matrices, gizmo drag math, or picking behavior subtly change because handedness, clipspace, or matrix layout assumptions moved during migration.

**Why it happens:**
Graphics math libraries often support multiple valid conventions, and brownfield codebases already have implicit choices spread across subsystems.

**How to avoid:**
Write down mdCAD’s required conventions once, enforce them in a local compatibility layer, and add regression scenes for camera, picking, and gizmo workflows before expanding rollout.

**Warning signs:**
Selection rays miss, frustum culling changes, orbit camera feels mirrored, or projection math differs between Metal and Vulkan builds.

**Phase to address:**
Phase 1: library selection, convention audit, and compatibility boundary

---

### Pitfall 2: SIMD Alignment Surprises

**What goes wrong:**
Code compiles, but stack/heap layout assumptions break performance or correctness because the adopted math types require stronger alignment than mdCAD currently guarantees everywhere.

**Why it happens:**
SIMD-friendly libraries often optimize around aligned vector/matrix storage, while legacy code may pass structs by value or embed them in other types without thinking about alignment.

**How to avoid:**
Treat alignment as an explicit adoption decision, benchmark both aligned and unaligned integration strategies, and contain raw vendor types behind project-owned wrappers at first.

**Warning signs:**
Platform-only crashes, compiler warnings around alignment, unexpected copies, or performance gains disappearing once integrated into real code.

**Phase to address:**
Phase 1 and Phase 2

---

### Pitfall 3: Big-Bang Migration Churn

**What goes wrong:**
Too many files change at once, making regressions in camera, ECS transforms, rendering, importers, and UI impossible to isolate quickly.

**Why it happens:**
The existing `math3d.h` surface is used widely enough that a full rewrite looks attractive until testing starts.

**How to avoid:**
Stage the rollout by subsystem, starting with the highest-value runtime hotspots and keeping long-tail helpers for later phases.

**Warning signs:**
Massive rename diffs, unclear rollback path, and review conversations that mix API debates with behavior regressions.

**Phase to address:**
Phase 1 and Phase 2

---

### Pitfall 4: Performance Wins on Paper Only

**What goes wrong:**
The adopted library looks faster in isolated math functions but the app becomes flat or slower because wrappers, conversions, or altered data flow add hidden cost.

**Why it happens:**
Microbenchmarks do not capture real camera/render/picking/gizmo workloads.

**How to avoid:**
Benchmark both isolated primitives and representative in-app workflows, then gate expansion on real measurements from the supported native targets.

**Warning signs:**
Frame times do not improve, picking/gizmo interaction gets less responsive, or compile time and binary growth outweigh runtime wins.

**Phase to address:**
Phase 2 and Phase 3

---

### Pitfall 5: Expanding Scope Before Parity

**What goes wrong:**
Quaternion/decomposition/helper expansion starts before the base migration is trusted, making regressions harder to attribute and slowing validation.

**Why it happens:**
Infrastructure migrations feel more rewarding when they add visible capabilities immediately.

**How to avoid:**
Keep the rollout order strict: parity first, then performance proof, then API cleanup, then feature expansion.

**Warning signs:**
New math helpers appear in the same diff as unresolved compatibility bugs, or phase goals become hard to explain in one sentence.

**Phase to address:**
Phase 3 and Phase 4

## Technical Debt Patterns

| Shortcut | Immediate Benefit | Long-term Cost | When Acceptable |
|----------|-------------------|----------------|-----------------|
| Directly aliasing vendor headers throughout the repo | Fast initial progress | Hard rollback, hard convention control | Never for this migration |
| Keeping old and new math conventions alive indefinitely | Easier short-term patching | Permanent ambiguity and bug surface | Only temporarily inside the compatibility layer |
| Skipping explicit benchmarks | Less setup work | “Optimized” migration with no proof | Never, given performance is a stated goal |

## Integration Gotchas

| Integration | Common Mistake | Correct Approach |
|-------------|----------------|------------------|
| cglm header-only adoption | Treating vendored headers as zero-risk and skipping wrapper design | Vendor first, but still route usage through project-owned math headers |
| cglm alignment options | Mixing aligned and unaligned assumptions across modules | Pick one strategy per phase and enforce it consistently in build settings and wrappers |
| Struct vs array API | Switching styles ad hoc by file | Choose one primary migration style, allow exceptions only when justified by measurements or ergonomics |

## Performance Traps

| Trap | Symptoms | Prevention | When It Breaks |
|------|----------|------------|----------------|
| Wrapper conversion churn | No frame-time gain despite “faster” primitives | Keep wrappers thin and benchmark hot paths after each slice | As soon as the wrapper starts copying more than it forwards |
| Unmeasured unaligned fallback use | Safe behavior but lower-than-expected native gains | Benchmark aligned vs unaligned strategies on macOS and Windows | Early, especially in transform-heavy loops |
| Over-inlining everything | Build times or binary size grow without proportional runtime gain | Keep call API as a later measured option, not a first assumption | Mid-migration as coverage expands |

## Security Mistakes

| Mistake | Risk | Prevention |
|---------|------|------------|
| Accepting invalid math states silently during migration | NaNs/Infs can propagate into camera, picking, or serialization flows | Add sanity checks around inverse, normalize, and projection-sensitive helpers in validation phases |
| Assuming imported geometry data stays well-behaved under new math paths | Malformed files can stress normalization and transform code differently | Re-run importer and serializer regression scenes under the new math path |

## UX Pitfalls

| Pitfall | User Impact | Better Approach |
|---------|-------------|-----------------|
| Slightly changed camera feel | Existing workflows feel “off” even if mathematically valid | Treat camera orbit/pan/zoom behavior as a regression-sensitive acceptance check |
| Picking precision drift | Users lose trust in selection and gizmo interaction | Keep pick/gizmo regression checks in every runtime migration phase |
| Inconsistent transform results across platforms | Scene edits feel unreliable | Use the stable native platforms as explicit gates before widening rollout |

## "Looks Done But Isn't" Checklist

- [ ] **Library adoption:** Often missing convention documentation — verify handedness, clipspace, and layout are written down
- [ ] **Hotspot migration:** Often missing end-to-end checks — verify camera, picking, gizmo, and rendering still feel correct
- [ ] **Performance claim:** Often missing native measurements — verify numbers on macOS Metal and Windows Vulkan
- [ ] **Feature expansion:** Often missing fallback discipline — verify new helpers did not bypass the compatibility boundary too early

## Recovery Strategies

| Pitfall | Recovery Cost | Recovery Steps |
|---------|---------------|----------------|
| Convention drift | HIGH | Re-lock conventions in one header, re-run regression scenes, revert the affected migration slice if needed |
| SIMD alignment breakage | MEDIUM | Switch to the safer integration mode, benchmark again, and narrow the rollout until layout issues are resolved |
| Big-bang churn | HIGH | Split the work back into subsystem slices and restore a wrapper-first boundary |
| False performance win | MEDIUM | Keep the library choice, but simplify wrappers or postpone broader rollout until hotspot measurements improve |

## Pitfall-to-Phase Mapping

| Pitfall | Prevention Phase | Verification |
|---------|------------------|--------------|
| Convention drift | Phase 1 | Camera/picking/gizmo regression scenes match current native behavior |
| SIMD alignment surprises | Phase 1-2 | Native builds remain stable and benchmarks are repeatable |
| Big-bang migration churn | Phase 1-2 | Each phase changes one bounded subsystem slice |
| Performance wins on paper only | Phase 2-3 | Real app measurements show parity or gains before broader rollout |
| Expanding scope before parity | Phase 3-4 | New helpers land only after core migration gates pass |

## Sources

- https://github.com/recp/cglm
- https://cglm.readthedocs.io/en/latest/features.html
- https://cglm.readthedocs.io/en/stable/getting_started.html
- https://cglm.readthedocs.io/en/stable/opt.html
- `.planning/PROJECT.md`
- local mdCAD codebase map and `src/math3d.h` usage scan

---
*Pitfalls research for: Brownfield migration from custom C graphics math to a mature MIT C math library for mdCAD*
*Researched: 2026-03-24*
