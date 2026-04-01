# Phase 13: Script Round-Trip Baseline - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md - this log preserves the alternatives considered.

**Date:** 2026-04-01
**Phase:** 13-script-round-trip-baseline
**Areas discussed:** Script grammar and scope, Editor interaction model, Round-trip identity mapping, Deterministic output policy

---

## Script grammar and scope

| Option | Description | Selected |
|--------|-------------|----------|
| Declarative Lua table model | Canonical declarative graph model for entities/constraints | ✓ |
| Imperative Lua API calls | Script is ordered API calls (create/add style) | |
| Hybrid | Declarative base with imperative helper layer | |

**User's choice:** Declarative Lua table model
**Notes:** Locked as canonical phase contract.

| Option | Description | Selected |
|--------|-------------|----------|
| Exact v1.2 sketch scope | Parse/reconstruct points, lines, arcs/circles + current constraints only | ✓ |
| Dimensional subset first | Start with limited dimensional-only scope | |
| Ignore unsupported nodes | Accept unknown/future nodes and skip unsupported entries | |

**User's choice:** Exact current v1.2 sketch scope only
**Notes:** No scope expansion in Phase 13.

---

## Editor interaction model

| Option | Description | Selected |
|--------|-------------|----------|
| Explicit Apply only | No preview while typing | |
| Auto-apply only | Debounced apply while typing | |
| Auto-preview + explicit Apply | Preview loop with explicit commit action | ✓ |

**User's choice:** Both auto-apply preview and explicit Apply commit
**Notes:** Requires clear preview vs commit behavior separation.

| Option | Description | Selected |
|--------|-------------|----------|
| Keep last valid state on preview failure | Show diagnostics and do not mutate committed scene | ✓ |
| Partial preview apply | Apply only parseable sections | |
| Disable preview on error | Stop preview until manual reset/apply | |

**User's choice:** Show diagnostics and keep scene at last committed valid state
**Notes:** Prevents corruption during iteration.

| Option | Description | Selected |
|--------|-------------|----------|
| Atomic all-or-nothing apply | Commit only with fully valid script and resolved references | ✓ |
| Partial apply | Commit valid subset only | |
| Confirm unresolved refs | Prompt user on unresolved references | |

**User's choice:** Atomic all-or-nothing commit
**Notes:** Commit gate must enforce full validity.

---

## Round-trip identity mapping

| Option | Description | Selected |
|--------|-------------|----------|
| Script-local stable IDs | Persist stable script IDs in component/serialization layer | ✓ |
| Labels only | Resolve links by human-readable names | |
| Raw ECS IDs | Expose ECS entity numbers directly | |

**User's choice:** Script-local stable IDs stored in components and serialized
**Notes:** Avoids label collisions and ECS-ID instability.

| Option | Description | Selected |
|--------|-------------|----------|
| Two-pass reconstruction | Create entities first, resolve links in second pass | ✓ |
| Single-pass reconstruction | Resolve references during creation scan | |
| Best-effort reconstruction | Drop unresolved references silently | |

**User's choice:** Two-pass atomic link resolution
**Notes:** Required for deterministic link integrity.

---

## Deterministic output policy

| Option | Description | Selected |
|--------|-------------|----------|
| Stable ID sort within type groups | Deterministic canonical ordering by script IDs | ✓ |
| Creation order | Emit in mutable historical order | |
| Label alphabetical | Sort by display labels | |

**User's choice:** Stable sort by script-local IDs within type groups
**Notes:** Supports deterministic diffs and reproducibility.

| Option | Description | Selected |
|--------|-------------|----------|
| Fixed decimal + trim, no scientific | Canonical readable decimal format | ✓ |
| Full precision floats | Max precision output | |
| Scientific allowed | Compact notation when shorter | |

**User's choice:** Fixed decimal with trimming and no scientific notation
**Notes:** Must stay human-readable and stable.

---

## the agent's Discretion

- Implementation-level Lua embedding and memory management details.
- Exact script module/file boundaries and helper function naming.
- Exact editor widget layout and diagnostics rendering composition.

## Deferred Ideas

- Script-side safe apply and rollback lifecycle controls beyond baseline reconstruction (Phase 14).
- Dynamic script IO controls and generated parameter widgets (Phase 14).
- Platform validation packaging/reporting for scripting acceptance (Phase 15).
