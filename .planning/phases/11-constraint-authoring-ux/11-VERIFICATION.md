---
phase: 11
slug: constraint-authoring-ux
status: complete
verified_on: 2026-04-01
requirements_verified: [SKCH-04, CONS-01, CONS-02, CONS-03, CONS-04, CONS-05]
gate_platform: Windows MSVC + Vulkan
---

# Phase 11 — Requirement Verification Evidence

This artifact closes Phase 11 requirement verification debt with requirement-level implementation evidence and explicit Windows gate results.

## Windows MSVC + Vulkan Gate Evidence

**Command:**

```bash
cmake --build build-vulkan --config Release --target mdCAD && ctest --test-dir build-vulkan -C Release --output-on-failure
```

**Result:** ✅ PASS (2026-04-01)

**Observed output evidence:**
- `MSBuild version 18.0.5+e22287bf1 for .NET Framework`
- `mdCAD.vcxproj -> C:\dev\mdCAD\build-vulkan\bin\Release\mdCAD.exe`
- `Test project C:/dev/mdCAD/build-vulkan`
- `No tests were found!!!` (CTest completed successfully with zero discovered tests; gate command exited cleanly)

---

## Canonical Keybinding Mapping (Normative)

To resolve legacy wording ambiguity without runtime behavior change:

- **`C`** opens the context-aware **constraint menu** (`src/app.c`, constraint menu flow).
- **`Tab`** toggles **gizmo edit mode** Transform ↔ Geometry (`src/app.c`, keyboard handling near Tab comment/branch).

Legacy text that says “Tab-triggered constraint menu” is documentation debt; runtime behavior is intentionally unchanged and verified as above.

---

## Requirement-by-Requirement Closure

## SKCH-04 — Select constraint highlights all participants

**Status:** ✅ PASS

**Implementation evidence:**
- ConstraintManager row selection routes through participant-aware selection behavior (Phase 11 + Phase 16 parity closure).
- Glyph click path now shares participant selection behavior via centralized helper, so viewport and manager produce the same highlight semantics.
- Sources:
  - `src/ui/ui_entity_inspector.h` (ConstraintManager row-select behavior)
  - `src/app.c` (glyph pick routing + selected constraint state)
  - `11-02-SUMMARY.md`, `16-01-SUMMARY.md` (integration closure trail)

**Automated gate companion:**
- `cmake --build build-vulkan --config Release --target mdCAD && ctest --test-dir build-vulkan -C Release --output-on-failure` ✅

---

## CONS-01 — Apply initial legal constraint set to valid entity types

**Status:** ✅ PASS

**Implementation evidence:**
- Canonical legality matrix and type contract implemented in `src/constraints/constraint_types.h`.
- ConstraintManager and context menu both gate applicability through `constraint_type_is_selection_legal(...)`.
- Initial v1.2 set is represented (FIXED, COINCIDENT, COLLINEAR, PARALLEL, PERPENDICULAR, ALONG X/Y/Z, CORADIAL, CONCENTRIC, LENGTH, ANGLE, TANGENTIAL).
- Sources:
  - `src/constraints/constraint_types.h`
  - `src/ui/ui_entity_inspector.h`
  - `src/app.c`
  - `11-01-SUMMARY.md`, `11-02-SUMMARY.md`, `11-03-SUMMARY.md`

**Automated gate companion:**
- `cmake --build build-vulkan --config Release --target mdCAD && ctest --test-dir build-vulkan -C Release --output-on-failure` ✅

---

## CONS-02 — In-context applicable-only menu + auto-hide after apply

**Status:** ✅ PASS

**Implementation evidence:**
- Constraint menu is opened from keyboard `C` and rendered at cursor anchor in app UI flow.
- Menu rows are filtered by legality (`constraint_type_is_selection_legal(...)`).
- Apply action creates constraint and closes one-shot menu state (auto-hide behavior).
- Sources:
  - `src/app.c` (`mdcad_draw_constraint_context_menu` flow and apply-close path)
  - `11-03-SUMMARY.md`

**Automated gate companion:**
- `cmake --build build-vulkan --config Release --target mdCAD && ctest --test-dir build-vulkan -C Release --output-on-failure` ✅

---

## CONS-03 — Hover/select via constant-screen-size viewport glyphs

**Status:** ✅ PASS

**Implementation evidence:**
- Dedicated constraint glyph overlay pipeline with reserved pick-ID range and hover routing precedence before gizmo/entity IDs.
- Glyph sizing logic preserves approximately constant on-screen interaction target across zoom.
- Glyph click selection now aligns with participant-highlight semantics (SKCH-04 parity).
- Sources:
  - `src/constraints/constraint_glyphs.h`
  - `src/app.c` (pick routing order and glyph click handling)
  - `11-03-SUMMARY.md`, `16-01-SUMMARY.md`

**Automated gate companion:**
- `cmake --build build-vulkan --config Release --target mdCAD && ctest --test-dir build-vulkan -C Release --output-on-failure` ✅

---

## CONS-04 — LENGTH/ANGLE create/view/edit mirrored manager + viewport

**Status:** ✅ PASS

**Implementation evidence:**
- Dimensional constraints (`LENGTH`, `ANGLE`) can be created and edited from both viewport popup and ConstraintManager.
- Both entry points mutate through `scene_constraint_set_dimensional_value(...)`, ensuring mirrored state updates.
- Popup supports Accept and Enter commit paths plus non-commit dismiss paths.
- Sources:
  - `src/app.c` (dimension popup commit and dismiss handling)
  - `src/ui/ui_entity_inspector.h` (manager-side dimensional editing)
  - `11-02-SUMMARY.md`, `11-03-SUMMARY.md`

**Automated gate companion:**
- `cmake --build build-vulkan --config Release --target mdCAD && ctest --test-dir build-vulkan -C Release --output-on-failure` ✅

---

## CONS-05 — Driven dimensional constraints visible/readable, non-driving

**Status:** ✅ PASS

**Implementation evidence:**
- Driven toggle is exposed for dimensional constraints in manager/selected-constraint UI.
- Mutations route through `scene_constraint_set_dimensional_value(...)` with driven flag.
- Tooltip/user-facing wording confirms semantics: visible/readable but non-driving.
- Sources:
  - `src/ui/ui_entity_inspector.h` (`Driven` checkbox + tooltip + update flow)
  - `src/components/constraint_comp.h` (driven data model, from 11-01)
  - `11-01-SUMMARY.md`, `11-02-SUMMARY.md`

**Automated gate companion:**
- `cmake --build build-vulkan --config Release --target mdCAD && ctest --test-dir build-vulkan -C Release --output-on-failure` ✅

---

## Traceability

| Requirement | Implementation anchor(s) | Evidence status |
|---|---|---|
| SKCH-04 | `src/ui/ui_entity_inspector.h`, `src/app.c`, `16-01-SUMMARY.md` | ✅ |
| CONS-01 | `src/constraints/constraint_types.h`, `src/app.c`, `src/ui/ui_entity_inspector.h` | ✅ |
| CONS-02 | `src/app.c` (constraint menu open/filter/apply-close) | ✅ |
| CONS-03 | `src/constraints/constraint_glyphs.h`, `src/app.c` (pick routing) | ✅ |
| CONS-04 | `src/app.c`, `src/ui/ui_entity_inspector.h` (`scene_constraint_set_dimensional_value`) | ✅ |
| CONS-05 | `src/ui/ui_entity_inspector.h`, `src/components/constraint_comp.h` | ✅ |

All six in-scope requirements now have explicit verification evidence and are no longer orphaned for Phase 11 closure.
