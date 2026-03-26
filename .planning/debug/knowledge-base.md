# GSD Debug Knowledge Base

Resolved debug sessions. Used by `gsd-debugger` to surface known-pattern hypotheses at the start of new investigations.

---

## phase5-quat-ops-gate-fail — Phase 5 quat gate fail was capture-noise from low iteration count
- **Date:** 2026-03-26
- **Error patterns:** gate fail, bench-quat-ops, slowdown criterion, OVERALL FAIL, no runtime error, performance regression row
- **Root cause:** Phase 5 gate policy/tooling intentionally treats bench-quat-ops as a required case and fails overall when any required case fails; the observed quaternion FAIL came from noisy single-shot 20k-iteration captures where short benchmark durations are highly variable, not from missing baseline or invalid row computation.
- **Fix:** Updated Phase 5 QUICKSTART performance-gate capture instructions to use 2,000,000 iterations (instead of 20,000) and documented rationale (reduce timer-noise outliers), while keeping strict per-case gate policy unchanged.
- **Files changed:** docs/QUICKSTART.md
---

