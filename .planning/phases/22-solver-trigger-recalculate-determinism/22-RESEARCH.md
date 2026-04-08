# Phase 22: Solver Trigger + Recalculate Determinism - Research

**Researched:** 2026-04-07  
**Domain:** Sketch solver trigger orchestration, deterministic recalculate policy, and bounded iterative behavior in mdCAD ECS scene solver  
**Confidence:** HIGH

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
### Auto-solve trigger semantics
- **D-01:** Auto-solve should use a **short debounce queue** rather than immediate per-mutation solve.
- **D-02:** Debounce default is **0ms**.
- **D-03:** If new mutations arrive while auto-solve is pending, requests are **coalesced** into the pending solve (no request fan-out).
- **D-04:** Manual **Recalculate Sketch** runs immediately and **clears pending auto-solve queue**.

### Recalculate pass policy
- **D-05:** Phase 22 must expose **position + angle tolerances in UI** (not internal-only hidden tolerances).
- **D-06:** Default max pass count is **400**.
- **D-07:** If pass-cap is hit without convergence, solver status becomes **Error** with explicit `"max passes reached"` diagnostic.
- **D-08:** On successful convergence, prior failure implication highlighting clears **immediately** (preserve existing successful-solve clear contract).

### Driving LENGTH/ANGLE contract
- **D-09:** For unsatisfiable driving LENGTH/ANGLE constraints: **no geometry mutation**, explicit failure diagnostic, and implicated constraints highlighted.
- **D-10:** For satisfiable driving LENGTH/ANGLE constraints: apply geometry update **atomically in one successful solve commit** (no gradual multi-click convergence UX).

### Migration safety guardrails
- **D-11:** Preserve diagnostics ring behavior unchanged: identical-consecutive dedupe, cap 100, explicit clear action only.
- **D-12:** Preserve solver authority boundary as hard requirement: runtime decisions stay in `scene_solver_*`; UI/app remain thin callers.

### the agent's Discretion
- Exact field names and storage location for the new tolerance UI-exposed values.
- Exact queue flush implementation details, provided default-immediate debounce semantics (0ms), coalescing, and manual override semantics remain intact.
- Exact diagnostic message text for max-pass failures, as long as explicit reason is present and testable.

### Deferred Ideas (OUT OF SCOPE)
- Grouped `ALONG X/Y/Z` constraints across mixed point participant sets (Phase 23).
- Advanced arc/line-arc constraints (arc-center axis relation, endpoint tangency, arc endpoint angle) (Phase 24).
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| SRLV-01 | User can rely on auto-solve to trigger after committed sketch mutations, including constraint add/remove/edit and geometry move operations. | Debounced scene-owned auto-solve scheduler pattern, callsite inventory, and coalescing/manual-clear behavior recommendations. |
| SRLV-02 | User can run manual recalculate and get deterministic, idempotent results for unchanged sketch state. | Deterministic pass-loop contract, stable ordering/epsilon guidance, and explicit test map for repeat recalc identity. |
| SRLV-03 | Solver runs iterative passes until tolerance is satisfied or max pass count is reached, with configurable pass cap (default `400`). | Solver config extension (position/angle tolerances + max pass), max-pass error contract, and UI exposure strategy. |
| SRLV-05 | Driving `LENGTH` and `ANGLE` constraints produce expected geometric effects or explicit failure diagnostics. | Atomic transactional solve commit/no-mutation failure pattern and targeted tests for satisfiable/unsatisfiable LENGTH+ANGLE pathways. |
</phase_requirements>

## Summary

Phase 22 is primarily an **architecture hardening phase**, not a new subsystem. The repository already has the right authority boundary: UI/app are thin callers and `scene_solver_*` owns solve outcomes. The biggest gap is that auto-solve currently sets `auto_solve_pending` and increments serials, but there is no debounce-time processing path that actually executes pending solves. That means trigger semantics exist as flags but not as deterministic queue execution behavior.

