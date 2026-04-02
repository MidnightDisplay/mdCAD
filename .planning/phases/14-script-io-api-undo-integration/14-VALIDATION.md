---
phase: 14
slug: script-io-api-undo-integration
status: complete
nyquist_compliant: true
wave_0_complete: true
created: 2026-04-01
---

# Phase 14 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | CTest + native C test executable (`script_roundtrip_tests`) |
| **Config file** | `CMakeLists.txt`, `src/CMakeLists.txt` |
| **Quick run command** | `ctest --test-dir build-vulkan -C Release -R script_roundtrip_tests --output-on-failure` |
| **Full suite command** | `ctest --test-dir build-vulkan -C Release --output-on-failure` |
| **Estimated runtime** | smoke <60s target; full suite ~60-180s |

---

## Sampling Rate

- **After every task commit:** Run `ctest --test-dir build-vulkan -C Release -R script_roundtrip_tests --output-on-failure`
- **After every plan wave:** Run `ctest --test-dir build-vulkan -C Release --output-on-failure`
- **Before `/gsd-verify-work`:** Full suite must be green
- **Max feedback latency:** 180 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Task-loop smoke (<60s) | Wave/phase gate | Wave-0 closure note | Status |
|---------|------|------|-------------|-----------|--------------------------|-----------------|--------------------|--------|
| 14-01-01 | 01 | 1 | SCRP-04 | integration | `ctest --test-dir build-vulkan -C Release -R script_roundtrip_tests --output-on-failure` | `ctest --test-dir build-vulkan -C Release --output-on-failure` | **Wave-0 closure task** for script transaction undo/redo tests | ✅ green |
| 14-01-02 | 01 | 1 | API-02 | integration/undo | `ctest --test-dir build-vulkan -C Release -R script_roundtrip_tests --output-on-failure` | `ctest --test-dir build-vulkan -C Release --output-on-failure` | Depends on 14-01-01 closure in same plan | ✅ green |
| 14-02-01 | 02 | 2 | SCRP-05 | integration/UI contract | `ctest --test-dir build-vulkan -C Release -R script_roundtrip_tests --output-on-failure` | `ctest --test-dir build-vulkan -C Release --output-on-failure` | **Wave-0-equivalent closure task** for numeric IO parse/emit/apply tests | ✅ green |
| 14-02-02 | 02 | 2 | API-01 | unit/integration | `ctest --test-dir build-vulkan -C Release -R script_roundtrip_tests --output-on-failure` | `ctest --test-dir build-vulkan -C Release --output-on-failure` | Depends on 14-02-01 closure in same plan | ✅ green |
| 14-03-01 | 03 | 3 | SCRP-05, API-02 | integration + manual UX gate | `ctest --test-dir build-vulkan -C Release -R script_roundtrip_tests --output-on-failure` | `ctest --test-dir build-vulkan -C Release --output-on-failure` | **Wave-0-equivalent closure task** for inspector/app IO contract tests before UI gate | ✅ green |
| 14-03-02 | 03 | 3 | SCRP-04, SCRP-05 | checkpoint:human-verify | `ctest --test-dir build-vulkan -C Release -R script_roundtrip_tests --output-on-failure` | `ctest --test-dir build-vulkan -C Release --output-on-failure` | Uses prior closure from 14-03-01 | ✅ green |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [x] `src/tests/script_roundtrip_tests.c` — add script transaction undo/redo atomicity tests for `API-02`.
- [x] `src/tests/script_roundtrip_tests.c` — add parser/emit/apply tests for numeric `inputs/outputs` and `min/max/step`.
- [x] `src/tests/script_roundtrip_tests.c` (or adjacent native test target) — verify IO live edit path uses the same transactional pipeline as `Apply Script`.

**Closure mapping note:** These are closed inside Phase 14 execution as Wave-0-equivalent prerequisite tasks at the start of each plan: 14-01 Task 1, 14-02 Task 1, and 14-03 Task 1. Later tasks in the same plans consume those closures before full-wave gating.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Dedicated IO window toggle and UX flow from SketchManager | SCRP-05 | Immediate-mode layout/interaction details are user-observable UX | Open IO window via SketchManager toggle next to Script Editor; verify window lifecycle and discoverability. |
| IO controls bidirectional sync with script/editor state under live auto-apply | SCRP-05, SCRP-04 | Needs interactive confirmation of real-time behavior and diagnostics | Edit inputs in IO window; confirm scene/script update, outputs refresh, and failures roll back with diagnostics. |
| One-apply-one-undo user behavior for script and IO-driven mutations | API-02 | User-level transaction feel is best validated interactively | Apply valid script/IO edit, then undo/redo and confirm full-state restore in a single step. |

---

## Validation Sign-Off

- [x] All tasks have `<automated>` verify or Wave 0 dependencies
- [x] Sampling continuity: no 3 consecutive tasks without automated verify
- [x] Wave 0 covers all MISSING references
- [x] No watch-mode flags
- [x] Feedback latency < 180s
- [x] `nyquist_compliant: true` set in frontmatter

**Approval:** complete
