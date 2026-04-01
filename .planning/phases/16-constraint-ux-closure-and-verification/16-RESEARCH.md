# Phase 16: Constraint UX Closure & Verification - Research

**Researched:** 2026-04-01  
**Domain:** mdCAD constraint UX gap-closure + verification evidence hardening  
**Confidence:** HIGH

## User Constraints

No `16-CONTEXT.md` exists yet in `.planning/phases/16-constraint-ux-closure-and-verification/`.  
Scope is therefore constrained by ROADMAP + REQUIREMENTS + milestone audit findings only.

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| SKCH-04 | User can select a constraint and see all participating geometry entities/sub-entities highlighted. | Root cause isolated to glyph click path in `src/app.c`; prescriptive fix is to route glyph click selection through participant selection logic (same behavior as ConstraintManager row select). |
| CONS-01 | Apply initial constraint set to legal entity types. | Keep centralized legality/type contracts in `src/constraints/constraint_types.h`; re-verify via context menu legality filter + manager workflows. |
| CONS-02 | Tab-triggered in-context menu, applicable-only, auto-hide after apply. | Menu flow exists in `mdcad_draw_constraint_context_menu`; verification artifact must prove trigger/filter/auto-hide behavior. |
| CONS-03 | Hover/select constraints via constant-screen-size viewport glyphs. | Glyph pipeline exists (`constraint_glyphs.h` + pick buffer routing in `app.c`); closure work must prove hover/select and consistent participant highlight on glyph select. |
| CONS-04 | LENGTH/ANGLE create/view/edit mirrored across viewport and manager. | Both popup and manager call `scene_constraint_set_dimensional_value`; evidence should verify mirroring and Accept/Cancel behaviors. |
| CONS-05 | LENGTH/ANGLE can be driven (visible/readable, non-driving). | Driven flag flows through manager and selected-constraint sections; verification must include driven toggle persistence/display checks. |

</phase_requirements>

## Summary

Phase 16 is a **targeted closure phase**, not a feature exploration phase. The codebase already contains most of Phase 11 UX mechanics (constraint legality matrix, ConstraintManager workflows, C-key menu, glyph overlay picking, dimensional popup editing), but the milestone audit found two blocking gaps: (1) **selection-flow inconsistency** on glyph click and (2) **missing/partial evidence artifacts** (`11-VERIFICATION.md` missing, `11-VALIDATION.md` still draft).

The integration failure is explicit in current code: glyph click in `src/app.c` sets `selection_set_single(clicked_constraint)` and comments that participant highlighting is handled elsewhere. This diverges from manager-row behavior (which clears selection then adds all participants), so SKCH-04 / CONS-03 can fail depending on entry path. The closure plan should centralize selection behavior for constraints so both manager and glyph paths produce identical participant highlighting.

Windows MSVC + Vulkan acceptance evidence should be treated as the gate for this closure: every task should preserve `mdCAD` Release build on `build-vulkan`, and final verification should include deterministic manual walkthrough evidence for SKCH-04 + CONS-01..05 and command output evidence for build/test commands.

**Primary recommendation:** Implement one shared “select constraint participants” path and then produce complete Phase 11 verification/validation artifacts anchored to Windows MSVC+Vulkan run evidence.

## Project Constraints (from copilot-instructions.md)

`./copilot-instructions.md` was not found in repository root at research time, so no additional project-specific constraints were extracted.

## Standard Stack

### Core
| Library / System | Version | Purpose | Why Standard |
|---|---:|---|---|
| Existing mdCAD ECS/UI stack (flecs + cimgui + sokol + pick buffer + scene helpers) | repo-pinned | Constraint UX behavior and verification target are already implemented here | Phase 16 is gap-closure; introducing new frameworks increases regression risk. |
| cglm (locked backend) | 0.9.6 | Math backend for transforms/projection interactions | Locked in `.planning/STATE.md`; planning must not change backend. |
| CMake + CTest + build-vulkan VS solution | CMake/CTest 4.3.0 (env) | Build and validation commands for gate evidence | Existing Phase 10/11 validation flow already uses these commands. |

### Supporting
| Library / Tool | Version | Purpose | When to Use |
|---|---:|---|---|
| `src/constraints/constraint_types.h` | in-repo | Canonical legality/type/display contracts | Always for constraint applicability checks and type naming. |
| `src/constraints/constraint_glyphs.h` | in-repo | Glyph ID mapping, hover/selection mapping, constant-screen-size overlay logic | Always for viewport glyph interactions and pick routing validation. |
| `src/selection.h` | in-repo | Selection tag and highlight propagation | Always for participant highlighting behavior. |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|---|---|---|
| Reusing existing selection + scene helpers | New ad-hoc glyph-only highlight state | Creates divergent behaviors and repeats the bug pattern already identified. |
| Completing verification artifacts now | Deferring to Phase 15 | Leaves orphaned requirements and blocks milestone readiness tracking. |

