# Stack Research: v1.4 Solver Robustness + Sketch Gizmo Corrections

**Domain:** Solver robustness and sketch-line gizmo corrections in mdCAD  
**Researched:** 2026-04-08  
**Confidence:** High

## Keep / Reuse

- Keep C-first architecture, CMake + CTest workflow, and current core deps (`cglm`, Flecs, Sokol, cimgui, cJSON).
- Keep `scene_solver_*` APIs as runtime solver authority and `constraint_type_is_selection_legal(...)` as legality gate.
- Keep Windows Vulkan path as milestone validation gate.

## Recommended Additions (internal only)

- Add focused tests for line-line constraints, line ALONG semantics, tangency drag robustness, and active-sketch line gizmo behavior.
- Add `docs/SOLVER_ARCHITECTURE.md` as the human-facing solver map + TL;DR primer.

## Verification Tooling

- Continue targeted CTest gates with deterministic reruns.
- Ensure v1.4 validation includes:
  - `scene_solver_contract`
  - `scene_solver_pass_policy`
  - `scene_solver_drag`
  - `scene_solver_diagnostics`
  - `endpoint_pick`

## Documentation Tooling

- Keep Markdown docs in `docs/`.
- Use lightweight diagrams and direct file/function cross-references.

## Do Not Add

- No C++ dependency expansion for solver work.
- No external heavy solver framework for this milestone.
- No build-system migration or test-runner churn.
- No platform-gate change away from Windows Vulkan.

## Risk Notes

- Biggest risk is behavior drift between legality and runtime solve paths.
- Mitigation: test-first contracts and deterministic ordering rules.