Recalculate today is single-pass transactional logic centered in `scene_solver_request_recalculate(...)`, with deterministic implication sorting and no-mutation failure on unsat for currently supported cases (coincident point participants). However, iterative pass policy, tolerance configuration, max-pass diagnostics, and driving LENGTH/ANGLE behavior are not implemented to Phase 22 contract. LENGTH is currently effectively ignored in solve pass; ANGLE is currently unsupported in transactional recalc path.

**Primary recommendation:** Implement a **scene-owned debounced auto-solve scheduler + bounded deterministic iterative recalc engine** in `scene_solver_*`, then expose pass/tolerance controls in inspector and lock behavior with new targeted solver tests before any broad UI changes.

## Standard Stack

### Core
| Library/Module | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| `src/ecs/ecs_scene.h` (`scene_solver_*`) | repo-current | Solver trigger/recalc authority and mutation transaction point | Existing architectural contract already centralizes runtime solver decisions here. |
| `SketchComp` (`src/components/sketch_comp.h`) | repo-current | Sketch-local solver state, diagnostics ring, serial counters | Holds per-sketch state required for queueing, pass config, and diagnostics continuity. |
| CTest + native C test binaries | CMake/CTest (repo toolchain) | Deterministic regression gating | Existing solver tests already run as first-class CTest targets; extends naturally for Phase 22. |

### Supporting
| Library/Module | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| `src/ui/ui_entity_inspector.h` | repo-current | Solver controls surface (auto/recalc/diagnostics) | Expose tolerances + max-pass with thin-caller semantics. |
| `src/app.c` drag integration | repo-current | Constrained drag path -> solver feedback | Keep drag behavior as caller-only; do not move policy to app loop. |
| `src/constraints/constraint_types.h` | repo-current | Constraint legality/type helpers | Use for LENGTH/ANGLE participation assumptions and dimensional behaviors. |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Scene-owned debounce execution | UI-level timers/callback debounce | Violates locked authority boundary and duplicates behavior across callsites. |
| Transactional stage+commit | Direct in-place geometry mutation each pass | Risks non-deterministic intermediate states and breaks no-mutation failure contract. |

**Installation:** Not applicable (phase uses existing in-repo C/CMake stack; no new package dependency required).

## Architecture Patterns

### Recommended Project Structure
```text
src/
├── ecs/ecs_scene.h                 # Solver authority (debounce queue + recalc engine)
├── components/sketch_comp.h        # Sketch solver config/state fields
├── ui/ui_entity_inspector.h        # Thin UI controls for tolerance/pass config + manual recalc
└── tests/
   ├── scene_solver_contract_test.c
   ├── scene_solver_diagnostics_test.c
   ├── scene_solver_drag_test.c
   └── (new) scene_solver_trigger_test.c / scene_solver_pass_policy_test.c
```

### Pattern 1: Scene-owned trigger queue with explicit coalescing
**What:** Mutations call `scene_solver_request_auto`; scene tracks pending request and executes after debounce window, coalescing bursts into one solve attempt.  
**When to use:** All committed sketch-affecting mutations (constraint add/remove/edit, geometry edits, endpoint sync, undo/redo replay).  
**Example (existing coalescing seed):**
```c
// Source: src/ecs/ecs_scene.h (2455-2465)
static inline bool scene_solver_request_auto(ecs_scene_t *scene, ecs_entity_t sketch) {
    ...
    if (!sk->auto_solve_pending) {
        sk->auto_solve_pending = true;
        sk->solve_request_serial++;
    }
    return true;
}
```

### Pattern 2: Transactional solve commit/no-mutation failure
**What:** Stage candidate geometry values first; commit only on full success; on failure keep original geometry and emit implication+diagnostic.  
**When to use:** All recalculate attempts, especially LENGTH/ANGLE driving constraints.  
**Example:**
```c
// Source: src/ecs/ecs_scene.h (2574-2595)
if (solve_failed) {
    scene_solver_set_failure_implication(...);
    scene_solver_apply_status(scene, sketch, SKETCH_STATUS_ERROR);
    return false;
}
for (int i = 0; i < candidate_count; i++) {
    scene_apply_local_point_to_participant(...);
}
return scene_solver_apply_status(scene, sketch, derived_status);
```

