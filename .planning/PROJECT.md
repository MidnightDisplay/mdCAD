# mdCAD

## What This Is

mdCAD is a cross-platform CAD viewer and geometry editor built in C on top of Sokol, Dear ImGui, and Flecs. v1.0 shipped a staged migration of core runtime math from the local `src/math3d.h` toward a project-owned `cglm` foundation while preserving native workflow stability.

## Core Value

Interactive geometry editing and rendering must remain stable, responsive, and trustworthy on supported native platforms while the math foundation evolves underneath it.

## Current Milestone: v1.1 Long-Tail Migration

**Goal:** Complete long-tail math migration slices and finish thin-entrypoint reduction while preserving native gate stability.

**Target features:**
- Migrate lower-priority math consumers (serializer, importers, undo/redo helpers, editor utilities) to cglm-backed paths.
- Complete `TAIL-02` by removing/reducing temporary thin-entrypoint migration glue where safe.
- Keep macOS Metal and Windows Vulkan behavior/performance gates stable for expanded migrated surfaces.
- Explicitly defer iOS/web validation work (`PLAT-01`, `PLAT-02`) to a follow-up milestone.

## Requirements

### Validated

- ✓ Cross-platform native app shell with Sokol-driven rendering and Dear ImGui UI — existing
- ✓ ECS-based scene model with transforms, hierarchy, selection, undo/redo, and serialization — existing
- ✓ Geometry editing workflows including viewport interaction, picking, and translation gizmo support — existing
- ✓ Import/export workflows for scene JSON, JSONL geometry logs, point clouds, and PLY mesh data — existing
- ✓ Native macOS Metal build path and Windows Vulkan build path for active development — existing
- ✓ Header-only subsystem pattern across most of `src/` with a minimal CMake/Ninja workflow — existing
- ✓ `cglm` `0.9.6` selected, vendored, and build-proven in the native workflow — Validated in Phase 1: selection-and-conventions
- ✓ Project-owned math convention contract established in `src/math/math_conventions.h` and `docs/MATH_CONVENTIONS.md` — Validated in Phase 1: selection-and-conventions
- ✓ Thin project-owned `cglm` entrypoint now owns clip-depth config and compile-time policy checks without wrapping vendor math types — Validated in Phase 2: direct-adoption-tooling-and-validation-harness
- ✓ Standalone native math validation harness and named regression/benchmark targets now exist for staged hotspot migration — Validated in Phase 2: direct-adoption-tooling-and-validation-harness
- ✓ Core camera/transform/render-matrix hotspots are migrated to the cglm-backed path on macOS with parity checks — Validated in Phase 3: macos-core-transform-migration
- ✓ Interaction math (pick/ray/gizmo) and quaternion helper expansion are migrated to the shared cglm-backed boundary with compare/bench coverage — Validated in Phase 4: interaction-math-and-api-expansion
- ✓ Migrated interaction runtime slices no longer depend on equivalent legacy `src/math3d.h` helpers (now scoped as deprecated) — Validated in Phase 4: interaction-math-and-api-expansion
- ✓ Windows Vulkan hardening and native performance gates closed with `Decision: GO` — Validated in Phase 5: windows-vulkan-hardening-and-performance-gates

### Active

- [ ] Complete `TAIL-01` migration across serializer/importer/undo/editor utility math paths.
- [ ] Complete `TAIL-02` thin-entrypoint reduction and retire safe temporary glue.
- [ ] Preserve or improve behavior/performance on macOS Metal and Windows Vulkan for the expanded migrated slice.

### Out of Scope

- Full repo-wide big-bang replacement in a single step — staged migration is easier to verify and safer for existing native builds
- Adoption of a C++ math library — the codebase is intentionally C-first and the user explicitly rejected C++ for this work
- Full iOS native and web/WASM validation in this milestone — intentionally deferred to keep v1.1 focused on long-tail migration execution

## Context

v1.0 is shipped and archived with all five planned phases complete, a passed milestone audit, and release tag `v1.0`. The next step is to finish long-tail migration work that was intentionally deferred after hotspot stabilization.

This milestone should reduce migration debt (temporary glue and remaining legacy helper dependence) while keeping existing native runtime confidence intact via compare/bench/manual gates.

## Current State

- Milestone `v1.1` initialized (planning)
- Scope set to `TAIL-01` + full `TAIL-02`
- Platform expansion (`PLAT-01`, `PLAT-02`) deferred

## Next Milestone Goals

1. Deliver cglm-backed long-tail parity for serializer/importer/undo/editor utility flows.
2. Remove or sharply reduce temporary migration glue in thin entrypoint boundaries.
3. Reconfirm native behavior and performance confidence on macOS and Windows.

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
| Treat this as a staged migration instead of a single-step swap | `src/math3d.h` is used across many runtime-critical systems and staged rollout is easier to validate | Confirmed in Phase 1 |
| Prioritize native stability on macOS and Windows Vulkan before other targets | These are the currently stable build paths and the safest regression gates | Confirmed in Phase 1 |
| Expand math capability during migration when it helps the foundation | The user wants to gain more than a 1:1 swap if the rollout remains safe | Confirmed in Phases 1-4 |
| Keep the replacement C-only and MIT-licensed | This preserves compatibility with mdCAD's architecture and dependency expectations | `cglm` `0.9.6` selected and vendored in Phase 1 |
| Use direct `cglm` adoption through a thin project-owned entrypoint | Direct vendor adoption reduces wrapper maintenance while preserving one integration choke point | Confirmed in Phase 1 |
| Use a harness-first validation workflow before hotspot migration | Staged rollout needs repeatable compare/bench gates before runtime math is swapped | Confirmed in Phase 2 |
| Close milestone only after native Windows Vulkan rerun resolves benchmark-noise gate ambiguity | Gate reliability matters more than low-iteration convenience | Confirmed in Phase 5 with 2,000,000-iteration rerun |
| Scope v1.1 to long-tail migration plus full thin-entrypoint reduction | Maximizes migration debt burn-down while retaining native gate confidence | Active for v1.1 |

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
*Last updated: 2026-03-26 at v1.1 milestone initialization*
