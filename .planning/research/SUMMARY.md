# Project Research Summary

**Project:** mdCAD  
**Domain:** v1.3 Sketch Solver Audit + Constraint Expansion  
**Researched:** 2026-04-07  
**Confidence:** MEDIUM-HIGH

## Executive Summary

This milestone is a reliability-first expansion of mdCAD’s constrained sketching system: harden the solver contract, stabilize transaction flow, and then broaden supported constraints and script-driven workflows. The research converges on a clear implementation style used by mature CAD tools: a single transactional mutation path, deterministic solve behavior, and explicit diagnostics that map failures to user-recoverable actions.

Recommended delivery is incremental and dependency-aware. Start by locking a single solver backend behind a project-owned interface, encode legality/driving semantics in data (not UI), and enforce one sketch transaction API for UI + script mutations. After this foundation, add glyph/picking UX and script round-trip sync, then expand constraints and harden undo/redo and performance.

Primary risk is architectural drift: if constraints, solver triggers, script sync, and undo/redo evolve through separate mutation paths, v1.3 will become unstable and non-deterministic. Mitigation is to keep all sketch edits on one orchestrated pipeline, add replay/idempotence tests early, and gate milestone closure on diagnostics quality and dense-sketch interaction performance.

## Key Findings

### Recommended Stack (additions/changes)