**Installation:** No new packages required for Phase 16.

## Architecture Patterns

### Recommended Project Structure
```text
src/
├── app.c                         # viewport input routing + constraint menu/popup + glyph click handling
├── constraints/
│   ├── constraint_types.h        # legality matrix and type contracts
│   └── constraint_glyphs.h       # glyph pick IDs, anchor projection, overlay draw helpers
├── ui/
│   └── ui_entity_inspector.h     # ConstraintManager participant selection + dimensional/driven edits
└── ecs/
    └── ecs_scene.h               # scene-level constraint create/remove/value-update helpers
```

### Pattern 1: Single Source of Truth for Constraint Selection Highlight
**What:** Any “constraint selected” action must route through one helper that selects all participant geometry entities.  
**When to use:** Glyph clicks, manager row clicks, selected-constraint inspector actions.  
**Example:**
```c
// Existing manager behavior pattern (ui_entity_inspector.h)
selection_clear(state->selection);
for (uint32_t pi = 0; pi < constraint->participant_count; pi++) {
    ecs_entity_t p = (ecs_entity_t)constraint->participants[pi];
    if (ecs_is_alive(w->world, p)) selection_add(state->selection, p);
}
```

### Pattern 2: Scene-Centric Constraint Mutation
**What:** Mutate dimensional/driven state only via `scene_constraint_set_dimensional_value`.  
**When to use:** ConstraintManager value edits, popup Accept/Enter, driven toggles.  
**Example:**
```c
scene_constraint_set_dimensional_value(
    &state.ecs_scene, constraint_entity, new_value, driven_flag);
```

### Pattern 3: Ordered Pick Routing
**What:** Resolve hovered/clicked pick IDs in this order: constraint glyphs → gizmo IDs → normal entities.  
**When to use:** Per-frame viewport pick handling.  
**Example:** `src/app.c` routes glyph IDs before gizmo/entity IDs (lines ~1011-1024).

### Anti-Patterns to Avoid
- **Glyph-only selection semantics:** Selecting the constraint entity itself (without participants) breaks SKCH-04 intent.
- **Duplicated legality logic in UI path:** bypassing `constraint_type_is_selection_legal` risks manager/menu drift.
- **Direct component writes for dimensional state:** bypasses rounding/decimal policy in scene helper.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---|---|---|---|
| Constraint applicability filtering | Per-window custom legality matrix | `constraint_type_is_selection_legal` in `constraint_types.h` | Keeps CONS-01/02 consistency across manager/menu. |
| Selection highlight propagation | Ad-hoc per-view highlight flags | `selection_clear/add/set_single` + ECS Selected tags | Existing renderer already keys highlight color off Selected tag. |
| Constraint participant link integrity | Manual unlink bookkeeping in UI | `scene_add_constraint_to_sketch` / `scene_remove_constraint` / `scene_constraint_remove_participant` | Avoids stale links and hidden data corruption. |
| Glyph pick-ID mapping | New ID registry | Reserved range in `selectable_comp.h` + `constraint_glyphs_*` helpers | Avoids collisions with gizmo/entity IDs. |

**Key insight:** Phase 16 should remove divergence, not add mechanisms. Reuse and unify existing pathways.

## Common Pitfalls

### Pitfall 1: “Selected constraint” vs “highlight participants” semantics drift
**What goes wrong:** one entry path highlights participants, another selects only constraint entity.  
**Why it happens:** separate handlers in manager and viewport evolved independently.  
**How to avoid:** implement one shared helper for participant highlight and call it from both paths.  
**Warning signs:** glyph click does not color all participants immediately.

### Pitfall 2: Verification artifact debt despite working code
**What goes wrong:** requirements remain orphaned even when summaries claim implementation.  
**Why it happens:** missing `11-VERIFICATION.md` and non-compliant `11-VALIDATION.md`.  
**How to avoid:** treat documentation evidence as required deliverable, not optional follow-up.  
**Warning signs:** audit still reports orphaned SKCH-04/CONS-01..05.

