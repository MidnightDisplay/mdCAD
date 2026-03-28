# Benchmark Evaluation: macos-metal

- Threshold: <= 5.00% slowdown per case
- Rerun policy: one rerun allowed when 5.00% < slowdown <= 8.00%
- Evaluation status: BLOCKED (required macOS Metal host unavailable in current execution environment)

| bench_id | baseline_avg_ns | candidate_avg_ns | rerun_avg_ns | effective_candidate_avg_ns | slowdown_pct | decision | note |
|---|---:|---:|---:|---:|---:|---|---|
| legacy-mat4-mul | 8.044 | - | - | - | - | FAIL | blocked-no-macos-candidate |
| cglm-mat4-mul | 7.397 | - | - | - | - | FAIL | blocked-no-macos-candidate |
| legacy-mat4-inverse | 11.304 | - | - | - | - | FAIL | blocked-no-macos-candidate |
| cglm-mat4-inv | 4.199 | - | - | - | - | FAIL | blocked-no-macos-candidate |
| legacy-screen-ray | 6.604 | - | - | - | - | FAIL | blocked-no-macos-candidate |
| cglm-screen-ray | 5.945 | - | - | - | - | FAIL | blocked-no-macos-candidate |
| bench-interaction-ray | 22.498 | - | - | - | - | FAIL | blocked-no-macos-candidate |
| bench-interaction-drag | 16.406 | - | - | - | - | FAIL | blocked-no-macos-candidate |
| bench-quat-ops | 33.752 | - | - | - | - | FAIL | blocked-no-macos-candidate |

OVERALL: FAIL
