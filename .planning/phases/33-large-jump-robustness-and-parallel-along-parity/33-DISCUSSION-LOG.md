# Phase 33: Large-Jump Robustness and PARALLEL/ALONG Parity - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-04-10
**Phase:** 33-large-jump-robustness-and-parallel-along-parity
**Areas discussed:** Large-jump solve policy and rollback semantics, PARALLEL vs ALONG parity model, deterministic diagnostics taxonomy

---

## Large-jump solve policy and rollback semantics

| Option | Description | Selected |
|--------|-------------|----------|
| Two-stage solve: large-jump preprojection + normal iterative passes | Add staged preprojection for large deltas before normal solver passes to reduce latch failures while preserving existing pass logic. | ✓ |
| Keep single-stage solver, only increase/adapt pass budget and tolerances | Preserve architecture and tune iteration/tolerance parameters only. | |
| Strict fixed budget/tolerances; improve only constraint ordering and anchor selection | Avoid tolerance/pass changes; improve ordering/anchor heuristics only. | |

**User's choice:** Two-stage solve: large-jump preprojection + normal iterative passes.
**Notes:** User aligned with solving observed "wiggle to latch" failures using staged convergence rather than pure budget tuning.

---

## Failure contract for unsatisfied large-jump edits

| Option | Description | Selected |
|--------|-------------|----------|
| Hard transactional rollback to last valid state + explicit failure implication + immediate edit responsiveness | On unsatisfied large-jump, restore last valid state, emit explicit diagnostics/implications, and keep next edit responsive. | ✓ |
| Keep partial progress if residual improved, but flag warning diagnostics | Preserve best-effort geometry even if unsatisfied. | |
| Auto-retry once with relaxed tolerances before deciding rollback | Add fallback retry branch before rollback decision. | |

**User's choice:** Hard transactional rollback to last valid state + explicit failure implication + immediate edit responsiveness.
**Notes:** This reinforces prior transactional no-partial-mutation posture and stale-lock prevention expectations.

---

## PARALLEL vs ALONG parity model

| Option | Description | Selected |
|--------|-------------|----------|
| Enforce geometric-equivalence parity policy for feasibility/outcome classes | Equivalent PARALLEL and ALONG setups should classify and behave consistently. | ✓ |
| Allow family-specific behavior as long as deterministic | Different outcomes permitted if internally deterministic per family. | |
| Enforce parity only for mirrored cases | Limit parity guarantees to mirrored subsets. | |

**User's choice:** Enforce geometric-equivalence parity policy for feasibility/outcome classes.
**Notes:** Parity requirement applies broadly to equivalent setups, not mirrored-only special cases.

---

## Deterministic diagnostics taxonomy

| Option | Description | Selected |
|--------|-------------|----------|
| Deterministic family+reason taxonomy with stable ordering | Introduce stable reason classes (large-jump unsatisfied, parity mismatch, max-passes) and deterministic ordering. | ✓ |
| Minimal user-facing message, detailed taxonomy only in debug logs | Keep UI diagnostics coarse and move detail to logs. | |
| Keep current text, only ensure deterministic ordering | Preserve current messages without extending reason taxonomy. | |

**User's choice:** Deterministic family+reason taxonomy with stable ordering.
**Notes:** Taxonomy detail is expected to remain actionable, not debug-only.

---

## the agent's Discretion

- Exact low-level two-stage preprojection math and pass transition mechanics.
- Exact diagnostic message wording and fixture distribution, while preserving deterministic taxonomy and ordering.

## Deferred Ideas

- Deterministic milestone closure rerun sign-off (`DIAG-03`) deferred to Phase 34 per roadmap.
