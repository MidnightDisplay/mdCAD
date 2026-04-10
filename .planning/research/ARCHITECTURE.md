# Architecture Research

**Domain:** mdCAD v1.5 solver workflow robustness + script re-apply integrity  
**Researched:** 2026-04-10  
**Confidence:** HIGH

## Standard Architecture

### System Overview

```text
Authoring/UI (app.c, script editor, gizmo)
    -> Scene orchestration (src/ecs/ecs_scene.h)
        -> Constraint graph + solver pass loop + diagnostics
        -> Script parse/emit/apply pipeline
            -> ECS entities/components + remap
```

## Major Integration Points

- `src/ecs/ecs_scene.h`: primary solve/recalc, diagnostics, failure implication, script apply transaction.
- `src/scripting/sketch_script_emit.h`: must evolve to preserve participant descriptor semantics.
- `src/scripting/sketch_script_parse.h`: must parse descriptor-rich participant forms.
- `src/scripting/sketch_script_apply.h`: must reconstruct descriptor-accurate constraints and preserve colors.
- `src/constraints/constraint_types.h`: legality/runtime parity helpers for descriptor-aware constraints.
- `src/app.c`: route composite authoring through explicit-coincidence workflows.

## Recommended Implementation Order

1. **Script fidelity foundation:** descriptor-preserving emit/parse/apply + color preservation.
2. **Explicit authoring semantics:** ArcAxisLine/tangency with explicit coincidence insertion.
3. **Large-jump robustness policy:** deterministic bounded adaptive solve behavior.
4. **PARALLEL/ALONG parity hardening:** equivalent-intent behavior and responsiveness parity.
5. **Closure verification gates:** deterministic rerun evidence for interactive + script flows.

## Anti-Patterns to Avoid

- Entity-only participant reconstruction for role-sensitive constraints.
- Hidden implicit coincidence behavior in composite constraints.
- Non-deterministic retries/jitter as convergence workaround.
- Partial state commits when solve/apply fails.

## Validation Hooks

- Script roundtrip descriptor equality (type + participant role/sub-index intent).
- Re-apply visual fidelity checks (entity color preservation).
- Large-jump quarter-arc and multi-tangency workflow convergence/rollback tests.
- PARALLEL vs ALONG metamorphic parity tests on equivalent arrangements.

---
*Architecture research for: mdCAD v1.5 solver workflow robustness + script re-apply integrity*  
*Researched: 2026-04-10*
