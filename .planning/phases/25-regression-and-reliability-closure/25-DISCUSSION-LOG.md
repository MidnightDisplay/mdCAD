# Phase 25: Regression and Reliability Closure - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-04-08
**Phase:** 25-regression-and-reliability-closure
**Areas discussed:** Regression test matrix scope, Failure policy for non-green runs, Evidence/provenance artifact expectations, Out-of-scope guardrails

---

## Regression test matrix scope

| Option | Description | Selected |
|--------|-------------|----------|
| Current targeted solver+script gate only | Keep closure on focused reliability gate already used in phase validation loops. | ✓ |
| Expand to include full `ctest` suite | Widen closure to entire repository test set. | |
| Tiered: fast gate + extended nightly gate | Keep fast closure gate and define a broader non-blocking tier. | |
| You decide | Delegate matrix decision to the agent. | |

**User's choice:** Current targeted solver+script gate only
**Notes:** User accepted recommended baseline posture and then further constrained scope to keep exactly the current 7-test target set.

| Option | Description | Selected |
|--------|-------------|----------|
| Strict pass/fail only | Any failing gate test blocks closure. | ✓ |
| Allow documented known-fail quarantine | Permit closure with curated known failures. | |
| You decide | Delegate policy to the agent. | |

**User's choice:** Strict pass/fail only
**Notes:** No known-fail quarantine for Phase 25.

| Option | Description | Selected |
|--------|-------------|----------|
| Keep exactly current 7-test set | Freeze closure gate to current named target set. | ✓ |
| Add one extra sentinel now (`mdCAD` build target) | Extend gate with one extra stability check. | |
| You decide | Delegate to the agent. | |

**User's choice:** Keep exactly current 7-test set
**Notes:** Baseline gate should stay scoped and stable unless failure triage forces additional checks.

---

## Failure policy for non-green runs

| Option | Description | Selected |
|--------|-------------|----------|
| Stop and fix in same phase before closure | Any gate failure must be resolved in Phase 25. | ✓ |
| Log gap and defer to follow-up phase | Carry failures forward as planned debt. | |
| You decide | Delegate to the agent. | |

**User's choice:** Stop and fix in same phase before closure
**Notes:** Closure is blocked until gate is green.

| Option | Description | Selected |
|--------|-------------|----------|
| Treat as failure; stabilize or isolate with deterministic test rewrite before closure | Flaky behavior must be eliminated before closure. | ✓ |
| Permit rerun-until-pass | Allow retry loops to pass gate. | |
| Mark flaky and continue | Permit closure with known flaky tests. | |
| You decide | Delegate to the agent. | |

**User's choice:** Treat as failure; stabilize or isolate with deterministic test rewrite before closure
**Notes:** Reliability requirement explicitly includes determinism, so flakiness is a blocking defect.

| Option | Description | Selected |
|--------|-------------|----------|
| Yes, preserve explicit diagnostics as mandatory | Keep explicit family diagnostics as closure requirement. | ✓ |
| No, only pass/fail status matters | Ignore diagnostics quality for closure. | |
| You decide | Delegate to the agent. | |

**User's choice:** Yes, preserve explicit diagnostics as mandatory
**Notes:** Diagnostics fidelity is part of reliability, not optional.

---

## Evidence/provenance artifact expectations

| Option | Description | Selected |
|--------|-------------|----------|
| Command + result summary in verification docs only | Keep evidence lightweight and reproducible in phase docs. | ✓ |
| Store full raw command outputs as files under phase evidence/ | Preserve raw logs as first-class artifacts. | |
| Both summary and raw output files | Require both compact and raw evidence. | |
| You decide | Delegate to the agent. | |

**User's choice:** Command + result summary in verification docs only
**Notes:** Raw output files not required by default.

| Option | Description | Selected |
|--------|-------------|----------|
| Yes, fresh rerun required at closure | Require final closure-time rerun even if earlier runs were green. | ✓ |
| No, reuse latest green run in phase | Allow previously green evidence for closure. | |
| You decide | Delegate to the agent. | |

**User's choice:** Yes, fresh rerun required at closure
**Notes:** Closure evidence must be fresh.

| Option | Description | Selected |
|--------|-------------|----------|
| Automation-first; manual only when a regression demands it | Manual checks are exception-triggered only. | ✓ |
| Always require a manual UAT pass | Manual step is always mandatory. | |
| Automation-only, never manual | Disallow manual confirmation paths entirely. | |
| You decide | Delegate to the agent. | |

**User's choice:** Automation-first; manual only when a regression demands it
**Notes:** Manual verification remains conditional, not baseline.

---

## Out-of-scope guardrails

| Option | Description | Selected |
|--------|-------------|----------|
| No new constraint features; regression closure only | Keep phase strictly as reliability closure. | ✓ |
| Allow small feature tweaks if discovered during regression | Permit opportunistic scope expansion. | |
| You decide | Delegate to the agent. | |

**User's choice:** No new constraint features; regression closure only
**Notes:** Feature work is explicitly out of scope in Phase 25.

| Option | Description | Selected |
|--------|-------------|----------|
| Record as deferred/backlog, keep Phase 25 unchanged | New ideas get captured but not implemented in this phase. | ✓ |
| Allow scope expansion if effort seems small | Pull in small extras ad hoc. | |
| You decide | Delegate to the agent. | |

**User's choice:** Record as deferred/backlog, keep Phase 25 unchanged
**Notes:** Scope creep should be captured, not executed.

| Option | Description | Selected |
|--------|-------------|----------|
| Stay on established Windows Vulkan gate for closure | Use current primary platform gate for reliable closure. | ✓ |
| Expand to cross-platform in this phase | Add platform expansion to Phase 25. | |
| You decide | Delegate to the agent. | |

**User's choice:** Stay on established Windows Vulkan gate for closure
**Notes:** Cross-platform expansion is deferred beyond this phase boundary.

---

## the agent's Discretion

- Exact organization of plan waves/tasks for Phase 25.
- Exact formatting style for verification summary evidence, as long as command and result are explicit.

## Deferred Ideas

- None captured during this discussion loop.