### Pattern 3: Success clears implication immediately
**What:** Status sink owns clear-on-success implication lifecycle.  
**When to use:** Every successful solve completion path must route through `scene_solver_apply_status(...)`.  
**Example:**
```c
// Source: src/ecs/ecs_scene.h (2669-2678)
if (status == SKETCH_STATUS_SOLVED) {
    scene_solver_clear_failure_implication(scene, sketch, true);
    assert(!imp || !imp->active || (imp->sketch != 0 && imp->sketch != sketch));
}
```

### Anti-Patterns to Avoid
- **UI-managed solve timers:** breaks D-12 authority boundary and creates inconsistent trigger behavior.
- **Per-callsite custom solve decisions:** encourages drift between app, inspector, undo paths.
- **Incremental visible mutation during failed solve:** violates D-09 no-mutation unsat contract.
- **Auto-clearing diagnostics on success:** violates D-11 explicit-clear-only diagnostics ring semantics.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Cross-layer debounce orchestration | Separate timers in app + inspector + undo | Single scene-owned per-sketch debounce policy | Preserves deterministic ordering and locked authority boundary. |
| Failure highlight plumbing | Ad-hoc participant scans in UI | `scene_solver_set_failure_implication` + `constraint_selection_apply_participants` | Already deterministic and tested for ordering/dedupe. |
| Diagnostics history store | New log list/vector | Existing ring (`SKETCH_SOLVER_DIAGNOSTICS_MAX=100`) | Dedupe/cap/clear semantics already implemented and tested. |
| Pass-loop convergence state | Ad-hoc globals | Sketch-local solver config/state in `SketchComp` | Keeps behavior sketch-scoped and serializable in existing metadata lifecycle. |

**Key insight:** Phase 22 should extend existing solver contracts, not replace them—most required behavior is already scaffolded in the right modules.

## Common Pitfalls

### Pitfall 1: Pending auto-solve never executes
**What goes wrong:** Mutations set `auto_solve_pending` but no debounced execution happens.  
**Why it happens:** No scene/frame processing hook currently consumes pending requests.  
**How to avoid:** Add explicit scene-level tick/flush API (e.g., `scene_solver_process_auto_queue(scene, now_ms)`) called once per frame.  
**Warning signs:** `solve_request_serial` increases while `solve_completed_serial` and geometry remain unchanged.

### Pitfall 2: Manual recalc races with pending auto queue
**What goes wrong:** Manual recalc runs but stale queued auto-solve fires afterward, changing status/diagnostics unexpectedly.  
**Why it happens:** Queue timestamp/request token not invalidated on manual override.  
**How to avoid:** Manual recalc must clear pending flag + cancel debounce token before/after immediate run (D-04).  
**Warning signs:** Two back-to-back solve diagnostics from one manual click.

### Pitfall 3: Non-deterministic pass-loop behavior
**What goes wrong:** Same unchanged sketch gives different outcomes across repeated recalc.  
**Why it happens:** Unstable constraint iteration order or tolerance comparisons without fixed epsilon policy.  
**How to avoid:** Stable entity ordering + deterministic pass order + explicit position/angle tolerance checks.  
**Warning signs:** flaky tests on repeated recalc idempotence.

### Pitfall 4: SRLV-05 false-positive "success"
**What goes wrong:** LENGTH/ANGLE constraints accepted but geometry unchanged and no explicit failure.  
**Why it happens:** Current solver path continues on LENGTH and fails unsupported constraints only in some branches.  
**How to avoid:** Implement explicit satisfiable-commit or unsat-diagnostic/no-mutation branches for driving LENGTH/ANGLE.  
**Warning signs:** recalc returns success with dimensional constraints but measurable target mismatch remains.

