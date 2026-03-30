# Phase 10 Sketch Managers Smoke Checklist (SKCH-01 / SKCH-02 / SKCH-03)

## Purpose
Repeatable manual verification checklist for Phase 10 sketch manager behavior with emphasis on SKCH-03 multi-select bulk actions and one-step undo semantics (D-05).

---

## Setup

1. Build app:
   - `cmake --build build-vulkan --config Release --target mdCAD`
2. Launch:
   - `build-vulkan/bin/Release/mdCAD.exe`
3. Create a new scene state (or clear existing entities) for clean verification.

---

## SKCH-01 — Sketch Creation & Attach Routing

1. Open **Scene Hierarchy** panel.
2. Use **Add Entity → Create Sketch**.
3. Confirm a sketch entity appears and can be set active.
4. Add geometry via **Add Entity** (Point, Line, Arc/Circle) while sketch is active.
5. Confirm new geometry is attached under the active sketch and sketch counts update.

Expected:
- Sketch is creatable from Add Entity flow.
- Geometry routes to active sketch ownership.
- Sketch metadata surfaces update immediately after add.

---

## SKCH-02 — Inspector Sketch Surfaces

1. Select the sketch entity in the hierarchy.
2. Open **Entity Inspector → SketchManager** section.
3. Verify surfaces:
   - `Status: solved|loose|fixed|error`
   - `Fixed-state: X/Y geometry fixed`
   - `Geometry count`
   - `Constraint count`
   - Color policy text

Expected:
- All required sketch-level status/count/copy surfaces are visible.
- Labels use Phase 10 taxonomy and update after model changes.

---

## SKCH-03 — GeometryManager Flat List + Multi-Select Bulk Actions

### A) Flat list rows and selection behavior
1. With sketch selected, locate **GeometryManager** in Entity Inspector.
2. Confirm rows render as a **flat list** (no tree/group nesting).
3. Confirm each row shows:
   - geometry type
   - name (or entity ID fallback)
   - fixed/loose status label
4. Selection checks:
   - Click one row → single select
   - Ctrl+click additional rows → toggle multi-select
   - Shift+click additional rows → additive multi-select

Expected:
- Flat rows with type/name/fixed-state metadata.
- Single + multi-select behavior matches ctrl/shift conventions.

### B) Bulk Fix / Unfix one-step undo
1. Multi-select 2+ geometry rows.
2. Click **Fix**.
3. Verify selected rows switch to `fixed` and sketch fixed count updates immediately.
4. Trigger **Undo** once.
5. Verify all affected rows revert together in one step.
6. Repeat with **Unfix** and one **Undo**.

Expected:
- Fix/Unfix act on full selection.
- Each action produces exactly one undo entry (`Bulk Fix/Unfix`).
- One Undo reverses the full set together (not per-entity).

### C) Bulk Delete confirmation + one-step undo
1. Multi-select 2+ geometry rows.
2. Click **Delete**.
3. Verify confirmation text:
   - `Delete {N} selected geometry items from this sketch? This will be one undo step.`
4. Confirm delete.
5. Verify rows are removed immediately and counts refresh.
6. Trigger **Undo** once.
7. Verify all deleted rows restore together in one step.

Expected:
- Delete uses destructive confirmation copy from UI contract.
- Deletion updates list state immediately.
- One Undo restores all deleted entities atomically (`Bulk Delete`).

---

## D-05 Explicit Undo Entry Check

After each bulk action (Fix, Unfix, Delete):
1. Observe Undo label/menu text (or undo stack display if present).
2. Confirm single action creates one stack entry (`Bulk Fix/Unfix` or `Bulk Delete`).
3. Confirm no per-entity sequence is required to fully revert.

---

## Out-of-Scope Reminders (Do Not Fail Phase 10)

- Phase 10 fixed-state is manager/visual semantics only (no gizmo transform lock enforcement yet).
- Constraint authoring/editing workflows belong to Phase 11.
- Full solver enforcement/diagnostics belong to Phase 12+.

---

## Result Log

- Date:
- Tester:
- Build hash:
- PASS / FAIL:
- If FAIL, list failing step number(s) and observed behavior:
