# mdCAD

## What This Is

mdCAD is a cross-platform CAD viewer and geometry editor built in C on top of Sokol, Dear ImGui, and Flecs. It supports interactive scene editing, GPU-accelerated rendering, sketch/constraint workflows, import/export, and linked JSONL-driven geometry refresh across native targets.

## Core Value

Interactive geometry editing and rendering must remain stable, responsive, and trustworthy on supported native platforms while the math foundation evolves underneath it.

## Milestone Status

**Shipped:** `v1.7` — Linked Flat JSONL Large-File Refresh Stability (2026-05-05)

**Current milestone:** `v1.8` — Embeddable Windows JSONL Viewer

**Current focus:** Phase 43 is verified complete; next work is Phase 44 interaction/layout planning for the embedded viewer while startup JSONL and milestone-complete host workflow work remain queued.

## Current Milestone: v1.8 Embeddable Windows JSONL Viewer

**Goal:** Let a Windows host application launch mdCAD as an embeddable child viewer that can auto-open a JSONL large dump and optionally live-refresh it from the command line.

**Target features:**
- Windows-only embedded launch mode using a parent HWND contract and child-window hosting semantics
- CLI parameters for embedding and JSONL auto-import with large-flat-dump defaults and observer opt-in
- Resize, focus, keyboard, and mouse behavior that stays correct inside a resizable Avalonia host control
- A minimal Avalonia sample host project with status messaging and a bundled example JSONL resource

## Last Shipped Milestone: v1.7 Linked Flat JSONL Large-File Refresh Stability

**Goal:** Fix the large-file regression in linked flat JSONL imports so import, refresh, and deletion all preserve the full rendered geometry.

**Delivered features:**
- Linked flat imports now arm their observer baseline at commit time and settle without self-refresh drift.
- Observer-driven refresh now returns to exact live slot occupancy with safe anomaly fallback and no late churn.
- Linked-root delete now cancels active refresh work before teardown and keeps undo limited to last committed content.
- Real-file `lamp_11.jsonl` lifecycle evidence now records PASS results for auto-reload, repeated manual refresh, settled delete cleanup, and delete during refresh.

<details>
<summary>Previous shipped milestone: v1.6 Observable Flat JSONL Import for Large Geometry Dumps</summary>

**Goal:** Deliver a high-performance flat JSONL import mode for very large geometry dumps with optional observer-driven refresh.

**Delivered features:**
- Dedicated flat JSONL import menu action and dialog path
- Anchor-scoped non-sketch ingest hierarchy with deterministic naming
- Persisted observer metadata with manual transactional same-anchor refresh
- Automatic observer safety loop (debounce/retry/auto-disable) and bounded coalesced reruns
- Repeated-refresh interaction coherence guardrails and manual very-large stress evidence

</details>

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
- ✓ Flat-large JSONL import entry/configuration contract now provides dedicated menu path, mirrored pre-import options, and forward-compatible observer opt-in capture defaults (`FIMP-01`, `FIMP-02`) — Validated in Phase 36: flat-import-entry-configuration
- ✓ Anchor-scoped flat ingest now creates deterministic root->entry->geometry hierarchies with empty-entry skip, root name suffixing, and selection invariance (`FIMP-03`) — Validated in Phase 37: anchor-scoped-flat-ingest
- ✓ Observable flat-root manual/automatic refresh now preserves same-anchor transactional semantics with observer metadata durability and safety lifecycle behavior (`OBSF-01..05`) — Validated in Phases 38-39
- ✓ Large-dump refresh stability/coherence now satisfies bounded coalescing, no-lockup stress gate, and repeated-refresh interaction invariants (`PERF-01..03`) — Validated in Phase 40
- ✓ Linked flat JSONL initial import now arms its baseline at commit time, stays visually complete after settle, and records converged Scene Hierarchy totals under live linking (`FIMP-05`, `FIMP-06`) — Validated in Phase 41: linked-import-convergence
- ✓ Linked flat JSONL refresh/delete lifecycle now preserves exact settled footprint, transactional replacement, delete cleanup, and manual large-file closure evidence (`OBSF-07`, `OBSF-08`, `PERF-04`, `PERF-05`) — Validated in Phase 42: refresh-teardown-stability
- ✓ Windows host can now launch mdCAD in an embeddable child-window mode via CLI-specified parent HWND with strict true-child bootstrap and fail-fast startup semantics (`EMBD-01`, `EMBD-02`, `EMBD-03`) — Validated in Phase 43: embed-contract-child-window-bootstrap
- ✓ Minimal Avalonia `NativeControlHost` sample now builds, launches mdCAD as a child window, and surfaces bootstrap attach/failure status (`HOST-01`) — Validated in Phase 43: embed-contract-child-window-bootstrap
- ✓ Embedded launch can auto-import an absolute-path JSONL through the large flat dump workflow with optional live refresh enabled from the command line — Validated in Phases 45-46 and preserved through Phase 48: reusable-avalonia-mdcad-user-control
- ✓ Embedded viewer preserves resize, focus, keyboard, and mouse correctness inside a resizable Avalonia `NativeControlHost` — Validated in Phase 44: embedded-resize-focus-viewer-layout and preserved through Phase 48: reusable-avalonia-mdcad-user-control
- ✓ Repository includes a milestone-complete Avalonia sample host with bundled example JSONL plus launch, attach, JSONL, and live-refresh status messaging, now backed by a reusable control consumer harness — Validated in Phases 47-48

