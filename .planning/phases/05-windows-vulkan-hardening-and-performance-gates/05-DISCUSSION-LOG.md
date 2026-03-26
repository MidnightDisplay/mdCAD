# Phase 5: Windows Vulkan Hardening and Performance Gates - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-03-25
**Phase:** 5-windows-vulkan-hardening-and-performance-gates
**Areas discussed:** Windows validation matrix, Performance gate policy, Vulkan hardening scope, Backlog handoff format

---

## Windows validation matrix

### Q1: Which Windows paths are hard release gates for Phase 5?

| Option | Description | Selected |
|--------|-------------|----------|
| MSVC + Vulkan hard gate; MinGW + Vulkan smoke-only | Keep one hard gate and retain secondary signal from MinGW | ✓ |
| Both MSVC + Vulkan and MinGW + Vulkan are hard gates | Maximum cross-toolchain strictness | |
| MSVC + Vulkan hard gate; MinGW compile-only | Minimal secondary verification | |
| You decide | Delegate to planner/implementer | |

**User's choice:** MSVC + Vulkan hard gate; MinGW + Vulkan smoke-only.

### Q2: How strict should manual Windows Vulkan interaction smoke be?

| Option | Description | Selected |
|--------|-------------|----------|
| Full Phase-4 interaction checklist + scene nav + import/save sanity | Broad runtime behavior coverage | ✓ |
| Phase-4 interaction checklist only | Interaction-focused smoke only | |
| Minimal smoke (launch + nav + one drag) | Fast but low coverage | |
| You decide | Delegate strictness level | |

**User's choice:** Full smoke coverage.

### Q3: What evidence format should be archived?

| Option | Description | Selected |
|--------|-------------|----------|
| Command log + harness outputs + concise checklist results | Reproducible and reviewable evidence bundle | ✓ |
| Command log only | Minimal artifact set | |
| Checklist narrative only | Manual-oriented artifact | |
| You decide | Delegate artifact policy | |

**User's choice:** Command log + harness outputs + concise checklist results.

### Q4: If MSVC+Vulkan fails but macOS passes, what should Phase 5 do?

| Option | Description | Selected |
|--------|-------------|----------|
| Keep Phase 5 open until Windows gate is green | Preserve explicit Windows gate integrity | ✓ |
| Mark partial completion and carry blocker forward | Allow phased acceptance | |
| Allow completion for isolated non-core failures | Soften gate strictness | |
| You decide | Delegate completion policy | |

**User's choice:** Keep Phase 5 open until Windows gate is green.

---

## Performance gate policy

### Q1: Primary pass/fail rule for performance?

| Option | Description | Selected |
|--------|-------------|----------|
| Per-benchmark-case no-regression on each target | Fine-grained and strict gate | ✓ |
| Aggregate suite average only | Coarse gate; may hide case regressions | |
| Hybrid aggregate + key-case enforcement | Mixed granularity | |
| You decide | Delegate gate design | |

**User's choice:** Per-case gating.

### Q2: Numeric tolerance for "no regression"?

| Option | Description | Selected |
|--------|-------------|----------|
| <= 5% slower allowed | Practical cross-machine noise buffer | ✓ |
| <= 3% slower allowed | Stricter tolerance | |
| <= 10% slower allowed | Looser tolerance | |
| You decide | Delegate threshold selection | |

**User's choice:** <= 5% slowdown allowed.

### Q3: Rerun policy for marginal misses?

| Option | Description | Selected |
|--------|-------------|----------|
| One rerun; use better result | Limits noise while preserving strictness | ✓ |
| One rerun; use average of both | Smoother but can penalize jitter | |
| No reruns | Fastest but noise-sensitive | |
| You decide | Delegate rerun handling | |

**User's choice:** One rerun; use better result.

### Q4: Which bench cases are gating?

