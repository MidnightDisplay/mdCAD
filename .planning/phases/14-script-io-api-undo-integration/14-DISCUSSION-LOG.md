# Phase 14: Script IO + API/Undo Integration - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in `14-CONTEXT.md` — this log preserves the alternatives considered.

**Date:** 2026-04-01
**Phase:** 14-script-io-api-undo-integration
**Areas discussed:** script apply safety, script IO schema, scene API surface, undo/redo transaction semantics, IO frontend behavior

---

## Script-side apply safety

| Option | Description | Selected |
|--------|-------------|----------|
| Strict transactional apply with full rollback on any parse/validation/apply error | Never commit partial state; restore full pre-apply state on any failure. | ✓ |
| Two-step model: preview builds a candidate snapshot, Apply commits snapshot atomically | Separate candidate generation from explicit commit path. | |
| Allow partial apply with per-item errors | Commit valid subset and report per-item failures. | |

**User's choice:** Strict transactional apply with full rollback on any parse/validation/apply error.
**Notes:** User accepted strict safety contract as the default for Phase 14.

---

## Script IO declaration contract

| Option | Description | Selected |
|--------|-------------|----------|
| Single typed numeric parameter table with optional `min/max/step`, split by `inputs` and `outputs` | Numeric-only schema matching roadmap requirement focus. | |
| Support multiple scalar types now (`int/float/bool/string`) with per-type widgets | Broader widget/type expansion in same phase. | |
| Keep IO schema minimal (numeric values only) and defer slider metadata | Simplest schema first; less UI richness. | |

**User's initial preference:** Multi-type support now (`int/float/bool/string`).
**Scope check outcome:** Treated as capability expansion beyond current roadmap requirement framing.
**Final decision after scope guardrail prompt:** Keep Phase 14 numeric-only and defer non-numeric types.

---

## Scene API surface (`API-01`)

| Option | Description | Selected |
|--------|-------------|----------|
| Single scene-level façade per workflow family | UI remains thin caller; logic centralized in scene layer. | ✓ |
| Expose lower-level ECS-style operations directly to UI and scripts | More direct control from callers. | |
| Hybrid façade + low-level split | Mix high-level and low-level call patterns. | |

**User's choice:** Single scene-level façade per workflow family.
**Notes:** Keeps continuity with prior phase architecture decisions.

---

## Undo/redo transaction behavior (`API-02`)

| Option | Description | Selected |
|--------|-------------|----------|
| One Apply Script action = one atomic undo entry; rollback restores full pre-apply state | Transaction-style script mutation behavior. | ✓ |
| Per-entity/per-constraint undo granularity within one apply | Finer-grained history for script applies. | |
| No undo integration for script apply in Phase 14 | Defer undo transaction support. | |

**User's choice:** One Apply Script action equals one atomic undo entry with full rollback behavior.

---

## IO frontend placement and behavior

| Option | Description | Selected |
|--------|-------------|----------|
| Dedicated IO panel in Script Editor | Keep controls inside existing editor window. | |
| Keep controls in Entity Inspector instead of Script Editor | Reuse inspector panel surface. | |
| Dedicated IO ImGui window toggled from SketchManager | Separate window for IO controls. | ✓ |

**User's choice:** Dedicated IO ImGui window with toggle button next to `Open Script Editor`.
**Notes:** Inputs editable, outputs read-only, immediate bidirectional sync with script state, sliders for `min/max/step`.

---

## IO edit commit mode

| Option | Description | Selected |
|--------|-------------|----------|
| Stage changes; only `Apply Script` commits | Safer explicit commit model. | |
| Auto-apply live on every input change | Immediate update feel for IO controls. | ✓ |

**User's choice:** Auto-apply live on every input change.

---

## IO live-edit safety rule

| Option | Description | Selected |
|--------|-------------|----------|
| Yes — each edit is an atomic mini-apply | Validate/apply/rollback for each live edit. | ✓ |
| No — allow temporary partial/invalid state during editing | Permit transient invalid state during input. | |

**User's choice:** Yes, each IO edit must be an atomic mini-apply through the same safe pipeline.

---

## the agent's Discretion

- Exact internal data layout for numeric IO declarations, as long as `inputs/outputs` + optional `min/max/step` semantics are preserved.
- Exact dedicated IO window layout details within existing ImGui patterns.
- Exact undo payload internals for script transactions, provided user-visible atomic behavior holds.

## Deferred Ideas

- Support for non-numeric/multi-type IO widgets (`bool`, `string`, expanded type system) deferred to a future phase.

