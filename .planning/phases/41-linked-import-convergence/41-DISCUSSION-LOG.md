# Phase 41: Linked Import Convergence - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-05-05
**Phase:** 41-linked-import-convergence
**Areas discussed:** Observer start timing, Steady-state visibility and counts, Failure fallback on initial link setup, Acceptance anchor for the fix

---

## Observer start timing

| Option | Description | Selected |
|--------|-------------|----------|
| Only after stamping the just-imported file as the baseline | First automatic observe waits for a real later file change; the initial linked import cannot self-refresh. | ✓ |
| Immediately after import, but any self-refresh must stay invisible and transactional | Preserve immediate observe semantics while hiding any same-file rerun from the user. | |
| You decide | Leave the startup policy to planning and implementation. | |

**User's choice:** Only after stamping the just-imported file as the baseline.
**Notes:** The initial linked import must not auto-refresh itself on startup.

---

## Steady-state visibility and counts

| Option | Description | Selected |
|--------|-------------|----------|
| Keep the last committed full geometry and final Scene Hierarchy totals stable with no late visible churn | After the first linked import commits, both viewport and totals stay fixed until a real file change occurs later. | ✓ |
| Allow counts/status to keep updating after the first visible result, but never let visible geometry regress | Background linked work may still change totals as long as the visible result stays complete. | |
| You decide | Leave steady-state presentation rules to planning and implementation. | |

**User's choice:** Keep the last committed full geometry and final Scene Hierarchy totals stable with no late visible churn.
**Notes:** Once the first linked import commits, both the viewport and Scene Hierarchy totals must remain stable until the source file changes.

---

## Failure fallback on initial link setup

| Option | Description | Selected |
|--------|-------------|----------|
| Keep the imported geometry, preserve the source path, turn automatic observe OFF, and show a warning | Preserve the successful import as last-good content even if baseline arming fails. | ✓ |
| Fail the whole linked import and roll back the import result | Treat linkage/setup failure as import failure and force the user to retry. | |
| Keep geometry and leave automatic observe ON so it keeps retrying in the background | Preserve import output but continue background retries immediately. | |

**User's choice:** Keep the imported geometry, preserve the source path, turn automatic observe OFF, and show a warning.
**Notes:** The source stays attached, but automatic observe drops to a safe OFF state until the user explicitly re-enables it.

---

## Acceptance anchor for the fix

| Option | Description | Selected |
|--------|-------------|----------|
| Deterministic automated regression coverage plus manual confirmation against lamp_11.jsonl and the slot-buffer/hierarchy invariants | Use both reproducible in-tree tests and the real large-file repro as the closure gate. | ✓ |
| Manual confirmation against lamp_11.jsonl is the main gate; automated coverage can stay lighter | Lean on the real repro file as the dominant acceptance proof. | |
| Deterministic automated coverage is enough; the real file can stay ad-hoc | Keep the real file as a debugging aid, not a required closure step. | |

**User's choice:** Deterministic automated regression coverage plus manual confirmation against lamp_11.jsonl and the slot-buffer/hierarchy invariants.
**Notes:** Phase 41 should close with both automated proof and a real-file confirmation pass.

---

## the agent's Discretion

- Exact source-state stamping hook location in the linked flat import completion flow
- Exact warning copy and UI surface for baseline-arming fallback
- Exact automated fixture composition and invariant helper layout

## Deferred Ideas

- Refresh-after-real-file-change hardening remains Phase 42.
- Linked-root delete and teardown cleanup remain Phase 42.
- Incremental diff/patch refresh architecture remains future-scope work beyond v1.7.
