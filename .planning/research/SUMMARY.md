# Project Research Summary

**Project:** mdCAD
**Domain:** Brownfield migration from custom C graphics math to a mature MIT C math library
**Researched:** 2026-03-24
**Confidence:** HIGH

## Executive Summary

The research points to `cglm` as the strongest replacement candidate for `src/math3d.h`. It is an MIT-licensed C library with official support for header-only and linked usage modes, documented SIMD paths for SSE/AVX/NEON/WASM, configurable graphics conventions, and the exact kind of helper surface mdCAD is already maintaining locally: camera math, quaternions, project/unproject, frustum helpers, Apple `simd` bridges, and ray-related utilities.

For mdCAD, the recommended approach is not a raw swap. The safer architecture is to vendor `cglm`, introduce a local compatibility facade in `src/math/`, and migrate hotspot subsystems in slices. That protects the currently stable native build paths, keeps rollback simple, and lets performance claims be measured rather than assumed.

The main risks are convention drift, alignment/SIMD surprises, and losing control of the migration by expanding scope too early. Those are manageable if the roadmap starts with explicit convention capture, a wrapper boundary, and regression/performance gates on macOS Metal and Windows Vulkan.

## Key Findings

### Recommended Stack

The stack recommendation is to adopt `cglm` 0.9.6 as the vendor math backend, but keep mdCAD’s own math boundary during the migration. This satisfies the user’s licensing and language constraints while preserving room to tune for build simplicity and performance. The inline/header-first path fits the current minimal CMake workflow best; the linked call API can remain a later measurement-backed option rather than a starting assumption.

**Core technologies:**
- `cglm` 0.9.6: primary math backend — MIT, C-only usage, SIMD-aware, graphics-focused helper coverage
- Local mdCAD compatibility layer: staged migration boundary — protects conventions and limits churn
- Native benchmark/regression harness: rollout gate — proves parity and performance on stable targets

### Expected Features

The must-have features for this project are not end-user UI additions; they are infrastructure guarantees. mdCAD needs behavior parity for current math-dependent workflows, explicit control of graphics conventions, and a staged rollout that can be validated on the current native platforms. Once that baseline is secure, richer transform helpers, quaternions, decomposition, and broader geometry utilities become worthwhile follow-on gains.

**Must have (table stakes):**
- Behavior parity for camera, transforms, rendering matrices, picking, and gizmo math — users expect no regressions
- Staged compatibility layer — required to migrate safely instead of rewriting the repo in one pass
- Native performance parity or gains — core justification for replacing the local library

**Should have (competitive):**
- Built-in quaternion and decomposition support — reduces future engine-maintenance burden
- Apple `simd` interop and richer graphics helper coverage — useful for native platform quality and long-term cleanup

**Defer (v2+):**
- iOS and web parity as a hard migration gate — valuable later, but not the first milestone priority

### Architecture Approach

The architecture should put a project-owned compatibility layer between mdCAD call sites and the chosen vendor library. That boundary should own naming, conventions, and any temporary type adaptation. Underneath it, `cglm` can be used first in the simplest integration mode that preserves build simplicity. Around it, mdCAD needs explicit benchmarks and regression checks so each migration slice can be expanded or rolled back with confidence.

**Major components:**
1. Vendor math backend — `cglm` integrated in the lightest viable mode
2. Compatibility facade — stable mdCAD-facing API and convention control
3. Validation harness — native regression and benchmark gates for each migration slice

### Critical Pitfalls

1. **Convention drift** — freeze layout, handedness, and clipspace decisions before migrating runtime subsystems
2. **SIMD alignment surprises** — benchmark and control alignment strategy explicitly instead of assuming it fits current structs
3. **Big-bang churn** — migrate hotspot subsystems in slices, not the whole repo at once
4. **Paper-only performance wins** — measure in real camera/render/pick/gizmo flows, not just microbenchmarks
5. **Expanding scope before parity** — add new math surface only after native behavior and performance gates pass

## Implications for Roadmap

Based on research, suggested phase structure:

### Phase 1: Selection and Compatibility Boundary
**Rationale:** The project needs one chosen library, one documented convention set, and one project-owned integration boundary before touching many call sites.
**Delivers:** Vendor decision, convention audit, compatibility headers, and initial benchmark/regression harness
**Addresses:** Safe adoption, build simplicity, migration control
**Avoids:** Convention drift and big-bang churn

### Phase 2: Hot-Path Runtime Migration
**Rationale:** The user’s priority order is parity first, then performance, and the highest leverage path is runtime-critical math.
**Delivers:** Migration of camera, transforms, rendering matrices, picking, and gizmo math
**Uses:** `cglm` plus the local facade
**Implements:** The new runtime math backbone in the most sensitive subsystems

### Phase 3: Performance Proof and Native Hardening
**Rationale:** Before widening the rollout, mdCAD needs proof that the new foundation is at least as good on the stable native paths.
**Delivers:** Benchmarks, regression fixes, wrapper cleanup, and native validation confidence

### Phase 4: API Expansion and Long-Tail Adoption
**Rationale:** Once the base is trusted, mdCAD can use the new library to retire more custom math and unlock broader helpers.
**Delivers:** Quaternion/decomposition/helper expansion plus migration of importers, serializer, and lower-priority call sites

### Phase Ordering Rationale

- Library choice and conventions must come before runtime migration
- Runtime hotspots must come before long-tail helper code because they dominate both risk and performance value
- Performance proof must happen before broader API expansion so the migration stays aligned with the project’s stated priorities

### Research Flags

Phases likely needing deeper research during planning:
- **Phase 1:** Exact cglm integration style (`array` vs `struct` API, alignment strategy, wrapper naming)
- **Phase 3:** Benchmark design and perf interpretation across macOS Metal and Windows Vulkan

Phases with standard patterns (skip research-phase):
- **Phase 2:** Staged hotspot migration once the compatibility boundary is decided

## Confidence Assessment

| Area | Confidence | Notes |
|------|------------|-------|
| Stack | HIGH | Verified from official cglm docs and repo plus alternative candidate repos |
| Features | HIGH | Strong fit between cglm’s official helper surface and mdCAD’s current `math3d.h` / runtime usage |
| Architecture | HIGH | Driven by local codebase blast radius and established brownfield migration patterns |
| Pitfalls | HIGH | Strong overlap between official library constraints and mdCAD’s math-sensitive runtime paths |

**Overall confidence:** HIGH

### Gaps to Address

- Exact wrapper strategy: whether mdCAD should start from cglm’s array API, struct API, or a mix in the compatibility layer
- Exact benchmark suite: which scenes and interactions best capture “no regressions” and “performance win” for this app

## Sources

### Primary (HIGH confidence)
- https://github.com/recp/cglm — license, release recency, project scope
- https://cglm.readthedocs.io/en/latest/features.html — SIMD, helper surface, graphics convention support
- https://cglm.readthedocs.io/en/latest/build.html — build and integration modes
- https://cglm.readthedocs.io/en/latest/api.html — API variants
- https://cglm.readthedocs.io/en/stable/getting_started.html — alignment and allocation model
- https://cglm.readthedocs.io/en/stable/opt.html — unaligned configuration option

### Secondary (MEDIUM confidence)
- https://github.com/HandmadeMath/HandmadeMath — alternative single-header candidate with weaker fit to the stated MIT requirement
- https://github.com/Kazade/kazmath — older simple C alternative, but not the best match for the stated optimization and licensing goals

### Tertiary (LOW confidence)
- Local inference from mdCAD usage scans — exact benchmark and wrapper choices still need phase-level planning

---
*Research completed: 2026-03-24*
*Ready for roadmap: yes*
