---
phase: 14
slug: script-io-api-undo-integration
status: draft
nyquist_compliant: false
wave_0_complete: false
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

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 14-01-01 | 01 | 1 | SCRP-04 | integration | `ctest --test-dir build-vulkan -C Release -R script_roundtrip_tests --output-on-failure` | ✅ | ⬜ pending |
| 14-01-02 | 01 | 1 | API-02 | integration/undo | `ctest --test-dir build-vulkan -C Release -R script_roundtrip_tests --output-on-failure` | ❌ W0 | ⬜ pending |
| 14-02-01 | 02 | 2 | SCRP-05 | integration/UI contract | `ctest --test-dir build-vulkan -C Release -R script_roundtrip_tests --output-on-failure` | ❌ W0 | ⬜ pending |
| 14-02-02 | 02 | 2 | API-01 | unit/integration | `ctest --test-dir build-vulkan -C Release -R script_roundtrip_tests --output-on-failure` | ❌ W0 | ⬜ pending |
| 14-03-01 | 03 | 3 | SCRP-05, API-02 | integration + manual UX gate | `ctest --test-dir build-vulkan -C Release -R script_roundtrip_tests --output-on-failure` | ❌ W0 | ⬜ pending |
| 14-03-02 | 03 | 3 | SCRP-04, SCRP-05 | checkpoint:human-verify | `ctest --test-dir build-vulkan -C Release --output-on-failure` | ✅ | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] `src/tests/script_roundtrip_tests.c` — add script transaction undo/redo atomicity tests for `API-02`.
- [ ] `src/tests/script_roundtrip_tests.c` — add parser/emit/apply tests for numeric `inputs/outputs` and `min/max/step`.
- [ ] `src/tests/script_roundtrip_tests.c` (or adjacent native test target) — verify IO live edit path uses the same transactional pipeline as `Apply Script`.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Dedicated IO window toggle and UX flow from SketchManager | SCRP-05 | Immediate-mode layout/interaction details are user-observable UX | Open IO window via SketchManager toggle next to Script Editor; verify window lifecycle and discoverability. |
| IO controls bidirectional sync with script/editor state under live auto-apply | SCRP-05, SCRP-04 | Needs interactive confirmation of real-time behavior and diagnostics | Edit inputs in IO window; confirm scene/script update, outputs refresh, and failures roll back with diagnostics. |
| One-apply-one-undo user behavior for script and IO-driven mutations | API-02 | User-level transaction feel is best validated interactively | Apply valid script/IO edit, then undo/redo and confirm full-state restore in a single step. |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 180s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
