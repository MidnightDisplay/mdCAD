# Domain Pitfalls: v1.6 Observable Flat JSONL Import for Large Geometry Dumps

**Domain:** Flat observable JSONL import for large, frequently refreshed geometry dumps in mdCAD (C + ECS + GPU app)  
**Researched:** 2026-04-27  
**Confidence:** HIGH (repo-code anchored)

## Critical Pitfalls

### 1) Accidentally reusing sketch import/reparse path for flat mode
**What goes wrong:** Flat import inherits sketch/script overhead and loses the expected performance win.  
**Why it happens:** Existing observable path is sketch-centric (`jsonl_sketch_import_job.h`, script re-emit, sketch metadata).  
**Consequences:** Slow imports/reloads, unnecessary script churn, user sees no benefit vs v1.5 flow.  
**Prevention:** Define flat import as a separate contract: non-sketch entities under one anchor, no sketch/script participation.  
**Detection:** Perf test: same large dump imported as sketch vs flat; flat must be materially faster and lower memory.

**Handle in:** Requirements + Planning + Implementation + Testing

---

### 2) Full-file parse/load in memory on every refresh
**What goes wrong:** Refresh stalls or OOM on large files.  
**Why it happens:** Current loader stores full parsed model (`jsonl_loader.h` entry arrays) and sketch import is synchronous.  
**Consequences:** Frame hitching, memory spikes, crashes for very large dumps.  
**Prevention:** Stream/chunk parse + chunk entity apply for flat mode; avoid full in-memory scene model for refresh path.  
**Detection:** Stress test with very large JSONL; track frame time and peak RSS during refresh.

**Handle in:** Requirements + Planning + Implementation + Testing

---

### 3) Double I/O tax and blocking first-pass line counting
**What goes wrong:** Import time scales poorly before real parsing starts.  
**Why it happens:** `jsonl_open` does line-count pass then parse pass.  
**Consequences:** 2x disk/network read cost, especially painful for huge/remote files.  
**Prevention:** Flat path should use single-pass progress estimation (bytes consumed) or optional sampling, not mandatory pre-count.  
**Detection:** Profiling shows disproportionate time before first entities appear.

**Handle in:** Planning + Implementation + Testing

---

### 4) Expensive change detection for observed large files
**What goes wrong:** Observer tick burns CPU on hashing/metadata checks under frequent updates.  
**Why it happens:** FNV hash of full file and per-frame tick integration (`app.c` calls observer tick each frame).  
**Consequences:** Constant background load, UI jitter, battery drain.  
**Prevention:** Rate-limit checks, defer heavy hash to worker/chunk pass, use staged "size/mtime quick gate then async verify".  
**Detection:** Idle-with-observer profiling shows high CPU when file churns.

**Handle in:** Requirements + Planning + Implementation + Testing

---

### 5) Non-transactional refresh that leaves partial world state
**What goes wrong:** Failed refresh leaves half-updated entities/orphans.  
**Why it happens:** Flat mode may skip transactional discipline while optimizing for speed.  
**Consequences:** Corrupted scene tree, stale references, hard-to-recover UX.  
**Prevention:** Keep authoritative commit model: stage new set, commit swap only on success, rollback on failure.  
**Detection:** Fault-injection tests (truncated JSONL, locked file, OOM) must preserve last-good anchor contents.

**Handle in:** Requirements + Implementation + Testing

---

### 6) Entity ID churn breaks selection/gizmo/inspector continuity
**What goes wrong:** User selection or tooling points to dead entities after refresh.  
**Why it happens:** Authoritative replace recreates entities each cycle.  
**Consequences:** Interaction glitches, stale handles, perceived instability.  
**Prevention:** Explicit post-refresh invalidation/remap policy for selection/highlight/gizmo caches at anchor scope.  
**Detection:** Repeated refresh tests while entities are selected/dragged/open in inspector.

**Handle in:** Planning + Implementation + Testing

---

