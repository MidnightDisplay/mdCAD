# Feature Research

**Domain:** CAD solver workflow robustness + script re-apply integrity (v1.5)  
**Researched:** 2026-04-10  
**Confidence:** HIGH

## Feature Landscape

### Table Stakes (Users Expect These)

| Feature | Why Expected | Complexity | Notes |
|---------|--------------|------------|-------|
| Deterministic solve outcomes for identical operations | Mature CAD expectation | MEDIUM | Same input -> same outcome class + diagnostics |
| Large-jump edit robustness in mixed arc/line loops | Users expect no "wiggle to latch" | HIGH | Must converge or fail transactionally |
| Explicit coincidence authoring for ArcAxisLine/tangency workflows | Hidden implicit links are fragile | MEDIUM | Coincidence should be first-class constraints |
| PARALLEL behavior parity vs equivalent ALONG setups | Equivalent intent should behave equivalently | HIGH | Especially rectangular/lattice workflows |
| Script re-apply integrity (constraint remap + color preservation) | Reapply should be lossless | HIGH | Preserve semantics despite ECS ID churn |
| Typed actionable diagnostics | Users need clear recovery guidance | MEDIUM | Avoid ambiguous unsupported-participant failures |

### Differentiators (Competitive Advantage)

| Feature | Value Proposition | Complexity | Notes |
|---------|-------------------|------------|-------|
| Determinism contract surfaced in workflow/logs | Improves trust in scripting workflows | MEDIUM | Useful for support/debug loops |
| Re-apply semantic diff visibility | Makes remap behavior auditable | MEDIUM | Great for script-heavy users |

### Anti-Features (Commonly Requested, Often Problematic)

| Feature | Why Requested | Why Problematic | Alternative |
|---------|---------------|-----------------|-------------|
| Unlimited hidden retry/tolerance inflation | Seems to improve solve success quickly | Degrades determinism/perf and hides root cause | Bounded deterministic adaptive strategy |
| Silent auto-rewriting of user constraints | "Just make it solve" convenience | Breaks intent traceability | Transactional fail + explicit diagnostics |
| Scope creep to new constraint families in this milestone | Feature pressure | Delays robustness closure | Keep v1.5 focused on robustness/replay integrity |

## MVP Definition

### Launch With (v1.5)

- [ ] Deterministic large-jump solve behavior for target user workflows
- [ ] Explicit coincidence semantics for ArcAxisLine + endpoint tangency authoring
- [ ] PARALLEL vs ALONG parity for equivalent arrangements
- [ ] Script re-apply descriptor fidelity and color preservation
- [ ] Typed diagnostics for solve/replay failure classes

### Add After Validation (v1.5.x)

- [ ] Recovery-hint UX layer on top of typed diagnostics
- [ ] Re-apply diff report for advanced debugging

### Future Consideration (v2+)

- [ ] Broader automatic constraint repair systems
- [ ] Runtime-selectable advanced solver strategy plugins

---
*Feature research for: v1.5 Solver Workflow Robustness + Script Re-apply Integrity*  
*Researched: 2026-04-10*
