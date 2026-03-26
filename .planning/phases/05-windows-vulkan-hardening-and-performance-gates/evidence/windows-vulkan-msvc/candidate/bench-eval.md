# Benchmark Evaluation: windows-vulkan-msvc

- Threshold: <= 5.00% slowdown per case
- Rerun policy: one rerun allowed when 5.00% < slowdown <= 8.00%

| bench_id | baseline_avg_ns | candidate_avg_ns | rerun_avg_ns | effective_candidate_avg_ns | slowdown_pct | decision | note |
|---|---:|---:|---:|---:|---:|---|---|
| legacy-mat4-mul | 3.945 | 3.856 | - | 3.856 | -2.256% | PASS | - |
| cglm-mat4-mul | 3.840 | 3.745 | - | 3.745 | -2.474% | PASS | - |
| legacy-mat4-inverse | 13.982 | 13.883 | - | 13.883 | -0.708% | PASS | - |
| cglm-mat4-inv | 4.769 | 4.817 | - | 4.817 | 1.007% | PASS | - |
| legacy-screen-ray | 6.731 | 6.773 | - | 6.773 | 0.624% | PASS | - |
| cglm-screen-ray | 29.512 | 29.564 | - | 29.564 | 0.176% | PASS | - |
| bench-interaction-ray | 25.396 | 26.459 | - | 26.459 | 4.186% | PASS | - |
| bench-interaction-drag | 100.857 | 98.947 | - | 98.947 | -1.894% | PASS | - |
| bench-quat-ops | 50.614 | 50.300 | - | 50.300 | -0.620% | PASS | - |

OVERALL: PASS
