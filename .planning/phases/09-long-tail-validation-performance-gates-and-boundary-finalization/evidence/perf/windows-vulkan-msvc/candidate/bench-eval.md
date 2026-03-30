# Benchmark Evaluation: windows-vulkan-msvc

- Threshold: <= 5.00% slowdown per case
- Rerun policy: one rerun allowed when 5.00% < slowdown <= 8.00%

| bench_id | baseline_avg_ns | candidate_avg_ns | rerun_avg_ns | effective_candidate_avg_ns | slowdown_pct | decision | note |
|---|---:|---:|---:|---:|---:|---|---|
| legacy-mat4-mul | 3.945 | 3.855 | - | 3.855 | -2.281% | PASS | - |
| cglm-mat4-mul | 3.840 | 3.833 | - | 3.833 | -0.182% | PASS | - |
| legacy-mat4-inverse | 13.982 | 14.361 | - | 14.361 | 2.711% | PASS | - |
| cglm-mat4-inv | 4.769 | 4.966 | - | 4.966 | 4.131% | PASS | - |
| legacy-screen-ray | 6.731 | 6.816 | - | 6.816 | 1.263% | PASS | - |
| cglm-screen-ray | 29.512 | 29.700 | - | 29.700 | 0.637% | PASS | - |
| bench-interaction-ray | 25.396 | 25.940 | - | 25.940 | 2.142% | PASS | - |
| bench-interaction-drag | 100.857 | 106.943 | 103.555 | 103.555 | 2.675% | PASS | rerun-applied |
| bench-quat-ops | 50.614 | 51.430 | - | 51.430 | 1.612% | PASS | - |

OVERALL: PASS