### Pitfall 3: Windows gate not enforced during closure
**What goes wrong:** behavior “fixed” but not proven on required platform/toolchain path.  
**Why it happens:** relying on code inspection only, skipping build/runtime proof.  
**How to avoid:** every plan wave includes `build-vulkan` command evidence + explicit manual UI checklist on Windows MSVC+Vulkan.  
**Warning signs:** no fresh build output attached to verification evidence.

## Implementation Risk Analysis (Windows MSVC + Vulkan Focus)

| Risk | Likelihood | Impact | Mitigation |
|---|---|---|---|
| Glyph click fix regresses dimension popup targeting | MEDIUM | MEDIUM | Preserve `state.selected_constraint_entity` / popup constraint entity state while changing selection highlight path. |
| Pick routing order regression (glyph vs gizmo/entity) | LOW-MEDIUM | HIGH | Keep existing routing priority; only adjust post-click selection semantics. |
| Toolchain discoverability mismatch in shell (`cl`/`msbuild` not on PATH) | MEDIUM | MEDIUM-HIGH | Run build via existing CMake generator context (`cmake --build build-vulkan --config Release --target mdCAD`) and capture output; if needed, run in VS Developer shell. |
| Validation remains draft after code closure | HIGH | HIGH | Plan explicit documentation tasks: create `11-VERIFICATION.md`, update frontmatter/status in `11-VALIDATION.md`, fill requirement-level evidence map. |

## Acceptance Evidence Strategy (Windows MSVC + Vulkan Gate)

1. **Code-level closure proof**
   - Show updated glyph click handler now applies participant highlight semantics.
   - Include line references in `11-VERIFICATION.md`.
2. **Build gate proof (Windows)**
   - Record command + success output for:
     - `cmake --build build-vulkan --config Release --target mdCAD`
     - `ctest --test-dir build-vulkan -C Release --output-on-failure`
3. **Manual UX checklist proof**
   - Re-run SKCH-04 + CONS-01..05 flows from both manager and viewport entrypoints.
   - Capture pass/fail with repro notes and any caveats.
4. **Artifact closure proof**
   - Add `11-VERIFICATION.md` with requirement-by-requirement status.
   - Update `11-VALIDATION.md` frontmatter to compliant state once Wave 0 gaps are addressed.

## Code Examples

### Fix target: glyph click path currently selects constraint entity (needs unification)
```c
// src/app.c (current behavior around lines ~1083-1086)
// Glyph click selects the constraint itself (not participants).
// Participant highlighting is handled in ConstraintManager row actions.
selection_set_single(&state.selection, clicked_constraint);
```

### Existing correct participant-highlight behavior (reference implementation)
```c
// src/ui/ui_entity_inspector.h (ConstraintManager row select)
selection_clear(state->selection);
for (uint32_t pi = 0; pi < constraint->participant_count; pi++) {
    ecs_entity_t p = (ecs_entity_t)constraint->participants[pi];
    if (ecs_is_alive(w->world, p)) selection_add(state->selection, p);
}
```

