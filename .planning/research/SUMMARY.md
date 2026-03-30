# Research Summary: v1.2 Sketches, Constraints, Scripting

**Date:** 2026-03-30  
**Scope:** Milestone research for `docs/feature-proposal/Sketches, Constraints, Scripting.md`

## Stack additions

1. Add a project-owned solver abstraction and lock one backend via an implementation spike.
2. Adopt Lua 5.4.x as the scripting runtime baseline.
3. Reuse existing Sokol/cimgui/pick-buffer stack for constraint glyph rendering and interaction.

## Feature table stakes

1. Legal constraint filtering by selection type.
2. Reliable apply/remove/edit + undo transaction behavior.
3. Clear solve-state and solver diagnostics.
4. Viewport glyph interaction and dimensional editing.
5. Auto/manual solve modes with dirty-state signaling.
6. Bidirectional script synchronization with deterministic round-trip.

## Architecture direction

1. Add sketch/constraint/script component families to ECS.
2. Route all mutations through one sketch transaction pipeline.
3. Keep solver/script/glyph behavior in dedicated modules, integrated in existing app frame flow.
4. Prioritize build order: model -> constraints -> solver -> glyphs -> scripting -> undo hardening.

## Watch outs

1. Solver non-determinism and incremental divergence.
2. Poor over/under-constrained diagnostics.
3. Glyph clutter and pick priority conflicts.
4. Undo/redo partial restores across UI/solver/script boundaries.
5. Script sync loops and non-canonical serialization.
6. Dependency lock choices that block future iOS/web expansion.

## Milestone recommendation

Start v1.2 with an MVP constraint subset (FIXED, COINCIDENT, PARALLEL, PERPENDICULAR, LENGTH, ANGLE), one solver backend, and deterministic script round-trip guarantees. Expand constraints and advanced behavior after core interaction and transaction integrity are proven.

