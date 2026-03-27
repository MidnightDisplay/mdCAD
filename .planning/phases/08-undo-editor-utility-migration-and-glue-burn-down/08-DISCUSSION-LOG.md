# Phase 8: Undo/Editor Utility Migration and Glue Burn-Down - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in `08-CONTEXT.md` — this log preserves alternatives considered.

**Date:** 2026-03-27
**Phase:** 08-undo-editor-utility-migration-and-glue-burn-down
**Areas discussed:** Transform migration policy, Glue burn-down scope, Validation depth

---

## Transform migration policy

| Option | Description | Selected |
|--------|-------------|----------|
| Strict parity only | Preserve exact pre-migration behavior | |
| Parity-first with correctness exceptions | Allow fixes when objectively wrong, with explicit evidence | |
| Correctness-first | Optimize correctness even if behavior shifts | ✓ |

**User's choice:** Correctness-first — optimize correctness even if behavior shifts.
**Follow-up choice:** Allow behavior shifts when mathematically justified and documented in validation evidence.
**Notes:** This phase may intentionally diverge from previous behavior where prior math is incorrect, provided evidence captures rationale and observed delta.

---

## Glue burn-down scope

| Option | Description | Selected |
|--------|-------------|----------|
| Conservative | Remove only glue proven unused by migrated paths | |
| Balanced | Remove with strong evidence; retain explicit temporary shims for uncertainty | |
| Aggressive | Remove most temporary glue in Phase 8 unless immediate blocker appears | ✓ |

**User's choice:** Aggressive glue burn-down.
**Follow-up choice:** If regressions occur, carry-over to Phase 9 is allowed if documented.
**Notes:** The phase should still surface impact and rationale for any deferred cleanup or regressions.

---

## Validation depth

| Option | Description | Selected |
|--------|-------------|----------|
| Light targeted checks | Compile + focused undo/editor parity workflows | ✓ |
| Moderate matrix | Targeted checks + extra representative variants | |
| Broad matrix | Near-Phase-9 parity depth in Phase 8 | |

**User's choice:** Light targeted checks.
**Mandatory workflows selected:** Undo/redo transform edits + gizmo vertex edit + inspector edits.
**Notes:** Validation remains focused and practical for Phase 8 while still covering key user-facing workflows.

---

## the agent's Discretion

- Exact helper API naming and placement under `src/math/`.
- Exact sequencing of glue removals and safety checks inside phase plans.
- Exact evidence file structure, provided required workflows and behavior-delta notes are captured.

## Deferred Ideas

- Broader editor UX contract discussion was intentionally not expanded in this discuss session.
- Any unresolved glue-removal regressions may be deferred to Phase 9 with explicit documentation.
