# Architecture Research: v1.2 Sketches, Constraints, Scripting

**Domain:** Integrating constrained sketches into existing mdCAD ECS/runtime  
**Researched:** 2026-03-30  
**Confidence:** High for integration shape, medium for solver backend specifics

## Recommended integration shape

Use ECS for state ownership, and add focused orchestration modules for solver/script/glyph behavior.

### New component families (proposed)

1. `SketchComp` (status, autosolve, color, versioning)
2. `SketchRefComp` (geometry belongs-to sketch mapping)
3. `SketchGeomFlagsComp` (fixed/loose metadata)
4. `ConstraintComp` (type, participants, value, driven flag)
5. `SolverStateComp` (dirty, last solve result, diagnostics summary)
6. `ScriptComp` (source text/hash + sync metadata)

### New module boundaries (proposed)

1. `src\sketch\sketch_registry.h` — ownership and indexing helpers
2. `src\sketch\sketch_constraints.h` — legality matrix + mutation helpers
3. `src\sketch\sketch_solver_orchestrator.h` — dirty-queue solve/apply lifecycle
4. `src\sketch\sketch_script_sync.h` — bidirectional scene<->script synchronization
5. `src\sketch\sketch_glyphs.h` — viewport glyph generation + picking integration
6. `src\ui\ui_sketch_inspector.h` — manager panels in Entity Inspector
7. `src\ui\ui_sketch_script_editor.h` — standalone script editor + IO panel

## Data-flow contract

All sketch mutations should flow through one transactional path:

`UI or Script event -> sketch transaction -> legality validation -> ECS mutation -> solve trigger -> status/log update -> render/pick refresh -> script sync`

This is critical for undo/redo coherence and avoiding script feedback loops.

## Build order recommendation

1. Data model + ownership components.
2. Constraint CRUD + legality matrix.
3. Solver orchestration with one backend.
4. Glyph rendering/picking integration.
5. Script serialization + parser + sync.
6. Undo/redo unification for sketch transactions.
7. Hardening and performance validation.

## Refactor guidance

Aggressive refactor is justified for:

- undo/redo transaction coherence
- centralized pick routing for glyph/geometry/gizmo priority
- clear module boundaries around sketch subsystems

Avoid destabilizing refactors to unrelated rendering or platform bootstrap paths in same wave.

