# Project Retrospective

*A living document updated after each milestone. Lessons feed forward into future planning.*

## Milestone: v1.3 — Sketch Solver Audit + Constraint Expansion

**Shipped:** 2026-04-08
**Phases:** 4 | **Plans:** 10 | **Sessions:** multi-session

### What Was Built
- Deterministic solver trigger and pass-policy behavior with bounded recalculate controls.
- Principal-direction ALONG X/Y/Z group constraints across standalone points and endpoint landmarks.
- Advanced ARCI constraints (arc-axis, line-end/arc-end tangency, arc endpoint-angle) with explicit transactional diagnostics.
- Strict regression closure gate proving reliability with mandatory fresh rerun evidence.

### What Worked
- Keeping solver authority in scene-owned APIs reduced UI/runtime drift.
- Focused named CTest gates (`-R` scoped) gave fast, reproducible closure loops.
- Role-descriptor legality centralization prevented authoring-path inconsistencies.

### What Was Inefficient
- Late summary/verification artifact reconciliation caused avoidable end-of-phase cleanup.
- State/roadmap metadata occasionally required manual normalization after tool-driven updates.

### Patterns Established
- Treat flaky outcomes as failures and require immediate deterministic rerun before closure.
- Preserve drag-anchor intent first when balancing direct-manipulation UX against tangency enforcement.
- Use strict closure gates with explicit command/result evidence in verification artifacts.

### Key Lessons
1. Reliability milestones close faster when verification evidence format is enforced from Wave 1.
2. Descriptor-role constraints require consistent legality + persistence + runtime handling to avoid subtle regressions.

### Cost Observations
- Model mix: not tracked in repo artifacts
- Sessions: multi-session
- Notable: focused gate strategy minimized unnecessary full-suite reruns during stabilization.

---

## Cross-Milestone Trends

### Process Evolution

| Milestone | Sessions | Phases | Key Change |
|-----------|----------|--------|------------|
| v1.3 | multi-session | 4 | Solver reliability closure shifted to strict targeted-gate + fresh-rerun protocol |

### Cumulative Quality

| Milestone | Tests | Coverage | Zero-Dep Additions |
|-----------|-------|----------|-------------------|
| v1.3 | 7-test strict closure gate | Requirement-mapped targeted coverage (14/14 v1.3 reqs complete) | Added focused native C test coverage; no new external deps |

### Top Lessons (Verified Across Milestones)

1. Scope-locked targeted gates improve determinism and reduce closure churn.
2. Milestone closure quality depends on continuous artifact hygiene (summaries, validation, verification, state).
