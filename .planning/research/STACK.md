# Stack Research

**Domain:** mdCAD v1.5 solver workflow robustness + script re-apply integrity  
**Researched:** 2026-04-10  
**Confidence:** HIGH

## Recommended Stack

### Core Technologies

| Technology | Version | Purpose | Why Recommended |
|------------|---------|---------|-----------------|
| C (C11) + header-inline architecture | Existing | Runtime implementation | v1.5 issues are behavior/integrity defects, not stack limitations |
| Flecs | 4.1.4 (vendored) | ECS identity/remap backbone | Re-apply fidelity depends on stable script-local-id to ECS remap |
| cglm | 0.9.6 (vendored) | Solver math operations | Already validated; no need for external math/solver runtime |
| cJSON | 1.7.19 (vendored) | Script/scene schema evolution | Supports descriptor/color field extensions cleanly |

### Required Internal Upgrades

- Preserve participant descriptors in script schema (`entity + role + sub_index`) for emit/parse/apply.
- Add explicit-coincidence authoring semantics for ArcAxisLine and endpoint tangency flows.
- Harden large-jump solve policy with deterministic adaptive handling (bounded, reproducible).
- Add targeted regression coverage for large-jump, parity, and re-apply integrity.

## What NOT to Add

| Avoid | Why | Use Instead |
|-------|-----|-------------|
| External C++ nonlinear solver frameworks | Violates C-first constraint and adds integration risk | Improve existing deterministic solver loop |
| Build/test stack churn | Unnecessary risk for robustness-only milestone | Keep CMake + CTest targeted gates |
| Non-deterministic retry tricks (random jitter) | Masks root causes and breaks reproducibility | Deterministic bounded adaptive policy |

## Validation Stack

- Keep Windows Vulkan as closure gate.
- Keep deterministic rerun policy: build + targeted baseline + immediate rerun.
- Extend existing tests: `scene_solver_contract`, `scene_solver_pass_policy`, `scene_solver_drag`, `scene_solver_diagnostics`, `script_roundtrip_tests`, `endpoint_pick`.

---
*Stack research for: mdCAD v1.5 solver robustness + script re-apply integrity*  
*Researched: 2026-04-10*
