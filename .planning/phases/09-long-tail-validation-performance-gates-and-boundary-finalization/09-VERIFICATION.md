---
phase: 09-long-tail-validation-performance-gates-and-boundary-finalization
verified: 2026-03-30T11:19:21Z
status: gaps_found
score: 2/4 must-haves verified
re_verification:
  previous_status: gaps_found
  previous_score: 2/4
  gaps_closed: []
  gaps_remaining:
    - "macOS and Windows performance gates show no regressions for expanded migrated surfaces."
    - "Manual smoke checks for serializer/import/undo/editor workflows pass on required native targets."
  regressions: []
gaps:
  - truth: "macOS and Windows performance gates show no regressions for expanded migrated surfaces."
    status: partial
    reason: "Windows rerun-aware VAL-02 is now OVERALL PASS (bench-interaction-drag resolved), but required macOS Metal candidate evidence is still blocked and macOS bench-eval remains OVERALL FAIL."
    artifacts:
      - path: ".planning/phases/09-long-tail-validation-performance-gates-and-boundary-finalization/evidence/perf/windows-vulkan-msvc/candidate/bench-eval.md"
        issue: "Resolved: OVERALL PASS with rerun-applied bench-interaction-drag (2.675%)."
      - path: ".planning/phases/09-long-tail-validation-performance-gates-and-boundary-finalization/evidence/perf/macos-metal/candidate/bench-eval.md"
        issue: "Still blocked placeholder; per-case rows marked blocked-no-macos-candidate and OVERALL FAIL."
      - path: ".planning/phases/09-long-tail-validation-performance-gates-and-boundary-finalization/evidence/perf/macos-metal/candidate/provenance.txt"
        issue: "capture_status=blocked; no native macOS host capture provenance."
    missing:
      - "Capture true native macOS Metal candidate benchmark run and provenance."
      - "Re-evaluate macOS candidate with scripts/eval_math_bench.py to produce non-blocked dual-target evidence."
  - truth: "Manual smoke checks for serializer/import/undo/editor workflows pass on required native targets."
    status: partial
    reason: "Windows LT-VAL03-02 is now PASS in both single-node and editable modes (with provenance note), but required macOS manual rows remain BLOCKED."
    artifacts:
      - path: ".planning/phases/09-long-tail-validation-performance-gates-and-boundary-finalization/evidence/manual/long-tail-smoke-report.md"
        issue: "Windows rows PASS, but macOS LT-VAL03-01..04 remain BLOCKED; overall status remains BLOCKED (BLOCKING)."
    missing:
      - "Execute LT-VAL03-01..04 on native macOS Metal and replace BLOCKED rows with concrete PASS/FAIL outcomes."
      - "Update overall VAL-03 status after required macOS execution."
---

# Phase 9: Long-Tail Validation, Performance Gates, and Boundary Finalization Verification Report

**Phase Goal:** Close v1.1 with expanded parity/performance confidence and finalized minimal thin-entrypoint boundary documentation.  
**Verified:** 2026-03-30T11:19:21Z  
**Status:** gaps_found  
**Re-verification:** Yes — after gap-closure attempts (09-04, 09-05)

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
| --- | --- | --- | --- |
| 1 | Long-tail compare/harness checks pass with strict parity coverage. | ✓ VERIFIED | `evidence/coverage/compare-strict.txt` contains 15 required long-tail compare PASS rows; `src/math_harness.c` includes mapped compare IDs (serializer/import/undo/editor) plus LT-VAL03-02 regression compare case. |
| 2 | macOS and Windows performance gates show no regressions for expanded migrated surfaces. | ✗ FAILED (partial progress) | Windows `bench-eval.md` now `OVERALL: PASS`; `bench-interaction-drag` resolved via rerun policy (2.675%, rerun-applied). macOS `bench-eval.md` still `OVERALL: FAIL` with blocked-no-macos-candidate rows. |
| 3 | Manual smoke checks for serializer/import/undo/editor workflows pass on required native targets. | ✗ FAILED (partial progress) | `long-tail-smoke-report.md` now shows Windows LT-VAL03-02 PASS for both single-node + editable modes (plus control sample PASS), but macOS rows remain BLOCKED and overall is `BLOCKED (BLOCKING)`. |
| 4 | Remaining thin-entrypoint surface is minimal, documented, and intentionally retained. | ✓ VERIFIED | `evidence/boundary/thin-entrypoint-boundary-finalization.md` documents retained minimal 3-symbol boundary with consumer traceability and deferred-removal rationale. |

