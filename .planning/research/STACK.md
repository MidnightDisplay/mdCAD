# Stack Research: v1.2 Sketches, Constraints, Scripting

**Domain:** New feature milestone for constrained sketches + scripting in mdCAD  
**Researched:** 2026-03-30  
**Confidence:** Medium (final library lock requires spike validation in this repo)

## Recommendations

### Solver

Use a **project-owned solver interface** first, then lock one backend after a short spike.

| Option | Fit | License | Notes |
|---|---|---|---|
| C-first lightweight backend (preferred) | Best alignment with C-first codebase | Must be permissive | Validate capability for FIXED/COINCIDENT/PARALLEL/PERPENDICULAR/LENGTH/ANGLE and arc/circle support |
| Ceres (fallback) | Strong nonlinear solve capability | BSD-3-Clause | Requires a C++ island wrapper and ABI boundary |
| In-house minimal solver | Full control | Project license | Only if off-the-shelf options fail acceptance criteria |

**Recommendation:** keep backend swappable behind `src\solver\` API and lock one solver during the first implementation phase.

### Scripting runtime

| Runtime | Recommendation | Why |
|---|---|---|
| Lua 5.4.x | Primary | Small, embeddable in C, permissive, cross-platform friendly |
| Duktape / QuickJS | Alternative | Viable but more runtime complexity vs Lua for this milestone |

**Recommendation:** lock **Lua 5.4.x** and implement a deterministic mdCAD sketch script format that round-trips cleanly.

### Rendering/UI support

- Reuse existing Sokol + ImGui + pick buffer stack.
- Add in-house constraint glyph batching/picking; no new UI framework.
- Keep dimension labels/editing in existing ImGui workflows first.

## What not to add in v1.2

- No GPL/viral-licensed dependencies.
- No full CAD kernel adoption for this milestone.
- No multi-solver feature set in first release slice.
- No scripting runtime that cannot reasonably target iOS/web later.

## Adoption checks (must pass before lock)

1. Builds on Windows MSVC + Vulkan path.
2. Permissive license confirmed.
3. Can represent required initial constraints and return actionable diagnostics.
4. Can be packaged in current CMake workflow without major build instability.

