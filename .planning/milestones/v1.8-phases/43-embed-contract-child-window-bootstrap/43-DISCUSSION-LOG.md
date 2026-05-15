# Phase 43: Embed Contract & Child-Window Bootstrap - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in `43-CONTEXT.md` — this log preserves the alternatives considered.

**Date:** 2026-05-14
**Phase:** 43-embed-contract-child-window-bootstrap
**Areas discussed:** Failure policy, Sample launch behavior, Bootstrap visual baseline, Readiness feedback

---

## Failure policy

| Option | Description | Selected |
|--------|-------------|----------|
| Fail fast and exit with a clear error; no standalone fallback | Strict embedded-mode failure contract | ✓ |
| Fail fast and let the sample host show the main visible error state | Viewer exits, host owns primary error surface | |
| Fall back to a normal standalone mdCAD window | Compatibility fallback | |

**User's choice:** Fail fast and exit with a clear error; no standalone fallback  
**Notes:** Embedded-mode failures should stay explicit and must never silently open a standalone top-level mdCAD window.

---

## Sample launch behavior

| Option | Description | Selected |
|--------|-------------|----------|
| Auto-launch mdCAD as soon as the host control is ready | Minimal bootstrap harness with no extra host interaction | ✓ |
| Show a simple "Launch mdCAD" action first, then embed it | Manual bootstrap trigger in the sample UI | |
| Start the sample without launching mdCAD until triggered another way | Minimal host shell first, viewer later | |

**User's choice:** Auto-launch mdCAD as soon as the host control is ready  
**Notes:** The sample host should stay intentionally minimal and prove the embedding contract immediately.

---

## Bootstrap visual baseline

| Option | Description | Selected |
|--------|-------------|----------|
| Keep current mdCAD UI/chrome for Phase 43 and trim later in Phase 44 | Lowest bootstrap scope | |
| Trim obvious standalone chrome immediately in Phase 43 | Pull viewer-like presentation into the bootstrap phase | ✓ |
| Show only a bare render surface in Phase 43 | Most aggressive early trimming | |

**User's choice:** Trim obvious standalone chrome immediately in Phase 43  
**Notes:** The user additionally wants embedded-mode defaults to set these Visibility-window debug controls OFF: `Pick Buffer Debug`, `Slot Buffer Debug`, and `FPS Debug`.

---

## Readiness feedback

| Option | Description | Selected |
|--------|-------------|----------|
| Show staged bootstrap states: launching, waiting for child attach, attached, timeout/failure | Rich enough to debug bootstrap and keep the host understandable | ✓ |
| Show only a simple embedded/not-embedded success state | Minimal host feedback | |
| Keep status minimal in Phase 43 and add real readiness states later | Delay most feedback work | |

**User's choice:** Show staged bootstrap states: launching, waiting for child attach, attached, timeout/failure  
**Notes:** This staged feedback is for bootstrap/attach behavior only; richer JSONL/live-refresh workflow status can remain later-phase work.

---

## the agent's Discretion

- Whether to accept both hex and decimal parent-HWND values under the same visible CLI contract.
- Exact attach-timeout duration and polling cadence.
- Exact list of standalone chrome to trim immediately, as long as the requested debug-window defaults remain OFF in embedded mode.

## Deferred Ideas

- Startup JSONL launch flags and live refresh wiring belong to Phases 45-46.
- Full embedded resize/focus/input hardening belongs to Phase 44.
- Bundled example JSONL and full workflow proof belong to Phase 47.