## Code Examples

Verified in-repo patterns:

### Auto request callsites are already centralized through scene API
```c
// Source: src/ecs/ecs_scene.h (2389-2391)
scene_refresh_sketch_metadata(scene, sketch);
scene_solver_request_auto(scene, sketch);
scene_script_reemit_for_sketch(scene, sketch);
```

### Drag unsat feedback remains thin caller
```c
// Source: src/app.c (1728-1746)
has_drag_decision = scene_solver_can_apply_drag(..., &drag_decision);
...
if (drag_decision.result == SCENE_SOLVER_DRAG_UNSATISFIABLE) {
    delta = vec3_make(0.0f, 0.0f, 0.0f);
    mdcad_apply_solver_failure_feedback(drag_sketch, &drag_decision);
}
```

### Diagnostics dedupe behavior to preserve
```c
// Source: src/ecs/ecs_scene.h (2607-2616)
if (last &&
    last->severity == severity &&
    last->implicated_constraint == (uint64_t)implicated_constraint &&
    ((last->message[0] == '\0' && (!message || message[0] == '\0')) ||
     (message && strcmp(last->message, message) == 0))) {
    return true;
}
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Placeholder sketch status derivation | Transactional recalc + implication contracts in `scene_solver_*` | Phases 12/17 | Established authority and no-mutation failure baseline. |
| Immediate/implicit per-edit solve intent | Pending auto-solve flag + serial coalescing seed | Phase 12+ | Good scaffold, but missing actual debounce execution in runtime loop. |
| Non-deterministic implication ordering risk | Deterministic sorted implications/participants | Phase 17 | Enables reliable idempotence assertions and stable UX focus behavior. |

**Deprecated/outdated (for Phase 22 scope):**
- Single-pass-only recalc behavior for reliability contract: replaced by bounded iterative pass loop with explicit tolerances/max-pass.
- Implicit dimensional support assumptions: replace with explicit LENGTH/ANGLE satisfiable/unsat branches.

## Open Questions

1. **Where should debounce queue processing be invoked each frame?**
   - What we know: `scene_solver_request_auto` exists; no processing hook found.
   - What's unclear: best central frame location for one-call-per-frame processing that covers all runtime modes.
   - Recommendation: add one scene API and call it from main app update loop near existing interaction/solver operations.

2. **Tolerance units and defaults**
   - What we know: locked decision requires position + angle tolerance in UI.
   - What's unclear: exact defaults and allowed ranges (especially angle unit display).
   - Recommendation: use conservative defaults (`position=1e-4`, `angle=1e-4 rad` equivalent display), clamp ranges, and test deterministic behavior.

3. **Driving LENGTH/ANGLE solve strategy**
   - What we know: must be atomic on success, no-mutation on unsat.
   - What's unclear: whether to implement direct analytic updates first for simple cases before generalized iterative projection.
   - Recommendation: start with deterministic minimal supported participant combos and explicit diagnostics for unsupported combos.

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| cmake | configure/build tests | ✓ | 4.3.0 | — |
| ctest | running validation binaries | ✓ | 4.3.0 | — |
| gcc | C compilation (native desktop) | ✓ | 15.2.0 (MSYS2) | — |
| ninja | common CMake generator | ✗ | — | Use CMake default generator (e.g., MinGW Makefiles/VS if installed) |
| cl (MSVC) | Windows MSVC build pathway | ✗ (current shell) | — | Use gcc toolchain path in this environment |
| git | workflow and verification scripts | ✓ | 2.51.1.windows.1 | — |
| node | GSD orchestration tools | ✓ | v25.9.0 | — |
| python | auxiliary tooling (if needed) | ✓ | 3.13.12 | — |

**Missing dependencies with no fallback:**
- None identified for Phase 22 implementation/testing in current environment.

**Missing dependencies with fallback:**
- `ninja` (fallback: non-Ninja CMake generators).
- `cl` in current shell (fallback: GCC/MSYS2 build for local verification).

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | CTest + native C executable test targets |
| Config file | `CMakeLists.txt`, `src/CMakeLists.txt` |
| Quick run command | `ctest --output-on-failure -R "scene_solver_contract|scene_solver_diagnostics|scene_solver_drag"` |
| Full suite command | `ctest --output-on-failure` |

### Phase Requirements → Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| SRLV-01 | Debounced auto trigger, coalescing, manual recalc clears queue | unit/contract | `ctest --output-on-failure -R scene_solver_trigger` | ❌ Wave 0 |
| SRLV-02 | Manual recalc idempotent on unchanged sketch | unit/contract | `ctest --output-on-failure -R scene_solver_contract` | ✅ |
| SRLV-03 | Iterative pass to tolerance or max-pass=10 error | unit/contract | `ctest --output-on-failure -R scene_solver_pass_policy` | ❌ Wave 0 |
| SRLV-05 | Driving LENGTH/ANGLE atomic success or explicit no-mutation failure | unit/contract | `ctest --output-on-failure -R scene_solver_contract` | ✅ (needs extension) |

### Sampling Rate
- **Per task commit:** `ctest --output-on-failure -R "scene_solver_contract|scene_solver_diagnostics|scene_solver_drag"`
- **Per wave merge:** `ctest --output-on-failure -R "scene_solver_.*|endpoint_pick|script_roundtrip_tests"`
- **Phase gate:** Full suite green before `/gsd-verify-work`

### Wave 0 Gaps
- [ ] `src/tests/scene_solver_trigger_test.c` — debounce 50ms, coalescing, manual-clear queue semantics (SRLV-01)
- [ ] `src/tests/scene_solver_pass_policy_test.c` — tolerance stop vs max-pass stop diagnostics/default=10 (SRLV-03)
- [ ] Extend `src/tests/scene_solver_contract_test.c` — deterministic repeated recalc + LENGTH/ANGLE success/failure atomicity (SRLV-02/SRLV-05)
- [ ] Register new test binaries in `src/CMakeLists.txt` via `add_executable(...)` + `add_test(...)`

## Sources

### Primary (HIGH confidence)
- `.planning/phases/22-solver-trigger-recalculate-determinism/22-CONTEXT.md` — locked decisions and scope boundaries.
- `.planning/ROADMAP.md` — Phase 22 goals and success criteria.
- `.planning/REQUIREMENTS.md` — SRLV requirement definitions and traceability.
- `src/ecs/ecs_scene.h` — solver API authority, request/recalc behavior, implication and diagnostics lifecycle.
- `src/components/sketch_comp.h` — diagnostics ring and sketch solver state model.
- `src/ui/ui_entity_inspector.h` — solver control surface and current UI trigger pathways.
- `src/app.c` — constrained drag decision integration and failure feedback.
- `src/tests/scene_solver_contract_test.c` — current deterministic/transactional solve tests.
- `src/tests/scene_solver_diagnostics_test.c` — diagnostics dedupe and implication ordering tests.
- `src/tests/scene_solver_drag_test.c` — drag unsat/projection diagnostics tests.
- `src/CMakeLists.txt` + `CMakeLists.txt` — CTest wiring and test target registration.

### Secondary (MEDIUM confidence)
- None (no external-doc dependency required for this phase research).

### Tertiary (LOW confidence)
- None.

## Metadata

**Confidence breakdown:**
- Standard stack: **HIGH** — all recommendations derive directly from existing project architecture and locked decisions.
- Architecture: **HIGH** — supported by current solver/UI/app call graph and test harness structure.
- Pitfalls: **HIGH** — directly observed from current code paths and missing execution links.

**Research date:** 2026-04-07  
**Valid until:** 2026-05-07 (stable codebase-local domain; revalidate if solver APIs change substantially)
