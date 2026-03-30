---
phase: 09-long-tail-validation-performance-gates-and-boundary-finalization
verified: 2026-03-30T11:53:07Z
status: passed
score: 4/4 must-haves verified
re_verification:
  previous_status: gaps_found
  previous_score: 2/4
  gaps_closed:
    - "macOS and Windows performance gates show no regressions for expanded migrated surfaces."
    - "Manual smoke checks for serializer/import/undo/editor workflows pass on required native targets."
  gaps_remaining: []
  regressions: []
---

# Phase 9: Long-Tail Validation, Performance Gates, and Boundary Finalization Verification Report

**Phase Goal:** Close v1.1 with expanded parity/performance confidence and finalized minimal thin-entrypoint boundary documentation.  
**Verified:** 2026-03-30T11:53:07Z  
**Status:** passed  
**Re-verification:** Yes — after operator-attested macOS evidence update

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
| --- | --- | --- | --- |
| 1 | Long-tail compare/harness checks pass with strict parity coverage. | ✓ VERIFIED | `evidence/coverage/compare-strict.txt` shows 15/15 required long-tail compare rows as `COMPARE PASS`. |
| 2 | macOS and Windows performance gates show no regressions for expanded migrated surfaces. | ✓ VERIFIED | Windows candidate `bench-eval.md` is `OVERALL: PASS` (with rerun policy applied for `bench-interaction-drag`); macOS candidate `bench-eval.md` now reports `OVERALL: PASS (ATTESTED)` and paired provenance is `capture_status=attested`. |
| 3 | Manual smoke checks for serializer/import/undo/editor workflows pass on required native targets. | ✓ VERIFIED | `evidence/manual/long-tail-smoke-report.md` records LT-VAL03-01..04 as PASS on Windows and PASS on macOS (operator attested); `Overall status` is `PASS`. |
| 4 | Remaining thin-entrypoint surface is minimal, documented, and intentionally retained. | ✓ VERIFIED | `evidence/boundary/thin-entrypoint-boundary-finalization.md` documents retained 3-symbol boundary with consumer-traceable rationale and deferred-removal notes. |

**Score:** 4/4 truths verified

### Required Artifacts

| Artifact | Expected | Status | Details |
| --- | --- | --- | --- |
| `src/math_harness.c` | Long-tail compare + bench coverage for VAL-01/VAL-02 | ✓ VERIFIED | Compare/bench IDs aligned with Phase 9 scope; strict compare evidence present. |
| `.planning/.../evidence/perf/windows-vulkan-msvc/candidate/bench-eval.md` | Explicit per-case + OVERALL performance decision | ✓ VERIFIED | Per-case table and `OVERALL: PASS` present; marginal case resolved by rerun policy. |
| `.planning/.../evidence/perf/macos-metal/candidate/bench-eval.md` | Native-target performance decision evidence | ✓ VERIFIED | Updated to attested native run with per-case PASS decisions and `OVERALL: PASS (ATTESTED)`. |
| `.planning/.../evidence/perf/macos-metal/candidate/provenance.txt` | Native capture provenance | ✓ VERIFIED | Contains source commit, command, host metadata, capture timestamp, and attestation source. |
| `.planning/.../evidence/manual/long-tail-smoke-report.md` | Dual-target VAL-03 outcomes | ✓ VERIFIED | LT-VAL03-01..04 all PASS for Windows/macOS; blockers section empty; overall PASS. |
| `.planning/.../evidence/boundary/thin-entrypoint-boundary-finalization.md` | Minimal retained boundary contract for TRED-02 | ✓ VERIFIED | Retained/removable/deferred boundary contract present with symbol mapping. |

### Key Link Verification

