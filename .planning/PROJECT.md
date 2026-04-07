# mdCAD

## What This Is

mdCAD is a cross-platform CAD viewer and geometry editor built in C on top of Sokol, Dear ImGui, and Flecs. v1.0 shipped a staged migration of core runtime math from the local `src/math3d.h` toward a project-owned `cglm` foundation while preserving native workflow stability.

## Core Value

Interactive geometry editing and rendering must remain stable, responsive, and trustworthy on supported native platforms while the math foundation evolves underneath it.

## Current Milestone: v1.3 Sketch Solver Audit + Constraint Expansion

**Goal:** Stabilize sketch solver behavior through targeted audit/fixes and add advanced line/arc constraint capabilities needed for robust sketch workflows.

**Current focus:**
- Audit and fix solver execution semantics: auto-solve triggers, deterministic recalculate behavior, and broken constraint pathways.
- Implement bounded multi-pass solve policy with tolerance stop criteria and configurable pass cap (default 10).
- Add advanced constraints for arc/line-arc interactions and directional group constraints across point sets.

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
- ✓ Serializer/save-load long-tail migration now runs through cglm-backed paths with explicit schema-v2 converter and targeted parity evidence — Validated in Phase 6: serializer-and-save-load-long-tail-migration
- ✓ Import long-tail migration now runs through cglm-backed importer paths with targeted parity evidence — Validated in Phase 7: import-pipeline-long-tail-migration
- ✓ Undo/editor utility migration now uses cglm-backed helper boundaries with glue burn-down evidence — Validated in Phase 8: undo-editor-utility-migration-and-glue-burn-down
- ✓ Phase 9 long-tail validation/performance and boundary finalization gates (`VAL-01`, `VAL-02`, `VAL-03`, `TRED-02`) are satisfied with final verification pass — Validated in Phase 9: long-tail-validation-performance-gates-and-boundary-finalization
- ✓ Constraint UX closure unified glyph and manager participant highlighting semantics and closed Phase 11 verification/validation evidence debt (`SKCH-04`, `CONS-01`, `CONS-02`, `CONS-03`, `CONS-04`, `CONS-05`) — Validated in Phase 16: constraint-ux-closure-and-verification
- ✓ Solver-control and constrained-interaction requirements (`SOLV-01`, `SOLV-02`, `SOLV-03`, `SOLV-04`, `API-03`) are fully verified, including implication clear-on-success lifecycle closure — Validated in Phase 12: solver-control-and-constrained-interaction

### Active

- [ ] Add sketch entity system with solver status, geometry/constraint management, and fix/loose workflow controls.
- [ ] Add bidirectional sketch scripting subsystem with script editor, parser, serializer, and IO parameter UI.
- [ ] Expand scene API + undo integration for sketch/constraint/script operations.
- [ ] Add constraint-focused example sketches/scripts for validation and debugging.

### Out of Scope

- Full repo-wide big-bang replacement in a single step — staged migration is easier to verify and safer for existing native builds
- Adoption of a C++ math library — the codebase is intentionally C-first and the user explicitly rejected C++ for this work
- macOS parity as an equal-time dev gate during feature buildout — deferred until Windows Vulkan feature completion

## Context

v1.0 and v1.1 are shipped and archived. v1.1 closed long-tail migration and validation gates across phases 6-9, including final boundary documentation and parity/perf/manual verification.

v1.2 pivots to a larger feature system proposal captured in `docs/feature-proposal/Sketches, Constraints, Scripting.md`: constrained sketches, solver UX, and scripting-first bidirectional workflows.

## Current State

- Milestone `v1.1` shipped with Phases 6-9 complete and archived.
- Long-tail migration closure requirements (`TAIL-01..03`, `TRED-01..02`, `VAL-01..03`) are complete.
- Milestone `v1.2` shipped with Phases 10-21 complete and archived.
- Sketch, constraint, solver, scripting, and endpoint-undo requirement sets are implemented and mapped across shipped v1.2 artifacts.
- Phase 21 closed traceability alignment with a product fix for SKCH hierarchy refresh and final requirement status promotion for SKCH/PH18 targets.
- Archived milestone audit for v1.2 remains `gaps_found` and is explicitly accepted as known debt at closure.
- Next active workflow is milestone initialization for v1.3.

## Next Milestone Goals

1. Close sketch solver reliability defects surfaced from interactive sketching (auto-solve, recalc convergence, and non-working constraint types).
2. Expand solver/constraint coverage for advanced arc-line interactions and principal-direction group constraints over point selections.
3. Preserve deterministic solve behavior and maintain Windows Vulkan as the primary development/test gate while introducing new constraint math.

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
| Scope v1.1 to long-tail migration plus full thin-entrypoint reduction | Maximizes migration debt burn-down while retaining native gate confidence | Completed in v1.1 |
| Pivot v1.2 from deferred platform validation to sketch/constraint/scripting feature expansion | New proposal defines a higher-value capability set with interconnected systems | Active for v1.2 |

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
*Last updated: 2026-04-07 after v1.3 milestone kickoff*
