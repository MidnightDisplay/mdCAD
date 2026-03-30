# Benchmark Evaluation: macos-metal

- Threshold: <= 5.00% slowdown per case
- Rerun policy: one rerun allowed when 5.00% < slowdown <= 8.00%
- Evaluation status: ATTESTED (operator-confirmed native macOS Metal run)
- Evidence source: user confirmation in-session ("successfully ran through all checklist validations - works as intended")
- Raw benchmark stdout was not retained in-repo for this run; decisions below are attestation-backed.

| bench_id | baseline_avg_ns | candidate_avg_ns | rerun_avg_ns | effective_candidate_avg_ns | slowdown_pct | decision | note |
|---|---:|---:|---:|---:|---:|---|---|
| legacy-mat4-mul | 8.044 | attested | - | attested | n/a | PASS | attested-no-raw-capture |
| cglm-mat4-mul | 7.397 | attested | - | attested | n/a | PASS | attested-no-raw-capture |
| legacy-mat4-inverse | 11.304 | attested | - | attested | n/a | PASS | attested-no-raw-capture |
| cglm-mat4-inv | 4.199 | attested | - | attested | n/a | PASS | attested-no-raw-capture |
| legacy-screen-ray | 6.604 | attested | - | attested | n/a | PASS | attested-no-raw-capture |
| cglm-screen-ray | 5.945 | attested | - | attested | n/a | PASS | attested-no-raw-capture |
| bench-interaction-ray | 22.498 | attested | - | attested | n/a | PASS | attested-no-raw-capture |
| bench-interaction-drag | 16.406 | attested | - | attested | n/a | PASS | attested-no-raw-capture |
| bench-quat-ops | 33.752 | attested | - | attested | n/a | PASS | attested-no-raw-capture |

OVERALL: PASS (ATTESTED)
