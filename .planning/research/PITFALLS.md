# Pitfalls Research

**Domain:** v1.5 solver robustness + script re-apply integrity  
**Researched:** 2026-04-10  
**Confidence:** HIGH

## Critical Pitfalls

### Pitfall 1: Over-relying on max-pass inflation

**What goes wrong:** Large-jump edits still stall while solve cost grows and behavior stays fragile.

**Why it happens:** Pass count tuning is used as a substitute for fixing conditioning/ordering/anchor issues.

**How to avoid:** Use bounded deterministic adaptive solve strategy with residual trend checks, not unbounded pass inflation.

**Phase to address:** Early robustness phase.

---

### Pitfall 2: Implicit coincidence coupling

**What goes wrong:** ArcAxisLine/tangency appear valid but break after subsequent larger edits.

**Why it happens:** Hidden topology assumptions are encoded implicitly instead of explicit coincidence constraints.

**How to avoid:** Author explicit coincidence constraints first, then apply orientation/tangency relations.

**Phase to address:** Authoring semantics phase.

---

### Pitfall 3: Descriptor loss in script re-apply

**What goes wrong:** Re-applied constraints become semantically wrong and trigger misleading solver errors.

**Why it happens:** Participant roles/sub-index are lost when script pipeline reconstructs entity-only descriptors.

**How to avoid:** Roundtrip full participant descriptors (`id`, `role`, `sub_index`) in emit/parse/apply.

**Phase to address:** Script fidelity phase.

---

### Pitfall 4: Metadata fidelity regressions

**What goes wrong:** Re-apply resets colors or other visible sketch metadata.

**Why it happens:** Script/apply pathway recreates defaults and omits metadata restore.

**How to avoid:** Preserve/restore color metadata explicitly during script roundtrip apply transactions.

**Phase to address:** Script fidelity phase.

---

### Pitfall 5: Stale ID/cache references after reapply remap

**What goes wrong:** UI/solver interactions use dead or mismapped entities after script re-apply.

**Why it happens:** ECS IDs are recreated, but all dependent caches/highlights/selections are not fully refreshed.

**How to avoid:** Enforce post-reapply remap propagation and stale-reference invalidation.

**Phase to address:** Integration hardening phase.

## "Looks Done But Isn't" Checklist

- [ ] Large-jump edits converge/fail cleanly without manual wiggle.
- [ ] ArcAxisLine/tangency authoring emits explicit coincidence relations.
- [ ] Script roundtrip preserves descriptor roles/sub-index.
- [ ] Script re-apply preserves visual metadata (including colors).
- [ ] No stale ECS IDs remain in selection/highlight/interaction caches after reapply.

---
*Pitfalls research for: v1.5 Solver Workflow Robustness + Script Re-apply Integrity*  
*Researched: 2026-04-10*
