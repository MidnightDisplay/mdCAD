# mdCAD

## What This Is

mdCAD is a cross-platform CAD viewer and geometry editor built in C on top of Sokol, Dear ImGui, and Flecs. It already supports interactive scene editing, GPU-accelerated rendering, import/export, and multiple native and web targets; the current work is to modernize its math foundation by replacing the local `src/math3d.h` with a mature MIT-licensed C library that is safer to maintain and better optimized on native platforms.

## Core Value

Interactive geometry editing and rendering must remain stable, responsive, and trustworthy on supported native platforms while the math foundation evolves underneath it.

## Requirements

### Validated

- ✓ Cross-platform native app shell with Sokol-driven rendering and Dear ImGui UI — existing
- ✓ ECS-based scene model with transforms, hierarchy, selection, undo/redo, and serialization — existing
- ✓ Geometry editing workflows including viewport interaction, picking, and translation gizmo support — existing
- ✓ Import/export workflows for scene JSON, JSONL geometry logs, point clouds, and PLY mesh data — existing
- ✓ Native macOS Metal build path and Windows Vulkan build path for active development — existing
- ✓ Header-only subsystem pattern across most of `src/` with a minimal CMake/Ninja workflow — existing

### Active

- [ ] Replace `src/math3d.h` with a mature MIT-licensed C math library that works with mdCAD's C-only codebase and current build setup
- [ ] Migrate math usage in staged slices so macOS and Windows Vulkan remain regression-free throughout the rollout
- [ ] Preserve or improve performance on native targets, especially in runtime-critical paths such as camera, transforms, picking, gizmo interaction, and rendering
- [ ] Expand math capabilities during migration where it directly improves the engine foundation, including broader transform helpers and future-ready GPU/CPU-friendly data handling

### Out of Scope

- Full repo-wide big-bang replacement in a single step — staged migration is easier to verify and safer for existing native builds
- iOS and web parity as a gate for the first migration milestone — those targets are intentionally on hold until macOS and Windows Vulkan are stable
- Adoption of a C++ math library — the codebase is intentionally C-first and the user explicitly rejected C++ for this work

## Context

mdCAD already ships a custom header-only math layer in `src/math3d.h` that provides vectors, matrices, inverse/unprojection helpers, and ray math. That local layer is used widely across the runtime, including `src/app.c`, `src/orbit_camera.h`, `src/ecs/ecs_scene.h`, `src/gpu/geometry_batch.h`, `src/gpu/pick_buffer.h`, `src/gizmo/`, importers, serializer code, undo/redo, and multiple UI panels.

The motivation for this project is to reduce the amount of custom math code mdCAD has to own while gaining access to a more mature, better optimized foundation. The replacement library must be MIT-licensed, written for C consumption, compatible with the current minimal CMake setup, and should not materially inflate build times. Header-only is preferred, but not mandatory if the integration remains lightweight.

The immediate success order is clear: first no regressions on the currently stable native paths, then measurable performance gains or at least no losses, then a cleaner API surface, then additional math capabilities that unlock future work. The active native validation environments today are macOS on Apple Silicon using Ninja in `build/` and Windows Vulkan builds; those are the primary gates for the early migration stages.

The current codebase map also highlights migration-sensitive areas: backend picking paths are fragile, serializer/import code is manual and allocation-heavy, and the math layer touches many hot rendering and interaction paths. That makes staged replacement and explicit regression testing mandatory rather than optional.

## Constraints

- **License**: MIT-licensed C library only — reduces legal and maintenance friction and matches the project's current dependency posture
- **Language**: No C++ dependency — the codebase is intentionally C-first and must remain easy to build across the supported native targets
- **Platform Priority**: macOS Metal and Windows Vulkan must stay stable during migration — these are the native build paths currently considered reliable enough to gate changes
- **Performance**: Native performance must not regress and should improve where possible — the migration is partly justified by the chance to gain SIMD and better low-level implementations
- **Rollout Strategy**: Migration must be staged — the blast radius across camera, ECS, picking, gizmo, rendering, importers, and serializer code is too large for a single cutover
- **Build Simplicity**: Integration must fit the existing lightweight CMake workflow without materially increasing build times — mdCAD currently relies on a minimal build setup and should keep that advantage

## Key Decisions

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| Treat this as a staged migration instead of a single-step swap | `src/math3d.h` is used across many runtime-critical systems and staged rollout is easier to validate | — Pending |
| Prioritize native stability on macOS and Windows Vulkan before other targets | These are the currently stable build paths and the safest regression gates | — Pending |
| Expand math capability during migration when it helps the foundation | The user wants to gain more than a 1:1 swap if the rollout remains safe | — Pending |
| Keep the replacement C-only and MIT-licensed | This preserves compatibility with mdCAD's architecture and dependency expectations | — Pending |
| Accept non-header-only integration if build simplicity and performance still hold | Header-only is preferred, but not at the cost of choosing an inferior library | — Pending |

## Evolution

This document evolves at phase transitions and milestone boundaries.

**After each phase transition** (via `$gsd-transition`):
1. Requirements invalidated? → Move to Out of Scope with reason
2. Requirements validated? → Move to Validated with phase reference
3. New requirements emerged? → Add to Active
4. Decisions to log? → Add to Key Decisions
5. "What This Is" still accurate? → Update if drifted

**After each milestone** (via `$gsd-complete-milestone`):
1. Full review of all sections
2. Core Value check — still the right priority?
3. Audit Out of Scope — reasons still valid?
4. Update Context with current state

---
*Last updated: 2026-03-24 after initialization*
