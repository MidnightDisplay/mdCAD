# Project Research Summary

**Project:** mdCAD  
**Domain:** v1.5 Solver Workflow Robustness + Script Re-apply Integrity  
**Researched:** 2026-04-10  
**Confidence:** HIGH

## Executive Summary

v1.5 should stay focused on solver reliability under real user workflows and script re-apply integrity. No external stack expansion is required: the highest leverage comes from descriptor-fidelity in script roundtrip, explicit coincidence authoring semantics, bounded deterministic large-jump robustness tuning, and tighter regression coverage.

## Key Findings

### Stack additions

- No new external libraries are required for milestone success.
- Required internal upgrades:
  - descriptor-preserving script schema/apply flow,
  - explicit coincidence insertion in composite authoring paths,
  - deterministic adaptive solve policy for large jumps,
  - targeted regression expansion for script + interactive workflows.

### Feature table stakes

- Deterministic solve/replay outcomes for identical operations.
- Large-jump robustness for mixed arc/line closed-loop edits.
- Explicit coincidence semantics for ArcAxisLine and endpoint tangency workflows.
- PARALLEL-vs-ALONG behavior parity in equivalent setups.
- Script re-apply fidelity: preserved constraint intent + preserved colors.
- Typed, actionable diagnostics for failure classes.

### Watch out for

1. Max-pass inflation masking root-cause convergence issues.
2. Implicit coincidence coupling in composite constraints.
3. Participant descriptor loss (role/sub-index) in script re-apply.
4. Metadata fidelity regressions (e.g., color resets).
5. Stale reference/caches after ECS remap on re-apply.

## Implications for Roadmap

### Phase 31: Script Re-apply Fidelity Contract
**Rationale:** Removes false solver errors caused by remap/descriptor loss before convergence tuning.  
**Delivers:** Role-preserving emit/parse/apply + color preservation + integrity tests.

### Phase 32: Explicit Composite Authoring Semantics
**Rationale:** Stabilizes constraint graph semantics before pass-policy tuning.  
**Delivers:** Explicit coincidence-first ArcAxisLine/tangency authoring and verification.

### Phase 33: Large-Jump Robustness + Parity Hardening
**Rationale:** Core user workflow pain; depends on solid graph semantics and script fidelity foundation.  
**Delivers:** Deterministic bounded adaptive solve behavior + PARALLEL/ALONG parity regression closure.

### Phase 34: Deterministic Closure Gate for v1.5
**Rationale:** Final sign-off requires reproducible confidence.  
**Delivers:** Build + targeted baseline + rerun evidence for all new requirements.

## Confidence Assessment

| Area | Confidence | Notes |
|------|------------|-------|
| Stack | HIGH | Existing stack is already sufficient and validated |
| Features | HIGH | Based on concrete user-provided failure scenarios |
| Architecture | HIGH | Touchpoints are clear and already centralized |
| Pitfalls | HIGH | Reproduced patterns map directly to current code paths |

---
*Research completed: 2026-04-10*  
*Ready for requirements: yes*