**Score:** 2/4 truths verified

### Required Artifacts

| Artifact | Expected | Status | Details |
| --- | --- | --- | --- |
| `src/math_harness.c` | Long-tail compare + bench coverage for VAL-01/VAL-02 | ✓ VERIFIED | Compare catalog includes serializer/import/undo/editor parity IDs and bench IDs including `bench-interaction-drag`. |
| `src/ply_import_job.h` | Runtime import job completes vertex-only modes correctly | ✓ VERIFIED | `ply_import_job_parse_step_complete()` and `ply_import_job_tick()` use vertex-complete gating for point-cloud/editable import paths; completion path is substantive and wired. |
| `evidence/perf/windows-vulkan-msvc/candidate/bench-eval.md` | Explicit per-case + OVERALL performance decision | ✓ VERIFIED | Includes rerun-aware table and `OVERALL: PASS`; `bench-interaction-drag` now PASS with rerun-applied effective candidate. |
| `evidence/perf/macos-metal/candidate/bench-eval.md` | Native macOS per-case + OVERALL decision from real candidate run | ✗ FAILED | Exists but still blocked placeholder output with `OVERALL: FAIL`. |
| `evidence/perf/macos-metal/candidate/provenance.txt` | Native capture provenance | ✗ FAILED | `capture_status=blocked`; no macOS host evidence present. |
| `evidence/manual/long-tail-smoke-report.md` | Dual-target VAL-03 workflow outcomes | ✗ FAILED | Windows rows PASS (including LT-VAL03-02 retest provenance), but all required macOS rows are BLOCKED; overall remains BLOCKED. |
| `evidence/boundary/thin-entrypoint-boundary-finalization.md` | Minimal retained boundary contract for TRED-02 | ✓ VERIFIED | Retained/removable/deferred sections present with concrete symbol/consumer mapping. |

### Key Link Verification

| From | To | Via | Status | Details |
| --- | --- | --- | --- | --- |
| `mdcad_math_harness --mode bench` outputs | `scripts/eval_math_bench.py` | BENCH rows parsed into eval table | ✓ WIRED | Windows candidate + rerun bench artifacts are reflected in rerun-aware `bench-eval.md`. |
| Windows perf eval | `VAL-02` | `OVERALL` + per-case decisions | ✓ WIRED | Windows gate now auditable PASS. |
| macOS perf eval artifacts | `VAL-02` | candidate bench/provenance -> eval | ⚠️ PARTIAL | Link structure exists but source capture is blocked, leaving unresolved required target. |
| Manual checklist execution | `long-tail-smoke-report.md` | LT-VAL03-01..04 status rows | ✓ WIRED | Windows statuses updated with retest provenance; macOS rows explicitly BLOCKED (truthful but unresolved). |
| Boundary doc | runtime/harness consumer inventory | symbol traceability | ✓ WIRED | `ray_from_screen`, `ray_axis_closest_t`, `ray_plane_intersect` mapped with rationale and consumers. |

### Data-Flow Trace (Level 4)

| Artifact | Data Variable | Source | Produces Real Data | Status |
| --- | --- | --- | --- | --- |
| `src/math_harness.c` | compare/bench case outputs | executed compare/bench functions | Yes | ✓ FLOWING |
| Windows perf eval | slowdown/decision rows | baseline + candidate + rerun bench files | Yes | ✓ FLOWING |
| macOS perf eval | slowdown/decision rows | candidate capture/provenance | No (blocked placeholder) | ✗ DISCONNECTED |
| Manual smoke report | LT-VAL03 status rows | operator/manual execution records | Partially (Windows real, macOS missing) | ⚠️ STATIC/BLOCKED |

