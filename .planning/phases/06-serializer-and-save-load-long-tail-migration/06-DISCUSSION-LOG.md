# Phase 6: Serializer and Save/Load Long-Tail Migration - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-03-26
**Phase:** 06-serializer-and-save-load-long-tail-migration
**Areas discussed:** Serializer scope cutline, Scene format compatibility policy, Validation/evidence depth, Legacy-helper retirement aggressiveness

---

## Serializer scope cutline

| Option | Description | Selected |
|--------|-------------|----------|
| `src/scene_serializer.h` only | Keep migration inside serializer implementation only | |
| `scene_serializer.h` + serializer UI callsites | Include serializer-related callsites in `ui_scene_hierarchy.h` | ✓ |
| Broader serializer+importer pass | Include importer shared helpers in the same phase | |

**User's choice:** Include `scene_serializer.h` plus serializer-related UI callsites.
**Notes:** UI changes should allow minor polish only when directly improving save/load clarity.

---

## Scene format compatibility policy

| Option | Description | Selected |
|--------|-------------|----------|
| Strict backward compatibility | Preserve old scene compatibility while migrating internals | |
| Best-effort compatibility with optional bump | Allow cleanup with limited compatibility accommodations | |
| Format cleanup now | Allow cleanup now with explicit migration notes | ✓ |

**User's choice:** Open to format cleanup now with explicit migration notes.
**Notes:** Follow-up decision: break old format and provide a converter workflow.

---

## Validation/evidence depth for phase gate

| Option | Description | Selected |
|--------|-------------|----------|
| Strong gate | Harness expansion + strict compare + macOS/Windows manual smoke | |
| Medium gate | Strict compare + one native smoke target | |
| Light gate | Compile + targeted save/load checks only | ✓ |

**User's choice:** Light gate.
**Notes:** Required targeted checks: representative save/reload verifying entity count, parent links, transform fields, and geometry types.

---

## Legacy-helper retirement aggressiveness

| Option | Description | Selected |
|--------|-------------|----------|
| Conservative retirement | Retire touched helpers only with low churn | |
| Internal migration only | Keep helpers, migrate internals | |
| Aggressive purge | Purge serializer legacy math helper usage in this phase | ✓ |

**User's choice:** Aggressively purge serializer legacy math helpers now.
**Notes:** Aggression applies within serializer scope; other subsystems remain phase-scoped for later work.

---

## the agent's Discretion

- Exact converter workflow implementation and UX.
- Exact targeted-check implementation mechanism.

## Deferred Ideas

- Importer migration specifics (Phase 7)
- Undo/editor utility migration specifics (Phase 8)
- Expanded long-tail strict gate/perf closure (Phase 9)
