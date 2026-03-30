# Pitfalls Research: v1.2 Sketches, Constraints, Scripting

**Domain:** Failure modes for sketch constraints + scripting integration in mdCAD  
**Researched:** 2026-03-30  
**Confidence:** Medium-high

## Critical pitfalls

1. **Non-deterministic solver outcomes**
   - Symptom: same sketch edits produce different final states.
   - Mitigation: deterministic solve contract + golden replay tests + residual reporting.

2. **Incremental solve divergence**
   - Symptom: local edits leave remote constraint violations or cause large jumps.
   - Mitigation: connected-component dirty graph + full-solve fallback on divergence.

3. **Weak over/under-constrained diagnostics**
   - Symptom: generic solver failures with unclear user recovery path.
   - Mitigation: structured diagnostics mapped to constraint/entity IDs and surfaced in UI.

4. **Degenerate geometry numeric instability**
   - Symptom: jitter, oscillation, NaN/Inf propagation.
   - Mitigation: sketch-local normalization, adaptive tolerances, explicit degenerate guards.

5. **Glyph clutter and pick conflicts**
   - Symptom: hard-to-select constraints, hover flicker with geometry/gizmo.
   - Mitigation: glyph LOD/filter modes, reserved pick ID ranges, enlarged hit proxies.

6. **Undo/redo transactional breakage**
   - Symptom: undo restores partial state (geometry yes, constraints/script no).
   - Mitigation: composite sketch transactions and stable IDs decoupled from transient entity ordering.

7. **Bidirectional script sync loops**
   - Symptom: repeated churn and unintended updates between UI and script.
   - Mitigation: change-origin tagging, canonical serialization, loop guards, idempotence tests.

8. **Dependency choice blocking iOS/web later**
   - Symptom: desktop-only runtime works now but blocks deferred targets.
   - Mitigation: early compile/license probes for future targets and abstraction boundaries around backend/runtime.

## Phase ownership hints

- Early phases: legality matrix, transaction model, deterministic solver contract.
- Mid phases: glyph UX/picking and script synchronization.
- Late phases: undo integration hardening and cross-target/performance closure.

## Practical prevention checklist

- [ ] Constraint legality matrix is single-source-of-truth.
- [ ] Driving vs driven is encoded in data model (not implied in UI only).
- [ ] Every mutation path routes through unified sketch transaction API.
- [ ] Deterministic replay tests exist for UI + solver + script round-trip.
- [ ] Pick-debug diagnostics include glyph IDs and interaction source.
- [ ] Script round-trip tests are idempotent for supported syntax.
- [ ] Windows Vulkan performance gate includes dense-sketch interaction cases.

