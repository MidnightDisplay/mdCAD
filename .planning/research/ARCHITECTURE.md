# Architecture Research

**Domain:** Brownfield migration from custom C graphics math to a mature MIT C math library for mdCAD
**Researched:** 2026-03-24
**Confidence:** HIGH

## Standard Architecture

### System Overview

```text
┌─────────────────────────────────────────────────────────────┐
│                    mdCAD Runtime Call Sites                │
├─────────────────────────────────────────────────────────────┤
│  app / camera / ECS / GPU / gizmo / importers / serializer │
└───────────────┬─────────────────────────────────────────────┘
                │
┌───────────────▼─────────────────────────────────────────────┐
│                 Local Math Compatibility Layer              │
├─────────────────────────────────────────────────────────────┤
│  Stable project-facing types, naming, conventions, guards   │
│  Conversion helpers, phased API shims, benchmark toggles    │
└───────────────┬─────────────────────────────────────────────┘
                │
┌───────────────▼─────────────────────────────────────────────┐
│                    Vendor Math Backend                      │
├─────────────────────────────────────────────────────────────┤
│  cglm inline / struct / call APIs, SIMD paths, conventions  │
└───────────────┬─────────────────────────────────────────────┘
                │
┌───────────────▼─────────────────────────────────────────────┐
│               Validation + Performance Harness              │
├─────────────────────────────────────────────────────────────┤
│  Native builds, regression scenes, benchmarks, comparisons  │
└─────────────────────────────────────────────────────────────┘
```

### Component Responsibilities

| Component | Responsibility | Typical Implementation |
|-----------|----------------|------------------------|
| Vendor math backend | Provide optimized primitives and richer graphics math coverage | Vendored `cglm` headers first, linked call API only if later measurement justifies it |
| Compatibility layer | Shield mdCAD call sites from immediate vendor lock-in and convention drift | Local headers that define project-facing types/functions and delegate to the backend |
| Validation harness | Prove behavior parity and performance results | Dedicated smoke scenes, benchmarks, and platform-gated checks tied to existing native build paths |
| Migration slices | Replace math usage in safe, testable batches | Hotspot-first subsystem rollout: camera → transforms/rendering → picking/gizmo → long-tail code |

## Recommended Project Structure

```text
vendors/
├── cglm/                 # Vendored upstream math library

src/
├── math/                 # New migration-owned math boundary
│   ├── math_backend.h    # Backend selection / upstream includes
│   ├── math_compat.h     # mdCAD-facing compatibility wrappers
│   ├── math_conventions.h# Handedness, clipspace, layout choices
│   └── math_bench.h      # Optional benchmark helpers
├── orbit_camera.h        # First hotspot migration candidate
├── ecs/                  # Transform-heavy runtime systems
├── gpu/                  # Rendering, picking, culling math
├── gizmo/                # Ray/plane/drag math
└── ...                   # Remaining staged adopters
```

### Structure Rationale

- **`vendors/cglm/`:** Keeps the upstream source visible, versionable, and isolated from mdCAD-owned wrapper policy
- **`src/math/`:** Prevents raw third-party types from leaking everywhere on day one and gives the rollout one stable control point

## Architectural Patterns

### Pattern 1: Compatibility Facade First

**What:** Introduce a project-owned math boundary before touching most call sites.
**When to use:** Brownfield migrations with broad usage across runtime-critical subsystems.
**Trade-offs:** Adds a temporary wrapper layer, but drastically reduces migration risk and makes rollback easier.

**Example:**
```c
/* math_compat.h */
typedef vec3s vec3_t;
typedef mat4s mat4_t;

static inline mat4_t mat4_identity(void) {
    return glms_mat4_identity();
}
```

### Pattern 2: Hotspot-First Rollout

**What:** Migrate the most performance-sensitive and convention-sensitive subsystems before long-tail helpers.
**When to use:** When the goal order is parity first, then performance, then API expansion.
**Trade-offs:** Leaves old and new math paths coexisting for a while, but makes testing and attribution much easier.

