# Phase 36: Flat Import Entry & Configuration - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-04-27
**Phase:** 36-flat-import-entry-configuration
**Areas discussed:** Entry point, dialog contract, observer opt-in UX, import-start behavior

---

## Entry point and naming

| Option | Description | Selected |
|--------|-------------|----------|
| Add third explicit action: `Import JSONL (Flat Large Dump)...` | Separate flow for large flat imports, keeps existing actions intact | ✓ |
| Fold into existing `Import JSONL Geometry Log...` dialog as mode switch | One action, mode-based split in popup | |
| Replace existing geometry-log action | New flow supersedes old path | |

**User's choice:** Add third explicit action.
**Notes:** User wants clear explicit discoverability for the large-dump pathway.

---

## Dialog contract

| Option | Description | Selected |
|--------|-------------|----------|
| Mirror existing JSONL options + observer opt-in | Units, colors/default color, shift-to-CoM, XYZ rotation, mesh mode, plus observer toggle | ✓ |
| Minimal dialog | Units + observer only | |
| Advanced dialog with new performance knobs | Expanded controls beyond current options | |

**User's choice:** Mirror existing JSONL options plus observer opt-in.
**Notes:** Preserve familiarity and avoid introducing extra control complexity in this phase.

---

## Observer opt-in UX

| Option | Description | Selected |
|--------|-------------|----------|
| Show optional link toggle default OFF and persist choice | Forward-compatible setup for future refresh phases | ✓ |
| Hide observer controls until Phase 38 | Delay observer UX entirely | |
| Default observer ON | Link by default for all imports | |

**User's choice:** Show optional link toggle default OFF and persist choice.
**Notes:** Aligns with prior user-approved default-OFF direction for large workflows.

---

## Import-start behavior

| Option | Description | Selected |
|--------|-------------|----------|
| Asynchronous chunked job + progress popup | Existing non-trivial import behavior | ✓ |
| Fully synchronous blocking import | No background/chunking path | |

**User's choice:** Asynchronous chunked job with progress popup.
**Notes:** Initial free-text response was ambiguous; resolved explicitly to async chunked behavior.

---

## the agent's Discretion

- Final copy for labels/tooltips.
- Popup layout ordering while retaining selected option coverage.

## Deferred Ideas

- Manual transactional refresh engine details (Phase 38).
- Automatic observer loop policy implementation details (Phase 39).
- Large-dump responsiveness/coherence closure gates (Phase 40).
