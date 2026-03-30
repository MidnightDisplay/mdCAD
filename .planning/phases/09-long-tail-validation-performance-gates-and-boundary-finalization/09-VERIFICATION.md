---
phase: 09-long-tail-validation-performance-gates-and-boundary-finalization
verified: 2026-03-30T10:04:49Z
status: gaps_found
score: 2/4 must-haves verified
gaps:
  - truth: "macOS and Windows performance gates show no regressions for expanded migrated surfaces."
    status: failed
    reason: "VAL-02 evidence shows blocking failures: Windows bench-interaction-drag regressed 6.034% (>5%), and macOS candidate benchmark capture is blocked/unavailable."
    artifacts:
      - path: ".planning/phases/09-long-tail-validation-performance-gates-and-boundary-finalization/evidence/perf/windows-vulkan-msvc/candidate/bench-eval.md"
        issue: "OVERALL: FAIL due to bench-interaction-drag slowdown 6.034%."
      - path: ".planning/phases/09-long-tail-validation-performance-gates-and-boundary-finalization/evidence/perf/macos-metal/candidate/bench-eval.md"
        issue: "OVERALL: FAIL with blocked-no-macos-candidate across required cases."
    missing:
      - "Resolve bench-interaction-drag Windows regression to <=5% slowdown (or pass rerun policy if marginal band applies)."
      - "Capture true native macOS Metal candidate bench artifact and re-evaluate with scripts/eval_math_bench.py."
  - truth: "Manual smoke checks for serializer/import/undo/editor workflows pass on required native targets."
    status: failed
    reason: "VAL-03 report records required workflow failures/blockers: LT-VAL03-02 import FAIL on Windows and all macOS rows BLOCKED."
    artifacts:
      - path: ".planning/phases/09-long-tail-validation-performance-gates-and-boundary-finalization/evidence/manual/long-tail-smoke-report.md"
        issue: "Overall status is FAIL (BLOCKING); LT-VAL03-02 is FAIL; macOS target remains BLOCKED."
    missing:
      - "Fix LT-VAL03-02 PLY import regression for sample 'pc1_Wednesday, 17 December 2025 at 15_08_15 Greenwich Mean Time.ply'."
      - "Execute and record full macOS Metal manual workflow outcomes for LT-VAL03-01..04."
---

# Phase 9: Long-Tail Validation, Performance Gates, and Boundary Finalization Verification Report

**Phase Goal:** Close v1.1 with expanded parity/performance confidence and finalized minimal thin-entrypoint boundary documentation.  
**Verified:** 2026-03-30T10:04:49Z  
**Status:** gaps_found  
**Re-verification:** No — initial verification

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
| --- | --- | --- | --- |
| 1 | Long-tail compare/harness checks pass with strict parity coverage. | ✓ VERIFIED | `src/math_harness.c:161-177` includes dedicated long-tail compare IDs; `evidence/coverage/compare-strict.txt` shows 15 `COMPARE PASS` rows including serializer/import/undo/editor cases. |
| 2 | macOS and Windows performance gates show no regressions for expanded migrated surfaces. | ✗ FAILED | Windows eval: `bench-interaction-drag` 6.034% slowdown and `OVERALL: FAIL` (`evidence/perf/windows-vulkan-msvc/candidate/bench-eval.md`). macOS eval is blocked with `OVERALL: FAIL` (`evidence/perf/macos-metal/candidate/bench-eval.md`). |
| 3 | Manual smoke checks for serializer/import/undo/editor workflows pass on required native targets. | ✗ FAILED | `evidence/manual/long-tail-smoke-report.md` marks LT-VAL03-02 import as `FAIL` on Windows and all macOS required rows as `BLOCKED`; overall `FAIL (BLOCKING)`. |
| 4 | Remaining thin-entrypoint surface is minimal, documented, and intentionally retained. | ✓ VERIFIED | `evidence/boundary/thin-entrypoint-boundary-finalization.md` documents retained 3-symbol surface; code scan shows legacy helper runtime usage concentrated in `src/math_harness.c` while runtime uses `mdcad_interaction_*` in `src/app.c` and `src/gizmo/gizmo.h`. |

**Score:** 2/4 truths verified

### Required Artifacts

