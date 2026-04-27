# Phase 39: Automatic Observer Safety Loop - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in `39-CONTEXT.md` — this log preserves alternatives considered.

**Date:** 2026-04-27
**Phase:** 39-automatic-observer-safety-loop
**Areas discussed:** Auto-refresh opt-in behavior

---

## Auto-refresh opt-in behavior

| Option | Description | Selected |
|--------|-------------|----------|
| Auto-enable immediately when link is active | Observe loop starts as soon as source is linked | ✓ |
| Require explicit Observe ON toggle after linking | Link only stores source; observe starts later | |
| Start OFF on first link, then remember preference | Conservative first-run behavior | |

**User's choice:** Auto-enable immediately when link is active.
**Notes:** Keep flat roots low-friction for live preview while preserving ability to disable observe separately.

---

## Link OFF behavior

| Option | Description | Selected |
|--------|-------------|----------|
| Stop observing immediately, keep source path | Quick re-enable without re-browse | ✓ |
| Stop observing and clear source path | Treat unlink as full detach | |
| Stop after current cycle completes | Graceful delayed stop | |

**User's choice:** Stop observing immediately, keep source path.
**Notes:** Unlink must be immediate and non-destructive to path metadata.

---

## Observe control surface

| Option | Description | Selected |
|--------|-------------|----------|
| Expose separate Observe toggle next to Link | Link and observe are independently controllable | ✓ |
| Link implies observing always | Simpler but less control | |
| Keep observe internal/hidden | No explicit operator control | |

**User's choice:** Expose separate Observe toggle next to Link.
**Notes:** Flat inspector must explicitly surface auto-loop state.

---

## Missing/invalid source while Observe is ON

| Option | Description | Selected |
|--------|-------------|----------|
| Keep observe ON but idle with warning, no retries consumed | Non-destructive waiting state | ✓ |
| Consume retries until auto-disabled | Treat as repeated failure | |
| Auto-turn observe OFF immediately | Hard fail-fast behavior | |

**User's choice:** Keep observe ON but idle with warning and do not consume retry budget.
**Notes:** Missing/invalid path is an operator-fixable setup state, not runtime failure churn.

---

## the agent's Discretion

- Exact microcopy and layout for flat inspector safety controls.
- Coalescing strategy for file-change events that arrive during in-flight flat refresh.
- Whether interval/retry tuning controls are directly exposed in this phase UI or preserved as defaults.

## Deferred Ideas

- Large-dump responsiveness/interaction hardening across repeated refreshes (Phase 40).
