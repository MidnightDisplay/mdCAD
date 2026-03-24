# Stack Research

**Domain:** Brownfield migration from custom C graphics math to a mature MIT C math library for mdCAD
**Researched:** 2026-03-24
**Confidence:** HIGH

## Recommended Stack

### Core Technologies

| Technology | Version | Purpose | Why Recommended |
|------------|---------|---------|-----------------|
| cglm | 0.9.6 | Primary replacement for `src/math3d.h` | Official docs and repo show MIT licensing, header-only or linked modes, SIMD support for SSE/AVX/NEON/WASM, clipspace/handedness options, quaternions, frustum/AABB/project-unproject helpers, and Apple `simd` interop |
| mdCAD math compatibility facade | Project-local | Stage migration behind a stable local API boundary | Lets mdCAD migrate subsystem by subsystem instead of rewriting every call site at once, and isolates vendor-specific types or naming choices |
| Native benchmark and regression harness | Project-local | Validate behavior and performance during migration | Required to prove no regressions on macOS Metal and Windows Vulkan before broader rollout |

### Supporting Libraries

| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| cglm inline array API (`cglm/cglm.h`) | 0.9.6 | Lowest-friction header-only integration | Use first if the goal is minimal build-system churn and close-to-metal performance |
| cglm struct API (`cglm/struct.h`) | 0.9.6 | Safer staged migration with `.x/.y/.z`-style access and configurable namespacing | Use when adapting existing `vec3_t` / `mat4_t` semantics or reducing call-site churn matters more than raw array style |
| cglm call API (`cglm/call.h`) | 0.9.6 | Precompiled non-inline alternative | Use only if measurements show binary size or compile-time pain from the fully inline path |

### Development Tools

| Tool | Purpose | Notes |
|------|---------|-------|
| Existing `CMakeLists.txt` + Ninja workflow | Native integration and validation | Prefer vendored header-only integration first so mdCAD does not need a more complex build graph |
| Native benchmark executable under `build/` | Measure old vs new math in runtime-critical paths | Use on the active M1 macOS path and mirror on Windows Vulkan before expanding rollout |
| Existing web/Puppeteer and native smoke paths | Secondary regression checks | Keep them after the native stable gates, not before |

## Installation

```bash
# Preferred first pass: vendor cglm as headers only
mkdir -p vendors
git clone https://github.com/recp/cglm.git vendors/cglm

# Minimal CMake integration pattern
# 1. add include path to vendors/cglm/include
# 2. include <cglm/...> from a local compatibility header
# 3. keep mdCAD's existing Ninja build path for validation

cmake -B build -G Ninja
ninja -C build
```

## Alternatives Considered

| Recommended | Alternative | When to Use Alternative |
|-------------|-------------|-------------------------|
| cglm | HandmadeMath 2.0.0 | Only if single-header convenience matters more than MIT licensing and broader built-in graphics helper coverage; HandmadeMath is CC0/public-domain style, not MIT |
| cglm | Kazmath | Only if preserving an older simple C API matters more than SIMD breadth, release cadence, and richer graphics-oriented helper coverage |
| cglm with compatibility facade | Direct repo-wide raw cglm adoption | Only after the hotspot migration is stable and naming/convention questions are fully settled |

## What NOT to Use

| Avoid | Why | Use Instead |
|-------|-----|-------------|
| GLM | C++ library, conflicts with the project’s C-only requirement | cglm |
| DirectXMath | C++ and Windows-centric, poor fit for macOS/iOS/Web portability | cglm |
| Growing `src/math3d.h` further | Increases maintenance burden without gaining the maturity or platform optimizations of a dedicated library | cglm behind a local facade |

## Stack Patterns by Variant

**If minimum build churn matters most:**
- Vendor `cglm` as headers and use the inline API first
- Because mdCAD can keep its current minimal CMake/Ninja setup and validate behavior before deciding whether linked call APIs are worth it

**If migration ergonomics matter most:**
- Start from `cglm` struct API plus a local compatibility header
- Because struct types, configurable namespace handling, and local wrappers make staged replacement easier to test

**If alignment becomes the biggest integration risk:**
- Start with cglm’s documented unaligned option strategy and benchmark the cost
- Because cglm’s SIMD-friendly types impose alignment rules by default and mdCAD should treat that as an explicit adoption decision rather than an accidental one

## Version Compatibility

| Package A | Compatible With | Notes |
|-----------|-----------------|-------|
| `cglm` 0.9.6 | C11/C17 mdCAD runtime | Good fit for mdCAD’s C-first codebase; no external dependencies required by cglm itself |
| `cglm` inline API | Existing minimal mdCAD CMake flow | Best match for preserving current build simplicity |
| `cglm` struct API | Local wrapper preserving `vec3_t` / `mat4_t` style semantics | Useful for staged migration where call-site churn is a risk |

## Sources

- https://github.com/recp/cglm — verified MIT license, release recency, header-only/SIMD/API claims
- https://cglm.readthedocs.io/en/latest/features.html — verified SIMD architectures, clipspace options, quaternion/frustum/project-unproject/ray helpers, Apple `simd` conversion helpers
- https://cglm.readthedocs.io/en/latest/build.html — verified no external dependencies and header-only / linked integration modes
- https://cglm.readthedocs.io/en/latest/api.html — verified inline, call, struct, and SIMD API split
- https://cglm.readthedocs.io/en/stable/getting_started.html — verified alignment expectations and heap-allocation model
- https://cglm.readthedocs.io/en/stable/opt.html — verified unaligned configuration option
- https://github.com/HandmadeMath/HandmadeMath — verified single-header C/C++ support, CC0 licensing, and release recency
- https://github.com/Kazade/kazmath — verified simpler legacy C alternative, non-MIT license, and optimization backlog

---
*Stack research for: Brownfield migration from custom C graphics math to a mature MIT C math library for mdCAD*
*Researched: 2026-03-24*
