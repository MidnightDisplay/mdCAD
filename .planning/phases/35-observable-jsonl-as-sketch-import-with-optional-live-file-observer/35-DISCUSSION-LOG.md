# Phase 35: Observable JSONL as sketch import with optional live file observer - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-04-12
**Phase:** 35-observable-jsonl-as-sketch-import-with-optional-live-file-observer
**Areas discussed:** Observer lifecycle defaults, Re-parse overwrite policy, UI placement, File-lock retry behavior, Script and metadata contract

---

## Observer lifecycle defaults

| Option | Description | Selected |
|--------|-------------|----------|
| Create link with Observe file ON by default | Linked import starts observing immediately. | ✓ |
| Create link with Observe file OFF by default | Link exists but user must enable observation manually. | |
| Do not create observer link unless user explicitly enables it during import | Observer is strictly opt-in during import flow. | |

**User's choice:** Create link with Observe file ON by default.
**Notes:** Default ON is desired baseline.

| Option | Description | Selected |
|--------|-------------|----------|
| Fail import transactionally (no sketch created) | Any observer setup failure aborts import cleanly. | ✓ |
| Keep sketch imported but without observer link, with warning | Import succeeds in degraded mode. | |
| Keep sketch imported and retry observer setup in background | Import succeeds and link attempts continue asynchronously. | |

**User's choice:** Fail import transactionally.
**Notes:** No partial success on observer setup failure.

| Option | Description | Selected |
|--------|-------------|----------|
| Yes, persist per linked sketch | Save/load keeps observe toggle and rate-limit settings. | ✓ |
| No, reset to defaults each app run | Observer settings are session-only. | |
| Persist only Observe ON/OFF, not rate-limit | Keep only partial setting persistence. | |

**User's choice:** Persist per linked sketch.
**Notes:** Full per-sketch persistence required.

---

## Re-parse overwrite policy

| Option | Description | Selected |
|--------|-------------|----------|
| Authoritative replace | Reparse fully overwrites linked sketch state. | ✓ |
| Merge heuristically | Attempt to preserve manual edits where possible. | |
| Prompt user on conflicts | Interactive conflict handling per reparse event. | |

**User's choice:** Authoritative replace.
**Notes:** JSONL is source of truth at reparse boundaries.

| Option | Description | Selected |
|--------|-------------|----------|
| Yes — reset to JSONL-derived geometry only | Remove user-added geometry/constraints on reparse. | ✓ |
| Keep user-added extras | Replace imported subset only. | |
| Block reparse when additions exist | Require cleanup before reparse. | |

**User's choice:** Reset to JSONL-derived geometry only.
**Notes:** Full replacement semantics.

| Option | Description | Selected |
|--------|-------------|----------|
| Keep last good imported sketch state; report error | Do not destroy valid current sketch on failed reparse. | ✓ |
| Clear sketch on failure | Remove potentially stale data aggressively. | |
| Disable observer after first failure | Stop observing immediately after one failure. | |

**User's choice:** Keep last good state and report error.
**Notes:** Failure should be visible but non-destructive.

---

## UI placement

| Option | Description | Selected |
|--------|-------------|----------|
| Entity Inspector sketch section | Observer controls live with sketch details. | ✓ |
| Script IO window | Observer controls colocated with script inputs/outputs. | |
| Separate global observer window | Centralized observer management panel. | |

**User's choice:** Inspector sketch section when inactive; move controls to top of Active Sketch Workspace when sketch is active.
**Notes:** User explicitly requested active-workspace promotion while active.

| Option | Description | Selected |
|--------|-------------|----------|
| Show controls only in Active Sketch Workspace while active | Avoid duplicate editable surfaces. | ✓ |
| Show synchronized controls in both places | Dual-edit surfaces. | |
| Read-only summary in Inspector, editable in Workspace | Split view/edit responsibilities. | |

**User's choice:** Show controls only in Active Sketch Workspace while active.
**Notes:** No duplicate editor surfaces during active sketch workflow.

| Option | Description | Selected |
|--------|-------------|----------|
| Show two most recent messages | Rolling short message history. | ✓ |
| Highest severity first, then recent | Severity-prioritized two-line area. | |
| Show only latest message | Single-line latest status only. | |

**User's choice:** Show two most recent messages.
**Notes:** Matches requested two-line status area behavior.

---

## File-lock retry behavior

| Option | Description | Selected |
|--------|-------------|----------|
| Keep observer ON, wait for next change cycle, show warning | Non-disabling fallback behavior. | |
| Auto-toggle observer OFF and require manual re-enable | Explicitly stops auto-observe after retry exhaustion. | ✓ |
| Retry forever immediately | Unbounded retry loop. | |

**User's choice:** Auto-toggle observer OFF and require manual re-enable.
**Notes:** User overrode recommended default here.

| Option | Description | Selected |
|--------|-------------|----------|
| Use observer rate-limit as retry debounce | Single timing control for observe + retry loop. | ✓ |
| Use separate fixed retry delay | Hidden fixed retry cadence. | |
| Add second user-configurable retry delay control | Additional UI control for retry cadence. | |

**User's choice:** Use observer rate-limit as retry debounce.
**Notes:** Reuses existing 1-1000 ms slider.

| Option | Description | Selected |
|--------|-------------|----------|
| Manual Re-parse remains available | One-shot reparse allowed even if auto-observe is OFF. | ✓ |
| Require Observe ON first | No reparse while observe OFF. | |
| Manual Re-parse auto-enables Observe | Reparse also resumes continuous observation. | |

**User's choice:** Manual Re-parse remains available.
**Notes:** Preserve operator recovery path.

---

## Script and metadata contract

| Option | Description | Selected |
|--------|-------------|----------|
| Generated-script-only authority | Script always derived; no interim authority. | |
| Script-primary authority | Script edits are source of truth and observer adapts. | |
| Dual-authoritative mode | Conflict-managed two-way authority. | |

**User's choice:** Hybrid rule — script is authoritative between successful reparses; each successful reparse recreates sketch from JSONL and script follows recreated sketch.
**Notes:** Explicit free-text lock from user; not a simple preset option.

| Option | Description | Selected |
|--------|-------------|----------|
| Enforce filename/path label contract and update on relink | Name = filename sans extension; description = full path. | ✓ |
| Set labels once only | Never auto-update later. | |
| Preserve user label edits always | Import/relink never overwrite labels. | |

**User's choice:** Enforce strict filename/path label contract and update on relink.
**Notes:** Required deterministic metadata linkage behavior.

| Option | Description | Selected |
|--------|-------------|----------|
| Show explicit overwrite warning near script/observer controls | User informed that next reparse can overwrite script edits. | ✓ |
| No warning | Implicit behavior only. | |
| Block script editing while observer ON | Hard guard against edits during active observe. | |

**User's choice:** Show explicit warning.
**Notes:** Keep editing possible, but make overwrite behavior explicit.

---

## the agent's Discretion

- Internal ECS/component storage shape for observer metadata.
- Exact UI wording and styling for warning/error labels.
- Exact transaction plumbing for full sketch replacement and script regeneration.

## Deferred Ideas

None.