### Mirrored dimensional edit behavior (already shared)
```c
// src/app.c popup Accept/Enter + ui_entity_inspector.h manager edits
scene_constraint_set_dimensional_value(scene, constraint_entity, value, driven);
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|---|---|---|---|
| ConstraintManager-only participant highlight expectation | Manager + viewport glyph flows required to match | Audit 2026-04-01 identified gap | Phase 16 must unify UX semantics and evidence. |
| Summary-only closure confidence | Requirement-level verification + Nyquist validation compliance | v1.2 audit process | Prevents orphaned requirements and hidden closure debt. |

**Deprecated/outdated:**
- “Implemented in summary implies complete” — replaced by requirement-level verification artifact requirement.

## Open Questions

1. **Should single-click glyph selection also keep constraint entity selected anywhere?**
   - What we know: current code tracks `selected_constraint_entity` for popup context.
   - What's unclear: whether inspector workflows require the constraint entity itself to remain selected.
   - Recommendation: keep `selected_constraint_entity` as separate UI state while selection buffer highlights participants.

2. **How strict should CONS-02 keybinding evidence be (Tab vs C mismatch in docs)?**
   - What we know: implementation uses `C` key in `app.c`; some roadmap text still says Tab-triggered.
   - What's unclear: whether requirement wording must be corrected in docs or implementation changed.
   - Recommendation: resolve wording during verification write-up (document current behavior and align requirement phrasing explicitly).

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|---|---|---|---|---|
| CMake | Build gate evidence | ✓ | 4.3.0 | — |
| CTest | Validation suite evidence | ✓ | 4.3.0 | Manual smoke only (lower confidence) |
| Vulkan runtime tooling (`vulkaninfo`) | Vulkan environment sanity | ✓ | version not reported | Use `mdCAD` runtime launch + build output evidence |
| MSVC compiler in current shell (`cl`) | Direct compiler probing | ✗ (current shell) | — | Run builds through configured CMake VS generator / Developer shell |
| MSBuild in current shell (`msbuild`) | Direct MSBuild invocation | ✗ (current shell) | — | `cmake --build build-vulkan --config Release --target mdCAD` |

**Missing dependencies with no fallback:**
- None identified for planned closure tasks, assuming existing `build-vulkan` generator remains valid.

**Missing dependencies with fallback:**
- `cl` / `msbuild` not directly on PATH in this shell; use CMake build entrypoint or VS Developer environment.

## Validation Architecture

### Test Framework
| Property | Value |
|---|---|
| Framework | CMake/CTest + manual UI verification |
| Config file | `CMakeLists.txt`, `src/CMakeLists.txt`, `.planning/phases/11-constraint-authoring-ux/11-VALIDATION.md` |
| Quick run command | `cmake --build build-vulkan --config Release --target mdCAD` |
| Full suite command | `ctest --test-dir build-vulkan -C Release --output-on-failure` |

### Phase Requirements → Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|---|---|---|---|---|
| SKCH-04 | Selecting a constraint highlights all participants from glyph + manager paths | manual integration UI + build | `cmake --build build-vulkan --config Release --target mdCAD` | ❌ (`11-VERIFICATION.md` missing) |
| CONS-01 | Only legal constraints can be applied for active selection signature | manual matrix + code trace | `cmake --build build-vulkan --config Release --target mdCAD` | ❌ |
| CONS-02 | Context menu applicability filter + auto-hide after apply | manual interaction | `cmake --build build-vulkan --config Release --target mdCAD` | ❌ |
| CONS-03 | Hover/select via constant-screen-size glyphs; consistent highlight | manual viewport interaction | `cmake --build build-vulkan --config Release --target mdCAD` | ❌ |
| CONS-04 | LENGTH/ANGLE mirrored manager/popup value editing | manual interaction + code trace | `cmake --build build-vulkan --config Release --target mdCAD` | ❌ |
| CONS-05 | Driven toggle visible/editable and preserved in UI state | manual interaction + code trace | `cmake --build build-vulkan --config Release --target mdCAD` | ❌ |

### Sampling Rate
- **Per task commit:** `cmake --build build-vulkan --config Release --target mdCAD`
- **Per wave merge:** `ctest --test-dir build-vulkan -C Release --output-on-failure`
- **Phase gate:** Full suite green + completed manual checklist evidence in `11-VERIFICATION.md`

### Wave 0 Gaps
- [ ] `.planning/phases/11-constraint-authoring-ux/11-VERIFICATION.md` — required requirement-level closure artifact (currently missing)
- [ ] Update `.planning/phases/11-constraint-authoring-ux/11-VALIDATION.md` frontmatter from draft to compliant state (`nyquist_compliant: true`, `wave_0_complete: true` when satisfied)
- [ ] Add explicit manual checklist entries proving glyph-click participant highlighting consistency on Windows MSVC+Vulkan

## Sources

### Primary (HIGH confidence)
- `.planning/v1.2-MILESTONE-AUDIT.md` — Phase 11 orphaned requirements + integration/flow gap evidence.
- `.planning/ROADMAP.md` — Phase 16 scope, success criteria, dependency and gate intent.
- `.planning/REQUIREMENTS.md` — authoritative requirement text for SKCH-04 and CONS-01..05.
- `src/app.c` — actual glyph click routing and constraint menu/popup behavior.
- `src/ui/ui_entity_inspector.h` — manager selection/highlight and dimensional/driven edit behavior.
- `src/constraints/constraint_glyphs.h` — glyph pick mapping and constant-size overlay behavior.
- `src/ecs/ecs_scene.h` — scene helper contracts for create/remove/update constraint data.
- `.planning/phases/11-constraint-authoring-ux/11-VALIDATION.md` — current draft validation status.

### Secondary (MEDIUM confidence)
- `.planning/phases/11-constraint-authoring-ux/11-01/02/03-SUMMARY.md` — implementation claim context used to target verification closure.

### Tertiary (LOW confidence)
- None.

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — closure is in-repo and uses existing pinned stack.
- Architecture: HIGH — behavior paths and divergence are directly visible in source.
- Pitfalls: HIGH — audit artifacts and code comments directly identify failure modes.

**Research date:** 2026-04-01  
**Valid until:** 2026-05-01 (stable for this gap-closure scope unless core constraint UX code changes)
