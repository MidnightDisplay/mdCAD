# Feature Research: v1.2 Sketches, Constraints, Scripting

**Domain:** Constraint-driven sketch workflows for mdCAD  
**Researched:** 2026-03-30  
**Confidence:** Medium-high

## Table stakes (must-have)

1. Sketch-level status (`Solved`, `Loose`, `Fixed`, `Error`) with actionable reason reporting.
2. Constraint legality filtering by selected entity/sub-entity types.
3. Deterministic apply/remove/edit constraint workflow with undo/redo integrity.
4. In-viewport, constant-size, hoverable/selectable constraint glyphs.
5. Length/Angle dimensional constraints editable in context and mirrored in manager panel.
6. Auto-solve toggle + manual solve button + clear dirty-state signaling.
7. Driven vs driving dimensional semantics.
8. Bidirectional UI/script synchronization that avoids state drift.

## Differentiators (high value)

1. Tab-driven in-context constraint palette with icons matching glyph language.
2. Script editor that round-trips sketch sub-scenes deterministically.
3. Dynamic IO parameter panel from script-declared inputs/outputs.
4. Constraint-to-geometry traceability highlights for debugging complex sketches.

## MVP scope recommendation

Start with points, lines, arcs/circles and a constrained first set:

- FIXED
- COINCIDENT
- PARALLEL
- PERPENDICULAR
- LENGTH
- ANGLE

Then expand into COLLINEAR/CONCENTRIC/TANGENTIAL/CORADIAL and axis constraints.

## Anti-features for this milestone

- Shipping all listed constraints in first implementation wave.
- Multi-sketch/global dependency solving.
- Free-form scripting runtime features (modules, IO side effects, async).
- Multiple solver backends exposed in UI.

## Acceptance-oriented workflow targets

1. User applies valid constraints via Tab menu without invalid options shown.
2. Applying/removing/editing constraints updates viewport, manager list, and solver state coherently.
3. Dimensional edits commit/cancel predictably and survive undo/redo.
4. Script changes can reconstruct the same supported sketch model and vice versa.

