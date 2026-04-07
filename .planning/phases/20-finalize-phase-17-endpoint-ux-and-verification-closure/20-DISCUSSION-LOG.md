# Phase 20: Finalize Phase 17 Endpoint UX and Verification Closure - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in `20-CONTEXT.md` — this log preserves the alternatives considered.

**Date:** 2026-04-07
**Phase:** 20-finalize-phase-17-endpoint-ux-and-verification-closure
**Areas discussed:** evidence strategy, artifact structure, closure strictness, scope guardrails, manual sign-off policy

---

## Evidence strategy (freshness/reuse policy)

| Option | Description | Selected |
|--------|-------------|----------|
| Fresh targeted reruns + fresh manual checkpoint, while citing prior UAT | Re-run key evidence broadly and use prior evidence as support | |
| Primarily reuse existing `17-UAT` + prior command evidence with minimal reruns | Reuse accepted evidence by default; rerun only where needed | ✓ |
| Require broad/full-suite reruns for every closure row | Strictest rerun posture for all rows | |

**User's choice:** Primarily reuse existing `17-UAT` + prior command evidence with minimal reruns.
**Notes:** Closure should not require broad reruns unless ambiguity demands it.

## Citation strictness for reused evidence

| Option | Description | Selected |
|--------|-------------|----------|
| Explicit per-row citations | Each requirement row links to exact source file/section | ✓ |
| Aggregate citation block | One shared citation section per artifact | |
| No strict citation format | Flexible references | |

**User's choice:** Require explicit per-row citations.
**Notes:** Traceability clarity is mandatory for audit-grade closure.

## Ambiguous-row closure policy

| Option | Description | Selected |
|--------|-------------|----------|
| Escalate ambiguous rows to fresh targeted rerun/manual recheck | No pass without clear evidence for the row | ✓ |
| Best-effort pass with rationale | Allow pass despite unresolved ambiguity | |
| Leave ambiguous rows partial and defer | Push unresolved ambiguity to Phase 21 | |

**User's choice:** Escalate ambiguous rows to fresh targeted rerun/manual recheck.
**Notes:** `passed` status requires unambiguous evidence.

## Verification artifact shape

| Option | Description | Selected |
|--------|-------------|----------|
| `17-VERIFICATION.md` authoritative, `17-VALIDATION.md` supporting, `17-UAT.md` preserved | Three-artifact closure model with clear role separation | ✓ |
| Update only `17-VALIDATION.md` + `17-UAT.md` | Skip formal Phase 17 verification artifact | |
| New Phase 20-only verification doc | Avoid touching Phase 17 verification artifacts | |

**User's choice:** Create/update `17-VERIFICATION.md` as authoritative closure; keep `17-VALIDATION.md` supporting; preserve `17-UAT.md`.
**Notes:** Authoritative closure remains attached to Phase 17 requirement ownership.

## Closure strictness across D-01..D-12

| Option | Description | Selected |
|--------|-------------|----------|
| Explicit row for every `D-01..D-12` with evidence links | Full requirement-level matrix | ✓ |
| Only endpoint subset (`D-09..D-12`) | Focus only previously open endpoint rows | |
| Group-level summary only | No per-requirement rows | |

**User's choice:** Explicit row for every `D-01..D-12` with evidence links.
**Notes:** Full phase verification completeness is required.

## Scope guardrails during closure

| Option | Description | Selected |
|--------|-------------|----------|
| Docs-first closure; minimal targeted code fixes only for reproducible failing required checks | Controlled closure with implementation fallback | ✓ |
| Allow opportunistic endpoint UX polish | Broaden implementation changes during closure | |
| No code changes at all | Artifact-only closure regardless of failures | |

**User's choice:** Docs-first closure with minimal targeted code fixes only if required checks fail reproducibly.
**Notes:** Avoid feature creep while still permitting unblock fixes.

## Manual sign-off checklist policy

| Option | Description | Selected |
|--------|-------------|----------|
| Reuse `17-UAT` tests 1-7 as mandatory set; fresh rerun only for ambiguous rows | Baseline manual contract with targeted escalation | ✓ |
| Endpoint-focused subset only | Reduced manual checklist scope | |
| New shorter checklist | Replace existing UAT baseline | |

**User's choice:** Reuse `17-UAT` tests 1-7 as mandatory set, with fresh reruns only for ambiguous rows.
**Notes:** Existing approved UAT remains canonical unless ambiguity reopens a row.

---

## the agent's Discretion

- Minimal targeted rerun command selection for ambiguous rows.
- Final verification table/citation formatting within required row-level traceability rules.
- Determination of whether ambiguity escalation needs command rerun, manual rerun, or both.

## Deferred Ideas

- Opportunistic endpoint UX polish beyond closure-driven fixes (deferred to future phase).
- Non-Phase-17 residual traceability cleanup (deferred to Phase 21 scope).
