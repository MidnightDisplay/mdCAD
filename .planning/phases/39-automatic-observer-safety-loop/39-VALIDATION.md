---
phase: 39
slug: automatic-observer-safety-loop
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-04-27
---

# Phase 39 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | CTest + native C test binaries |
| **Config file** | `CMakeLists.txt`, `src/CMakeLists.txt` |
| **Quick run command** | `ctest --test-dir build-vulkan -C Release -R "jsonl_flat_observer_(manual_refresh_test|inspector_contract_test|auto_safety_test)" --output-on-failure` |
| **Full suite command** | `ctest --test-dir build-vulkan -C Release --output-on-failure` |
| **Estimated runtime** | ~90 seconds |

---

## Sampling Rate

- **After every task commit:** Run `ctest --test-dir build-vulkan -C Release -R "jsonl_flat_observer_(manual_refresh_test|inspector_contract_test|auto_safety_test)" --output-on-failure`
- **After every plan wave:** Run `ctest --test-dir build-vulkan -C Release -R "jsonl_(observer_state_test|flat_observer_manual_refresh_test|flat_observer_inspector_contract_test|flat_observer_auto_safety_test)" --output-on-failure`
- **Before `/gsd-verify-work`:** Full suite must be green
- **Max feedback latency:** 120 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 39-01-01 | 01 | 1 | OBSF-04 | unit/integration | `ctest --test-dir build-vulkan -C Release -R "jsonl_flat_observer_auto_safety_test" --output-on-failure` | ❌ W0 | ⬜ pending |
| 39-01-02 | 01 | 1 | OBSF-04 | contract | `ctest --test-dir build-vulkan -C Release -R "jsonl_flat_observer_inspector_contract_test" --output-on-failure` | ✅ | ⬜ pending |
| 39-02-01 | 02 | 2 | OBSF-04 | integration | `ctest --test-dir build-vulkan -C Release -R "jsonl_flat_observer_manual_refresh_test|jsonl_flat_observer_auto_safety_test" --output-on-failure` | ⚠️ partial | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] `src/tests/jsonl_flat_observer_auto_safety_test.c` — add dedicated OBSF-04 auto-loop safety coverage
- [ ] `src/CMakeLists.txt` — add test target and CTest registration for `jsonl_flat_observer_auto_safety_test`

---

## Manual-Only Verifications

All phase behaviors have automated verification.

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 120s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
