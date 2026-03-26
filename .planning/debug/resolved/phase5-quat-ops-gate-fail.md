---
status: resolved
trigger: "Investigate issue: phase5-quat-ops-gate-fail"
created: 2026-03-26T16:33:09.6432116+00:00
updated: 2026-03-26T16:49:52.4756727+00:00
---

## Current Focus
hypothesis: Confirmed and closed — methodology noise at 20k iterations caused prior false FAIL; updated 2M-iteration workflow validated by user rerun.
test: User-provided human verification rerun with higher iterations.
expecting: bench-quat-ops row passes and OVERALL result is PASS.
next_action: Archive this resolved session and update knowledge base.

## Symptoms
expected: Gate decision should be logically justified; if quat case is invalid/non-comparable, result should not incorrectly force FAIL.
actual: Evaluator output includes a FAIL row for quaternion benchmark due to slowdown criterion. User reports line: `bench-quat-ops\t46.899\t58.374\t-\t58.374\t24.467%\tFAIL`
errors: No explicit runtime error; just gate FAIL.
reproduction: User followed provided Windows checklist commands exactly. Baseline and candidate captured from two sequential runs, then evaluated with `py -3 scripts\\eval_math_bench.py --label windows-vulkan-msvc --baseline <baseline bench-run1.txt> --candidate <candidate bench-run1.txt> --output <candidate bench-eval.md>`.
started: First real Windows MSVC Vulkan run according to Phase 5 instructions.

## Eliminated

## Evidence
- timestamp: 2026-03-26T16:34:10+00:00
  checked: .planning/debug/knowledge-base.md
  found: Knowledge base file is missing.
  implication: No known-pattern shortcut available; continue first-principles investigation.

- timestamp: 2026-03-26T16:35:00+00:00
  checked: scripts/eval_math_bench.py
  found: REQUIRED_BENCH_IDS includes bench-quat-ops; evaluator enforces exact ID set and fails overall if any required case exceeds threshold.
  implication: Tool currently treats quat benchmark as mandatory gate input and mandatory pass criterion.

- timestamp: 2026-03-26T16:35:40+00:00
  checked: docs/QUICKSTART.md (Phase 5 sections)
  found: Phase 5 instructions use eval_math_bench.py with no exclusion for quat and state per-case no-regression threshold.
  implication: Process docs align with current evaluator behavior; any single FAIL row currently causes OVERALL FAIL.

- timestamp: 2026-03-26T16:37:30+00:00
  checked: windows-vulkan-msvc baseline/candidate bench artifacts + provenance
  found: bench-quat-ops exists in both files (baseline 46.899 ns, candidate 58.374 ns); both artifacts share identical source_commit and capture_cmd provenance.
  implication: Reported FAIL row is computed from a valid benchmark ID with same-commit captures; not caused by missing ID or commit mismatch.

- timestamp: 2026-03-26T16:38:20+00:00
  checked: src/math_harness.c bench case list and evaluator REQUIRED_BENCH_IDS
  found: bench-quat-ops is explicitly included in harness bench cases and in evaluator mandatory gate set.
  implication: Quaternion bench is intentionally part of current gate policy/tooling, not an accidental unknown row.

- timestamp: 2026-03-26T16:39:10+00:00
  checked: phase planning/policy docs (05-RESEARCH.md, 05-03-GATE-STATUS.md)
  found: PERF gate rule says "use all current harness bench IDs as gating set"; final decision rule is HOLD if any required gate FAIL.
  implication: Current policy does not allow declaring PASS when one required case fails.

- timestamp: 2026-03-26T16:42:00+00:00
  checked: repeated local Windows Vulkan bench runs at 20k iterations (20+ runs)
  found: bench-quat-ops and several other cases show high run-to-run variance at 20k; bench-quat-ops observed min 46.875 ns and max 60.278 ns across 80 runs (same binary/session), and sequential-run >5% slowdowns occur frequently across multiple IDs.
  implication: Single baseline/candidate captures at 20k are noisy enough to trigger false gate FAILs unrelated to code changes.

- timestamp: 2026-03-26T16:44:20+00:00
  checked: repeated local Windows Vulkan bench runs at 2,000,000 iterations
  found: bench-quat-ops sequential pair slowdowns were 0.428%, -2.079%, 4.600%, -2.596% (all <= 5%); >5% frequency dropped materially for key benches versus 20k.
  implication: Increasing iterations stabilizes timing and directly mitigates methodology-driven false FAILs.

- timestamp: 2026-03-26T16:49:20+00:00
  checked: docs/QUICKSTART.md
  found: Phase 5 native performance gate capture commands were updated from --iterations 20000 to --iterations 2000000 for both macOS and windows-vulkan-msvc baseline/candidate capture and provenance examples, with note explaining timer-noise reduction.
  implication: Process documentation now directs reproducible higher-confidence captures, reducing false FAIL likelihood without weakening gate policy.

- timestamp: 2026-03-26T16:49:45+00:00
  checked: scripts/eval_math_bench.py execution on existing artifacts
  found: Evaluator still returns OVERALL: FAIL on current previously captured 20k artifacts.
  implication: No tool logic regression introduced; fix is process-level and requires fresh evidence capture to change gate outcome.

## Resolution
root_cause:
  Phase 5 gate policy/tooling intentionally treats bench-quat-ops as a required case and fails overall when any required case fails; the observed quaternion FAIL came from noisy single-shot 20k-iteration captures where short benchmark durations are highly variable, not from missing baseline or invalid row computation.
fix:
  Updated Phase 5 QUICKSTART performance-gate capture instructions to use 2,000,000 iterations (instead of 20,000) and documented rationale (reduce timer-noise outliers), while keeping strict per-case gate policy unchanged.
verification:
  Verified bench-quat-ops is present and correctly computed in baseline/candidate/evaluator; verified policy requires all required cases to pass; verified repeated 20k runs are unstable and repeated 2M runs are materially more stable; verified evaluator behavior unchanged on existing artifacts; human verification confirmed rerun output `bench-quat-ops ... -0.620% | PASS` and `OVERALL: PASS`.
files_changed:
  - docs/QUICKSTART.md
