# Phase 9: Long-Tail Validation, Performance Gates, and Boundary Finalization - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in `09-CONTEXT.md` — this log preserves the alternatives considered.

**Date:** 2026-03-28
**Phase:** 09-long-tail-validation-performance-gates-and-boundary-finalization
**Areas discussed:** Validation/performance closure posture (carried forward), Manual smoke closure scope, Thin-entrypoint boundary finalization

---

## Validation/performance closure posture

| Option | Description | Selected |
|--------|-------------|----------|
| Re-open strategy decisions | Re-discuss validation and perf gate policy from scratch | |
| Carry forward prior gate policy | Reuse established harness-first and per-case native perf gate posture from Phases 5-8 | ✓ |
| Minimal closure only | Reduce to basic compile checks and lightweight notes | |

**User's choice:** Implicit carry-forward based on immediate proceed request after Phase 8 closeout.
**Notes:** No new behavioral preferences were requested; prior locked gate posture remains authoritative for Phase 9.

---

## Manual smoke closure scope

| Option | Description | Selected |
|--------|-------------|----------|
| Subsystem-isolated checks | Validate each subsystem independently only | |
| End-to-end migration smoke | Run serializer/import/undo/editor workflows as integrated closure checks | ✓ |
| Defer manual smoke to post-milestone | Close with automation only | |

**User's choice:** Carry-forward implied end-to-end closure expectation (requested immediate next-phase continuation after successful Phase 8 UAT).
**Notes:** Aligns with `VAL-03` and prior phase evidence style.

---

## Thin-entrypoint boundary finalization

| Option | Description | Selected |
|--------|-------------|----------|
| Keep boundary undocumented | Rely on code state only | |
| Explicit minimal boundary docs | Document retained thin-entrypoint boundary, rationale, and consumer traceability | ✓ |
| Broad additional refactor | Extend into wider repo-wide cleanup | |

**User's choice:** Implicitly aligned with roadmap requirement (`TRED-02`) and phase goal.
**Notes:** Documentation finalization is required closure work; broad cleanup remains out of scope.

---

## the agent's Discretion

- Exact artifact templates and naming for Phase 9 evidence files.
- Exact compare/bench case additions needed to satisfy `VAL-01` and `VAL-02`.
- Exact sequencing between automation capture and manual smoke evidence capture.

## Deferred Ideas

- iOS and web platform expansion validation remain deferred (`PLAT-01`, `PLAT-02`).
- Repo-wide one-shot legacy helper removal remains deferred beyond Phase 9 boundary finalization.
