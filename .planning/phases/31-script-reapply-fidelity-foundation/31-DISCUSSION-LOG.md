# Phase 31: Script Reapply Fidelity Foundation - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md - this log preserves the alternatives considered.

**Date:** 2026-04-10
**Phase:** 31-script-reapply-fidelity-foundation
**Areas discussed:** Participant descriptor contract (roles/sub-index), Color preservation semantics, Reapply/remap transaction behavior, Determinism verification contract

---

## Participant descriptor contract (roles/sub-index)

| Option | Description | Selected |
|--------|-------------|----------|
| Yes - strict descriptor contract with explicit failure | Require role/sub-index descriptor fidelity; fail when missing/ambiguous | ✓ |
| Allow legacy entity-only fallback for known-safe cases | Accept entity-only participant fallback under constrained cases | |
| Mixed mode: strict for ARCI families, fallback for others | Enforce descriptor strictness only for subset families | |

**User's choice:** Yes - strict descriptor contract with explicit failure.
**Notes:** User explicitly rejected degradation into entity-only fallback semantics.

---

## Color preservation semantics

| Option | Description | Selected |
|--------|-------------|----------|
| Yes - exact color roundtrip for script-managed entities only | Re-apply restores script color exactly for script-owned geometry and leaves non-script entities untouched | ✓ |
| Apply script color only at first import, preserve runtime edits thereafter | Re-apply would avoid overwriting post-import color edits | |
| Only preserve sketch-level color, not per-entity color | Coarse-grained color persistence at sketch level only | |

**User's choice:** Yes - exact color roundtrip for script-managed entities only.
**Notes:** Scope covers points/lines/arcs under script management only.

---

## Reapply/remap transaction behavior

| Option | Description | Selected |
|--------|-------------|----------|
| Yes - atomic reapply with deterministic failure taxonomy | Any unresolved id/illegal signature/metadata mismatch causes full rollback and explicit deterministic error | ✓ |
| Allow partial apply of valid entities/constraints and report skipped items | Commit surviving items and skip broken ones | |
| Atomic for geometry only; constraints may partially apply | Keep geometry transactional but permit partial constraint commit | |

**User's choice:** Yes - atomic reapply with deterministic failure taxonomy.
**Notes:** User locked no-partial-mutation behavior for this phase.

---

## Determinism verification contract

| Option | Description | Selected |
|--------|-------------|----------|
| Yes - lock deterministic repeated-apply + rerun parity gate | Validate repeated apply in-process plus parity on fresh reruns for emitted script and key diagnostics | ✓ |
| Single-process repeated apply only | Check determinism only inside one process lifetime | |
| Only assert no crashes/errors, not parity | Minimal reliability checks without strict deterministic parity | |

**User's choice:** Yes - lock deterministic repeated-apply + rerun parity gate.
**Notes:** User rejected weak non-parity validation for phase closure quality.

---

## the agent's Discretion

- Exact internal descriptor persistence strategy and fixture construction.
- Exact deterministic assertion structure, as long as parity gate intent is preserved.

## Deferred Ideas

None.
