# Phase 40: Large-Dump Stability & Interaction Coherence - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-04-27
**Phase:** 40-large-dump-stability-interaction-coherence
**Areas discussed:** performance gate model, burst refresh scheduling, interaction coherence policy, verification scope

---

## Performance gate model

| Option | Description | Selected |
|--------|-------------|----------|
| Hybrid gate | Hard no-lockup/no-crash requirement, with timing budgets captured as warnings | ✓ |
| Behavioral-only gate | Require only no-lockup/no-crash behavior | |
| Hard numeric gate | Fail phase when fixed timing thresholds are exceeded | |

**User's choice:** Hybrid gate (recommended).
**Notes:** Timing evidence should be collected, but numeric overruns should not block completion in this phase.

---

## Burst refresh scheduling

| Option | Description | Selected |
|--------|-------------|----------|
| Single-flight + coalesced pending rerun | One refresh running; collapse burst updates into one follow-up rerun | ✓ |
| Single-flight drop policy | Ignore changes while refresh is active and wait for next interval | |
| Queued policy | Allow multiple queued refresh jobs per anchor | |

**User's choice:** Single-flight with coalesced pending rerun (recommended).
**Notes:** Avoid refresh thrash and queue growth while preserving eventual update application.

---

## Interaction coherence policy

| Option | Description | Selected |
|--------|-------------|----------|
| Anchor-stable policy | Keep root stable, remap replaced-child selection to root, preserve visibility/inspector continuity | ✓ |
| Strict remap policy | Attempt to map old selections to equivalent new child entities | |
| Reset policy | Clear selection and collapse visibility after each refresh | |

**User's choice:** Anchor-stable policy (recommended).
**Notes:** Root identity remains the stable interaction contract over repeated replacements.

---

## Verification scope

| Option | Description | Selected |
|--------|-------------|----------|
| Tiered fixture suite | Deterministic small/medium/large C tests + one manual very-large stress pass | ✓ |
| Automated-only | Deterministic fixtures only, no manual stress pass | |
| Manual-heavy | Minimal automation, rely mostly on manual stress checks | |

**User's choice:** Tiered fixture suite (recommended).
**Notes:** Manual very-large pass complements deterministic native test coverage.

---

## the agent's Discretion

- Exact fixture sizing and composition details.
- Exact implementation details for pending-rerun coalescing internals.
- Exact advisory timing telemetry formatting.

## Deferred Ideas

- Hard numeric SLA gates for phase pass/fail.
- Incremental diff/patch refresh architecture.
- Broader backend parity performance matrix.