- **Solver architecture change:** keep a **swappable solver interface in `src\solver\`**, but lock one backend in early implementation wave after acceptance spike.
- **Preferred backend profile:** lightweight C-first backend with permissive license; **Ceres (BSD-3) fallback** only if wrapped as a C++ island.
- **Scripting runtime:** lock **Lua 5.4.x** for deterministic embedded scripting and cross-platform viability.
- **UI/rendering:** reuse existing **Sokol + ImGui + pick buffer** stack; add in-house glyph batching/picking (no new UI framework).
- **Critical adoption checks:** Windows/MSVC+Vulkan build, permissive licensing, required constraint coverage, stable CMake packaging.

### Expected Features

**Must have (table stakes):**
- Sketch status model (`Solved/Loose/Fixed/Error`) with actionable reason reporting.
- Constraint legality filtering by selection context.
- Deterministic apply/remove/edit with undo/redo integrity.
- In-viewport constraint glyphs (constant size, hover/select).
- Editable length/angle dimensions in viewport and manager panel.
- Auto-solve toggle + manual solve + clear dirty-state signaling.
- Driven vs driving dimension semantics encoded and visible.
- Bidirectional UI/script sync without drift.

**Should have (differentiators):**
- In-context tab-driven constraint palette aligned with glyph icon language.
- Deterministic script editor round-tripping sketch sub-scenes.
- Dynamic IO panel from script-declared inputs/outputs.
- Constraint-to-geometry traceability highlights.

**Defer (v2+):**
- Full constraint catalog in first wave (ship MVP set first).
- Multi-sketch/global dependency solving.
- Open-ended scripting runtime features (modules/side effects/async).
- Exposing multiple solver backends in product UI.

### Architecture Approach (implementation waves/dependencies)

Core pattern: ECS owns state; focused sketch modules orchestrate legality, solve lifecycle, script sync, glyph rendering, and UI panels. All mutations must follow one contract:

`UI or Script event -> sketch transaction -> legality validation -> ECS mutation -> solve trigger -> status/log update -> render/pick refresh -> script sync`

**Wave 1 — Model + ownership foundation**
- Add `SketchComp`, `SketchRefComp`, `SketchGeomFlagsComp`, `ConstraintComp`, `SolverStateComp`, `ScriptComp`.
- Implement sketch registry/indexing.

**Wave 2 — Constraint legality + CRUD**
- Single-source legality matrix and mutation helpers.
- Encode driving/driven semantics in data layer.

**Wave 3 — Solver orchestration (single backend)**
- Dirty queue, deterministic solve/apply lifecycle, diagnostics mapping.
- Full-solve fallback path for incremental divergence.

**Wave 4 — UX integration**
- Glyph generation, pick routing/priority, inspector manager and dimension editing.

**Wave 5 — Script sync + transaction unification**
- Canonical serialization, parser, bidirectional sync guards, idempotence tests.
- Composite sketch transactions for undo/redo coherence.

**Wave 6 — Expansion + hardening**
- Constraint-set expansion beyond MVP.
- Dense-sketch perf closure and cross-target viability probes.

## Critical Pitfalls (top) + Mitigations

1. **Non-deterministic solver outcomes**  
   Mitigate with deterministic solve contract, golden replay tests, residual reporting.
2. **Incremental solve divergence**  
   Mitigate with connected-component dirty graph + full-solve fallback trigger.
3. **Weak over/under-constrained diagnostics**  
   Mitigate with structured diagnostics mapped to constraint/entity IDs in UI.
4. **Undo/redo transactional breakage**  
   Mitigate with composite sketch transactions and stable IDs decoupled from transient ordering.
5. **Script sync feedback loops**  
   Mitigate with change-origin tagging, canonical serialization, loop guards, idempotence tests.
6. **Glyph clutter/pick conflicts**  
   Mitigate with glyph LOD/filter modes, reserved pick ID ranges, enlarged hit proxies.

## Implications for Roadmap

### Suggested phase structure

### Phase 1: Deterministic Sketch Core
**Rationale:** All later features depend on trusted legality + transaction + solver contracts.  
**Delivers:** ECS sketch model, legality matrix, transaction API, single solver backend lock/spike.  
**Addresses:** Core table stakes (status, legality filtering, deterministic edits).  
**Avoids:** Non-determinism, divergence, weak diagnostics.

### Phase 2: Constraint UX + Dimension Editing
**Rationale:** Once solve core is stable, expose user-visible workflows safely.  
**Delivers:** Glyph rendering/picking, manager panel, in-context dimension edit, auto/manual solve controls.  
**Addresses:** Glyph/dimension table stakes and palette differentiator foundation.  
**Avoids:** Pick conflicts, partial state updates.

### Phase 3: Script Round-Trip + Undo/Redo Unification
**Rationale:** Script integration is high-value but risky without stable transaction foundation.  
**Delivers:** Lua-backed deterministic sketch script sync, origin-tagged updates, composite undo/redo transactions.  
**Addresses:** UI/script parity table stakes + script editor differentiator.  
**Avoids:** Sync loops, state drift, transactional breakage.

### Phase 4: Constraint Expansion + Hardening
**Rationale:** Expand coverage only after core reliability and sync behavior are proven.  
**Delivers:** Additional constraint types, dense-sketch performance tuning, closure on deferred platform viability checks.  
**Addresses:** Post-MVP capability depth.  
**Avoids:** Premature complexity and regression risk.

### Research flags

- **Needs deeper `/gsd-research-phase`:**
  - Phase 1 solver backend lock (final backend selection and exact capability/ABI validation).
  - Phase 4 expanded constraint math behavior/tolerances (degenerate geometry handling under load).
- **Likely standard patterns (can skip deeper research):**
  - Phase 2 glyph/panel UI composition using existing ImGui + pick-buffer patterns.
  - Base ECS component introduction and module boundary setup from architecture guidance.

### Recommended requirement categories for milestone scoping

1. **Determinism & correctness requirements** (solver stability, legality, diagnostics, replay parity).  
2. **User workflow requirements** (constraint authoring/edit/remove, dimension editing, status feedback).  
3. **Data model & transaction integrity requirements** (single mutation path, stable IDs, undo/redo atomicity).  
4. **Script interoperability requirements** (round-trip fidelity, idempotence, origin-tagged sync).  
5. **Interaction UX requirements** (glyph readability/selectability, pick priority, clutter controls).  
6. **Performance & scale requirements** (dense sketch latency budgets, solve cadence, viewport responsiveness).  
7. **Platform/build/packaging requirements** (Windows Vulkan gate, CMake integration, license compliance, future iOS/web viability).

## Confidence Assessment

| Area | Confidence | Notes |
|------|------------|-------|
| Stack | MEDIUM | Direction is clear; final solver backend still requires spike lock. |
| Features | HIGH | Strong agreement on MVP table stakes vs defer list. |
| Architecture | HIGH | Integration shape and dependency order are concrete and actionable. |
| Pitfalls | MEDIUM-HIGH | Risk catalog is comprehensive; mitigation quality depends on test discipline. |

**Overall confidence:** MEDIUM-HIGH

### Assumptions (explicit)

- v1.3 scope reuses and extends the researched v1.2 sketch/constraint/scripting foundation.
- Existing Sokol/ImGui/pick-buffer infrastructure remains intact and is preferred over UI stack changes.
- Milestone success prioritizes deterministic behavior and transaction integrity over broad first-pass constraint coverage.
- Lua runtime and chosen solver backend must not block later iOS/web targets.

### Gaps to Address

- Final solver backend choice is not yet conclusively locked (needs short acceptance spike).
- Quantified performance budgets for dense sketches are not yet formalized.
- Exact supported script grammar surface for “deterministic round-trip” needs milestone-level definition.

## Sources

### Primary
- `.planning/research/STACK.md`
- `.planning/research/FEATURES.md`
- `.planning/research/ARCHITECTURE.md`
- `.planning/research/PITFALLS.md`

---
*Research completed: 2026-04-07*  
*Ready for roadmap: yes*
