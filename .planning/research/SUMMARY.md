# Project Research Summary

**Project:** mdCAD v1.6 — Observable Flat JSONL Import for Large Geometry Dumps  
**Domain:** High-volume CAD geometry ingestion with optional live file refresh  
**Researched:** 2026-04-27  
**Confidence:** HIGH

## Executive Summary

v1.6 is a performance-and-reliability milestone: import very large flat JSONL geometry dumps into scene entities (not sketches), and optionally keep that import linked to source for observable refresh. The winning pattern is additive, not disruptive: keep the existing JSONL import flow, add a flat observer path that is opt-in and root-anchor scoped, and preserve current defaults for non-observed imports.

The recommended implementation is to stay on the current stack (C11 + Flecs + cJSON + existing polling observer primitives), add no new third-party dependencies, and build a dedicated flat refresh contract with transactional replace semantics. This avoids scope and regression risk while directly targeting large-file stability and operator trust.

Top risks are accidental reuse of sketch paths, full-file memory-heavy reparsing, non-transactional partial updates, and observer thrash during in-progress writes. Mitigate with strict flat-mode boundaries, chunked/streaming-friendly pipeline work, atomic commit/rollback refresh, and debounce/retry/stability-window policy.

## Key Findings

### Recommended Stack (v1.6 decisions)

- **Keep current core stack:** C11 modules, Flecs 4.1.4, cJSON 1.7.19, existing import math/helpers.
- **No new external libs:** no new watcher framework, no parser swap, no threaded importer for this milestone.
- **Add internal modules/capabilities:**
  - Flat import execution path optimized for large files.
  - Flat observer component/system (or explicit mode flag) attached to import root anchor.
  - Loader/path updates to reduce mandatory double-pass and full-retention overhead.
- **Critical behavior requirement:** transactional refresh (stage -> commit swap on success -> rollback on failure).

### Expected Features (milestone-scoped)

**Must-have / table stakes**
- Dedicated menu entry/path for flat-large JSONL import.
- Dedicated flat import dialog with transform/color options.
- Import into a **single controllable anchor** with plain scene entities.
- Optional observer-link at import time (default OFF).
- Manual refresh action.
- Transactional refresh semantics (never partial mixed state).
- Stable behavior on very large geometry dumps.

**Should-have / differentiators**
- Flat refresh replaces anchor subtree only (no sketch/script pipeline).
- Safety-tuned observer behavior (debounce/retry/auto-disable).
- Clear operator status/messages on anchor.

**Defer (v2+)**
- Sketch/constraint integration for flat imports.
- Smart diff/patch refresh (full subtree replace acceptable in v1.6).
- Parser/library migration.

### Architecture Approach

- Additive architecture: preserve current import defaults, add optional flat observer path.
- Prefer a **new flat observer component/system** to isolate from sketch observer behavior.
- Keep root anchor identity stable; refresh replaces child subtree transactionally.
- Integrate at existing seams:
  - Import/UI: scene hierarchy import dialog + completion hook.
  - Runtime tick: app-level observer tick integration.
  - Persistence: component registration + serializer support.

### Major Pitfalls to Watch

1. **Sketch-path leakage into flat mode** — enforce non-sketch contract.  
2. **Full-memory reparsing on refresh** — use chunked/low-retention approach.  
3. **Mandatory double I/O pre-pass** — avoid hard line-count pre-scan for flat path.  
4. **Observer CPU thrash on churn** — rate-limit + quick metadata gate + stable-window logic.  
5. **Non-transactional refresh corruption** — enforce stage/commit/rollback invariant.  
6. **Selection/undo instability from entity churn** — define anchor-scoped invalidation and undo policy.

## Implications for Roadmap (implementation sequencing)

### Phase 1: Contract & Persistence Scaffold
**Rationale:** Locks behavior boundaries early and reduces regression risk.  
**Delivers:** Flat observer component schema, ECS registration, serialization keys, explicit non-sketch contract, perf/scale acceptance targets.  
**Covers features:** opt-in observer metadata foundation, anchor-scoped model.  
**Pitfalls addressed:** sketch-path leakage, ambiguous ownership, path/link brittleness.

### Phase 2: Flat Import UX + Root Attachment
**Rationale:** Users need explicit entry point and mode clarity before refresh engine work.  
**Delivers:** dedicated menu/dialog, options capture, single-anchor flat import completion hook, observer opt-in default OFF.  
**Covers features:** table-stakes import workflow and operator intent clarity.  
**Pitfalls addressed:** wrong-path usage, hierarchy bloat.

### Phase 3: Transactional Manual Refresh Engine
**Rationale:** Manual deterministic refresh validates correctness before automation.  
**Delivers:** stage/commit/rollback flat re-import into same root anchor; preserved last-good state on failures.  
**Covers features:** manual refresh, transactional semantics.  
**Pitfalls addressed:** partial corruption, entity/tree inconsistency.

### Phase 4: Automatic Observer Loop & Safety Policies
**Rationale:** Automation after correctness minimizes blast radius.  
**Delivers:** per-frame observer tick integration, debounce/retry/auto-disable, file-stable window policy, status messaging.  
**Covers features:** optional live refresh with operator trust signals.  
**Pitfalls addressed:** refresh thrash, CPU overuse, noisy failure loops.

### Phase 5: Scale Hardening & UX Stability
**Rationale:** v1.6 success is measured under large real-world loads.  
**Delivers:** large-fixture perf/soak/fault-injection tests; hierarchy/selection/undo behavior validation and tuning.  
**Covers features:** "large dump stable" table-stake promise.  
**Pitfalls addressed:** UI lag, counter overflow, selection/undo regressions.

### Phase Ordering Rationale

- Contract first prevents accidental architecture drift into sketch semantics.
- UX + attach next provides visible milestone value and integration certainty.
- Manual transactional refresh before auto-observe isolates correctness from scheduling complexity.
- Auto policies after core engine reduces noisy field failures.
- Final hardening ensures milestone claim ("large geometry dumps") is credibly met.

### Research Flags

**Likely needs deeper `/gsd-research-phase`:**
- Phase 3 (transactional subtree swap edge cases and ECS integrity under failure injection).
- Phase 5 (large-scale hierarchy/render/undo behavior under repeated refresh).

**Likely safe with standard patterns (can skip extra research):**
- Phase 1 (component registration/serialization scaffolding).
- Phase 2 (menu/dialog wiring and import option plumbing).

## Confidence Assessment

| Area | Confidence | Notes |
|------|------------|-------|
| Stack | HIGH | Repo-anchored; additive changes; no new external dependencies required. |
| Features | HIGH | Clear milestone-specific feature contract and anti-features. |
| Architecture | HIGH | Strong existing integration seams and proven observer transaction pattern to adapt. |
| Pitfalls | HIGH | Identified directly from current loader/import/observer/runtime behavior at scale. |

**Overall confidence:** HIGH

### Gaps to Address During Planning

- Exact performance targets (import time/frame budget/peak memory) must be numerically defined.
- Final decision on flat component shape (new component vs mode flag) should be fixed early.
- Undo semantics for observer-driven refresh must be explicitly specified (likely non-undoable auto updates).
- Large-file counter/type audit scope (int->size_t/64-bit) needs concrete checklist coverage.

## Sources

- `.planning/research/STACK.md`
- `.planning/research/FEATURES.md`
- `.planning/research/ARCHITECTURE.md`
- `.planning/research/PITFALLS.md`

---
*Research completed: 2026-04-27*  
*Ready for roadmap: yes*