| Option | Description | Selected |
|--------|-------------|----------|
| All current bench cases in harness | Full hotspot coverage | ✓ |
| Interaction/render-adjacent subset only | Narrower gating | |
| Legacy-vs-cglm paired subset only | Minimal migration-focused gating | |
| You decide | Delegate scope | |

**User's choice:** All current bench cases.

---

## Vulkan hardening scope

### Q1: If Vulkan-only pick-readback hitches are found, what is scope?

| Option | Description | Selected |
|--------|-------------|----------|
| Fix in Phase 5 if reproducible | Hardening remains in-scope | ✓ |
| Defer unless functional failure | Performance-only regressions deferred | |
| Minimal mitigation only | Avoid structural fix in phase | |
| You decide | Delegate scope | |

**User's choice:** Fix in Phase 5 if reproducible.

### Q2: If full fix is too large, what fallback rule?

| Option | Description | Selected |
|--------|-------------|----------|
| Smallest safe mitigation + measured residual risk + block only if gates still fail | Pragmatic bounded fallback | ✓ |
| Defer entirely | No in-phase mitigation | |
| Keep implementing until full fix lands | Unbounded phase expansion risk | |
| You decide | Delegate fallback behavior | |

**User's choice:** Smallest safe mitigation with measured residual risk.

### Q3: Mandatory Windows runtime workflows to harden?

| Option | Description | Selected |
|--------|-------------|----------|
| Camera nav + pick/hover + gizmo axis/plane + vertex drag + undo/redo | Full interaction workflow coverage | ✓ |
| Camera nav + pick/hover + gizmo axis | Partial interaction coverage | |
| Camera nav + pick/hover only | Minimal runtime set | |
| You decide | Delegate workflow set | |

**User's choice:** Full interaction workflow coverage.

### Q4: How to treat non-Windows failures during gate runs?

| Option | Description | Selected |
|--------|-------------|----------|
| Track as collateral; non-blocking unless impacting Phase-5 requirements | Keep phase boundary intact | ✓ |
| Block on any native regression anywhere | Broad blocking scope | |
| Ignore non-Windows findings | Risk of losing useful signal | |
| You decide | Delegate collateral policy | |

**User's choice:** Track as collateral; block only if Phase-5 requirements affected.

---

## Backlog handoff format

### Q1: How structured should next-wave backlog capture be?

| Option | Description | Selected |
|--------|-------------|----------|
| Structured table per item: area, issue, evidence, impact, recommended phase | Planning-ready handoff | ✓ |
| Simple bullet list | Low structure | |
| Hybrid structured/blockers + bullet/minors | Medium structure | |
| You decide | Delegate format | |

**User's choice:** Structured table format.

### Q2: Which classes of items must be included?

| Option | Description | Selected |
|--------|-------------|----------|
| Long-tail migration + platform expansion + residual perf/hardening | Full next-wave scope | ✓ |
| Long-tail migration only | Narrow scope | |
| Platform expansion only | Narrow scope | |
| You decide | Delegate scope | |

**User's choice:** Include all three classes.

### Q3: How should priority be encoded?

| Option | Description | Selected |
|--------|-------------|----------|
| P1 must-next / P2 should-next / P3 future | Explicit triage | ✓ |
| Ordered list only | Implicit priority | |
| Two-tier next-vs-later | Coarser triage | |
| You decide | Delegate scheme | |

**User's choice:** Three-tier labels.

### Q4: Where should backlog be captured?

| Option | Description | Selected |
|--------|-------------|----------|
| 05-CONTEXT deferred section + mirrored summary in STATE notes | Dual visibility (phase + project state) | ✓ |
| 05-CONTEXT only | Phase-local visibility only | |
| Separate backlog doc only | Detached from phase context | |
| You decide | Delegate placement | |

**User's choice:** CONTEXT deferred section + STATE mirror.

---

## the agent's Discretion

- None explicitly delegated during discussion beyond implementation detail freedom captured in CONTEXT.md.

## Deferred Ideas

- None.
