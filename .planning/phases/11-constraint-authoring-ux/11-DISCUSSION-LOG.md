# Phase 11: Constraint Authoring UX - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md -- this log preserves the alternatives considered.

**Date:** 2026-03-31
**Phase:** 11-constraint-authoring-ux
**Areas discussed:** Tab conflict, Constraint data model, Glyph rendering, ConstraintManager, LENGTH/ANGLE editing, Constraint menu trigger

---

## Tab conflict resolution

| Option | Description | Selected |
|--------|-------------|----------|
| Contextual Tab | Tab shows constraint menu when sketch selected, gizmo toggle otherwise | |
| Replace gizmo toggle | Tab repurposed for constraint menu, gizmo moves to different key | |
| C key instead | Use C key for constraint menu; Tab unchanged | V |

**User's choice:** Use C key for constraint menu. Tab stays as gizmo toggle.
**Notes:** User asked to verify C is unused -- confirmed no C binding exists in app.c.

---

## Constraint data model

| Option | Description | Selected |
|--------|-------------|----------|
| ECS entities (A) | Each constraint = own ECS entity parented to sketch; ConstraintComp + back-reference on geometry | V |
| Embedded in SketchComp (B) | Constraints stored as dynamic array inside SketchComp | |

**User's choice:** A -- ECS entities parented to sketch.
**Notes:** User clarified: constraints and geometry are siblings under the sketch (not parented to each other). ConstraintComp holds participant geometry IDs. Geometry entities carry ConstraintParticipantComp for bidirectional lookup.

---

## Glyph rendering approach

| Option | Description | Selected |
|--------|-------------|----------|
| Pick buffer overlay (A) | Reuse gizmo's overlay system; own reserved pick ID range | V |
| ImGui DrawList overlay (B) | 2D shapes on viewport DrawList, no GPU pick | |
| Separate sokol render pass (C) | Most flexible, heaviest lift | |

**User's choice:** A -- pick buffer overlay.
**Notes:** User clarified symbol glyphs are screen-flat; dimension glyphs (LENGTH/ANGLE) are 3D-rotatable at constant screen size like gizmo.

---

## ConstraintManager inspector section

| Option | Description | Selected |
|--------|-------------|----------|
| Flat list + delete (A) | List constraints, select-to-highlight, delete action | |
| Flat list + type filter + substring search (B) | As A plus dropdown type filter and substring search | V |

**User's choice:** B plus auto-naming convention.
**Notes:** Every geometry entity, constraint entity, and sketch entity gets a LabelComp pre-filled with type_N naming (per-sketch sequential counter). Description pre-filled with parent sketch name. All editable. ECS IDs are true unique identifiers. Naming convention also applies retroactively to Phase 10 scene_add_* helpers.

---

## LENGTH/ANGLE editing workflow

| Option | Description | Selected |
|--------|-------------|----------|
| Floating ImGui popup (A) | Popup stack, auto-dismiss on Escape/outside click, InputFloat + Accept/Cancel | V |
| Inline viewport overlay (B) | Non-popup window, manual show/hide state | |
| Inspector mirror only (C) | No viewport popup, all editing in inspector | |

**User's choice:** A -- floating ImGui popup.
**Notes:** User asked for A vs B comparison before deciding. A chosen for simpler keyboard management.

---

## Constraint menu trigger position

| Option | Description | Selected |
|--------|-------------|----------|
| Fixed corner of viewport | Appears at top-left or near selected entity | |
| Centered in viewport | Fixed center position | |
| Mouse cursor position | Appears where C was pressed | V |

**User's choice:** C -- at mouse cursor position.

---

## the agent's Discretion

- Exact glyph geometry shapes per constraint type.
- Exact pick ID range allocation for constraints.
- Per-sketch label sequence storage approach.
- ConstraintManager row layout details.

## Deferred Ideas

- Solver execution backend (Phase 12).
- Auto-solve, solver diagnostics (Phase 12).
- Constraint-respecting gizmo transforms (Phase 12).
- Script editor and round-trip (Phase 13+).