### Active

- [ ] Reusable control ships with the smallest possible sealed Avalonia host sample and a step-by-step quickstart showing `JsonlPath` binding from a viewmodel.

### Out of Scope

- Host-to-viewer IPC beyond the CLI launch contract — keep the first embedding milestone process-launched and integration-light
- In-process or SDK-style embedding — mdCAD remains a separate process hosted through a Win32 child HWND
- Cross-platform embedding parity beyond the Windows/Avalonia workflow — target the immediate Windows desktop host need first
- Broader flat-import UX expansion unrelated to launch-time JSONL viewing — keep scope on embeddable viewer reliability

## Context

v1.0 and v1.1 are shipped and archived. v1.1 closed long-tail migration and validation gates across phases 6-9, including final boundary documentation and parity/perf/manual verification.

v1.2 pivots to a larger feature system proposal captured in `docs/feature-proposal/Sketches, Constraints, Scripting.md`: constrained sketches, solver UX, and scripting-first bidirectional workflows.

v1.7 is driven by a large-file regression in the flat JSONL observer path: with `File -> Import JSONL (Flat Large Dump)` plus `Link file for refresh (optional)`, scene totals keep climbing after the initial import appears complete, visible line segments collapse to a later tail subset, some joints remain as orphaned points, and deleting the import root leaves dangling geometry. The same file imported without live refresh does not reproduce the issue.

v1.8 is driven by a Windows host-integration workflow: an Avalonia desktop application needs to launch mdCAD as a child-window viewer process, pass a parent HWND plus launch-time JSONL parameters, and rely on the already-stabilized flat JSONL observer path without introducing a richer IPC layer yet.

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
- Milestone `v1.6` is shipped with Phases 36-40 complete and archived.
- v1.6 delivered flat-large JSONL import UX, anchor-scoped ingest, observer metadata/manual transactional refresh, automatic safety loop, and repeated-refresh coherence closure.
- Milestone audit result is `tech_debt`: no blocker gaps; follow-up traceability/Nyquist metadata cleanup is deferred.
- Milestone `v1.7` is shipped with Phases 41-42 complete and archived.
- v1.7 delivers stable linked flat JSONL convergence across initial load, observer refresh, repeated manual refresh, and linked-root deletion on the real `lamp_11.jsonl` dataset.
- Milestone audit result is `tech_debt`: all requirements are satisfied; deferred debt is `Clear Scene` lifecycle parity plus Nyquist validation backfill for Phases 41-42.
- Milestone `v1.8` remains active: Phases 43-48 closed the child-HWND embedding contract, embedded input/layout closure, launch-time JSONL import/live refresh, sample-host proof, and the reusable Avalonia control package/harness.
- Phase 49 now tops off v1.8 with the smallest sealed-mode Avalonia host sample and a step-by-step reusable-control quickstart before final milestone closeout.
## Next Milestone Goals

1. Add a Windows embedding contract so an external host can launch mdCAD into a child HWND with predictable lifecycle and resize behavior.
2. Reuse the large flat JSONL import path for launch-time auto-open with optional live refresh opt-in from absolute file paths.
3. Prove the integration end-to-end with a minimal Avalonia sample host and bundled example JSONL data.
4. Preserve current native build reliability while keeping `Clear Scene` lifecycle parity and Nyquist backfill explicitly deferred unless the new work exposes them as blockers.

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
| Scope v1.6 on observable flat JSONL scene import for high-entity files | Sketch import observability path is valuable but too heavy for huge geometry dumps; flat anchor import keeps live preview practical while preserving opt-in observability | Completed in v1.6 |
| Scope v1.7 on linked flat JSONL large-file refresh stability and cleanup correctness | The regression is isolated to observer-enabled large flat imports, so a focused milestone keeps diagnosis and closure measurable | Completed in v1.7 |
| Arm linked flat import observer baseline at import commit time | Prevent immediate self-refresh drift on the first linked import settle | Completed in v1.7 |
| Use exact-footprint refresh compaction plus cancel-before-delete teardown ordering | The large-file regression was a lifecycle correctness problem, not a signal to redesign the entire slot-buffer architecture | Completed in v1.7 |
| Scope v1.8 on Windows-only process-hosted embedding with CLI launch arguments | The immediate need is an Avalonia host integration path without committing mdCAD to a new SDK or IPC surface yet | Validated in Phase 43 |
| Pull embedded child resize syncing into Phase 43 instead of deferring it | Manual attach approval depended on drag/snap/fullscreen resize behavior working in the sample host, so the bootstrap proof had to include host-side bounds synchronization | Confirmed in Phase 43 |
| Keep requested mdCAD panels visible during the embedded bootstrap proof and defer chrome trimming polish | Manual acceptance required the hierarchy/inspector/debug/visibility/controls panels to remain visible in the hosted viewer; embedded chrome trimming can follow later polish work | Confirmed in Phase 43 |
| Keep the reusable control sealed-by-default and surface the old host chrome only in explicit diagnostic mode | External consumers need a minimal embeddable API, while the in-repo harness still needs truthful launch/relaunch/warning diagnostics | Confirmed in Phase 48 |

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
*Last updated: 2026-05-15 after Phase 48 completion*
