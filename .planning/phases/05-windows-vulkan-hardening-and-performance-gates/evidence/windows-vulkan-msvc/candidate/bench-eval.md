# Benchmark Evaluation: windows-vulkan-msvc

- Threshold: <= 5.00% slowdown per case
- Rerun policy: one rerun allowed when 5.00% < slowdown <= 8.00%

| bench_id | baseline_avg_ns | candidate_avg_ns | rerun_avg_ns | effective_candidate_avg_ns | slowdown_pct | decision | note |
|---|---:|---:|---:|---:|---:|---|---|
| legacy-mat4-mul | 7.397 | 7.397 | - | 7.397 | 0.000% | PASS | - |
| cglm-mat4-mul | 7.202 | 7.190 | - | 7.190 | -0.167% | PASS | - |
| legacy-mat4-inverse | 10.950 | 10.950 | - | 10.950 | 0.000% | PASS | - |
| cglm-mat4-inv | 4.089 | 4.150 | - | 4.150 | 1.492% | PASS | - |
| legacy-screen-ray | 6.506 | 6.506 | - | 6.506 | 0.000% | PASS | - |
| cglm-screen-ray | 5.798 | 5.750 | - | 5.750 | -0.828% | PASS | - |
| bench-interaction-ray | 21.997 | 21.948 | - | 21.948 | -0.223% | PASS | - |
| bench-interaction-drag | 16.406 | 16.296 | - | 16.296 | -0.670% | PASS | - |
| bench-quat-ops | 34.302 | 33.447 | - | 33.447 | -2.493% | PASS | - |

OVERALL: PASS