| Artifact | Expected | Status | Details |
| --- | --- | --- | --- |
| `src/math_harness.c` | Compare/bench catalog with long-tail parity cases | ✓ VERIFIED | Contains `mdcad_compare_cases` with `serializer-transform-parity`, `import-hierarchy-parity`, `undo-axis-drag-parity`, `editor-plane-drag-parity`. |
| `evidence/coverage/harness-coverage-map.md` | Explicit VAL-01 touchpoint mapping | ✓ VERIFIED | Maps serializer/import/undo/editor touchpoints to compare IDs and references strict output artifact. |
| `evidence/coverage/compare-strict.txt` | Strict compare proof | ✓ VERIFIED | Contains only `COMPARE PASS` rows for all listed required long-tail cases. |
| `evidence/perf/windows-vulkan-msvc/candidate/bench-eval.md` | Windows perf gate result | ✗ FAILED | Exists and wired, but `OVERALL: FAIL` due to `bench-interaction-drag` regression. |
| `evidence/perf/macos-metal/candidate/bench-eval.md` | macOS perf gate result | ✗ FAILED | Exists and wired, but blocked candidate data results in `OVERALL: FAIL`. |
| `scripts/eval_math_bench.py` | Deterministic per-case gate policy engine | ✓ VERIFIED | Defines `REQUIRED_BENCH_IDS` and threshold/rerun policy; inputs/outputs match phase perf artifacts. |
| `evidence/manual/long-tail-smoke-checklist.md` | Integrated VAL-03 checklist contract | ✓ VERIFIED | Required workflows and status vocabulary (`PASS|FAIL|BLOCKED`) present. |
| `evidence/manual/long-tail-smoke-report.md` | Manual run outcomes + overall | ✗ FAILED | Report exists and is substantive, but required rows include `FAIL/BLOCKED`, so truth not met. |
| `evidence/boundary/thin-entrypoint-boundary-finalization.md` | TRED-02 retained/removable/deferred contract | ✓ VERIFIED | Includes retained/removable/deferred sections and symbol-level rationale. |
| `docs/QUICKSTART.md` | Phase 9 closure workflow linkage | ✓ VERIFIED | Contains Phase 9 section with manual/perf/boundary artifact paths and D-03 blocking policy. |

### Key Link Verification

| From | To | Via | Status | Details |
| --- | --- | --- | --- | --- |
| `src/math_harness.c` | `mdcad_math_harness --mode compare --strict` | `mdcad_compare_cases` execution | ✓ WIRED | Compare IDs declared in source are reflected in strict output artifact with pass rows. |
| `harness-coverage-map.md` | `VAL-01` | touchpoint mapping rows | ✓ WIRED | Explicit serializer/import/undo/editor mapping present. |
| `mdcad_math_harness --mode bench` output | `scripts/eval_math_bench.py` | `bench-run1.txt` baseline/candidate inputs | ✓ WIRED | Bench files contain required `BENCH ... avg_ns=` lines consumed by evaluator contract. |
| `bench-eval.md` | `VAL-02` | `OVERALL PASS/FAIL` case table | ⚠️ WIRED-BLOCKING | Link is implemented and explicit, but both required targets evaluate to `OVERALL: FAIL`. |
| `long-tail-smoke-checklist.md` | `long-tail-smoke-report.md` | required IDs recorded with statuses | ✓ WIRED | LT-VAL03-01..04 checklist IDs present in report with `PASS/FAIL/BLOCKED`. |
| `thin-entrypoint-boundary-finalization.md` | `src/math3d.h` consumer inventory | symbol rationale mapping | ✓ WIRED | Documented symbols (`ray_from_screen`, `ray_axis_closest_t`, `ray_plane_intersect`) match code grep usage (primarily harness). |

### Data-Flow Trace (Level 4)

| Artifact | Data Variable | Source | Produces Real Data | Status |
| --- | --- | --- | --- | --- |
| `src/math_harness.c` compare suite | `mdcad_compare_cases` execution results | Actual compare functions and numeric checks | Yes (`COMPARE PASS/FAIL` emitted per case) | ✓ FLOWING |
| Perf gate outputs | benchmark averages per required bench ID | `bench-run1.txt` BENCH rows + `scripts/eval_math_bench.py` | Yes (real values parsed), but failing thresholds/blocked host | ⚠️ FLOWING-BUT-FAILING |
| Manual smoke report | LT-VAL03-* status rows | Human execution records in report | Yes (real FAIL/BLOCKED captured) | ⚠️ FLOWING-BUT-FAILING |

