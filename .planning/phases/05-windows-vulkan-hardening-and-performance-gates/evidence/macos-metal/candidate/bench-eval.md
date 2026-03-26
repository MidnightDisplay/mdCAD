# Benchmark Evaluation: macos-metal

- Threshold: <= 5.00% slowdown per case
- Rerun policy: one rerun allowed when 5.00% < slowdown <= 8.00%

| bench_id | baseline_avg_ns | candidate_avg_ns | rerun_avg_ns | effective_candidate_avg_ns | slowdown_pct | decision | note |
|---|---:|---:|---:|---:|---:|---|---|
| legacy-mat4-mul | 8.044 | 7.458 | - | 7.458 | -7.285% | PASS | - |
| cglm-mat4-mul | 7.397 | 7.153 | - | 7.153 | -3.299% | PASS | - |
| legacy-mat4-inverse | 11.304 | 10.901 | - | 10.901 | -3.565% | PASS | - |
| cglm-mat4-inv | 4.199 | 4.102 | - | 4.102 | -2.310% | PASS | - |
| legacy-screen-ray | 6.604 | 6.494 | - | 6.494 | -1.666% | PASS | - |
| cglm-screen-ray | 5.945 | 5.750 | - | 5.750 | -3.280% | PASS | - |
| bench-interaction-ray | 22.498 | 21.948 | - | 21.948 | -2.445% | PASS | - |
| bench-interaction-drag | 16.406 | 16.797 | - | 16.797 | 2.383% | PASS | - |
| bench-quat-ops | 33.752 | 34.155 | - | 34.155 | 1.194% | PASS | - |

OVERALL: PASS