| From | To | Via | Status | Details |
| --- | --- | --- | --- | --- |
| Harness bench output contract | `bench-eval.md` (Windows/macOS) | evaluator decision tables | ✓ WIRED | Both target eval files contain per-case decisions and explicit OVERALL status. |
| Perf evidence | `VAL-02` | dual-target gate outcome | ✓ WIRED | Windows PASS + macOS PASS (attested) now jointly satisfy required target coverage. |
| Manual checklist execution | `long-tail-smoke-report.md` | LT-VAL03-01..04 status rows | ✓ WIRED | Required workflows recorded with PASS outcomes on both required targets. |
| Boundary inventory | retained thin-entrypoint symbols | documented consumer mapping | ✓ WIRED | `ray_from_screen`, `ray_axis_closest_t`, `ray_plane_intersect` traceability retained. |

### Data-Flow Trace (Level 4)

| Artifact | Data Variable | Source | Produces Real Data | Status |
| --- | --- | --- | --- | --- |
| `compare-strict.txt` | compare pass rows | harness compare execution output | Yes | ✓ FLOWING |
| Windows perf eval | slowdown/decision rows | baseline + candidate + rerun bench captures | Yes | ✓ FLOWING |
| macOS perf eval | decision rows | operator-native run attestation + provenance metadata | Attested evidence (raw stdout not retained) | ✓ FLOWING (ATTESTED) |
| Manual smoke report | LT-VAL03 workflow statuses | operator/manual execution records | Yes (Windows direct + macOS attested) | ✓ FLOWING (ATTESTED) |

### Behavioral Spot-Checks

| Behavior | Command | Result | Status |
| --- | --- | --- | --- |
| Windows perf gate closure | `rg -n "bench-interaction-drag|OVERALL" windows.../bench-eval.md` | marginal case PASS with rerun; OVERALL PASS | ✓ PASS |
| macOS perf evidence closure | `rg -n "Evaluation status|OVERALL|attested" macos.../bench-eval.md provenance.txt` | evaluation marked ATTESTED and OVERALL PASS; provenance `capture_status=attested` | ✓ PASS |
| VAL-03 dual-target completion | `rg -n "LT-VAL03-0[1-4]|Overall status|PASS" evidence/manual/long-tail-smoke-report.md` | all required rows PASS on Windows/macOS; overall PASS | ✓ PASS |

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
| --- | --- | --- | --- | --- |
| `VAL-01` | 09-01 | Long-tail compare coverage is explicit and strict-pass for required parity cases. | ✓ SATISFIED | `compare-strict.txt` shows strict PASS rows for required long-tail checks. |
| `VAL-02` | 09-02, 09-04 | No native perf regression on required macOS + Windows targets. | ✓ SATISFIED | Windows `OVERALL: PASS`; macOS `OVERALL: PASS (ATTESTED)` + provenance metadata. |
| `VAL-03` | 09-03, 09-05 | Manual serializer/import/undo/editor workflows pass on required native targets. | ✓ SATISFIED | LT-VAL03-01..04 rows PASS for both required targets; overall PASS. |
| `TRED-02` | 09-03 | Remaining thin-entrypoint surface is minimal, documented, and intentional. | ✓ SATISFIED | Boundary finalization document with retained 3-symbol inventory and rationale. |

Orphaned requirements check (Phase 9): none. Required IDs (`TRED-02`, `VAL-01`, `VAL-02`, `VAL-03`) are covered by Phase 9 plans.

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
| --- | --- | --- | --- | --- |
| `.../evidence/perf/macos-metal/candidate/bench-run1.txt` | 1-3 | attestation stub in place of raw BENCH stdout | ℹ️ Info | Evidence is attested rather than raw-log-backed; acceptable for current closure but lower forensic granularity. |
| `.../evidence/perf/macos-metal/candidate/bench-eval.md` | multiple | `attested-no-raw-capture` notes | ℹ️ Info | Transparent provenance annotation; not a hidden placeholder. |

### Gaps Summary

Previously blocking gaps are closed in this re-verification. Required dual-target performance and manual validation evidence now records PASS outcomes (with explicit macOS attestation provenance), and no new regressions were found against previously verified artifacts.

---

_Verified: 2026-03-30T11:53:07Z_  
_Verifier: the agent (gsd-verifier)_