### Pattern 3: Explicit Convention Capsule

**What:** Centralize layout, handedness, and clipspace decisions in one project-owned location.
**When to use:** Any graphics codebase targeting multiple APIs and platforms.
**Trade-offs:** Up-front discipline required, but it prevents subtle transform and projection bugs from spreading.

## Data Flow

### Request Flow

```text
[Viewport / Scene Action]
    ↓
[Subsystem math call]
    ↓
[mdCAD compatibility wrapper]
    ↓
[cglm API + selected convention]
    ↓
[Result returned to ECS / GPU / gizmo code]
```

### State Management

```text
[Conventions config]
    ↓
[Compatibility layer] ←→ [Subsystem migration slices] → [Bench / regression checks]
```

### Key Data Flows

1. **Runtime math flow:** Camera, transform, pick, and gizmo code call the local wrapper, which forwards to the selected vendor API with fixed conventions
2. **Migration validation flow:** Old-path expectations and new-path outputs are compared in targeted regression scenarios before widening rollout

## Scaling Considerations

| Scale | Architecture Adjustments |
|-------|--------------------------|
| 0-5 subsystems migrated | Wrapper-heavy approach is fine; optimize for confidence and reversibility |
| 5-15 subsystems migrated | Tighten conventions, remove redundant conversions, and expand benchmark coverage |
| Repo-wide adoption | Decide whether to keep the facade permanently or collapse wrappers where they no longer add value |

### Scaling Priorities

1. **First bottleneck:** Convention mismatch between old and new math — solve with one explicit compatibility boundary
2. **Second bottleneck:** Hidden copy/alignment costs — solve with benchmark-driven wrapper cleanup after hotspot migration

## Anti-Patterns

### Anti-Pattern 1: Raw Vendor Types Everywhere on Day One

**What people do:** Include the third-party headers directly throughout the repo and rename call sites opportunistically.
**Why it's wrong:** Makes convention bugs, rollback, and staged testing much harder.
**Do this instead:** Route adoption through `src/math/` first.

### Anti-Pattern 2: Measure in Microbenchmarks Only

**What people do:** Declare victory from isolated math timings without validating frame-critical workflows.
**Why it's wrong:** Real regressions often come from wrappers, conversions, cache behavior, or changed conventions.
**Do this instead:** Pair microbenchmarks with real app flows on macOS Metal and Windows Vulkan.

## Integration Points

### External Services

| Service | Integration Pattern | Notes |
|---------|---------------------|-------|
| Vendored upstream math library | Source vendoring into `vendors/` | Prefer minimal build impact first |
| Native compiler SIMD support | Implicit via the chosen library and toolchain | Keep parity fallback paths for unsupported targets |

### Internal Boundaries

| Boundary | Communication | Notes |
|----------|---------------|-------|
| `src/math/` ↔ `src/orbit_camera.h` | Direct wrapper calls | Good first migration slice |
| `src/math/` ↔ `src/ecs/ecs_scene.h` / `src/gpu/` | Direct wrapper calls | Highest-risk convention and performance area |
| `src/math/` ↔ `src/gizmo/` | Direct wrapper calls | Sensitive to ray/plane precision and interaction feel |
| `src/math/` ↔ serializer/importers | Direct wrapper calls | Lower urgency, migrate after runtime hotspots stabilize |

## Sources

- https://github.com/recp/cglm
- https://cglm.readthedocs.io/en/latest/features.html
- https://cglm.readthedocs.io/en/latest/api.html
- https://cglm.readthedocs.io/en/latest/api_struct.html
- `.planning/PROJECT.md`
- `src/math3d.h`
- local usage scan across `src/`

---
*Architecture research for: Brownfield migration from custom C graphics math to a mature MIT C math library for mdCAD*
*Researched: 2026-03-24*
