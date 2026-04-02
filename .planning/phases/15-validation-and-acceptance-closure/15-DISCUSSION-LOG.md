# Phase 15: Validation & Acceptance Closure - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in `15-CONTEXT.md` — this log preserves the alternatives considered.

**Date:** 2026-04-02
**Phase:** 15-validation-and-acceptance-closure
**Areas discussed:** validation artifact contract, Windows acceptance strictness, macOS parity scope, case-study depth/content

---

## Validation artifact format

| Option | Description | Selected |
|--------|-------------|----------|
| `15-VALIDATION.md` + `15-03-SUMMARY.md` + update `CHECKPOINT.md` | Full closure bundle with validation, summary, and continuity update. | ✓ |
| `15-VALIDATION.md` only | Minimal evidence file only. | |
| `15-VALIDATION.md` + dedicated acceptance log file | Separate acceptance artifact in addition to validation. | |
| Custom bundle | User-defined alternative set. | |

**User's choice:** `15-VALIDATION.md` + `15-03-SUMMARY.md` + update `CHECKPOINT.md`.

---

## Windows MSVC + Vulkan acceptance strictness

| Option | Description | Selected |
|--------|-------------|----------|
| Build + full CTest required | Require successful `mdCAD` Release build and full `ctest -C Release --output-on-failure` in `build-vulkan`. | ✓ |
| Build only | Accept compile success without full test run. | |
| CTest only | Accept test run without explicit `mdCAD` Release build pass. | |
| Custom criteria | User-defined gate semantics. | |

**User's choice:** Require both build and full CTest pass.

---

## macOS parity requirement in Phase 15

| Option | Description | Selected |
|--------|-------------|----------|
| Document command set; execution optional | Record expected parity run path if host unavailable. | |
| Mandatory fresh macOS execution evidence | Require logs/screenshots before phase closure. | |
| Skip macOS parity in Phase 15 | Defer `VAL-03` evidence beyond this phase. | ✓ |
| Custom parity rule | User-defined parity expectation. | |

**User's choice:** Skip macOS parity in Phase 15.

---

## Case-study scope for `VAL-01`

| Option | Description | Selected |
|--------|-------------|----------|
| One sketch + one Script IO scenario | Minimal representative sample set. | |
| Two sketches + one Script IO scenario | Broader sample depth with dedicated IO behavior coverage. | ✓ |
| Script IO scenario only | No dedicated sketch case-study requirement. | |
| Custom scope | User-defined case-study set. | |

**User's choice:** Two representative sketches + one Script IO scenario.

---

## the agent's Discretion

- Exact case-study names and where sample artifacts are stored.
- Exact section structure/order inside `15-VALIDATION.md` and `15-03-SUMMARY.md`.
- Exact command sequencing for evidence capture, provided required gates are preserved.

## Deferred Ideas

- Execute/record macOS parity (`VAL-03`) in a follow-up phase or closure addendum.

