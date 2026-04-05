# Phase 19: Backfill Script/API Verification Artifacts - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-04-05
**Phase:** 19-backfill-script-api-verification-artifacts
**Areas discussed:** Evidence freshness, Manual evidence policy, Verification status normalization, Scope boundaries

---

## Evidence freshness

| Option | Description | Selected |
|--------|-------------|----------|
| Fresh targeted rerun + new verification docs | Re-run script/API-focused checks and produce new verification artifacts with fresh command evidence. | ✓ |
| Document-only backfill | Build verification docs from existing historical artifacts only, no fresh reruns. | |
| Full rebuild + full CTest rerun | Run complete rebuild/full test gates for closure evidence. | |

**User's choice:** Fresh targeted rerun + new verification docs.
**Notes:** Preferred for strong closure confidence without over-spending on broad reruns.

---

## Manual evidence policy

| Option | Description | Selected |
|--------|-------------|----------|
| Reuse prior approved manual evidence with explicit references | Carry forward already-approved manual checkpoints/UAT and cite artifacts directly. | ✓ |
| Require all-new manual reruns | Repeat all manual flows now before closure. | |
| Hybrid reruns | Re-run high-risk manual rows only; reuse the rest. | |

**User's choice:** Reuse prior approved manual evidence with explicit references.
**Notes:** Avoid redundant manual loops when acceptance is already captured.

---

## Verification status normalization

| Option | Description | Selected |
|--------|-------------|----------|
| Upgrade to passed with explicit cross-artifact evidence links | Allow status normalization when implementation + acceptance are already documented. | ✓ |
| Keep `human_needed` until new rerun | Require brand-new manual rerun for status change. | |
| Partial upgrade only | Upgrade script/API rows only, leave other legacy rows unchanged. | |

**User's choice:** Allow status upgrade to passed with explicit cross-artifact evidence links.
**Notes:** Any upgrades must be auditable and justified in verification artifacts.

---

## Scope boundaries

| Option | Description | Selected |
|--------|-------------|----------|
| Keep strictly to Phase 19 requirements only | Focus only on `SCRP-01..06` and `API-01..02` closure artifacts. | |
| Also pre-stage small prep notes for Phase 20/21 | Keep core scope strict, but include concise downstream handoff notes. | ✓ |
| Add broader milestone cleanup | Opportunistically include broader closure work if quick. | |

**User's choice:** Also pre-stage small prep notes for Phase 20/21.
**Notes:** Prep notes are informational only and must not expand Phase 19 implementation scope.

---

## the agent's Discretion

- Exact targeted command sequence and verification doc layout.
- Exact wording of status-upgrade rationale and cross-links.

## Deferred Ideas

- Broad milestone cleanup outside Phase 19 requirement set is deferred to later closure phases.