### Behavioral Spot-Checks

| Behavior | Command | Result | Status |
| --- | --- | --- | --- |
| Strict compare artifact includes required long-tail case coverage | Count `COMPARE PASS` and required case IDs in `compare-strict.txt` | 15 PASS rows total; 4 required long-tail IDs present | ✓ PASS |
| Windows VAL-02 decision reflects gate policy output | Search `bench-interaction-drag` and `OVERALL` in Windows `bench-eval.md` | `bench-interaction-drag` 6.034% and `OVERALL: FAIL` | ✗ FAIL |
| macOS VAL-02 required target coverage | Search evaluation status + OVERALL in macOS `bench-eval.md` | `Evaluation status: BLOCKED`; `OVERALL: FAIL` | ✗ FAIL |
| VAL-03 integrated manual outcome | Search LT-VAL03 rows + overall in smoke report | LT-VAL03-02 = FAIL, macOS rows = BLOCKED, overall = FAIL (BLOCKING) | ✗ FAIL |

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
| --- | --- | --- | --- | --- |
| `TRED-02` | 09-03 | Remaining thin-entrypoint surface is intentionally minimal, documented, and aligned to long-term boundaries. | ✓ SATISFIED | Boundary finalization doc explicitly inventories retained/deferred symbols and maps runtime consumers to `mdcad_interaction_*`. |
| `VAL-01` | 09-01 | Compare harness coverage includes long-tail touchpoints and passes strict required parity cases. | ✓ SATISFIED | `src/math_harness.c` has dedicated long-tail compare IDs; `compare-strict.txt` shows all required cases passing. |
| `VAL-02` | 09-02 | No native perf regression on macOS Metal + Windows Vulkan gates for expanded slice. | ✗ BLOCKED | Windows regression >5% and macOS candidate blocked; both bench-eval artifacts are `OVERALL: FAIL`. |
| `VAL-03` | 09-03 | Manual serializer/import/undo/editor workflows pass on native macOS + Windows paths. | ✗ BLOCKED | Manual report has required import FAIL and macOS BLOCKED rows; overall status is blocking fail. |

Orphaned requirements check (Phase 9): none. `REQUIREMENTS.md` Phase 9 mapping matches plan-declared IDs (`TRED-02`, `VAL-01`, `VAL-02`, `VAL-03`).

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
| --- | --- | --- | --- | --- |
| `evidence/manual/long-tail-smoke-report.md` | 30 | “not available” host status | ℹ️ Info | This is explicit blocker documentation, not a stub. |
| `src/math_harness.c` | 1292 | `char *end = NULL;` grep hit | ℹ️ Info | Normal parsing variable initialization; no stub behavior. |

### Human Verification Required

Not the primary status gate for this report (automated/documented evidence already proves blocking gaps).  
Still required to close gaps:

### 1. macOS native perf capture
**Test:** Run `mdcad_math_harness --mode bench --iterations 2000000` on a macOS Metal host and re-run `scripts/eval_math_bench.py` for macOS artifacts.  
**Expected:** macOS `bench-eval.md` reports valid candidate data and `OVERALL: PASS`.  
**Why human:** Requires unavailable macOS host/environment outside this session.

### 2. Integrated macOS manual smoke execution
**Test:** Execute LT-VAL03-01..04 on macOS Metal and fill report statuses/notes.  
**Expected:** All required macOS rows become `PASS`.  
**Why human:** UI workflow execution and host-specific runtime behavior cannot be programmatically verified from current Windows-only environment.

### Gaps Summary

Phase 9 did not achieve its closure goal. The parity/harness and boundary-documentation slices are implemented and evidenced, but closure is blocked by two unresolved requirement truths: (1) performance gates are failing (`VAL-02`) due to a measurable Windows regression and missing macOS candidate run, and (2) manual workflow closure is failing (`VAL-03`) due to a required import regression plus missing macOS manual execution. These are outcome-level blockers, not documentation-only gaps.

---

_Verified: 2026-03-30T10:04:49Z_  
_Verifier: the agent (gsd-verifier)_