### 7) Undo/redo explosion from high-entity refreshes
**What goes wrong:** Massive undo snapshots or unusable history during auto-refresh.  
**Why it happens:** Treating observed refreshes like normal user edits.  
**Consequences:** Memory bloat, long pauses, confusing undo semantics.  
**Prevention:** Define refresh undo policy up front (typically non-undoable observer updates, with explicit user-triggered import checkpoints).  
**Detection:** Soak test with frequent file updates; inspect undo stack growth and latency.

**Handle in:** Requirements + Planning + Implementation + Testing

---

### 8) Hierarchy/UI rebuild cost dominates at scale
**What goes wrong:** Every refresh triggers expensive cache rebuild and tree redraw over huge entity sets.  
**Why it happens:** Scene hierarchy cache rebuild scans geometry + anchors globally.  
**Consequences:** UI lag even if parse/apply is optimized.  
**Prevention:** Anchor-scoped dirtying, incremental hierarchy updates, collapsed-by-default import anchors, virtualized listing if needed.  
**Detection:** Profiling around hierarchy rebuild with 100k+ imported entities and frequent refresh.

**Handle in:** Planning + Implementation + Testing

---

### 9) Refresh thrash on partial file writes
**What goes wrong:** Import repeatedly fails while producer is still writing; observer disables too aggressively or spams warnings.  
**Why it happens:** Polling sees intermediate file states; parser reads incomplete JSONL.  
**Consequences:** User confusion, missed updates, noisy error loop.  
**Prevention:** Introduce "file-stable window" (no size/mtime changes for N ms) before parse; keep retry semantics explicit.  
**Detection:** Simulated writer that appends in bursts; verify stable apply behavior and clear messaging.

**Handle in:** Planning + Implementation + Testing

---

### 10) Integer/counter overflow on very large dumps
**What goes wrong:** Wrong progress/counting/allocation behavior for extreme element counts.  
**Why it happens:** Many counters are `int`/32-bit style in current import structures.  
**Consequences:** Corrupted progress, out-of-bounds, crashes.  
**Prevention:** Audit large-count paths; move counts/capacities to `size_t`/64-bit where needed and guard conversions.  
**Detection:** Synthetic max-scale tests near boundary values.

**Handle in:** Requirements + Implementation + Testing

## Moderate Pitfalls

### A) Ambiguous ownership between manual import and observer refresh
**What goes wrong:** "Import once" and "live observed" paths diverge in behavior over time.  
**Prevention:** Single shared flat refresh engine used by both manual refresh and observer tick.

### B) Weak observability/diagnostics for operator trust
**What goes wrong:** Hard to know if refresh is current, failed, or stale.  
**Prevention:** Show last success time, source fingerprint, replaced count, last error.

### C) Path/link brittleness (rename/move/case differences)
**What goes wrong:** Anchor silently points to stale/missing source.  
**Prevention:** Normalize path handling and provide explicit relink affordance with clear status.

## Phase-Specific Warnings

| Phase Topic | Likely Pitfall | Mitigation |
|---|---|---|
| Requirements | "Flat" not explicitly excluding sketch/script semantics | Add hard non-sketch contract + perf targets |
| Planning | Reuse old sketch observer internals "for speed" | Split flat pipeline and list forbidden dependencies |
| Implementation | Synchronous parse/apply in UI path | Mandatory chunked job + cancellation/progress |
| Implementation | Non-atomic anchor replace | Stage/commit/rollback invariant tests |
| Testing | Only correctness tests, no scale tests | Add large-dump perf/soak/fault-injection gates |

## Sources (repo-internal)

- `src/jsonl_loader.h` (full-model parse behavior, line counting, counters)
- `src/jsonl_sketch_import_job.h` (sync parse + transactional sketch reparse model)
- `src/jsonl_observer_system.h` (observer tick/retry semantics)
- `src/app.c` (observer tick called each frame)
- `src/ui/ui_scene_hierarchy.h` (import UI flow and sync sketch import callsite)
- `src/tests/jsonl_reparse_transaction_test.c`
- `src/tests/jsonl_observer_state_test.c`
- `.planning/PROJECT.md` (v1.6 scope and intent)
- `.planning/phases/35-observable-jsonl-as-sketch-import-with-optional-live-file-observer/35-03-SUMMARY.md`