### Behavioral Spot-Checks

| Behavior | Command | Result | Status |
| --- | --- | --- | --- |
| Windows rerun-policy closure for marginal case | grep `bench-interaction-drag|OVERALL` in Windows `bench-eval.md` | `bench-interaction-drag ... 2.675% ... PASS ... rerun-applied`; `OVERALL: PASS` | ✓ PASS |
| macOS candidate perf evidence closure | grep `Evaluation status|OVERALL` in macOS `bench-eval.md` | `Evaluation status: BLOCKED`; `OVERALL: FAIL` | ✗ FAIL |
| LT-VAL03-02 Windows retest status | grep LT-VAL03-02 row in smoke report | PASS in both single-node + editable modes with provenance note | ✓ PASS |
| Required macOS manual workflow completion | grep macOS rows in smoke report | LT-VAL03-01..04 macOS all BLOCKED | ✗ FAIL |

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
| --- | --- | --- | --- | --- |
| `TRED-02` | 09-03 | Remaining thin-entrypoint surface is minimal, documented, and intentional. | ✓ SATISFIED | Boundary finalization doc defines retained 3-symbol minimal surface with traceability. |
| `VAL-01` | 09-01 | Long-tail compare coverage is explicit and strict-pass for required parity cases. | ✓ SATISFIED | Coverage map + strict compare artifact show required long-tail case pass evidence. |
| `VAL-02` | 09-02, 09-04 | No native perf regression on required macOS + Windows targets. | ✗ BLOCKED | Windows target now PASS; required macOS target remains blocked (no native candidate evidence). |
| `VAL-03` | 09-03, 09-05 | Manual serializer/import/undo/editor workflows pass on required macOS + Windows targets. | ✗ BLOCKED | Windows rows now PASS (including LT-VAL03-02), but required macOS rows remain BLOCKED. |

Orphaned requirements check (Phase 9): none. Required IDs (`TRED-02`, `VAL-01`, `VAL-02`, `VAL-03`) are covered by Phase 9 plans.

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
| --- | --- | --- | --- | --- |
| `evidence/manual/long-tail-smoke-report.md` | 30 | “host: not available” | ℹ️ Info | Truthful blocker reporting; not a code stub. |
| `src/ply_import_job.h` | multiple | `= NULL` initialization hits | ℹ️ Info | Normal pointer lifecycle/init logic; not user-visible hollow behavior. |

### Human Verification Required

### 1. macOS native VAL-02 perf capture
**Test:** Run `./build/bin/mdcad_math_harness --mode bench --iterations 2000000` on native macOS Metal and regenerate macOS `bench-eval.md`.  
**Expected:** Required macOS perf gate has real candidate evidence and explicit `OVERALL: PASS|FAIL` from true run data (not blocked placeholder).  
**Why human:** Requires native macOS host unavailable in this environment.

### 2. macOS native VAL-03 manual workflows
**Test:** Execute LT-VAL03-01..04 on macOS Metal and update report rows.  
**Expected:** macOS rows become explicit PASS/FAIL outcomes; phase can only close if required rows pass.  
**Why human:** Native UI/manual execution on macOS cannot be performed from this Windows-only session.

### Gaps Summary

Re-verification confirms meaningful progress: Windows VAL-02 is now policy-correct PASS, and Windows LT-VAL03-02 is now PASS in both import modes with provenance recorded. However, Phase 9 goal achievement is still blocked because required macOS native evidence is missing for both perf (`VAL-02`) and manual smoke (`VAL-03`). Until macOS required rows are executed and captured, closure claims would be untruthful.

---

_Verified: 2026-03-30T11:19:21Z_  
_Verifier: the agent (gsd-verifier)_
