---
phase: 29
slug: active-sketch-line-gizmo-endpoint-authority
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-04-09
---

# Phase 29 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Native C tests via CMake + CTest |
| **Config file** | `CMakeLists.txt`, `src/CMakeLists.txt` |
| **Quick run command** | `ctest --test-dir build --output-on-failure -R "endpoint_pick\|scene_solver_drag"` |
| **Full suite command** | `ctest --test-dir build --output-on-failure` |
| **Estimated runtime** | ~20-90 seconds |

---

## Sampling Rate

- **After every task commit:** Run `endpoint_pick|scene_solver_drag` targeted slice.
- **After every plan wave:** Run full Phase 29-targeted regression slice.
- **Before `/gsd-verify-work`:** Full suite must be green.
- **Max feedback latency:** <120 seconds.

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 29-01-01 | 01 | 1 | GZM-01, GZM-03 | integration | `ctest --test-dir build --output-on-failure -R endpoint_pick` | ✅ | ⬜ pending |
| 29-01-02 | 01 | 1 | GZM-02, GZM-03 | integration | `ctest --test-dir build --output-on-failure -R "endpoint_pick\|scene_solver_drag"` | ✅ | ⬜ pending |
| 29-01-03 | 01 | 1 | GZM-04 | integration | `ctest --test-dir build --output-on-failure -R endpoint_pick` | ✅ | ⬜ pending |
| 29-02-01 | 02 | 2 | GZM-01, GZM-02, GZM-03, GZM-04 | deterministic rerun | `ctest --test-dir build --output-on-failure -R "endpoint_pick\|scene_solver_drag" && ctest --test-dir build --output-on-failure -R "endpoint_pick\|scene_solver_drag"` | ✅ | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠ flaky*

---

## Wave 0 Requirements

- [ ] Extend `src/tests/endpoint_pick_test.c` with Phase 29 midpoint anchoring, mixed-selection guardrail, and grouped undo/redo coverage.
- [ ] Extend `src/tests/scene_solver_drag_test.c` for active-sketch line endpoint-authority drag integration and projected-delta compatibility.
- [ ] If a new grouped undo command is added, add command-level replay roundtrip assertions in undo-focused tests.

---

## Manual-Only Verifications

All Phase 29 behaviors are expected to have automated verification through native test suites.

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify commands or explicit Wave 0 dependencies
- [ ] Sampling continuity preserved (no long no-test streaks)
- [ ] Wave 0 gaps resolved
- [ ] No watch-mode flags
- [ ] Feedback latency under target
- [ ] `nyquist_compliant: true` set in frontmatter before closure

**Approval:** pending

