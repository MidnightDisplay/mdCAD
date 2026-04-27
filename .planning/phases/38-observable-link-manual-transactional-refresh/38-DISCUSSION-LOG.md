# Phase 38: Observable Link + Manual Transactional Refresh - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-04-27
**Phase:** 38-observable-link-manual-transactional-refresh
**Areas discussed:** observer controls location, manual refresh UX, transaction semantics, link/edit lifecycle

---

## Observer controls location on flat imports

| Option | Description | Selected |
|--------|-------------|----------|
| Entity Inspector when flat root anchor is selected | Reuse inspector-centric entity workflow and keep controls scoped to selected anchor | ✓ |
| Scene Hierarchy panel controls | Put controls in hierarchy UI instead of inspector | |
| Both Inspector and Hierarchy | Duplicate controls across both surfaces | |

**User's choice:** Entity Inspector when flat root anchor is selected.  
**Notes:** Keep Phase 38 scope minimal and avoid extra duplicated UI surfaces.

### Scope of Inspector controls

| Option | Description | Selected |
|--------|-------------|----------|
| Minimal controls | Link toggle, source path, Re-import now, last refresh message | ✓ |
| Extended controls | Include interval/retry controls now | |
| Full sketch observer parity | Mirror full sketch observer panel now | |

**User's choice:** Minimal controls.

---

## Manual refresh trigger + status UX

| Option | Description | Selected |
|--------|-------------|----------|
| Async with existing progress popup | Reuse modal progress popup and cancel affordance | |
| Synchronous blocking refresh | Block while refresh runs | |
| Background refresh without popup | Run in background with local status feedback only | ✓ |

**User's choice:** Background refresh with no popup.

### Status surface

| Option | Description | Selected |
|--------|-------------|----------|
| Inspector only | Show current state and last result in Inspector | ✓ |
| Inspector + global status | Mirror to app-level status/toast too | |
| Global status only | No local inspector status | |

**User's choice:** Inspector only.

### Concurrent trigger behavior

| Option | Description | Selected |
|--------|-------------|----------|
| Single in-flight | Ignore repeated refresh clicks while one run is active | ✓ |
| Queue one additional run | Schedule one follow-up run | |
| Cancel-and-restart | Interrupt current run and restart immediately | |

**User's choice:** Single in-flight only.

---

## Refresh transaction semantics on failure/success

| Option | Description | Selected |
|--------|-------------|----------|
| Preserve root identity + replace subtree | Keep same root entity, swap imported children on success | ✓ |
| Create new root per refresh | New root entity each refresh run | |

**User's choice:** Preserve root identity; replace subtree only.

### Failure behavior

| Option | Description | Selected |
|--------|-------------|----------|
| Preserve last-good subtree | Keep existing imported children untouched on failure | ✓ |
| Clear subtree on failure | Remove old children even if new import fails | |
| Keep partial results | Allow partially refreshed subtree | |

**User's choice:** Preserve last-good subtree.

### Selection fallback on subtree replacement

| Option | Description | Selected |
|--------|-------------|----------|
| Select root anchor | If selected child is replaced, move selection to root | ✓ |
| Clear selection | Drop selection on replace | |
| Attempt child remap | Try to match and preserve child selection | |

**User's choice:** Select root anchor.

---

## Post-import editing of observer settings

| Option | Description | Selected |
|--------|-------------|----------|
| Allow linking later | Anchors imported with link OFF can be linked in Inspector | ✓ |
| Link fixed at import time | No post-import linking changes | |

**User's choice:** Allow linking later.

### Replay settings editability in Phase 38

| Option | Description | Selected |
|--------|-------------|----------|
| Locked replay settings | Keep original import replay settings unchanged in Phase 38 | ✓ |
| Editable all settings | Expose scale/rotation/shift/colour/mesh editing now | |
| Editable transforms only | Expose only scale/rotation/shift editing now | |

**User's choice:** Keep replay settings locked for Phase 38.

### Relink label contract

| Option | Description | Selected |
|--------|-------------|----------|
| Update root label metadata | On relink, update root name/description to new stem/path | ✓ |
| Keep original root label metadata | Preserve old label values after relink | |

**User's choice:** Update root label metadata on relink.

---

## the agent's Discretion

- Exact flat-anchor observer metadata structure and internal orchestration details, within locked Phase 38 decisions.

## Deferred Ideas

- Automatic observer safety loop (debounce/retry/auto-disable) remains Phase 39.
- Replay-setting editing remains outside Phase 38.
