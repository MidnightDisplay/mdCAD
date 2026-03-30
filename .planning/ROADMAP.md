# Roadmap: mdCAD

## Milestones

- ✅ **v1.0 Math Migration** — Phases 1-5 shipped 2026-03-26 ([archive](milestones/v1.0-ROADMAP.md))
- 🚧 **v1.1 Long-Tail Migration** — Phases 6-9 (planned)

## Overview

v1.1 executes the deferred long-tail migration and thin-entrypoint reduction work now that v1.0 hotspot/native gates are closed. The milestone focuses on serializer/importer/undo/editor utility migration plus hardening of reduced glue boundaries, while deferring iOS/web validation to the next cycle.

## Phases

**Phase Numbering:**
- Integer phases continue from previous milestone numbering.
- Decimal phases (e.g. 6.1) may be inserted for urgent work.

- [x] **Phase 6: Serializer and Save/Load Long-Tail Migration** — Migrate scene serialization math paths to cglm-backed helpers and preserve behavior. (completed 2026-03-27)
- [x] **Phase 7: Import Pipeline Long-Tail Migration** — Migrate JSONL/PLY importer math paths to cglm-backed helpers and preserve behavior. (completed 2026-03-27)
- [x] **Phase 8: Undo/Editor Utility Migration and Glue Burn-Down** — Migrate undo/editor utility paths and remove safe temporary thin-entrypoint glue. (completed 2026-03-28)
- [ ] **Phase 9: Long-Tail Validation, Performance Gates, and Boundary Finalization** — Validate long-tail parity/perf and finalize minimal thin-entrypoint boundary docs.

## Phase Details

### Phase 6: Serializer and Save/Load Long-Tail Migration
**Goal**: Ensure save/load workflows use migrated math paths without regression or hidden legacy-helper fallback.
**Depends on**: Phase 5
**Requirements**: TAIL-01
**Canonical refs**: `src/scene_serializer.h`, `src/math/`, `.planning/REQUIREMENTS.md`, `docs/QUICKSTART.md`
**Success Criteria** (what must be TRUE):
  1. Scene save/load paths rely on cglm-backed math helpers for migrated concerns.
  2. Save/load behavior remains parity-safe for representative scene content.
  3. No newly migrated serializer path depends on deprecated `math3d` helper equivalents.
**Plans**: 5 plans

Plans:
- [x] 06-01: Map and migrate serializer math touchpoints to cglm-backed helpers
- [x] 06-02: Add/extend harness or targeted checks for serializer parity-sensitive cases
- [x] 06-03: Validate native save/load workflows and document evidence

### Phase 7: Import Pipeline Long-Tail Migration
**Goal**: Migrate importer math paths (JSONL/PLY) to cglm-backed helpers while preserving user-visible import behavior.
**Depends on**: Phase 6
**Requirements**: TAIL-02
**Canonical refs**: `src/jsonl_loader.h`, `src/jsonl_import_job.h`, `src/ply_loader.h`, `src/ply_import_job.h`, `src/ply_mesh_import_job.h`, `src/math/`
**Success Criteria** (what must be TRUE):
  1. JSONL/PLY import math operations in scope use cglm-backed paths.
  2. Imported geometry behavior (orientation/scale/placement) remains parity-safe.
  3. Import flows avoid reintroducing migrated deprecated `math3d` helper dependencies.
**Plans**: 5 plans

Plans:
- [x] 07-01-PLAN.md — Migrate importer math touchpoints and update boundaries
- [x] 07-02-PLAN.md — Add parity checks for importer geometry transforms and placement
- [x] 07-03-PLAN.md — Validate native import workflows and capture evidence

### Phase 8: Undo/Editor Utility Migration and Glue Burn-Down
**Goal**: Complete undo/editor utility migration and remove temporary glue that is no longer required.
**Depends on**: Phase 7
**Requirements**: TAIL-03, TRED-01
**Canonical refs**: `src/undo_redo.h`, `src/undo_redo_exec.h`, `src/ui/`, `src/math/`, `src/math3d.h`
**Success Criteria** (what must be TRUE):
  1. Undo/redo and editor utility math paths in scope use cglm-backed helpers.
  2. Temporary migration glue removed in this phase has no required runtime consumers.
  3. User-visible edit/undo behavior remains parity-safe after glue reduction.
**Plans**: 3 plans

Plans:
- [x] 08-01: Migrate undo/editor utility math touchpoints to cglm-backed helpers
- [x] 08-02: Remove safe thin-entrypoint glue and update call-site boundaries
- [x] 08-03: Validate edit/undo parity and document migration deltas

### Phase 9: Long-Tail Validation, Performance Gates, and Boundary Finalization
**Goal**: Close v1.1 with expanded parity/performance confidence and finalized minimal thin-entrypoint boundary documentation.
**Depends on**: Phase 8
**Requirements**: TRED-02, VAL-01, VAL-02, VAL-03
**Canonical refs**: `src/math_harness.c`, `docs/QUICKSTART.md`, `.planning/STATE.md`, `.planning/REQUIREMENTS.md`, `.planning/PROJECT.md`
**Success Criteria** (what must be TRUE):
  1. Long-tail compare/harness checks pass with strict parity coverage.
  2. macOS and Windows performance gates show no regressions for expanded migrated surfaces.
  3. Manual smoke checks for serializer/import/undo/editor workflows pass on required native targets.
  4. Remaining thin-entrypoint surface is minimal, documented, and intentionally retained.
**Plans**: 3 plans

Plans:
- [x] 09-01: Expand/verify long-tail compare and harness coverage
- [x] 09-02: Execute native benchmark/perf gates for expanded migration slice
- [x] 09-03: Run manual smoke workflows and finalize boundary docs
- [x] 09-04-PLAN.md — Gap closure for VAL-02 perf blockers (Windows rerun policy + macOS native candidate capture)
- [ ] 09-05-PLAN.md — Gap closure for VAL-03 blockers (LT-VAL03-02 import regression + macOS manual rows)

## Progress

**Execution Order:**
Phases execute in numeric order: 6 → 7 → 8 → 9

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 6. Serializer and Save/Load Long-Tail Migration | 3/3 | Complete | 2026-03-27 |
| 7. Import Pipeline Long-Tail Migration | 3/3 | Complete | 2026-03-27 |
| 8. Undo/Editor Utility Migration and Glue Burn-Down | 3/3 | Complete    | 2026-03-28 |
| 9. Long-Tail Validation, Performance Gates, and Boundary Finalization | 3/5 | Blocked |  |
