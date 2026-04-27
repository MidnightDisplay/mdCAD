# mdCAD

## What This Is

mdCAD is a cross-platform CAD viewer and geometry editor built in C on top of Sokol, Dear ImGui, and Flecs. v1.0 shipped a staged migration of core runtime math from the local `src/math3d.h` toward a project-owned `cglm` foundation while preserving native workflow stability.

## Core Value

Interactive geometry editing and rendering must remain stable, responsive, and trustworthy on supported native platforms while the math foundation evolves underneath it.

## Milestone Status

**Shipped:** `v1.5` — Solver Workflow Robustness + Script Reapply Integrity (2026-04-13)

**Current milestone:** `v1.6` — Observable Flat JSONL Import for Large Geometry Dumps

**Current focus:** define requirements and roadmap for high-volume flat JSONL observable import.

## Current Milestone: v1.6 Observable Flat JSONL Import for Large Geometry Dumps

**Goal:** Add a high-performance JSONL import mode that flattens geometry log entries into plain scene entities under one import anchor, with optional live file observability and refresh semantics tuned for very large files.

**Target features:**
- New import menu action and dialog for a flat JSONL import mode
- Import geometry log entries as non-sketch scene entities under a single import anchor
- Attach observer metadata/component to the import anchor with an opt-out toggle in the import dialog
- Reuse sketch-style refresh behavior for anchor re-import while preserving large-scene efficiency

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
- ✓ Line-line `PARALLEL` and `PERPENDICULAR` constraints for pair and multi-line groups are legality-validated, deterministic, and transactional (`LCON-01..05`) — Validated in Phase 26: line-line-constraint-coverage
- ✓ Arc-line tangency drag robustness now satisfies stable feasible drags, transactional infeasible rollback, explicit diagnostics, and mirrored determinism (`TRDG-01..04`) — Validated in Phase 28: tangency-drag-robustness
- ✓ Active-sketch line gizmo behavior now satisfies midpoint anchoring, endpoint-authority drag semantics, mixed-selection guardrails, and grouped undo/redo interaction coherence (`GZM-01..04`) — Validated in Phase 29: active-sketch-line-gizmo-endpoint-authority
- ✓ Human-facing solver architecture docs now provide practical code anchors, literature-backed context, and a TL;DR debug primer (`SDOC-01..03`) — Validated in Phase 30: deterministic-closure-gate-windows-vulkan-solver-docs
- ✓ Deterministic Windows Vulkan closure gate now enforces build + canonical 7-test baseline + immediate rerun sign-off (`V14-01..02`) — Validated in Phase 30: deterministic-closure-gate-windows-vulkan-solver-docs
- ✓ Script re-apply now preserves descriptor participant semantics (role/sub-index), script-managed color metadata, and deterministic repeated outcomes (`SCRI-01..03`) — Validated in Phase 31: script-reapply-fidelity-foundation
- ✓ Explicit coincidence authoring semantics now preserve deterministic ArcAxisLine/tangency pair intent with durable lifecycle + script roundtrip behavior (`COIN-01..02`) — Validated in Phase 32: explicit-coincidence-authoring-semantics
- ✓ Large-jump mixed arc/line workflows now use staged robustness semantics with transactional rollback/recovery and deterministic diagnostics/parity behavior for equivalent PARALLEL/ALONG setups (`SROB-01..03`, `DIAG-01..02`, `PARI-01..02`) — Validated in Phase 33: large-jump-robustness-and-parallel-along-parity
- ✓ Deterministic v1.5 closure gate now proves canonical Windows Vulkan baseline + immediate rerun parity for targeted 7-test suite (`DIAG-03`) — Validated in Phase 34: deterministic-v1-5-closure-gate
- ✓ Observable JSONL sketch import now supports persisted observer state, transactional reparse safety, and stable large-script editor/apply behavior with user-approved UAT (`P35-01..06`) — Validated in Phase 35: observable-jsonl-as-sketch-import-with-optional-live-file-observer

### Active

- [ ] Define v1.6 requirements and phase mapping for observable flat JSONL import.
- [ ] Deliver flat non-sketch JSONL geometry import under a single anchor entity.
- [ ] Add optional observer linking and sketch-style refresh controls for flat JSONL imports.
- [ ] Validate high-entity refresh stability/performance for large JSONL geometry dumps.

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
- Milestone `v1.3` is complete: Phases 22-25 closed with deterministic solver reliability and advanced arc/line-arc constraint behavior verified.
- Requirement `V13-01` was validated in Phase 25 with strict seven-test closure gate evidence and mandatory fresh rerun pass.
- Milestone `v1.4` is complete: Phases 26-30 shipped with all plans closed (`10/10`) and all v1.4 requirements complete (`22/22`).
- v1.4 delivered line-line `PARALLEL`/`PERPENDICULAR` coverage, principal-axis ALONG reliability, tangency drag robustness, and active-sketch line gizmo endpoint-authority behavior.
- v1.4 closure is deterministic on Windows Vulkan with build + canonical seven-test baseline + immediate rerun pass evidence captured in phase verification artifacts.
- Solver architecture documentation is now published with practical code anchors, literature references, and a targeted debugging primer.
- Milestone `v1.5` is shipped with Phases 31-35 complete and archived.
- v1.5 delivered descriptor-safe script re-apply fidelity, explicit coincidence authoring semantics, parity-safe large-jump solver behavior, deterministic Windows Vulkan closure parity, and observable JSONL sketch import with transactional observer reparse semantics.
- Milestone audit result is `tech_debt`: no functional blockers; follow-up validation metadata cleanup is deferred.
## Next Milestone Goals

1. Add a new import action and dialog path for flat JSONL scene import.
2. Flatten geometry log entries into plain scene entities beneath one import anchor.
3. Attach optional file observability to the anchor and support sketch-like refresh flows.
4. Prioritize responsiveness and reliability for very large geometry log files.

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
| Scope v1.4 as robustness-first with targeted UX corrections and solver documentation | User-reported reliability issues now block smooth sketch editing; docs reduce future solver iteration risk | Completed in v1.4 |
| Lock final v1.4 closure on a canonical deterministic 7-test Windows Vulkan rerun gate and solver architecture docs | Keeps phase-close confidence reproducible and improves future solver debugging velocity | Completed in v1.4 |
| Scope v1.5 around solver workflow robustness + script re-apply integrity from real user scenarios | v1.4 closed baseline reliability, but user workflows still expose convergence and remap failures under larger jumps and script replay | Completed in v1.5 |
| Add observable JSONL-as-sketch import with optional live observer and transactional reparse safety | User workflow required persisted link/reparse UX and non-destructive recovery behavior for iterative JSONL editing | Completed in v1.5 |
| Scope v1.6 on observable flat JSONL scene import for high-entity files | Sketch import observability path is valuable but too heavy for huge geometry dumps; flat anchor import keeps live preview practical while preserving opt-in observability | Active for v1.6 |

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
*Last updated: 2026-04-27 after starting milestone v1.6*
