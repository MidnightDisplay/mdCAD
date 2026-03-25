#!/usr/bin/env python3
"""Evaluate Phase 5 benchmark artifacts against no-regression policy."""

import argparse
import re
import sys
from pathlib import Path

BENCH_LINE_RE = re.compile(r"^BENCH (?P<id>\S+) .* avg_ns=(?P<avg>[0-9]+(?:\.[0-9]+)?)$")
REQUIRED_BENCH_IDS = [
    "legacy-mat4-mul",
    "cglm-mat4-mul",
    "legacy-mat4-inverse",
    "cglm-mat4-inv",
    "legacy-screen-ray",
    "cglm-screen-ray",
    "bench-interaction-ray",
    "bench-interaction-drag",
    "bench-quat-ops",
]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Evaluate benchmark artifacts with deterministic per-case slowdown gates.",
    )
    parser.add_argument("--baseline", required=True, help="Path to baseline bench-run output")
    parser.add_argument("--candidate", required=True, help="Path to candidate bench-run output")
    parser.add_argument("--output", required=True, help="Path to markdown output report")
    parser.add_argument("--label", required=True, help="Target label, e.g. macos-metal")
    parser.add_argument("--rerun", help="Optional rerun bench-run output used for marginal misses")
    parser.add_argument(
        "--threshold-pct",
        type=float,
        default=5.0,
        help="Slowdown threshold percentage for PASS/FAIL (default: 5.0)",
    )
    parser.add_argument(
        "--rerun-threshold-pct",
        type=float,
        default=8.0,
        help="Upper bound for one-rerun policy (default: 8.0)",
    )
    return parser.parse_args()


def parse_bench_file(path: Path) -> dict[str, float]:
    values: dict[str, float] = {}
    try:
        lines = path.read_text(encoding="utf-8").splitlines()
    except FileNotFoundError:
        raise SystemExit(f"Missing benchmark file: {path}")

    for line in lines:
        match = BENCH_LINE_RE.match(line.strip())
        if not match:
            continue
        values[match.group("id")] = float(match.group("avg"))
    return values


def ensure_required_ids(values: dict[str, float], label: str) -> None:
    ids = set(values.keys())
    required = set(REQUIRED_BENCH_IDS)
    missing = sorted(required - ids)
    extra = sorted(ids - required)
    if missing or extra:
        problems: list[str] = []
        if missing:
            problems.append(f"missing={','.join(missing)}")
        if extra:
            problems.append(f"extra={','.join(extra)}")
        raise SystemExit(f"{label} bench IDs mismatch: {'; '.join(problems)}")


def compute_slowdown(candidate: float, baseline: float) -> float:
    slowdown_pct = ((candidate - baseline) / baseline) * 100
    return slowdown_pct


def render_report(
    label: str,
    baseline: dict[str, float],
    candidate: dict[str, float],
    rerun: dict[str, float] | None,
    threshold_pct: float,
    rerun_threshold_pct: float,
) -> tuple[list[str], bool]:
    lines = [
        f"# Benchmark Evaluation: {label}",
        "",
        f"- Threshold: <= {threshold_pct:.2f}% slowdown per case",
        f"- Rerun policy: one rerun allowed when {threshold_pct:.2f}% < slowdown <= {rerun_threshold_pct:.2f}%",
        "",
        "| bench_id | baseline_avg_ns | candidate_avg_ns | rerun_avg_ns | effective_candidate_avg_ns | slowdown_pct | decision | note |",
        "|---|---:|---:|---:|---:|---:|---|---|",
    ]
    overall_pass = True

    for bench_id in REQUIRED_BENCH_IDS:
        base = baseline[bench_id]
        cand = candidate[bench_id]
        if base <= 0.0:
            raise SystemExit(f"Invalid baseline avg_ns for {bench_id}: {base}")

        slowdown_pct = compute_slowdown(cand, base)
        effective = cand
        rerun_value = "-"
        note = ""
        decision = "PASS"

        if slowdown_pct > threshold_pct:
            if rerun and threshold_pct < slowdown_pct <= rerun_threshold_pct:
                rerun_value_num = rerun[bench_id]
                rerun_value = f"{rerun_value_num:.3f}"
                effective = min(cand, rerun_value_num)
                note = "rerun-applied"
                slowdown_pct = compute_slowdown(effective, base)
            if slowdown_pct > threshold_pct:
                decision = "FAIL"
                overall_pass = False

        lines.append(
            "| {bench_id} | {base:.3f} | {cand:.3f} | {rerun_value} | {effective:.3f} | {slowdown:.3f}% | {decision} | {note} |".format(
                bench_id=bench_id,
                base=base,
                cand=cand,
                rerun_value=rerun_value,
                effective=effective,
                slowdown=slowdown_pct,
                decision=decision,
                note=note or "-",
            )
        )

    lines.extend(
        [
            "",
            f"OVERALL: {'PASS' if overall_pass else 'FAIL'}",
        ]
    )
    return lines, overall_pass


def main() -> int:
    args = parse_args()
    baseline_path = Path(args.baseline)
    candidate_path = Path(args.candidate)
    output_path = Path(args.output)
    rerun_path = Path(args.rerun) if args.rerun else None

    baseline = parse_bench_file(baseline_path)
    candidate = parse_bench_file(candidate_path)
    rerun_values = parse_bench_file(rerun_path) if rerun_path else None

    ensure_required_ids(baseline, "baseline")
    ensure_required_ids(candidate, "candidate")
    if rerun_values is not None:
        ensure_required_ids(rerun_values, "rerun")

    report_lines, overall_pass = render_report(
        label=args.label,
        baseline=baseline,
        candidate=candidate,
        rerun=rerun_values,
        threshold_pct=args.threshold_pct,
        rerun_threshold_pct=args.rerun_threshold_pct,
    )

    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text("\n".join(report_lines) + "\n", encoding="utf-8")
    print(f"OVERALL: {'PASS' if overall_pass else 'FAIL'}")
    return 0 if overall_pass else 1


if __name__ == "__main__":
    sys.exit(main())
